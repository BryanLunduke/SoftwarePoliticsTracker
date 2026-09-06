// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

//! App creation and bridge connection lifecycle.
//!
//! Submodules:
//! - `bridge_lifecycle`: spawning the bridge process, init handshake, event loop
//! - `event_dispatch`: routing `BridgeEvent` envelopes to `ClientEvent` messages
//! - `type_converters`: bridge wire types -> app model types

mod bridge_lifecycle;
mod event_dispatch;
mod session_start;
mod type_converters;

use super::config::ConfigState;
use super::dialog::DialogState;
use super::plugins::PluginsState;
use super::state::{
    CacheMetrics, HistoryRetentionPolicy, HistoryRetentionStats, RenderCacheBudget,
};
use super::trust;
use super::view::ActiveView;
use super::{App, AppStatus, ChatViewport, FocusManager, HelpView, SelectionState, TodoItem};
use crate::Cli;
use crate::agent::client::AgentConnection;
use crate::agent::events::ClientEvent;
use crate::agent::model;
use crate::agent::wire::SessionLaunchSettings;
use crate::error::AppError;
use std::collections::{HashMap, HashSet};
use std::path::PathBuf;
use std::rc::Rc;
use tokio::sync::mpsc;

/// Shorten cwd for display: use `~` for the home directory prefix.
fn shorten_cwd(cwd: &std::path::Path) -> String {
    let cwd_str = cwd.to_string_lossy().to_string();
    if let Some(home) = dirs::home_dir() {
        let home_str = home.to_string_lossy().to_string();
        if cwd_str.starts_with(&home_str) {
            return format!("~{}", &cwd_str[home_str.len()..]);
        }
    }
    cwd_str
}

fn resolve_startup_cwd(cli: &Cli) -> PathBuf {
    cli.dir
        .clone()
        .unwrap_or_else(|| std::env::current_dir().unwrap_or_else(|_| PathBuf::from(".")))
}

fn extract_app_error(err: &anyhow::Error) -> Option<AppError> {
    err.chain().find_map(|cause| cause.downcast_ref::<AppError>().cloned())
}

struct StartConnectionParams {
    event_tx: mpsc::UnboundedSender<ClientEvent>,
    cwd_raw: String,
    bridge_script: Option<std::path::PathBuf>,
    resume_id: Option<String>,
    resume_requested: bool,
    session_launch_settings: SessionLaunchSettings,
}

pub(crate) use session_start::{SessionStartReason, resume_session, start_new_session};

/// Create the `App` struct in `Connecting` state and load shared settings state.
#[allow(clippy::too_many_lines)]
pub fn create_app(cli: &Cli) -> App {
    let cwd = resolve_startup_cwd(cli);

    let (event_tx, event_rx) = mpsc::unbounded_channel();
    let terminals: crate::agent::events::TerminalMap =
        Rc::new(std::cell::RefCell::new(HashMap::new()));

    let cwd_display = shorten_cwd(&cwd);
    let initial_model_name = "Connecting...".to_owned();

    let mut app = App {
        active_view: ActiveView::Chat,
        config: ConfigState::default(),
        trust: trust::TrustState::default(),
        settings_home_override: None,
        messages: vec![super::ChatMessage::welcome_with_recent(
            &initial_model_name,
            &cwd_display,
            &[],
        )],
        message_retained_bytes: Vec::new(),
        retained_history_bytes: 0,
        viewport: ChatViewport::new(),
        input: super::InputState::new(),
        status: AppStatus::Connecting,
        resuming_session_id: None,
        pending_command_label: None,
        pending_command_ack: None,
        should_quit: false,
        exit_error: None,
        session_id: None,
        conn: None,
        session_scope_epoch: 0,
        model_name: initial_model_name,
        welcome_model_resolved: false,
        cwd_raw: cwd.to_string_lossy().to_string(),
        cwd: cwd_display,
        files_accessed: 0,
        mode: None,
        config_options: std::collections::BTreeMap::new(),
        login_hint: None,
        pending_compact_clear: false,
        help_view: HelpView::Keys,
        help_open: false,
        help_dialog: DialogState::default(),
        help_visible_count: 0,
        pending_interaction_ids: Vec::new(),
        cancelled_turn_pending_hint: false,
        pending_cancel_origin: None,
        pending_auto_submit_after_cancel: false,
        event_tx,
        event_rx,
        spinner_frame: 0,
        spinner_last_advance_at: None,
        active_turn_assistant_message_idx: None,
        tools_collapsed: true,
        active_task_ids: HashSet::new(),
        tool_call_scopes: HashMap::new(),
        active_subagent_tool_ids: HashSet::new(),
        subagent_idle_since: None,
        terminals,
        force_redraw: false,
        tool_call_index: HashMap::new(),
        todos: Vec::<TodoItem>::new(),
        show_todo_panel: false,
        todo_scroll: 0,
        todo_selected: 0,
        focus: FocusManager::default(),
        available_commands: Vec::new(),
        plugins: PluginsState::default(),
        available_agents: Vec::new(),
        available_models: Vec::new(),
        recent_sessions: Vec::new(),
        cached_frame_area: ratatui::layout::Rect::new(0, 0, 0, 0),
        selection: Option::<SelectionState>::None,
        scrollbar_drag: None,
        rendered_chat_lines: Vec::new(),
        rendered_chat_area: ratatui::layout::Rect::new(0, 0, 0, 0),
        rendered_input_lines: Vec::new(),
        rendered_input_area: ratatui::layout::Rect::new(0, 0, 0, 0),
        mention: None,
        slash: None,
        subagent: None,
        pending_submit: None,
        paste_burst: super::paste_burst::PasteBurstDetector::new(),
        pending_paste_text: String::new(),
        pending_paste_session: None,
        active_paste_session: None,
        next_paste_session_id: 1,
        cached_todo_compact: None,
        git_branch: None,
        update_check_hint: None,
        session_usage: super::SessionUsageState::default(),
        usage: super::UsageState::default(),
        mcp: super::McpState::default(),
        fast_mode_state: model::FastModeState::Off,
        last_rate_limit_update: None,
        is_compacting: false,
        account_info: None,
        terminal_tool_calls: Vec::new(),
        terminal_tool_call_membership: HashSet::new(),
        needs_redraw: true,
        notifications: super::notify::NotificationManager::new(),
        perf: cli
            .perf_log
            .as_deref()
            .and_then(|path| crate::perf::PerfLogger::open(path, cli.perf_append)),
        render_cache_budget: RenderCacheBudget::default(),
        render_cache_slots: Vec::new(),
        render_cache_total_bytes: 0,
        render_cache_protected_bytes: 0,
        render_cache_evictable: std::collections::BTreeSet::new(),
        render_cache_tail_msg_idx: None,
        history_retention: HistoryRetentionPolicy::default(),
        history_retention_stats: HistoryRetentionStats::default(),
        cache_metrics: CacheMetrics::default(),
        fps_ema: None,
        last_frame_at: None,
        startup_connection_requested: false,
        connection_started: false,
        startup_bridge_script: cli.bridge_script.clone(),
        startup_resume_id: cli.resume.clone(),
        startup_resume_requested: cli.resume.is_some(),
    };

    if let Err(err) = super::config::initialize_shared_state(&mut app) {
        tracing::warn!("failed to initialize shared settings state: {err}");
        app.config.last_error = Some(err);
    }

    app.rebuild_history_retention_accounting();
    app.rebuild_render_cache_accounting();
    trust::initialize(&mut app);
    app.refresh_git_branch();
    app
}

/// Spawn the background bridge task.
pub fn start_connection(app: &mut App) {
    if !app.startup_connection_requested || app.connection_started {
        return;
    }

    app.connection_started = true;
    let params = StartConnectionParams {
        event_tx: app.event_tx.clone(),
        cwd_raw: app.cwd_raw.clone(),
        bridge_script: app.startup_bridge_script.clone(),
        resume_id: app.startup_resume_id.clone(),
        resume_requested: app.startup_resume_requested,
        session_launch_settings: session_start::session_launch_settings_for_reason(
            app,
            session_start::SessionStartReason::Startup,
        ),
    };
    let conn_slot: Rc<std::cell::RefCell<Option<ConnectionSlot>>> =
        Rc::new(std::cell::RefCell::new(None));
    let conn_slot_writer = Rc::clone(&conn_slot);

    tokio::task::spawn_local(async move {
        bridge_lifecycle::run_connection_task(params, conn_slot_writer).await;
    });

    CONN_SLOT.with(|slot| {
        debug_assert!(
            slot.borrow().is_none(),
            "CONN_SLOT already populated -- start_connection() called twice?"
        );
        *slot.borrow_mut() = Some(conn_slot);
    });
}

/// Shared slot for passing `Rc<AgentConnection>` from the background task to the event loop.
pub struct ConnectionSlot {
    pub conn: Rc<AgentConnection>,
}

thread_local! {
    pub static CONN_SLOT: std::cell::RefCell<Option<Rc<std::cell::RefCell<Option<ConnectionSlot>>>>> =
        const { std::cell::RefCell::new(None) };
}

/// Take the connection data from the thread-local slot.
pub(super) fn take_connection_slot() -> Option<ConnectionSlot> {
    CONN_SLOT.with(|slot| slot.borrow().as_ref().and_then(|inner| inner.borrow_mut().take()))
}

#[cfg(test)]
mod tests {
    use super::type_converters::map_session_update;
    use crate::agent::model;
    use crate::agent::types;

    #[test]
    fn map_session_update_preserves_config_option_update() {
        let mapped = map_session_update(types::SessionUpdate::ConfigOptionUpdate {
            option_id: "model".to_owned(),
            value: serde_json::Value::String("sonnet".to_owned()),
        });

        let Some(model::SessionUpdate::ConfigOptionUpdate(cfg)) = mapped else {
            panic!("expected ConfigOptionUpdate mapping");
        };
        assert_eq!(cfg.option_id, "model");
        assert_eq!(cfg.value, serde_json::Value::String("sonnet".to_owned()));
    }
}
