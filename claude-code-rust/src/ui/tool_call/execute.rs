// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

//! Two-layer rendering for Execute/Bash tool calls: content is cached
//! (width-independent), borders are applied at render time.

use crate::agent::model;
use crate::app::ToolCallInfo;
use crate::ui::highlight;
use crate::ui::theme;
use ratatui::style::{Color, Modifier, Style};
use ratatui::text::{Line, Span};

use super::errors::failed_execute_first_line;
use super::interactions::{render_permission_lines, render_question_lines};
use super::{
    markdown_inline_spans, spans_width, status_icon, tool_output_badge_spans,
    truncate_spans_to_width,
};

/// Max visible output lines for Execute/Bash tool calls.
/// Total box height = 1 (title) + 1 (command) + this + 1 (bottom border) = 15.
pub(super) const TERMINAL_MAX_LINES: usize = 12;

/// Render Execute/Bash content lines WITHOUT any border decoration.
/// This is width-independent and safe to cache across resizes.
/// Returns: command line + output lines + permission lines (no border prefixes).
pub(super) fn render_execute_content(tc: &ToolCallInfo) -> Vec<Line<'static>> {
    let mut lines: Vec<Line<'static>> = Vec::new();

    // Command line (no border prefix)
    if let Some(ref cmd) = tc.terminal_command {
        let mut spans = vec![Span::styled(
            "$ ",
            Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD),
        )];
        let mut command_spans = highlight::highlight_shell_command(cmd);
        if command_spans.is_empty() {
            command_spans.push(Span::styled(cmd.clone(), Style::default().fg(Color::Yellow)));
        }
        spans.extend(command_spans);
        lines.push(Line::from(spans));
    }

    // Output lines (capped, no border prefix)
    let mut body_lines: Vec<Line<'static>> = Vec::new();

    if let Some(ref output) = tc.terminal_output {
        let stripped_output = highlight::strip_ansi(output);
        if matches!(tc.status, model::ToolCallStatus::Failed)
            && let Some(first_line) = failed_execute_first_line(&stripped_output)
        {
            body_lines.push(Line::from(Span::styled(
                first_line,
                Style::default().fg(theme::STATUS_ERROR),
            )));
        } else {
            let raw_lines = highlight::render_terminal_output(&stripped_output);

            let total = raw_lines.len();
            if total > TERMINAL_MAX_LINES {
                let skipped = total - TERMINAL_MAX_LINES;
                body_lines.push(Line::from(Span::styled(
                    format!("... {skipped} lines hidden ..."),
                    Style::default().fg(theme::DIM),
                )));
                body_lines.extend(raw_lines.into_iter().skip(skipped));
            } else {
                body_lines = raw_lines;
            }
        }
    } else if matches!(tc.status, model::ToolCallStatus::InProgress) {
        body_lines.push(Line::from(Span::styled("running...", Style::default().fg(theme::DIM))));
    }

    lines.extend(body_lines);

    // Inline permission controls (no border prefix)
    if let Some(ref perm) = tc.pending_permission {
        lines.extend(render_permission_lines(tc, perm));
    }
    if let Some(ref question) = tc.pending_question {
        lines.extend(render_question_lines(question));
    }

    lines
}

/// Apply Execute/Bash box borders around pre-rendered content lines.
/// This is called at render time with the current width, so borders always
/// fill the terminal correctly even after resize.
pub(super) fn render_execute_with_borders(
    tc: &ToolCallInfo,
    content: &[Line<'static>],
    width: u16,
    spinner_frame: usize,
) -> Vec<Line<'static>> {
    let border = Style::default().fg(theme::DIM);
    let inner_w = (width as usize).saturating_sub(2);
    let mut out = Vec::with_capacity(content.len() + 2);

    // Top border with status icon and title
    let (status_icon_str, icon_color) = status_icon(tc.status, spinner_frame);
    let (_tool_icon, tool_label) = theme::tool_name_label(&tc.sdk_tool_name);
    let line_budget = width as usize;
    let left_prefix = vec![
        Span::styled("  \u{256D}\u{2500}", border),
        Span::styled(format!(" {status_icon_str} "), Style::default().fg(icon_color)),
        Span::styled(
            format!("{tool_label} "),
            Style::default().fg(Color::White).add_modifier(Modifier::BOLD),
        ),
    ];
    let badge_spans = tool_output_badge_spans(tc);
    let prefix_w = spans_width(&left_prefix);
    let badges_w = spans_width(&badge_spans);
    let right_border_w = 1; // "right-corner"
    // Reserve at least one fill char so the border looks continuous.
    let title_max_w = line_budget.saturating_sub(prefix_w + badges_w + right_border_w + 1);
    let title_spans = truncate_spans_to_width(markdown_inline_spans(&tc.title), title_max_w);
    let title_w = spans_width(&title_spans);
    let fill_w = line_budget.saturating_sub(prefix_w + title_w + badges_w + right_border_w);
    let top_fill: String = "\u{2500}".repeat(fill_w);

    let mut top = left_prefix;
    top.extend(title_spans);
    top.extend(badge_spans);
    top.push(Span::styled(format!("{top_fill}\u{256E}"), border));
    out.push(Line::from(top));

    // Content lines with left border prefix
    for line in content {
        let mut spans = vec![Span::styled("  \u{2502} ", border)];
        spans.extend(line.spans.iter().cloned());
        out.push(Line::from(spans));
    }

    // Bottom border
    let bottom_fill: String = "\u{2500}".repeat(inner_w.saturating_sub(2));
    out.push(Line::from(Span::styled(format!("  \u{2570}{bottom_fill}\u{256F}"), border)));

    out
}
