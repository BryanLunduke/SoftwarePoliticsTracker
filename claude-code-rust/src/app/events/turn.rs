// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::super::{
    App, AppStatus, CancelOrigin, ChatMessage, FocusTarget, InlinePermission, InlineQuestion,
    InvalidationLevel, MessageBlock, MessageRole, SystemSeverity, TextBlock,
};
use super::clear_compaction_state;
use super::rate_limit::format_rate_limit_summary;
use crate::agent::error_handling::{TurnErrorClass, classify_turn_error, summarize_internal_error};
use crate::agent::model;
use std::collections::BTreeSet;

const CONVERSATION_INTERRUPTED_HINT: &str =
    "Conversation interrupted. Tell the model how to proceed.";
const TURN_ERROR_INPUT_LOCK_HINT: &str =
    "Input disabled after an error. Press Ctrl+Q to quit and try again.";
const PLAN_LIMIT_NEXT_STEPS_HINT: &str = "Next steps:\n\
1. Wait a few minutes and retry.\n\
2. Reduce request size or request frequency.\n\
3. Check quota/billing for your account or switch plans.";
const AUTH_REQUIRED_NEXT_STEPS_HINT: &str = "Authentication required. Type /login to authenticate, or run `claude auth login` in a terminal.";

#[derive(Clone, Copy)]
struct TurnExitState {
    tail_assistant_idx: Option<usize>,
    turn_was_active: bool,
    cancelled_requested: Option<CancelOrigin>,
    show_interrupted_hint: bool,
}

pub(super) fn handle_permission_request_event(
    app: &mut App,
    request: model::RequestPermissionRequest,
    response_tx: tokio::sync::oneshot::Sender<model::RequestPermissionResponse>,
) {
    let tool_id = request.tool_call.tool_call_id.clone();
    let options = request.options.clone();

    let Some((mi, bi)) = app.lookup_tool_call(&tool_id) else {
        tracing::warn!("Permission request for unknown tool call: {tool_id}; auto-rejecting");
        reject_permission_request(response_tx, &options);
        return;
    };

    if app.pending_interaction_ids.iter().any(|id| id == &tool_id) {
        tracing::warn!(
            "Duplicate permission request for tool call: {tool_id}; auto-rejecting duplicate"
        );
        reject_permission_request(response_tx, &options);
        return;
    }

    let mut layout_dirty = false;
    if let Some(MessageBlock::ToolCall(tc)) =
        app.messages.get_mut(mi).and_then(|m| m.blocks.get_mut(bi))
    {
        let tc = tc.as_mut();
        let is_first = app.pending_interaction_ids.is_empty();
        tc.pending_permission = Some(InlinePermission {
            options: request.options,
            response_tx,
            selected_index: 0,
            focused: is_first,
        });
        tc.mark_tool_call_layout_dirty();
        layout_dirty = true;
        app.pending_interaction_ids.push(tool_id);
        app.claim_focus_target(FocusTarget::Permission);
        app.viewport.engage_auto_scroll();
        app.notifications.notify(
            app.config.preferred_notification_channel_effective(),
            super::super::notify::NotifyEvent::PermissionRequired,
        );
    } else {
        tracing::warn!("Permission request for non-tool block index: {tool_id}; auto-rejecting");
        reject_permission_request(response_tx, &options);
    }

    if layout_dirty {
        app.sync_render_cache_slot(mi, bi);
        app.recompute_message_retained_bytes(mi);
        app.invalidate_layout(InvalidationLevel::MessageChanged(mi));
    }
}

pub(super) fn handle_question_request_event(
    app: &mut App,
    request: model::RequestQuestionRequest,
    response_tx: tokio::sync::oneshot::Sender<model::RequestQuestionResponse>,
) {
    let tool_id = request.tool_call.tool_call_id.clone();

    let Some((mi, bi)) = app.lookup_tool_call(&tool_id) else {
        tracing::warn!("Question request for unknown tool call: {tool_id}; auto-cancelling");
        let _ = response_tx
            .send(model::RequestQuestionResponse::new(model::RequestQuestionOutcome::Cancelled));
        return;
    };

    if app.pending_interaction_ids.iter().any(|id| id == &tool_id) {
        tracing::warn!(
            "Duplicate inline interaction request for tool call: {tool_id}; auto-cancelling duplicate"
        );
        let _ = response_tx
            .send(model::RequestQuestionResponse::new(model::RequestQuestionOutcome::Cancelled));
        return;
    }

    let mut layout_dirty = false;
    if let Some(MessageBlock::ToolCall(tc)) =
        app.messages.get_mut(mi).and_then(|m| m.blocks.get_mut(bi))
    {
        let tc = tc.as_mut();
        let is_first = app.pending_interaction_ids.is_empty();
        tc.pending_question = Some(InlineQuestion {
            prompt: request.prompt,
            response_tx,
            focused_option_index: 0,
            selected_option_indices: BTreeSet::new(),
            notes: String::new(),
            notes_cursor: 0,
            editing_notes: false,
            focused: is_first,
            question_index: request.question_index,
            total_questions: request.total_questions,
        });
        tc.mark_tool_call_layout_dirty();
        layout_dirty = true;
        app.pending_interaction_ids.push(tool_id);
        app.claim_focus_target(FocusTarget::Permission);
        app.viewport.engage_auto_scroll();
        app.notifications.notify(
            app.config.preferred_notification_channel_effective(),
            super::super::notify::NotifyEvent::QuestionRequired,
        );
    } else {
        tracing::warn!("Question request for non-tool block index: {tool_id}; auto-cancelling");
        let _ = response_tx
            .send(model::RequestQuestionResponse::new(model::RequestQuestionOutcome::Cancelled));
    }

    if layout_dirty {
        app.sync_render_cache_slot(mi, bi);
        app.recompute_message_retained_bytes(mi);
        app.invalidate_layout(InvalidationLevel::MessageChanged(mi));
    }
}

fn reject_permission_request(
    response_tx: tokio::sync::oneshot::Sender<model::RequestPermissionResponse>,
    options: &[model::PermissionOption],
) {
    if let Some(last_opt) = options.last() {
        let _ = response_tx.send(model::RequestPermissionResponse::new(
            model::RequestPermissionOutcome::Selected(model::SelectedPermissionOutcome::new(
                last_opt.option_id.clone(),
            )),
        ));
    }
}

pub(super) fn handle_turn_cancelled_event(app: &mut App) {
    if app.pending_cancel_origin.is_none() {
        app.pending_cancel_origin = Some(CancelOrigin::Manual);
    }
    app.cancelled_turn_pending_hint =
        matches!(app.pending_cancel_origin, Some(CancelOrigin::Manual));
    let _ = app.finalize_in_progress_tool_calls(model::ToolCallStatus::Failed);
}

fn begin_turn_exit(app: &mut App, emit_manual_compaction_success: bool) -> TurnExitState {
    let state = TurnExitState {
        tail_assistant_idx: app
            .messages
            .iter()
            .rposition(|m| matches!(m.role, MessageRole::Assistant)),
        turn_was_active: matches!(app.status, AppStatus::Thinking | AppStatus::Running),
        cancelled_requested: app.pending_cancel_origin,
        show_interrupted_hint: matches!(app.pending_cancel_origin, Some(CancelOrigin::Manual)),
    };
    clear_compaction_state(app, emit_manual_compaction_success);
    app.pending_cancel_origin = None;
    app.cancelled_turn_pending_hint = false;
    state
}

fn finish_ready_turn_exit(app: &mut App, exit: TurnExitState, tool_status: model::ToolCallStatus) {
    app.finalize_turn_runtime_artifacts(tool_status);
    app.status = AppStatus::Ready;
    app.files_accessed = 0;
    app.refresh_git_branch();

    let removed_tail_assistant = remove_empty_tail_assistant(app, exit.tail_assistant_idx);
    if exit.show_interrupted_hint {
        push_interrupted_hint(app);
    }
    if removed_tail_assistant.is_none()
        && (exit.turn_was_active || exit.cancelled_requested.is_some())
    {
        mark_turn_exit_assistant_layout_dirty(app, exit.tail_assistant_idx);
    }
    app.clear_active_turn_assistant();
}

pub(super) fn handle_turn_complete_event(app: &mut App) {
    let exit = begin_turn_exit(app, true);
    let turn_was_active = exit.turn_was_active;
    let tool_status = if exit.cancelled_requested.is_some() {
        model::ToolCallStatus::Failed
    } else {
        model::ToolCallStatus::Completed
    };
    finish_ready_turn_exit(app, exit, tool_status);
    if turn_was_active {
        app.notifications.notify(
            app.config.preferred_notification_channel_effective(),
            super::super::notify::NotifyEvent::TurnComplete,
        );
    }
    if app.active_view == super::super::ActiveView::Chat {
        super::super::input_submit::maybe_auto_submit_after_cancel(app);
    }
}

pub(super) fn handle_turn_error_event(
    app: &mut App,
    msg: &str,
    classified: Option<TurnErrorClass>,
) {
    let exit = begin_turn_exit(app, true);

    if exit.cancelled_requested.is_some() {
        let summary = summarize_internal_error(msg);
        tracing::warn!(
            error_preview = %summary,
            "Turn error suppressed after cancellation request"
        );
        app.pending_submit = None;
        finish_ready_turn_exit(app, exit, model::ToolCallStatus::Failed);
        if app.active_view == super::super::ActiveView::Chat {
            super::super::input_submit::maybe_auto_submit_after_cancel(app);
        }
        return;
    }

    let error_class = classified.unwrap_or_else(|| classify_turn_error(msg));
    tracing::error!("Turn error: {msg}");
    let summary = summarize_internal_error(msg);
    match error_class {
        TurnErrorClass::PlanLimit => {
            tracing::warn!(
                error_preview = %summary,
                "Turn error classified as plan/usage limit"
            );
        }
        TurnErrorClass::AuthRequired => {
            tracing::warn!(
                error_preview = %summary,
                "Turn error indicates authentication is required"
            );
            app.exit_error = Some(crate::error::AppError::AuthRequired);
            app.should_quit = true;
        }
        TurnErrorClass::Internal => {
            tracing::debug!(
                error_preview = %summary,
                "Internal Agent SDK turn error payload"
            );
        }
        TurnErrorClass::Other => {}
    }
    app.finalize_turn_runtime_artifacts(model::ToolCallStatus::Failed);
    app.pending_auto_submit_after_cancel = false;
    app.input.clear();
    app.pending_submit = None;
    app.status = AppStatus::Error;
    let rate_limit_context = if matches!(error_class, TurnErrorClass::PlanLimit) {
        app.last_rate_limit_update
            .clone()
            .filter(|update| !matches!(update.status, model::RateLimitStatus::Allowed))
    } else {
        None
    };
    let removed_tail_assistant = remove_empty_tail_assistant(app, exit.tail_assistant_idx);
    push_turn_error_message(app, msg, error_class, rate_limit_context.as_ref());
    if removed_tail_assistant.is_none() && exit.turn_was_active {
        mark_turn_exit_assistant_layout_dirty(app, exit.tail_assistant_idx);
    }
    app.clear_active_turn_assistant();
}

fn push_interrupted_hint(app: &mut App) {
    app.push_message_tracked(ChatMessage {
        role: MessageRole::System(Some(SystemSeverity::Info)),
        blocks: vec![MessageBlock::Text(TextBlock::from_complete(CONVERSATION_INTERRUPTED_HINT))],
        usage: None,
    });
    app.enforce_history_retention_tracked();
    app.viewport.engage_auto_scroll();
}

fn remove_empty_tail_assistant(app: &mut App, idx: Option<usize>) -> Option<usize> {
    let idx = idx?;
    let should_remove = app
        .messages
        .get(idx)
        .is_some_and(|msg| matches!(msg.role, MessageRole::Assistant) && msg.blocks.is_empty());
    if !should_remove {
        return None;
    }
    app.remove_message_tracked(idx)?;
    Some(idx)
}

fn mark_turn_exit_assistant_layout_dirty(app: &mut App, idx: Option<usize>) {
    let Some(idx) = idx else {
        return;
    };
    if app.messages.get(idx).is_some_and(|msg| matches!(msg.role, MessageRole::Assistant)) {
        app.invalidate_layout(InvalidationLevel::MessageChanged(idx));
    }
}

fn push_turn_error_message(
    app: &mut App,
    error: &str,
    class: TurnErrorClass,
    rate_limit_context: Option<&model::RateLimitUpdate>,
) {
    let base_message = match class {
        TurnErrorClass::PlanLimit => {
            let summary = summarize_internal_error(error);
            format!(
                "Turn blocked by account or plan limits: {summary}\n\n{PLAN_LIMIT_NEXT_STEPS_HINT}\n\n{TURN_ERROR_INPUT_LOCK_HINT}"
            )
        }
        TurnErrorClass::AuthRequired => {
            format!("{AUTH_REQUIRED_NEXT_STEPS_HINT}\n\n{TURN_ERROR_INPUT_LOCK_HINT}")
        }
        TurnErrorClass::Internal | TurnErrorClass::Other => {
            format!("Turn failed: {error}\n\n{TURN_ERROR_INPUT_LOCK_HINT}")
        }
    };
    let (severity, message) = if matches!(class, TurnErrorClass::PlanLimit)
        && let Some(update) = rate_limit_context
    {
        let prefix = format_rate_limit_summary(update);
        let severity = match update.status {
            model::RateLimitStatus::AllowedWarning => Some(SystemSeverity::Warning),
            model::RateLimitStatus::Rejected | model::RateLimitStatus::Allowed => None,
        };
        (severity, format!("{prefix}\n\n{base_message}"))
    } else {
        (None, base_message)
    };
    super::push_system_message_with_severity(app, severity, &message);
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::app::App;

    fn empty_assistant_message() -> ChatMessage {
        ChatMessage { role: MessageRole::Assistant, blocks: Vec::new(), usage: None }
    }

    fn user_message(text: &str) -> ChatMessage {
        ChatMessage {
            role: MessageRole::User,
            blocks: vec![MessageBlock::Text(TextBlock::from_complete(text))],
            usage: None,
        }
    }

    #[test]
    fn turn_complete_removes_empty_tail_assistant() {
        let mut app = App::test_default();
        app.status = AppStatus::Thinking;
        app.messages.push(user_message("hello"));
        app.messages.push(empty_assistant_message());

        handle_turn_complete_event(&mut app);

        assert_eq!(app.messages.len(), 1);
        assert!(matches!(app.messages[0].role, MessageRole::User));
    }

    #[test]
    fn cancelled_turn_error_removes_empty_tail_assistant_before_hint() {
        let mut app = App::test_default();
        app.status = AppStatus::Thinking;
        app.pending_cancel_origin = Some(CancelOrigin::Manual);
        app.messages.push(user_message("hello"));
        app.messages.push(empty_assistant_message());

        handle_turn_error_event(&mut app, "cancelled", None);

        assert_eq!(app.messages.len(), 2);
        assert!(matches!(app.messages[0].role, MessageRole::User));
        assert!(matches!(app.messages[1].role, MessageRole::System(Some(SystemSeverity::Info))));
    }

    #[test]
    fn turn_error_removes_empty_tail_assistant_before_error_message() {
        let mut app = App::test_default();
        app.status = AppStatus::Thinking;
        app.messages.push(user_message("hello"));
        app.messages.push(empty_assistant_message());

        handle_turn_error_event(&mut app, "boom", None);

        assert_eq!(app.messages.len(), 2);
        assert!(matches!(app.messages[0].role, MessageRole::User));
        assert!(matches!(app.messages[1].role, MessageRole::System(None)));
    }
}
