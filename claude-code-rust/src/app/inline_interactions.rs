// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::{
    App, FocusTarget, InvalidationLevel, MessageBlock, ToolCallInfo, permissions, questions,
};
use crossterm::event::{KeyCode, KeyEvent};

pub(super) fn focused_interaction_id(app: &App) -> Option<&str> {
    app.pending_interaction_ids.first().map(String::as_str)
}

fn interaction_id_is_valid(app: &App, tool_id: &str) -> bool {
    let Some((mi, bi)) = app.lookup_tool_call(tool_id) else {
        return false;
    };
    matches!(
        app.messages.get(mi).and_then(|msg| msg.blocks.get(bi)),
        Some(MessageBlock::ToolCall(tc))
            if tc.pending_permission.is_some() || tc.pending_question.is_some()
    )
}

pub(super) fn focused_interaction(app: &App) -> Option<&ToolCallInfo> {
    let tool_id = focused_interaction_id(app)?;
    let (mi, bi) = app.tool_call_index.get(tool_id).copied()?;
    let MessageBlock::ToolCall(tc) = app.messages.get(mi)?.blocks.get(bi)? else {
        return None;
    };
    Some(tc.as_ref())
}

pub(super) fn get_focused_interaction_tc(app: &mut App) -> Option<&mut ToolCallInfo> {
    let tool_id = focused_interaction_id(app)?;
    let (mi, bi) = app.tool_call_index.get(tool_id).copied()?;
    match app.messages.get_mut(mi)?.blocks.get_mut(bi)? {
        MessageBlock::ToolCall(tc)
            if tc.pending_permission.is_some() || tc.pending_question.is_some() =>
        {
            Some(tc.as_mut())
        }
        _ => None,
    }
}

pub(super) fn focused_interaction_dirty_idx(app: &App) -> Option<(usize, usize)> {
    focused_interaction_id(app).and_then(|tool_id| app.lookup_tool_call(tool_id))
}

pub(super) fn invalidate_if_changed(
    app: &mut App,
    dirty_idx: Option<(usize, usize)>,
    changed: bool,
) {
    if changed && let Some((mi, bi)) = dirty_idx {
        app.sync_render_cache_slot(mi, bi);
        app.recompute_message_retained_bytes(mi);
        app.invalidate_layout(InvalidationLevel::MessageChanged(mi));
    }
}

pub(super) fn set_interaction_focused(app: &mut App, queue_index: usize, focused: bool) {
    let Some(tool_id) = app.pending_interaction_ids.get(queue_index) else {
        return;
    };
    let Some((mi, bi)) = app.tool_call_index.get(tool_id).copied() else {
        return;
    };
    let mut invalidated = false;
    if let Some(msg) = app.messages.get_mut(mi)
        && let Some(MessageBlock::ToolCall(tc)) = msg.blocks.get_mut(bi)
    {
        let tc = tc.as_mut();
        if let Some(ref mut perm) = tc.pending_permission
            && perm.focused != focused
        {
            perm.focused = focused;
            tc.mark_tool_call_layout_dirty();
            invalidated = true;
        }
        if let Some(ref mut question) = tc.pending_question
            && question.focused != focused
        {
            question.focused = focused;
            tc.mark_tool_call_layout_dirty();
            invalidated = true;
        }
    }
    if invalidated {
        app.sync_render_cache_slot(mi, bi);
        app.invalidate_layout(InvalidationLevel::MessageChanged(mi));
    }
}

pub(super) fn focused_interaction_is_active(app: &App) -> bool {
    focused_interaction(app).is_some_and(|tc| {
        tc.pending_permission.as_ref().is_some_and(|permission| permission.focused)
            || tc.pending_question.as_ref().is_some_and(|question| question.focused)
    })
}

pub(super) fn focus_next_inline_interaction(app: &mut App) {
    normalize_pending_interaction_queue(app);
    set_interaction_focused(app, 0, true);
    if app.pending_interaction_ids.is_empty() {
        app.release_focus_target(FocusTarget::Permission);
    } else {
        app.claim_focus_target(FocusTarget::Permission);
    }
}

pub(super) fn normalize_pending_interaction_queue(app: &mut App) {
    let previous = std::mem::take(&mut app.pending_interaction_ids);
    let previous_order = previous.clone();
    let mut queue = Vec::with_capacity(previous.len());
    for id in previous {
        if interaction_id_is_valid(app, &id) {
            queue.push(id);
        }
    }
    let changed = queue != previous_order;
    app.pending_interaction_ids = queue;

    if app.pending_interaction_ids.is_empty() {
        app.release_focus_target(FocusTarget::Permission);
        app.normalize_focus_stack();
        return;
    }

    if changed {
        for idx in 0..app.pending_interaction_ids.len() {
            set_interaction_focused(app, idx, idx == 0);
        }
    }
    app.claim_focus_target(FocusTarget::Permission);
    app.normalize_focus_stack();
}

pub(super) fn pop_next_valid_interaction_id(app: &mut App) -> Option<String> {
    normalize_pending_interaction_queue(app);
    (!app.pending_interaction_ids.is_empty()).then(|| app.pending_interaction_ids.remove(0))
}

pub(super) fn handle_interaction_focus_cycle(
    app: &mut App,
    key: KeyEvent,
    interaction_has_focus: bool,
    blocks_vertical_navigation: bool,
) -> Option<bool> {
    if !interaction_has_focus {
        return None;
    }
    if !matches!(key.code, KeyCode::Up | KeyCode::Down) {
        return None;
    }
    if app.pending_interaction_ids.len() <= 1 {
        if blocks_vertical_navigation {
            return None;
        }
        return Some(true);
    }

    set_interaction_focused(app, 0, false);

    if key.code == KeyCode::Down {
        let first = app.pending_interaction_ids.remove(0);
        app.pending_interaction_ids.push(first);
    } else {
        let Some(last) = app.pending_interaction_ids.pop() else {
            return Some(false);
        };
        app.pending_interaction_ids.insert(0, last);
    }

    set_interaction_focused(app, 0, true);
    app.viewport.engage_auto_scroll();
    Some(true)
}

pub(super) fn handle_inline_interaction_key(app: &mut App, key: KeyEvent) -> bool {
    normalize_pending_interaction_queue(app);
    let interaction_has_focus = focused_interaction_is_active(app);
    let has_question = questions::has_focused_question(app);
    let plan_approval = permissions::focused_permission_is_plan_approval(app);

    if let Some(consumed) = handle_interaction_focus_cycle(
        app,
        key,
        interaction_has_focus,
        has_question || plan_approval,
    ) {
        return consumed;
    }
    if has_question {
        return questions::handle_question_key(app, key, interaction_has_focus).unwrap_or(false);
    }
    permissions::handle_permission_key(app, key, interaction_has_focus)
}
