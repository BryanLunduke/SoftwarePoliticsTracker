// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use crate::agent::types;
use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;

#[derive(Debug, Clone, Default, PartialEq, Eq, Serialize, Deserialize)]
pub struct SessionLaunchSettings {
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub language: Option<String>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub settings: Option<serde_json::Value>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub agent_progress_summaries: Option<bool>,
}

impl SessionLaunchSettings {
    #[must_use]
    pub fn is_empty(&self) -> bool {
        self.language.is_none()
            && self.settings.is_none()
            && self.agent_progress_summaries.is_none()
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct CommandEnvelope {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub request_id: Option<String>,
    #[serde(flatten)]
    pub command: BridgeCommand,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(tag = "command", rename_all = "snake_case")]
pub enum BridgeCommand {
    Initialize {
        cwd: String,
        #[serde(default, skip_serializing_if = "BTreeMap::is_empty")]
        metadata: BTreeMap<String, serde_json::Value>,
    },
    CreateSession {
        cwd: String,
        resume: Option<String>,
        #[serde(default, skip_serializing_if = "SessionLaunchSettings::is_empty")]
        launch_settings: SessionLaunchSettings,
        #[serde(default, skip_serializing_if = "BTreeMap::is_empty")]
        metadata: BTreeMap<String, serde_json::Value>,
    },
    ResumeSession {
        session_id: String,
        #[serde(default, skip_serializing_if = "SessionLaunchSettings::is_empty")]
        launch_settings: SessionLaunchSettings,
        #[serde(default, skip_serializing_if = "BTreeMap::is_empty")]
        metadata: BTreeMap<String, serde_json::Value>,
    },
    Prompt {
        session_id: String,
        chunks: Vec<types::PromptChunk>,
    },
    CancelTurn {
        session_id: String,
    },
    SetModel {
        session_id: String,
        model: String,
    },
    SetMode {
        session_id: String,
        mode: String,
    },
    GenerateSessionTitle {
        session_id: String,
        description: String,
    },
    RenameSession {
        session_id: String,
        title: String,
    },
    NewSession {
        cwd: String,
        #[serde(default, skip_serializing_if = "SessionLaunchSettings::is_empty")]
        launch_settings: SessionLaunchSettings,
    },
    PermissionResponse {
        session_id: String,
        tool_call_id: String,
        outcome: types::PermissionOutcome,
    },
    QuestionResponse {
        session_id: String,
        tool_call_id: String,
        outcome: types::QuestionOutcome,
    },
    ElicitationResponse {
        session_id: String,
        elicitation_request_id: String,
        action: types::ElicitationAction,
        #[serde(default, skip_serializing_if = "Option::is_none")]
        content: Option<serde_json::Value>,
    },
    GetStatusSnapshot {
        session_id: String,
    },
    GetMcpSnapshot {
        session_id: String,
    },
    McpReconnect {
        session_id: String,
        server_name: String,
    },
    McpToggle {
        session_id: String,
        server_name: String,
        enabled: bool,
    },
    McpSetServers {
        session_id: String,
        #[serde(default, skip_serializing_if = "BTreeMap::is_empty")]
        servers: BTreeMap<String, types::McpServerConfig>,
    },
    McpAuthenticate {
        session_id: String,
        server_name: String,
    },
    McpClearAuth {
        session_id: String,
        server_name: String,
    },
    McpOauthCallbackUrl {
        session_id: String,
        server_name: String,
        callback_url: String,
    },
    Shutdown,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct EventEnvelope {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub request_id: Option<String>,
    #[serde(flatten)]
    pub event: BridgeEvent,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(tag = "event", rename_all = "snake_case")]
pub enum BridgeEvent {
    Connected {
        session_id: String,
        cwd: String,
        model_name: String,
        #[serde(default)]
        available_models: Vec<types::AvailableModel>,
        mode: Option<types::ModeState>,
        history_updates: Option<Vec<types::SessionUpdate>>,
    },
    AuthRequired {
        method_name: String,
        method_description: String,
    },
    ConnectionFailed {
        message: String,
    },
    SessionUpdate {
        session_id: String,
        update: types::SessionUpdate,
    },
    PermissionRequest {
        session_id: String,
        request: types::PermissionRequest,
    },
    QuestionRequest {
        session_id: String,
        request: types::QuestionRequest,
    },
    ElicitationRequest {
        session_id: String,
        request: types::ElicitationRequest,
    },
    ElicitationComplete {
        session_id: String,
        elicitation_id: String,
        server_name: Option<String>,
    },
    McpAuthRedirect {
        session_id: String,
        redirect: types::McpAuthRedirect,
    },
    McpOperationError {
        session_id: String,
        error: types::McpOperationError,
    },
    TurnComplete {
        session_id: String,
    },
    TurnError {
        session_id: String,
        message: String,
        error_kind: Option<String>,
        sdk_result_subtype: Option<String>,
        assistant_error: Option<String>,
    },
    SlashError {
        session_id: String,
        message: String,
    },
    SessionReplaced {
        session_id: String,
        cwd: String,
        model_name: String,
        #[serde(default)]
        available_models: Vec<types::AvailableModel>,
        mode: Option<types::ModeState>,
        history_updates: Option<Vec<types::SessionUpdate>>,
    },
    Initialized {
        result: types::InitializeResult,
    },
    SessionsListed {
        sessions: Vec<types::SessionListEntry>,
    },
    StatusSnapshot {
        session_id: String,
        account: types::AccountInfo,
    },
    McpSnapshot {
        session_id: String,
        #[serde(default)]
        servers: Vec<types::McpServerStatus>,
        error: Option<String>,
    },
}

#[cfg(test)]
mod tests {
    use super::{
        BridgeCommand, BridgeEvent, CommandEnvelope, EventEnvelope, SessionLaunchSettings,
    };
    use crate::agent::types;

    #[test]
    fn command_envelope_roundtrip_json() {
        let env = CommandEnvelope {
            request_id: Some("req-1".to_owned()),
            command: BridgeCommand::SetMode {
                session_id: "s1".to_owned(),
                mode: "plan".to_owned(),
            },
        };
        let json = serde_json::to_string(&env).expect("serialize");
        let decoded: CommandEnvelope = serde_json::from_str(&json).expect("deserialize");
        assert_eq!(decoded, env);
    }

    #[test]
    fn event_envelope_roundtrip_json() {
        let env = EventEnvelope {
            request_id: None,
            event: BridgeEvent::SessionUpdate {
                session_id: "session-1".to_owned(),
                update: types::SessionUpdate::CurrentModeUpdate {
                    current_mode_id: "default".to_owned(),
                },
            },
        };
        let json = serde_json::to_string(&env).expect("serialize");
        let decoded: EventEnvelope = serde_json::from_str(&json).expect("deserialize");
        assert_eq!(decoded, env);
    }

    #[test]
    fn session_launch_settings_serializes_agent_progress_summaries() {
        let settings = SessionLaunchSettings {
            settings: Some(serde_json::json!({ "model": "haiku" })),
            agent_progress_summaries: Some(true),
            ..SessionLaunchSettings::default()
        };

        let json = serde_json::to_value(&settings).expect("serialize");
        assert_eq!(
            json,
            serde_json::json!({
                "settings": { "model": "haiku" },
                "agent_progress_summaries": true
            })
        );
    }
}
