// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

//! Error rendering and tool-use error extraction for failed tool calls.

use crate::agent::error_handling::{
    looks_like_internal_error as shared_looks_like_internal_error,
    summarize_internal_error as shared_summarize_internal_error,
};
use crate::agent::model;
use crate::app::ToolCallInfo;
use crate::ui::theme;
use ratatui::style::{Modifier, Style};
use ratatui::text::{Line, Span};

pub(super) fn render_internal_failure_content(payload: &str) -> Vec<Line<'static>> {
    let summary = summarize_internal_error(payload);
    let mut lines = vec![Line::from(Span::styled(
        "Internal Agent SDK error",
        Style::default().fg(theme::STATUS_ERROR).add_modifier(Modifier::BOLD),
    ))];
    if !summary.is_empty() {
        lines.push(Line::from(Span::styled(summary, Style::default().fg(theme::STATUS_ERROR))));
    }
    lines
}

pub(super) fn render_tool_use_error_content(message: &str) -> Vec<Line<'static>> {
    message
        .lines()
        .filter(|line| !line.trim().is_empty())
        .map(|line| {
            Line::from(Span::styled(line.to_owned(), Style::default().fg(theme::STATUS_ERROR)))
        })
        .collect()
}

pub(super) fn debug_failed_tool_render(tc: &ToolCallInfo) {
    if !matches!(tc.status, model::ToolCallStatus::Failed) {
        return;
    }

    let Some(text_payload) = tc.content.iter().find_map(|content| match content {
        model::ToolCallContent::Content(c) => match &c.content {
            model::ContentBlock::Text(t) => Some(t.text.as_str().to_owned()),
            model::ContentBlock::Image(_) => None,
        },
        _ => None,
    }) else {
        return;
    };
    if !looks_like_internal_error(&text_payload) {
        return;
    }
    let text_preview = summarize_internal_error(&text_payload);

    let terminal_preview = tc
        .terminal_output
        .as_deref()
        .map_or_else(|| "<no terminal output>".to_owned(), preview_for_log);

    tracing::debug!(
        tool_call_id = %tc.id,
        title = %tc.title,
        sdk_tool_name = %tc.sdk_tool_name,
        content_blocks = tc.content.len(),
        text_preview = %text_preview,
        terminal_preview = %terminal_preview,
        "Failed tool call render payload"
    );
}

fn preview_for_log(input: &str) -> String {
    const LIMIT: usize = 240;
    let mut out = String::new();
    for (i, ch) in input.chars().enumerate() {
        if i >= LIMIT {
            out.push_str("...");
            break;
        }
        out.push(ch);
    }
    out.replace('\n', "\\n")
}

pub(super) fn failed_execute_first_line(output: &str) -> Option<String> {
    if let Some(msg) = extract_tool_use_error_message(output) {
        return Some(msg);
    }
    output.lines().find(|line| !line.trim().is_empty()).map(str::trim).map(str::to_owned)
}

pub(super) fn looks_like_internal_error(input: &str) -> bool {
    shared_looks_like_internal_error(input)
}

pub(super) fn extract_tool_use_error_message(input: &str) -> Option<String> {
    extract_xml_tag_value(input, "tool_use_error")
        .map(str::trim)
        .filter(|s| !s.is_empty())
        .map(str::to_owned)
}

pub(super) fn summarize_internal_error(input: &str) -> String {
    shared_summarize_internal_error(input)
}

fn extract_xml_tag_value<'a>(input: &'a str, tag: &str) -> Option<&'a str> {
    let lower = input.to_ascii_lowercase();
    let open = format!("<{tag}>");
    let close = format!("</{tag}>");
    let start = lower.find(&open)? + open.len();
    let end = start + lower[start..].find(&close)?;
    let value = input[start..end].trim();
    (!value.is_empty()).then_some(value)
}
