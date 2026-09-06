// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

//! Bridge event dispatch: routes incoming `BridgeEvent` envelopes to appropriate
//! `ClientEvent` messages, and handles permission request/response forwarding.

use crate::agent::error_handling::parse_turn_error_class;
use crate::agent::events::ClientEvent;
use crate::agent::model;
use crate::agent::types;
use crate::agent::wire::{BridgeCommand, CommandEnvelope, EventEnvelope};
use crate::error::AppError;
use tokio::sync::mpsc;

use super::bridge_lifecycle::emit_connection_failed;
use super::type_converters::{
    convert_mode_state, map_available_models, map_permission_request, map_question_request,
    map_session_update,
};

struct ConnectedEventData {
    session_id: String,
    cwd: String,
    model_name: String,
    available_models: Vec<types::AvailableModel>,
    mode: Option<types::ModeState>,
    history_updates: Option<Vec<types::SessionUpdate>>,
}

#[allow(clippy::too_many_lines)]
pub(super) fn handle_bridge_event(
    event_tx: &mpsc::UnboundedSender<ClientEvent>,
    cmd_tx: &mpsc::UnboundedSender<CommandEnvelope>,
    connected_once: &mut bool,
    resume_requested: bool,
    envelope: EventEnvelope,
) {
    match envelope.event {
        crate::agent::wire::BridgeEvent::Connected {
            session_id,
            cwd,
            model_name,
            available_models,
            mode,
            history_updates,
        } => {
            handle_connected_event(
                event_tx,
                connected_once,
                ConnectedEventData {
                    session_id,
                    cwd,
                    model_name,
                    available_models,
                    mode,
                    history_updates,
                },
            );
        }
        crate::agent::wire::BridgeEvent::AuthRequired { method_name, method_description } => {
            tracing::warn!(
                "bridge reported auth required: method={} desc={}",
                method_name,
                method_description
            );
            let _ = event_tx.send(ClientEvent::AuthRequired { method_name, method_description });
        }
        crate::agent::wire::BridgeEvent::ConnectionFailed { message } => {
            tracing::error!("bridge connection_failed: {message}");
            emit_connection_failed(event_tx, message, AppError::ConnectionFailed);
        }
        crate::agent::wire::BridgeEvent::SessionUpdate { update, .. } => {
            if let Some(update) = map_session_update(update) {
                let _ = event_tx.send(ClientEvent::SessionUpdate(update));
            }
        }
        crate::agent::wire::BridgeEvent::PermissionRequest { session_id, request } => {
            handle_permission_request_event(event_tx, cmd_tx, session_id, request);
        }
        crate::agent::wire::BridgeEvent::QuestionRequest { session_id, request } => {
            handle_question_request_event(event_tx, cmd_tx, session_id, request);
        }
        crate::agent::wire::BridgeEvent::ElicitationRequest { session_id, request } => {
            handle_elicitation_request_event(event_tx, &session_id, request);
        }
        crate::agent::wire::BridgeEvent::ElicitationComplete {
            elicitation_id,
            server_name,
            ..
        } => {
            let _ =
                event_tx.send(ClientEvent::McpElicitationCompleted { elicitation_id, server_name });
        }
        crate::agent::wire::BridgeEvent::McpAuthRedirect { redirect, .. } => {
            let _ = event_tx.send(ClientEvent::McpAuthRedirect { redirect });
        }
        crate::agent::wire::BridgeEvent::McpOperationError { error, .. } => {
            tracing::warn!(
                "bridge mcp_operation_error: operation={} server={} message={}",
                error.operation,
                error.server_name.as_deref().unwrap_or("<none>"),
                error.message
            );
            let _ = event_tx.send(ClientEvent::McpOperationError { error });
        }
        crate::agent::wire::BridgeEvent::TurnComplete { .. } => {
            let _ = event_tx.send(ClientEvent::TurnComplete);
        }
        crate::agent::wire::BridgeEvent::TurnError { message, error_kind, .. } => {
            tracing::warn!("bridge turn_error: {message}");
            if let Some(class) = error_kind.as_deref().and_then(parse_turn_error_class) {
                let _ = event_tx.send(ClientEvent::TurnErrorClassified { message, class });
            } else {
                let _ = event_tx.send(ClientEvent::TurnError(message));
            }
        }
        crate::agent::wire::BridgeEvent::SlashError { message, .. } => {
            tracing::warn!("bridge slash_error: {message}");
            if resume_requested
                && !*connected_once
                && message.to_ascii_lowercase().contains("unknown session")
            {
                let _ = event_tx.send(ClientEvent::FatalError(AppError::SessionNotFound));
                return;
            }
            let _ = event_tx.send(ClientEvent::SlashCommandError(message));
        }
        crate::agent::wire::BridgeEvent::SessionReplaced {
            session_id,
            cwd,
            model_name,
            available_models,
            mode,
            history_updates,
        } => {
            let history_updates = history_updates
                .unwrap_or_default()
                .into_iter()
                .filter_map(map_session_update)
                .collect();
            let _ = event_tx.send(ClientEvent::SessionReplaced {
                session_id: model::SessionId::new(session_id),
                cwd,
                model_name,
                available_models: map_available_models(available_models),
                mode: mode.map(convert_mode_state),
                history_updates,
            });
        }
        crate::agent::wire::BridgeEvent::SessionsListed { sessions } => {
            let _ = event_tx.send(ClientEvent::SessionsListed { sessions });
        }
        crate::agent::wire::BridgeEvent::Initialized { .. } => {}
        crate::agent::wire::BridgeEvent::StatusSnapshot { session_id, account } => {
            let _ = event_tx.send(ClientEvent::StatusSnapshotReceived { session_id, account });
        }
        crate::agent::wire::BridgeEvent::McpSnapshot { session_id, servers, error } => {
            let _ = event_tx.send(ClientEvent::McpSnapshotReceived { session_id, servers, error });
        }
    }
}

fn handle_connected_event(
    event_tx: &mpsc::UnboundedSender<ClientEvent>,
    connected_once: &mut bool,
    event: ConnectedEventData,
) {
    tracing::info!(
        "bridge connected: session_id={} cwd={} model={}",
        event.session_id,
        event.cwd,
        event.model_name
    );
    let mode = event.mode.map(convert_mode_state);
    let history_updates = event
        .history_updates
        .unwrap_or_default()
        .into_iter()
        .filter_map(map_session_update)
        .collect();
    if *connected_once {
        let _ = event_tx.send(ClientEvent::SessionReplaced {
            session_id: model::SessionId::new(event.session_id),
            cwd: event.cwd,
            model_name: event.model_name,
            available_models: map_available_models(event.available_models),
            mode,
            history_updates,
        });
    } else {
        *connected_once = true;
        let _ = event_tx.send(ClientEvent::Connected {
            session_id: model::SessionId::new(event.session_id),
            cwd: event.cwd,
            model_name: event.model_name,
            available_models: map_available_models(event.available_models),
            mode,
            history_updates,
        });
    }
}

fn handle_permission_request_event(
    event_tx: &mpsc::UnboundedSender<ClientEvent>,
    cmd_tx: &mpsc::UnboundedSender<CommandEnvelope>,
    session_id: String,
    request: types::PermissionRequest,
) {
    tracing::debug!(
        "bridge permission_request: session_id={} tool_call_id={} options={}",
        session_id,
        request.tool_call.tool_call_id,
        request.options.len()
    );
    let (request, tool_call_id) = map_permission_request(&session_id, request);
    let (response_tx, response_rx) = tokio::sync::oneshot::channel();
    if event_tx.send(ClientEvent::PermissionRequest { request, response_tx }).is_ok() {
        spawn_permission_response_forwarder(cmd_tx.clone(), response_rx, session_id, tool_call_id);
    }
}

fn handle_question_request_event(
    event_tx: &mpsc::UnboundedSender<ClientEvent>,
    cmd_tx: &mpsc::UnboundedSender<CommandEnvelope>,
    session_id: String,
    request: types::QuestionRequest,
) {
    tracing::debug!(
        "bridge question_request: session_id={} tool_call_id={} options={}",
        session_id,
        request.tool_call.tool_call_id,
        request.prompt.options.len()
    );
    let (request, tool_call_id) = map_question_request(&session_id, request);
    let (response_tx, response_rx) = tokio::sync::oneshot::channel();
    if event_tx.send(ClientEvent::QuestionRequest { request, response_tx }).is_ok() {
        spawn_question_response_forwarder(cmd_tx.clone(), response_rx, session_id, tool_call_id);
    }
}

fn handle_elicitation_request_event(
    event_tx: &mpsc::UnboundedSender<ClientEvent>,
    session_id: &str,
    request: types::ElicitationRequest,
) {
    tracing::debug!(
        "bridge elicitation_request: session_id={} request_id={} server_name={} mode={:?}",
        session_id,
        request.request_id,
        request.server_name,
        request.mode
    );
    let _ = event_tx.send(ClientEvent::McpElicitationRequest { request });
}

fn spawn_permission_response_forwarder(
    cmd_tx: mpsc::UnboundedSender<CommandEnvelope>,
    response_rx: tokio::sync::oneshot::Receiver<model::RequestPermissionResponse>,
    session_id: String,
    tool_call_id: String,
) {
    tokio::task::spawn_local(async move {
        let Ok(response) = response_rx.await else {
            return;
        };
        let outcome = match response.outcome {
            model::RequestPermissionOutcome::Selected(selected) => {
                let option_id = selected.option_id.clone();
                tracing::debug!(
                    "forward permission_response: session_id={} tool_call_id={} option_id={}",
                    session_id,
                    tool_call_id,
                    option_id
                );
                types::PermissionOutcome::Selected { option_id }
            }
            model::RequestPermissionOutcome::Cancelled => {
                tracing::debug!(
                    "forward permission_response: session_id={} tool_call_id={} outcome=cancelled",
                    session_id,
                    tool_call_id
                );
                types::PermissionOutcome::Cancelled
            }
        };
        let _ = cmd_tx.send(CommandEnvelope {
            request_id: None,
            command: BridgeCommand::PermissionResponse { session_id, tool_call_id, outcome },
        });
    });
}

fn spawn_question_response_forwarder(
    cmd_tx: mpsc::UnboundedSender<CommandEnvelope>,
    response_rx: tokio::sync::oneshot::Receiver<model::RequestQuestionResponse>,
    session_id: String,
    tool_call_id: String,
) {
    tokio::task::spawn_local(async move {
        let Ok(response) = response_rx.await else {
            return;
        };
        let outcome = match response.outcome {
            model::RequestQuestionOutcome::Answered(answered) => {
                tracing::debug!(
                    "forward question_response: session_id={} tool_call_id={} selections={}",
                    session_id,
                    tool_call_id,
                    answered.selected_option_ids.len()
                );
                types::QuestionOutcome::Answered {
                    selected_option_ids: answered.selected_option_ids,
                    annotation: answered.annotation.map(|annotation| types::QuestionAnnotation {
                        preview: annotation.preview,
                        notes: annotation.notes,
                    }),
                }
            }
            model::RequestQuestionOutcome::Cancelled => {
                tracing::debug!(
                    "forward question_response: session_id={} tool_call_id={} outcome=cancelled",
                    session_id,
                    tool_call_id
                );
                types::QuestionOutcome::Cancelled
            }
        };
        let _ = cmd_tx.send(CommandEnvelope {
            request_id: None,
            command: BridgeCommand::QuestionResponse { session_id, tool_call_id, outcome },
        });
    });
}
