// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::super::connect::take_connection_slot;
use super::super::connect::{SessionStartReason, start_new_session};
use super::super::state::RecentSessionInfo;
use super::super::{
    App, AppStatus, ChatMessage, InvalidationLevel, LoginHint, MessageBlock, MessageRole,
    SystemSeverity, TextBlock,
};
use super::push_system_message_with_severity;
use super::session_reset::{load_resume_history, reset_for_new_session};
use crate::agent::client::AgentConnection;
use crate::agent::events::ServiceStatusSeverity;
use crate::agent::model;
use crate::error::AppError;
use std::rc::Rc;

const TURN_ERROR_INPUT_LOCK_HINT: &str =
    "Input disabled after an error. Press Ctrl+Q to quit and try again.";

pub(super) fn handle_connected_client_event(
    app: &mut App,
    session_id: model::SessionId,
    cwd: String,
    model_name: String,
    available_models: Vec<model::AvailableModel>,
    mode: Option<super::super::ModeState>,
    history_updates: &[model::SessionUpdate],
) {
    if let Some(slot) = take_connection_slot() {
        app.conn = Some(slot.conn);
    }
    apply_session_cwd(app, cwd);
    reset_for_new_session(app, session_id, model_name, mode);
    app.available_models = available_models;
    app.update_welcome_model_once();
    app.sync_welcome_recent_sessions();
    if !history_updates.is_empty() {
        load_resume_history(app, history_updates);
    }
    clear_pending_command(app);
    app.resuming_session_id = None;
    app.rebuild_chat_focus_from_state();
    crate::app::config::refresh_runtime_tabs_for_session_change(app);
}

pub(super) fn handle_sessions_listed_event(
    app: &mut App,
    sessions: Vec<crate::agent::types::SessionListEntry>,
) {
    let pending_title_change = app.config.pending_session_title_change.take();
    app.recent_sessions = sessions
        .into_iter()
        .map(|entry| RecentSessionInfo {
            session_id: entry.session_id,
            summary: entry.summary,
            last_modified_ms: entry.last_modified_ms,
            file_size_bytes: entry.file_size_bytes,
            cwd: entry.cwd,
            git_branch: entry.git_branch,
            custom_title: entry.custom_title,
            first_prompt: entry.first_prompt,
        })
        .collect();
    if let Some(pending_title_change) = pending_title_change {
        let renamed_session_present = app
            .recent_sessions
            .iter()
            .any(|session| session.session_id == pending_title_change.session_id);
        if renamed_session_present {
            app.config.last_error = None;
            app.config.status_message = Some(match pending_title_change.kind {
                crate::app::config::PendingSessionTitleChangeKind::Rename { requested_title } => {
                    match requested_title {
                        Some(title) => format!("Renamed session to {title}"),
                        None => "Cleared session name".to_owned(),
                    }
                }
                crate::app::config::PendingSessionTitleChangeKind::Generate => {
                    "Generated session title".to_owned()
                }
            });
        }
    }
    app.sync_welcome_recent_sessions();
}

pub(super) fn handle_auth_required_event(
    app: &mut App,
    method_name: String,
    method_description: String,
) {
    clear_pending_command(app);
    app.resuming_session_id = None;
    app.login_hint = Some(LoginHint { method_name, method_description });
    app.bump_session_scope_epoch();
    app.clear_session_runtime_identity();
    super::clear_compaction_state(app, false);
    app.last_rate_limit_update = None;
    app.cancelled_turn_pending_hint = false;
    app.pending_cancel_origin = None;
    app.pending_auto_submit_after_cancel = false;
    app.account_info = None;
    app.mcp = super::super::McpState::default();
    app.config.pending_session_title_change = None;
    crate::app::usage::reset_for_session_change(app);
    app.finalize_turn_runtime_artifacts(model::ToolCallStatus::Failed);
    app.clear_active_turn_assistant();
}

pub(super) fn handle_connection_failed_event(app: &mut App, msg: &str) {
    app.bump_session_scope_epoch();
    app.clear_session_runtime_identity();
    super::clear_compaction_state(app, false);
    app.cancelled_turn_pending_hint = false;
    app.pending_cancel_origin = None;
    app.pending_auto_submit_after_cancel = false;
    app.last_rate_limit_update = None;
    app.account_info = None;
    app.mcp = super::super::McpState::default();
    app.config.pending_session_title_change = None;
    crate::app::usage::reset_for_session_change(app);
    app.resuming_session_id = None;
    app.pending_command_label = None;
    app.pending_command_ack = None;
    app.finalize_turn_runtime_artifacts(model::ToolCallStatus::Failed);
    app.input.clear();
    app.pending_submit = None;
    app.status = AppStatus::Error;
    app.clear_active_turn_assistant();
    push_connection_error_message(app, msg);
}

pub(super) fn handle_slash_command_error_event(app: &mut App, msg: &str) {
    if app.config.pending_session_title_change.take().is_some() {
        app.config.last_error = Some(msg.to_owned());
        app.config.status_message = None;
        app.needs_redraw = true;
        return;
    }
    app.push_message_tracked(ChatMessage {
        role: MessageRole::System(None),
        blocks: vec![MessageBlock::Text(TextBlock::from_complete(msg))],
        usage: None,
    });
    app.enforce_history_retention_tracked();
    app.viewport.engage_auto_scroll();
    clear_pending_command(app);
    app.resuming_session_id = None;
}

pub(super) fn handle_auth_completed_event(app: &mut App, conn: &Rc<AgentConnection>) {
    tracing::info!("Authentication completed via /login");
    app.login_hint = None;
    app.pending_command_label = Some("Starting session...".to_owned());
    app.pending_command_ack = None;
    push_system_message_with_severity(
        app,
        Some(SystemSeverity::Info),
        "Authentication successful. Starting new session...",
    );
    app.force_redraw = true;

    if let Err(e) = start_new_session(app, conn, SessionStartReason::Login) {
        clear_pending_command(app);
        push_system_message_with_severity(
            app,
            Some(SystemSeverity::Error),
            &format!("Failed to start session after login: {e}"),
        );
    }
}

pub(super) fn handle_logout_completed_event(app: &mut App) {
    tracing::info!("Logout completed via /logout");
    // Clear the session and start a new one. The bridge now checks auth
    // during initialization and will fire AuthRequired immediately.
    app.bump_session_scope_epoch();
    app.clear_session_runtime_identity();
    app.account_info = None;
    app.mcp = super::super::McpState::default();
    app.config.pending_session_title_change = None;
    crate::app::usage::reset_for_session_change(app);
    app.force_redraw = true;

    if let Some(ref conn) = app.conn {
        app.pending_command_label = Some("Starting session...".to_owned());
        app.pending_command_ack = None;
        if let Err(e) = start_new_session(app, conn, SessionStartReason::Logout) {
            clear_pending_command(app);
            push_system_message_with_severity(
                app,
                Some(SystemSeverity::Error),
                &format!("Failed to start new session after logout: {e}"),
            );
        }
    } else {
        tracing::warn!("No connection available after logout; cannot start new session");
        clear_pending_command(app);
        push_system_message_with_severity(
            app,
            Some(SystemSeverity::Warning),
            "Logged out, but no connection available to start a new session.",
        );
    }
}

pub(super) fn handle_session_replaced_event(
    app: &mut App,
    session_id: model::SessionId,
    cwd: String,
    model_name: String,
    available_models: Vec<model::AvailableModel>,
    mode: Option<super::super::ModeState>,
    history_updates: &[model::SessionUpdate],
) {
    super::clear_compaction_state(app, false);
    app.pending_cancel_origin = None;
    app.pending_auto_submit_after_cancel = false;
    apply_session_cwd(app, cwd);
    app.available_models = available_models;
    reset_for_new_session(app, session_id, model_name, mode);
    if !history_updates.is_empty() {
        load_resume_history(app, history_updates);
    }
    clear_pending_command(app);
    app.resuming_session_id = None;
    crate::app::config::refresh_runtime_tabs_for_session_change(app);
}

pub(super) fn handle_update_available_event(
    app: &mut App,
    latest_version: &str,
    current_version: &str,
) {
    app.update_check_hint = Some(format!(
        "Update available: v{latest_version} (current v{current_version})  Ctrl+U to hide"
    ));
}

pub(super) fn handle_service_status_event(
    app: &mut App,
    severity: ServiceStatusSeverity,
    message: &str,
) {
    let ui_severity = match severity {
        ServiceStatusSeverity::Warning => SystemSeverity::Warning,
        ServiceStatusSeverity::Error => SystemSeverity::Error,
    };
    push_system_message_with_severity(app, Some(ui_severity), message);
}

pub(super) fn handle_fatal_error_event(app: &mut App, error: AppError) {
    app.finalize_turn_runtime_artifacts(model::ToolCallStatus::Failed);
    app.clear_active_turn_assistant();
    app.exit_error = Some(error);
    app.should_quit = true;
    app.status = AppStatus::Error;
    app.pending_submit = None;
    app.pending_command_label = None;
    app.pending_command_ack = None;
}

/// Clear the `CommandPending` state and restore `Ready`.
pub(super) fn clear_pending_command(app: &mut App) {
    app.pending_command_label = None;
    app.pending_command_ack = None;
    app.status = AppStatus::Ready;
}

fn push_connection_error_message(app: &mut App, error: &str) {
    let message = format!("Connection failed: {error}\n\n{TURN_ERROR_INPUT_LOCK_HINT}");
    push_system_message_with_severity(app, None, &message);
}

fn shorten_cwd_display(cwd_raw: &str) -> String {
    if let Some(home) = dirs::home_dir() {
        let home_str = home.to_string_lossy();
        if cwd_raw.starts_with(home_str.as_ref()) {
            return format!("~{}", &cwd_raw[home_str.len()..]);
        }
    }
    cwd_raw.to_owned()
}

fn sync_welcome_cwd(app: &mut App) {
    let Some(first) = app.messages.first_mut() else {
        return;
    };
    if !matches!(first.role, MessageRole::Welcome) {
        return;
    }
    let Some(MessageBlock::Welcome(welcome)) = first.blocks.first_mut() else {
        return;
    };
    welcome.cwd.clone_from(&app.cwd);
    welcome.cache.invalidate();
    app.sync_render_cache_slot(0, 0);
    app.recompute_message_retained_bytes(0);
    app.invalidate_layout(InvalidationLevel::MessagesFrom(0));
}

pub(super) fn apply_session_cwd(app: &mut App, cwd_raw: String) {
    app.cwd_raw = cwd_raw;
    app.cwd = shorten_cwd_display(&app.cwd_raw);
    app.refresh_git_branch();
    sync_welcome_cwd(app);
    app.reconcile_trust_state_from_preferences_and_cwd();
}
