// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

//! Slash command executors: dispatching parsed commands to their handler functions.

use super::{
    parse, push_system_message, push_user_message, require_active_session, require_connection,
    set_command_pending,
};
use crate::agent::events::ClientEvent;
use crate::app::connect::{SessionStartReason, resume_session, start_new_session};
use crate::app::events::push_system_message_with_severity;
use crate::app::{App, AppStatus, CancelOrigin, SystemSeverity};

/// Handle slash command submission.
///
/// Returns `true` if the slash input was fully handled and should not be sent as a prompt.
/// Returns `false` when the input should continue through the normal prompt path.
pub fn try_handle_submit(app: &mut App, text: &str) -> bool {
    let Some(parsed) = parse(text) else {
        return false;
    };

    match parsed.name {
        "/cancel" => handle_cancel_submit(app),
        "/compact" => handle_compact_submit(app, &parsed.args),
        "/config" => handle_config_submit(app, &parsed.args),
        "/mcp" => handle_mcp_submit(app, &parsed.args),
        "/plugins" => handle_plugins_submit(app, &parsed.args),
        "/status" => handle_status_submit(app, &parsed.args),
        "/usage" => handle_usage_submit(app, &parsed.args),
        "/login" => handle_login_submit(app, &parsed.args),
        "/logout" => handle_logout_submit(app, &parsed.args),
        "/mode" => handle_mode_submit(app, &parsed.args),
        "/model" => handle_model_submit(app, &parsed.args),
        "/new-session" => handle_new_session_submit(app, &parsed.args),
        "/resume" => handle_resume_submit(app, &parsed.args),
        _ => handle_unknown_submit(app, parsed.name),
    }
}

fn handle_cancel_submit(app: &mut App) -> bool {
    if !matches!(app.status, AppStatus::Thinking | AppStatus::Running) {
        return true;
    }
    if let Err(message) = crate::app::input_submit::request_cancel(app, CancelOrigin::Manual) {
        push_system_message(app, format!("Failed to run /cancel: {message}"));
    }
    true
}

fn handle_compact_submit(app: &mut App, args: &[&str]) -> bool {
    if !args.is_empty() {
        push_system_message(app, "Usage: /compact");
        return true;
    }
    if require_active_session(
        app,
        "Cannot compact: not connected yet.",
        "Cannot compact: no active session.",
    )
    .is_none()
    {
        return true;
    }

    app.is_compacting = true;
    false
}

fn handle_config_submit(app: &mut App, args: &[&str]) -> bool {
    if !args.is_empty() {
        push_system_message(app, "Usage: /config");
        return true;
    }

    if let Err(err) = crate::app::config::open(app) {
        push_system_message(app, format!("Failed to open settings: {err}"));
    }
    true
}

fn handle_plugins_submit(app: &mut App, args: &[&str]) -> bool {
    let _ = args;

    if let Err(err) = crate::app::config::open(app) {
        push_system_message(app, format!("Failed to open plugins: {err}"));
        return true;
    }
    crate::app::config::activate_tab(app, crate::app::ConfigTab::Plugins);
    true
}

fn handle_mcp_submit(app: &mut App, args: &[&str]) -> bool {
    if !args.is_empty() {
        push_system_message(app, "Usage: /mcp");
        return true;
    }

    if let Err(err) = crate::app::config::open(app) {
        push_system_message(app, format!("Failed to open MCP: {err}"));
        return true;
    }
    crate::app::config::activate_tab(app, crate::app::ConfigTab::Mcp);
    true
}

fn handle_status_submit(app: &mut App, args: &[&str]) -> bool {
    if !args.is_empty() {
        push_system_message(app, "Usage: /status");
        return true;
    }

    if let Err(err) = crate::app::config::open(app) {
        push_system_message(app, format!("Failed to open status: {err}"));
        return true;
    }
    crate::app::config::activate_tab(app, crate::app::ConfigTab::Status);
    true
}

fn handle_usage_submit(app: &mut App, args: &[&str]) -> bool {
    if !args.is_empty() {
        push_system_message(app, "Usage: /usage");
        return true;
    }

    if let Err(err) = crate::app::config::open(app) {
        push_system_message(app, format!("Failed to open usage: {err}"));
        return true;
    }
    crate::app::config::activate_tab(app, crate::app::ConfigTab::Usage);
    true
}

fn handle_login_submit(app: &mut App, args: &[&str]) -> bool {
    if !args.is_empty() {
        push_system_message(app, "Usage: /login");
        return true;
    }

    push_user_message(app, "/login");
    tracing::debug!("Handling /login command");

    if crate::app::auth::has_credentials() {
        push_system_message_with_severity(
            app,
            Some(SystemSeverity::Info),
            "Already authenticated. Use /logout first to re-authenticate.",
        );
        return true;
    }

    let Some(claude_path) = resolve_claude_cli(app, "login") else {
        return true;
    };

    set_command_pending(app, "Authenticating...", None);

    let tx = app.event_tx.clone();
    let conn = app.conn.clone();
    tokio::task::spawn_local(async move {
        tracing::debug!("Suspending TUI for claude auth login");
        crate::app::suspend_terminal();

        let result = tokio::process::Command::new(&claude_path)
            .args(["auth", "login"])
            .stdin(std::process::Stdio::inherit())
            .stdout(std::process::Stdio::inherit())
            .stderr(std::process::Stdio::inherit())
            .status()
            .await;

        crate::app::resume_terminal();

        match result {
            Ok(status) => {
                tracing::debug!(
                    success = status.success(),
                    code = ?status.code(),
                    "claude auth login exited"
                );
                if status.success() {
                    if !crate::app::auth::has_credentials() {
                        let _ = tx.send(ClientEvent::SlashCommandError(
                            "Login exited successfully but no credentials were saved. \
                             Try /login again or run `claude auth login` in another terminal."
                                .to_owned(),
                        ));
                        return;
                    }
                    if let Some(conn) = conn {
                        let _ = tx.send(ClientEvent::AuthCompleted { conn });
                    } else {
                        let _ = tx.send(ClientEvent::SlashCommandError(
                            "Login succeeded but no connection available to start a session."
                                .to_owned(),
                        ));
                    }
                } else {
                    let _ = tx.send(ClientEvent::SlashCommandError(format!(
                        "/login failed (exit code: {})",
                        status.code().map_or("unknown".to_owned(), |c| c.to_string())
                    )));
                }
            }
            Err(e) => {
                let _ = tx.send(ClientEvent::SlashCommandError(format!(
                    "Failed to run claude auth login: {e}"
                )));
            }
        }
    });
    true
}

fn handle_logout_submit(app: &mut App, args: &[&str]) -> bool {
    if !args.is_empty() {
        push_system_message(app, "Usage: /logout");
        return true;
    }

    push_user_message(app, "/logout");
    tracing::debug!("Handling /logout command");

    if !crate::app::auth::has_credentials() {
        push_system_message_with_severity(
            app,
            Some(SystemSeverity::Info),
            "Not currently authenticated. Nothing to log out from.",
        );
        return true;
    }

    let Some(claude_path) = resolve_claude_cli(app, "logout") else {
        return true;
    };

    set_command_pending(app, "Signing out...", None);

    let tx = app.event_tx.clone();
    tokio::task::spawn_local(async move {
        tracing::debug!("Suspending TUI for claude auth logout");
        crate::app::suspend_terminal();

        let result = tokio::process::Command::new(&claude_path)
            .args(["auth", "logout"])
            .stdin(std::process::Stdio::inherit())
            .stdout(std::process::Stdio::inherit())
            .stderr(std::process::Stdio::inherit())
            .status()
            .await;

        crate::app::resume_terminal();

        match result {
            Ok(status) => {
                tracing::debug!(
                    success = status.success(),
                    code = ?status.code(),
                    "claude auth logout exited"
                );
                if status.success() {
                    if crate::app::auth::has_credentials() {
                        let _ = tx.send(ClientEvent::SlashCommandError(
                            "Logout exited successfully but credentials are still present. \
                             Try /logout again or run `claude auth logout` in another terminal."
                                .to_owned(),
                        ));
                        return;
                    }
                    let _ = tx.send(ClientEvent::LogoutCompleted);
                } else {
                    let _ = tx.send(ClientEvent::SlashCommandError(format!(
                        "/logout failed (exit code: {})",
                        status.code().map_or("unknown".to_owned(), |c| c.to_string())
                    )));
                }
            }
            Err(e) => {
                let _ = tx.send(ClientEvent::SlashCommandError(format!(
                    "Failed to run claude auth logout: {e}"
                )));
            }
        }
    });
    true
}

/// Resolve the `claude` CLI binary from PATH, or push an error message and return `None`.
fn resolve_claude_cli(app: &mut App, subcommand: &str) -> Option<std::path::PathBuf> {
    if let Ok(path) = which::which("claude") {
        tracing::debug!(path = %path.display(), "Resolved claude CLI binary");
        Some(path)
    } else {
        push_system_message(
            app,
            format!(
                "claude CLI not found in PATH. Install it and retry /{subcommand}, \
                 or run `claude auth {subcommand}` manually in another terminal."
            ),
        );
        None
    }
}

fn handle_mode_submit(app: &mut App, args: &[&str]) -> bool {
    let [requested_mode_arg] = args else {
        push_system_message(app, "Usage: /mode <id>");
        return true;
    };
    let requested_mode = requested_mode_arg.trim();
    if requested_mode.is_empty() {
        push_system_message(app, "Usage: /mode <id>");
        return true;
    }

    let Some((conn, sid)) = require_active_session(
        app,
        "Cannot switch mode: not connected yet.",
        "Cannot switch mode: no active session.",
    ) else {
        return true;
    };

    if let Some(ref mode) = app.mode
        && !mode.available_modes.iter().any(|m| m.id == requested_mode)
    {
        push_system_message(app, format!("Unknown mode: {requested_mode}"));
        return true;
    }

    set_command_pending(
        app,
        "Switching mode...",
        Some(crate::app::PendingCommandAck::CurrentModeUpdate),
    );

    let tx = app.event_tx.clone();
    let requested_mode_owned = requested_mode.to_owned();
    tokio::task::spawn_local(async move {
        match conn.set_mode(sid.to_string(), requested_mode_owned) {
            Ok(()) => {}
            Err(e) => {
                let _ =
                    tx.send(ClientEvent::SlashCommandError(format!("Failed to run /mode: {e}")));
            }
        }
    });
    true
}

fn handle_model_submit(app: &mut App, args: &[&str]) -> bool {
    let model_name = args.join(" ");
    if model_name.trim().is_empty() {
        push_system_message(app, "Usage: /model <name>");
        return true;
    }

    let Some((conn, sid)) = require_active_session(
        app,
        "Cannot switch model: not connected yet.",
        "Cannot switch model: no active session.",
    ) else {
        return true;
    };

    if !app.available_models.is_empty()
        && !app.available_models.iter().any(|candidate| candidate.id == model_name)
    {
        push_system_message(app, format!("Unknown model: {model_name}"));
        return true;
    }

    set_command_pending(
        app,
        "Switching model...",
        Some(crate::app::PendingCommandAck::ConfigOptionUpdate { option_id: "model".to_owned() }),
    );

    let tx = app.event_tx.clone();
    tokio::task::spawn_local(async move {
        match conn.set_model(sid.to_string(), model_name) {
            Ok(()) => {}
            Err(e) => {
                let _ =
                    tx.send(ClientEvent::SlashCommandError(format!("Failed to run /model: {e}")));
            }
        }
    });
    true
}

fn handle_new_session_submit(app: &mut App, args: &[&str]) -> bool {
    if !args.is_empty() {
        push_system_message(app, "Usage: /new-session");
        return true;
    }

    push_user_message(app, "/new-session");

    let Some(conn) = require_connection(app, "Cannot create new session: not connected yet.")
    else {
        return true;
    };

    set_command_pending(app, "Starting new session...", None);

    if let Err(e) = start_new_session(app, &conn, SessionStartReason::NewSession) {
        let _ = app
            .event_tx
            .send(ClientEvent::SlashCommandError(format!("Failed to run /new-session: {e}")));
    }
    true
}

fn handle_resume_submit(app: &mut App, args: &[&str]) -> bool {
    let [session_id_arg] = args else {
        push_system_message(app, "Usage: /resume <session_id>");
        return true;
    };
    let session_id = session_id_arg.trim();
    if session_id.is_empty() {
        push_system_message(app, "Usage: /resume <session_id>");
        return true;
    }

    push_user_message(app, format!("/resume {session_id}"));
    let Some(conn) = require_connection(app, "Cannot resume session: not connected yet.") else {
        return true;
    };

    set_command_pending(app, &format!("Resuming session {session_id}..."), None);
    app.resuming_session_id = Some(session_id.to_owned());
    let session_id = session_id.to_owned();
    if let Err(e) = resume_session(app, &conn, session_id) {
        let _ = app
            .event_tx
            .send(ClientEvent::SlashCommandError(format!("Failed to run /resume: {e}")));
    }
    true
}

fn handle_unknown_submit(app: &mut App, command_name: &str) -> bool {
    if super::candidates::is_supported_command(app, command_name) {
        return false;
    }
    push_system_message(app, format!("{command_name} is not yet supported"));
    true
}
