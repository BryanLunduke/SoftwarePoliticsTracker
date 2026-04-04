// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::super::{
    App, AppStatus, BlockCache, ChatMessage, InvalidationLevel, MessageBlock, MessageRole,
    ToolCallInfo, ToolCallScope,
};
use super::tool_updates::raw_output_to_terminal_text;
use crate::agent::model;
use crate::app::todos::{parse_todos_if_present, set_todos};
use std::time::Instant;

pub(super) fn handle_tool_call(app: &mut App, tc: model::ToolCall) {
    log_tool_call_received(&tc);
    let id_str = tc.tool_call_id.clone();
    let sdk_tool_name = resolve_sdk_tool_name(tc.kind, tc.meta.as_ref());
    let scope = register_tool_call_scope(app, &id_str, &sdk_tool_name);
    maybe_apply_todo_write_from_tool_call(app, &id_str, &sdk_tool_name, tc.raw_input.as_ref());
    update_subagent_scope_state(app, scope, tc.status, &id_str);

    let tool_info = build_tool_info_from_tool_call(app, tc, sdk_tool_name);
    if should_jump_on_large_write(&tool_info) {
        app.viewport.engage_auto_scroll();
    }
    upsert_tool_call_into_assistant_message(app, tool_info);

    app.status = AppStatus::Running;
    app.files_accessed += 1;
}

fn log_tool_call_received(tc: &model::ToolCall) {
    let id_str = tc.tool_call_id.clone();
    let title = tc.title.clone();
    let kind = tc.kind;
    tracing::debug!(
        "ToolCall: id={id_str} title={title} kind={kind:?} status={:?} content_blocks={} has_raw_output={}",
        tc.status,
        tc.content.len(),
        tc.raw_output.is_some()
    );
}

pub(super) fn register_tool_call_scope(
    app: &mut App,
    id: &str,
    sdk_tool_name: &str,
) -> ToolCallScope {
    // TODO: When the bridge exposes an explicit Task/Agent <-> child-tool relation,
    // redesign subagent rendering so the parent Task/Agent summary block becomes
    // the primary visible surface and child agent tools are not rendered directly.
    let is_task = matches!(sdk_tool_name, "Task" | "Agent");
    let scope = if is_task {
        ToolCallScope::Task
    } else if app.active_task_ids.is_empty() {
        ToolCallScope::MainAgent
    } else {
        ToolCallScope::Subagent
    };
    app.register_tool_call_scope(id.to_owned(), scope);
    if is_task {
        app.insert_active_task(id.to_owned());
    }
    scope
}

fn maybe_apply_todo_write_from_tool_call(
    app: &mut App,
    id: &str,
    sdk_tool_name: &str,
    raw_input: Option<&serde_json::Value>,
) {
    if sdk_tool_name != "TodoWrite" {
        return;
    }
    tracing::info!("TodoWrite ToolCall detected: id={id}, raw_input={raw_input:?}");
    if let Some(raw_input) = raw_input {
        if let Some(todos) = parse_todos_if_present(raw_input) {
            tracing::info!("Parsed {} todos from ToolCall raw_input", todos.len());
            set_todos(app, todos);
        } else {
            tracing::debug!(
                "TodoWrite ToolCall raw_input has no todos array yet; preserving existing todos"
            );
        }
    } else {
        tracing::warn!("TodoWrite ToolCall has no raw_input");
    }
}

pub(super) fn update_subagent_scope_state(
    app: &mut App,
    scope: ToolCallScope,
    status: model::ToolCallStatus,
    id: &str,
) {
    match (scope, status) {
        (
            ToolCallScope::Subagent,
            model::ToolCallStatus::InProgress | model::ToolCallStatus::Pending,
        ) => app.mark_subagent_tool_started(id),
        (
            ToolCallScope::Subagent,
            model::ToolCallStatus::Completed | model::ToolCallStatus::Failed,
        ) => app.mark_subagent_tool_finished(id, Instant::now()),
        _ => app.refresh_subagent_idle_since(Instant::now()),
    }
}

fn build_tool_info_from_tool_call(
    app: &App,
    tc: model::ToolCall,
    sdk_tool_name: String,
) -> ToolCallInfo {
    let terminal_id = tc.content.iter().find_map(|content| match content {
        model::ToolCallContent::Terminal(term) => Some(term.terminal_id.clone()),
        _ => None,
    });
    let terminal_command = terminal_id.as_ref().and_then(|terminal_id| {
        app.terminals.borrow().get(terminal_id).map(|terminal| terminal.command.clone())
    });
    let initial_execute_output = if super::super::is_execute_tool_name(&sdk_tool_name) {
        tc.raw_output.as_ref().and_then(raw_output_to_terminal_text)
    } else {
        None
    };

    let mut tool_info = ToolCallInfo {
        id: tc.tool_call_id,
        title: shorten_tool_title(&tc.title, &app.cwd_raw),
        sdk_tool_name,
        raw_input: tc.raw_input,
        raw_input_bytes: 0,
        output_metadata: tc.output_metadata,
        status: tc.status,
        content: tc.content,
        hidden: false,
        terminal_id,
        terminal_command,
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
    };
    tool_info.raw_input_bytes =
        tool_info.raw_input.as_ref().map_or(0, ToolCallInfo::estimate_json_value_bytes);
    if let Some(output) = initial_execute_output {
        tool_info.terminal_output_len = output.len();
        tool_info.terminal_bytes_seen = output.len();
        tool_info.terminal_output = Some(output);
        tool_info.terminal_snapshot_mode = crate::app::TerminalSnapshotMode::ReplaceSnapshot;
    }
    tool_info
}

pub(super) fn upsert_tool_call_into_assistant_message(app: &mut App, tool_info: ToolCallInfo) {
    let existing_pos = app.lookup_tool_call(&tool_info.id);

    if let Some((mi, bi)) = existing_pos {
        update_existing_tool_call(app, mi, bi, &tool_info);
        return;
    }

    if let Some(msg_idx) = app.active_turn_assistant_idx()
        && let Some(owner) = app.messages.get_mut(msg_idx)
    {
        let block_idx = owner.blocks.len();
        let tc_id = tool_info.id.clone();
        let terminal_id = App::tracked_terminal_id_for_tool(&tool_info);
        owner.blocks.push(MessageBlock::ToolCall(Box::new(tool_info)));
        app.sync_after_message_blocks_changed(msg_idx);
        app.index_tool_call(tc_id, msg_idx, block_idx);
        sync_tool_call_terminal_tracking(app, msg_idx, block_idx, terminal_id);
        return;
    }

    let msg_idx = app.messages.len().saturating_sub(1);
    if app.messages.last().is_some_and(|m| matches!(m.role, MessageRole::Assistant)) {
        if let Some(last) = app.messages.last_mut() {
            let block_idx = last.blocks.len();
            let tc_id = tool_info.id.clone();
            let terminal_id = App::tracked_terminal_id_for_tool(&tool_info);
            last.blocks.push(MessageBlock::ToolCall(Box::new(tool_info)));
            app.bind_active_turn_assistant(msg_idx);
            app.sync_after_message_blocks_changed(msg_idx);
            app.index_tool_call(tc_id, msg_idx, block_idx);
            sync_tool_call_terminal_tracking(app, msg_idx, block_idx, terminal_id);
        }
    } else {
        let tc_id = tool_info.id.clone();
        let terminal_id = App::tracked_terminal_id_for_tool(&tool_info);
        let new_idx = app.messages.len();
        app.push_message_tracked(ChatMessage {
            role: MessageRole::Assistant,
            blocks: vec![MessageBlock::ToolCall(Box::new(tool_info))],
            usage: None,
        });
        app.bind_active_turn_assistant(new_idx);
        app.index_tool_call(tc_id, new_idx, 0);
        sync_tool_call_terminal_tracking(app, new_idx, 0, terminal_id);
    }
}

fn update_existing_tool_call(app: &mut App, mi: usize, bi: usize, tool_info: &ToolCallInfo) {
    let mut layout_dirty = false;
    let mut terminal_tracking = None;
    if let Some(MessageBlock::ToolCall(existing)) =
        app.messages.get_mut(mi).and_then(|m| m.blocks.get_mut(bi))
    {
        let existing = existing.as_mut();
        let mut changed = false;
        changed |= sync_if_changed(&mut existing.title, &tool_info.title);
        changed |= sync_if_changed(&mut existing.status, &tool_info.status);
        changed |= sync_if_changed(&mut existing.content, &tool_info.content);
        changed |= sync_if_changed(&mut existing.sdk_tool_name, &tool_info.sdk_tool_name);
        changed |= existing.set_raw_input(tool_info.raw_input.clone());
        changed |= sync_if_changed(&mut existing.output_metadata, &tool_info.output_metadata);
        if tool_info.terminal_id.is_some() {
            changed |= sync_if_changed(&mut existing.terminal_id, &tool_info.terminal_id);
        }
        if tool_info.terminal_command.is_some() {
            changed |= sync_if_changed(&mut existing.terminal_command, &tool_info.terminal_command);
        }
        if tool_info.terminal_output.is_some() {
            changed |= sync_if_changed(&mut existing.terminal_output, &tool_info.terminal_output);
            changed |=
                sync_if_changed(&mut existing.terminal_output_len, &tool_info.terminal_output_len);
            changed |=
                sync_if_changed(&mut existing.terminal_bytes_seen, &tool_info.terminal_bytes_seen);
            changed |= sync_if_changed(
                &mut existing.terminal_snapshot_mode,
                &tool_info.terminal_snapshot_mode,
            );
        }
        if changed {
            existing.mark_tool_call_layout_dirty();
            layout_dirty = true;
        } else {
            crate::perf::mark("tool_update_noop_skips");
        }
        terminal_tracking = Some(App::tracked_terminal_id_for_tool(existing));
    }
    sync_tool_call_terminal_tracking(app, mi, bi, terminal_tracking.flatten());
    if layout_dirty {
        app.sync_render_cache_slot(mi, bi);
        app.recompute_message_retained_bytes(mi);
        app.invalidate_layout(InvalidationLevel::MessageChanged(mi));
    }
}

fn sync_tool_call_terminal_tracking(
    app: &mut App,
    msg_idx: usize,
    block_idx: usize,
    terminal_id: Option<String>,
) {
    if let Some(terminal_id) = terminal_id {
        app.sync_terminal_tool_call(terminal_id, msg_idx, block_idx);
    } else {
        app.untrack_terminal_tool_call(msg_idx, block_idx);
    }
}

pub(super) fn sync_if_changed<T: PartialEq + Clone>(dst: &mut T, src: &T) -> bool {
    if dst == src {
        return false;
    }
    dst.clone_from(src);
    true
}

pub(super) fn sdk_tool_name_from_meta(meta: Option<&serde_json::Value>) -> Option<&str> {
    meta.and_then(|m| m.get("claudeCode")).and_then(|v| v.get("toolName")).and_then(|v| v.as_str())
}

fn fallback_sdk_tool_name(kind: model::ToolKind) -> &'static str {
    match kind {
        model::ToolKind::Read => "Read",
        model::ToolKind::Edit => "Edit",
        model::ToolKind::Delete => "Delete",
        model::ToolKind::Move => "Move",
        model::ToolKind::Search => "Search",
        model::ToolKind::Execute => "Bash",
        model::ToolKind::Think => "Think",
        model::ToolKind::Fetch => "Fetch",
        model::ToolKind::SwitchMode => "ExitPlanMode",
        model::ToolKind::Other => "Tool",
    }
}

pub(super) fn resolve_sdk_tool_name(
    kind: model::ToolKind,
    meta: Option<&serde_json::Value>,
) -> String {
    if let Some(name) = sdk_tool_name_from_meta(meta).filter(|name| !name.trim().is_empty()) {
        name.to_owned()
    } else {
        let fallback = fallback_sdk_tool_name(kind);
        if matches!(kind, model::ToolKind::Think) {
            tracing::warn!(
                "ToolKind::Think tool arrived with no meta.claudeCode.toolName -- \
                 Task/Agent scope detection may be incorrect; falling back to '{fallback}'"
            );
        }
        fallback.to_owned()
    }
}

/// Shorten absolute paths in tool titles to relative paths based on cwd.
/// e.g. "Read C:\\Users\\me\\project\\src\\main.rs" -> "Read src/main.rs"
/// Handles both `/` and `\\` separators on all platforms since the bridge adapter
/// may use either regardless of the host OS.
pub(super) fn shorten_tool_title(title: &str, cwd_raw: &str) -> String {
    if cwd_raw.is_empty() {
        return title.to_owned();
    }

    // Quick check: if title doesn't contain any part of cwd, skip normalization
    // Use the first path component of cwd as a heuristic
    let cwd_start = cwd_raw.split(['/', '\\']).find(|s| !s.is_empty()).unwrap_or(cwd_raw);
    if !title.contains(cwd_start) {
        return title.to_owned();
    }

    // Normalize both to forward slashes for matching
    let cwd_norm = cwd_raw.replace('\\', "/");
    let title_norm = title.replace('\\', "/");

    // Ensure cwd ends with slash so we strip the separator too
    let with_sep = if cwd_norm.ends_with('/') { cwd_norm } else { format!("{cwd_norm}/") };

    if title_norm.contains(&with_sep) {
        return title_norm.replace(&with_sep, "");
    }
    title_norm
}

pub(super) const WRITE_DIFF_JUMP_THRESHOLD_LINES: usize = 40;

pub(super) fn should_jump_on_large_write(tc: &ToolCallInfo) -> bool {
    if tc.sdk_tool_name != "Write" {
        return false;
    }
    tc.content.iter().any(|c| match c {
        model::ToolCallContent::Diff(diff) => {
            let new_lines = diff.new_text.lines().count();
            let old_lines = diff.old_text.as_deref().map_or(0, |t| t.lines().count());
            new_lines.max(old_lines) >= WRITE_DIFF_JUMP_THRESHOLD_LINES
        }
        _ => false,
    })
}

/// Check if any tool call in the current assistant message is still in-progress.
pub(super) fn has_in_progress_tool_calls(app: &App) -> bool {
    if let Some(owner_idx) = app.active_turn_assistant_idx()
        && let Some(owner) = app.messages.get(owner_idx)
    {
        return owner.blocks.iter().any(|block| {
            matches!(
                block,
                MessageBlock::ToolCall(tc)
                    if matches!(tc.status, model::ToolCallStatus::InProgress | model::ToolCallStatus::Pending)
            )
        });
    }
    false
}
