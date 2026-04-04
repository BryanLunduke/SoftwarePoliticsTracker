// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

//! Slash command autocomplete navigation: activate, deactivate, sync,
//! move selection, and confirm.

use super::candidates::build_slash_state;
use super::{MAX_VISIBLE, SlashContext};
use crate::app::{App, FocusTarget};

pub fn activate(app: &mut App) {
    let Some(state) = build_slash_state(app) else {
        return;
    };

    app.slash = Some(state);
    app.mention = None;
    app.subagent = None;
    app.claim_focus_target(FocusTarget::Mention);
}

pub fn update_query(app: &mut App) {
    let Some(next_state) = build_slash_state(app) else {
        deactivate(app);
        return;
    };

    if let Some(ref mut slash) = app.slash {
        let keep_selection = slash.context == next_state.context;
        let dialog = if keep_selection { slash.dialog } else { super::DialogState::default() };
        slash.trigger_row = next_state.trigger_row;
        slash.trigger_col = next_state.trigger_col;
        slash.query = next_state.query;
        slash.context = next_state.context;
        slash.candidates = next_state.candidates;
        slash.dialog = dialog;
        slash.dialog.clamp(slash.candidates.len(), MAX_VISIBLE);
    } else {
        app.slash = Some(next_state);
        app.claim_focus_target(FocusTarget::Mention);
    }
}

pub fn sync_with_cursor(app: &mut App) {
    match (build_slash_state(app), app.slash.is_some()) {
        (Some(_), true) => update_query(app),
        (Some(_), false) => activate(app),
        (None, true) => deactivate(app),
        (None, false) => {}
    }
}

pub fn deactivate(app: &mut App) {
    app.slash = None;
    if app.mention.is_none() && app.subagent.is_none() {
        app.release_focus_target(FocusTarget::Mention);
    }
}

pub fn move_up(app: &mut App) {
    if let Some(ref mut slash) = app.slash {
        slash.dialog.move_up(slash.candidates.len(), MAX_VISIBLE);
    }
}

pub fn move_down(app: &mut App) {
    if let Some(ref mut slash) = app.slash {
        slash.dialog.move_down(slash.candidates.len(), MAX_VISIBLE);
    }
}

/// Confirm selected candidate in input.
pub fn confirm_selection(app: &mut App) {
    let Some(slash) = app.slash.take() else {
        return;
    };

    let Some(candidate) = slash.candidates.get(slash.dialog.selected) else {
        if app.mention.is_none() && app.subagent.is_none() {
            app.release_focus_target(FocusTarget::Mention);
        }
        return;
    };

    let mut lines = app.input.lines().to_vec();
    let Some(line) = lines.get(slash.trigger_row) else {
        tracing::debug!(
            trigger_row = slash.trigger_row,
            line_count = app.input.lines().len(),
            "Slash confirm aborted: trigger row out of bounds"
        );
        if app.mention.is_none() && app.subagent.is_none() {
            app.release_focus_target(FocusTarget::Mention);
        }
        return;
    };

    let chars: Vec<char> = line.chars().collect();
    let (replace_start, replace_end) = match slash.context {
        SlashContext::CommandName => {
            if slash.trigger_col >= chars.len() {
                tracing::debug!(
                    trigger_col = slash.trigger_col,
                    line_len = chars.len(),
                    "Slash confirm aborted: trigger column out of bounds"
                );
                if app.mention.is_none() && app.subagent.is_none() {
                    app.release_focus_target(FocusTarget::Mention);
                }
                return;
            }
            if chars[slash.trigger_col] != '/' {
                tracing::debug!(
                    trigger_col = slash.trigger_col,
                    found = ?chars[slash.trigger_col],
                    "Slash confirm aborted: trigger column is not slash"
                );
                if app.mention.is_none() && app.subagent.is_none() {
                    app.release_focus_target(FocusTarget::Mention);
                }
                return;
            }

            let token_end = (slash.trigger_col + 1..chars.len())
                .find(|&i| chars[i].is_whitespace())
                .unwrap_or(chars.len());
            (slash.trigger_col, token_end)
        }
        SlashContext::Argument { token_range, .. } => {
            let (start, end) = token_range;
            if start > end || end > chars.len() {
                tracing::debug!(
                    start,
                    end,
                    line_len = chars.len(),
                    "Slash confirm aborted: invalid argument token range"
                );
                if app.mention.is_none() && app.subagent.is_none() {
                    app.release_focus_target(FocusTarget::Mention);
                }
                return;
            }
            (start, end)
        }
    };

    let before: String = chars[..replace_start].iter().collect();
    let after: String = chars[replace_end..].iter().collect();
    let replacement = if after.is_empty() {
        format!("{} ", candidate.insert_value)
    } else {
        candidate.insert_value.clone()
    };
    let new_line = format!("{before}{replacement}{after}");
    let new_cursor_col = replace_start + replacement.chars().count();
    let new_line_len = new_line.chars().count();
    if new_cursor_col > new_line_len {
        tracing::warn!(
            cursor_col = new_cursor_col,
            line_len = new_line_len,
            "Slash confirm produced cursor beyond line length; clamping"
        );
    }
    lines[slash.trigger_row] = new_line;
    app.input.replace_lines_and_cursor(lines, slash.trigger_row, new_cursor_col.min(new_line_len));

    sync_with_cursor(app);
    if app.slash.is_none() && app.mention.is_none() && app.subagent.is_none() {
        app.release_focus_target(FocusTarget::Mention);
    }
}
