// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

//! Inline interaction rendering: permissions, plan approvals, and `AskUserQuestion`.

use crate::agent::model::PermissionOptionKind;
use crate::app::{InlinePermission, InlineQuestion, ToolCallInfo};
use crate::ui::theme;
use ratatui::style::{Color, Modifier, Style};
use ratatui::text::{Line, Span};

use super::markdown_inline_spans;

/// Render inline permission options on a single compact line.
/// Options are dynamic and include shortcuts only when applicable.
/// Unfocused permissions are dimmed to indicate they don't have keyboard input.
pub(super) fn render_permission_lines(
    tc: &ToolCallInfo,
    perm: &InlinePermission,
) -> Vec<Line<'static>> {
    if tc.is_exit_plan_mode_tool() || is_plan_approval_permission(perm) {
        return render_plan_approval_lines(tc, perm);
    }

    // Unfocused permissions: show a dimmed "waiting for focus" line
    if !perm.focused {
        return vec![
            Line::default(),
            Line::from(Span::styled(
                "  \u{25cb} Waiting for input\u{2026} (\u{2191}\u{2193} to focus)",
                Style::default().fg(theme::DIM),
            )),
        ];
    }

    let mut spans: Vec<Span<'static>> = Vec::new();
    let dot = Span::styled("  \u{00b7}  ", Style::default().fg(theme::DIM));

    for (i, opt) in perm.options.iter().enumerate() {
        let is_selected = i == perm.selected_index;
        let is_allow = matches!(
            opt.kind,
            PermissionOptionKind::AllowOnce
                | PermissionOptionKind::AllowSession
                | PermissionOptionKind::AllowAlways
        );

        let (icon, icon_color) = if is_allow {
            ("\u{2713}", Color::Green) // check
        } else {
            ("\u{2717}", Color::Red) // cross
        };

        // Separator between options
        if i > 0 {
            spans.push(dot.clone());
        }

        // Selection indicator
        if is_selected {
            spans.push(Span::styled(
                "\u{25b8} ",
                Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD),
            ));
        }

        spans.push(Span::styled(format!("{icon} "), Style::default().fg(icon_color)));

        let name_style = if is_selected {
            Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD)
        } else {
            Style::default().fg(Color::Gray)
        };
        let mut name_spans = markdown_inline_spans(&opt.name);
        if name_spans.is_empty() {
            spans.push(Span::styled(opt.name.clone(), name_style));
        } else {
            for span in &mut name_spans {
                span.style = span.style.patch(name_style);
            }
            spans.extend(name_spans);
        }

        let shortcut = match opt.kind {
            PermissionOptionKind::AllowOnce => " (Ctrl+y)",
            PermissionOptionKind::AllowSession | PermissionOptionKind::AllowAlways => " (Ctrl+a)",
            PermissionOptionKind::RejectOnce => " (Ctrl+n)",
            PermissionOptionKind::RejectAlways
            | PermissionOptionKind::QuestionChoice
            | PermissionOptionKind::PlanApprove
            | PermissionOptionKind::PlanReject => "",
        };
        spans.push(Span::styled(shortcut, Style::default().fg(theme::DIM)));
    }

    vec![
        Line::default(),
        Line::from(spans),
        Line::from(Span::styled(
            "\u{2190}\u{2192} select  \u{2191}\u{2193} next  enter confirm  esc reject",
            Style::default().fg(theme::DIM),
        )),
    ]
}

fn is_plan_approval_permission(perm: &InlinePermission) -> bool {
    perm.options.iter().any(|opt| {
        matches!(opt.kind, PermissionOptionKind::PlanApprove | PermissionOptionKind::PlanReject)
    })
}

fn parse_exit_plan_mode_allowed_prompts(raw_input: Option<&serde_json::Value>) -> Vec<String> {
    let Some(raw) = raw_input else {
        return Vec::new();
    };
    let Some(arr) = raw.get("allowedPrompts").and_then(|v| v.as_array()) else {
        return Vec::new();
    };
    arr.iter()
        .filter_map(|item| {
            let prompt = item.get("prompt")?.as_str()?;
            let tool = item.get("tool")?.as_str()?;
            Some(format!("{tool}: {prompt}"))
        })
        .collect()
}

fn render_plan_approval_lines(tc: &ToolCallInfo, perm: &InlinePermission) -> Vec<Line<'static>> {
    if !perm.focused {
        return vec![
            Line::default(),
            Line::from(Span::styled(
                "  \u{25cb} Waiting for input\u{2026} (\u{2191}\u{2193} to focus)",
                Style::default().fg(theme::DIM),
            )),
        ];
    }

    let mut lines = vec![Line::default()];

    // Show pre-approved actions requested by Claude, if any.
    let allowed_prompts = parse_exit_plan_mode_allowed_prompts(tc.raw_input.as_ref());
    if !allowed_prompts.is_empty() {
        lines.push(Line::from(Span::styled(
            "  Pre-approved actions:",
            Style::default().fg(theme::DIM),
        )));
        for prompt_text in allowed_prompts {
            lines.push(Line::from(vec![
                Span::styled("    \u{2022} ", Style::default().fg(theme::DIM)),
                Span::styled(prompt_text, Style::default().fg(Color::White)),
            ]));
        }
        lines.push(Line::default());
    }

    // Stacked approve / reject options.
    for (i, opt) in perm.options.iter().enumerate() {
        let is_selected = i == perm.selected_index;
        let (icon, icon_color, shortcut) = match opt.kind {
            PermissionOptionKind::PlanApprove => ("\u{2713}", Color::Green, " [y]"),
            PermissionOptionKind::PlanReject => ("\u{2717}", Color::Red, " [n]"),
            _ => ("\u{00b7}", Color::Gray, ""),
        };

        let name_style = if is_selected {
            Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD)
        } else {
            Style::default().fg(Color::Gray)
        };

        let mut line_spans: Vec<Span<'static>> = Vec::new();
        if is_selected {
            line_spans.push(Span::styled(
                "  \u{25b8} ",
                Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD),
            ));
        } else {
            line_spans.push(Span::raw("    "));
        }
        line_spans.push(Span::styled(format!("{icon} "), Style::default().fg(icon_color)));
        line_spans.push(Span::styled(opt.name.clone(), name_style));
        line_spans.push(Span::styled(shortcut, Style::default().fg(theme::DIM)));
        lines.push(Line::from(line_spans));
    }

    lines.push(Line::from(Span::styled(
        "  \u{2191}\u{2193} select  enter confirm  y approve  n/esc reject",
        Style::default().fg(theme::DIM),
    )));

    lines
}

#[allow(clippy::too_many_lines)]
pub(super) fn render_question_lines(question: &InlineQuestion) -> Vec<Line<'static>> {
    let progress = match question.total_questions {
        total if total > 0 => format!(" ({}/{total})", question.question_index + 1),
        _ => String::new(),
    };

    let mut lines = vec![
        Line::default(),
        Line::from(vec![
            Span::styled("  ? ", Style::default().fg(theme::RUST_ORANGE)),
            Span::styled(
                format!("{}{}", question.prompt.header, progress),
                Style::default().fg(Color::White).add_modifier(Modifier::BOLD),
            ),
        ]),
    ];

    for row in question.prompt.question.lines() {
        lines.push(Line::from(vec![Span::styled(
            format!("    {row}"),
            if question.focused {
                Style::default().fg(theme::RUST_ORANGE)
            } else {
                Style::default().fg(Color::Gray)
            },
        )]));
    }

    if !question.focused {
        lines.push(Line::from(Span::styled(
            "  waiting for input... (Up/Down to focus)",
            Style::default().fg(theme::DIM),
        )));
        return lines;
    }

    let horizontal = question.prompt.options.len() <= 3
        && question.prompt.options.iter().all(|opt| {
            opt.description.as_deref().is_none_or(str::is_empty) && opt.label.chars().count() <= 20
        });

    if horizontal {
        let mut spans: Vec<Span<'static>> = Vec::new();
        for (i, opt) in question.prompt.options.iter().enumerate() {
            if i > 0 {
                spans.push(Span::styled("  |  ", Style::default().fg(theme::DIM)));
            }
            let selected = i == question.focused_option_index;
            let checked = question.selected_option_indices.contains(&i);
            if selected {
                spans.push(Span::styled(
                    "\u{25b8} ",
                    Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD),
                ));
            } else {
                spans.push(Span::styled("  ", Style::default().fg(theme::DIM)));
            }
            let style = if selected {
                Style::default().fg(Color::White).add_modifier(Modifier::BOLD)
            } else {
                Style::default().fg(Color::Gray)
            };
            let marker = if checked { "[x] " } else { "[ ] " };
            if question.prompt.multi_select {
                spans.push(Span::styled(marker, Style::default().fg(theme::DIM)));
            }
            spans.push(Span::styled(opt.label.clone(), style));
        }
        lines.push(Line::from(spans));
    } else {
        for (i, opt) in question.prompt.options.iter().enumerate() {
            let selected = i == question.focused_option_index;
            let checked = question.selected_option_indices.contains(&i);
            let bullet = if selected { "  \u{25b8} " } else { "  \u{25cb} " };
            let name_style = if selected {
                Style::default().fg(Color::White).add_modifier(Modifier::BOLD)
            } else {
                Style::default().fg(Color::Gray)
            };
            lines.push(Line::from(vec![
                Span::styled(
                    bullet,
                    if selected {
                        Style::default().fg(theme::RUST_ORANGE)
                    } else {
                        Style::default().fg(theme::DIM)
                    },
                ),
                Span::styled(
                    if question.prompt.multi_select {
                        if checked { "[x] " } else { "[ ] " }
                    } else {
                        ""
                    },
                    Style::default().fg(theme::DIM),
                ),
                Span::styled(opt.label.clone(), name_style),
            ]));
            if let Some(desc) = opt.description.as_ref().map(|d| d.trim()).filter(|d| !d.is_empty())
            {
                lines.push(Line::from(Span::styled(
                    format!("      {desc}"),
                    Style::default().fg(theme::DIM),
                )));
            }
        }
    }

    if let Some(preview) = question
        .prompt
        .options
        .get(question.focused_option_index)
        .and_then(|option| option.preview.as_deref())
        .map(str::trim)
        .filter(|preview| !preview.is_empty())
    {
        lines.push(Line::default());
        lines.push(Line::from(Span::styled(
            "  Preview",
            Style::default().fg(Color::White).add_modifier(Modifier::BOLD),
        )));
        for row in preview.lines() {
            lines.push(Line::from(Span::styled(
                format!("    {row}"),
                Style::default().fg(theme::DIM),
            )));
        }
    }

    lines.push(Line::default());
    lines.push(Line::from(vec![
        Span::styled(
            format!("  Notes{}: ", if question.editing_notes { " [editing]" } else { "" }),
            Style::default().fg(Color::White).add_modifier(Modifier::BOLD),
        ),
        Span::styled(
            if question.notes.is_empty() { "<empty>".to_owned() } else { question.notes.clone() },
            if question.editing_notes {
                Style::default().fg(Color::White)
            } else {
                Style::default().fg(theme::DIM)
            },
        ),
    ]));

    lines.push(Line::from(Span::styled(
        if question.prompt.multi_select {
            "  Left/Right move  Space toggle  Tab notes  Enter confirm  Esc cancel"
        } else {
            "  Left/Right select  Tab notes  Enter confirm  Esc cancel"
        },
        Style::default().fg(theme::DIM),
    )));
    lines
}

#[cfg(test)]
mod tests {
    use super::{render_permission_lines, render_plan_approval_lines, render_question_lines};
    use crate::agent::model::{
        PermissionOption, PermissionOptionKind, QuestionOption, QuestionPrompt, ToolCallStatus,
    };
    use crate::app::{InlinePermission, InlineQuestion, ToolCallInfo};
    use crate::ui::theme;
    use ratatui::style::Color;
    use std::collections::BTreeSet;

    fn test_question() -> InlineQuestion {
        let (response_tx, _response_rx) = tokio::sync::oneshot::channel();
        InlineQuestion {
            prompt: QuestionPrompt::new(
                "Which mode should we use?",
                "Mode",
                false,
                vec![
                    QuestionOption::new("safe", "Safer path"),
                    QuestionOption::new("fast", "Faster path"),
                ],
            ),
            response_tx,
            focused_option_index: 0,
            selected_option_indices: BTreeSet::new(),
            notes: String::new(),
            notes_cursor: 0,
            editing_notes: false,
            focused: true,
            question_index: 0,
            total_questions: 2,
        }
    }

    fn test_tool_call(sdk_tool_name: &str) -> ToolCallInfo {
        ToolCallInfo {
            id: "tool-1".into(),
            title: "Tool".into(),
            sdk_tool_name: sdk_tool_name.into(),
            raw_input: None,
            raw_input_bytes: 0,
            output_metadata: None,
            status: ToolCallStatus::InProgress,
            content: Vec::new(),
            hidden: false,
            terminal_id: None,
            terminal_command: None,
            terminal_output: None,
            terminal_output_len: 0,
            terminal_bytes_seen: 0,
            terminal_snapshot_mode: crate::app::TerminalSnapshotMode::AppendOnly,
            render_epoch: 0,
            layout_epoch: 0,
            last_measured_width: 0,
            last_measured_height: 0,
            last_measured_layout_epoch: 0,
            last_measured_layout_generation: 0,
            cache: crate::app::BlockCache::default(),
            pending_permission: None,
            pending_question: None,
        }
    }

    fn test_permission(kind: PermissionOptionKind) -> InlinePermission {
        let (response_tx, _response_rx) = tokio::sync::oneshot::channel();
        InlinePermission {
            options: vec![
                PermissionOption::new("allow", "Allow", kind),
                PermissionOption::new("deny", "Deny", PermissionOptionKind::RejectOnce),
            ],
            response_tx,
            selected_index: 0,
            focused: true,
        }
    }

    #[test]
    fn focused_question_uses_left_right_footer_hint() {
        let lines = render_question_lines(&test_question());
        let footer = lines.last().expect("question footer line");
        assert_eq!(
            footer.spans[0].content.as_ref(),
            "  Left/Right select  Tab notes  Enter confirm  Esc cancel"
        );
    }

    #[test]
    fn focused_question_text_turns_orange() {
        let lines = render_question_lines(&test_question());
        assert_eq!(lines[2].spans[0].style.fg, Some(theme::RUST_ORANGE));
    }

    #[test]
    fn unfocused_question_text_stays_gray() {
        let mut question = test_question();
        question.focused = false;
        let lines = render_question_lines(&question);
        let footer = lines.last().expect("question footer line");
        assert_eq!(footer.spans[0].content.as_ref(), "  waiting for input... (Up/Down to focus)");
        assert_eq!(lines[2].spans[0].style.fg, Some(Color::Gray));
    }

    #[test]
    fn selected_permission_option_uses_orange_label() {
        let tc = test_tool_call("Bash");
        let perm = test_permission(PermissionOptionKind::AllowOnce);

        let lines = render_permission_lines(&tc, &perm);
        let selected_label = lines[1]
            .spans
            .iter()
            .find(|span| span.content.as_ref() == "Allow")
            .expect("selected permission label");

        assert_eq!(selected_label.style.fg, Some(theme::RUST_ORANGE));
    }

    #[test]
    fn selected_plan_option_uses_orange_label() {
        let tc = test_tool_call("ExitPlanMode");
        let perm = test_permission(PermissionOptionKind::PlanApprove);

        let lines = render_plan_approval_lines(&tc, &perm);
        let selected_label = lines[1]
            .spans
            .iter()
            .find(|span| span.content.as_ref() == "Allow")
            .expect("selected plan label");

        assert_eq!(selected_label.style.fg, Some(theme::RUST_ORANGE));
    }
}
