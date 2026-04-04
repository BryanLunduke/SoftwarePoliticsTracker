// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::dialog::DialogState;
use super::paste_burst::CharAction;
use super::{
    App, AppStatus, CancelOrigin, FocusOwner, FocusTarget, HelpView, InvalidationLevel, ModeInfo,
    ModeState,
};
use crate::app::inline_interactions::handle_inline_interaction_key;
use crate::app::selection::{clear_selection, selection_text_from_rendered_lines};
use crate::app::state::AutocompleteKind;
use crate::app::{mention, slash, subagent};
use crossterm::event::{KeyCode, KeyEvent, KeyModifiers};
#[cfg(test)]
use std::cell::Cell;
use std::rc::Rc;
use std::time::Instant;

const HELP_TAB_PREV_KEY: KeyCode = KeyCode::Left;
const HELP_TAB_NEXT_KEY: KeyCode = KeyCode::Right;

fn is_ctrl_shortcut(modifiers: KeyModifiers) -> bool {
    modifiers.contains(KeyModifiers::CONTROL) && !modifiers.contains(KeyModifiers::ALT)
}

fn is_ctrl_char_shortcut(key: KeyEvent, expected: char) -> bool {
    is_ctrl_shortcut(key.modifiers)
        && matches!(key.code, KeyCode::Char(c) if c.eq_ignore_ascii_case(&expected))
}

fn is_permission_ctrl_shortcut(key: KeyEvent) -> bool {
    is_ctrl_char_shortcut(key, 'y')
        || is_ctrl_char_shortcut(key, 'a')
        || is_ctrl_char_shortcut(key, 'n')
}

fn handle_always_allowed_shortcuts(app: &mut App, key: KeyEvent) -> bool {
    if is_ctrl_char_shortcut(key, 'q') {
        app.should_quit = true;
        return true;
    }
    if is_ctrl_char_shortcut(key, 'c') {
        match copy_selection_to_clipboard(app) {
            ClipboardCopyResult::Copied => {
                clear_selection(app);
                return true;
            }
            ClipboardCopyResult::Failed => {
                return true;
            }
            ClipboardCopyResult::NoText => {}
        }
        app.should_quit = true;
        return true;
    }
    false
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum ClipboardCopyResult {
    Copied,
    Failed,
    NoText,
}

fn copy_selection_to_clipboard(app: &mut App) -> ClipboardCopyResult {
    let Some(selected_text) = selection_text_for_copy(app) else {
        return ClipboardCopyResult::NoText;
    };

    write_text_to_clipboard(selected_text)
}

fn write_text_to_clipboard(selected_text: String) -> ClipboardCopyResult {
    #[cfg(test)]
    {
        match TEST_CLIPBOARD_MODE.with(Cell::get) {
            TestClipboardMode::Succeed => return ClipboardCopyResult::Copied,
            TestClipboardMode::Fail => return ClipboardCopyResult::Failed,
            TestClipboardMode::System => {}
        }
    }

    let Ok(mut clipboard) = arboard::Clipboard::new() else {
        tracing::warn!("failed to access clipboard while copying selection");
        return ClipboardCopyResult::Failed;
    };

    if clipboard.set_text(selected_text).is_ok() {
        ClipboardCopyResult::Copied
    } else {
        tracing::warn!("failed to write selection text to clipboard");
        ClipboardCopyResult::Failed
    }
}

fn selection_text_for_copy(app: &mut App) -> Option<String> {
    let selection = app.selection?;
    crate::ui::refresh_selection_snapshot(app);
    let lines = match selection.kind {
        super::SelectionKind::Chat => &app.rendered_chat_lines,
        super::SelectionKind::Input => &app.rendered_input_lines,
    };
    let selected_text = selection_text_from_rendered_lines(lines, selection);
    (!selected_text.is_empty()).then_some(selected_text)
}

#[cfg(test)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum TestClipboardMode {
    System,
    Succeed,
    Fail,
}

#[cfg(test)]
thread_local! {
    static TEST_CLIPBOARD_MODE: Cell<TestClipboardMode> = const { Cell::new(TestClipboardMode::System) };
}

#[cfg(test)]
pub(crate) struct TestClipboardGuard {
    previous: TestClipboardMode,
}

#[cfg(test)]
impl Drop for TestClipboardGuard {
    fn drop(&mut self) {
        TEST_CLIPBOARD_MODE.with(|mode| mode.set(self.previous));
    }
}

#[cfg(test)]
pub(crate) fn override_test_clipboard(mode: TestClipboardMode) -> TestClipboardGuard {
    let previous = TEST_CLIPBOARD_MODE.with(|current| {
        let previous = current.get();
        current.set(mode);
        previous
    });
    TestClipboardGuard { previous }
}

pub(super) fn dispatch_key_by_focus(app: &mut App, key: KeyEvent) -> bool {
    if handle_always_allowed_shortcuts(app, key) {
        return true;
    }

    if matches!(app.status, AppStatus::Connecting | AppStatus::CommandPending | AppStatus::Error)
        || app.is_compacting
    {
        return handle_blocked_input_shortcuts(app, key);
    }

    sync_help_focus(app);

    if handle_global_shortcuts(app, key) {
        return true;
    }

    match app.focus_owner() {
        FocusOwner::Mention => handle_autocomplete_key(app, key),
        FocusOwner::Help => handle_help_key(app, key),
        FocusOwner::Permission => {
            if handle_inline_interaction_key(app, key) {
                true
            } else {
                handle_normal_key(app, key)
            }
        }
        FocusOwner::Input | FocusOwner::TodoList => handle_normal_key(app, key),
    }
}

/// During blocked-input states (Connecting, `CommandPending`, Error), keep input disabled and only allow
/// navigation/help shortcuts.
fn handle_blocked_input_shortcuts(app: &mut App, key: KeyEvent) -> bool {
    if is_ctrl_char_shortcut(key, 'u') && app.update_check_hint.is_some() {
        app.update_check_hint = None;
        sync_help_focus(app);
        return true;
    }

    if is_ctrl_char_shortcut(key, 'l') {
        app.force_redraw = true;
        sync_help_focus(app);
        return true;
    }

    let changed = match (key.code, key.modifiers) {
        (KeyCode::Char('?'), m) if !m.intersects(KeyModifiers::CONTROL | KeyModifiers::ALT) => {
            if app.is_help_active() {
                app.help_open = false;
                app.input.clear();
            } else {
                app.help_open = true;
                app.input.set_text("?");
            }
            true
        }
        (HELP_TAB_PREV_KEY, m) if m == KeyModifiers::NONE && app.is_help_active() => {
            set_help_view(app, prev_help_view(app.help_view));
            true
        }
        (HELP_TAB_NEXT_KEY, m) if m == KeyModifiers::NONE && app.is_help_active() => {
            set_help_view(app, next_help_view(app.help_view));
            true
        }
        (KeyCode::Up, m) if m == KeyModifiers::NONE || m == KeyModifiers::CONTROL => {
            app.viewport.scroll_up(1);
            true
        }
        (KeyCode::Down, m) if m == KeyModifiers::NONE || m == KeyModifiers::CONTROL => {
            app.viewport.scroll_down(1);
            true
        }
        _ => false,
    };

    sync_help_focus(app);
    changed
}

/// Handle shortcuts that should work regardless of current focus owner.
fn handle_global_shortcuts(app: &mut App, key: KeyEvent) -> bool {
    // Session-only dismiss for update hint.
    if is_ctrl_char_shortcut(key, 'u') && app.update_check_hint.is_some() {
        app.update_check_hint = None;
        return true;
    }

    // Permission quick shortcuts are global when permissions are pending.
    if !app.pending_interaction_ids.is_empty() && is_permission_ctrl_shortcut(key) {
        return handle_inline_interaction_key(app, key);
    }

    match (key.code, key.modifiers) {
        (KeyCode::Char('t'), m) if m == KeyModifiers::CONTROL => {
            toggle_todo_panel_focus(app);
            true
        }
        (KeyCode::Char('o'), m) if m == KeyModifiers::CONTROL => {
            toggle_all_tool_calls(app);
            true
        }
        (KeyCode::Char('l'), m) if m == KeyModifiers::CONTROL => {
            app.force_redraw = true;
            true
        }
        (KeyCode::Up, m) if m == KeyModifiers::CONTROL => {
            app.viewport.scroll_up(1);
            true
        }
        (KeyCode::Down, m) if m == KeyModifiers::CONTROL => {
            app.viewport.scroll_down(1);
            true
        }
        _ => false,
    }
}

#[inline]
pub(super) fn is_printable_text_modifiers(modifiers: KeyModifiers) -> bool {
    let ctrl_alt =
        modifiers.contains(KeyModifiers::CONTROL) && modifiers.contains(KeyModifiers::ALT);
    !modifiers.intersects(KeyModifiers::CONTROL | KeyModifiers::ALT) || ctrl_alt
}

pub(super) fn handle_normal_key(app: &mut App, key: KeyEvent) -> bool {
    sync_help_focus(app);
    let input_version_before = app.input.version;

    if should_ignore_key_during_paste(app, key) {
        return false;
    }

    let changed = handle_normal_key_actions(app, key);

    if app.input.version != input_version_before {
        sync_help_open_after_input_change(app);
    }

    if app.input.version != input_version_before && should_sync_autocomplete_after_key(app, key) {
        mention::sync_with_cursor(app);
        slash::sync_with_cursor(app);
        subagent::sync_with_cursor(app);
    }

    sync_help_focus(app);
    changed
}

fn should_ignore_key_during_paste(app: &mut App, key: KeyEvent) -> bool {
    if app.pending_submit.is_some() && is_editing_like_key(key) {
        app.pending_submit = None;
    }
    !app.pending_paste_text.is_empty() && is_editing_like_key(key)
}

fn is_editing_like_key(key: KeyEvent) -> bool {
    matches!(
        key.code,
        KeyCode::Char(_) | KeyCode::Enter | KeyCode::Tab | KeyCode::Backspace | KeyCode::Delete
    )
}

fn handle_normal_key_actions(app: &mut App, key: KeyEvent) -> bool {
    if handle_turn_control_key(app, key) {
        return true;
    }
    if handle_submit_key(app, key) {
        return true;
    }
    if handle_history_key(app, key) {
        return true;
    }
    if handle_navigation_key(app, key) {
        return true;
    }
    if handle_focus_toggle_key(app, key) {
        return true;
    }
    if handle_mode_cycle_key(app, key) {
        return true;
    }
    if handle_editing_key(app, key) {
        return true;
    }
    handle_printable_key(app, key)
}

fn handle_turn_control_key(app: &mut App, key: KeyEvent) -> bool {
    if !matches!(key.code, KeyCode::Esc) {
        return false;
    }
    app.pending_submit = None;
    if app.focus_owner() == FocusOwner::TodoList {
        app.release_focus_target(FocusTarget::TodoList);
        return true;
    }
    if matches!(app.status, AppStatus::Thinking | AppStatus::Running)
        && let Err(message) = super::input_submit::request_cancel(app, CancelOrigin::Manual)
    {
        tracing::error!("Failed to send cancel: {message}");
    }
    true
}

fn handle_submit_key(app: &mut App, key: KeyEvent) -> bool {
    if !matches!(key.code, KeyCode::Enter) || app.focus_owner() == FocusOwner::TodoList {
        return false;
    }

    let now = Instant::now();

    // During an active burst or the post-burst suppression window, Enter
    // becomes a newline to keep multi-line pastes grouped.
    if app.paste_burst.on_enter(now) {
        tracing::debug!("paste_enter: enter routed through paste buffer");
        return true;
    }

    if !key.modifiers.contains(KeyModifiers::SHIFT)
        && !key.modifiers.contains(KeyModifiers::CONTROL)
    {
        app.pending_submit = Some(app.input.snapshot());
        tracing::debug!("paste_enter: armed deferred submit snapshot");
        return false;
    }
    app.pending_submit = None;
    tracing::debug!("paste_enter: inserted explicit newline");
    app.input.textarea_insert_newline()
}

fn handle_history_key(app: &mut App, key: KeyEvent) -> bool {
    if app.focus_owner() == FocusOwner::TodoList {
        return false;
    }
    match (key.code, key.modifiers) {
        (KeyCode::Char('z'), m) if m == KeyModifiers::CONTROL => app.input.textarea_undo(),
        (KeyCode::Char('y'), m) if m == KeyModifiers::CONTROL => app.input.textarea_redo(),
        _ => false,
    }
}

fn handle_navigation_key(app: &mut App, key: KeyEvent) -> bool {
    match (key.code, key.modifiers) {
        (KeyCode::Left, m)
            if app.focus_owner() != FocusOwner::TodoList
                && m.contains(KeyModifiers::CONTROL)
                && !m.contains(KeyModifiers::ALT) =>
        {
            app.input.textarea_move_word_left()
        }
        (KeyCode::Right, m)
            if app.focus_owner() != FocusOwner::TodoList
                && m.contains(KeyModifiers::CONTROL)
                && !m.contains(KeyModifiers::ALT) =>
        {
            app.input.textarea_move_word_right()
        }
        (KeyCode::Left, _) if app.focus_owner() != FocusOwner::TodoList => {
            app.input.textarea_move_left()
        }
        (KeyCode::Right, _) if app.focus_owner() != FocusOwner::TodoList => {
            app.input.textarea_move_right()
        }
        (KeyCode::Up, _) if app.focus_owner() == FocusOwner::TodoList => {
            move_todo_selection_up(app);
            true
        }
        (KeyCode::Down, _) if app.focus_owner() == FocusOwner::TodoList => {
            move_todo_selection_down(app);
            true
        }
        (KeyCode::Up, _) => {
            if !try_move_input_cursor_up(app) {
                app.viewport.scroll_up(1);
            }
            true
        }
        (KeyCode::Down, _) => {
            if !try_move_input_cursor_down(app) {
                app.viewport.scroll_down(1);
            }
            true
        }
        (KeyCode::Home, _) if app.focus_owner() != FocusOwner::TodoList => {
            app.input.textarea_move_home()
        }
        (KeyCode::End, _) if app.focus_owner() != FocusOwner::TodoList => {
            app.input.textarea_move_end()
        }
        _ => false,
    }
}

fn handle_focus_toggle_key(app: &mut App, key: KeyEvent) -> bool {
    match (key.code, key.modifiers) {
        (KeyCode::Tab, m)
            if !m.contains(KeyModifiers::SHIFT)
                && !m.contains(KeyModifiers::CONTROL)
                && !m.contains(KeyModifiers::ALT)
                && app.show_todo_panel
                && !app.todos.is_empty() =>
        {
            if app.focus_owner() == FocusOwner::TodoList {
                app.release_focus_target(FocusTarget::TodoList);
            } else {
                app.claim_focus_target(FocusTarget::TodoList);
            }
            true
        }
        _ => false,
    }
}

fn handle_mode_cycle_key(app: &mut App, key: KeyEvent) -> bool {
    if !matches!(key.code, KeyCode::BackTab) {
        return false;
    }
    let Some(ref mode) = app.mode else {
        return true;
    };
    if mode.available_modes.len() <= 1 {
        return true;
    }

    let current_idx =
        mode.available_modes.iter().position(|m| m.id == mode.current_mode_id).unwrap_or(0);
    let next_idx = (current_idx + 1) % mode.available_modes.len();
    let next = &mode.available_modes[next_idx];

    if let Some(ref conn) = app.conn
        && let Some(sid) = app.session_id.clone()
    {
        let mode_id = next.id.clone();
        let conn = Rc::clone(conn);
        tokio::task::spawn_local(async move {
            if let Err(e) = conn.set_mode(sid.to_string(), mode_id) {
                tracing::error!("Failed to set mode: {e}");
            }
        });
    }

    let next_id = next.id.clone();
    let next_name = next.name.clone();
    let modes = mode
        .available_modes
        .iter()
        .map(|m| ModeInfo { id: m.id.clone(), name: m.name.clone() })
        .collect();
    app.mode = Some(ModeState {
        current_mode_id: next_id,
        current_mode_name: next_name,
        available_modes: modes,
    });
    true
}

fn handle_editing_key(app: &mut App, key: KeyEvent) -> bool {
    match (key.code, key.modifiers) {
        (KeyCode::Backspace, m)
            if app.focus_owner() != FocusOwner::TodoList
                && m.contains(KeyModifiers::CONTROL)
                && !m.contains(KeyModifiers::ALT) =>
        {
            app.input.textarea_delete_word_before()
        }
        (KeyCode::Delete, m)
            if app.focus_owner() != FocusOwner::TodoList
                && m.contains(KeyModifiers::CONTROL)
                && !m.contains(KeyModifiers::ALT) =>
        {
            app.input.textarea_delete_word_after()
        }
        (KeyCode::Backspace, _) if app.focus_owner() != FocusOwner::TodoList => {
            app.input.textarea_delete_char_before()
        }
        (KeyCode::Delete, _) if app.focus_owner() != FocusOwner::TodoList => {
            app.input.textarea_delete_char_after()
        }
        _ => false,
    }
}

fn handle_printable_key(app: &mut App, key: KeyEvent) -> bool {
    let (KeyCode::Char(c), m) = (key.code, key.modifiers) else {
        // Non-char key: reset burst state to prevent leakage.
        app.paste_burst.on_non_char_key(Instant::now());
        return false;
    };
    if !is_printable_text_modifiers(m) {
        return false;
    }
    if app.focus_owner() == FocusOwner::TodoList {
        app.release_focus_target(FocusTarget::TodoList);
    }

    let now = Instant::now();
    match app.paste_burst.on_char(c, now) {
        CharAction::Consumed => {
            // Character absorbed into burst buffer. Don't insert.
            tracing::debug!(ch = %c.escape_default(), "paste_key: consumed char into burst");
            return false;
        }
        CharAction::RetroCapture(delete_count) => {
            // Burst confirmation retro-captured already-inserted leading chars.
            for _ in 0..delete_count {
                let _ = app.input.textarea_delete_char_before();
            }
            tracing::debug!(
                ch = %c.escape_default(),
                delete_count,
                "paste_key: retro-captured leaked chars"
            );
            return true;
        }
        CharAction::Passthrough(ch) => {
            // Normal typing or a previously-held char released.
            // If `ch == c`, single normal insert. Otherwise the detector
            // emitted a held char; insert it first, then the current char.
            tracing::debug!(
                input = %c.escape_default(),
                emitted = %ch.escape_default(),
                "paste_key: passthrough"
            );
            if ch == c {
                let _ = app.input.textarea_insert_char(c);
            } else {
                let _ = app.input.textarea_insert_char(ch);
                let _ = app.input.textarea_insert_char(c);
            }
        }
    }

    if c == '?' && app.input.text().trim() == "?" {
        app.help_open = true;
    }

    if c == '@' {
        mention::activate(app);
    } else if c == '/' {
        slash::activate(app);
    } else if c == '&' {
        subagent::activate(app);
    }
    true
}

fn sync_help_open_after_input_change(app: &mut App) {
    if app.is_help_active() && app.input.text().trim() != "?" {
        app.help_open = false;
    }
}

fn try_move_input_cursor_up(app: &mut App) -> bool {
    let before = (app.input.cursor_row(), app.input.cursor_col());
    let _ = app.input.textarea_move_up();
    (app.input.cursor_row(), app.input.cursor_col()) != before
}

fn try_move_input_cursor_down(app: &mut App) -> bool {
    let before = (app.input.cursor_row(), app.input.cursor_col());
    let _ = app.input.textarea_move_down();
    (app.input.cursor_row(), app.input.cursor_col()) != before
}

fn should_sync_autocomplete_after_key(app: &App, key: KeyEvent) -> bool {
    if app.focus_owner() == FocusOwner::TodoList {
        return false;
    }

    match (key.code, key.modifiers) {
        (
            KeyCode::Up
            | KeyCode::Down
            | KeyCode::Left
            | KeyCode::Right
            | KeyCode::Home
            | KeyCode::End
            | KeyCode::Backspace
            | KeyCode::Delete
            | KeyCode::Enter,
            _,
        ) => true,
        (KeyCode::Char('z' | 'y'), m) if m == KeyModifiers::CONTROL => true,
        (KeyCode::Char(_), m) if is_printable_text_modifiers(m) => true,
        _ => false,
    }
}

pub(super) fn toggle_todo_panel_focus(app: &mut App) {
    if app.todos.is_empty() {
        app.show_todo_panel = false;
        app.release_focus_target(FocusTarget::TodoList);
        app.todo_scroll = 0;
        app.todo_selected = 0;
        return;
    }

    app.show_todo_panel = !app.show_todo_panel;
    if app.show_todo_panel {
        // Start at in-progress todo when available; fallback to first item.
        app.todo_selected =
            app.todos.iter().position(|t| t.status == super::TodoStatus::InProgress).unwrap_or(0);
        app.claim_focus_target(FocusTarget::TodoList);
    } else {
        app.release_focus_target(FocusTarget::TodoList);
    }
}

pub(super) fn move_todo_selection_up(app: &mut App) {
    if app.todos.is_empty() || !app.show_todo_panel {
        app.release_focus_target(FocusTarget::TodoList);
        return;
    }
    app.todo_selected = app.todo_selected.saturating_sub(1);
}

pub(super) fn move_todo_selection_down(app: &mut App) {
    if app.todos.is_empty() || !app.show_todo_panel {
        app.release_focus_target(FocusTarget::TodoList);
        return;
    }
    let max = app.todos.len().saturating_sub(1);
    if app.todo_selected < max {
        app.todo_selected += 1;
    }
}

/// Handle keystrokes while mention/slash autocomplete dropdown is active.
pub(super) fn handle_autocomplete_key(app: &mut App, key: KeyEvent) -> bool {
    match app.active_autocomplete_kind() {
        Some(AutocompleteKind::Mention) => return handle_mention_key(app, key),
        Some(AutocompleteKind::Slash) => return handle_slash_key(app, key),
        Some(AutocompleteKind::Subagent) => return handle_subagent_key(app, key),
        None => {}
    }
    dispatch_key_by_focus(app, key)
}

fn handle_help_key(app: &mut App, key: KeyEvent) -> bool {
    match (key.code, key.modifiers) {
        (HELP_TAB_PREV_KEY, m) if m == KeyModifiers::NONE => {
            set_help_view(app, prev_help_view(app.help_view));
            true
        }
        (HELP_TAB_NEXT_KEY, m) if m == KeyModifiers::NONE => {
            set_help_view(app, next_help_view(app.help_view));
            true
        }
        (KeyCode::Up, m) if m == KeyModifiers::NONE => {
            if matches!(app.help_view, HelpView::SlashCommands | HelpView::Subagents) {
                let count = crate::ui::help::help_item_count(app);
                app.help_dialog.move_up(count, app.help_visible_count);
            }
            true
        }
        (KeyCode::Down, m) if m == KeyModifiers::NONE => {
            if matches!(app.help_view, HelpView::SlashCommands | HelpView::Subagents) {
                let count = crate::ui::help::help_item_count(app);
                app.help_dialog.move_down(count, app.help_visible_count);
            }
            true
        }
        _ => handle_normal_key(app, key),
    }
}

const fn next_help_view(current: HelpView) -> HelpView {
    match current {
        HelpView::Keys => HelpView::SlashCommands,
        HelpView::SlashCommands => HelpView::Subagents,
        HelpView::Subagents => HelpView::Keys,
    }
}

const fn prev_help_view(current: HelpView) -> HelpView {
    match current {
        HelpView::Keys => HelpView::Subagents,
        HelpView::SlashCommands => HelpView::Keys,
        HelpView::Subagents => HelpView::SlashCommands,
    }
}

fn set_help_view(app: &mut App, next: HelpView) {
    if app.help_view != next {
        tracing::debug!(from = ?app.help_view, to = ?next, "Help view changed via keyboard");
        app.help_view = next;
        app.help_dialog = DialogState::default();
    }
}

fn sync_help_focus(app: &mut App) {
    if app.is_help_active()
        && app.pending_interaction_ids.is_empty()
        && !app.autocomplete_focus_available()
    {
        app.claim_focus_target(FocusTarget::Help);
    } else {
        app.release_focus_target(FocusTarget::Help);
    }
}

/// Handle keystrokes while the `@` mention autocomplete dropdown is active.
pub(super) fn handle_mention_key(app: &mut App, key: KeyEvent) -> bool {
    match (key.code, key.modifiers) {
        (KeyCode::Up, _) => {
            mention::move_up(app);
            true
        }
        (KeyCode::Down, _) => {
            mention::move_down(app);
            true
        }
        (KeyCode::Enter | KeyCode::Tab, _) => {
            mention::confirm_selection(app);
            true
        }
        (KeyCode::Esc, _) => {
            mention::deactivate(app);
            true
        }
        (KeyCode::Backspace, _) => {
            let changed = app.input.textarea_delete_char_before();
            mention::update_query(app);
            changed
        }
        (KeyCode::Char(c), m) if is_printable_text_modifiers(m) => {
            let changed = app.input.textarea_insert_char(c);
            if c.is_whitespace() {
                mention::deactivate(app);
            } else {
                mention::update_query(app);
            }
            changed
        }
        // Any other key: deactivate mention and forward to normal handling
        _ => {
            mention::deactivate(app);
            dispatch_key_by_focus(app, key)
        }
    }
}

/// Handle keystrokes while slash autocomplete dropdown is active.
fn handle_slash_key(app: &mut App, key: KeyEvent) -> bool {
    match (key.code, key.modifiers) {
        (KeyCode::Up, _) => {
            slash::move_up(app);
            true
        }
        (KeyCode::Down, _) => {
            slash::move_down(app);
            true
        }
        (KeyCode::Enter | KeyCode::Tab, _) => {
            slash::confirm_selection(app);
            true
        }
        (KeyCode::Esc, _) => {
            slash::deactivate(app);
            true
        }
        (KeyCode::Backspace, _) => {
            let changed = app.input.textarea_delete_char_before();
            slash::update_query(app);
            changed
        }
        (KeyCode::Char(c), m) if is_printable_text_modifiers(m) => {
            let changed = app.input.textarea_insert_char(c);
            slash::update_query(app);
            changed
        }
        _ => {
            slash::deactivate(app);
            dispatch_key_by_focus(app, key)
        }
    }
}

/// Handle keystrokes while `&` subagent autocomplete dropdown is active.
fn handle_subagent_key(app: &mut App, key: KeyEvent) -> bool {
    match (key.code, key.modifiers) {
        (KeyCode::Up, _) => {
            subagent::move_up(app);
            true
        }
        (KeyCode::Down, _) => {
            subagent::move_down(app);
            true
        }
        (KeyCode::Enter | KeyCode::Tab, _) => {
            subagent::confirm_selection(app);
            true
        }
        (KeyCode::Esc, _) => {
            subagent::deactivate(app);
            true
        }
        (KeyCode::Backspace, _) => {
            let changed = app.input.textarea_delete_char_before();
            subagent::update_query(app);
            changed
        }
        (KeyCode::Char(c), m) if is_printable_text_modifiers(m) => {
            let changed = app.input.textarea_insert_char(c);
            subagent::update_query(app);
            changed
        }
        _ => {
            subagent::deactivate(app);
            dispatch_key_by_focus(app, key)
        }
    }
}

/// Toggle the session-level collapsed preference for non-Execute tool calls.
pub(super) fn toggle_all_tool_calls(app: &mut App) {
    app.tools_collapsed = !app.tools_collapsed;
    app.invalidate_layout(InvalidationLevel::Global);
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::app::{
        ChatMessage, MessageBlock, MessageRole, SelectionKind, SelectionPoint, SelectionState,
        TextBlock,
    };
    use crossterm::event::{KeyCode, KeyModifiers};
    use ratatui::layout::Rect;
    use std::time::{Duration, Instant};

    #[test]
    fn queued_paste_still_blocks_overlapping_key_text() {
        let mut app = App::test_default();
        app.pending_paste_text = "clipboard".to_owned();

        let blocked = should_ignore_key_during_paste(
            &mut app,
            KeyEvent::new(KeyCode::Char('x'), KeyModifiers::NONE),
        );
        assert!(blocked);
    }

    #[test]
    fn burst_active_does_not_block_followup_chars() {
        let mut app = App::test_default();
        let t0 = Instant::now();

        assert_eq!(app.paste_burst.on_char('a', t0), CharAction::Passthrough('a'));
        assert_eq!(
            app.paste_burst.on_char('b', t0 + Duration::from_millis(1)),
            CharAction::Consumed
        );
        assert!(app.paste_burst.is_buffering());

        let blocked = should_ignore_key_during_paste(
            &mut app,
            KeyEvent::new(KeyCode::Char('c'), KeyModifiers::NONE),
        );
        assert!(!blocked);
    }

    #[test]
    fn selection_text_for_copy_refreshes_chat_snapshot_before_redraw() {
        let mut app = App::test_default();
        app.status = AppStatus::Running;
        app.messages.push(ChatMessage {
            role: MessageRole::Assistant,
            blocks: vec![MessageBlock::Text(TextBlock::from_complete("hello"))],
            usage: None,
        });
        app.bind_active_turn_assistant(0);
        app.rendered_chat_area = Rect::new(0, 0, 20, 6);
        app.rendered_chat_lines = vec!["hello".to_owned()];
        app.selection = Some(SelectionState {
            kind: SelectionKind::Chat,
            start: SelectionPoint { row: 0, col: 0 },
            end: SelectionPoint { row: 0, col: 11 },
            dragging: false,
        });

        if let Some(MessageBlock::Text(block)) =
            app.messages.get_mut(0).and_then(|message| message.blocks.get_mut(0))
        {
            block.text.push_str(" world");
            block.markdown.append(" world");
            block.cache.invalidate();
        }
        app.invalidate_layout(InvalidationLevel::MessageChanged(0));

        assert!(selection_text_for_copy(&mut app).is_some());
        assert!(app.rendered_chat_lines.iter().any(|line| line.contains("world")));
    }

    #[test]
    fn selection_text_for_copy_refreshes_input_snapshot_before_redraw() {
        let mut app = App::test_default();
        app.input.set_text("hello");
        app.rendered_input_area = Rect::new(0, 0, 20, 4);
        app.rendered_input_lines = vec!["hello".to_owned()];
        app.selection = Some(SelectionState {
            kind: SelectionKind::Input,
            start: SelectionPoint { row: 0, col: 0 },
            end: SelectionPoint { row: 0, col: 11 },
            dragging: false,
        });

        app.input.set_text("hello world");

        assert_eq!(selection_text_for_copy(&mut app), Some("hello world".to_owned()));
    }
}
