// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::super::{App, AppStatus, InvalidationLevel, MessageBlock, ToolCallInfo, ToolCallScope};
use super::tool_calls::{
    has_in_progress_tool_calls, sdk_tool_name_from_meta, should_jump_on_large_write,
};
use crate::agent::error_handling::{looks_like_internal_error, summarize_internal_error};
use crate::agent::model;
use crate::app::todos::{parse_todos_if_present, set_todos};
use std::time::Instant;

pub(super) fn handle_tool_call_update_session(app: &mut App, tcu: &model::ToolCallUpdate) {
    let id_str = tcu.tool_call_id.clone();
    log_tool_call_update_received(&id_str, tcu);
    maybe_log_internal_failed_tool_update(&id_str, tcu);
    let Some((mi, bi)) = app.lookup_tool_call(&id_str) else {
        tracing::warn!("ToolCallUpdate: id={id_str} not found in index");
        return;
    };
    let tool_scope = app.tool_call_scope(&id_str);
    apply_tool_scope_status_update(app, &id_str, tool_scope, tcu.fields.status);

    let update_outcome = apply_tool_call_update_to_indexed_block(app, mi, bi, &id_str, tcu);
    if let Some(mi) = update_outcome.layout_dirty_idx {
        app.recompute_message_retained_bytes(mi);
        app.invalidate_layout(InvalidationLevel::MessageChanged(mi));
    }
    if let Some(todos) = update_outcome.pending_todos {
        set_todos(app, todos);
    }
    if matches!(app.status, AppStatus::Running) && !has_in_progress_tool_calls(app) {
        app.status = AppStatus::Thinking;
    }
}

fn log_tool_call_update_received(id_str: &str, tcu: &model::ToolCallUpdate) {
    let has_content = tcu.fields.content.as_ref().map_or(0, Vec::len);
    let has_raw_output = tcu.fields.raw_output.is_some();
    tracing::debug!(
        "ToolCallUpdate: id={id_str} new_title={:?} new_status={:?} content_blocks={has_content} has_raw_output={has_raw_output}",
        tcu.fields.title,
        tcu.fields.status
    );
    if has_raw_output {
        tracing::debug!("ToolCallUpdate raw_output: id={id_str} {:?}", tcu.fields.raw_output);
    }
}

fn maybe_log_internal_failed_tool_update(id_str: &str, tcu: &model::ToolCallUpdate) {
    if matches!(tcu.fields.status, Some(model::ToolCallStatus::Failed))
        && let Some(content_preview) = internal_failed_tool_content_preview(
            tcu.fields.content.as_deref(),
            tcu.fields.raw_output.as_ref(),
        )
    {
        let sdk_tool_name = sdk_tool_name_from_meta(tcu.meta.as_ref());
        tracing::debug!(
            tool_call_id = %id_str,
            title = ?tcu.fields.title,
            sdk_tool_name = ?sdk_tool_name,
            content_preview = %content_preview,
            "Internal failed ToolCallUpdate payload"
        );
    }
}

fn apply_tool_scope_status_update(
    app: &mut App,
    id_str: &str,
    tool_scope: Option<ToolCallScope>,
    status: Option<model::ToolCallStatus>,
) {
    let Some(status) = status else {
        return;
    };
    match tool_scope {
        Some(ToolCallScope::Subagent) => match status {
            model::ToolCallStatus::Pending | model::ToolCallStatus::InProgress => {
                app.mark_subagent_tool_started(id_str);
            }
            model::ToolCallStatus::Completed | model::ToolCallStatus::Failed => {
                app.mark_subagent_tool_finished(id_str, Instant::now());
            }
        },
        Some(ToolCallScope::Task) => match status {
            model::ToolCallStatus::Pending | model::ToolCallStatus::InProgress => {
                app.refresh_subagent_idle_since(Instant::now());
            }
            model::ToolCallStatus::Completed | model::ToolCallStatus::Failed => {
                app.remove_active_task(id_str);
                app.refresh_subagent_idle_since(Instant::now());
            }
        },
        Some(ToolCallScope::MainAgent) | None => {}
    }
}

struct ToolCallUpdateApplyOutcome {
    layout_dirty_idx: Option<usize>,
    pending_todos: Option<Vec<super::super::TodoItem>>,
}

fn apply_tool_call_update_to_indexed_block(
    app: &mut App,
    mi: usize,
    bi: usize,
    id_str: &str,
    tcu: &model::ToolCallUpdate,
) -> ToolCallUpdateApplyOutcome {
    let mut out = ToolCallUpdateApplyOutcome { layout_dirty_idx: None, pending_todos: None };
    let terminals = std::rc::Rc::clone(&app.terminals);
    let mut terminal_subscription: Option<String> = None;
    let mut detach_terminal = false;

    if let Some(MessageBlock::ToolCall(tc)) =
        app.messages.get_mut(mi).and_then(|m| m.blocks.get_mut(bi))
    {
        let tc = tc.as_mut();
        let mut changed = false;
        changed |= apply_tool_call_status_update(tc, tcu.fields.status);
        changed |= apply_tool_call_title_update(tc, tcu.fields.title.as_deref(), &app.cwd_raw);
        changed |= apply_tool_call_content_update(
            tc,
            tcu.fields.content.as_deref(),
            &terminals,
            &mut terminal_subscription,
        );
        changed |= apply_tool_call_raw_input_update(tc, tcu.fields.raw_input.as_ref());
        changed |= apply_tool_call_output_metadata_update(tc, tcu.fields.output_metadata.as_ref());
        changed |= apply_tool_call_raw_output_update(tc, tcu.fields.raw_output.as_ref());
        changed |= apply_tool_call_name_update(tc, tcu.meta.as_ref());
        out.pending_todos =
            extract_todo_updates_from_tool_call_update(id_str, tc, tcu.fields.raw_input.as_ref());
        detach_terminal = detach_terminal_if_final(tc);

        if changed {
            if should_jump_on_large_write(tc) {
                app.viewport.engage_auto_scroll();
            }
            tc.mark_tool_call_layout_dirty();
            app.sync_render_cache_slot(mi, bi);
            out.layout_dirty_idx = Some(mi);
        } else {
            crate::perf::mark("tool_update_noop_skips");
        }
    }

    if detach_terminal {
        app.untrack_terminal_tool_call(mi, bi);
    } else if let Some(terminal_id) = terminal_subscription {
        app.sync_terminal_tool_call(terminal_id, mi, bi);
    }

    out
}

fn apply_tool_call_status_update(
    tc: &mut ToolCallInfo,
    status: Option<model::ToolCallStatus>,
) -> bool {
    if let Some(status) = status
        && tc.status != status
    {
        tc.status = status;
        return true;
    }
    false
}

fn apply_tool_call_title_update(tc: &mut ToolCallInfo, title: Option<&str>, cwd_raw: &str) -> bool {
    let Some(title) = title else {
        return false;
    };
    let shortened = super::tool_calls::shorten_tool_title(title, cwd_raw);
    if tc.title == shortened {
        return false;
    }
    tc.title = shortened;
    true
}

fn apply_tool_call_content_update(
    tc: &mut ToolCallInfo,
    content: Option<&[model::ToolCallContent]>,
    terminals: &crate::agent::events::TerminalMap,
    terminal_subscription: &mut Option<String>,
) -> bool {
    let Some(content) = content else {
        return false;
    };
    let mut changed = false;
    for cb in content {
        if let model::ToolCallContent::Terminal(t) = cb {
            let tid = t.terminal_id.clone();
            if let Some(terminal) = terminals.borrow().get(&tid)
                && tc.terminal_command.as_deref() != Some(terminal.command.as_str())
            {
                tc.terminal_command = Some(terminal.command.clone());
                changed = true;
            }
            if tc.terminal_id.as_deref() != Some(tid.as_str()) {
                tc.terminal_id = Some(tid.clone());
                changed = true;
            }
            *terminal_subscription = Some(tid);
        }
    }
    if tc.content != content {
        tc.content = content.to_vec();
        changed = true;
    }
    changed
}

fn apply_tool_call_raw_input_update(
    tc: &mut ToolCallInfo,
    raw_input: Option<&serde_json::Value>,
) -> bool {
    let Some(raw_input) = raw_input else {
        return false;
    };
    tc.set_raw_input(Some(raw_input.clone()))
}

fn apply_tool_call_output_metadata_update(
    tc: &mut ToolCallInfo,
    output_metadata: Option<&model::ToolOutputMetadata>,
) -> bool {
    let Some(output_metadata) = output_metadata else {
        return false;
    };
    if tc.output_metadata.as_ref() == Some(output_metadata) {
        return false;
    }
    tc.output_metadata = Some(output_metadata.clone());
    true
}

fn apply_tool_call_raw_output_update(
    tc: &mut ToolCallInfo,
    raw_output: Option<&serde_json::Value>,
) -> bool {
    if !tc.is_execute_tool() {
        return false;
    }
    let Some(raw_output) = raw_output else {
        return false;
    };
    let Some(output) = raw_output_to_terminal_text(raw_output) else {
        return false;
    };
    if tc.terminal_output.as_deref() == Some(output.as_str()) {
        return false;
    }
    tc.terminal_output_len = output.len();
    tc.terminal_bytes_seen = output.len();
    tc.terminal_output = Some(output);
    tc.terminal_snapshot_mode = crate::app::TerminalSnapshotMode::ReplaceSnapshot;
    true
}

fn apply_tool_call_name_update(tc: &mut ToolCallInfo, meta: Option<&serde_json::Value>) -> bool {
    let Some(name) = sdk_tool_name_from_meta(meta) else {
        return false;
    };
    if name.trim().is_empty() || tc.sdk_tool_name == name {
        return false;
    }
    name.clone_into(&mut tc.sdk_tool_name);
    true
}

fn detach_terminal_if_final(tc: &mut ToolCallInfo) -> bool {
    if !tc.is_execute_tool()
        || matches!(tc.status, model::ToolCallStatus::Pending | model::ToolCallStatus::InProgress)
        || tc.terminal_id.is_none()
    {
        return false;
    }

    tc.terminal_id = None;
    true
}

fn extract_todo_updates_from_tool_call_update(
    id_str: &str,
    tc: &ToolCallInfo,
    raw_input: Option<&serde_json::Value>,
) -> Option<Vec<super::super::TodoItem>> {
    if tc.sdk_tool_name != "TodoWrite" {
        return None;
    }
    tracing::info!("TodoWrite ToolCallUpdate: id={id_str}, raw_input={raw_input:?}");
    let raw_input = raw_input?;
    if let Some(todos) = parse_todos_if_present(raw_input) {
        tracing::info!("Parsed {} todos from ToolCallUpdate raw_input", todos.len());
        return Some(todos);
    }
    tracing::debug!(
        "TodoWrite ToolCallUpdate raw_input has no todos array yet; preserving existing todos"
    );
    None
}

pub(super) fn raw_output_to_terminal_text(raw_output: &serde_json::Value) -> Option<String> {
    match raw_output {
        serde_json::Value::Null => None,
        serde_json::Value::String(s) => (!s.is_empty()).then(|| s.clone()),
        serde_json::Value::Array(items) => {
            let chunks: Vec<&str> = items.iter().filter_map(extract_text_field).collect();
            if chunks.is_empty() {
                serde_json::to_string_pretty(raw_output).ok().filter(|s| !s.is_empty())
            } else {
                Some(chunks.join("\n"))
            }
        }
        value => extract_text_field(value)
            .map(str::to_owned)
            .or_else(|| serde_json::to_string_pretty(value).ok().filter(|s| !s.is_empty())),
    }
}

fn extract_text_field(value: &serde_json::Value) -> Option<&str> {
    value.get("text").and_then(serde_json::Value::as_str)
}

fn internal_failed_tool_content_preview(
    content: Option<&[model::ToolCallContent]>,
    raw_output: Option<&serde_json::Value>,
) -> Option<String> {
    let text = content
        .and_then(|items| {
            items.iter().find_map(|c| match c {
                model::ToolCallContent::Content(inner) => match &inner.content {
                    model::ContentBlock::Text(t) => Some(t.text.clone()),
                    model::ContentBlock::Image(_) => None,
                },
                _ => None,
            })
        })
        .or_else(|| raw_output.and_then(raw_output_to_terminal_text))?;
    if !looks_like_internal_error(&text) {
        return None;
    }
    Some(summarize_internal_error(&text))
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::app::{
        App, BlockCache, ChatMessage, MessageBlock, MessageRole, TerminalSnapshotMode,
    };

    fn make_bash_tool_call(
        id: &str,
        status: model::ToolCallStatus,
        terminal_id: Option<&str>,
    ) -> ToolCallInfo {
        ToolCallInfo {
            id: id.to_owned(),
            title: format!("tool {id}"),
            sdk_tool_name: "Bash".to_owned(),
            raw_input: None,
            raw_input_bytes: 0,
            output_metadata: None,
            status,
            content: Vec::new(),
            hidden: false,
            terminal_id: terminal_id.map(str::to_owned),
            terminal_command: Some("echo test".to_owned()),
            terminal_output: None,
            terminal_output_len: 0,
            terminal_bytes_seen: 0,
            terminal_snapshot_mode: TerminalSnapshotMode::AppendOnly,
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

    fn terminal_content(terminal_id: &str) -> Vec<model::ToolCallContent> {
        vec![model::ToolCallContent::Terminal(model::TerminalToolCallContent::new(terminal_id))]
    }

    #[test]
    fn completed_execute_update_detaches_terminal_subscription() {
        let mut app = App::test_default();
        let tool_id = "tool-1";
        app.messages.push(ChatMessage {
            role: MessageRole::Assistant,
            blocks: vec![MessageBlock::ToolCall(Box::new(make_bash_tool_call(
                tool_id,
                model::ToolCallStatus::InProgress,
                Some("term-1"),
            )))],
            usage: None,
        });
        app.index_tool_call(tool_id.to_owned(), 0, 0);
        app.sync_terminal_tool_call("term-1".to_owned(), 0, 0);

        let update = model::ToolCallUpdate::new(
            tool_id,
            model::ToolCallUpdateFields::new()
                .status(model::ToolCallStatus::Completed)
                .raw_output(serde_json::Value::String("done".to_owned())),
        );

        handle_tool_call_update_session(&mut app, &update);

        let MessageBlock::ToolCall(tc) = &app.messages[0].blocks[0] else {
            panic!("expected tool call block");
        };
        assert_eq!(tc.status, model::ToolCallStatus::Completed);
        assert_eq!(tc.terminal_id, None);
        assert_eq!(tc.terminal_output.as_deref(), Some("done"));
        assert!(app.terminal_tool_calls.is_empty());
        assert!(app.terminal_tool_call_membership.is_empty());
    }

    #[test]
    fn repeated_terminal_updates_do_not_duplicate_subscription() {
        let mut app = App::test_default();
        let tool_id = "tool-1";
        app.messages.push(ChatMessage {
            role: MessageRole::Assistant,
            blocks: vec![MessageBlock::ToolCall(Box::new(make_bash_tool_call(
                tool_id,
                model::ToolCallStatus::InProgress,
                None,
            )))],
            usage: None,
        });
        app.index_tool_call(tool_id.to_owned(), 0, 0);

        let update = model::ToolCallUpdate::new(
            tool_id,
            model::ToolCallUpdateFields::new().content(terminal_content("term-1")),
        );

        handle_tool_call_update_session(&mut app, &update);
        handle_tool_call_update_session(&mut app, &update);

        assert_eq!(app.terminal_tool_calls.len(), 1);
        assert_eq!(app.terminal_tool_call_membership.len(), 1);
        assert_eq!(app.terminal_tool_calls[0].terminal_id, "term-1");
    }

    #[test]
    fn terminal_update_replaces_stale_subscription_for_same_tool_call() {
        let mut app = App::test_default();
        let tool_id = "tool-1";
        app.messages.push(ChatMessage {
            role: MessageRole::Assistant,
            blocks: vec![MessageBlock::ToolCall(Box::new(make_bash_tool_call(
                tool_id,
                model::ToolCallStatus::InProgress,
                Some("term-1"),
            )))],
            usage: None,
        });
        app.index_tool_call(tool_id.to_owned(), 0, 0);
        app.sync_terminal_tool_call("term-1".to_owned(), 0, 0);

        let update = model::ToolCallUpdate::new(
            tool_id,
            model::ToolCallUpdateFields::new().content(terminal_content("term-2")),
        );

        handle_tool_call_update_session(&mut app, &update);

        assert_eq!(app.terminal_tool_calls.len(), 1);
        assert_eq!(app.terminal_tool_call_membership.len(), 1);
        assert_eq!(app.terminal_tool_calls[0].terminal_id, "term-2");
        let MessageBlock::ToolCall(tc) = &app.messages[0].blocks[0] else {
            panic!("expected tool call block");
        };
        assert_eq!(tc.terminal_id.as_deref(), Some("term-2"));
    }
}
