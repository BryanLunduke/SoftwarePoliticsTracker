use super::{App, session, turn};
use crate::agent::events::ClientEvent;

#[allow(clippy::too_many_lines)]
pub fn handle_client_event(app: &mut App, event: ClientEvent) {
    app.needs_redraw = true;
    match event {
        ClientEvent::SessionUpdate(update) => super::handle_session_update_event(app, update),
        ClientEvent::PermissionRequest { request, response_tx } => {
            turn::handle_permission_request_event(app, request, response_tx);
        }
        ClientEvent::QuestionRequest { request, response_tx } => {
            turn::handle_question_request_event(app, request, response_tx);
        }
        ClientEvent::McpElicitationRequest { request } => {
            crate::app::config::present_mcp_elicitation_request(app, request);
        }
        ClientEvent::McpAuthRedirect { redirect } => {
            crate::app::config::present_mcp_auth_redirect(app, redirect);
        }
        ClientEvent::McpOperationError { error } => {
            crate::app::config::handle_mcp_operation_error(app, &error);
        }
        ClientEvent::McpElicitationCompleted { elicitation_id, server_name } => {
            crate::app::config::handle_mcp_elicitation_completed(app, &elicitation_id, server_name);
        }
        ClientEvent::TurnCancelled => turn::handle_turn_cancelled_event(app),
        ClientEvent::TurnComplete => turn::handle_turn_complete_event(app),
        ClientEvent::TurnError(msg) => turn::handle_turn_error_event(app, &msg, None),
        ClientEvent::TurnErrorClassified { message, class } => {
            turn::handle_turn_error_event(app, &message, Some(class));
        }
        ClientEvent::Connected {
            session_id,
            cwd,
            model_name,
            available_models,
            mode,
            history_updates,
        } => {
            session::handle_connected_client_event(
                app,
                session_id,
                cwd,
                model_name,
                available_models,
                mode,
                &history_updates,
            );
            crate::app::config::refresh_mcp_snapshot(app);
        }
        ClientEvent::SessionsListed { sessions } => {
            session::handle_sessions_listed_event(app, sessions);
        }
        ClientEvent::AuthRequired { method_name, method_description } => {
            session::handle_auth_required_event(app, method_name, method_description);
        }
        ClientEvent::ConnectionFailed(msg) => {
            session::handle_connection_failed_event(app, &msg);
        }
        ClientEvent::SlashCommandError(msg) => {
            session::handle_slash_command_error_event(app, &msg);
        }
        ClientEvent::SessionReplaced {
            session_id,
            cwd,
            model_name,
            available_models,
            mode,
            history_updates,
        } => {
            session::handle_session_replaced_event(
                app,
                session_id,
                cwd,
                model_name,
                available_models,
                mode,
                &history_updates,
            );
            crate::app::config::refresh_mcp_snapshot(app);
        }
        ClientEvent::UpdateAvailable { latest_version, current_version } => {
            session::handle_update_available_event(app, &latest_version, &current_version);
        }
        ClientEvent::ServiceStatus { severity, message } => {
            session::handle_service_status_event(app, severity, &message);
        }
        ClientEvent::AuthCompleted { conn } => {
            session::handle_auth_completed_event(app, &conn);
        }
        ClientEvent::LogoutCompleted => {
            session::handle_logout_completed_event(app);
        }
        ClientEvent::StatusSnapshotReceived { session_id, account } => {
            if app.session_id.as_ref().map(ToString::to_string).as_deref()
                != Some(session_id.as_str())
            {
                return;
            }
            app.account_info = Some(account);
            app.needs_redraw = true;
        }
        ClientEvent::McpSnapshotReceived { session_id, servers, error } => {
            if app.session_id.as_ref().map(ToString::to_string).as_deref()
                != Some(session_id.as_str())
            {
                return;
            }
            tracing::debug!(
                "received MCP snapshot: servers={} error_present={}",
                servers.len(),
                error.is_some()
            );
            app.mcp.servers = servers;
            app.mcp.in_flight = false;
            app.mcp.last_error = error;
            app.config.mcp_selected_server_index =
                app.config.mcp_selected_server_index.min(app.mcp.servers.len().saturating_sub(1));
            if let Some(overlay) = app.config.mcp_auth_redirect_overlay() {
                let server_name = overlay.redirect.server_name.clone();
                if let Some(server) =
                    app.mcp.servers.iter().find(|server| server.name == server_name)
                    && !matches!(
                        server.status,
                        crate::agent::types::McpServerConnectionStatus::NeedsAuth
                            | crate::agent::types::McpServerConnectionStatus::Pending
                    )
                {
                    if matches!(
                        server.status,
                        crate::agent::types::McpServerConnectionStatus::Connected
                    ) {
                        app.config.status_message =
                            Some(format!("{} authenticated successfully.", server.name));
                        app.config.last_error = None;
                    }
                    app.config.overlay = None;
                }
            }
        }
        ClientEvent::UsageRefreshStarted { epoch } => {
            if app.session_scope_epoch != epoch {
                return;
            }
            crate::app::usage::apply_refresh_started(app);
        }
        ClientEvent::UsageSnapshotReceived { epoch, snapshot } => {
            if app.session_scope_epoch != epoch {
                return;
            }
            crate::app::usage::apply_refresh_success(app, snapshot);
        }
        ClientEvent::UsageRefreshFailed { epoch, message, source } => {
            if app.session_scope_epoch != epoch {
                return;
            }
            crate::app::usage::apply_refresh_failure(app, message, source);
        }
        ClientEvent::PluginsInventoryUpdated { cwd_raw, snapshot, claude_path } => {
            if app.cwd_raw != cwd_raw {
                return;
            }
            crate::app::plugins::apply_inventory_refresh_success(app, snapshot, claude_path);
        }
        ClientEvent::PluginsInventoryRefreshFailed { cwd_raw, message } => {
            if app.cwd_raw != cwd_raw {
                return;
            }
            crate::app::plugins::apply_inventory_refresh_failure(app, message);
        }
        ClientEvent::PluginsCliActionSucceeded { cwd_raw, result } => {
            if app.cwd_raw != cwd_raw {
                return;
            }
            crate::app::plugins::apply_cli_action_success(app, result);
        }
        ClientEvent::PluginsCliActionFailed { cwd_raw, message } => {
            if app.cwd_raw != cwd_raw {
                return;
            }
            crate::app::plugins::apply_cli_action_failure(app, message);
        }
        ClientEvent::FatalError(error) => session::handle_fatal_error_event(app, error),
    }
}
