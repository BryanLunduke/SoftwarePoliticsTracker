// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::{App, AppStatus, CancelOrigin, ChatMessage, MessageBlock, MessageRole, TextBlock};
use crate::agent::events::ClientEvent;
use crate::agent::model;
use crate::app::slash;

pub(super) fn submit_input(app: &mut App) {
    if matches!(app.status, AppStatus::Connecting | AppStatus::CommandPending | AppStatus::Error) {
        return;
    }

    // Dismiss any open mention dropdown
    app.mention = None;
    app.slash = None;
    app.subagent = None;

    // No connection yet - can't submit
    let text = app.input.text();
    if text.trim().is_empty() {
        return;
    }

    // `/cancel` is an explicit control action: execute immediately.
    if slash::is_cancel_command(&text) {
        app.pending_auto_submit_after_cancel = false;
        app.input.clear();
        dispatch_submission(app, text);
        return;
    }

    // While a turn is active, keep the current draft text in the input and
    // only request cancellation of the running turn.
    if is_turn_busy(app) {
        match request_cancel(app, CancelOrigin::AutoQueue) {
            Ok(()) => {
                app.pending_auto_submit_after_cancel = true;
            }
            Err(message) => {
                app.pending_auto_submit_after_cancel = false;
                tracing::error!("Failed to request cancel for deferred submit: {message}");
            }
        }
        return;
    }

    app.pending_auto_submit_after_cancel = false;
    app.input.clear();
    dispatch_submission(app, text);
}

fn is_turn_busy(app: &App) -> bool {
    matches!(app.status, AppStatus::Thinking | AppStatus::Running)
        || app.pending_cancel_origin.is_some()
        || app.is_compacting
}

pub(super) fn request_cancel(app: &mut App, origin: CancelOrigin) -> Result<(), String> {
    if matches!(origin, CancelOrigin::Manual) {
        app.pending_auto_submit_after_cancel = false;
    }

    if !matches!(app.status, AppStatus::Thinking | AppStatus::Running) {
        return Ok(());
    }

    if let Some(existing_origin) = app.pending_cancel_origin {
        if matches!(existing_origin, CancelOrigin::AutoQueue)
            && matches!(origin, CancelOrigin::Manual)
        {
            app.pending_cancel_origin = Some(CancelOrigin::Manual);
            app.cancelled_turn_pending_hint = true;
        }
        return Ok(());
    }

    let Some(ref conn) = app.conn else {
        return Err("not connected yet".to_owned());
    };
    let Some(sid) = app.session_id.clone() else {
        return Err("no active session".to_owned());
    };

    conn.cancel(sid.to_string()).map_err(|e| e.to_string())?;
    app.pending_cancel_origin = Some(origin);
    app.cancelled_turn_pending_hint = matches!(origin, CancelOrigin::Manual);
    let _ = app.event_tx.send(ClientEvent::TurnCancelled);
    Ok(())
}

pub(super) fn maybe_auto_submit_after_cancel(app: &mut App) {
    if !app.pending_auto_submit_after_cancel {
        return;
    }
    if !matches!(app.status, AppStatus::Ready) || app.pending_cancel_origin.is_some() {
        return;
    }
    if app.input.text().trim().is_empty() {
        app.pending_auto_submit_after_cancel = false;
        return;
    }
    app.pending_auto_submit_after_cancel = false;
    submit_input(app);
}

fn dispatch_submission(app: &mut App, text: String) {
    if slash::try_handle_submit(app, &text) {
        return;
    }
    dispatch_prompt_turn(app, text);
}

fn dispatch_prompt_turn(app: &mut App, text: String) {
    // New turn started by user input: force-stop stale tool calls from older turns
    // so their spinners don't continue during this turn.
    let _ = app.finalize_in_progress_tool_calls(model::ToolCallStatus::Failed);

    let Some(conn) = app.conn.clone() else { return };
    let Some(sid) = app.session_id.clone() else {
        return;
    };

    app.push_message_tracked(ChatMessage {
        role: MessageRole::User,
        blocks: vec![MessageBlock::Text(TextBlock::from_complete(&text))],
        usage: None,
    });
    // Create empty assistant message immediately -- message.rs shows thinking indicator
    app.push_message_tracked(ChatMessage {
        role: MessageRole::Assistant,
        blocks: Vec::new(),
        usage: None,
    });
    app.bind_active_turn_assistant_to_tail();
    app.enforce_history_retention_tracked();
    app.status = AppStatus::Thinking;
    app.viewport.engage_auto_scroll();

    let tx = app.event_tx.clone();
    match conn.prompt_text(sid.to_string(), text) {
        Ok(resp) => {
            tracing::debug!("Prompt dispatched: stop_reason={:?}", resp.stop_reason);
        }
        Err(e) => {
            let _ = tx.send(ClientEvent::TurnError(e.to_string()));
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::agent::wire::BridgeCommand;
    use crate::app::ActiveView;

    fn app_with_connection()
    -> (App, tokio::sync::mpsc::UnboundedReceiver<crate::agent::wire::CommandEnvelope>) {
        let mut app = App::test_default();
        let (tx, rx) = tokio::sync::mpsc::unbounded_channel();
        app.conn = Some(std::rc::Rc::new(crate::agent::client::AgentConnection::new(tx)));
        app.session_id = Some(model::SessionId::new("session-1"));
        (app, rx)
    }

    #[test]
    fn submit_input_while_running_keeps_input_and_requests_cancel() {
        let (mut app, mut rx) = app_with_connection();
        app.status = AppStatus::Running;
        app.input.set_text("queued prompt");

        submit_input(&mut app);

        assert_eq!(app.input.text(), "queued prompt");
        assert_eq!(app.pending_cancel_origin, Some(CancelOrigin::AutoQueue));
        assert!(app.pending_auto_submit_after_cancel);
        assert!(matches!(app.status, AppStatus::Running));
        assert!(app.messages.is_empty());
        let envelope = rx.try_recv().expect("cancel command should be sent");
        assert!(matches!(
            envelope.command,
            BridgeCommand::CancelTurn { session_id } if session_id == "session-1"
        ));
    }

    #[test]
    fn manual_cancel_promotes_existing_auto_cancel() {
        let (mut app, mut rx) = app_with_connection();
        app.status = AppStatus::Thinking;
        app.pending_auto_submit_after_cancel = true;

        request_cancel(&mut app, CancelOrigin::AutoQueue).expect("auto cancel request");
        request_cancel(&mut app, CancelOrigin::Manual).expect("manual cancel request");

        assert_eq!(app.pending_cancel_origin, Some(CancelOrigin::Manual));
        assert!(app.cancelled_turn_pending_hint);
        assert!(!app.pending_auto_submit_after_cancel);
        let envelope = rx.try_recv().expect("single cancel command should be sent");
        assert!(matches!(
            envelope.command,
            BridgeCommand::CancelTurn { session_id } if session_id == "session-1"
        ));
        assert!(rx.try_recv().is_err(), "manual promotion should not send second cancel");
    }

    #[test]
    fn manual_cancel_prevents_later_auto_submit_after_cancel() {
        let (mut app, mut rx) = app_with_connection();
        app.status = AppStatus::Running;
        app.input.set_text("draft");

        submit_input(&mut app);
        assert_eq!(app.pending_cancel_origin, Some(CancelOrigin::AutoQueue));
        assert!(app.pending_auto_submit_after_cancel);
        let cancel = rx.try_recv().expect("cancel command should be sent");
        assert!(matches!(
            cancel.command, BridgeCommand::CancelTurn { session_id } if session_id == "session-1"
        ));

        request_cancel(&mut app, CancelOrigin::Manual).expect("manual cancel request");
        assert_eq!(app.pending_cancel_origin, Some(CancelOrigin::Manual));
        assert!(!app.pending_auto_submit_after_cancel);

        app.status = AppStatus::Ready;
        app.pending_cancel_origin = None;
        maybe_auto_submit_after_cancel(&mut app);

        assert_eq!(app.input.text(), "draft");
        assert!(matches!(app.status, AppStatus::Ready));
        assert!(app.messages.is_empty());
        assert!(rx.try_recv().is_err(), "manual cancel should suppress queued prompt submit");
    }

    #[test]
    fn submit_input_with_pending_cancel_keeps_input_and_sends_no_second_cancel() {
        let (mut app, mut rx) = app_with_connection();
        app.status = AppStatus::Running;
        app.input.set_text("draft");

        submit_input(&mut app);
        submit_input(&mut app);

        assert_eq!(app.input.text(), "draft");
        assert_eq!(app.pending_cancel_origin, Some(CancelOrigin::AutoQueue));
        assert!(app.pending_auto_submit_after_cancel);
        let envelope = rx.try_recv().expect("first cancel command should be sent");
        assert!(matches!(
            envelope.command, BridgeCommand::CancelTurn { session_id } if session_id == "session-1"
        ));
        assert!(rx.try_recv().is_err(), "second submit should not send extra cancel");
    }

    #[test]
    fn submit_input_cancel_command_requests_manual_cancel() {
        let (mut app, mut rx) = app_with_connection();
        app.status = AppStatus::Running;
        app.input.set_text("/cancel");

        submit_input(&mut app);

        assert!(app.input.text().is_empty());
        assert_eq!(app.pending_cancel_origin, Some(CancelOrigin::Manual));
        let envelope = rx.try_recv().expect("cancel command should be sent");
        assert!(matches!(
            envelope.command,
            BridgeCommand::CancelTurn { session_id } if session_id == "session-1"
        ));
    }

    #[test]
    fn auto_submit_dispatches_draft_once_ready() {
        let (mut app, mut rx) = app_with_connection();
        app.status = AppStatus::Running;
        app.input.set_text("send after cancel");

        submit_input(&mut app);
        assert!(app.pending_auto_submit_after_cancel);
        let cancel = rx.try_recv().expect("cancel command should be sent");
        assert!(matches!(
            cancel.command, BridgeCommand::CancelTurn { session_id } if session_id == "session-1"
        ));

        app.status = AppStatus::Ready;
        app.pending_cancel_origin = None;
        maybe_auto_submit_after_cancel(&mut app);

        assert!(!app.pending_auto_submit_after_cancel);
        assert!(app.input.text().is_empty());
        assert!(matches!(app.status, AppStatus::Thinking));
        assert_eq!(app.messages.len(), 2);
        let prompt = rx.try_recv().expect("prompt command should be sent");
        assert!(matches!(
            prompt.command,
            BridgeCommand::Prompt { session_id, .. } if session_id == "session-1"
        ));
    }

    #[test]
    fn auto_submit_opens_config_only_after_cancel_finishes() {
        let (mut app, mut rx) = app_with_connection();
        let dir = tempfile::tempdir().expect("tempdir");
        app.settings_home_override = Some(dir.path().to_path_buf());
        app.cwd_raw = dir.path().to_string_lossy().to_string();
        app.status = AppStatus::Running;
        app.input.set_text("/config");

        submit_input(&mut app);

        assert_eq!(app.active_view, ActiveView::Chat);
        assert_eq!(app.input.text(), "/config");
        assert_eq!(app.pending_cancel_origin, Some(CancelOrigin::AutoQueue));
        assert!(app.pending_auto_submit_after_cancel);
        let cancel = rx.try_recv().expect("cancel command should be sent");
        assert!(matches!(
            cancel.command, BridgeCommand::CancelTurn { session_id } if session_id == "session-1"
        ));

        app.status = AppStatus::Ready;
        app.pending_cancel_origin = None;
        maybe_auto_submit_after_cancel(&mut app);

        assert!(!app.pending_auto_submit_after_cancel);
        assert_eq!(app.active_view, ActiveView::Config);
        assert!(app.input.text().is_empty());
        assert!(matches!(app.status, AppStatus::Ready));
        assert!(rx.try_recv().is_err(), "config open should not dispatch a prompt turn");
    }

    #[test]
    fn dispatch_prompt_turn_without_session_id_leaves_state_unchanged() {
        let mut app = App::test_default();
        let (tx, _rx) = tokio::sync::mpsc::unbounded_channel();
        app.conn = Some(std::rc::Rc::new(crate::agent::client::AgentConnection::new(tx)));
        app.status = AppStatus::Ready;

        dispatch_prompt_turn(&mut app, "hello".into());

        assert!(app.messages.is_empty());
        assert!(matches!(app.status, AppStatus::Ready));
    }
}
