// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

//! Slash command types, parsing, and delegation.
//!
//! Submodules:
//! - `candidates`: candidate detection, filtering, and building
//! - `navigation`: autocomplete activation, movement, and confirm
//! - `executors`: slash command execution handlers

mod candidates;
mod executors;
mod navigation;

use super::{
    App, AppStatus, ChatMessage, MessageBlock, MessageRole, TextBlock, dialog::DialogState,
};
use crate::agent::model;
use std::rc::Rc;

pub const MAX_VISIBLE: usize = 8;
const MAX_CANDIDATES: usize = 50;

// Re-export public API
pub use executors::try_handle_submit;
pub use navigation::{
    activate, confirm_selection, deactivate, move_down, move_up, sync_with_cursor, update_query,
};

#[derive(Debug, Clone)]
pub struct SlashCandidate {
    pub insert_value: String,
    pub primary: String,
    pub secondary: Option<String>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum SlashContext {
    CommandName,
    Argument { command: String, arg_index: usize, token_range: (usize, usize) },
}

#[derive(Debug, Clone)]
pub struct SlashState {
    /// Character position where `/` token starts.
    pub trigger_row: usize,
    pub trigger_col: usize,
    /// Current typed query for the active slash context.
    pub query: String,
    /// Command-name or argument context.
    pub context: SlashContext,
    /// Filtered list of supported candidates.
    pub candidates: Vec<SlashCandidate>,
    /// Shared autocomplete dialog navigation state.
    pub dialog: DialogState,
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct SlashDetection {
    trigger_row: usize,
    trigger_col: usize,
    query: String,
    context: SlashContext,
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct ParsedSlash<'a> {
    name: &'a str,
    args: Vec<&'a str>,
}

fn parse(text: &str) -> Option<ParsedSlash<'_>> {
    let trimmed = text.trim();
    if !trimmed.starts_with('/') {
        return None;
    }
    let mut parts = trimmed.split_whitespace();
    let name = parts.next()?;
    Some(ParsedSlash { name, args: parts.collect() })
}

pub fn is_cancel_command(text: &str) -> bool {
    parse(text).is_some_and(|parsed| parsed.name == "/cancel")
}

fn normalize_slash_name(name: &str) -> String {
    if name.starts_with('/') { name.to_owned() } else { format!("/{name}") }
}

fn push_system_message(app: &mut App, text: impl Into<String>) {
    let text = text.into();
    app.push_message_tracked(ChatMessage {
        role: MessageRole::System(None),
        blocks: vec![MessageBlock::Text(TextBlock::from_complete(&text))],
        usage: None,
    });
    app.enforce_history_retention_tracked();
    app.viewport.engage_auto_scroll();
}

fn push_user_message(app: &mut App, text: impl Into<String>) {
    let text = text.into();
    app.push_message_tracked(ChatMessage {
        role: MessageRole::User,
        blocks: vec![MessageBlock::Text(TextBlock::from_complete(&text))],
        usage: None,
    });
    app.enforce_history_retention_tracked();
    app.viewport.engage_auto_scroll();
}

fn require_connection(
    app: &mut App,
    not_connected_msg: &'static str,
) -> Option<Rc<crate::agent::client::AgentConnection>> {
    let Some(conn) = app.conn.as_ref() else {
        push_system_message(app, not_connected_msg);
        return None;
    };
    Some(Rc::clone(conn))
}

fn require_active_session(
    app: &mut App,
    not_connected_msg: &'static str,
    no_session_msg: &'static str,
) -> Option<(Rc<crate::agent::client::AgentConnection>, model::SessionId)> {
    let conn = require_connection(app, not_connected_msg)?;
    let Some(session_id) = app.session_id.clone() else {
        push_system_message(app, no_session_msg);
        return None;
    };
    Some((conn, session_id))
}

/// Block the input field while a slash command is in flight.
fn set_command_pending(app: &mut App, label: &str, ack: Option<super::PendingCommandAck>) {
    app.status = AppStatus::CommandPending;
    app.pending_command_label = Some(label.to_owned());
    app.pending_command_ack = ack;
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::app::App;

    // Re-import submodule items needed by tests
    use super::candidates::{
        argument_candidates, detect_slash_at_cursor, supported_command_candidates,
    };

    #[test]
    fn parse_non_slash_returns_none() {
        assert!(parse("hello world").is_none());
    }

    #[test]
    fn parse_slash_name_and_args() {
        let parsed = parse("/mode plan").expect("slash command");
        assert_eq!(parsed.name, "/mode");
        assert_eq!(parsed.args, vec!["plan"]);
    }

    #[test]
    fn unsupported_command_is_handled_locally() {
        let mut app = App::test_default();
        let consumed = try_handle_submit(&mut app, "/definitely-unknown");
        assert!(consumed);
        let Some(last) = app.messages.last() else {
            panic!("expected system message");
        };
        assert!(matches!(last.role, MessageRole::System(_)));
    }

    #[test]
    fn advertised_command_is_forwarded() {
        let mut app = App::test_default();
        app.available_commands = vec![model::AvailableCommand::new("/help", "Help")];
        let consumed = try_handle_submit(&mut app, "/help");
        assert!(!consumed);
    }

    #[test]
    fn login_logout_appear_in_candidates_as_builtins() {
        let app = App::test_default();
        let names: Vec<String> =
            supported_command_candidates(&app).into_iter().map(|c| c.primary).collect();
        assert!(names.iter().any(|n| n == "/config"), "missing /config");
        assert!(names.iter().any(|n| n == "/login"), "missing /login");
        assert!(names.iter().any(|n| n == "/logout"), "missing /logout");
        assert!(names.iter().any(|n| n == "/mcp"), "missing /mcp");
        assert!(names.iter().any(|n| n == "/plugins"), "missing /plugins");
        assert!(names.iter().any(|n| n == "/usage"), "missing /usage");
    }

    #[test]
    fn config_without_args_opens_settings_view() {
        let dir = tempfile::tempdir().expect("tempdir");
        let mut app = App::test_default();
        app.settings_home_override = Some(dir.path().to_path_buf());

        let consumed = try_handle_submit(&mut app, "/config");

        assert!(consumed);
        assert_eq!(app.active_view, super::super::ActiveView::Config);
    }

    #[test]
    fn config_with_extra_args_returns_usage_message() {
        let mut app = App::test_default();

        let consumed = try_handle_submit(&mut app, "/config extra");

        assert!(consumed);
        let Some(last) = app.messages.last() else {
            panic!("expected usage message");
        };
        let Some(MessageBlock::Text(block)) = last.blocks.first() else {
            panic!("expected text block");
        };
        assert_eq!(block.text, "Usage: /config");
    }

    #[test]
    fn plugins_without_args_opens_plugins_tab() {
        let dir = tempfile::tempdir().expect("tempdir");
        let mut app = App::test_default();
        app.settings_home_override = Some(dir.path().to_path_buf());

        let consumed = try_handle_submit(&mut app, "/plugins");

        assert!(consumed);
        assert_eq!(app.active_view, super::super::ActiveView::Config);
        assert_eq!(app.config.active_tab, super::super::ConfigTab::Plugins);
    }

    #[test]
    fn mcp_opens_config_at_mcp_tab() {
        let dir = tempfile::tempdir().expect("tempdir");
        let mut app = App::test_default();
        app.settings_home_override = Some(dir.path().to_path_buf());

        let consumed = try_handle_submit(&mut app, "/mcp");

        assert!(consumed);
        assert_eq!(app.active_view, super::super::ActiveView::Config);
        assert_eq!(app.config.active_tab, super::super::ConfigTab::Mcp);
    }

    #[test]
    fn mcp_with_extra_args_returns_usage() {
        let mut app = App::test_default();

        let consumed = try_handle_submit(&mut app, "/mcp extra");

        assert!(consumed);
        let Some(last) = app.messages.last() else {
            panic!("expected usage message");
        };
        let Some(MessageBlock::Text(block)) = last.blocks.first() else {
            panic!("expected text block");
        };
        assert_eq!(block.text, "Usage: /mcp");
    }

    #[test]
    fn plugins_with_extra_args_still_opens_plugins_tab() {
        let mut app = App::test_default();
        let dir = tempfile::tempdir().expect("tempdir");
        app.settings_home_override = Some(dir.path().to_path_buf());

        let consumed = try_handle_submit(&mut app, "/plugins extra");

        assert!(consumed);
        assert_eq!(app.active_view, super::super::ActiveView::Config);
        assert_eq!(app.config.active_tab, super::super::ConfigTab::Plugins);
    }

    #[tokio::test(flavor = "current_thread")]
    async fn login_is_handled_as_builtin_and_sets_command_pending() {
        tokio::task::LocalSet::new()
            .run_until(async {
                let mut app = App::test_default();
                let consumed = try_handle_submit(&mut app, "/login");
                assert!(consumed, "/login should be handled locally");
                // Status becomes CommandPending (or stays Ready if claude CLI is not in PATH)
                assert!(
                    matches!(app.status, AppStatus::CommandPending | AppStatus::Ready),
                    "expected CommandPending or Ready, got {:?}",
                    app.status
                );
            })
            .await;
    }

    #[tokio::test(flavor = "current_thread")]
    async fn logout_is_handled_as_builtin_and_sets_command_pending() {
        tokio::task::LocalSet::new()
            .run_until(async {
                let mut app = App::test_default();
                let consumed = try_handle_submit(&mut app, "/logout");
                assert!(consumed, "/logout should be handled locally");
                assert!(
                    matches!(app.status, AppStatus::CommandPending | AppStatus::Ready),
                    "expected CommandPending or Ready, got {:?}",
                    app.status
                );
            })
            .await;
    }

    #[test]
    fn login_rejects_extra_args() {
        let mut app = App::test_default();
        let consumed = try_handle_submit(&mut app, "/login somearg");
        assert!(consumed);
        let last = app.messages.last().expect("expected system message");
        assert!(matches!(last.role, MessageRole::System(_)));
    }

    #[test]
    fn detect_slash_argument_context_after_first_space() {
        let lines = vec!["/mode pla".to_owned()];
        let detection = detect_slash_at_cursor(&lines, 0, "/mode pla".chars().count())
            .expect("slash detection");

        match detection.context {
            SlashContext::Argument { command, arg_index, token_range } => {
                assert_eq!(command, "/mode");
                assert_eq!(arg_index, 0);
                assert_eq!(token_range, (6, 9));
            }
            SlashContext::CommandName => panic!("expected argument context"),
        }
        assert_eq!(detection.query, "pla");
    }

    #[test]
    fn mode_argument_candidates_are_dynamic() {
        let mut app = App::test_default();
        app.mode = Some(super::super::ModeState {
            current_mode_id: "plan".to_owned(),
            current_mode_name: "Plan".to_owned(),
            available_modes: vec![
                super::super::ModeInfo { id: "plan".to_owned(), name: "Plan".to_owned() },
                super::super::ModeInfo { id: "code".to_owned(), name: "Code".to_owned() },
            ],
        });

        let candidates = argument_candidates(&app, "/mode", 0);
        assert!(candidates.iter().any(|c| c.insert_value == "plan"));
        assert!(candidates.iter().any(|c| c.insert_value == "code"));
        assert!(candidates.iter().any(|c| c.primary == "Plan"));
        assert!(candidates.iter().any(|c| c.secondary.as_deref() == Some("plan")));
    }

    #[test]
    fn model_argument_candidates_are_dynamic() {
        let mut app = App::test_default();
        app.available_models = vec![
            crate::agent::model::AvailableModel::new("sonnet", "Claude Sonnet")
                .description("Balanced coding model"),
            crate::agent::model::AvailableModel::new("opus", "Claude Opus"),
        ];
        let candidates = argument_candidates(&app, "/model", 0);
        assert!(candidates.iter().any(|c| c.insert_value == "sonnet"));
        assert!(candidates.iter().any(|c| c.primary == "Claude Sonnet"));
        assert!(candidates.iter().any(|c| c.secondary.as_deref() == Some("Balanced coding model")));
        assert!(candidates.iter().any(|c| c.insert_value == "opus"));
    }

    #[test]
    fn non_variable_command_argument_mode_is_disabled() {
        let mut app = App::test_default();
        app.input.set_text("/cancel now");
        let _ = app.input.set_cursor(0, "/cancel now".chars().count());
        sync_with_cursor(&mut app);
        assert!(app.slash.is_none());
    }

    #[test]
    fn variable_command_argument_mode_deactivates_when_no_match() {
        let mut app = App::test_default();
        app.mode = Some(super::super::ModeState {
            current_mode_id: "plan".to_owned(),
            current_mode_name: "Plan".to_owned(),
            available_modes: vec![super::super::ModeInfo {
                id: "plan".to_owned(),
                name: "Plan".to_owned(),
            }],
        });
        app.input.set_text("/mode xyz");
        let _ = app.input.set_cursor(0, "/mode xyz".chars().count());
        sync_with_cursor(&mut app);
        assert!(app.slash.is_none());
    }

    #[test]
    fn confirm_selection_replaces_only_active_argument_token() {
        let mut app = App::test_default();
        app.input.set_text("/resume old-id trailing");
        let _ = app.input.set_cursor(0, "/resume old-id".chars().count());
        app.slash = Some(SlashState {
            trigger_row: 0,
            trigger_col: 8,
            query: "old-id".to_owned(),
            context: SlashContext::Argument {
                command: "/resume".to_owned(),
                arg_index: 0,
                token_range: (8, 14),
            },
            candidates: vec![SlashCandidate {
                insert_value: "new-id".to_owned(),
                primary: "New".to_owned(),
                secondary: None,
            }],
            dialog: DialogState::default(),
        });

        confirm_selection(&mut app);

        assert_eq!(app.input.text(), "/resume new-id trailing");
    }

    #[tokio::test(flavor = "current_thread")]
    async fn login_is_handled_as_builtin_even_when_advertised() {
        tokio::task::LocalSet::new()
            .run_until(async {
                let mut app = App::test_default();
                app.available_commands = vec![model::AvailableCommand::new("/login", "Login")];

                let consumed = try_handle_submit(&mut app, "/login");
                assert!(consumed, "/login should be handled locally even when SDK advertises it");
            })
            .await;
    }

    #[test]
    fn new_session_command_is_rendered_as_user_message() {
        let mut app = App::test_default();

        let consumed = try_handle_submit(&mut app, "/new-session");
        assert!(consumed);
        assert!(app.messages.len() >= 2);

        let Some(first) = app.messages.first() else {
            panic!("expected first message");
        };
        assert!(matches!(first.role, MessageRole::User));
        let Some(MessageBlock::Text(block)) = first.blocks.first() else {
            panic!("expected user text block");
        };
        assert_eq!(block.text, "/new-session");
    }

    #[test]
    fn resume_with_missing_id_returns_usage() {
        let mut app = App::test_default();
        let consumed = try_handle_submit(&mut app, "/resume");
        assert!(consumed);
        let Some(last) = app.messages.last() else {
            panic!("expected usage message");
        };
        let Some(MessageBlock::Text(block)) = last.blocks.first() else {
            panic!("expected text block");
        };
        assert_eq!(block.text, "Usage: /resume <session_id>");
    }

    #[test]
    fn resume_command_is_rendered_as_user_message() {
        let mut app = App::test_default();

        let consumed = try_handle_submit(&mut app, "/resume abc-123");
        assert!(consumed);
        assert!(app.messages.len() >= 2);

        let Some(first) = app.messages.first() else {
            panic!("expected user message");
        };
        assert!(matches!(first.role, MessageRole::User));
        let Some(MessageBlock::Text(block)) = first.blocks.first() else {
            panic!("expected text block");
        };
        assert_eq!(block.text, "/resume abc-123");
    }

    #[tokio::test(flavor = "current_thread")]
    async fn resume_sets_command_pending_when_connected() {
        tokio::task::LocalSet::new()
            .run_until(async {
                let mut app = App::test_default();
                let (tx, mut rx) = tokio::sync::mpsc::unbounded_channel();
                app.conn = Some(std::rc::Rc::new(crate::agent::client::AgentConnection::new(tx)));

                let consumed = try_handle_submit(&mut app, "/resume abc-123");
                assert!(consumed);
                assert!(matches!(app.status, AppStatus::CommandPending));
                assert_eq!(app.resuming_session_id.as_deref(), Some("abc-123"));

                tokio::task::yield_now().await;
                assert!(rx.try_recv().is_ok());
            })
            .await;
    }

    #[tokio::test(flavor = "current_thread")]
    async fn mode_sets_command_pending_and_mode_update_restores_ready() {
        tokio::task::LocalSet::new()
            .run_until(async {
                let mut app = App::test_default();
                let (tx, _rx) = tokio::sync::mpsc::unbounded_channel();
                app.conn = Some(std::rc::Rc::new(crate::agent::client::AgentConnection::new(tx)));
                app.session_id = Some("sess-1".into());
                app.mode = Some(super::super::ModeState {
                    current_mode_id: "code".to_owned(),
                    current_mode_name: "Code".to_owned(),
                    available_modes: vec![
                        super::super::ModeInfo { id: "plan".to_owned(), name: "Plan".to_owned() },
                        super::super::ModeInfo { id: "code".to_owned(), name: "Code".to_owned() },
                    ],
                });

                let consumed = try_handle_submit(&mut app, "/mode plan");
                assert!(consumed);
                assert!(
                    matches!(app.status, AppStatus::CommandPending),
                    "expected CommandPending, got {:?}",
                    app.status
                );
                assert_eq!(app.pending_command_label.as_deref(), Some("Switching mode..."));

                // Simulate mode-update ack arriving from bridge.
                super::super::events::handle_client_event(
                    &mut app,
                    crate::agent::events::ClientEvent::SessionUpdate(
                        crate::agent::model::SessionUpdate::CurrentModeUpdate(
                            crate::agent::model::CurrentModeUpdate::new("plan"),
                        ),
                    ),
                );
                assert!(
                    matches!(app.status, AppStatus::Ready),
                    "expected Ready after CurrentModeUpdate ack, got {:?}",
                    app.status
                );
                assert!(app.pending_command_label.is_none());
            })
            .await;
    }

    #[tokio::test(flavor = "current_thread")]
    async fn model_sets_command_pending_and_config_ack_updates_model_and_restores_ready() {
        tokio::task::LocalSet::new()
            .run_until(async {
                let mut app = App::test_default();
                let (tx, _rx) = tokio::sync::mpsc::unbounded_channel();
                app.conn = Some(std::rc::Rc::new(crate::agent::client::AgentConnection::new(tx)));
                app.session_id = Some("sess-1".into());
                app.model_name = "old-model".to_owned();

                let consumed = try_handle_submit(&mut app, "/model sonnet");
                assert!(consumed);
                assert!(
                    matches!(app.status, AppStatus::CommandPending),
                    "expected CommandPending, got {:?}",
                    app.status
                );
                assert_eq!(app.pending_command_label.as_deref(), Some("Switching model..."));
                assert_eq!(app.model_name, "old-model");

                super::super::events::handle_client_event(
                    &mut app,
                    crate::agent::events::ClientEvent::SessionUpdate(
                        crate::agent::model::SessionUpdate::ConfigOptionUpdate(
                            crate::agent::model::ConfigOptionUpdate {
                                option_id: "model".to_owned(),
                                value: serde_json::Value::String("sonnet".to_owned()),
                            },
                        ),
                    ),
                );
                assert!(
                    matches!(app.status, AppStatus::Ready),
                    "expected Ready after model config ack, got {:?}",
                    app.status
                );
                assert_eq!(app.model_name, "sonnet");
                assert!(app.pending_command_label.is_none());
            })
            .await;
    }

    #[tokio::test(flavor = "current_thread")]
    async fn new_session_sets_command_pending() {
        tokio::task::LocalSet::new()
            .run_until(async {
                let mut app = App::test_default();
                let (tx, _rx) = tokio::sync::mpsc::unbounded_channel();
                app.conn = Some(std::rc::Rc::new(crate::agent::client::AgentConnection::new(tx)));

                let consumed = try_handle_submit(&mut app, "/new-session");
                assert!(consumed);
                assert!(
                    matches!(app.status, AppStatus::CommandPending),
                    "expected CommandPending, got {:?}",
                    app.status
                );
                assert_eq!(app.pending_command_label.as_deref(), Some("Starting new session..."));
            })
            .await;
    }

    #[test]
    fn compact_without_connection_is_handled_locally() {
        let mut app = App::test_default();

        let consumed = try_handle_submit(&mut app, "/compact");
        assert!(consumed);
        assert!(!app.pending_compact_clear);
        let Some(last) = app.messages.last() else {
            panic!("expected system message");
        };
        assert!(matches!(last.role, MessageRole::System(_)));
        let Some(MessageBlock::Text(block)) = last.blocks.first() else {
            panic!("expected text block");
        };
        assert_eq!(block.text, "Cannot compact: not connected yet.");
    }

    #[test]
    fn compact_with_active_session_sets_compacting_without_success_pending() {
        let mut app = App::test_default();
        let (tx, _rx) = tokio::sync::mpsc::unbounded_channel();
        app.conn = Some(std::rc::Rc::new(crate::agent::client::AgentConnection::new(tx)));
        app.session_id = Some(model::SessionId::new("session-1"));

        let consumed = try_handle_submit(&mut app, "/compact");
        assert!(!consumed);
        assert!(!app.pending_compact_clear);
        assert!(app.is_compacting);
    }

    #[test]
    fn compact_with_args_returns_usage_message() {
        let mut app = App::test_default();
        app.messages.push(ChatMessage {
            role: MessageRole::User,
            blocks: vec![MessageBlock::Text(TextBlock::from_complete("keep"))],
            usage: None,
        });

        let consumed = try_handle_submit(&mut app, "/compact now");
        assert!(consumed);
        assert!(app.messages.len() >= 2);
        let Some(last) = app.messages.last() else {
            panic!("expected system usage message");
        };
        assert!(matches!(last.role, MessageRole::System(_)));
        let Some(MessageBlock::Text(block)) = last.blocks.first() else {
            panic!("expected text block");
        };
        assert_eq!(block.text, "Usage: /compact");
    }

    #[test]
    fn mode_with_extra_args_returns_usage_message() {
        let mut app = App::test_default();

        let consumed = try_handle_submit(&mut app, "/mode plan extra");
        assert!(consumed);
        let Some(last) = app.messages.last() else {
            panic!("expected system usage message");
        };
        assert!(matches!(last.role, MessageRole::System(_)));
        let Some(MessageBlock::Text(block)) = last.blocks.first() else {
            panic!("expected text block");
        };
        assert_eq!(block.text, "Usage: /mode <id>");
    }

    #[test]
    fn confirm_selection_with_invalid_trigger_row_is_noop() {
        let mut app = App::test_default();
        app.input.set_text("/mode");
        app.slash = Some(SlashState {
            trigger_row: 99,
            trigger_col: 0,
            query: "m".into(),
            context: SlashContext::CommandName,
            candidates: vec![SlashCandidate {
                insert_value: "/mode".into(),
                primary: "/mode".into(),
                secondary: None,
            }],
            dialog: DialogState::default(),
        });

        confirm_selection(&mut app);

        assert_eq!(app.input.text(), "/mode");
    }

    #[test]
    fn status_opens_config_at_status_tab() {
        let dir = tempfile::tempdir().expect("tempdir");
        let mut app = App::test_default();
        app.settings_home_override = Some(dir.path().to_path_buf());

        let consumed = try_handle_submit(&mut app, "/status");

        assert!(consumed);
        assert_eq!(app.active_view, super::super::ActiveView::Config);
        assert_eq!(app.config.active_tab, super::super::ConfigTab::Status);
    }

    #[test]
    fn usage_opens_config_at_usage_tab() {
        let dir = tempfile::tempdir().expect("tempdir");
        let mut app = App::test_default();
        app.settings_home_override = Some(dir.path().to_path_buf());

        let consumed = try_handle_submit(&mut app, "/usage");

        assert!(consumed);
        assert_eq!(app.active_view, super::super::ActiveView::Config);
        assert_eq!(app.config.active_tab, super::super::ConfigTab::Usage);
    }

    #[test]
    fn status_with_extra_args_returns_usage() {
        let mut app = App::test_default();

        let consumed = try_handle_submit(&mut app, "/status extra");

        assert!(consumed);
        let Some(last) = app.messages.last() else {
            panic!("expected usage message");
        };
        let Some(MessageBlock::Text(block)) = last.blocks.first() else {
            panic!("expected text block");
        };
        assert_eq!(block.text, "Usage: /status");
    }

    #[test]
    fn usage_with_extra_args_returns_usage() {
        let mut app = App::test_default();

        let consumed = try_handle_submit(&mut app, "/usage extra");

        assert!(consumed);
        let Some(last) = app.messages.last() else {
            panic!("expected usage message");
        };
        let Some(MessageBlock::Text(block)) = last.blocks.first() else {
            panic!("expected text block");
        };
        assert_eq!(block.text, "Usage: /usage");
    }

    #[test]
    fn status_appears_in_candidates() {
        let app = App::test_default();
        let names: Vec<String> =
            supported_command_candidates(&app).into_iter().map(|c| c.primary).collect();
        assert!(names.iter().any(|n| n == "/status"), "missing /status");
    }

    #[test]
    fn usage_appears_in_candidates() {
        let app = App::test_default();
        let names: Vec<String> =
            supported_command_candidates(&app).into_iter().map(|c| c.primary).collect();
        assert!(names.iter().any(|n| n == "/usage"), "missing /usage");
    }

    #[test]
    fn mcp_appears_in_candidates() {
        let app = App::test_default();
        let names: Vec<String> =
            supported_command_candidates(&app).into_iter().map(|c| c.primary).collect();
        assert!(names.iter().any(|n| n == "/mcp"), "missing /mcp");
    }
}
