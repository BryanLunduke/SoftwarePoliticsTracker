// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use crate::agent::error_handling::TurnErrorClass;
use crate::agent::model;
use crate::app::plugins::{PluginsCliActionSuccess, PluginsInventorySnapshot};
use crate::app::{UsageSnapshot, UsageSourceKind};
use crate::error::AppError;
use std::cell::RefCell;
use std::collections::HashMap;
use std::path::PathBuf;
use std::rc::Rc;
use std::sync::{Arc, Mutex};

/// Messages sent from the backend bridge path to the App/UI layer.
pub enum ClientEvent {
    /// Session update notification (streaming text, tool calls, etc.)
    SessionUpdate(model::SessionUpdate),
    /// Permission request that needs user input.
    PermissionRequest {
        request: model::RequestPermissionRequest,
        response_tx: tokio::sync::oneshot::Sender<model::RequestPermissionResponse>,
    },
    /// Question request from `AskUserQuestion` that needs structured user input.
    QuestionRequest {
        request: model::RequestQuestionRequest,
        response_tx: tokio::sync::oneshot::Sender<model::RequestQuestionResponse>,
    },
    /// MCP elicitation request that needs auth or other MCP input.
    McpElicitationRequest { request: crate::agent::types::ElicitationRequest },
    /// MCP elicitation completed in the SDK.
    McpElicitationCompleted { elicitation_id: String, server_name: Option<String> },
    /// MCP auth redirect returned directly by the SDK auth call.
    McpAuthRedirect { redirect: crate::agent::types::McpAuthRedirect },
    /// MCP operation failed and should be surfaced in the MCP config UI.
    McpOperationError { error: crate::agent::types::McpOperationError },
    /// A prompt turn completed successfully.
    TurnComplete,
    /// `cancel` notification was accepted by the bridge.
    TurnCancelled,
    /// A prompt turn failed with an error.
    TurnError(String),
    /// A prompt turn failed with bridge-provided classification metadata.
    TurnErrorClassified { message: String, class: TurnErrorClass },
    /// Background connection completed successfully.
    Connected {
        session_id: model::SessionId,
        cwd: String,
        model_name: String,
        available_models: Vec<model::AvailableModel>,
        mode: Option<crate::app::ModeState>,
        history_updates: Vec<model::SessionUpdate>,
    },
    /// Background connection failed.
    ConnectionFailed(String),
    /// Authentication is required before a session can be created.
    AuthRequired { method_name: String, method_description: String },
    /// Slash-command execution failed with a user-facing error.
    SlashCommandError(String),
    /// Custom slash command replaced the active session.
    SessionReplaced {
        session_id: model::SessionId,
        cwd: String,
        model_name: String,
        available_models: Vec<model::AvailableModel>,
        mode: Option<crate::app::ModeState>,
        history_updates: Vec<model::SessionUpdate>,
    },
    /// Recent sessions discovered via SDK session listing.
    SessionsListed { sessions: Vec<crate::agent::types::SessionListEntry> },
    /// Startup update check found a newer published version.
    UpdateAvailable { latest_version: String, current_version: String },
    /// Startup Claude Code status check detected degraded/outage conditions.
    ServiceStatus { severity: ServiceStatusSeverity, message: String },
    /// /login completed via `claude auth login` -- credentials stored, ready to start a session.
    AuthCompleted { conn: Rc<crate::agent::client::AgentConnection> },
    /// /logout completed via `claude auth logout`.
    LogoutCompleted,
    /// Status snapshot received from bridge (account info).
    StatusSnapshotReceived { session_id: String, account: crate::agent::types::AccountInfo },
    /// MCP server snapshot received from bridge.
    McpSnapshotReceived {
        session_id: String,
        servers: Vec<crate::agent::types::McpServerStatus>,
        error: Option<String>,
    },
    /// Usage refresh task started.
    UsageRefreshStarted { epoch: u64 },
    /// Usage refresh completed successfully.
    UsageSnapshotReceived { epoch: u64, snapshot: UsageSnapshot },
    /// Usage refresh failed.
    UsageRefreshFailed { epoch: u64, message: String, source: UsageSourceKind },
    /// Claude CLI plugin inventory refresh completed.
    PluginsInventoryUpdated {
        cwd_raw: String,
        snapshot: PluginsInventorySnapshot,
        claude_path: PathBuf,
    },
    /// Claude CLI plugin inventory refresh failed.
    PluginsInventoryRefreshFailed { cwd_raw: String, message: String },
    /// Plugin CLI action completed and returned a refreshed inventory snapshot.
    PluginsCliActionSucceeded { cwd_raw: String, result: PluginsCliActionSuccess },
    /// Plugin CLI action failed.
    PluginsCliActionFailed { cwd_raw: String, message: String },
    /// Fatal app error that should terminate and map to an exit code.
    FatalError(AppError),
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ServiceStatusSeverity {
    Warning,
    Error,
}

/// Shared handle to all spawned terminal processes.
pub type TerminalMap = Rc<RefCell<HashMap<String, TerminalProcess>>>;

/// Minimal terminal process state used by UI snapshot rendering.
pub struct TerminalProcess {
    pub child: Option<tokio::process::Child>,
    /// Accumulated stdout+stderr - append-only, never cleared.
    pub output_buffer: Arc<Mutex<Vec<u8>>>,
    /// The shell command that was executed.
    pub command: String,
}

/// Kill all spawned terminal child processes. Call on app exit.
pub fn kill_all_terminals(terminals: &TerminalMap) {
    let mut map = terminals.borrow_mut();
    for (_, terminal) in map.iter_mut() {
        if let Some(child) = terminal.child.as_mut() {
            let _ = child.start_kill();
        }
    }
    map.clear();
}
