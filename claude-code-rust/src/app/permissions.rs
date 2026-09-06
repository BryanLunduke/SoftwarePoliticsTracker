// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::inline_interactions::{
    focus_next_inline_interaction, focused_interaction, focused_interaction_dirty_idx,
    get_focused_interaction_tc, invalidate_if_changed, pop_next_valid_interaction_id,
};
use super::{App, InvalidationLevel, MessageBlock};
use crate::agent::model;
use crate::agent::model::PermissionOptionKind;
use crossterm::event::{KeyCode, KeyEvent, KeyModifiers};

fn focused_permission(app: &App) -> Option<&crate::app::InlinePermission> {
    focused_interaction(app)?.pending_permission.as_ref()
}

fn focused_option_index_by_kind(app: &App, kind: PermissionOptionKind) -> Option<usize> {
    focused_option_index_where(app, |opt| opt.kind == kind)
}

fn focused_option_index_where<F>(app: &App, mut predicate: F) -> Option<usize>
where
    F: FnMut(&model::PermissionOption) -> bool,
{
    focused_permission(app)?.options.iter().position(&mut predicate)
}

fn is_ctrl_shortcut(modifiers: KeyModifiers) -> bool {
    modifiers.contains(KeyModifiers::CONTROL) && !modifiers.contains(KeyModifiers::ALT)
}

fn is_ctrl_char_shortcut(key: KeyEvent, expected: char) -> bool {
    is_ctrl_shortcut(key.modifiers)
        && matches!(key.code, KeyCode::Char(c) if c.eq_ignore_ascii_case(&expected))
}

fn normalized_option_tokens(option: &model::PermissionOption) -> String {
    let mut out = String::new();
    for ch in option.name.chars().chain(option.option_id.chars()) {
        if ch.is_ascii_alphanumeric() {
            out.push(ch.to_ascii_lowercase());
        }
    }
    out
}

fn option_tokens(option: &model::PermissionOption) -> (bool, bool, bool, bool) {
    let tokens = normalized_option_tokens(option);
    let allow_like =
        tokens.contains("allow") || tokens.contains("accept") || tokens.contains("approve");
    let reject_like =
        tokens.contains("reject") || tokens.contains("deny") || tokens.contains("disallow");
    let persistent_like = tokens.contains("always")
        || tokens.contains("dontask")
        || tokens.contains("remember")
        || tokens.contains("persist")
        || tokens.contains("bypasspermissions");
    let session_like = tokens.contains("session") || tokens.contains("onesession");
    (allow_like, reject_like, persistent_like, session_like)
}

fn option_is_allow_once_fallback(option: &model::PermissionOption) -> bool {
    let (allow_like, reject_like, persistent_like, session_like) = option_tokens(option);
    allow_like && !reject_like && !persistent_like && !session_like
}

fn option_is_allow_always_fallback(option: &model::PermissionOption) -> bool {
    let (allow_like, reject_like, persistent_like, _) = option_tokens(option);
    allow_like && !reject_like && persistent_like
}

fn option_is_allow_non_once_fallback(option: &model::PermissionOption) -> bool {
    let (allow_like, reject_like, persistent_like, session_like) = option_tokens(option);
    allow_like && !reject_like && (persistent_like || session_like)
}

fn option_is_reject_once_fallback(option: &model::PermissionOption) -> bool {
    let (allow_like, reject_like, persistent_like, _) = option_tokens(option);
    reject_like && !allow_like && !persistent_like
}

fn option_is_reject_fallback(option: &model::PermissionOption) -> bool {
    let (allow_like, reject_like, _, _) = option_tokens(option);
    reject_like && !allow_like
}

pub(super) fn focused_permission_is_plan_approval(app: &App) -> bool {
    focused_permission(app).is_some_and(|pending| {
        pending.options.iter().any(|opt| {
            matches!(opt.kind, PermissionOptionKind::PlanApprove | PermissionOptionKind::PlanReject)
        })
    })
}

fn move_permission_option_left(app: &mut App) {
    let dirty_idx = focused_interaction_dirty_idx(app);
    let mut changed = false;
    if let Some(tc) = get_focused_interaction_tc(app)
        && let Some(ref mut permission) = tc.pending_permission
    {
        let next = permission.selected_index.saturating_sub(1);
        if next != permission.selected_index {
            permission.selected_index = next;
            tc.mark_tool_call_layout_dirty();
            changed = true;
        }
    }
    invalidate_if_changed(app, dirty_idx, changed);
}

fn move_permission_option_right(app: &mut App, option_count: usize) {
    let dirty_idx = focused_interaction_dirty_idx(app);
    let mut changed = false;
    if let Some(tc) = get_focused_interaction_tc(app)
        && let Some(ref mut permission) = tc.pending_permission
        && permission.selected_index + 1 < option_count
    {
        permission.selected_index += 1;
        tc.mark_tool_call_layout_dirty();
        changed = true;
    }
    invalidate_if_changed(app, dirty_idx, changed);
}

fn handle_permission_option_keys(
    app: &mut App,
    key: KeyEvent,
    interaction_has_focus: bool,
    option_count: usize,
    plan_approval: bool,
) -> Option<bool> {
    if !interaction_has_focus {
        return None;
    }
    match key.code {
        KeyCode::Left if option_count > 0 => {
            move_permission_option_left(app);
            Some(true)
        }
        KeyCode::Right if option_count > 0 => {
            move_permission_option_right(app, option_count);
            Some(true)
        }
        KeyCode::Up if plan_approval && option_count > 0 => {
            move_permission_option_left(app);
            Some(true)
        }
        KeyCode::Down if plan_approval && option_count > 0 => {
            move_permission_option_right(app, option_count);
            Some(true)
        }
        KeyCode::Enter if option_count > 0 => {
            respond_permission(app, None);
            Some(true)
        }
        KeyCode::Esc => {
            if let Some(idx) = focused_option_index_by_kind(app, PermissionOptionKind::RejectOnce)
                .or_else(|| focused_option_index_by_kind(app, PermissionOptionKind::RejectAlways))
                .or_else(|| focused_option_index_where(app, option_is_reject_fallback))
            {
                respond_permission(app, Some(idx));
                Some(true)
            } else if option_count > 0 {
                respond_permission(app, Some(option_count - 1));
                Some(true)
            } else {
                Some(false)
            }
        }
        _ => None,
    }
}

fn handle_permission_quick_shortcuts(app: &mut App, key: KeyEvent) -> Option<bool> {
    if !matches!(key.code, KeyCode::Char(_)) {
        return None;
    }
    if focused_permission_is_plan_approval(app) {
        if is_ctrl_char_shortcut(key, 'y')
            || is_ctrl_char_shortcut(key, 'a')
            || is_ctrl_char_shortcut(key, 'n')
        {
            return Some(false);
        }
        if !is_ctrl_shortcut(key.modifiers) {
            if matches!(key.code, KeyCode::Char('y' | 'Y'))
                && let Some(idx) =
                    focused_option_index_by_kind(app, PermissionOptionKind::PlanApprove)
            {
                respond_permission(app, Some(idx));
                return Some(true);
            }
            if matches!(key.code, KeyCode::Char('n' | 'N'))
                && let Some(idx) =
                    focused_option_index_by_kind(app, PermissionOptionKind::PlanReject)
            {
                respond_permission(app, Some(idx));
                return Some(true);
            }
        }
        return None;
    }
    if is_ctrl_char_shortcut(key, 'y') {
        if let Some(idx) = focused_option_index_by_kind(app, PermissionOptionKind::AllowOnce)
            .or_else(|| focused_option_index_where(app, option_is_allow_once_fallback))
            .or_else(|| focused_option_index_by_kind(app, PermissionOptionKind::AllowSession))
            .or_else(|| focused_option_index_by_kind(app, PermissionOptionKind::AllowAlways))
            .or_else(|| focused_option_index_where(app, option_is_allow_always_fallback))
            .or_else(|| focused_option_index_where(app, option_is_allow_non_once_fallback))
        {
            respond_permission(app, Some(idx));
            return Some(true);
        }
        return Some(false);
    }
    if is_ctrl_char_shortcut(key, 'a') {
        if let Some(idx) = focused_option_index_by_kind(app, PermissionOptionKind::AllowSession)
            .or_else(|| focused_option_index_by_kind(app, PermissionOptionKind::AllowAlways))
            .or_else(|| focused_option_index_where(app, option_is_allow_non_once_fallback))
        {
            respond_permission(app, Some(idx));
            return Some(true);
        }
        return Some(false);
    }
    if is_ctrl_char_shortcut(key, 'n') {
        if let Some(idx) = focused_option_index_by_kind(app, PermissionOptionKind::RejectOnce)
            .or_else(|| focused_option_index_where(app, option_is_reject_once_fallback))
        {
            respond_permission(app, Some(idx));
            return Some(true);
        }
        return Some(false);
    }
    None
}

pub(super) fn handle_permission_key(
    app: &mut App,
    key: KeyEvent,
    interaction_has_focus: bool,
) -> bool {
    let option_count = focused_permission(app).map_or(0, |permission| permission.options.len());
    let plan_approval = focused_permission_is_plan_approval(app);

    if let Some(consumed) =
        handle_permission_option_keys(app, key, interaction_has_focus, option_count, plan_approval)
    {
        return consumed;
    }
    if let Some(consumed) = handle_permission_quick_shortcuts(app, key) {
        return consumed;
    }
    false
}

fn respond_permission(app: &mut App, override_index: Option<usize>) {
    let Some(tool_id) = pop_next_valid_interaction_id(app) else {
        return;
    };

    let Some((mi, bi)) = app.tool_call_index.get(&tool_id).copied() else {
        return;
    };
    let Some(MessageBlock::ToolCall(tc)) =
        app.messages.get_mut(mi).and_then(|m| m.blocks.get_mut(bi))
    else {
        return;
    };
    let tc = tc.as_mut();
    let mut invalidated = false;
    if let Some(pending) = tc.pending_permission.take() {
        let idx = override_index.unwrap_or(pending.selected_index);
        if let Some(opt) = pending.options.get(idx) {
            tracing::debug!(
                "permission selection: tool_call_id={} option_id={} option_name={} option_kind={:?}",
                tool_id,
                opt.option_id,
                opt.name,
                opt.kind
            );
            let _ = pending.response_tx.send(model::RequestPermissionResponse::new(
                model::RequestPermissionOutcome::Selected(model::SelectedPermissionOutcome::new(
                    opt.option_id.clone(),
                )),
            ));
        } else {
            tracing::warn!(
                "permission selection index out of bounds: tool_call_id={} selected_index={} options={}",
                tool_id,
                idx,
                pending.options.len()
            );
        }
        tc.mark_tool_call_layout_dirty();
        invalidated = true;
    }
    if invalidated {
        app.sync_render_cache_slot(mi, bi);
        app.recompute_message_retained_bytes(mi);
        app.invalidate_layout(InvalidationLevel::MessageChanged(mi));
    }

    focus_next_inline_interaction(app);
}

#[cfg(test)]
fn respond_permission_cancel(app: &mut App) {
    let Some(tool_id) = pop_next_valid_interaction_id(app) else {
        return;
    };

    let Some((mi, bi)) = app.tool_call_index.get(&tool_id).copied() else {
        return;
    };
    let Some(MessageBlock::ToolCall(tc)) =
        app.messages.get_mut(mi).and_then(|m| m.blocks.get_mut(bi))
    else {
        return;
    };
    let tc = tc.as_mut();
    if let Some(pending) = tc.pending_permission.take() {
        let _ = pending.response_tx.send(model::RequestPermissionResponse::new(
            model::RequestPermissionOutcome::Cancelled,
        ));
        tc.mark_tool_call_layout_dirty();
        app.sync_render_cache_slot(mi, bi);
        app.recompute_message_retained_bytes(mi);
        app.invalidate_layout(InvalidationLevel::MessageChanged(mi));
    }

    focus_next_inline_interaction(app);
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::app::{
        App, AppStatus, BlockCache, ChatMessage, IncrementalMarkdown, InlinePermission,
        MessageBlock, MessageRole, ToolCallInfo,
    };
    use pretty_assertions::assert_eq;
    use tokio::sync::oneshot;

    fn test_tool_call(id: &str) -> ToolCallInfo {
        ToolCallInfo {
            id: id.to_owned(),
            title: format!("Tool {id}"),
            sdk_tool_name: "Read".to_owned(),
            raw_input: None,
            raw_input_bytes: 0,
            output_metadata: None,
            status: model::ToolCallStatus::InProgress,
            content: Vec::new(),
            hidden: false,
            terminal_id: None,
            terminal_command: None,
            terminal_output: None,
            terminal_output_len: 0,
            terminal_bytes_seen: 0,
            terminal_snapshot_mode: crate::app::TerminalSnapshotMode::AppendOnly,
            render_epoch: 0,
            layout_epoch: 0,
            last_measured_width: 0,
            last_measured_height: 0,
            last_measured_layout_epoch: 0,
            last_measured_layout_generation: 0,
            cache: BlockCache::default(),
            pending_permission: None,
            pending_question: None,
        }
    }

    fn assistant_tool_msg(tc: ToolCallInfo) -> ChatMessage {
        ChatMessage {
            role: MessageRole::Assistant,
            blocks: vec![MessageBlock::ToolCall(Box::new(tc))],
            usage: None,
        }
    }

    fn allow_options() -> Vec<model::PermissionOption> {
        vec![
            model::PermissionOption::new(
                "allow-once",
                "Allow once",
                PermissionOptionKind::AllowOnce,
            ),
            model::PermissionOption::new(
                "allow-always",
                "Allow always",
                PermissionOptionKind::AllowAlways,
            ),
            model::PermissionOption::new("reject-once", "Reject", PermissionOptionKind::RejectOnce),
        ]
    }

    fn add_permission(
        app: &mut App,
        tool_id: &str,
        options: Vec<model::PermissionOption>,
        focused: bool,
    ) -> oneshot::Receiver<model::RequestPermissionResponse> {
        let msg_idx = app.messages.len();
        app.messages.push(assistant_tool_msg(test_tool_call(tool_id)));
        app.index_tool_call(tool_id.to_owned(), msg_idx, 0);

        let (tx, rx) = oneshot::channel();
        if let Some(MessageBlock::ToolCall(tc)) =
            app.messages.get_mut(msg_idx).and_then(|m| m.blocks.get_mut(0))
        {
            tc.pending_permission =
                Some(InlinePermission { options, response_tx: tx, selected_index: 0, focused });
        }
        app.pending_interaction_ids.push(tool_id.to_owned());
        rx
    }

    fn permission_focused(app: &App, tool_id: &str) -> bool {
        let Some((mi, bi)) = app.lookup_tool_call(tool_id) else {
            return false;
        };
        let Some(MessageBlock::ToolCall(tc)) = app.messages.get(mi).and_then(|m| m.blocks.get(bi))
        else {
            return false;
        };
        tc.pending_permission.as_ref().is_some_and(|permission| permission.focused)
    }

    #[test]
    fn step2_up_down_rotates_permission_focus_and_enter_targets_focused_prompt() {
        let mut app = App::test_default();
        app.status = AppStatus::Ready;
        let mut rx1 = add_permission(&mut app, "perm-1", allow_options(), true);
        let mut rx2 = add_permission(&mut app, "perm-2", allow_options(), false);

        assert_eq!(app.pending_interaction_ids, vec!["perm-1", "perm-2"]);
        assert!(permission_focused(&app, "perm-1"));
        assert!(!permission_focused(&app, "perm-2"));

        let consumed = crate::app::inline_interactions::handle_interaction_focus_cycle(
            &mut app,
            KeyEvent::new(KeyCode::Down, KeyModifiers::NONE),
            true,
            false,
        );
        assert_eq!(consumed, Some(true));
        assert_eq!(app.pending_interaction_ids, vec!["perm-2", "perm-1"]);
        assert!(permission_focused(&app, "perm-2"));
        assert!(!permission_focused(&app, "perm-1"));

        let consumed = handle_permission_key(
            &mut app,
            KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE),
            true,
        );
        assert!(consumed);

        let resp2 = rx2.try_recv().expect("focused permission should receive response");
        let model::RequestPermissionOutcome::Selected(sel2) = resp2.outcome else {
            panic!("expected selected permission response");
        };
        assert_eq!(sel2.option_id.clone(), "allow-once");
        assert!(matches!(rx1.try_recv(), Err(tokio::sync::oneshot::error::TryRecvError::Empty)));
        assert_eq!(app.pending_interaction_ids, vec!["perm-1"]);
    }

    #[test]
    fn step3_lowercase_a_is_not_consumed_by_permission_shortcuts() {
        let mut app = App::test_default();
        let mut rx = add_permission(&mut app, "perm-1", allow_options(), true);

        let consumed = handle_permission_key(
            &mut app,
            KeyEvent::new(KeyCode::Char('a'), KeyModifiers::NONE),
            true,
        );

        assert!(!consumed, "lowercase 'a' should flow to normal typing");
        assert_eq!(app.pending_interaction_ids, vec!["perm-1"]);
        assert!(matches!(rx.try_recv(), Err(tokio::sync::oneshot::error::TryRecvError::Empty)));
    }

    #[test]
    fn step4_ctrl_y_maps_to_allow_once_kind_and_only_resolves_one_permission() {
        let mut app = App::test_default();
        let mut rx1 = add_permission(
            &mut app,
            "perm-1",
            vec![
                model::PermissionOption::new(
                    "allow-always",
                    "Allow always",
                    PermissionOptionKind::AllowAlways,
                ),
                model::PermissionOption::new(
                    "allow-once",
                    "Allow once",
                    PermissionOptionKind::AllowOnce,
                ),
                model::PermissionOption::new(
                    "reject-once",
                    "Reject",
                    PermissionOptionKind::RejectOnce,
                ),
            ],
            true,
        );
        let mut rx2 = add_permission(&mut app, "perm-2", allow_options(), false);

        let consumed = handle_permission_key(
            &mut app,
            KeyEvent::new(KeyCode::Char('y'), KeyModifiers::CONTROL),
            true,
        );
        assert!(consumed);

        let resp1 = rx1.try_recv().expect("first permission should be answered");
        let model::RequestPermissionOutcome::Selected(sel1) = resp1.outcome else {
            panic!("expected selected permission response");
        };
        assert_eq!(sel1.option_id.clone(), "allow-once");
        assert_eq!(app.pending_interaction_ids, vec!["perm-2"]);
        assert!(matches!(rx2.try_recv(), Err(tokio::sync::oneshot::error::TryRecvError::Empty)));
    }

    #[test]
    fn plain_y_and_n_are_not_consumed() {
        let mut app = App::test_default();
        let mut rx = add_permission(&mut app, "perm-1", allow_options(), true);

        let consumed_y = handle_permission_key(
            &mut app,
            KeyEvent::new(KeyCode::Char('y'), KeyModifiers::NONE),
            true,
        );
        let consumed_n = handle_permission_key(
            &mut app,
            KeyEvent::new(KeyCode::Char('n'), KeyModifiers::NONE),
            true,
        );

        assert!(!consumed_y);
        assert!(!consumed_n);
        assert_eq!(app.pending_interaction_ids, vec!["perm-1"]);
        assert!(matches!(rx.try_recv(), Err(tokio::sync::oneshot::error::TryRecvError::Empty)));
    }

    #[test]
    fn ctrl_n_rejects_focused_permission() {
        let mut app = App::test_default();
        let mut rx = add_permission(&mut app, "perm-1", allow_options(), true);

        let consumed = handle_permission_key(
            &mut app,
            KeyEvent::new(KeyCode::Char('n'), KeyModifiers::CONTROL),
            true,
        );
        assert!(consumed);
        assert!(app.pending_interaction_ids.is_empty());

        let resp = rx.try_recv().expect("permission should be answered by ctrl+n");
        let model::RequestPermissionOutcome::Selected(sel) = resp.outcome else {
            panic!("expected selected permission response");
        };
        assert_eq!(sel.option_id.clone(), "reject-once");
    }

    #[test]
    fn ctrl_n_does_not_trigger_reject_always() {
        let mut app = App::test_default();
        let mut rx = add_permission(
            &mut app,
            "perm-1",
            vec![
                model::PermissionOption::new(
                    "allow-once",
                    "Allow once",
                    PermissionOptionKind::AllowOnce,
                ),
                model::PermissionOption::new(
                    "reject-always",
                    "Reject always",
                    PermissionOptionKind::RejectAlways,
                ),
            ],
            true,
        );

        let consumed = handle_permission_key(
            &mut app,
            KeyEvent::new(KeyCode::Char('n'), KeyModifiers::CONTROL),
            true,
        );
        assert!(!consumed);
        assert_eq!(app.pending_interaction_ids, vec!["perm-1"]);
        assert!(matches!(rx.try_recv(), Err(tokio::sync::oneshot::error::TryRecvError::Empty)));
    }

    #[test]
    fn ctrl_a_matches_allow_always_by_label_when_kind_is_missing() {
        let mut app = App::test_default();
        let mut rx = add_permission(
            &mut app,
            "perm-1",
            vec![
                model::PermissionOption::new(
                    "allow-once",
                    "Allow once",
                    PermissionOptionKind::AllowOnce,
                ),
                model::PermissionOption::new(
                    "allow-always",
                    "Allow always",
                    PermissionOptionKind::AllowOnce,
                ),
                model::PermissionOption::new(
                    "reject-once",
                    "Reject",
                    PermissionOptionKind::RejectOnce,
                ),
            ],
            true,
        );

        let consumed = handle_permission_key(
            &mut app,
            KeyEvent::new(KeyCode::Char('a'), KeyModifiers::CONTROL),
            true,
        );
        assert!(consumed);

        let resp = rx.try_recv().expect("permission should be answered by ctrl+a fallback");
        let model::RequestPermissionOutcome::Selected(sel) = resp.outcome else {
            panic!("expected selected permission response");
        };
        assert_eq!(sel.option_id.clone(), "allow-always");
    }

    #[test]
    fn ctrl_a_accepts_uppercase_with_shift_modifier() {
        let mut app = App::test_default();
        let mut rx = add_permission(&mut app, "perm-1", allow_options(), true);

        let consumed = handle_permission_key(
            &mut app,
            KeyEvent::new(KeyCode::Char('A'), KeyModifiers::CONTROL | KeyModifiers::SHIFT),
            true,
        );
        assert!(consumed);

        let resp = rx.try_recv().expect("permission should be answered by uppercase ctrl+a");
        let model::RequestPermissionOutcome::Selected(sel) = resp.outcome else {
            panic!("expected selected permission response");
        };
        assert_eq!(sel.option_id.clone(), "allow-always");
    }

    #[test]
    fn left_right_not_consumed_when_permission_not_focused() {
        let mut app = App::test_default();
        let mut rx = add_permission(&mut app, "perm-1", allow_options(), false);

        let consumed_left = handle_permission_key(
            &mut app,
            KeyEvent::new(KeyCode::Left, KeyModifiers::NONE),
            false,
        );
        let consumed_right = handle_permission_key(
            &mut app,
            KeyEvent::new(KeyCode::Right, KeyModifiers::NONE),
            false,
        );

        assert!(!consumed_left);
        assert!(!consumed_right);
        assert!(matches!(rx.try_recv(), Err(tokio::sync::oneshot::error::TryRecvError::Empty)));
    }

    #[test]
    fn enter_not_consumed_when_permission_not_focused() {
        let mut app = App::test_default();
        let mut rx = add_permission(&mut app, "perm-1", allow_options(), false);

        let consumed = handle_permission_key(
            &mut app,
            KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE),
            false,
        );

        assert!(!consumed);
        assert_eq!(app.pending_interaction_ids, vec!["perm-1"]);
        assert!(matches!(rx.try_recv(), Err(tokio::sync::oneshot::error::TryRecvError::Empty)));
    }

    #[test]
    fn keeps_non_tool_blocks_untouched() {
        let app = App::test_default();
        let _ = IncrementalMarkdown::default();
        assert!(app.messages.is_empty());
    }

    #[test]
    fn single_focused_permission_consumes_up_down_without_rotation() {
        let mut app = App::test_default();
        let mut rx = add_permission(&mut app, "perm-1", allow_options(), true);
        app.viewport.scroll_target = 7;

        let consumed_up = crate::app::inline_interactions::handle_interaction_focus_cycle(
            &mut app,
            KeyEvent::new(KeyCode::Up, KeyModifiers::NONE),
            true,
            false,
        );
        let consumed_down = crate::app::inline_interactions::handle_interaction_focus_cycle(
            &mut app,
            KeyEvent::new(KeyCode::Down, KeyModifiers::NONE),
            true,
            false,
        );

        assert_eq!(consumed_up, Some(true));
        assert_eq!(consumed_down, Some(true));
        assert_eq!(app.pending_interaction_ids, vec!["perm-1"]);
        assert_eq!(app.viewport.scroll_target, 7);
        assert!(matches!(rx.try_recv(), Err(tokio::sync::oneshot::error::TryRecvError::Empty)));
    }

    #[test]
    fn esc_cancels_permission_when_no_reject_option_exists() {
        let mut app = App::test_default();
        let mut rx = add_permission(
            &mut app,
            "perm-1",
            vec![model::PermissionOption::new(
                "allow-once",
                "Allow once",
                PermissionOptionKind::AllowOnce,
            )],
            true,
        );

        respond_permission_cancel(&mut app);

        let resp = rx.try_recv().expect("permission should be cancelled");
        assert!(matches!(resp.outcome, model::RequestPermissionOutcome::Cancelled));
    }
}
