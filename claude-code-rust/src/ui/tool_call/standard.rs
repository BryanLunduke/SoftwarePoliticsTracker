// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

//! Rendering for non-Execute tool calls (Read, Write, Glob, etc.) and
//! content summary for collapsed tool calls.

use crate::agent::model;
use crate::app::ToolCallInfo;
use crate::ui::diff::{is_markdown_file, lang_from_title, render_diff, strip_outer_code_fence};
use crate::ui::highlight;
use crate::ui::markdown;
use crate::ui::theme;
use ratatui::style::{Modifier, Style};
use ratatui::text::{Line, Span};

use super::errors::{
    debug_failed_tool_render, extract_tool_use_error_message, failed_execute_first_line,
    looks_like_internal_error, render_internal_failure_content, render_tool_use_error_content,
};
use super::interactions::{render_permission_lines, render_question_lines};
use super::{markdown_inline_spans, status_icon, tool_output_badge_spans};

pub(super) const WRITE_DIFF_MAX_LINES: usize = 50;
pub(super) const WRITE_DIFF_HEAD_LINES: usize = 10;

/// Render just the title line for a non-Execute tool call (the line containing the spinner icon).
/// Used for in-progress tool calls where only the spinner changes each frame.
/// Execute tool calls are handled separately via `render_execute_with_borders`.
pub(super) fn render_tool_call_title(
    tc: &ToolCallInfo,
    _width: u16,
    spinner_frame: usize,
) -> Line<'static> {
    let (icon, icon_color) = status_icon(tc.status, spinner_frame);
    let (kind_icon, _kind_name) = theme::tool_name_label(&tc.sdk_tool_name);

    let mut title_spans = vec![
        Span::styled(format!("  {icon} "), Style::default().fg(icon_color)),
        Span::styled(
            format!("{kind_icon} "),
            Style::default().fg(ratatui::style::Color::White).add_modifier(Modifier::BOLD),
        ),
    ];

    title_spans.extend(markdown_inline_spans(&tc.title));
    title_spans.extend(tool_output_badge_spans(tc));

    Line::from(title_spans)
}

/// Render the body lines (everything after the title) for a non-Execute tool call.
/// Used for in-progress tool calls where the body is cached separately from the title.
/// Execute tool calls are handled separately via `render_execute_with_borders`.
pub(super) fn render_tool_call_body(tc: &ToolCallInfo) -> Vec<Line<'static>> {
    let mut lines = Vec::new();
    render_standard_body(tc, &mut lines);
    lines
}

#[must_use]
pub(super) fn tool_call_effectively_collapsed(tc: &ToolCallInfo, tools_collapsed: bool) -> bool {
    let has_permission = tc.pending_permission.is_some();
    let has_question = tc.pending_question.is_some();
    let has_diff = tc.content.iter().any(|c| matches!(c, model::ToolCallContent::Diff(_)));
    tools_collapsed && !has_diff && !has_permission && !has_question
}

pub(super) fn render_collapsed_tool_call_summary(
    tc: &ToolCallInfo,
    lines: &mut Vec<Line<'static>>,
) {
    let pipe_style = Style::default().fg(theme::DIM);
    lines.push(Line::from(vec![
        Span::styled("  \u{2514}\u{2500} ", pipe_style),
        Span::styled(content_summary(tc), Style::default().fg(theme::DIM)),
        Span::styled("  ctrl+o to expand", Style::default().fg(theme::DIM)),
    ]));
}

/// Render the body (everything after the title line) of a standard (non-Execute) tool call.
fn render_standard_body(tc: &ToolCallInfo, lines: &mut Vec<Line<'static>>) {
    let pipe_style = Style::default().fg(theme::DIM);
    let has_permission = tc.pending_permission.is_some();
    let has_question = tc.pending_question.is_some();

    if tc.content.is_empty() && !has_permission && !has_question {
        return;
    }

    // Expanded: render full content with | prefix on each line
    let mut content_lines = render_tool_content(tc);

    // Append inline permission controls if pending
    if let Some(ref perm) = tc.pending_permission {
        content_lines.extend(render_permission_lines(tc, perm));
    }
    if let Some(ref question) = tc.pending_question {
        content_lines.extend(render_question_lines(question));
    }

    let last_idx = content_lines.len().saturating_sub(1);
    for (i, content_line) in content_lines.into_iter().enumerate() {
        let prefix = if i == last_idx {
            "  \u{2514}\u{2500} " // corner
        } else {
            "  \u{2502}  " // pipe
        };
        let mut spans = vec![Span::styled(prefix.to_owned(), pipe_style)];
        spans.extend(content_line.spans);
        lines.push(Line::from(spans));
    }
}

/// One-line summary for collapsed tool calls.
pub(super) fn content_summary(tc: &ToolCallInfo) -> String {
    // For Execute tool calls, show last non-empty line of terminal output
    if tc.terminal_id.is_some() {
        if let Some(ref output) = tc.terminal_output {
            let stripped_output = highlight::strip_ansi(output);
            if matches!(tc.status, model::ToolCallStatus::Failed)
                && let Some(first_line) = failed_execute_first_line(&stripped_output)
            {
                return if first_line.chars().count() > 80 {
                    let truncated: String = first_line.chars().take(77).collect();
                    format!("{truncated}...")
                } else {
                    first_line
                };
            }
            let last = stripped_output.lines().rev().find(|l| !l.trim().is_empty());
            if let Some(line) = last {
                return if line.chars().count() > 80 {
                    let truncated: String = line.chars().take(77).collect();
                    format!("{truncated}...")
                } else {
                    line.to_owned()
                };
            }
        }
        return if matches!(tc.status, model::ToolCallStatus::InProgress) {
            "running...".to_owned()
        } else {
            String::new()
        };
    }

    for content in &tc.content {
        match content {
            model::ToolCallContent::Diff(diff) => {
                let name = diff.path.file_name().map_or_else(
                    || diff.path.to_string_lossy().into_owned(),
                    |f| f.to_string_lossy().into_owned(),
                );
                return name;
            }
            model::ToolCallContent::McpResource(resource) => {
                if let Some(path) = &resource.blob_saved_to {
                    return path.file_name().map_or_else(
                        || path.to_string_lossy().into_owned(),
                        |f| f.to_string_lossy().into_owned(),
                    );
                }
                if let Some(text) = resource.text.as_deref() {
                    let first = text.lines().find(|line| !line.trim().is_empty()).unwrap_or("");
                    if first.chars().count() > 60 {
                        let truncated: String = first.chars().take(57).collect();
                        return format!("{truncated}...");
                    }
                    return first.to_owned();
                }
                return resource.uri.clone();
            }
            model::ToolCallContent::Content(c) => {
                if let model::ContentBlock::Text(text) = &c.content {
                    let stripped = strip_outer_code_fence(&text.text);
                    if matches!(tc.status, model::ToolCallStatus::Failed)
                        && let Some(msg) = extract_tool_use_error_message(&stripped)
                    {
                        return msg;
                    }
                    let first = stripped.lines().next().unwrap_or("");
                    return if first.chars().count() > 60 {
                        let truncated: String = first.chars().take(57).collect();
                        format!("{truncated}...")
                    } else {
                        first.to_owned()
                    };
                }
            }
            model::ToolCallContent::Terminal(_) => {}
        }
    }
    String::new()
}

/// Render the full content of a tool call as lines.
fn render_tool_content(tc: &ToolCallInfo) -> Vec<Line<'static>> {
    let is_execute = tc.is_execute_tool();
    let mut lines: Vec<Line<'static>> = Vec::new();

    // For Execute tool calls with terminal output, render the live output
    if is_execute {
        if let Some(ref output) = tc.terminal_output {
            let stripped_output = highlight::strip_ansi(output);
            if matches!(tc.status, model::ToolCallStatus::Failed)
                && let Some(first_line) = failed_execute_first_line(&stripped_output)
            {
                lines.push(Line::from(Span::styled(
                    first_line,
                    Style::default().fg(theme::STATUS_ERROR),
                )));
            } else {
                lines.extend(highlight::render_terminal_output(&stripped_output));
            }
        } else if matches!(tc.status, model::ToolCallStatus::InProgress) {
            lines.push(Line::from(Span::styled("running...", Style::default().fg(theme::DIM))));
        }
        debug_failed_tool_render(tc);
        return lines;
    }

    for content in &tc.content {
        match content {
            model::ToolCallContent::Diff(diff) => {
                let raw = render_diff(diff);
                if tc.sdk_tool_name == "Write" && !is_plan_file_path(&diff.path) {
                    lines.extend(cap_write_diff_lines(raw));
                } else {
                    lines.extend(raw);
                }
            }
            model::ToolCallContent::McpResource(resource) => {
                lines.extend(render_mcp_resource_content(tc, resource));
            }
            model::ToolCallContent::Content(c) => {
                if let model::ContentBlock::Text(text) = &c.content {
                    render_text_content(tc, &text.text, &mut lines);
                }
            }
            model::ToolCallContent::Terminal(_) => {}
        }
    }

    debug_failed_tool_render(tc);
    lines
}

fn render_text_content(tc: &ToolCallInfo, text: &str, lines: &mut Vec<Line<'static>>) {
    let stripped = strip_outer_code_fence(text);
    if matches!(tc.status, model::ToolCallStatus::Failed)
        && let Some(msg) = extract_tool_use_error_message(&stripped)
    {
        lines.extend(render_tool_use_error_content(&msg));
        return;
    }
    if matches!(tc.status, model::ToolCallStatus::Failed) && looks_like_internal_error(&stripped) {
        lines.extend(render_internal_failure_content(&stripped));
        return;
    }
    let md_source = if is_markdown_file(&tc.title) {
        stripped
    } else {
        let lang = lang_from_title(&tc.title);
        lines.extend(highlight::highlight_code(
            &stripped,
            (!lang.is_empty()).then_some(lang.as_str()),
        ));
        return;
    };
    for line in markdown::render_markdown_safe(&md_source, None) {
        let owned: Vec<Span<'static>> =
            line.spans.into_iter().map(|s| Span::styled(s.content.into_owned(), s.style)).collect();
        lines.push(Line::from(owned));
    }
}

fn render_mcp_resource_content(
    tc: &ToolCallInfo,
    resource: &model::McpResource,
) -> Vec<Line<'static>> {
    let mut lines = Vec::new();
    if let Some(text) = resource.text.as_deref() {
        render_text_content(tc, text, &mut lines);
    }
    if let Some(blob_saved_to) = &resource.blob_saved_to {
        let saved_path = blob_saved_to.to_string_lossy().into_owned();
        let text_mentions_path =
            resource.text.as_deref().is_some_and(|text| text.contains(saved_path.as_str()));
        if !text_mentions_path {
            lines.push(Line::from(vec![
                Span::styled(
                    "Saved to: ",
                    Style::default().fg(theme::DIM).add_modifier(Modifier::BOLD),
                ),
                Span::styled(saved_path, Style::default().fg(theme::DIM)),
            ]));
        }
    }
    if lines.is_empty() {
        lines.push(Line::from(Span::styled(resource.uri.clone(), Style::default().fg(theme::DIM))));
    }
    lines
}

/// Returns `true` for paths inside `.claude/plans/` (cross-platform).
/// Write diffs for these files are never capped so the full plan is always visible.
fn is_plan_file_path(path: &std::path::Path) -> bool {
    path.components()
        .zip(path.components().skip(1))
        .any(|(a, b)| a.as_os_str() == ".claude" && b.as_os_str() == "plans")
}

pub(super) fn cap_write_diff_lines(lines: Vec<Line<'static>>) -> Vec<Line<'static>> {
    if lines.len() <= WRITE_DIFF_MAX_LINES {
        return lines;
    }
    let total = lines.len();
    let separator_lines = 3usize; // blank + marker + blank
    let head = WRITE_DIFF_HEAD_LINES.min(WRITE_DIFF_MAX_LINES.saturating_sub(separator_lines));
    let tail = WRITE_DIFF_MAX_LINES.saturating_sub(head + separator_lines);
    let tail_start = total.saturating_sub(tail);
    let omitted = tail_start.saturating_sub(head);

    let mut out = Vec::with_capacity(WRITE_DIFF_MAX_LINES);
    out.extend(lines.iter().take(head).cloned());
    out.push(Line::default());
    out.push(Line::from(Span::styled(
        format!("... {omitted} diff lines omitted ..."),
        Style::default().fg(theme::DIM).add_modifier(Modifier::ITALIC),
    )));
    out.push(Line::default());
    out.extend(lines.iter().skip(tail_start).cloned());
    out
}
