// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

pub mod block_cache;
pub mod cache_metrics;
mod history_retention;
pub mod messages;
mod render_budget;
pub mod tool_call_info;
pub mod types;
pub mod viewport;

// Re-export all public types so external `use crate::app::state::X` paths still work.
pub use block_cache::BlockCache;
pub use cache_metrics::CacheMetrics;
pub(crate) use messages::MarkdownRenderKey;
pub use messages::{
    ChatMessage, IncrementalMarkdown, MessageBlock, MessageRole, SystemSeverity, TextBlock,
    TextBlockSpacing, WelcomeBlock,
};
pub use tool_call_info::{
    InlinePermission, InlineQuestion, TerminalSnapshotMode, ToolCallInfo, is_execute_tool_name,
};
pub use types::{
    AppStatus, CancelOrigin, ExtraUsage, HelpView, HistoryRetentionPolicy, HistoryRetentionStats,
    LoginHint, McpState, MessageUsage, ModeInfo, ModeState, PasteSessionState, PendingCommandAck,
    RecentSessionInfo, RenderCacheBudget, SUBAGENT_THINKING_DEBOUNCE, ScrollbarDragState,
    SelectionKind, SelectionPoint, SelectionState, SessionUsageState, TodoItem, TodoStatus,
    ToolCallScope, UsageSnapshot, UsageSourceKind, UsageSourceMode, UsageState, UsageWindow,
};
pub use viewport::{
    ChatViewport, LayoutInvalidation, LayoutInvalidation as InvalidationLevel,
    LayoutRemeasureReason,
};

use crate::agent::events::ClientEvent;
use crate::agent::model;
use std::collections::{BTreeMap, BTreeSet, HashMap, HashSet};
use std::path::{Path, PathBuf};
use std::rc::Rc;
use std::time::Instant;
use tokio::sync::mpsc;

use super::config::ConfigState;
use super::dialog;
use super::focus::{FocusContext, FocusManager, FocusOwner, FocusTarget};
use super::input::{InputSnapshot, InputState, parse_paste_placeholder_before_cursor};
use super::mention;
use super::plugins::PluginsState;
use super::slash;
use super::subagent;
use super::trust::TrustState;
use super::view::ActiveView;

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct TerminalToolCallRef {
    pub terminal_id: String,
    pub msg_idx: usize,
    pub block_idx: usize,
}

impl TerminalToolCallRef {
    #[must_use]
    pub fn new(terminal_id: String, msg_idx: usize, block_idx: usize) -> Self {
        Self { terminal_id, msg_idx, block_idx }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AutocompleteKind {
    Mention,
    Slash,
    Subagent,
}

#[allow(clippy::struct_excessive_bools)]
pub struct App {
    pub active_view: ActiveView,
    pub config: ConfigState,
    pub trust: TrustState,
    pub settings_home_override: Option<PathBuf>,
    pub messages: Vec<ChatMessage>,
    /// Cached approximate retained bytes for each message, parallel to `messages`.
    pub message_retained_bytes: Vec<usize>,
    /// Rolling total of `message_retained_bytes`.
    pub retained_history_bytes: usize,
    /// Single owner of all chat layout state: scroll, per-message heights, prefix sums.
    pub viewport: ChatViewport,
    pub input: InputState,
    pub status: AppStatus,
    /// Session id currently being resumed via `/resume`.
    pub resuming_session_id: Option<String>,
    /// Spinner label shown while a slash command is in flight (`CommandPending`).
    pub pending_command_label: Option<String>,
    /// Ack marker required to clear `CommandPending` for strict completion semantics.
    pub pending_command_ack: Option<PendingCommandAck>,
    pub should_quit: bool,
    /// Optional fatal app error that should be surfaced at CLI boundary.
    pub exit_error: Option<crate::error::AppError>,
    pub session_id: Option<model::SessionId>,
    /// Agent connection handle. `None` while connecting (before bridge is ready).
    pub conn: Option<Rc<crate::agent::client::AgentConnection>>,
    /// Monotonic session authority epoch used to ignore stale async view data.
    pub session_scope_epoch: u64,
    pub model_name: String,
    /// True once the welcome banner has captured its one-time session model label.
    pub welcome_model_resolved: bool,
    pub cwd: String,
    pub cwd_raw: String,
    pub files_accessed: usize,
    pub mode: Option<ModeState>,
    /// Latest config options observed from bridge `config_option_update` events.
    pub config_options: BTreeMap<String, serde_json::Value>,
    /// Login hint shown when authentication is required. Rendered above the input field.
    pub login_hint: Option<LoginHint>,
    /// When true, the current/next turn completion should clear local conversation history.
    /// Set by `/compact` once the command is accepted for bridge forwarding.
    pub pending_compact_clear: bool,
    /// Active help overlay view when `?` help is open.
    pub help_view: HelpView,
    /// Whether the help overlay is explicitly open.
    pub help_open: bool,
    /// Scroll/selection state for the Slash and Subagents help tabs.
    pub help_dialog: dialog::DialogState,
    /// Number of items that currently fit in the help viewport (updated each render).
    /// Used by key handlers for accurate scroll step size.
    pub help_visible_count: usize,
    /// Tool call IDs with pending inline interactions, ordered by arrival.
    /// The first entry is the focused interaction that receives keyboard input.
    /// Up / Down arrow keys cycle focus through the list.
    pub pending_interaction_ids: Vec<String>,
    /// Set when a cancel notification succeeds; consumed on `TurnComplete`
    /// to render a red interruption hint in chat.
    pub cancelled_turn_pending_hint: bool,
    /// Origin of the in-flight cancellation request, if any.
    pub pending_cancel_origin: Option<CancelOrigin>,
    /// Auto-submit the current input draft once cancellation transitions the app
    /// back to `Ready`.
    pub pending_auto_submit_after_cancel: bool,
    pub event_tx: mpsc::UnboundedSender<ClientEvent>,
    pub event_rx: mpsc::UnboundedReceiver<ClientEvent>,
    pub spinner_frame: usize,
    pub spinner_last_advance_at: Option<Instant>,
    /// Message index that owns the current main-assistant turn indicators.
    pub active_turn_assistant_message_idx: Option<usize>,
    /// Session-level preference for collapsing non-Execute tool call bodies.
    /// Toggled by Ctrl+O and applied at render/layout time.
    pub tools_collapsed: bool,
    /// IDs of Task/Agent tool calls currently `InProgress` -- their children get hidden.
    /// Use `insert_active_task()`, `remove_active_task()`.
    pub active_task_ids: HashSet<String>,
    /// Tool scope keyed by tool call ID; used to distinguish main-agent from subagent tools.
    pub tool_call_scopes: HashMap<String, ToolCallScope>,
    /// IDs of non Task/Agent subagent tool calls currently `InProgress`/`Pending`.
    pub active_subagent_tool_ids: HashSet<String>,
    /// Timestamp when subagent entered an idle gap (no active child tool calls).
    pub subagent_idle_since: Option<Instant>,
    /// Shared terminal process map - used to snapshot output on completion.
    pub terminals: crate::agent::events::TerminalMap,
    /// Force a full terminal clear on next render frame.
    pub force_redraw: bool,
    /// O(1) lookup: `tool_call_id` -> `(message_index, block_index)`.
    /// Use `lookup_tool_call()`, `index_tool_call()`.
    pub tool_call_index: HashMap<String, (usize, usize)>,
    /// Current todo list from Claude's `TodoWrite` tool calls.
    pub todos: Vec<TodoItem>,
    /// Whether the todo panel is expanded (true) or shows compact status line (false).
    /// Toggled by Ctrl+T.
    pub show_todo_panel: bool,
    /// Scroll offset for the expanded todo panel (capped at 5 visible lines).
    pub todo_scroll: usize,
    /// Selected todo index used for keyboard navigation in the open todo panel.
    pub todo_selected: usize,
    /// Focus manager for directional/navigation key ownership.
    pub focus: FocusManager,
    /// Commands advertised by the agent via `AvailableCommandsUpdate`.
    pub available_commands: Vec<model::AvailableCommand>,
    /// Plugin inventory and UI state for the Config > Plugins view.
    pub plugins: PluginsState,
    /// Subagents advertised by the agent via `AvailableAgentsUpdate`.
    pub available_agents: Vec<model::AvailableAgent>,
    /// Models advertised by the agent SDK for the active session.
    pub available_models: Vec<model::AvailableModel>,
    /// Recently persisted session IDs discovered at startup.
    pub recent_sessions: Vec<RecentSessionInfo>,
    /// Last known frame area (for mouse selection mapping).
    pub cached_frame_area: ratatui::layout::Rect,
    /// Current selection state for mouse-based selection.
    pub selection: Option<SelectionState>,
    /// Active scrollbar drag state while left mouse button is held on the rail.
    pub scrollbar_drag: Option<ScrollbarDragState>,
    /// Cached rendered chat lines for selection/copy.
    pub rendered_chat_lines: Vec<String>,
    /// Area where chat content was rendered (for selection mapping).
    pub rendered_chat_area: ratatui::layout::Rect,
    /// Cached rendered input lines for selection/copy.
    pub rendered_input_lines: Vec<String>,
    /// Area where input content was rendered (for selection mapping).
    pub rendered_input_area: ratatui::layout::Rect,
    /// Active `@` file mention autocomplete state.
    pub mention: Option<mention::MentionState>,
    /// Active slash-command autocomplete state.
    pub slash: Option<slash::SlashState>,
    /// Active subagent autocomplete state (`&name`).
    pub subagent: Option<subagent::SubagentState>,
    /// Deferred plain-Enter submit. Stores the exact input state from before the
    /// Enter key so submission can restore and use the original draft text.
    ///
    /// If another editing-like event or a paste payload arrives in the same
    /// drain cycle, this is cleared and no submit occurs.
    pub pending_submit: Option<InputSnapshot>,
    /// Timing-based paste burst detector. Detects rapid character streams
    /// (paste delivered as individual key events) and buffers them into a
    /// single paste payload. Fallback for terminals without bracketed paste.
    pub paste_burst: super::paste_burst::PasteBurstDetector,
    /// Buffered `Event::Paste` payload for this drain cycle.
    /// Some terminals split one clipboard paste into multiple chunks; we merge
    /// them and apply placeholder threshold to the merged content once per cycle.
    pub pending_paste_text: String,
    /// Pending paste session metadata for the currently queued `Event::Paste` payload.
    pub pending_paste_session: Option<PasteSessionState>,
    /// Most recent active placeholder paste session, used for safe chunk continuation.
    pub active_paste_session: Option<PasteSessionState>,
    /// Monotonic counter for paste session identifiers.
    pub next_paste_session_id: u64,
    /// Cached todo compact line (invalidated on `set_todos()`).
    pub cached_todo_compact: Option<ratatui::text::Line<'static>>,
    /// Current git branch (refreshed on focus gain + turn complete).
    pub git_branch: Option<String>,
    /// Optional startup update-check hint rendered at the footer's right edge.
    pub update_check_hint: Option<String>,
    /// Session-wide usage and cost telemetry from the bridge.
    pub session_usage: SessionUsageState,
    /// Config > Usage snapshot and refresh lifecycle.
    pub usage: UsageState,
    /// Config > MCP live server snapshot and refresh lifecycle.
    pub mcp: McpState,
    /// Fast mode state telemetry from the SDK.
    pub fast_mode_state: model::FastModeState,
    /// Latest rate-limit telemetry from the SDK.
    pub last_rate_limit_update: Option<model::RateLimitUpdate>,
    /// True while the SDK reports active compaction.
    pub is_compacting: bool,
    /// Account info from the bridge status snapshot (email, org, subscription).
    pub account_info: Option<crate::agent::types::AccountInfo>,

    /// Indexed terminal tool calls for per-frame terminal snapshot updates.
    /// Avoids O(n*m) scan of all messages/blocks every frame.
    pub terminal_tool_calls: Vec<TerminalToolCallRef>,
    /// Membership index for `terminal_tool_calls`, used to avoid linear duplicate checks.
    pub terminal_tool_call_membership: HashSet<TerminalToolCallRef>,
    /// Dirty flag: skip `terminal.draw()` when nothing changed since last frame.
    pub needs_redraw: bool,
    /// Central notification manager (bell + desktop toast when unfocused).
    pub notifications: super::notify::NotificationManager,
    /// Performance logger. Present only when built with `--features perf`.
    /// Taken out (`Option::take`) during render, used, then put back to avoid
    /// borrow conflicts with `&mut App`.
    pub perf: Option<crate::perf::PerfLogger>,
    /// Global in-memory budget for rendered block caches (message + tool + welcome).
    pub render_cache_budget: RenderCacheBudget,
    /// Cached render-cache slot metadata parallel to `messages[*].blocks[*]`.
    pub(crate) render_cache_slots: Vec<Vec<render_budget::RenderCacheSlotState>>,
    /// Rolling total of cached render bytes across all blocks.
    pub(crate) render_cache_total_bytes: usize,
    /// Rolling total of cached render bytes currently excluded from the budget.
    pub(crate) render_cache_protected_bytes: usize,
    /// Evictable cached blocks ordered by LRU and size tie-breaker.
    pub(crate) render_cache_evictable: BTreeSet<render_budget::RenderCacheEvictionKey>,
    /// Last message index currently protected as the streaming tail, if any.
    pub(crate) render_cache_tail_msg_idx: Option<usize>,
    /// Byte budget for source conversation history retained in memory.
    pub history_retention: HistoryRetentionPolicy,
    /// Last history-retention enforcement statistics.
    pub history_retention_stats: HistoryRetentionStats,
    /// Cross-cutting cache metrics accumulator (enforcement counts, watermarks, rate limits).
    pub cache_metrics: CacheMetrics,
    /// Smoothed frames-per-second (EMA of presented frame cadence).
    pub fps_ema: Option<f32>,
    /// Timestamp of the previous presented frame.
    pub last_frame_at: Option<Instant>,
    pub startup_connection_requested: bool,
    pub connection_started: bool,
    pub startup_bridge_script: Option<PathBuf>,
    pub startup_resume_id: Option<String>,
    pub startup_resume_requested: bool,
}

impl App {
    /// Queue a paste payload for drain-cycle finalization.
    ///
    /// This is fed by paste payloads captured from terminal events.
    pub fn queue_paste_text(&mut self, text: &str) {
        if text.is_empty() {
            return;
        }
        tracing::debug!(
            text = %debug_paste_text(text),
            pending_len = self.pending_paste_text.chars().count(),
            had_pending_submit = self.pending_submit.is_some(),
            "paste_queue: enqueue"
        );
        self.pending_submit = None;
        if self.pending_paste_text.is_empty() {
            let continued_session = self.active_paste_session.and_then(|session| {
                let current_line = self.input.lines().get(self.input.cursor_row())?;
                let idx =
                    parse_paste_placeholder_before_cursor(current_line, self.input.cursor_col())?;
                (session.placeholder_index == Some(idx)).then_some(session)
            });
            self.pending_paste_session = Some(continued_session.unwrap_or_else(|| {
                let id = self.next_paste_session_id;
                self.next_paste_session_id = self.next_paste_session_id.saturating_add(1);
                PasteSessionState {
                    id,
                    start: SelectionPoint {
                        row: self.input.cursor_row(),
                        col: self.input.cursor_col(),
                    },
                    placeholder_index: None,
                }
            }));
            if let Some(session) = self.pending_paste_session {
                tracing::debug!(
                    session_id = session.id,
                    start_row = session.start.row,
                    start_col = session.start.col,
                    placeholder_index = ?session.placeholder_index,
                    "paste_queue: opened session"
                );
            }
        }
        self.pending_paste_text.push_str(text);
        tracing::debug!(
            merged = %debug_paste_text(&self.pending_paste_text),
            merged_len = self.pending_paste_text.chars().count(),
            "paste_queue: merged pending text"
        );
    }

    /// Mark one presented frame at `now`, updating smoothed FPS.
    pub fn mark_frame_presented(&mut self, now: Instant) {
        let Some(prev) = self.last_frame_at.replace(now) else {
            return;
        };
        let dt = now.saturating_duration_since(prev).as_secs_f32();
        if dt <= f32::EPSILON {
            return;
        }
        let fps = (1.0 / dt).clamp(0.0, 240.0);
        self.fps_ema = Some(match self.fps_ema {
            Some(current) => current * 0.9 + fps * 0.1,
            None => fps,
        });
    }

    #[must_use]
    pub fn is_project_trusted(&self) -> bool {
        self.trust.is_trusted()
    }

    #[must_use]
    pub fn frame_fps(&self) -> Option<f32> {
        self.fps_ema
    }

    /// Ensure the synthetic welcome message exists at index 0.
    pub fn ensure_welcome_message(&mut self) {
        if self.messages.first().is_some_and(|m| matches!(m.role, MessageRole::Welcome)) {
            return;
        }
        self.insert_message_tracked(
            0,
            ChatMessage::welcome_with_recent(
                self.welcome_model_display_name(),
                &self.cwd,
                &self.recent_sessions,
            ),
        );
        self.welcome_model_resolved = self.model_name_is_authoritative();
    }

    fn model_name_is_authoritative(&self) -> bool {
        let model_name = self.model_name.trim();
        if model_name.is_empty() || model_name == "Connecting..." {
            return false;
        }
        if model_name != "default" {
            return true;
        }
        matches!(
            crate::app::config::store::model(&self.config.committed_settings_document),
            Ok(Some(configured_model)) if configured_model.trim() == "default"
        )
    }

    #[must_use]
    pub fn model_display_name(&self) -> &str {
        let model_name = self.model_name.trim();
        if self.session_id.is_none()
            && (model_name.is_empty() || model_name == "Connecting..." || model_name == "default")
        {
            "Connecting..."
        } else if model_name.is_empty() || model_name == "Connecting..." {
            "default"
        } else {
            &self.model_name
        }
    }

    #[must_use]
    fn welcome_model_display_name(&self) -> &str {
        self.model_display_name()
    }

    /// Update the welcome message's model name once, when the session model becomes authoritative.
    pub fn update_welcome_model_once(&mut self) {
        if self.welcome_model_resolved {
            return;
        }
        let welcome_model = self.welcome_model_display_name().to_owned();
        let model_is_authoritative = self.model_name_is_authoritative();
        let Some(first) = self.messages.first_mut() else {
            return;
        };
        if !matches!(first.role, MessageRole::Welcome) {
            return;
        }
        let Some(MessageBlock::Welcome(welcome)) = first.blocks.first_mut() else {
            return;
        };
        if welcome.model_name != welcome_model {
            welcome.model_name = welcome_model;
            welcome.cache.invalidate();
            self.sync_render_cache_slot(0, 0);
            self.recompute_message_retained_bytes(0);
            self.invalidate_layout(InvalidationLevel::MessagesFrom(0));
        }
        if model_is_authoritative {
            self.welcome_model_resolved = true;
        }
    }

    /// Update the welcome message with latest discovered recent sessions.
    pub fn sync_welcome_recent_sessions(&mut self) {
        let Some(first) = self.messages.first_mut() else {
            return;
        };
        if !matches!(first.role, MessageRole::Welcome) {
            return;
        }
        let Some(MessageBlock::Welcome(welcome)) = first.blocks.first_mut() else {
            return;
        };
        welcome.recent_sessions.clone_from(&self.recent_sessions);
        welcome.cache.invalidate();
        self.sync_render_cache_slot(0, 0);
        self.recompute_message_retained_bytes(0);
        self.invalidate_layout(InvalidationLevel::MessagesFrom(0));
    }

    /// Track a Task/Agent tool call as active (in-progress subagent).
    pub fn insert_active_task(&mut self, id: String) {
        self.active_task_ids.insert(id);
    }

    /// Remove a Task/Agent tool call from the active set (completed/failed).
    pub fn remove_active_task(&mut self, id: &str) {
        self.active_task_ids.remove(id);
    }

    pub fn register_tool_call_scope(&mut self, id: String, scope: ToolCallScope) {
        self.tool_call_scopes.insert(id, scope);
    }

    #[must_use]
    pub fn tool_call_scope(&self, id: &str) -> Option<ToolCallScope> {
        self.tool_call_scopes.get(id).copied()
    }

    #[must_use]
    pub(crate) fn tracked_terminal_id_for_tool(tc: &ToolCallInfo) -> Option<String> {
        (tc.is_execute_tool()
            && matches!(
                tc.status,
                model::ToolCallStatus::Pending | model::ToolCallStatus::InProgress
            ))
        .then(|| tc.terminal_id.clone())
        .flatten()
    }

    pub fn mark_subagent_tool_started(&mut self, id: &str) {
        self.active_subagent_tool_ids.insert(id.to_owned());
        self.subagent_idle_since = None;
    }

    pub fn mark_subagent_tool_finished(&mut self, id: &str, now: Instant) {
        self.active_subagent_tool_ids.remove(id);
        self.refresh_subagent_idle_since(now);
    }

    pub fn refresh_subagent_idle_since(&mut self, now: Instant) {
        if self.active_task_ids.is_empty() || !self.active_subagent_tool_ids.is_empty() {
            self.subagent_idle_since = None;
            return;
        }
        if self.subagent_idle_since.is_none() {
            self.subagent_idle_since = Some(now);
        }
    }

    #[must_use]
    pub fn should_show_subagent_thinking(&self, now: Instant) -> bool {
        if self.active_task_ids.is_empty() || !self.active_subagent_tool_ids.is_empty() {
            return false;
        }
        self.subagent_idle_since
            .is_some_and(|since| now.saturating_duration_since(since) >= SUBAGENT_THINKING_DEBOUNCE)
    }

    pub fn clear_tool_scope_tracking(&mut self) {
        self.tool_call_scopes.clear();
        self.active_task_ids.clear();
        self.active_subagent_tool_ids.clear();
        self.subagent_idle_since = None;
    }

    /// Look up the (`message_index`, `block_index`) for a tool call ID.
    #[must_use]
    pub fn lookup_tool_call(&self, id: &str) -> Option<(usize, usize)> {
        self.tool_call_index.get(id).copied()
    }

    /// Register a tool call's position in the message/block arrays.
    pub fn index_tool_call(&mut self, id: String, msg_idx: usize, block_idx: usize) {
        self.tool_call_index.insert(id, (msg_idx, block_idx));
    }

    pub(crate) fn sync_terminal_tool_call(
        &mut self,
        terminal_id: String,
        msg_idx: usize,
        block_idx: usize,
    ) {
        let desired = TerminalToolCallRef::new(terminal_id, msg_idx, block_idx);
        if self.terminal_tool_call_membership.contains(&desired) {
            return;
        }
        self.untrack_terminal_tool_call(msg_idx, block_idx);
        self.terminal_tool_call_membership.insert(desired.clone());
        self.terminal_tool_calls.push(desired);
    }

    pub(crate) fn untrack_terminal_tool_call(&mut self, msg_idx: usize, block_idx: usize) {
        let removed: Vec<_> = self
            .terminal_tool_calls
            .iter()
            .filter(|entry| entry.msg_idx == msg_idx && entry.block_idx == block_idx)
            .cloned()
            .collect();
        if removed.is_empty() {
            return;
        }
        self.terminal_tool_calls
            .retain(|entry| entry.msg_idx != msg_idx || entry.block_idx != block_idx);
        for entry in removed {
            self.terminal_tool_call_membership.remove(&entry);
        }
    }

    pub(crate) fn clear_terminal_tool_call_tracking(&mut self) {
        self.terminal_tool_calls.clear();
        self.terminal_tool_call_membership.clear();
    }

    pub(crate) fn sync_after_message_blocks_changed(&mut self, msg_idx: usize) {
        self.note_render_cache_structure_changed();
        self.sync_render_cache_message(msg_idx);
        self.recompute_message_retained_bytes(msg_idx);
    }

    /// Invalidate message layout caches at the given level.
    ///
    /// Single entry point for all layout invalidation. Replaces the former
    /// `mark_message_layout_dirty` / `mark_all_message_layout_dirty` methods.
    pub fn invalidate_layout(&mut self, level: LayoutInvalidation) {
        match level {
            LayoutInvalidation::MessageChanged(idx) => {
                self.viewport.invalidate_message(idx);
            }
            LayoutInvalidation::MessagesFrom(idx) => {
                self.viewport.invalidate_messages_from(idx);
            }
            LayoutInvalidation::Global => {
                if self.messages.is_empty() {
                    return;
                }
                self.viewport.invalidate_all_messages(LayoutRemeasureReason::Global);
                self.viewport.bump_layout_generation();
            }
            LayoutInvalidation::Resize => {
                // Resize is handled by viewport.on_frame(). This arm exists
                // for exhaustiveness; production code should not reach it.
                debug_assert!(false, "Resize should not be dispatched through invalidate_layout");
            }
        }
    }

    pub(crate) fn invalidate_message_set<I>(&mut self, indices: I)
    where
        I: IntoIterator<Item = usize>,
    {
        let unique: BTreeSet<_> =
            indices.into_iter().filter(|&idx| idx < self.messages.len()).collect();
        for idx in unique {
            self.viewport.invalidate_message(idx);
        }
    }

    /// Enforce history retention and record metrics.
    ///
    /// Wrapper around [`enforce_history_retention`] that feeds the returned stats
    /// into `CacheMetrics` and emits rate-limited structured tracing. Call this
    /// instead of `enforce_history_retention()` at all non-test call sites.
    pub fn enforce_history_retention_tracked(&mut self) {
        let stats = self.enforce_history_retention();
        let should_log =
            self.cache_metrics.record_history_enforcement(&stats, self.history_retention);
        if should_log {
            let snap = cache_metrics::build_snapshot(
                &self.render_cache_budget,
                &self.history_retention_stats,
                self.history_retention,
                &self.cache_metrics,
                &self.viewport,
                0, // entry_count not needed for history-only log
                0,
                stats.dropped_messages,
                0, // protected_bytes not relevant for history-only log
            );
            cache_metrics::emit_history_metrics(&snap);
        }
    }

    /// Force-finish any lingering in-progress tool calls.
    /// Returns the number of tool calls that were transitioned.
    pub fn finalize_in_progress_tool_calls(&mut self, new_status: model::ToolCallStatus) -> usize {
        let mut changed = 0usize;
        let mut cleared_interaction = false;
        let mut changed_message_indices = Vec::new();
        let mut changed_slots = Vec::new();
        let mut detached_terminal = false;

        for (msg_idx, msg) in self.messages.iter_mut().enumerate() {
            for (block_idx, block) in msg.blocks.iter_mut().enumerate() {
                if let MessageBlock::ToolCall(tc) = block {
                    let tc = tc.as_mut();
                    if matches!(
                        tc.status,
                        model::ToolCallStatus::InProgress | model::ToolCallStatus::Pending
                    ) {
                        tc.status = new_status;
                        tc.mark_tool_call_layout_dirty();
                        changed_slots.push((msg_idx, block_idx));
                        if tc.pending_permission.take().is_some() {
                            cleared_interaction = true;
                        }
                        if tc.pending_question.take().is_some() {
                            cleared_interaction = true;
                        }
                        if tc.is_execute_tool() && tc.terminal_id.take().is_some() {
                            detached_terminal = true;
                        }
                        if changed_message_indices.last().copied() != Some(msg_idx) {
                            changed_message_indices.push(msg_idx);
                        }
                        changed += 1;
                    }
                }
            }
        }

        if detached_terminal {
            self.rebuild_tool_indices_and_terminal_refs();
        }

        for (msg_idx, block_idx) in changed_slots {
            self.sync_render_cache_slot(msg_idx, block_idx);
        }

        for msg_idx in changed_message_indices.iter().copied() {
            self.recompute_message_retained_bytes(msg_idx);
        }

        if changed > 0 || cleared_interaction {
            self.invalidate_message_set(changed_message_indices.iter().copied());
            self.pending_interaction_ids.clear();
            self.release_focus_target(FocusTarget::Permission);
        }

        changed
    }

    /// Clear any inline permission/question UI still attached to tool calls.
    /// Returns the number of tool call blocks that changed.
    pub fn clear_inline_tool_interactions(&mut self) -> usize {
        let mut changed = 0usize;
        let mut changed_message_indices = Vec::new();
        let mut changed_slots = Vec::new();

        for (msg_idx, msg) in self.messages.iter_mut().enumerate() {
            for (block_idx, block) in msg.blocks.iter_mut().enumerate() {
                let MessageBlock::ToolCall(tc) = block else {
                    continue;
                };
                let tc = tc.as_mut();
                let mut block_changed = false;
                if tc.pending_permission.take().is_some() {
                    block_changed = true;
                }
                if tc.pending_question.take().is_some() {
                    block_changed = true;
                }
                if !block_changed {
                    continue;
                }
                tc.mark_tool_call_layout_dirty();
                changed_slots.push((msg_idx, block_idx));
                if changed_message_indices.last().copied() != Some(msg_idx) {
                    changed_message_indices.push(msg_idx);
                }
                changed += 1;
            }
        }

        for (msg_idx, block_idx) in changed_slots {
            self.sync_render_cache_slot(msg_idx, block_idx);
        }

        for msg_idx in changed_message_indices.iter().copied() {
            self.recompute_message_retained_bytes(msg_idx);
        }

        if changed > 0 {
            self.invalidate_message_set(changed_message_indices.iter().copied());
        }

        if changed > 0 || !self.pending_interaction_ids.is_empty() {
            self.pending_interaction_ids.clear();
            self.release_focus_target(FocusTarget::Permission);
        }

        changed
    }

    /// Clear runtime-only turn tracking while preserving the message history itself.
    pub fn finalize_turn_runtime_artifacts(&mut self, new_status: model::ToolCallStatus) {
        let _ = self.finalize_in_progress_tool_calls(new_status);
        let _ = self.clear_inline_tool_interactions();
        self.clear_tool_scope_tracking();
    }

    /// Build a minimal `App` for unit/integration tests.
    /// All fields get sensible defaults; the `mpsc` channel is wired up internally.
    #[doc(hidden)]
    #[must_use]
    #[allow(clippy::too_many_lines)]
    pub fn test_default() -> Self {
        let (tx, rx) = mpsc::unbounded_channel();
        Self {
            active_view: ActiveView::Chat,
            config: ConfigState::default(),
            trust: TrustState::default(),
            settings_home_override: None,
            messages: Vec::new(),
            message_retained_bytes: Vec::new(),
            retained_history_bytes: 0,
            viewport: ChatViewport::new(),
            input: InputState::new(),
            status: AppStatus::Ready,
            resuming_session_id: None,
            pending_command_label: None,
            pending_command_ack: None,
            should_quit: false,
            exit_error: None,
            session_id: None,
            conn: None,
            session_scope_epoch: 0,
            model_name: "test-model".into(),
            welcome_model_resolved: true,
            cwd: "/test".into(),
            cwd_raw: "/test".into(),
            files_accessed: 0,
            mode: None,
            config_options: BTreeMap::new(),
            login_hint: None,
            pending_compact_clear: false,
            help_view: HelpView::Keys,
            help_open: false,
            help_dialog: dialog::DialogState::default(),
            help_visible_count: 0,
            pending_interaction_ids: Vec::new(),
            cancelled_turn_pending_hint: false,
            pending_cancel_origin: None,
            pending_auto_submit_after_cancel: false,
            event_tx: tx,
            event_rx: rx,
            spinner_frame: 0,
            spinner_last_advance_at: None,
            active_turn_assistant_message_idx: None,
            tools_collapsed: false,
            active_task_ids: HashSet::default(),
            tool_call_scopes: HashMap::default(),
            active_subagent_tool_ids: HashSet::default(),
            subagent_idle_since: None,
            terminals: std::rc::Rc::default(),
            force_redraw: false,
            tool_call_index: HashMap::default(),
            todos: Vec::new(),
            show_todo_panel: false,
            todo_scroll: 0,
            todo_selected: 0,
            focus: FocusManager::default(),
            available_commands: Vec::new(),
            plugins: PluginsState::default(),
            available_agents: Vec::new(),
            available_models: Vec::new(),
            recent_sessions: Vec::new(),
            cached_frame_area: ratatui::layout::Rect::default(),
            selection: None,
            scrollbar_drag: None,
            rendered_chat_lines: Vec::new(),
            rendered_chat_area: ratatui::layout::Rect::default(),
            rendered_input_lines: Vec::new(),
            rendered_input_area: ratatui::layout::Rect::default(),
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
            session_usage: SessionUsageState::default(),
            usage: UsageState::default(),
            mcp: McpState::default(),
            fast_mode_state: model::FastModeState::Off,
            last_rate_limit_update: None,
            is_compacting: false,
            account_info: None,
            terminal_tool_calls: Vec::new(),
            terminal_tool_call_membership: HashSet::new(),
            needs_redraw: true,
            notifications: super::notify::NotificationManager::new(),
            perf: None,
            render_cache_budget: RenderCacheBudget::default(),
            render_cache_slots: Vec::new(),
            render_cache_total_bytes: 0,
            render_cache_protected_bytes: 0,
            render_cache_evictable: BTreeSet::new(),
            render_cache_tail_msg_idx: None,
            history_retention: HistoryRetentionPolicy::default(),
            history_retention_stats: HistoryRetentionStats::default(),
            cache_metrics: CacheMetrics::default(),
            fps_ema: None,
            last_frame_at: None,
            startup_connection_requested: false,
            connection_started: false,
            startup_bridge_script: None,
            startup_resume_id: None,
            startup_resume_requested: false,
        }
    }

    /// Detect the current git branch.
    pub fn refresh_git_branch(&mut self) {
        let new_branch = std::process::Command::new("git")
            .args(["branch", "--show-current"])
            .current_dir(&self.cwd_raw)
            .output()
            .ok()
            .and_then(|o| {
                if o.status.success() {
                    let s = String::from_utf8_lossy(&o.stdout).trim().to_owned();
                    if s.is_empty() { None } else { Some(s) }
                } else {
                    None
                }
            });
        if new_branch != self.git_branch {
            self.git_branch = new_branch;
        }
    }

    /// Resolve the effective focus owner for Up/Down and other directional keys.
    #[must_use]
    pub fn focus_owner(&self) -> FocusOwner {
        self.focus.owner(self.focus_context())
    }

    #[must_use]
    pub fn active_turn_assistant_idx(&self) -> Option<usize> {
        self.active_turn_assistant_message_idx.filter(|&idx| {
            self.messages.get(idx).is_some_and(|msg| matches!(msg.role, MessageRole::Assistant))
        })
    }

    pub fn bind_active_turn_assistant(&mut self, idx: usize) {
        self.active_turn_assistant_message_idx = self
            .messages
            .get(idx)
            .is_some_and(|msg| matches!(msg.role, MessageRole::Assistant))
            .then_some(idx);
    }

    pub fn bind_active_turn_assistant_to_tail(&mut self) {
        if let Some(idx) = self.messages.len().checked_sub(1) {
            self.bind_active_turn_assistant(idx);
        } else {
            self.clear_active_turn_assistant();
        }
    }

    pub fn clear_active_turn_assistant(&mut self) {
        self.active_turn_assistant_message_idx = None;
    }

    pub fn bump_session_scope_epoch(&mut self) {
        self.session_scope_epoch = self.session_scope_epoch.saturating_add(1);
    }

    pub fn clear_session_runtime_identity(&mut self) {
        self.session_id = None;
        "Connecting...".clone_into(&mut self.model_name);
        self.mode = None;
        self.fast_mode_state = model::FastModeState::Off;
        self.welcome_model_resolved = false;
    }

    pub fn reconcile_trust_state_from_preferences_and_cwd(&mut self) {
        let lookup = crate::app::trust::store::read_status(
            &self.config.committed_preferences_document,
            Path::new(&self.cwd_raw),
        );
        self.trust.project_key = lookup.project_key;
        self.trust.status = if lookup.trusted {
            crate::app::trust::TrustStatus::Trusted
        } else {
            crate::app::trust::TrustStatus::Untrusted
        };
        self.trust.selection = crate::app::trust::TrustSelection::Yes;
        self.trust.last_error = self
            .config
            .preferences_path
            .is_none()
            .then(|| "Trust preferences path is not available".to_owned());
    }

    pub fn reconcile_runtime_from_persisted_settings_change(&mut self) {
        self.welcome_model_resolved = false;
        self.reconcile_trust_state_from_preferences_and_cwd();
        self.update_welcome_model_once();
    }

    pub(crate) fn shift_active_turn_assistant_for_insert(&mut self, idx: usize) {
        if let Some(owner_idx) = self.active_turn_assistant_message_idx
            && idx <= owner_idx
        {
            self.active_turn_assistant_message_idx = Some(owner_idx.saturating_add(1));
        }
    }

    pub(crate) fn shift_active_turn_assistant_for_remove(&mut self, idx: usize) {
        let Some(owner_idx) = self.active_turn_assistant_message_idx else {
            return;
        };
        self.active_turn_assistant_message_idx = match idx.cmp(&owner_idx) {
            std::cmp::Ordering::Less => Some(owner_idx.saturating_sub(1)),
            std::cmp::Ordering::Equal => None,
            std::cmp::Ordering::Greater => Some(owner_idx),
        };
    }

    #[must_use]
    pub fn active_autocomplete_kind(&self) -> Option<AutocompleteKind> {
        if self.mention.is_some() {
            Some(AutocompleteKind::Mention)
        } else if self.slash.is_some() {
            Some(AutocompleteKind::Slash)
        } else if self.subagent.is_some() {
            Some(AutocompleteKind::Subagent)
        } else {
            None
        }
    }

    #[must_use]
    pub fn is_help_active(&self) -> bool {
        self.help_open
    }

    #[must_use]
    pub fn autocomplete_focus_available(&self) -> bool {
        self.mention.as_ref().is_some_and(mention::MentionState::has_selectable_candidates)
            || self.slash.is_some()
            || self.subagent.is_some()
    }

    pub fn rebuild_chat_focus_from_state(&mut self) {
        if self.active_view != ActiveView::Chat {
            return;
        }

        self.normalize_focus_stack();

        if self.pending_interaction_ids.is_empty() {
            self.release_focus_target(FocusTarget::Permission);
        } else {
            self.claim_focus_target(FocusTarget::Permission);
        }

        if self.autocomplete_focus_available() {
            self.claim_focus_target(FocusTarget::Mention);
        } else {
            self.release_focus_target(FocusTarget::Mention);
        }

        if self.is_help_active()
            && self.pending_interaction_ids.is_empty()
            && !self.autocomplete_focus_available()
        {
            self.claim_focus_target(FocusTarget::Help);
        } else {
            self.release_focus_target(FocusTarget::Help);
        }

        self.normalize_focus_stack();
    }

    /// Claim key routing for a navigation target.
    /// The latest claimant wins.
    pub fn claim_focus_target(&mut self, target: FocusTarget) {
        let context = self.focus_context();
        self.focus.claim(target, context);
    }

    /// Release key routing claim for a navigation target.
    pub fn release_focus_target(&mut self, target: FocusTarget) {
        let context = self.focus_context();
        self.focus.release(target, context);
    }

    /// Drop claims that are no longer valid for current state.
    pub fn normalize_focus_stack(&mut self) {
        let context = self.focus_context();
        self.focus.normalize(context);
    }

    #[must_use]
    fn focus_context(&self) -> FocusContext {
        FocusContext::new(
            self.show_todo_panel && !self.todos.is_empty(),
            self.autocomplete_focus_available(),
            !self.pending_interaction_ids.is_empty(),
        )
        .with_help(self.is_help_active())
    }
}

fn debug_paste_text(text: &str) -> String {
    const MAX_CHARS: usize = 60;
    let mut out = String::new();
    let mut iter = text.chars();
    for _ in 0..MAX_CHARS {
        let Some(ch) = iter.next() else {
            return out;
        };
        out.extend(ch.escape_default());
    }
    if iter.next().is_some() {
        out.push_str("...");
    }
    out
}

#[cfg(test)]
mod tests {
    // =====
    // TESTS: 26
    // =====

    use super::*;
    use crate::app::dialog;
    use crate::app::slash::{SlashCandidate, SlashContext, SlashState};
    use pretty_assertions::assert_eq;
    use ratatui::style::{Color, Style};
    use ratatui::text::{Line, Span};

    // BlockCache

    #[test]
    fn cache_default_returns_none() {
        let cache = BlockCache::default();
        assert!(cache.get().is_none());
    }

    #[test]
    fn cache_store_then_get() {
        let mut cache = BlockCache::default();
        cache.store(vec![Line::from("hello")]);
        assert!(cache.get().is_some());
        assert_eq!(cache.get().unwrap().len(), 1);
    }

    #[test]
    fn cache_invalidate_then_get_returns_none() {
        let mut cache = BlockCache::default();
        cache.store(vec![Line::from("data")]);
        cache.invalidate();
        assert!(cache.get().is_none());
    }

    // BlockCache

    #[test]
    fn cache_store_after_invalidate() {
        let mut cache = BlockCache::default();
        cache.store(vec![Line::from("old")]);
        cache.invalidate();
        assert!(cache.get().is_none());
        cache.store(vec![Line::from("new")]);
        let lines = cache.get().unwrap();
        assert_eq!(lines.len(), 1);
        let span_content: String = lines[0].spans.iter().map(|s| s.content.as_ref()).collect();
        assert_eq!(span_content, "new");
    }

    #[test]
    fn cache_multiple_invalidations() {
        let mut cache = BlockCache::default();
        cache.store(vec![Line::from("data")]);
        cache.invalidate();
        cache.invalidate();
        cache.invalidate();
        assert!(cache.get().is_none());
        cache.store(vec![Line::from("fresh")]);
        assert!(cache.get().is_some());
    }

    #[test]
    fn cache_store_empty_lines() {
        let mut cache = BlockCache::default();
        cache.store(Vec::new());
        let lines = cache.get().unwrap();
        assert!(lines.is_empty());
    }

    /// Store twice without invalidating - second store overwrites first.
    #[test]
    fn cache_store_overwrite_without_invalidate() {
        let mut cache = BlockCache::default();
        cache.store(vec![Line::from("first")]);
        cache.store(vec![Line::from("second"), Line::from("line2")]);
        let lines = cache.get().unwrap();
        assert_eq!(lines.len(), 2);
        let content: String = lines[0].spans.iter().map(|s| s.content.as_ref()).collect();
        assert_eq!(content, "second");
    }

    /// `get()` called twice returns consistent data.
    #[test]
    fn cache_get_twice_consistent() {
        let mut cache = BlockCache::default();
        cache.store(vec![Line::from("stable")]);
        let first = cache.get().unwrap().len();
        let second = cache.get().unwrap().len();
        assert_eq!(first, second);
    }

    // BlockCache

    #[test]
    fn cache_store_many_lines() {
        let mut cache = BlockCache::default();
        let lines: Vec<Line<'static>> =
            (0..1000).map(|i| Line::from(Span::raw(format!("line {i}")))).collect();
        cache.store(lines);
        assert_eq!(cache.get().unwrap().len(), 1000);
    }

    #[test]
    fn cache_store_splits_into_kb_segments() {
        let mut cache = BlockCache::default();
        let long = "x".repeat(800);
        let lines: Vec<Line<'static>> = (0..12).map(|_| Line::from(long.clone())).collect();
        cache.store(lines);
        assert!(cache.segment_count() > 1);
        assert!(cache.cached_bytes() > 0);
    }

    #[test]
    fn cache_invalidate_without_store() {
        let mut cache = BlockCache::default();
        cache.invalidate();
        assert!(cache.get().is_none());
    }

    #[test]
    fn cache_rapid_store_invalidate_cycle() {
        let mut cache = BlockCache::default();
        for i in 0..50 {
            cache.store(vec![Line::from(format!("v{i}"))]);
            assert!(cache.get().is_some());
            cache.invalidate();
            assert!(cache.get().is_none());
        }
        cache.store(vec![Line::from("final")]);
        assert!(cache.get().is_some());
    }

    /// Store styled lines with multiple spans per line.
    #[test]
    fn cache_store_styled_lines() {
        let mut cache = BlockCache::default();
        let line = Line::from(vec![
            Span::styled("bold", Style::default().fg(Color::Red)),
            Span::raw(" normal "),
            Span::styled("blue", Style::default().fg(Color::Blue)),
        ]);
        cache.store(vec![line]);
        let lines = cache.get().unwrap();
        assert_eq!(lines[0].spans.len(), 3);
    }

    /// Version counter after many invalidations - verify it doesn't
    /// accidentally wrap to 0 (which would make stale data appear fresh).
    /// With u64, 10K invalidations is nowhere near overflow.
    #[test]
    fn cache_version_no_false_fresh_after_many_invalidations() {
        let mut cache = BlockCache::default();
        cache.store(vec![Line::from("data")]);
        for _ in 0..10_000 {
            cache.invalidate();
        }
        // Cache was invalidated 10K times without re-storing - must be stale
        assert!(cache.get().is_none());
    }

    /// Invalidate, store, invalidate, store - alternating pattern.
    #[test]
    fn cache_alternating_invalidate_store() {
        let mut cache = BlockCache::default();
        for i in 0..100 {
            cache.invalidate();
            assert!(cache.get().is_none(), "stale after invalidate at iter {i}");
            cache.store(vec![Line::from(format!("v{i}"))]);
            assert!(cache.get().is_some(), "fresh after store at iter {i}");
        }
    }

    // BlockCache height

    #[test]
    fn cache_height_default_returns_none() {
        let cache = BlockCache::default();
        assert!(cache.height_at(80).is_none());
    }

    #[test]
    fn cache_store_with_height_then_height_at() {
        let mut cache = BlockCache::default();
        cache.store_with_height(vec![Line::from("hello")], 1, 80);
        assert_eq!(cache.height_at(80), Some(1));
        assert!(cache.get().is_some());
    }

    #[test]
    fn cache_height_at_wrong_width_returns_none() {
        let mut cache = BlockCache::default();
        cache.store_with_height(vec![Line::from("hello")], 1, 80);
        assert!(cache.height_at(120).is_none());
    }

    #[test]
    fn cache_height_invalidated_returns_none() {
        let mut cache = BlockCache::default();
        cache.store_with_height(vec![Line::from("hello")], 1, 80);
        cache.invalidate();
        assert!(cache.height_at(80).is_none());
    }

    #[test]
    fn cache_store_without_height_has_no_height() {
        let mut cache = BlockCache::default();
        cache.store(vec![Line::from("hello")]);
        // store() without height leaves wrapped_width at 0
        assert!(cache.height_at(80).is_none());
    }

    #[test]
    fn cache_store_with_height_overwrite() {
        let mut cache = BlockCache::default();
        cache.store_with_height(vec![Line::from("old")], 1, 80);
        cache.invalidate();
        cache.store_with_height(vec![Line::from("new long line")], 3, 120);
        assert_eq!(cache.height_at(120), Some(3));
        assert!(cache.height_at(80).is_none());
    }

    // BlockCache set_height (separate from store)

    #[test]
    fn cache_set_height_after_store() {
        let mut cache = BlockCache::default();
        cache.store(vec![Line::from("hello")]);
        assert!(cache.height_at(80).is_none()); // no height yet
        cache.set_height(1, 80);
        assert_eq!(cache.height_at(80), Some(1));
        assert!(cache.get().is_some()); // lines still valid
    }

    #[test]
    fn cache_set_height_update_width() {
        let mut cache = BlockCache::default();
        cache.store(vec![Line::from("hello world")]);
        cache.set_height(1, 80);
        assert_eq!(cache.height_at(80), Some(1));
        // Re-measure at new width
        cache.set_height(2, 40);
        assert_eq!(cache.height_at(40), Some(2));
        assert!(cache.height_at(80).is_none()); // old width no longer valid
    }

    #[test]
    fn cache_set_height_invalidate_clears_height() {
        let mut cache = BlockCache::default();
        cache.store(vec![Line::from("data")]);
        cache.set_height(3, 80);
        cache.invalidate();
        assert!(cache.height_at(80).is_none()); // version mismatch
    }

    #[test]
    fn cache_set_height_on_invalidated_cache_returns_none() {
        let mut cache = BlockCache::default();
        cache.store(vec![Line::from("data")]);
        cache.invalidate(); // version != 0
        cache.set_height(5, 80);
        // height_at returns None because cache is stale (version != 0)
        assert!(cache.height_at(80).is_none());
    }

    #[test]
    fn cache_store_then_set_height_matches_store_with_height() {
        let mut cache_a = BlockCache::default();
        cache_a.store(vec![Line::from("test")]);
        cache_a.set_height(2, 100);

        let mut cache_b = BlockCache::default();
        cache_b.store_with_height(vec![Line::from("test")], 2, 100);

        assert_eq!(cache_a.height_at(100), cache_b.height_at(100));
        assert_eq!(cache_a.get().unwrap().len(), cache_b.get().unwrap().len());
    }

    #[test]
    fn cache_measure_and_set_height_from_segments() {
        let mut cache = BlockCache::default();
        let lines = vec![
            Line::from("alpha beta gamma delta epsilon"),
            Line::from("zeta eta theta iota kappa lambda"),
            Line::from("mu nu xi omicron pi rho sigma"),
        ];
        cache.store(lines.clone());
        let measured = cache.measure_and_set_height(16).expect("expected measured height");
        let expected = ratatui::widgets::Paragraph::new(ratatui::text::Text::from(lines))
            .wrap(ratatui::widgets::Wrap { trim: false })
            .line_count(16);
        assert_eq!(measured, expected);
        assert_eq!(cache.height_at(16), Some(expected));
    }

    #[test]
    fn cache_get_updates_last_access_tick() {
        let mut cache = BlockCache::default();
        cache.store(vec![Line::from("tick")]);
        let before = cache.last_access_tick();
        let _ = cache.get();
        let after = cache.last_access_tick();
        assert!(after > before);
    }

    // App tool_call_index

    fn make_test_app() -> App {
        App::test_default()
    }

    fn assistant_text_block(text: &str) -> MessageBlock {
        MessageBlock::Text(TextBlock::from_complete(text))
    }

    fn user_text_message(text: &str) -> ChatMessage {
        ChatMessage {
            role: MessageRole::User,
            blocks: vec![assistant_text_block(text)],
            usage: None,
        }
    }

    fn assistant_tool_message(id: &str, status: model::ToolCallStatus) -> ChatMessage {
        ChatMessage {
            role: MessageRole::Assistant,
            blocks: vec![MessageBlock::ToolCall(Box::new(ToolCallInfo {
                id: id.to_owned(),
                title: format!("tool {id}"),
                sdk_tool_name: "Read".to_owned(),
                raw_input: None,
                raw_input_bytes: 0,
                output_metadata: None,
                status,
                content: Vec::new(),
                hidden: false,
                terminal_id: None,
                terminal_command: None,
                terminal_output: Some("x".repeat(1024)),
                terminal_output_len: 1024,
                terminal_bytes_seen: 1024,
                terminal_snapshot_mode: TerminalSnapshotMode::AppendOnly,
                render_epoch: 0,
                layout_epoch: 0,
                last_measured_width: 0,
                last_measured_height: 0,
                last_measured_layout_epoch: 0,
                last_measured_layout_generation: 0,
                cache: BlockCache::default(),
                pending_permission: None,
                pending_question: None,
            }))],
            usage: None,
        }
    }

    fn assistant_bash_tool_message(
        id: &str,
        status: model::ToolCallStatus,
        terminal_id: &str,
    ) -> ChatMessage {
        ChatMessage {
            role: MessageRole::Assistant,
            blocks: vec![MessageBlock::ToolCall(Box::new(ToolCallInfo {
                id: id.to_owned(),
                title: format!("tool {id}"),
                sdk_tool_name: "Bash".to_owned(),
                raw_input: None,
                raw_input_bytes: 0,
                output_metadata: None,
                status,
                content: Vec::new(),
                hidden: false,
                terminal_id: Some(terminal_id.to_owned()),
                terminal_command: Some("echo hi".to_owned()),
                terminal_output: Some("x".repeat(1024)),
                terminal_output_len: 1024,
                terminal_bytes_seen: 1024,
                terminal_snapshot_mode: TerminalSnapshotMode::AppendOnly,
                render_epoch: 0,
                layout_epoch: 0,
                last_measured_width: 0,
                last_measured_height: 0,
                last_measured_layout_epoch: 0,
                last_measured_layout_generation: 0,
                cache: BlockCache::default(),
                pending_permission: None,
                pending_question: None,
            }))],
            usage: None,
        }
    }

    fn assistant_tool_message_with_pending_permission(id: &str) -> ChatMessage {
        let (tx, _rx) = tokio::sync::oneshot::channel();
        ChatMessage {
            role: MessageRole::Assistant,
            blocks: vec![MessageBlock::ToolCall(Box::new(ToolCallInfo {
                id: id.to_owned(),
                title: format!("tool {id}"),
                sdk_tool_name: "Read".to_owned(),
                raw_input: None,
                raw_input_bytes: 0,
                output_metadata: None,
                status: model::ToolCallStatus::Completed,
                content: Vec::new(),
                hidden: false,
                terminal_id: None,
                terminal_command: None,
                terminal_output: Some("x".repeat(1024)),
                terminal_output_len: 1024,
                terminal_bytes_seen: 1024,
                terminal_snapshot_mode: TerminalSnapshotMode::AppendOnly,
                render_epoch: 0,
                layout_epoch: 0,
                last_measured_width: 0,
                last_measured_height: 0,
                last_measured_layout_epoch: 0,
                last_measured_layout_generation: 0,
                cache: BlockCache::default(),
                pending_permission: Some(InlinePermission {
                    options: vec![model::PermissionOption::new(
                        "allow-once",
                        "Allow once",
                        model::PermissionOptionKind::AllowOnce,
                    )],
                    response_tx: tx,
                    selected_index: 0,
                    focused: false,
                }),
                pending_question: None,
            }))],
            usage: None,
        }
    }

    #[test]
    fn enforce_render_cache_budget_evicts_lru_block() {
        let mut app = make_test_app();
        app.messages = vec![
            ChatMessage {
                role: MessageRole::Assistant,
                blocks: vec![assistant_text_block("a")],
                usage: None,
            },
            ChatMessage {
                role: MessageRole::Assistant,
                blocks: vec![assistant_text_block("b")],
                usage: None,
            },
        ];

        let bytes_a = if let MessageBlock::Text(block) = &mut app.messages[0].blocks[0] {
            block.cache.store(vec![Line::from("x".repeat(2200))]);
            block.cache.cached_bytes()
        } else {
            0
        };
        let bytes_b = if let MessageBlock::Text(block) = &mut app.messages[1].blocks[0] {
            block.cache.store(vec![Line::from("y".repeat(2200))]);
            let _ = block.cache.get();
            block.cache.cached_bytes()
        } else {
            0
        };

        app.render_cache_budget.max_bytes = bytes_b;
        let stats = app.enforce_render_cache_budget();
        assert!(stats.evicted_blocks >= 1);
        assert!(stats.evicted_bytes >= bytes_a);
        assert!(stats.total_after_bytes <= app.render_cache_budget.max_bytes);
        assert_eq!(stats.protected_bytes, 0);

        if let MessageBlock::Text(block) = &app.messages[0].blocks[0] {
            assert_eq!(block.cache.cached_bytes(), 0);
        } else {
            panic!("expected text block");
        }
        if let MessageBlock::Text(block) = &app.messages[1].blocks[0] {
            assert_eq!(block.cache.cached_bytes(), bytes_b);
        } else {
            panic!("expected text block");
        }
    }

    #[test]
    fn enforce_render_cache_budget_protects_streaming_tail_message() {
        let mut app = make_test_app();
        app.status = AppStatus::Thinking;
        app.messages = vec![ChatMessage {
            role: MessageRole::Assistant,
            blocks: vec![assistant_text_block("streaming tail")],
            usage: None,
        }];

        let before = if let MessageBlock::Text(block) = &mut app.messages[0].blocks[0] {
            block.cache.store(vec![Line::from("z".repeat(4096))]);
            block.cache.cached_bytes()
        } else {
            0
        };
        app.render_cache_budget.max_bytes = 64;
        let stats = app.enforce_render_cache_budget();
        assert_eq!(stats.evicted_blocks, 0);
        assert_eq!(stats.evicted_bytes, 0);
        assert_eq!(stats.protected_bytes, before);

        if let MessageBlock::Text(block) = &app.messages[0].blocks[0] {
            assert_eq!(block.cache.cached_bytes(), before);
        } else {
            panic!("expected text block");
        }
    }

    #[test]
    fn enforce_render_cache_budget_excludes_protected_from_budget() {
        let mut app = make_test_app();
        app.status = AppStatus::Running;
        app.messages = vec![
            ChatMessage {
                role: MessageRole::Assistant,
                blocks: vec![assistant_text_block("old message")],
                usage: None,
            },
            ChatMessage {
                role: MessageRole::Assistant,
                blocks: vec![assistant_text_block("streaming tail")],
                usage: None,
            },
        ];

        let bytes_a = if let MessageBlock::Text(block) = &mut app.messages[0].blocks[0] {
            block.cache.store(vec![Line::from("x".repeat(2200))]);
            block.cache.cached_bytes()
        } else {
            0
        };
        let bytes_b = if let MessageBlock::Text(block) = &mut app.messages[1].blocks[0] {
            block.cache.store(vec![Line::from("y".repeat(5000))]);
            block.cache.cached_bytes()
        } else {
            0
        };

        // Budget fits old message alone but not old + tail combined.
        app.render_cache_budget.max_bytes = bytes_a + 100;
        assert!(bytes_a + bytes_b > app.render_cache_budget.max_bytes);

        let stats = app.enforce_render_cache_budget();

        // Protected bytes should be the streaming tail.
        assert_eq!(stats.protected_bytes, bytes_b);
        // No eviction: budgeted bytes (bytes_a) are under max_bytes.
        assert_eq!(stats.evicted_blocks, 0);
        assert_eq!(stats.evicted_bytes, 0);
        // Old message cache intact.
        if let MessageBlock::Text(block) = &app.messages[0].blocks[0] {
            assert_eq!(block.cache.cached_bytes(), bytes_a);
        } else {
            panic!("expected text block");
        }
    }

    #[test]
    fn enforce_render_cache_budget_protects_active_streaming_owner_not_physical_tail() {
        let mut app = make_test_app();
        app.status = AppStatus::Running;
        app.messages = vec![
            ChatMessage {
                role: MessageRole::Assistant,
                blocks: vec![assistant_text_block("old message")],
                usage: None,
            },
            ChatMessage {
                role: MessageRole::Assistant,
                blocks: vec![assistant_text_block("active streaming owner")],
                usage: None,
            },
            ChatMessage {
                role: MessageRole::System(Some(SystemSeverity::Info)),
                blocks: vec![assistant_text_block("late trailing system row")],
                usage: None,
            },
        ];
        app.bind_active_turn_assistant(1);

        if let MessageBlock::Text(block) = &mut app.messages[0].blocks[0] {
            block.cache.store(vec![Line::from("x".repeat(2000))]);
        }
        let protected_bytes = if let MessageBlock::Text(block) = &mut app.messages[1].blocks[0] {
            block.cache.store(vec![Line::from("y".repeat(4000))]);
            block.cache.cached_bytes()
        } else {
            0
        };
        if let MessageBlock::Text(block) = &mut app.messages[2].blocks[0] {
            block.cache.store(vec![Line::from("z".repeat(5000))]);
        }

        app.render_cache_budget.max_bytes = 64;
        let stats = app.enforce_render_cache_budget();

        assert_eq!(stats.protected_bytes, protected_bytes);
    }

    #[test]
    fn enforce_render_cache_budget_evicts_when_budgeted_over_limit() {
        let mut app = make_test_app();
        app.status = AppStatus::Running;
        app.messages = vec![
            ChatMessage {
                role: MessageRole::Assistant,
                blocks: vec![assistant_text_block("old-a")],
                usage: None,
            },
            ChatMessage {
                role: MessageRole::Assistant,
                blocks: vec![assistant_text_block("old-b")],
                usage: None,
            },
            ChatMessage {
                role: MessageRole::Assistant,
                blocks: vec![assistant_text_block("streaming")],
                usage: None,
            },
        ];

        // Populate caches: messages 0 and 1 evictable, message 2 protected.
        if let MessageBlock::Text(block) = &mut app.messages[0].blocks[0] {
            block.cache.store(vec![Line::from("x".repeat(3000))]);
        }
        let bytes_b = if let MessageBlock::Text(block) = &mut app.messages[1].blocks[0] {
            block.cache.store(vec![Line::from("y".repeat(3000))]);
            let _ = block.cache.get(); // touch to make more recently accessed
            block.cache.cached_bytes()
        } else {
            0
        };
        let bytes_c = if let MessageBlock::Text(block) = &mut app.messages[2].blocks[0] {
            block.cache.store(vec![Line::from("z".repeat(5000))]);
            block.cache.cached_bytes()
        } else {
            0
        };

        // Budget fits message B but not A+B (excludes C as protected).
        app.render_cache_budget.max_bytes = bytes_b + 100;

        let stats = app.enforce_render_cache_budget();

        assert_eq!(stats.protected_bytes, bytes_c);
        assert!(stats.evicted_blocks >= 1); // message A evicted (older access)
        // Message B should survive (more recent access).
        if let MessageBlock::Text(block) = &app.messages[1].blocks[0] {
            assert_eq!(block.cache.cached_bytes(), bytes_b);
        } else {
            panic!("expected text block");
        }
    }

    #[test]
    fn enforce_render_cache_budget_protected_bytes_zero_when_not_streaming() {
        let mut app = make_test_app();
        app.status = AppStatus::Ready;
        app.messages = vec![ChatMessage {
            role: MessageRole::Assistant,
            blocks: vec![assistant_text_block("done")],
            usage: None,
        }];

        if let MessageBlock::Text(block) = &mut app.messages[0].blocks[0] {
            block.cache.store(vec![Line::from("x".repeat(2000))]);
        }
        app.render_cache_budget.max_bytes = usize::MAX;

        let stats = app.enforce_render_cache_budget();
        assert_eq!(stats.protected_bytes, 0);
    }

    #[test]
    fn enforce_history_retention_noop_under_budget() {
        let mut app = make_test_app();
        app.messages = vec![
            ChatMessage::welcome("model", "/cwd"),
            user_text_message("small message"),
            user_text_message("another message"),
        ];
        app.history_retention.max_bytes = usize::MAX / 4;

        let stats = app.enforce_history_retention();
        assert_eq!(stats.dropped_messages, 0);
        assert_eq!(stats.total_dropped_messages, 0);
        assert!(!app.messages.iter().any(App::is_history_hidden_marker_message));
    }

    #[test]
    fn enforce_history_retention_drops_oldest_and_adds_marker() {
        let mut app = make_test_app();
        app.messages = vec![
            ChatMessage::welcome("model", "/cwd"),
            user_text_message("first old message"),
            user_text_message("second old message"),
            user_text_message("third old message"),
        ];
        app.history_retention.max_bytes = 1;

        let stats = app.enforce_history_retention();
        assert_eq!(stats.dropped_messages, 3);
        assert!(matches!(app.messages[0].role, MessageRole::Welcome));
        assert!(app.messages.iter().any(App::is_history_hidden_marker_message));
        assert_eq!(app.messages.len(), 2);
    }

    #[test]
    fn enforce_history_retention_preserves_in_progress_tool_message() {
        let mut app = make_test_app();
        app.messages = vec![
            ChatMessage::welcome("model", "/cwd"),
            user_text_message("droppable"),
            assistant_tool_message("tool-keep", model::ToolCallStatus::InProgress),
        ];
        app.history_retention.max_bytes = 1;

        let stats = app.enforce_history_retention();
        assert_eq!(stats.dropped_messages, 1);
        assert!(app.messages.iter().any(|msg| {
            msg.blocks.iter().any(|block| {
                matches!(
                    block,
                    MessageBlock::ToolCall(tc) if tc.id == "tool-keep"
                        && matches!(tc.status, model::ToolCallStatus::InProgress)
                )
            })
        }));
    }

    #[test]
    fn enforce_history_retention_preserves_pending_tool_message() {
        let mut app = make_test_app();
        app.messages = vec![
            ChatMessage::welcome("model", "/cwd"),
            user_text_message("droppable"),
            assistant_tool_message("tool-pending", model::ToolCallStatus::Pending),
        ];
        app.history_retention.max_bytes = 1;

        let stats = app.enforce_history_retention();
        assert_eq!(stats.dropped_messages, 1);
        assert!(app.messages.iter().any(|msg| {
            msg.blocks
                .iter()
                .any(|block| matches!(block, MessageBlock::ToolCall(tc) if tc.id == "tool-pending"))
        }));
    }

    #[test]
    fn enforce_history_retention_preserves_permission_tool_message() {
        let mut app = make_test_app();
        app.messages = vec![
            ChatMessage::welcome("model", "/cwd"),
            user_text_message("droppable"),
            assistant_tool_message_with_pending_permission("tool-perm"),
        ];
        app.history_retention.max_bytes = 1;

        let stats = app.enforce_history_retention();
        assert_eq!(stats.dropped_messages, 1);
        assert!(app.messages.iter().any(|msg| {
            msg.blocks
                .iter()
                .any(|block| matches!(block, MessageBlock::ToolCall(tc) if tc.id == "tool-perm"))
        }));
    }

    #[test]
    fn enforce_history_retention_rebuilds_tool_index_after_prune() {
        let mut app = make_test_app();
        app.messages = vec![
            ChatMessage::welcome("model", "/cwd"),
            user_text_message("drop this"),
            assistant_bash_tool_message("tool-idx", model::ToolCallStatus::InProgress, "term-1"),
        ];
        app.index_tool_call("tool-idx".to_owned(), 99, 99);
        app.sync_terminal_tool_call("stale-term".to_owned(), 99, 99);
        app.history_retention.max_bytes = 1;

        let _ = app.enforce_history_retention();
        assert_eq!(app.lookup_tool_call("tool-idx"), Some((2, 0)));
        assert_eq!(app.terminal_tool_calls.len(), 1);
        assert_eq!(app.terminal_tool_call_membership.len(), 1);
        assert_eq!(app.terminal_tool_calls[0].terminal_id, "term-1");
        assert_eq!(app.terminal_tool_calls[0].msg_idx, 2);
        assert_eq!(app.terminal_tool_calls[0].block_idx, 0);
    }

    #[test]
    fn enforce_history_retention_preserves_active_turn_assistant_message() {
        let mut app = make_test_app();
        app.status = AppStatus::Thinking;
        app.messages = vec![
            ChatMessage::welcome("model", "/cwd"),
            user_text_message("drop this"),
            ChatMessage { role: MessageRole::Assistant, blocks: Vec::new(), usage: None },
        ];
        app.bind_active_turn_assistant(2);
        app.history_retention.max_bytes = 1;

        let stats = app.enforce_history_retention();

        assert_eq!(stats.dropped_messages, 1);
        assert_eq!(app.active_turn_assistant_idx(), Some(2));
        assert!(matches!(app.messages[2].role, MessageRole::Assistant));
    }

    #[test]
    fn enforce_history_retention_remaps_active_turn_assistant_after_prune() {
        let mut app = make_test_app();
        app.status = AppStatus::Thinking;
        app.messages = vec![
            user_text_message("drop this"),
            ChatMessage {
                role: MessageRole::Assistant,
                blocks: vec![assistant_text_block("streaming reply")],
                usage: None,
            },
        ];
        app.bind_active_turn_assistant(1);
        app.history_retention.max_bytes = App::measure_message_bytes(&app.messages[1]);

        let stats = app.enforce_history_retention();

        assert_eq!(stats.dropped_messages, 1);
        assert_eq!(app.active_turn_assistant_idx(), Some(1));
        assert!(App::is_history_hidden_marker_message(&app.messages[0]));
        assert!(matches!(app.messages[1].role, MessageRole::Assistant));
    }

    #[test]
    fn enforce_history_retention_keeps_single_marker_on_repeat() {
        let mut app = make_test_app();
        app.messages = vec![ChatMessage::welcome("model", "/cwd"), user_text_message("drop me")];
        app.history_retention.max_bytes = 1;

        let first = app.enforce_history_retention();
        let second = app.enforce_history_retention();
        let marker_count =
            app.messages.iter().filter(|msg| App::is_history_hidden_marker_message(msg)).count();

        assert_eq!(first.dropped_messages, 1);
        assert_eq!(second.dropped_messages, 0);
        assert_eq!(marker_count, 1);
    }

    #[allow(clippy::cast_precision_loss)]
    #[test]
    fn enforce_history_retention_preserves_manual_scroll_anchor_across_drop_and_marker_insert() {
        let mut app = make_test_app();
        app.messages = vec![
            ChatMessage::welcome("model", "/cwd"),
            user_text_message("drop me first"),
            user_text_message("keep this anchored"),
            user_text_message("tail"),
        ];
        let _ = app.viewport.on_frame(40, 12);
        app.viewport.sync_message_count(app.messages.len());
        for idx in 0..app.messages.len() {
            app.viewport.set_message_height(idx, 4);
        }
        app.viewport.mark_heights_valid();
        app.viewport.rebuild_prefix_sums();

        app.viewport.auto_scroll = false;
        app.viewport.scroll_offset = 9;
        app.viewport.scroll_target = 9;
        app.viewport.scroll_pos = 9.0;
        app.history_retention.max_bytes = app
            .measure_history_bytes()
            .saturating_sub(App::measure_message_bytes(&app.messages[1]));

        let _ = app.enforce_history_retention();

        assert!(app.messages.iter().any(App::is_history_hidden_marker_message));
        assert_eq!(app.viewport.scroll_anchor_to_restore(), Some((2, 1)));
    }

    #[test]
    fn lookup_missing_returns_none() {
        let app = make_test_app();
        assert!(app.lookup_tool_call("nonexistent").is_none());
    }

    #[test]
    fn index_and_lookup() {
        let mut app = make_test_app();
        app.index_tool_call("tc-123".into(), 2, 5);
        assert_eq!(app.lookup_tool_call("tc-123"), Some((2, 5)));
    }

    // App tool_call_index

    /// Index same ID twice - second write overwrites first.
    #[test]
    fn index_overwrite_existing() {
        let mut app = make_test_app();
        app.index_tool_call("tc-1".into(), 0, 0);
        app.index_tool_call("tc-1".into(), 5, 10);
        assert_eq!(app.lookup_tool_call("tc-1"), Some((5, 10)));
    }

    /// Empty string as tool call ID.
    #[test]
    fn index_empty_string_id() {
        let mut app = make_test_app();
        app.index_tool_call(String::new(), 1, 2);
        assert_eq!(app.lookup_tool_call(""), Some((1, 2)));
    }

    /// Stress: 1000 tool calls indexed and looked up.
    #[test]
    fn index_stress_1000_entries() {
        let mut app = make_test_app();
        for i in 0..1000 {
            app.index_tool_call(format!("tc-{i}"), i, i * 2);
        }
        // Spot check first, middle, last
        assert_eq!(app.lookup_tool_call("tc-0"), Some((0, 0)));
        assert_eq!(app.lookup_tool_call("tc-500"), Some((500, 1000)));
        assert_eq!(app.lookup_tool_call("tc-999"), Some((999, 1998)));
        // Non-existent still returns None
        assert!(app.lookup_tool_call("tc-1000").is_none());
    }

    /// Unicode in tool call ID.
    #[test]
    fn index_unicode_id() {
        let mut app = make_test_app();
        app.index_tool_call("\u{1F600}-tool".into(), 3, 7);
        assert_eq!(app.lookup_tool_call("\u{1F600}-tool"), Some((3, 7)));
    }

    // active_task_ids

    #[test]
    fn active_task_insert_remove() {
        let mut app = make_test_app();
        app.insert_active_task("task-1".into());
        assert!(app.active_task_ids.contains("task-1"));
        app.remove_active_task("task-1");
        assert!(!app.active_task_ids.contains("task-1"));
    }

    #[test]
    fn remove_nonexistent_task_is_noop() {
        let mut app = make_test_app();
        app.remove_active_task("does-not-exist");
        assert!(app.active_task_ids.is_empty());
    }

    // active_task_ids

    /// Insert same ID twice - set deduplicates; one remove clears it.
    #[test]
    fn active_task_insert_duplicate() {
        let mut app = make_test_app();
        app.insert_active_task("task-1".into());
        app.insert_active_task("task-1".into());
        assert_eq!(app.active_task_ids.len(), 1);
        app.remove_active_task("task-1");
        assert!(app.active_task_ids.is_empty());
    }

    /// Insert many tasks, remove in different order.
    #[test]
    fn active_task_insert_many_remove_out_of_order() {
        let mut app = make_test_app();
        for i in 0..100 {
            app.insert_active_task(format!("task-{i}"));
        }
        assert_eq!(app.active_task_ids.len(), 100);
        // Remove in reverse order
        for i in (0..100).rev() {
            app.remove_active_task(&format!("task-{i}"));
        }
        assert!(app.active_task_ids.is_empty());
    }

    /// Mixed insert/remove interleaving.
    #[test]
    fn active_task_interleaved_insert_remove() {
        let mut app = make_test_app();
        app.insert_active_task("a".into());
        app.insert_active_task("b".into());
        app.remove_active_task("a");
        app.insert_active_task("c".into());
        assert!(!app.active_task_ids.contains("a"));
        assert!(app.active_task_ids.contains("b"));
        assert!(app.active_task_ids.contains("c"));
        assert_eq!(app.active_task_ids.len(), 2);
    }

    /// Remove from empty set multiple times - no panic.
    #[test]
    fn active_task_remove_from_empty_repeatedly() {
        let mut app = make_test_app();
        for i in 0..100 {
            app.remove_active_task(&format!("ghost-{i}"));
        }
        assert!(app.active_task_ids.is_empty());
    }

    /// `clear_tool_scope_tracking` must also clear `active_task_ids`.
    /// Regression test: before the fix, a leaked task ID from a cancelled turn
    /// caused main-agent tools on the next turn to be misclassified as Subagent scope.
    #[test]
    fn clear_tool_scope_tracking_also_clears_active_task_ids() {
        let mut app = make_test_app();
        app.insert_active_task("task-leaked".into());
        assert!(!app.active_task_ids.is_empty());
        app.clear_tool_scope_tracking();
        assert!(app.active_task_ids.is_empty(), "active_task_ids must be cleared at turn end");
        assert!(app.active_subagent_tool_ids.is_empty());
        assert!(app.subagent_idle_since.is_none());
    }

    #[test]
    fn finalize_in_progress_tool_calls_detaches_execute_terminal_refs() {
        let mut app = make_test_app();
        app.messages.push(assistant_bash_tool_message(
            "bash-1",
            model::ToolCallStatus::InProgress,
            "term-1",
        ));
        app.index_tool_call("bash-1".to_owned(), 0, 0);
        app.sync_terminal_tool_call("term-1".to_owned(), 0, 0);

        let changed = app.finalize_in_progress_tool_calls(model::ToolCallStatus::Completed);

        assert_eq!(changed, 1);
        assert!(app.terminal_tool_calls.is_empty());
        assert!(app.terminal_tool_call_membership.is_empty());
        let MessageBlock::ToolCall(tc) = &app.messages[0].blocks[0] else {
            panic!("expected tool call");
        };
        assert_eq!(tc.status, model::ToolCallStatus::Completed);
        assert_eq!(tc.terminal_id, None);
    }

    #[test]
    fn insert_message_tracked_nontail_rebuilds_tool_indices_and_invalidates_suffix() {
        let mut app = make_test_app();
        app.messages.push(user_text_message("before"));
        app.messages.push(assistant_tool_message("tool-1", model::ToolCallStatus::Completed));
        app.messages.push(user_text_message("after"));
        app.index_tool_call("tool-1".to_owned(), 1, 0);

        let _ = app.viewport.on_frame(80, 24);
        app.viewport.sync_message_count(3);
        app.viewport.mark_heights_valid();
        app.viewport.rebuild_prefix_sums();

        app.insert_message_tracked(1, user_text_message("inserted"));
        app.viewport.sync_message_count(app.messages.len());

        assert_eq!(app.lookup_tool_call("tool-1"), Some((2, 0)));
        assert_eq!(app.viewport.oldest_stale_index(), Some(1));
        assert_eq!(app.viewport.prefix_dirty_from(), Some(1));
    }

    #[test]
    fn remove_message_tracked_nontail_rebuilds_tool_indices_and_invalidates_suffix() {
        let mut app = make_test_app();
        app.messages.push(user_text_message("before"));
        app.messages.push(assistant_tool_message("tool-1", model::ToolCallStatus::Completed));
        app.messages.push(user_text_message("after"));
        app.index_tool_call("tool-1".to_owned(), 1, 0);

        let _ = app.viewport.on_frame(80, 24);
        app.viewport.sync_message_count(3);
        app.viewport.mark_heights_valid();
        app.viewport.rebuild_prefix_sums();

        let removed = app.remove_message_tracked(0);
        app.viewport.sync_message_count(app.messages.len());

        assert!(removed.is_some());
        assert_eq!(app.lookup_tool_call("tool-1"), Some((0, 0)));
        assert_eq!(app.viewport.oldest_stale_index(), Some(0));
        assert_eq!(app.viewport.prefix_dirty_from(), Some(0));
    }

    #[test]
    fn remove_message_tracked_tail_removes_orphaned_tool_indices() {
        let mut app = make_test_app();
        app.messages.push(user_text_message("before"));
        app.messages.push(assistant_tool_message("tool-1", model::ToolCallStatus::Completed));
        app.index_tool_call("tool-1".to_owned(), 1, 0);

        let removed = app.remove_message_tracked(1);

        assert!(removed.is_some());
        assert!(app.lookup_tool_call("tool-1").is_none());
    }

    #[test]
    fn remove_message_tracked_prunes_tool_scope_entries() {
        let mut app = make_test_app();
        app.messages.push(assistant_tool_message("tool-1", model::ToolCallStatus::Completed));
        app.index_tool_call("tool-1".to_owned(), 0, 0);
        app.register_tool_call_scope("tool-1".to_owned(), ToolCallScope::Subagent);

        let removed = app.remove_message_tracked(0);

        assert!(removed.is_some());
        assert_eq!(app.tool_call_scope("tool-1"), None);
    }

    #[test]
    fn clear_messages_tracked_clears_tool_and_terminal_tracking() {
        let mut app = make_test_app();
        app.messages.push(assistant_bash_tool_message(
            "bash-1",
            model::ToolCallStatus::InProgress,
            "term-1",
        ));
        app.index_tool_call("bash-1".to_owned(), 0, 0);
        app.sync_terminal_tool_call("term-1".to_owned(), 0, 0);
        app.pending_interaction_ids.push("bash-1".into());

        app.clear_messages_tracked();

        assert!(app.messages.is_empty());
        assert!(app.tool_call_index.is_empty());
        assert!(app.terminal_tool_calls.is_empty());
        assert!(app.terminal_tool_call_membership.is_empty());
        assert!(app.pending_interaction_ids.is_empty());
    }

    #[test]
    fn rebuild_tool_indices_skips_completed_terminal_refs() {
        let mut app = make_test_app();
        app.messages.push(assistant_bash_tool_message(
            "bash-1",
            model::ToolCallStatus::Completed,
            "term-1",
        ));
        app.index_tool_call("bash-1".to_owned(), 0, 0);
        app.sync_terminal_tool_call("term-1".to_owned(), 0, 0);

        app.rebuild_tool_indices_and_terminal_refs();

        assert!(app.terminal_tool_calls.is_empty());
        assert!(app.terminal_tool_call_membership.is_empty());
    }

    #[test]
    fn finalize_in_progress_tool_calls_invalidates_all_changed_messages() {
        let mut app = make_test_app();
        app.messages.push(assistant_tool_message("tool-1", model::ToolCallStatus::InProgress));
        app.messages.push(user_text_message("gap"));
        app.messages.push(assistant_tool_message("tool-2", model::ToolCallStatus::InProgress));

        let _ = app.viewport.on_frame(80, 24);
        app.viewport.sync_message_count(3);
        app.viewport.mark_heights_valid();
        app.viewport.rebuild_prefix_sums();

        let changed = app.finalize_in_progress_tool_calls(model::ToolCallStatus::Completed);

        assert_eq!(changed, 2);
        assert!(!app.viewport.message_height_is_current(0));
        assert!(app.viewport.message_height_is_current(1));
        assert!(!app.viewport.message_height_is_current(2));
        assert_eq!(app.viewport.oldest_stale_index(), Some(0));
    }

    // IncrementalMarkdown

    /// Simple render function for tests: wraps each line in a `Line`.
    fn test_render(src: &str) -> Vec<Line<'static>> {
        src.lines().map(|l| Line::from(l.to_owned())).collect()
    }

    fn test_render_key() -> super::messages::MarkdownRenderKey {
        super::messages::MarkdownRenderKey { width: 80, bg: None, preserve_newlines: false }
    }

    #[test]
    fn incr_default_empty() {
        let incr = IncrementalMarkdown::default();
        assert!(incr.full_text().is_empty());
    }

    #[test]
    fn incr_from_complete() {
        let incr = IncrementalMarkdown::from_complete("hello world");
        assert_eq!(incr.full_text(), "hello world");
    }

    #[test]
    fn incr_append_single_chunk() {
        let mut incr = IncrementalMarkdown::default();
        incr.append("hello");
        assert_eq!(incr.full_text(), "hello");
    }

    #[test]
    fn incr_append_accumulates_chunks() {
        let mut incr = IncrementalMarkdown::default();
        incr.append("line1");
        incr.append("\nline2");
        incr.append("\nline3");
        assert_eq!(incr.full_text(), "line1\nline2\nline3");
    }

    #[test]
    fn incr_append_preserves_paragraph_delimiters() {
        let mut incr = IncrementalMarkdown::default();
        incr.append("para1\n\npara2");
        assert_eq!(incr.full_text(), "para1\n\npara2");
    }

    #[test]
    fn incr_full_text_reconstruction() {
        let mut incr = IncrementalMarkdown::default();
        incr.append("p1\n\np2\n\np3");
        assert_eq!(incr.full_text(), "p1\n\np2\n\np3");
    }

    #[test]
    fn incr_lines_renders_all() {
        let mut incr = IncrementalMarkdown::default();
        incr.append("line1\n\nline2\n\nline3");
        let lines = incr.lines(test_render_key(), &test_render);
        // test_render maps each source line to one output line
        assert_eq!(lines.len(), 5);
    }

    #[test]
    fn incr_ensure_rendered_preserves_text() {
        let mut incr = IncrementalMarkdown::default();
        incr.append("p1\n\np2\n\ntail");
        incr.ensure_rendered(test_render_key(), &test_render);
        assert_eq!(incr.full_text(), "p1\n\np2\n\ntail");
    }

    #[test]
    fn incr_invalidate_renders_preserves_text() {
        let mut incr = IncrementalMarkdown::default();
        incr.append("p1\n\np2\n\ntail");
        incr.invalidate_renders();
        assert_eq!(incr.full_text(), "p1\n\np2\n\ntail");
    }

    #[test]
    fn incr_reuses_rendered_prefix_chunks() {
        use std::cell::Cell;

        let calls = Cell::new(0usize);
        let render = |src: &str| -> Vec<Line<'static>> {
            calls.set(calls.get() + 1);
            test_render(src)
        };

        let mut incr = IncrementalMarkdown::default();
        incr.append("p1\n\np2");
        let _ = incr.lines(test_render_key(), &render);
        assert_eq!(calls.get(), 2);

        incr.append(" tail");
        let _ = incr.lines(test_render_key(), &render);
        assert_eq!(calls.get(), 3);
    }

    #[test]
    fn incr_does_not_split_inside_fenced_code_blocks() {
        let calls = std::cell::Cell::new(0usize);
        let render = |src: &str| -> Vec<Line<'static>> {
            calls.set(calls.get() + 1);
            test_render(src)
        };

        let mut incr = IncrementalMarkdown::default();
        incr.append("```rust\nfn main() {\n\nprintln!(\"hi\");\n}\n```\n\nafter");
        let _ = incr.lines(test_render_key(), &render);

        assert_eq!(calls.get(), 2);
    }

    #[test]
    fn incr_streaming_simulation() {
        // Simulate a realistic streaming scenario
        let mut incr = IncrementalMarkdown::default();
        let chunks = ["Here is ", "some text.\n", "\nNext para", "graph here.\n\n", "Final."];
        for chunk in chunks {
            incr.append(chunk);
        }
        assert_eq!(incr.full_text(), "Here is some text.\n\nNext paragraph here.\n\nFinal.");
    }

    // ChatViewport

    #[test]
    fn viewport_new_defaults() {
        let vp = ChatViewport::new();
        assert_eq!(vp.scroll_offset, 0);
        assert_eq!(vp.scroll_target, 0);
        assert!(vp.auto_scroll);
        assert_eq!(vp.width, 0);
        assert!(vp.message_heights.is_empty());
        assert!(vp.oldest_stale_index().is_none());
        assert!(!vp.resize_remeasure_active());
        assert!(vp.height_prefix_sums.is_empty());
    }

    #[test]
    fn viewport_on_frame_sets_width() {
        let mut vp = ChatViewport::new();
        let _ = vp.on_frame(80, 24);
        assert_eq!(vp.width, 80);
        assert_eq!(vp.height, 24);
    }

    #[test]
    fn viewport_on_frame_resize_invalidates() {
        let mut vp = ChatViewport::new();
        let _ = vp.on_frame(80, 24);
        vp.set_message_height(0, 10);
        vp.set_message_height(1, 20);
        vp.rebuild_prefix_sums();

        // Resize: old heights are kept as approximations,
        // but width markers are invalidated so re-measurement happens.
        let _ = vp.on_frame(120, 24);
        assert_eq!(vp.message_height(0), 10); // kept, not zeroed
        assert_eq!(vp.message_height(1), 20); // kept, not zeroed
        assert_eq!(vp.message_heights_width, 0); // forces re-measure
        assert_eq!(vp.prefix_sums_width, 0); // forces rebuild
    }

    #[test]
    fn viewport_on_frame_same_width_no_invalidation() {
        let mut vp = ChatViewport::new();
        let _ = vp.on_frame(80, 24);
        vp.set_message_height(0, 10);
        let _ = vp.on_frame(80, 24); // same width
        assert_eq!(vp.message_height(0), 10); // not zeroed
    }

    #[test]
    fn viewport_on_frame_height_change_preserves_message_measurements() {
        let mut vp = ChatViewport::new();
        let _ = vp.on_frame(80, 24);
        vp.sync_message_count(2);
        vp.set_message_height(0, 10);
        vp.set_message_height(1, 20);
        vp.mark_heights_valid();
        vp.rebuild_prefix_sums();

        let change = vp.on_frame(80, 12);

        assert!(!change.width_changed);
        assert!(change.height_changed);
        assert_eq!(vp.height, 12);
        assert_eq!(vp.message_heights_width, 80);
        assert_eq!(vp.prefix_sums_width, 80);
        assert!(!vp.resize_remeasure_active());
        assert!(vp.message_height_is_current(0));
        assert!(vp.message_height_is_current(1));
    }

    #[test]
    fn viewport_message_height_set_and_get() {
        let mut vp = ChatViewport::new();
        let _ = vp.on_frame(80, 24);
        vp.set_message_height(0, 5);
        vp.set_message_height(1, 10);
        assert_eq!(vp.message_height(0), 5);
        assert_eq!(vp.message_height(1), 10);
        assert_eq!(vp.message_height(2), 0); // out of bounds
    }

    #[test]
    fn viewport_message_height_grows_vec() {
        let mut vp = ChatViewport::new();
        let _ = vp.on_frame(80, 24);
        vp.set_message_height(5, 42);
        assert_eq!(vp.message_heights.len(), 6);
        assert_eq!(vp.message_height(5), 42);
        assert_eq!(vp.message_height(3), 0); // gap filled with 0
    }

    #[test]
    fn viewport_invalidate_message_tracks_oldest_index() {
        let mut vp = ChatViewport::new();
        vp.sync_message_count(8);
        vp.mark_heights_valid();
        vp.invalidate_message(5);
        vp.invalidate_message(2);
        vp.invalidate_message(7);
        assert_eq!(vp.oldest_stale_index(), Some(2));
    }

    #[test]
    fn viewport_mark_heights_valid_clears_dirty_index() {
        let mut vp = ChatViewport::new();
        let _ = vp.on_frame(80, 24);
        vp.sync_message_count(2);
        vp.mark_heights_valid();
        vp.invalidate_message(1);
        assert_eq!(vp.oldest_stale_index(), Some(1));
        vp.mark_heights_valid();
        assert!(vp.oldest_stale_index().is_none());
    }

    #[test]
    fn viewport_resize_remeasure_tracks_partial_exactness() {
        let mut vp = ChatViewport::new();
        let _ = vp.on_frame(80, 24);
        vp.sync_message_count(3);
        vp.set_message_height(0, 4);
        vp.set_message_height(1, 5);
        vp.set_message_height(2, 6);
        vp.mark_heights_valid();

        let _ = vp.on_frame(120, 24);
        assert!(vp.resize_remeasure_active());
        assert!(!vp.message_height_is_current(0));

        vp.mark_message_height_measured(1);
        assert!(vp.message_height_is_current(1));
        assert!(!vp.message_height_is_current(0));

        vp.mark_heights_valid();
        assert_eq!(vp.message_heights_width, 120);
        assert!(vp.message_height_is_current(0));
        assert!(!vp.resize_remeasure_active());
    }

    #[test]
    fn viewport_resize_remeasure_expands_outward_from_anchor() {
        let mut vp = ChatViewport::new();
        let _ = vp.on_frame(80, 24);
        vp.sync_message_count(6);
        vp.mark_heights_valid();

        let _ = vp.on_frame(100, 24);
        vp.ensure_resize_remeasure_anchor(2, 3, 6);

        assert_eq!(vp.next_resize_remeasure_index(6), Some(1));
        assert_eq!(vp.next_resize_remeasure_index(6), Some(0));
        assert_eq!(vp.next_resize_remeasure_index(6), Some(4));
        assert_eq!(vp.next_resize_remeasure_index(6), Some(5));
        assert_eq!(vp.next_resize_remeasure_index(6), None);
        assert!(!vp.resize_remeasure_active());
    }

    #[allow(clippy::cast_precision_loss)]
    #[test]
    fn viewport_restore_resize_anchor_keeps_same_message_visible() {
        let mut vp = ChatViewport::new();
        let _ = vp.on_frame(80, 24);
        vp.sync_message_count(4);
        for idx in 0..4 {
            vp.set_message_height(idx, 5);
        }
        vp.mark_heights_valid();
        vp.rebuild_prefix_sums();

        vp.auto_scroll = false;
        vp.scroll_offset = 7;
        vp.scroll_target = 7;
        vp.scroll_pos = 7.0;

        let _ = vp.on_frame(40, 24);
        let (anchor_idx, anchor_offset) =
            vp.resize_scroll_anchor().expect("resize should snapshot a scroll anchor");
        assert_eq!((anchor_idx, anchor_offset), (1, 2));

        vp.set_message_height(0, 12);
        vp.set_message_height(1, 8);
        vp.set_message_height(2, 6);
        vp.set_message_height(3, 6);
        vp.prefix_sums_width = 0;
        vp.rebuild_prefix_sums();
        vp.restore_scroll_anchor(anchor_idx, anchor_offset);

        assert_eq!(vp.scroll_offset, 14);
        assert_eq!(vp.find_first_visible(vp.scroll_offset), 1);
    }

    #[allow(clippy::cast_precision_loss)]
    #[test]
    fn viewport_preserves_resize_anchor_when_followup_remeasure_replaces_plan() {
        let mut vp = ChatViewport::new();
        let _ = vp.on_frame(80, 24);
        vp.sync_message_count(4);
        for idx in 0..4 {
            vp.set_message_height(idx, 5);
        }
        vp.mark_heights_valid();
        vp.rebuild_prefix_sums();

        vp.auto_scroll = false;
        vp.scroll_offset = 7;
        vp.scroll_target = 7;
        vp.scroll_pos = 7.0;

        let _ = vp.on_frame(40, 24);
        let resize_anchor = vp.resize_scroll_anchor().expect("resize should preserve an anchor");
        assert_eq!(resize_anchor, (1, 2));
        assert_eq!(vp.remeasure_reason(), Some(LayoutRemeasureReason::Resize));

        vp.invalidate_messages_from(0);

        assert_eq!(vp.remeasure_reason(), Some(LayoutRemeasureReason::MessagesFrom));
        assert_eq!(vp.resize_scroll_anchor(), Some(resize_anchor));
        assert_eq!(vp.scroll_anchor_to_restore(), Some(resize_anchor));
    }

    #[allow(clippy::cast_precision_loss)]
    #[test]
    fn viewport_delays_anchor_restore_until_prefix_above_is_exact() {
        let mut vp = ChatViewport::new();
        let _ = vp.on_frame(80, 24);
        vp.sync_message_count(4);
        for idx in 0..4 {
            vp.set_message_height(idx, 5);
        }
        vp.mark_heights_valid();
        vp.rebuild_prefix_sums();

        vp.auto_scroll = false;
        vp.scroll_offset = 12;
        vp.scroll_target = 12;
        vp.scroll_pos = 12.0;

        let _ = vp.on_frame(40, 24);
        let anchor = vp.resize_scroll_anchor().expect("resize should preserve an anchor");
        assert_eq!(anchor, (2, 2));
        assert_eq!(vp.scroll_anchor_to_restore(), Some(anchor));
        assert_eq!(vp.ready_scroll_anchor_to_restore(), None);

        vp.set_message_height(2, 9);
        vp.mark_message_height_measured(2);
        vp.rebuild_prefix_sums();
        assert_eq!(vp.ready_scroll_anchor_to_restore(), None);

        vp.set_message_height(0, 11);
        vp.mark_message_height_measured(0);
        vp.set_message_height(1, 8);
        vp.mark_message_height_measured(1);
        vp.rebuild_prefix_sums();

        assert_eq!(vp.ready_scroll_anchor_to_restore(), Some(anchor));
    }

    #[test]
    fn viewport_prioritizes_rows_above_preserved_anchor_until_restore_is_exact() {
        let mut vp = ChatViewport::new();
        let _ = vp.on_frame(80, 24);
        vp.sync_message_count(6);
        for idx in 0..6 {
            vp.set_message_height(idx, 5);
        }
        vp.mark_heights_valid();
        vp.rebuild_prefix_sums();

        vp.auto_scroll = false;
        vp.scroll_offset = 12;
        vp.scroll_target = 12;
        vp.scroll_pos = 12.0;

        let _ = vp.on_frame(40, 24);
        vp.ensure_resize_remeasure_anchor(2, 3, 6);

        assert_eq!(vp.next_resize_remeasure_index(6), Some(1));
        assert_eq!(vp.next_resize_remeasure_index(6), Some(0));
        assert_eq!(vp.next_resize_remeasure_index(6), Some(4));
    }

    #[allow(clippy::cast_precision_loss)]
    #[test]
    fn viewport_global_remeasure_preserves_anchor_while_prefix_above_converges() {
        let mut vp = ChatViewport::new();
        let _ = vp.on_frame(80, 24);
        vp.sync_message_count(6);
        for idx in 0..6 {
            vp.set_message_height(idx, 5);
        }
        vp.mark_heights_valid();
        vp.rebuild_prefix_sums();

        vp.auto_scroll = false;
        vp.scroll_offset = 17;
        vp.scroll_target = 17;
        vp.scroll_pos = 17.0;

        vp.invalidate_all_messages(LayoutRemeasureReason::Global);
        let anchor =
            vp.scroll_anchor_to_restore().expect("global remeasure should preserve an anchor");
        assert_eq!(anchor, (3, 2));

        vp.invalidate_message(5);

        assert_eq!(vp.remeasure_reason(), Some(LayoutRemeasureReason::MessageChanged));
        assert_eq!(vp.scroll_anchor_to_restore(), Some(anchor));

        vp.set_message_height(0, 12);
        vp.mark_message_height_measured(0);
        vp.set_message_height(1, 8);
        vp.mark_message_height_measured(1);
        vp.rebuild_prefix_sums();

        assert_eq!(vp.find_first_visible(vp.scroll_offset), 1);

        vp.restore_scroll_anchor(anchor.0, anchor.1);

        assert_eq!(vp.find_first_visible(vp.scroll_offset), 3);
        assert_eq!(vp.scroll_offset, 27);
    }

    #[test]
    fn viewport_prefix_sums_basic() {
        let mut vp = ChatViewport::new();
        let _ = vp.on_frame(80, 24);
        vp.set_message_height(0, 5);
        vp.set_message_height(1, 10);
        vp.set_message_height(2, 3);
        vp.rebuild_prefix_sums();
        assert_eq!(vp.total_message_height(), 18);
        assert_eq!(vp.cumulative_height_before(0), 0);
        assert_eq!(vp.cumulative_height_before(1), 5);
        assert_eq!(vp.cumulative_height_before(2), 15);
    }

    #[test]
    fn viewport_prefix_sums_streaming_fast_path() {
        let mut vp = ChatViewport::new();
        let _ = vp.on_frame(80, 24);
        vp.set_message_height(0, 5);
        vp.set_message_height(1, 10);
        vp.rebuild_prefix_sums();
        assert_eq!(vp.total_message_height(), 15);

        // Simulate streaming: last message grows
        vp.set_message_height(1, 20);
        vp.rebuild_prefix_sums(); // should hit fast path
        assert_eq!(vp.total_message_height(), 25);
        assert_eq!(vp.cumulative_height_before(1), 5);
    }

    #[test]
    fn viewport_find_first_visible() {
        let mut vp = ChatViewport::new();
        let _ = vp.on_frame(80, 24);
        vp.set_message_height(0, 10);
        vp.set_message_height(1, 10);
        vp.set_message_height(2, 10);
        vp.rebuild_prefix_sums();

        assert_eq!(vp.find_first_visible(0), 0);
        assert_eq!(vp.find_first_visible(10), 1);
        assert_eq!(vp.find_first_visible(15), 1);
        assert_eq!(vp.find_first_visible(20), 2);
    }

    #[test]
    fn viewport_find_first_visible_handles_offsets_before_first_boundary() {
        let mut vp = ChatViewport::new();
        let _ = vp.on_frame(80, 24);
        vp.set_message_height(0, 10);
        vp.set_message_height(1, 10);
        vp.rebuild_prefix_sums();

        assert_eq!(vp.find_first_visible(0), 0);
        assert_eq!(vp.find_first_visible(5), 0);
        assert_eq!(vp.find_first_visible(15), 1);
    }

    #[test]
    fn viewport_scroll_up_down() {
        let mut vp = ChatViewport::new();
        vp.scroll_target = 20;
        vp.scroll_pos = 20.0;
        vp.scroll_offset = 20;
        vp.auto_scroll = true;

        vp.scroll_up(5);
        assert_eq!(vp.scroll_target, 15);
        assert!((vp.scroll_pos - 15.0).abs() < f32::EPSILON);
        assert_eq!(vp.scroll_offset, 15);
        assert!(!vp.auto_scroll); // disabled on manual scroll

        vp.scroll_down(3);
        assert_eq!(vp.scroll_target, 18);
        assert!((vp.scroll_pos - 18.0).abs() < f32::EPSILON);
        assert_eq!(vp.scroll_offset, 18);
        assert!(!vp.auto_scroll); // not re-engaged by scroll_down
    }

    #[test]
    fn viewport_scroll_up_saturates() {
        let mut vp = ChatViewport::new();
        vp.scroll_target = 2;
        vp.scroll_pos = 2.0;
        vp.scroll_offset = 2;
        vp.scroll_up(10);
        assert_eq!(vp.scroll_target, 0);
        assert!(vp.scroll_pos.abs() < f32::EPSILON);
        assert_eq!(vp.scroll_offset, 0);
    }

    #[test]
    fn viewport_engage_auto_scroll() {
        let mut vp = ChatViewport::new();
        vp.auto_scroll = false;
        vp.engage_auto_scroll();
        assert!(vp.auto_scroll);
    }

    #[test]
    fn viewport_default_eq_new() {
        let a = ChatViewport::new();
        let b = ChatViewport::default();
        assert_eq!(a.width, b.width);
        assert_eq!(a.auto_scroll, b.auto_scroll);
        assert_eq!(a.message_heights.len(), b.message_heights.len());
    }

    #[test]
    fn focus_owner_defaults_to_input() {
        let app = make_test_app();
        assert_eq!(app.focus_owner(), FocusOwner::Input);
    }

    #[test]
    fn focus_owner_todo_when_panel_open_and_focused() {
        let mut app = make_test_app();
        app.todos.push(TodoItem {
            content: "Task".into(),
            status: TodoStatus::Pending,
            active_form: String::new(),
        });
        app.show_todo_panel = true;
        app.claim_focus_target(FocusTarget::TodoList);
        assert_eq!(app.focus_owner(), FocusOwner::TodoList);
    }

    #[test]
    fn focus_owner_permission_overrides_todo() {
        let mut app = make_test_app();
        app.todos.push(TodoItem {
            content: "Task".into(),
            status: TodoStatus::Pending,
            active_form: String::new(),
        });
        app.show_todo_panel = true;
        app.claim_focus_target(FocusTarget::TodoList);
        app.pending_interaction_ids.push("perm-1".into());
        app.claim_focus_target(FocusTarget::Permission);
        assert_eq!(app.focus_owner(), FocusOwner::Permission);
    }

    #[test]
    fn focus_owner_mention_overrides_permission_and_todo() {
        let mut app = make_test_app();
        app.todos.push(TodoItem {
            content: "Task".into(),
            status: TodoStatus::Pending,
            active_form: String::new(),
        });
        app.show_todo_panel = true;
        app.claim_focus_target(FocusTarget::TodoList);
        app.pending_interaction_ids.push("perm-1".into());
        app.claim_focus_target(FocusTarget::Permission);
        app.slash = Some(SlashState {
            trigger_row: 0,
            trigger_col: 0,
            query: String::new(),
            context: SlashContext::CommandName,
            candidates: vec![SlashCandidate {
                insert_value: "/config".into(),
                primary: "/config".into(),
                secondary: Some("Open settings".into()),
            }],
            dialog: dialog::DialogState::default(),
        });
        app.claim_focus_target(FocusTarget::Mention);
        assert_eq!(app.focus_owner(), FocusOwner::Mention);
    }

    #[test]
    fn focus_owner_falls_back_to_input_when_claim_is_not_available() {
        let mut app = make_test_app();
        app.claim_focus_target(FocusTarget::TodoList);
        assert_eq!(app.focus_owner(), FocusOwner::Input);
    }

    #[test]
    fn claim_and_release_focus_target() {
        let mut app = make_test_app();
        app.todos.push(TodoItem {
            content: "Task".into(),
            status: TodoStatus::Pending,
            active_form: String::new(),
        });
        app.show_todo_panel = true;
        app.claim_focus_target(FocusTarget::TodoList);
        assert_eq!(app.focus_owner(), FocusOwner::TodoList);
        app.release_focus_target(FocusTarget::TodoList);
        assert_eq!(app.focus_owner(), FocusOwner::Input);
    }

    #[test]
    fn latest_claim_wins_across_equal_targets() {
        let mut app = make_test_app();
        app.todos.push(TodoItem {
            content: "Task".into(),
            status: TodoStatus::Pending,
            active_form: String::new(),
        });
        app.show_todo_panel = true;
        app.slash = Some(SlashState {
            trigger_row: 0,
            trigger_col: 0,
            query: String::new(),
            context: SlashContext::CommandName,
            candidates: vec![SlashCandidate {
                insert_value: "/config".into(),
                primary: "/config".into(),
                secondary: Some("Open settings".into()),
            }],
            dialog: dialog::DialogState::default(),
        });
        app.pending_interaction_ids.push("perm-1".into());

        app.claim_focus_target(FocusTarget::TodoList);
        assert_eq!(app.focus_owner(), FocusOwner::TodoList);

        app.claim_focus_target(FocusTarget::Permission);
        assert_eq!(app.focus_owner(), FocusOwner::Permission);

        app.claim_focus_target(FocusTarget::Mention);
        assert_eq!(app.focus_owner(), FocusOwner::Mention);

        app.release_focus_target(FocusTarget::Mention);
        assert_eq!(app.focus_owner(), FocusOwner::Permission);
    }

    // --- InvalidationLevel tests ---

    #[test]
    fn invalidate_single_tail_preserves_prefix_sums() {
        let mut app = make_test_app();
        app.messages.push(user_text_message("a"));
        app.messages.push(user_text_message("b"));
        app.messages.push(user_text_message("c"));
        let _ = app.viewport.on_frame(80, 24);
        app.viewport.set_message_height(0, 5);
        app.viewport.set_message_height(1, 10);
        app.viewport.set_message_height(2, 3);
        app.viewport.mark_heights_valid();
        app.viewport.rebuild_prefix_sums();

        app.invalidate_layout(InvalidationLevel::MessageChanged(2)); // tail

        assert_eq!(app.viewport.oldest_stale_index(), Some(2));
        assert_eq!(app.viewport.prefix_dirty_from(), Some(2));
        assert_eq!(app.viewport.prefix_sums_width, 0);
    }

    #[test]
    fn invalidate_single_nontail_invalidates_prefix_sums() {
        let mut app = make_test_app();
        app.messages.push(user_text_message("a"));
        app.messages.push(user_text_message("b"));
        app.messages.push(user_text_message("c"));
        let _ = app.viewport.on_frame(80, 24);
        app.viewport.set_message_height(0, 5);
        app.viewport.set_message_height(1, 10);
        app.viewport.set_message_height(2, 3);
        app.viewport.mark_heights_valid();
        app.viewport.rebuild_prefix_sums();

        app.invalidate_layout(InvalidationLevel::MessageChanged(1)); // non-tail

        assert_eq!(app.viewport.oldest_stale_index(), Some(1));
        assert_eq!(app.viewport.prefix_dirty_from(), Some(1));
        assert_eq!(app.viewport.prefix_sums_width, 0);
    }

    #[test]
    fn invalidate_from_always_invalidates_prefix_sums() {
        let mut app = make_test_app();
        app.messages.push(user_text_message("a"));
        app.messages.push(user_text_message("b"));
        app.messages.push(user_text_message("c"));
        let _ = app.viewport.on_frame(80, 24);
        app.viewport.set_message_height(0, 5);
        app.viewport.set_message_height(1, 10);
        app.viewport.set_message_height(2, 3);
        app.viewport.mark_heights_valid();
        app.viewport.rebuild_prefix_sums();
        assert_ne!(app.viewport.prefix_sums_width, 0);

        // From at tail index still invalidates prefix sums (unlike Single).
        app.invalidate_layout(InvalidationLevel::MessagesFrom(2));

        assert_eq!(app.viewport.oldest_stale_index(), Some(2));
        assert_eq!(app.viewport.prefix_dirty_from(), Some(2));
        assert_eq!(app.viewport.prefix_sums_width, 0);
    }

    #[test]
    fn invalidate_from_zero_matches_old_mark_all() {
        let mut app = make_test_app();
        app.messages.push(user_text_message("a"));
        app.messages.push(user_text_message("b"));
        app.messages.push(user_text_message("c"));
        let _ = app.viewport.on_frame(80, 24);
        app.viewport.set_message_height(0, 5);
        app.viewport.set_message_height(1, 10);
        app.viewport.set_message_height(2, 3);
        app.viewport.mark_heights_valid();
        app.viewport.rebuild_prefix_sums();

        app.invalidate_layout(InvalidationLevel::MessagesFrom(0));

        assert_eq!(app.viewport.oldest_stale_index(), Some(0));
        assert_eq!(app.viewport.prefix_dirty_from(), Some(0));
        assert_eq!(app.viewport.prefix_sums_width, 0);
    }

    #[test]
    fn invalidate_global_bumps_generation() {
        let mut app = make_test_app();
        app.messages.push(user_text_message("a"));
        app.messages.push(user_text_message("b"));
        app.messages.push(user_text_message("c"));
        let _ = app.viewport.on_frame(80, 24);
        app.viewport.sync_message_count(3);
        app.viewport.mark_heights_valid();
        app.viewport.rebuild_prefix_sums();
        let gen_before = app.viewport.layout_generation;

        app.invalidate_layout(InvalidationLevel::Global);

        assert_eq!(app.viewport.oldest_stale_index(), Some(0));
        assert_eq!(app.viewport.prefix_dirty_from(), Some(0));
        assert_eq!(app.viewport.prefix_sums_width, 0);
        assert_eq!(app.viewport.layout_generation, gen_before + 1);
    }

    #[test]
    fn invalidate_global_noop_on_empty() {
        let mut app = make_test_app();
        assert!(app.messages.is_empty());
        let gen_before = app.viewport.layout_generation;

        app.invalidate_layout(InvalidationLevel::Global);

        assert!(app.viewport.oldest_stale_index().is_none());
        assert_eq!(app.viewport.layout_generation, gen_before);
    }

    #[test]
    fn invalidate_message_tracks_oldest_stale_index() {
        let mut app = make_test_app();
        // Need enough messages so all indices are non-tail for consistent behavior.
        for _ in 0..10 {
            app.messages.push(user_text_message("x"));
        }
        app.viewport.sync_message_count(10);
        app.viewport.mark_heights_valid();

        app.invalidate_layout(InvalidationLevel::MessageChanged(5));
        app.invalidate_layout(InvalidationLevel::MessageChanged(2));
        app.invalidate_layout(InvalidationLevel::MessageChanged(7));

        assert_eq!(app.viewport.oldest_stale_index(), Some(2));
    }

    #[test]
    fn invalidation_level_eq_and_debug() {
        assert_eq!(InvalidationLevel::MessageChanged(5), InvalidationLevel::MessageChanged(5));
        assert_ne!(InvalidationLevel::MessageChanged(5), InvalidationLevel::MessagesFrom(5));
        assert_eq!(InvalidationLevel::Global, InvalidationLevel::Global);
        assert_eq!(InvalidationLevel::Resize, InvalidationLevel::Resize);
        // Debug derive works
        let dbg = format!("{:?}", InvalidationLevel::MessagesFrom(3));
        assert!(dbg.contains("MessagesFrom"));
    }
}
