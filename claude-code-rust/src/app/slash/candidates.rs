// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

//! Slash command candidate detection, filtering, and building.

use super::{
    MAX_CANDIDATES, SlashCandidate, SlashContext, SlashDetection, SlashState, normalize_slash_name,
};
use crate::app::App;
use crate::app::dialog::DialogState;
use std::time::{SystemTime, UNIX_EPOCH};

pub(super) fn detect_argument_at_cursor(
    chars: &[char],
    mut idx: usize,
    cursor_col: usize,
) -> Option<(usize, usize, usize)> {
    if cursor_col > chars.len() {
        return None;
    }

    let mut arg_index = 0usize;
    loop {
        while idx < chars.len() && chars[idx].is_whitespace() {
            if cursor_col == idx {
                return Some((arg_index, cursor_col, cursor_col));
            }
            idx += 1;
        }

        if idx >= chars.len() {
            if cursor_col >= idx {
                return Some((arg_index, cursor_col, cursor_col));
            }
            return None;
        }

        let token_start = idx;
        while idx < chars.len() && !chars[idx].is_whitespace() {
            idx += 1;
        }
        let token_end = idx;
        if (token_start..=token_end).contains(&cursor_col) {
            return Some((arg_index, token_start, token_end));
        }
        arg_index += 1;
    }
}

pub(super) fn detect_slash_at_cursor(
    lines: &[String],
    cursor_row: usize,
    cursor_col: usize,
) -> Option<SlashDetection> {
    let line = lines.get(cursor_row)?;
    let first_non_ws = line.find(|c: char| !c.is_whitespace())?;
    let chars: Vec<char> = line.chars().collect();
    if chars.get(first_non_ws).copied() != Some('/') {
        return None;
    }

    let token_start = first_non_ws;
    let token_end =
        (token_start + 1..chars.len()).find(|&i| chars[i].is_whitespace()).unwrap_or(chars.len());

    if cursor_col <= token_start || cursor_col > chars.len() {
        return None;
    }

    if cursor_col <= token_end {
        let query: String = chars[token_start + 1..cursor_col].iter().collect();
        if query.chars().any(char::is_whitespace) {
            return None;
        }
        return Some(SlashDetection {
            trigger_row: cursor_row,
            trigger_col: token_start,
            query,
            context: SlashContext::CommandName,
        });
    }

    let command: String = chars[token_start..token_end].iter().collect();
    let (arg_index, token_start, token_end) =
        detect_argument_at_cursor(&chars, token_end, cursor_col)?;
    let query: String = chars[token_start..cursor_col.min(token_end)].iter().collect();

    Some(SlashDetection {
        trigger_row: cursor_row,
        trigger_col: token_start,
        query,
        context: SlashContext::Argument {
            command,
            arg_index,
            token_range: (token_start, token_end),
        },
    })
}

fn advertised_commands(app: &App) -> Vec<String> {
    app.available_commands.iter().map(|cmd| normalize_slash_name(&cmd.name)).collect()
}

pub(super) fn find_advertised_command<'a>(
    app: &'a App,
    command_name: &str,
) -> Option<&'a crate::agent::model::AvailableCommand> {
    app.available_commands.iter().find(|cmd| normalize_slash_name(&cmd.name) == command_name)
}

fn is_builtin_variable_input_command(command_name: &str) -> bool {
    matches!(command_name, "/mode" | "/model" | "/resume")
}

pub(super) fn is_variable_input_command(app: &App, command_name: &str) -> bool {
    if is_builtin_variable_input_command(command_name) {
        return true;
    }

    find_advertised_command(app, command_name)
        .and_then(|cmd| cmd.input_hint.as_ref())
        .is_some_and(|hint| !hint.trim().is_empty())
}

pub(super) fn supported_command_candidates(app: &App) -> Vec<SlashCandidate> {
    use std::collections::BTreeMap;

    let mut by_name: BTreeMap<String, String> = BTreeMap::new();
    by_name.insert("/cancel".into(), "Cancel active turn".into());
    by_name.insert("/compact".into(), "Compact session context".into());
    by_name.insert("/config".into(), "Open settings".into());
    by_name.insert("/login".into(), "Authenticate with Claude".into());
    by_name.insert("/logout".into(), "Sign out of Claude".into());
    by_name.insert("/mcp".into(), "Open MCP".into());
    by_name.insert("/mode".into(), "Set session mode".into());
    by_name.insert("/model".into(), "Set session model".into());
    by_name.insert("/new-session".into(), "Start a fresh session".into());
    by_name.insert("/resume".into(), "Resume a session by ID".into());
    by_name.insert("/plugins".into(), "Open plugins".into());
    by_name.insert("/status".into(), "Show session status".into());
    by_name.insert("/usage".into(), "Open usage".into());

    for cmd in &app.available_commands {
        let name = normalize_slash_name(&cmd.name);
        by_name.entry(name).or_insert_with(|| cmd.description.clone());
    }

    by_name
        .into_iter()
        .map(|(name, description)| SlashCandidate {
            insert_value: name.clone(),
            primary: name,
            secondary: if description.trim().is_empty() { None } else { Some(description) },
        })
        .collect()
}

pub(super) fn filter_command_candidates(
    candidates: &[SlashCandidate],
    query: &str,
) -> Vec<SlashCandidate> {
    if query.is_empty() {
        return candidates.iter().take(MAX_CANDIDATES).cloned().collect();
    }

    let query_lower = query.to_lowercase();
    candidates
        .iter()
        .filter(|candidate| {
            let body = candidate.primary.strip_prefix('/').unwrap_or(&candidate.primary);
            body.to_lowercase().contains(&query_lower)
        })
        .take(MAX_CANDIDATES)
        .cloned()
        .collect()
}

fn candidate_matches(candidate: &SlashCandidate, query_lower: &str) -> bool {
    candidate.primary.to_lowercase().contains(query_lower)
        || candidate.insert_value.to_lowercase().contains(query_lower)
        || candidate
            .secondary
            .as_ref()
            .is_some_and(|secondary| secondary.to_lowercase().contains(query_lower))
}

pub(super) fn filter_argument_candidates(
    candidates: &[SlashCandidate],
    query: &str,
) -> Vec<SlashCandidate> {
    if query.is_empty() {
        return candidates.iter().take(MAX_CANDIDATES).cloned().collect();
    }

    let query_lower = query.to_lowercase();
    candidates
        .iter()
        .filter(|candidate| candidate_matches(candidate, &query_lower))
        .take(MAX_CANDIDATES)
        .cloned()
        .collect()
}

fn now_epoch_seconds() -> i64 {
    match SystemTime::now().duration_since(UNIX_EPOCH) {
        Ok(duration) => i64::try_from(duration.as_secs()).unwrap_or(i64::MAX),
        Err(_) => 0,
    }
}

fn format_relative_age(epoch_seconds: i64) -> String {
    let now_seconds = now_epoch_seconds();
    let delta_seconds = if now_seconds >= epoch_seconds {
        now_seconds - epoch_seconds
    } else {
        epoch_seconds - now_seconds
    };

    if delta_seconds < 5 * 60 {
        return "<5m".to_owned();
    }
    if delta_seconds < 60 * 60 {
        return format!("{}m", delta_seconds / 60);
    }
    if delta_seconds < 24 * 60 * 60 {
        return format!("{}h", delta_seconds / (60 * 60));
    }

    let total_hours = delta_seconds / (60 * 60);
    let days = total_hours / 24;
    let hours = total_hours % 24;
    format!("{days}d {hours}h")
}

fn session_age_label(last_modified_ms: Option<u64>) -> String {
    let Some(last_modified_ms) = last_modified_ms else {
        return "--".to_owned();
    };
    let epoch = i64::try_from(last_modified_ms / 1_000).ok();
    let Some(epoch) = epoch else {
        return "--".to_owned();
    };
    format_relative_age(epoch)
}

pub(super) fn argument_candidates(
    app: &App,
    command_name: &str,
    arg_index: usize,
) -> Vec<SlashCandidate> {
    if arg_index > 0 {
        return Vec::new();
    }

    match command_name {
        "/resume" => app
            .recent_sessions
            .iter()
            .map(|session| {
                let summary = session.summary.trim();
                let summary = if summary.is_empty() { "(no summary)" } else { summary };
                let age = session_age_label(Some(session.last_modified_ms));
                SlashCandidate {
                    insert_value: session.session_id.clone(),
                    primary: format!("{age} - {summary}"),
                    secondary: Some(session.session_id.clone()),
                }
            })
            .collect(),
        "/mode" => app
            .mode
            .as_ref()
            .map(|mode| {
                mode.available_modes
                    .iter()
                    .map(|entry| SlashCandidate {
                        insert_value: entry.id.clone(),
                        primary: entry.name.clone(),
                        secondary: Some(entry.id.clone()),
                    })
                    .collect()
            })
            .unwrap_or_default(),
        "/model" => app
            .available_models
            .iter()
            .map(|model| SlashCandidate {
                insert_value: model.id.clone(),
                primary: model.display_name.clone(),
                secondary: model
                    .description
                    .clone()
                    .or_else(|| (model.display_name != model.id).then(|| model.id.clone())),
            })
            .collect(),
        _ => Vec::new(),
    }
}

pub(super) fn build_slash_state(app: &App) -> Option<SlashState> {
    let detection =
        detect_slash_at_cursor(app.input.lines(), app.input.cursor_row(), app.input.cursor_col())?;

    let candidates = match &detection.context {
        SlashContext::CommandName => {
            filter_command_candidates(&supported_command_candidates(app), &detection.query)
        }
        SlashContext::Argument { command, arg_index, .. } => {
            if !is_variable_input_command(app, command) {
                return None;
            }
            filter_argument_candidates(
                &argument_candidates(app, command, *arg_index),
                &detection.query,
            )
        }
    };
    if candidates.is_empty() {
        return None;
    }

    Some(SlashState {
        trigger_row: detection.trigger_row,
        trigger_col: detection.trigger_col,
        query: detection.query,
        context: detection.context,
        candidates,
        dialog: DialogState::default(),
    })
}

pub fn is_supported_command(app: &App, command_name: &str) -> bool {
    matches!(
        command_name,
        "/cancel"
            | "/compact"
            | "/config"
            | "/mcp"
            | "/mode"
            | "/model"
            | "/new-session"
            | "/resume"
            | "/plugins"
            | "/status"
            | "/usage"
    ) || advertised_commands(app).iter().any(|c| c == command_name)
}
