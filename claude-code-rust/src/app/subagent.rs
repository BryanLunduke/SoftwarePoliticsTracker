// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::{App, FocusTarget, dialog::DialogState};

/// Maximum candidates shown in the dropdown.
pub const MAX_VISIBLE: usize = 8;
const MAX_CANDIDATES: usize = 50;

#[derive(Debug, Clone)]
pub struct SubagentCandidate {
    pub name: String,
    pub description: String,
    pub model: Option<String>,
}

#[derive(Debug, Clone)]
pub struct SubagentState {
    /// Character position where the `&` token starts.
    pub trigger_row: usize,
    pub trigger_col: usize,
    /// Current query text after `&`.
    pub query: String,
    /// Filtered subagent candidates.
    pub candidates: Vec<SubagentCandidate>,
    /// Shared autocomplete dialog navigation state.
    pub dialog: DialogState,
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct SubagentDetection {
    trigger_row: usize,
    trigger_col: usize,
    query: String,
}

fn is_subagent_char(ch: char) -> bool {
    ch.is_ascii_alphanumeric() || ch == '_' || ch == '-'
}

fn detect_subagent_at_cursor(
    lines: &[String],
    cursor_row: usize,
    cursor_col: usize,
) -> Option<SubagentDetection> {
    let line = lines.get(cursor_row)?;
    let chars: Vec<char> = line.chars().collect();
    if cursor_col > chars.len() {
        return None;
    }

    let mut token_start = cursor_col;
    while token_start > 0 && !chars[token_start - 1].is_whitespace() {
        token_start -= 1;
    }
    if token_start >= chars.len() || chars[token_start] != '&' {
        return None;
    }

    // Reject shell-style logical and operators (`&&`).
    if chars.get(token_start + 1).copied() == Some('&') {
        return None;
    }

    let token_end =
        (token_start + 1..chars.len()).find(|&i| chars[i].is_whitespace()).unwrap_or(chars.len());
    if cursor_col <= token_start || cursor_col > token_end {
        return None;
    }

    // Eager activation: allow a bare `&` only when it is at token end (`... &` with no
    // trailing chars). This avoids triggering on spacing/operator patterns like `& `.
    if cursor_col == token_start + 1 {
        if token_end == chars.len() {
            return Some(SubagentDetection {
                trigger_row: cursor_row,
                trigger_col: token_start,
                query: String::new(),
            });
        }
        return None;
    }

    // First character after '&' must be alphabetic to start a valid subagent token.
    if !chars[token_start + 1].is_ascii_alphabetic() {
        return None;
    }

    if chars[token_start + 1..token_end].iter().any(|ch| !is_subagent_char(*ch)) {
        return None;
    }
    let query: String = chars[token_start + 1..cursor_col].iter().collect();

    Some(SubagentDetection { trigger_row: cursor_row, trigger_col: token_start, query })
}

fn candidate_matches(candidate: &SubagentCandidate, query_lower: &str) -> bool {
    candidate.name.to_lowercase().contains(query_lower)
        || candidate.description.to_lowercase().contains(query_lower)
        || candidate.model.as_ref().is_some_and(|model| model.to_lowercase().contains(query_lower))
}

fn filter_candidates(
    candidates: &[crate::agent::model::AvailableAgent],
    query: &str,
) -> Vec<SubagentCandidate> {
    let query_lower = query.to_lowercase();
    candidates
        .iter()
        .filter(|agent| !agent.name.trim().is_empty())
        .map(|agent| SubagentCandidate {
            name: agent.name.clone(),
            description: agent.description.clone(),
            model: agent.model.clone(),
        })
        .filter(|candidate| candidate_matches(candidate, &query_lower))
        .take(MAX_CANDIDATES)
        .collect()
}

fn build_subagent_state(app: &App) -> Option<SubagentState> {
    let detection = detect_subagent_at_cursor(
        app.input.lines(),
        app.input.cursor_row(),
        app.input.cursor_col(),
    )?;
    let candidates = filter_candidates(&app.available_agents, &detection.query);
    if candidates.is_empty() {
        return None;
    }
    Some(SubagentState {
        trigger_row: detection.trigger_row,
        trigger_col: detection.trigger_col,
        query: detection.query,
        candidates,
        dialog: DialogState::default(),
    })
}

pub fn activate(app: &mut App) {
    let Some(state) = build_subagent_state(app) else {
        return;
    };
    app.subagent = Some(state);
    app.mention = None;
    app.slash = None;
    app.claim_focus_target(FocusTarget::Mention);
}

pub fn update_query(app: &mut App) {
    let Some(next_state) = build_subagent_state(app) else {
        deactivate(app);
        return;
    };

    if let Some(ref mut subagent) = app.subagent {
        subagent.trigger_row = next_state.trigger_row;
        subagent.trigger_col = next_state.trigger_col;
        subagent.query = next_state.query;
        subagent.candidates = next_state.candidates;
        subagent.dialog.clamp(subagent.candidates.len(), MAX_VISIBLE);
    } else {
        app.subagent = Some(next_state);
        app.claim_focus_target(FocusTarget::Mention);
    }
}

pub fn sync_with_cursor(app: &mut App) {
    match (build_subagent_state(app), app.subagent.is_some()) {
        (Some(_), true) => update_query(app),
        (Some(_), false) => activate(app),
        (None, true) => deactivate(app),
        (None, false) => {}
    }
}

pub fn deactivate(app: &mut App) {
    app.subagent = None;
    if app.mention.is_none() && app.slash.is_none() {
        app.release_focus_target(FocusTarget::Mention);
    }
}

pub fn move_up(app: &mut App) {
    if let Some(ref mut subagent) = app.subagent {
        subagent.dialog.move_up(subagent.candidates.len(), MAX_VISIBLE);
    }
}

pub fn move_down(app: &mut App) {
    if let Some(ref mut subagent) = app.subagent {
        subagent.dialog.move_down(subagent.candidates.len(), MAX_VISIBLE);
    }
}

pub fn confirm_selection(app: &mut App) {
    let Some(subagent) = app.subagent.take() else {
        return;
    };

    let Some(candidate) = subagent.candidates.get(subagent.dialog.selected) else {
        if app.mention.is_none() && app.slash.is_none() {
            app.release_focus_target(FocusTarget::Mention);
        }
        return;
    };

    let mut lines = app.input.lines().to_vec();
    let Some(line) = lines.get(subagent.trigger_row) else {
        if app.mention.is_none() && app.slash.is_none() {
            app.release_focus_target(FocusTarget::Mention);
        }
        return;
    };

    let chars: Vec<char> = line.chars().collect();
    if subagent.trigger_col >= chars.len() || chars[subagent.trigger_col] != '&' {
        if app.mention.is_none() && app.slash.is_none() {
            app.release_focus_target(FocusTarget::Mention);
        }
        return;
    }

    let token_end = (subagent.trigger_col + 1..chars.len())
        .find(|&i| chars[i].is_whitespace())
        .unwrap_or(chars.len());
    let before: String = chars[..subagent.trigger_col].iter().collect();
    let after: String = chars[token_end..].iter().collect();
    let replacement = if after.is_empty() {
        format!("&{} ", candidate.name)
    } else {
        format!("&{}", candidate.name)
    };
    let new_line = format!("{before}{replacement}{after}");
    let new_cursor_col = subagent.trigger_col + replacement.chars().count();
    let new_line_len = new_line.chars().count();
    lines[subagent.trigger_row] = new_line;
    app.input.replace_lines_and_cursor(
        lines,
        subagent.trigger_row,
        new_cursor_col.min(new_line_len),
    );

    sync_with_cursor(app);
    if app.mention.is_none() && app.slash.is_none() && app.subagent.is_none() {
        app.release_focus_target(FocusTarget::Mention);
    }
}

pub fn find_subagent_spans(text: &str) -> Vec<(usize, usize, String)> {
    let mut spans = Vec::new();
    let chars: Vec<char> = text.chars().collect();
    let mut i = 0;

    while i < chars.len() {
        if chars[i] != '&' {
            i += 1;
            continue;
        }
        if i > 0 && !chars[i - 1].is_whitespace() {
            i += 1;
            continue;
        }
        if i + 1 >= chars.len() || !chars[i + 1].is_ascii_alphabetic() {
            i += 1;
            continue;
        }
        if chars[i + 1] == '&' {
            i += 1;
            continue;
        }

        let mut end = i + 1;
        while end < chars.len() && !chars[end].is_whitespace() {
            if !is_subagent_char(chars[end]) {
                break;
            }
            end += 1;
        }
        if end <= i + 1 {
            i += 1;
            continue;
        }
        if end < chars.len() && !chars[end].is_whitespace() {
            i = end + 1;
            continue;
        }

        let byte_start: usize = chars[..i].iter().map(|c| c.len_utf8()).sum();
        let byte_end: usize = chars[..end].iter().map(|c| c.len_utf8()).sum();
        let name: String = chars[i + 1..end].iter().collect();
        spans.push((byte_start, byte_end, name));
        i = end;
    }

    spans
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::app::App;

    #[test]
    fn detect_subagent_requires_alpha_after_ampersand() {
        let lines = vec!["& review".to_owned()];
        assert!(detect_subagent_at_cursor(&lines, 0, 1).is_none());
    }

    #[test]
    fn detect_subagent_rejects_double_ampersand() {
        let lines = vec!["cmd && build".to_owned()];
        assert!(detect_subagent_at_cursor(&lines, 0, 6).is_none());
    }

    #[test]
    fn sync_with_cursor_activates_when_subagent_token_is_valid() {
        let mut app = App::test_default();
        app.available_agents = vec![
            crate::agent::model::AvailableAgent::new("reviewer", "Review code"),
            crate::agent::model::AvailableAgent::new("explore", "Explore codebase"),
        ];
        app.input.set_text("&re");
        let _ = app.input.set_cursor_col(3);

        sync_with_cursor(&mut app);

        let state = app.subagent.as_ref().expect("subagent state should be active");
        assert_eq!(state.query, "re");
        assert!(!state.candidates.is_empty());
    }

    #[test]
    fn sync_with_cursor_activates_on_bare_ampersand_at_line_end() {
        let mut app = App::test_default();
        app.available_agents =
            vec![crate::agent::model::AvailableAgent::new("reviewer", "Review code")];
        app.input.set_text("&");

        sync_with_cursor(&mut app);

        let state = app.subagent.as_ref().expect("subagent state should be active");
        assert_eq!(state.query, "");
        assert!(!state.candidates.is_empty());
    }

    #[test]
    fn find_subagent_spans_ignores_double_ampersand() {
        let spans = find_subagent_spans("run && wait &reviewer");
        assert_eq!(spans.len(), 1);
        assert_eq!(spans[0].2, "reviewer");
    }
}
