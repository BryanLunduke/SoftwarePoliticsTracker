// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

pub(crate) mod auth;
mod cache_policy;
pub(crate) mod config;
mod connect;
mod dialog;
mod events;
mod focus;
mod inline_interactions;
pub(crate) mod input;
mod input_submit;
mod keys;
pub(crate) mod mention;
mod notify;
pub(crate) mod paste_burst;
mod permissions;
pub(crate) mod plugins;
mod questions;
mod selection;
mod service_status_check;
pub(crate) mod slash;
mod state;
pub(crate) mod subagent;
mod terminal;
mod todos;
mod trust;
mod update_check;
pub(crate) mod usage;
mod view;

// Re-export all public types so `crate::app::App`, `crate::app::BlockCache`, etc. still work.
pub use cache_policy::{
    CacheSplitPolicy, DEFAULT_CACHE_SPLIT_HARD_LIMIT_BYTES, DEFAULT_CACHE_SPLIT_SOFT_LIMIT_BYTES,
    DEFAULT_TOOL_PREVIEW_LIMIT_BYTES, TextSplitDecision, TextSplitKind, default_cache_split_policy,
    find_text_split, find_text_split_index,
};
pub use config::{ConfigState, ConfigTab};
pub use connect::{create_app, start_connection};
pub use events::{handle_client_event, handle_terminal_event};
pub use focus::{FocusManager, FocusOwner, FocusTarget};
pub use input::InputState;
pub(crate) use selection::normalize_selection;
pub use service_status_check::start_service_status_check;
pub(crate) use state::MarkdownRenderKey;
pub(crate) use state::cache_metrics;
pub use state::{
    App, AppStatus, BlockCache, CacheMetrics, CancelOrigin, ChatMessage, ChatViewport, ExtraUsage,
    HelpView, IncrementalMarkdown, InlinePermission, InlineQuestion, InvalidationLevel,
    LayoutInvalidation, LoginHint, McpState, MessageBlock, MessageRole, MessageUsage, ModeInfo,
    ModeState, PasteSessionState, PendingCommandAck, RecentSessionInfo, SelectionKind,
    SelectionPoint, SelectionState, SessionUsageState, SystemSeverity, TerminalSnapshotMode,
    TextBlock, TextBlockSpacing, TodoItem, TodoStatus, ToolCallInfo, ToolCallScope, UsageSnapshot,
    UsageSourceKind, UsageSourceMode, UsageState, UsageWindow, WelcomeBlock, is_execute_tool_name,
};
pub use trust::TrustSelection;
pub use update_check::start_update_check;
pub use view::ActiveView;

use crate::agent::model;
use crossterm::event::{
    EventStream, KeyboardEnhancementFlags, PopKeyboardEnhancementFlags,
    PushKeyboardEnhancementFlags,
};
use futures::{FutureExt as _, StreamExt};
use std::time::{Duration, Instant};

const SPINNER_FRAME_INTERVAL_NORMAL: Duration = Duration::from_millis(30);
const SPINNER_FRAME_INTERVAL_REDUCED: Duration = Duration::from_millis(120);

// ---------------------------------------------------------------------------
// Terminal suspend / resume helpers (reused by /login, /logout)
// ---------------------------------------------------------------------------

/// Disable raw mode and crossterm features so a child process can own the
/// terminal (e.g. `claude auth login` which opens a browser flow).
pub(crate) fn suspend_terminal() {
    let _ = crossterm::execute!(
        std::io::stdout(),
        crossterm::event::DisableBracketedPaste,
        crossterm::event::DisableMouseCapture,
        crossterm::event::DisableFocusChange,
        PopKeyboardEnhancementFlags
    );
    let _ = crossterm::terminal::disable_raw_mode();
}

/// Re-enable raw mode and crossterm features after a child process finishes.
pub(crate) fn resume_terminal() {
    let _ = crossterm::terminal::enable_raw_mode();
    let _ = crossterm::execute!(
        std::io::stdout(),
        crossterm::event::EnableBracketedPaste,
        crossterm::event::EnableMouseCapture,
        crossterm::event::EnableFocusChange,
        PushKeyboardEnhancementFlags(
            KeyboardEnhancementFlags::DISAMBIGUATE_ESCAPE_CODES
                | KeyboardEnhancementFlags::REPORT_EVENT_TYPES
                | KeyboardEnhancementFlags::REPORT_ALTERNATE_KEYS
        )
    );
}

// ---------------------------------------------------------------------------
// TUI event loop
// ---------------------------------------------------------------------------

#[allow(clippy::too_many_lines, clippy::cast_precision_loss)]
pub async fn run_tui(app: &mut App) -> anyhow::Result<()> {
    let mut terminal = ratatui::init();
    let mut os_shutdown = Box::pin(wait_for_shutdown_signal());

    // Enable bracketed paste, mouse capture, and enhanced keyboard protocol
    resume_terminal();

    let mut events = EventStream::new();
    let tick_duration = Duration::from_millis(16);
    let mut last_render = Instant::now();

    loop {
        start_connection(app);

        // Phase 1: wait for at least one event or the next frame tick
        let time_to_next = tick_duration.saturating_sub(last_render.elapsed());
        tokio::select! {
            Some(Ok(event)) = events.next() => {
                events::handle_terminal_event(app, event);
            }
            Some(event) = app.event_rx.recv() => {
                events::handle_client_event(app, event);
            }
            shutdown = &mut os_shutdown => {
                if let Err(err) = shutdown {
                    tracing::warn!(%err, "OS shutdown signal listener failed");
                }
                app.should_quit = true;
            }
            () = tokio::time::sleep(time_to_next) => {}
        }

        // Phase 2: drain all remaining queued events (non-blocking)
        loop {
            // Try terminal events first (keeps typing responsive)
            if let Some(Some(Ok(event))) = events.next().now_or_never() {
                events::handle_terminal_event(app, event);
                continue;
            }
            // Then client events
            match app.event_rx.try_recv() {
                Ok(event) => {
                    events::handle_client_event(app, event);
                }
                Err(_) => break,
            }
        }

        // Tick the burst detector: flush any held/buffered content that
        // has timed out. EmitChar re-inserts a single held character;
        // EmitPaste feeds the accumulated burst into the paste queue.
        if app.active_view == ActiveView::Chat
            && let Some(action) = app.paste_burst.tick(Instant::now())
        {
            match action {
                paste_burst::FlushAction::EmitChar(ch) => {
                    let _ = app.input.textarea_insert_char(ch);
                }
                paste_burst::FlushAction::EmitPaste(text) => {
                    app.queue_paste_text(&text);
                }
            }
        }

        // Merge and process `Event::Paste` chunks as one paste action.
        if app.active_view == ActiveView::Chat && !app.pending_paste_text.is_empty() {
            finalize_pending_paste_event(app);
        }

        mention::tick(app, Instant::now());

        // Deferred submit: if Enter was pressed and no paste payload arrived
        // in this drain cycle, restore the exact pre-submit snapshot and
        // submit that unchanged draft.
        if app.active_view == ActiveView::Chat && app.pending_submit.is_some() {
            finalize_deferred_submit(app);
        }

        if app.should_quit {
            break;
        }

        // Phase 3: render once (only when something changed)
        let is_animating = matches!(
            app.status,
            AppStatus::Connecting
                | AppStatus::CommandPending
                | AppStatus::Thinking
                | AppStatus::Running
        ) || app.is_compacting;
        if is_animating {
            advance_spinner_frame(app, Instant::now());
            app.needs_redraw = true;
        } else {
            app.spinner_last_advance_at = None;
        }
        // Smooth scroll still settling
        let scroll_delta = (app.viewport.scroll_target as f32 - app.viewport.scroll_pos).abs();
        if scroll_delta >= 0.01 {
            app.needs_redraw = true;
        }
        if terminal::update_terminal_outputs(app) {
            app.needs_redraw = true;
        }
        if app.force_redraw {
            terminal.clear()?;
            app.force_redraw = false;
            app.needs_redraw = true;
        }
        if app.needs_redraw {
            if let Some(ref mut perf) = app.perf {
                perf.next_frame();
            }
            if app.perf.is_some() {
                app.mark_frame_presented(Instant::now());
            }
            #[allow(clippy::drop_non_drop)]
            {
                let timer = app.perf.as_ref().map(|p| p.start("frame_total"));
                let draw_timer = app.perf.as_ref().map(|p| p.start("frame::terminal_draw"));
                terminal.draw(|f| crate::ui::render(f, app))?;
                drop(draw_timer);
                drop(timer);
            }
            app.needs_redraw = false;
            last_render = Instant::now();
        }
    }

    // --- Graceful shutdown ---

    // Dismiss all pending inline permissions (reject via last option)
    for tool_id in std::mem::take(&mut app.pending_interaction_ids) {
        if let Some((mi, bi)) = app.tool_call_index.get(&tool_id).copied()
            && let Some(MessageBlock::ToolCall(tc)) =
                app.messages.get_mut(mi).and_then(|m| m.blocks.get_mut(bi))
        {
            let tc = tc.as_mut();
            if let Some(pending) = tc.pending_permission.take()
                && let Some(last_opt) = pending.options.last()
            {
                let _ = pending.response_tx.send(model::RequestPermissionResponse::new(
                    model::RequestPermissionOutcome::Selected(
                        model::SelectedPermissionOutcome::new(last_opt.option_id.clone()),
                    ),
                ));
            }
            if let Some(pending) = tc.pending_question.take() {
                let _ = pending.response_tx.send(model::RequestQuestionResponse::new(
                    model::RequestQuestionOutcome::Cancelled,
                ));
            }
        }
    }

    // Cancel any active turn and give the adapter a moment to clean up
    if matches!(app.status, AppStatus::Thinking | AppStatus::Running)
        && let Some(ref conn) = app.conn
        && let Some(sid) = app.session_id.clone()
    {
        let _ = conn.cancel(sid.to_string());
    }

    // Restore terminal
    suspend_terminal();
    ratatui::restore();

    Ok(())
}

fn advance_spinner_frame(app: &mut App, now: Instant) {
    let interval = if app.config.prefers_reduced_motion_effective() {
        SPINNER_FRAME_INTERVAL_REDUCED
    } else {
        SPINNER_FRAME_INTERVAL_NORMAL
    };

    match app.spinner_last_advance_at {
        Some(last_advance) if now.duration_since(last_advance) < interval => {}
        Some(_) | None => {
            app.spinner_frame = app.spinner_frame.wrapping_add(1);
            app.spinner_last_advance_at = Some(now);
        }
    }
}

async fn wait_for_shutdown_signal() -> std::io::Result<()> {
    #[cfg(unix)]
    {
        let mut sigterm =
            tokio::signal::unix::signal(tokio::signal::unix::SignalKind::terminate())?;
        tokio::select! {
            sigint = tokio::signal::ctrl_c() => {
                sigint?;
            }
            _ = sigterm.recv() => {}
        }
        Ok(())
    }
    #[cfg(not(unix))]
    {
        tokio::signal::ctrl_c().await
    }
}

/// Finalize queued `Event::Paste` chunks for this drain cycle.
fn finalize_pending_paste_event(app: &mut App) {
    let pasted = std::mem::take(&mut app.pending_paste_text);
    if pasted.is_empty() {
        return;
    }
    tracing::debug!(
        text = %debug_paste_text(&pasted),
        len = pasted.chars().count(),
        cursor_row = app.input.cursor_row(),
        cursor_col = app.input.cursor_col(),
        "paste_finalize: begin"
    );

    let session = app.pending_paste_session.take().unwrap_or_else(|| {
        let id = app.next_paste_session_id;
        app.next_paste_session_id = app.next_paste_session_id.saturating_add(1);
        state::PasteSessionState {
            id,
            start: SelectionPoint { row: app.input.cursor_row(), col: app.input.cursor_col() },
            placeholder_index: None,
        }
    });
    tracing::debug!(
        session_id = session.id,
        start_row = session.start.row,
        start_col = session.start.col,
        placeholder_index = ?session.placeholder_index,
        "paste_finalize: session"
    );

    if session.placeholder_index.is_none() {
        let end = SelectionPoint { row: app.input.cursor_row(), col: app.input.cursor_col() };
        tracing::debug!(
            end_row = end.row,
            end_col = end.col,
            "paste_finalize: strip leaked inline range"
        );
        strip_input_range(app, session.start, end);
    }

    let appended = session
        .placeholder_index
        .and_then(|session_idx| {
            let current_line = app.input.lines().get(app.input.cursor_row())?;
            let current_idx =
                input::parse_paste_placeholder_before_cursor(current_line, app.input.cursor_col())?;
            (current_idx == session_idx).then_some(())
        })
        .is_some()
        && app.input.append_to_active_paste_block(&pasted);
    if appended {
        app.active_paste_session = Some(session);
        app.needs_redraw = true;
        tracing::debug!("paste_finalize: appended to active placeholder");
        return;
    }

    let char_count = input::count_text_chars(&pasted);
    if char_count > input::PASTE_PLACEHOLDER_CHAR_THRESHOLD {
        app.input.insert_paste_block(&pasted);
        let idx = app.input.lines().get(app.input.cursor_row()).and_then(|line| {
            input::parse_paste_placeholder_before_cursor(line, app.input.cursor_col())
        });
        app.active_paste_session =
            Some(state::PasteSessionState { placeholder_index: idx, ..session });
        tracing::debug!(char_count, placeholder_index = ?idx, "paste_finalize: inserted placeholder");
    } else {
        app.input.insert_str(&pasted);
        app.active_paste_session = None;
        tracing::debug!(
            char_count,
            lines = app.input.lines().len(),
            "paste_finalize: inserted inline text"
        );
    }
    app.needs_redraw = true;
}

fn cursor_gt(a: SelectionPoint, b: SelectionPoint) -> bool {
    a.row > b.row || (a.row == b.row && a.col > b.col)
}

fn cursor_to_byte_offset(lines: &[String], cursor: SelectionPoint) -> Option<usize> {
    let line = lines.get(cursor.row)?;
    let mut offset = 0usize;
    for prior in &lines[..cursor.row] {
        offset = offset.saturating_add(prior.len().saturating_add(1));
    }
    Some(offset.saturating_add(char_to_byte_index(line, cursor.col)))
}

fn char_to_byte_index(text: &str, char_idx: usize) -> usize {
    text.char_indices().nth(char_idx).map_or(text.len(), |(i, _)| i)
}

fn byte_offset_to_cursor(text: &str, byte_offset: usize) -> SelectionPoint {
    let mut row = 0usize;
    let mut col = 0usize;
    let mut seen = 0usize;
    for ch in text.chars() {
        let ch_len = ch.len_utf8();
        if seen + ch_len > byte_offset {
            break;
        }
        seen += ch_len;
        if ch == '\n' {
            row += 1;
            col = 0;
        } else {
            col += 1;
        }
    }
    SelectionPoint { row, col }
}

fn apply_merged_input_snapshot(app: &mut App, merged: &str, cursor_offset: usize) {
    let mut lines: Vec<String> = merged.split('\n').map(ToOwned::to_owned).collect();
    if lines.is_empty() {
        lines.push(String::new());
    }
    let mut cursor = byte_offset_to_cursor(merged, cursor_offset.min(merged.len()));
    if cursor.row >= lines.len() {
        cursor.row = lines.len().saturating_sub(1);
        cursor.col = lines[cursor.row].chars().count();
    } else {
        cursor.col = cursor.col.min(lines[cursor.row].chars().count());
    }

    app.input.replace_lines_and_cursor(lines, cursor.row, cursor.col);
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

fn strip_input_range(app: &mut App, start: SelectionPoint, end: SelectionPoint) {
    if cursor_gt(start, end) || start == end {
        return;
    }
    let Some(start_offset) = cursor_to_byte_offset(app.input.lines(), start) else {
        return;
    };
    let Some(end_offset) = cursor_to_byte_offset(app.input.lines(), end) else {
        return;
    };
    if start_offset >= end_offset {
        return;
    }
    let raw = app.input.lines().join("\n");
    if end_offset > raw.len() {
        return;
    }
    let mut merged = String::with_capacity(raw.len().saturating_sub(end_offset - start_offset));
    merged.push_str(&raw[..start_offset]);
    merged.push_str(&raw[end_offset..]);
    apply_merged_input_snapshot(app, &merged, start_offset);
}

/// Finalize a deferred Enter by restoring the exact pre-submit input snapshot
/// and submitting that original draft text.
fn finalize_deferred_submit(app: &mut App) {
    let Some(snapshot) = app.pending_submit.take() else {
        return;
    };
    app.input.restore_snapshot(snapshot);
    input_submit::submit_input(app);
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::agent::model;
    use crate::agent::wire::BridgeCommand;
    use crate::app::{MessageBlock, MessageRole};
    use crossterm::event::{Event, KeyCode, KeyEvent, KeyModifiers};

    fn app_with_connection()
    -> (App, tokio::sync::mpsc::UnboundedReceiver<crate::agent::wire::CommandEnvelope>) {
        let mut app = App::test_default();
        let (tx, rx) = tokio::sync::mpsc::unbounded_channel();
        app.conn = Some(std::rc::Rc::new(crate::agent::client::AgentConnection::new(tx)));
        app.session_id = Some(model::SessionId::new("session-1"));
        (app, rx)
    }

    #[test]
    fn pending_paste_chunks_are_merged_before_threshold_check() {
        let mut app = App::test_default();
        let first = "a".repeat(700);
        let second = "b".repeat(401);
        events::handle_terminal_event(&mut app, Event::Paste(first.clone()));
        events::handle_terminal_event(&mut app, Event::Paste(second.clone()));

        // Not applied until post-drain finalization.
        assert!(app.input.is_empty());
        assert!(!app.pending_paste_text.is_empty());

        finalize_pending_paste_event(&mut app);

        assert_eq!(app.input.lines(), vec!["[Pasted Text 1 - 1101 chars]"]);
        assert_eq!(app.input.text(), format!("{first}{second}"));
    }

    #[test]
    fn pending_paste_chunk_appends_to_same_session_placeholder() {
        let mut app = App::test_default();
        app.input.insert_paste_block("a\nb\nc\nd\ne\nf\ng\nh\ni\nj\nk");
        app.active_paste_session = Some(state::PasteSessionState {
            id: 7,
            start: SelectionPoint { row: 0, col: 0 },
            placeholder_index: Some(0),
        });
        app.pending_paste_session = app.active_paste_session;
        app.pending_paste_text = "\nl\nm".to_owned();

        finalize_pending_paste_event(&mut app);

        assert_eq!(app.input.lines(), vec!["[Pasted Text 1 - 25 chars]"]);
        assert_eq!(app.input.text(), "a\nb\nc\nd\ne\nf\ng\nh\ni\nj\nk\nl\nm");
    }

    #[test]
    fn pending_paste_exact_1000_chars_stays_inline() {
        let mut app = App::test_default();
        app.pending_paste_text = "x".repeat(1000);

        finalize_pending_paste_event(&mut app);

        assert_eq!(app.input.lines(), vec!["x".repeat(1000)]);
    }

    #[test]
    fn pending_paste_finalization_marks_redraw() {
        let mut app = App::test_default();
        app.needs_redraw = false;
        app.pending_paste_text = "hello\nworld".to_owned();

        finalize_pending_paste_event(&mut app);

        assert!(app.needs_redraw);
        assert_eq!(app.input.lines(), vec!["hello", "world"]);
    }

    #[test]
    fn suppressed_enter_preserves_multiline_inline_paste() {
        let mut app = App::test_default();
        let t0 = Instant::now();

        assert_eq!(app.paste_burst.on_char('a', t0), paste_burst::CharAction::Passthrough('a'));
        let _ = app.input.textarea_insert_char('a');
        assert_eq!(
            app.paste_burst.on_char('b', t0 + Duration::from_millis(2)),
            paste_burst::CharAction::Consumed
        );
        assert_eq!(
            app.paste_burst.on_char('c', t0 + Duration::from_millis(4)),
            paste_burst::CharAction::RetroCapture(1)
        );
        let _ = app.input.textarea_delete_char_before();

        let t_flush = t0 + Duration::from_millis(200);
        assert_eq!(
            app.paste_burst.tick(t_flush),
            Some(paste_burst::FlushAction::EmitPaste("abc".to_owned()))
        );
        app.queue_paste_text("abc");
        finalize_pending_paste_event(&mut app);
        assert_eq!(app.input.text(), "abc");

        let t_enter = t_flush + Duration::from_millis(10);
        assert!(app.paste_burst.on_enter(t_enter));
        assert_eq!(
            app.paste_burst.on_char('d', t_enter + Duration::from_millis(1)),
            paste_burst::CharAction::Consumed
        );
        assert_eq!(
            app.paste_burst.on_char('e', t_enter + Duration::from_millis(2)),
            paste_burst::CharAction::Consumed
        );
        assert_eq!(
            app.paste_burst.on_char('f', t_enter + Duration::from_millis(3)),
            paste_burst::CharAction::Consumed
        );

        let t_second_flush = t_enter + Duration::from_millis(200);
        assert_eq!(
            app.paste_burst.tick(t_second_flush),
            Some(paste_burst::FlushAction::EmitPaste("\ndef".to_owned()))
        );
        app.queue_paste_text("\ndef");
        finalize_pending_paste_event(&mut app);

        assert_eq!(app.input.lines(), vec!["abc", "def"]);
        assert_eq!(app.input.text(), "abc\ndef");
    }

    #[test]
    fn pending_paste_1001_chars_becomes_placeholder() {
        let mut app = App::test_default();
        app.pending_paste_text = "x".repeat(1001);

        finalize_pending_paste_event(&mut app);

        assert_eq!(app.input.lines(), vec!["[Pasted Text 1 - 1001 chars]"]);
        assert_eq!(app.input.text(), "x".repeat(1001));
    }

    #[test]
    fn pending_paste_session_isolation_prevents_unintended_append() {
        let mut app = App::test_default();
        app.pending_paste_text = "a".repeat(1001);
        finalize_pending_paste_event(&mut app);
        events::handle_terminal_event(
            &mut app,
            Event::Key(crossterm::event::KeyEvent::new(
                crossterm::event::KeyCode::Char('v'),
                crossterm::event::KeyModifiers::CONTROL,
            )),
        );

        app.pending_paste_text = "b".repeat(1001);
        finalize_pending_paste_event(&mut app);

        assert_eq!(
            app.input.lines(),
            vec!["[Pasted Text 1 - 1001 chars][Pasted Text 2 - 1001 chars]"]
        );
        assert_eq!(app.input.text(), format!("{}{}", "a".repeat(1001), "b".repeat(1001)));
    }

    #[test]
    fn plain_enter_preserves_single_line_draft_before_submit() {
        let (mut app, mut rx) = app_with_connection();
        app.input.set_text("hello world");
        let _ = app.input.set_cursor(0, "hello".chars().count());

        events::handle_terminal_event(
            &mut app,
            Event::Key(KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE)),
        );

        assert_eq!(app.input.text(), "hello world");
        assert_eq!(app.input.cursor(), (0, "hello".chars().count()));
        assert!(app.pending_submit.is_some());

        finalize_deferred_submit(&mut app);

        assert!(app.pending_submit.is_none());
        assert!(app.input.text().is_empty());
        assert_eq!(app.messages.len(), 2);
        assert!(matches!(app.messages[0].role, MessageRole::User));
        assert!(matches!(
            app.messages[0].blocks.as_slice(),
            [MessageBlock::Text(block)] if block.text == "hello world"
        ));
        let envelope = rx.try_recv().expect("prompt command should be sent");
        assert!(matches!(
            envelope.command,
            BridgeCommand::Prompt { session_id, .. } if session_id == "session-1"
        ));
    }

    #[test]
    fn plain_enter_preserves_multiline_draft_with_mid_buffer_cursor() {
        let (mut app, mut rx) = app_with_connection();
        app.input.set_text("alpha beta\ngamma");
        let _ = app.input.set_cursor(0, "alpha".chars().count());

        events::handle_terminal_event(
            &mut app,
            Event::Key(KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE)),
        );

        assert_eq!(app.input.text(), "alpha beta\ngamma");
        assert_eq!(app.input.cursor(), (0, "alpha".chars().count()));
        assert!(app.pending_submit.is_some());

        finalize_deferred_submit(&mut app);

        assert!(app.pending_submit.is_none());
        assert!(matches!(
            app.messages[0].blocks.as_slice(),
            [MessageBlock::Text(block)] if block.text == "alpha beta\ngamma"
        ));
        let envelope = rx.try_recv().expect("prompt command should be sent");
        assert!(matches!(
            envelope.command,
            BridgeCommand::Prompt { session_id, .. } if session_id == "session-1"
        ));
    }

    #[test]
    fn paste_event_cancels_deferred_submit_snapshot() {
        let mut app = App::test_default();
        app.input.set_text("draft");

        events::handle_terminal_event(
            &mut app,
            Event::Key(KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE)),
        );
        assert!(app.pending_submit.is_some());

        events::handle_terminal_event(&mut app, Event::Paste("pasted".into()));

        assert!(app.pending_submit.is_none());
        assert_eq!(app.pending_paste_text, "pasted");
        assert_eq!(app.input.text(), "draft");
    }

    #[test]
    fn esc_cancels_deferred_submit_snapshot_before_finalize() {
        let (mut app, mut rx) = app_with_connection();
        app.input.set_text("draft");

        events::handle_terminal_event(
            &mut app,
            Event::Key(KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE)),
        );
        assert!(app.pending_submit.is_some());

        events::handle_terminal_event(
            &mut app,
            Event::Key(KeyEvent::new(KeyCode::Esc, KeyModifiers::NONE)),
        );

        assert!(app.pending_submit.is_none());
        finalize_deferred_submit(&mut app);
        assert_eq!(app.input.text(), "draft");
        assert!(app.messages.is_empty());
        assert!(rx.try_recv().is_err(), "Esc should prevent deferred submit dispatch");
    }

    #[test]
    fn spinner_advances_less_frequently_when_reduced_motion_enabled() {
        let mut app = App::test_default();
        let base = Instant::now();

        advance_spinner_frame(&mut app, base);
        assert_eq!(app.spinner_frame, 1);
        advance_spinner_frame(&mut app, base + Duration::from_millis(40));
        assert_eq!(app.spinner_frame, 2);

        crate::app::config::store::set_prefers_reduced_motion(
            &mut app.config.committed_local_settings_document,
            true,
        );
        app.spinner_last_advance_at = None;
        app.spinner_frame = 0;

        advance_spinner_frame(&mut app, base);
        assert_eq!(app.spinner_frame, 1);
        advance_spinner_frame(&mut app, base + Duration::from_millis(95));
        assert_eq!(app.spinner_frame, 1);
        advance_spinner_frame(&mut app, base + Duration::from_millis(121));
        assert_eq!(app.spinner_frame, 2);
    }
}
