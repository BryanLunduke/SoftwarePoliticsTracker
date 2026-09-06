// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use crate::agent::bridge::BridgeLauncher;
use crate::agent::wire::{BridgeCommand, CommandEnvelope, EventEnvelope, SessionLaunchSettings};
use crate::error::AppError;
use anyhow::Context as _;
use tokio::io::{AsyncBufReadExt as _, AsyncWriteExt as _, BufReader, BufWriter};
use tokio::process::{Child, ChildStderr, ChildStdin, ChildStdout};
use tokio::sync::mpsc;

pub struct BridgeClient {
    child: Child,
    stdin: BufWriter<ChildStdin>,
    stdout: tokio::io::Lines<BufReader<ChildStdout>>,
}

impl BridgeClient {
    pub fn spawn(launcher: &BridgeLauncher) -> anyhow::Result<Self> {
        let mut child = launcher
            .command()
            .spawn()
            .map_err(|_| anyhow::Error::new(AppError::AdapterCrashed))
            .with_context(|| format!("failed to spawn bridge process: {}", launcher.describe()))?;

        let stdin = child.stdin.take().context("bridge stdin not available")?;
        let stdout = child.stdout.take().context("bridge stdout not available")?;
        let stderr = child.stderr.take().context("bridge stderr not available")?;
        Self::spawn_stderr_logger(stderr);

        Ok(Self { child, stdin: BufWriter::new(stdin), stdout: BufReader::new(stdout).lines() })
    }

    fn spawn_stderr_logger(stderr: ChildStderr) {
        tokio::task::spawn_local(async move {
            let mut lines = BufReader::new(stderr).lines();
            loop {
                match lines.next_line().await {
                    Ok(Some(line)) => Self::log_bridge_stderr_line(&line),
                    Ok(None) => break,
                    Err(err) => {
                        tracing::error!("failed to read bridge stderr: {err}");
                        break;
                    }
                }
            }
        });
    }

    fn log_bridge_stderr_line(line: &str) {
        // The bridge uses a structured "[sdk <verb>]" prefix format.
        // Extract an explicit level from it; fall back to debug for ordinary chatter.
        let lower = line.to_ascii_lowercase();
        if lower.contains("[sdk error]") || lower.starts_with("error") || lower.contains("panic") {
            tracing::error!("bridge stderr: {line}");
        } else if lower.contains("[sdk warn]") || lower.starts_with("warn") {
            tracing::warn!("bridge stderr: {line}");
        } else {
            tracing::debug!("bridge stderr: {line}");
        }
    }

    pub async fn send(&mut self, envelope: CommandEnvelope) -> anyhow::Result<()> {
        let line =
            serde_json::to_string(&envelope).context("failed to serialize bridge command")?;
        self.stdin.write_all(line.as_bytes()).await.context("failed to write bridge command")?;
        self.stdin.write_all(b"\n").await.context("failed to write bridge newline")?;
        self.stdin.flush().await.context("failed to flush bridge stdin")?;
        Ok(())
    }

    pub async fn recv(&mut self) -> anyhow::Result<Option<EventEnvelope>> {
        let Some(line) = self.stdout.next_line().await.context("failed to read bridge stdout")?
        else {
            return Ok(None);
        };
        let event: EventEnvelope =
            serde_json::from_str(&line).context("failed to decode bridge event json")?;
        Ok(Some(event))
    }

    pub async fn shutdown(&mut self) -> anyhow::Result<()> {
        self.send(CommandEnvelope { request_id: None, command: BridgeCommand::Shutdown }).await?;
        Ok(())
    }

    pub async fn wait(mut self) -> anyhow::Result<std::process::ExitStatus> {
        self.child.wait().await.context("failed to wait for bridge process")
    }
}

#[derive(Clone)]
pub struct AgentConnection {
    command_tx: mpsc::UnboundedSender<CommandEnvelope>,
}

#[derive(Debug, Clone)]
pub struct PromptResponse {
    pub stop_reason: String,
}

impl AgentConnection {
    #[must_use]
    pub fn new(command_tx: mpsc::UnboundedSender<CommandEnvelope>) -> Self {
        Self { command_tx }
    }

    pub fn prompt_text(&self, session_id: String, text: String) -> anyhow::Result<PromptResponse> {
        self.send(CommandEnvelope {
            request_id: None,
            command: BridgeCommand::Prompt {
                session_id,
                chunks: vec![crate::agent::types::PromptChunk {
                    kind: "text".to_owned(),
                    value: serde_json::Value::String(text),
                }],
            },
        })?;
        Ok(PromptResponse { stop_reason: "end_turn".to_owned() })
    }

    pub fn cancel(&self, session_id: String) -> anyhow::Result<()> {
        self.send(CommandEnvelope {
            request_id: None,
            command: BridgeCommand::CancelTurn { session_id },
        })
    }

    pub fn set_mode(&self, session_id: String, mode: String) -> anyhow::Result<()> {
        self.send(CommandEnvelope {
            request_id: None,
            command: BridgeCommand::SetMode { session_id, mode },
        })
    }

    pub fn generate_session_title(
        &self,
        session_id: String,
        description: String,
    ) -> anyhow::Result<()> {
        self.send(CommandEnvelope {
            request_id: None,
            command: BridgeCommand::GenerateSessionTitle { session_id, description },
        })
    }

    pub fn rename_session(&self, session_id: String, title: String) -> anyhow::Result<()> {
        self.send(CommandEnvelope {
            request_id: None,
            command: BridgeCommand::RenameSession { session_id, title },
        })
    }

    pub fn set_model(&self, session_id: String, model: String) -> anyhow::Result<()> {
        self.send(CommandEnvelope {
            request_id: None,
            command: BridgeCommand::SetModel { session_id, model },
        })
    }

    pub fn get_status_snapshot(&self, session_id: String) -> anyhow::Result<()> {
        self.send(CommandEnvelope {
            request_id: None,
            command: BridgeCommand::GetStatusSnapshot { session_id },
        })
    }

    pub fn get_mcp_snapshot(&self, session_id: String) -> anyhow::Result<()> {
        self.send(CommandEnvelope {
            request_id: None,
            command: BridgeCommand::GetMcpSnapshot { session_id },
        })
    }

    pub fn respond_to_elicitation(
        &self,
        session_id: String,
        elicitation_request_id: String,
        action: crate::agent::types::ElicitationAction,
        content: Option<serde_json::Value>,
    ) -> anyhow::Result<()> {
        self.send(CommandEnvelope {
            request_id: None,
            command: BridgeCommand::ElicitationResponse {
                session_id,
                elicitation_request_id,
                action,
                content,
            },
        })
    }

    pub fn reconnect_mcp_server(
        &self,
        session_id: String,
        server_name: String,
    ) -> anyhow::Result<()> {
        self.send(CommandEnvelope {
            request_id: None,
            command: BridgeCommand::McpReconnect { session_id, server_name },
        })
    }

    pub fn toggle_mcp_server(
        &self,
        session_id: String,
        server_name: String,
        enabled: bool,
    ) -> anyhow::Result<()> {
        self.send(CommandEnvelope {
            request_id: None,
            command: BridgeCommand::McpToggle { session_id, server_name, enabled },
        })
    }

    pub fn set_mcp_servers(
        &self,
        session_id: String,
        servers: std::collections::BTreeMap<String, crate::agent::types::McpServerConfig>,
    ) -> anyhow::Result<()> {
        self.send(CommandEnvelope {
            request_id: None,
            command: BridgeCommand::McpSetServers { session_id, servers },
        })
    }

    pub fn authenticate_mcp_server(
        &self,
        session_id: String,
        server_name: String,
    ) -> anyhow::Result<()> {
        self.send(CommandEnvelope {
            request_id: None,
            command: BridgeCommand::McpAuthenticate { session_id, server_name },
        })
    }

    pub fn clear_mcp_auth(&self, session_id: String, server_name: String) -> anyhow::Result<()> {
        self.send(CommandEnvelope {
            request_id: None,
            command: BridgeCommand::McpClearAuth { session_id, server_name },
        })
    }

    pub fn submit_mcp_oauth_callback_url(
        &self,
        session_id: String,
        server_name: String,
        callback_url: String,
    ) -> anyhow::Result<()> {
        self.send(CommandEnvelope {
            request_id: None,
            command: BridgeCommand::McpOauthCallbackUrl { session_id, server_name, callback_url },
        })
    }

    pub fn new_session(
        &self,
        cwd: String,
        launch_settings: SessionLaunchSettings,
    ) -> anyhow::Result<()> {
        self.send(CommandEnvelope {
            request_id: None,
            command: BridgeCommand::NewSession { cwd, launch_settings },
        })
    }

    pub fn resume_session(
        &self,
        session_id: String,
        launch_settings: SessionLaunchSettings,
    ) -> anyhow::Result<()> {
        self.send(CommandEnvelope {
            request_id: None,
            command: BridgeCommand::ResumeSession {
                session_id,
                launch_settings,
                metadata: std::collections::BTreeMap::new(),
            },
        })
    }

    fn send(&self, envelope: CommandEnvelope) -> anyhow::Result<()> {
        self.command_tx.send(envelope).map_err(|_| anyhow::anyhow!("bridge command channel closed"))
    }
}

#[cfg(test)]
mod tests {
    use super::AgentConnection;
    use crate::agent::types::ElicitationAction;
    use crate::agent::wire::BridgeCommand;
    use std::collections::BTreeMap;

    #[test]
    fn generate_session_title_sends_bridge_command() {
        let (tx, mut rx) = tokio::sync::mpsc::unbounded_channel();
        let conn = AgentConnection::new(tx);

        conn.generate_session_title("session-1".to_owned(), "Summarize work".to_owned())
            .expect("generate");

        let envelope = rx.try_recv().expect("command");
        assert_eq!(
            envelope.command,
            BridgeCommand::GenerateSessionTitle {
                session_id: "session-1".to_owned(),
                description: "Summarize work".to_owned(),
            }
        );
    }

    #[test]
    fn rename_session_sends_bridge_command() {
        let (tx, mut rx) = tokio::sync::mpsc::unbounded_channel();
        let conn = AgentConnection::new(tx);

        conn.rename_session("session-1".to_owned(), "Renamed".to_owned()).expect("rename");

        let envelope = rx.try_recv().expect("command");
        assert_eq!(
            envelope.command,
            BridgeCommand::RenameSession {
                session_id: "session-1".to_owned(),
                title: "Renamed".to_owned(),
            }
        );
    }

    #[test]
    fn get_mcp_snapshot_sends_bridge_command() {
        let (tx, mut rx) = tokio::sync::mpsc::unbounded_channel();
        let conn = AgentConnection::new(tx);

        conn.get_mcp_snapshot("session-1".to_owned()).expect("mcp snapshot");

        let envelope = rx.try_recv().expect("command");
        assert_eq!(
            envelope.command,
            BridgeCommand::GetMcpSnapshot { session_id: "session-1".to_owned() }
        );
    }

    #[test]
    fn reconnect_mcp_server_sends_bridge_command() {
        let (tx, mut rx) = tokio::sync::mpsc::unbounded_channel();
        let conn = AgentConnection::new(tx);

        conn.reconnect_mcp_server("session-1".to_owned(), "notion".to_owned())
            .expect("mcp reconnect");

        let envelope = rx.try_recv().expect("command");
        assert_eq!(
            envelope.command,
            BridgeCommand::McpReconnect {
                session_id: "session-1".to_owned(),
                server_name: "notion".to_owned(),
            }
        );
    }

    #[test]
    fn toggle_mcp_server_sends_bridge_command() {
        let (tx, mut rx) = tokio::sync::mpsc::unbounded_channel();
        let conn = AgentConnection::new(tx);

        conn.toggle_mcp_server("session-1".to_owned(), "notion".to_owned(), false)
            .expect("mcp toggle");

        let envelope = rx.try_recv().expect("command");
        assert_eq!(
            envelope.command,
            BridgeCommand::McpToggle {
                session_id: "session-1".to_owned(),
                server_name: "notion".to_owned(),
                enabled: false,
            }
        );
    }

    #[test]
    fn set_mcp_servers_sends_bridge_command() {
        let (tx, mut rx) = tokio::sync::mpsc::unbounded_channel();
        let conn = AgentConnection::new(tx);
        let servers = BTreeMap::from([(
            "notion".to_owned(),
            crate::agent::types::McpServerConfig::Http {
                url: "https://mcp.notion.com/mcp".to_owned(),
                headers: BTreeMap::new(),
            },
        )]);

        conn.set_mcp_servers("session-1".to_owned(), servers.clone()).expect("mcp set servers");

        let envelope = rx.try_recv().expect("command");
        assert_eq!(
            envelope.command,
            BridgeCommand::McpSetServers { session_id: "session-1".to_owned(), servers }
        );
    }

    #[test]
    fn respond_to_elicitation_sends_bridge_command() {
        let (tx, mut rx) = tokio::sync::mpsc::unbounded_channel();
        let conn = AgentConnection::new(tx);

        conn.respond_to_elicitation(
            "session-1".to_owned(),
            "elicitation-1".to_owned(),
            ElicitationAction::Accept,
            None,
        )
        .expect("elicitation response");

        let envelope = rx.try_recv().expect("command");
        assert_eq!(
            envelope.command,
            BridgeCommand::ElicitationResponse {
                session_id: "session-1".to_owned(),
                elicitation_request_id: "elicitation-1".to_owned(),
                action: ElicitationAction::Accept,
                content: None,
            }
        );
    }
}
