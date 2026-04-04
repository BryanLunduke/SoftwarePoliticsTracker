// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use crate::app::input::parse_paste_placeholder_ranges;
use crate::app::mention;
use crate::app::subagent;
use crate::app::{App, AppStatus};
use crate::ui::theme;
use ratatui::Frame;
use ratatui::buffer::Buffer;
use ratatui::layout::{Constraint, Layout, Rect};
use ratatui::style::{Color, Modifier, Style};
use ratatui::text::{Line, Span};
use ratatui::widgets::Paragraph;
use ratatui::widgets::Widget;
use tui_textarea::TextArea;

/// Horizontal padding to match header/footer inset.
const INPUT_PAD: u16 = 2;

/// Extra right-side breathing room so text doesn't touch the padded edge.
const INPUT_RIGHT_PAD: u16 = 1;

/// Prompt column width: "❯ " = 2 columns (icon + space)
const PROMPT_WIDTH: u16 = 2;

/// Maximum input area height (lines) to prevent the input from consuming the entire screen.
const MAX_INPUT_HEIGHT: u16 = 12;
const HIGHLIGHT_SLASH_PRIORITY: u8 = 6;
const HIGHLIGHT_MENTION_PRIORITY: u8 = 7;
const HIGHLIGHT_SUBAGENT_PRIORITY: u8 = 8;
const HIGHLIGHT_PASTE_PRIORITY: u8 = 9;

/// Braille spinner frames (same as message.rs) for the connecting animation.
const SPINNER_FRAMES: &[char] = &[
    '\u{280B}', '\u{2819}', '\u{2839}', '\u{2838}', '\u{283C}', '\u{2834}', '\u{2826}', '\u{2827}',
    '\u{2807}', '\u{280F}',
];

/// Height of the login hint banner in lines (0 when no hint is active).
/// Used internally by `visual_line_count` and `render` so the layout
/// calculation and rendering stay in sync.
const LOGIN_HINT_LINES: u16 = 2;
const CANCEL_HINT_LINES: u16 = 1;

#[derive(Clone, Copy)]
pub(crate) struct InputRenderGeometry {
    pub hint_pad: Option<Rect>,
    pub padded: Rect,
    pub prompt: Rect,
    pub text: Rect,
}

/// Whether a login hint banner is active.
fn has_login_hint(app: &App) -> bool {
    app.login_hint.is_some()
}

fn has_cancel_hint(app: &App) -> bool {
    app.pending_cancel_origin.is_some()
}

pub(crate) fn hint_line_count(app: &App) -> u16 {
    let login = if has_login_hint(app) { LOGIN_HINT_LINES } else { 0 };
    let cancel = if has_cancel_hint(app) { CANCEL_HINT_LINES } else { 0 };
    login + cancel
}

pub(crate) fn compute_render_geometry(area: Rect, hint_lines: u16) -> InputRenderGeometry {
    let (hint_area, input_main_area) = if hint_lines > 0 {
        let [hint, main] =
            Layout::vertical([Constraint::Length(hint_lines), Constraint::Min(1)]).areas(area);
        (Some(hint), main)
    } else {
        (None, area)
    };

    let hint_pad = hint_area.map(|hint| Rect {
        x: hint.x.saturating_add(INPUT_PAD),
        y: hint.y,
        width: hint.width.saturating_sub(INPUT_PAD * 2 + INPUT_RIGHT_PAD),
        height: hint.height,
    });

    let padded = Rect {
        x: input_main_area.x.saturating_add(INPUT_PAD),
        y: input_main_area.y,
        width: input_main_area.width.saturating_sub(INPUT_PAD * 2 + INPUT_RIGHT_PAD),
        height: input_main_area.height,
    };
    let [prompt, text] =
        Layout::horizontal([Constraint::Length(PROMPT_WIDTH), Constraint::Min(1)]).areas(padded);

    InputRenderGeometry { hint_pad, padded, prompt, text }
}

#[allow(clippy::cast_possible_truncation, clippy::too_many_lines)]
pub fn render(frame: &mut Frame, area: Rect, app: &mut App) {
    let hint_lines = hint_line_count(app);
    let geometry = compute_render_geometry(area, hint_lines);

    if let Some(hint_pad) = geometry.hint_pad {
        let mut next_hint_row = hint_pad.y;

        if let Some(hint) = &app.login_hint {
            let lines = vec![
                Line::from(Span::styled(
                    format!(
                        "Authentication required: {} -- {}",
                        hint.method_name, hint.method_description
                    ),
                    Style::default().fg(Color::Yellow),
                )),
                Line::from(Span::styled(
                    "Type /login to authenticate, or run `claude auth login` in another terminal",
                    Style::default().fg(theme::DIM),
                )),
            ];
            let login_area = Rect {
                x: hint_pad.x,
                y: next_hint_row,
                width: hint_pad.width,
                height: LOGIN_HINT_LINES,
            };
            frame.render_widget(Paragraph::new(lines), login_area);
            next_hint_row = next_hint_row.saturating_add(LOGIN_HINT_LINES);
        }

        if has_cancel_hint(app) {
            let spinner_ch = SPINNER_FRAMES[app.spinner_frame % SPINNER_FRAMES.len()];
            let cancel_line = Line::from(vec![
                Span::styled(format!("{spinner_ch} "), Style::default().fg(theme::DIM)),
                Span::styled(
                    "Cancelling current turn... draft will auto-submit when ready.",
                    Style::default().fg(theme::DIM),
                ),
            ]);
            let cancel_area = Rect {
                x: hint_pad.x,
                y: next_hint_row,
                width: hint_pad.width,
                height: CANCEL_HINT_LINES,
            };
            frame.render_widget(Paragraph::new(cancel_line), cancel_area);
        }
    }

    // During Connecting state, show a spinner with static text
    if app.status == AppStatus::Connecting {
        let spinner_ch = SPINNER_FRAMES[app.spinner_frame % SPINNER_FRAMES.len()];
        let line = Line::from(vec![
            Span::styled(format!("{spinner_ch} "), Style::default().fg(theme::DIM)),
            Span::styled("Connecting to Claude Code...", Style::default().fg(theme::DIM)),
        ]);
        frame.render_widget(Paragraph::new(line), geometry.padded);
        return;
    }

    if app.status == AppStatus::CommandPending {
        let spinner_ch = SPINNER_FRAMES[app.spinner_frame % SPINNER_FRAMES.len()];
        let label = app.pending_command_label.as_deref().unwrap_or("Processing command...");
        let line = Line::from(vec![
            Span::styled(format!("{spinner_ch} "), Style::default().fg(theme::DIM)),
            Span::styled(label.to_owned(), Style::default().fg(theme::DIM)),
        ]);
        frame.render_widget(Paragraph::new(line), geometry.padded);
        return;
    }

    if app.status == AppStatus::Error {
        let lines = vec![
            Line::from(Span::styled(
                "Input disabled due to error",
                Style::default().fg(theme::STATUS_ERROR),
            )),
            Line::from(Span::styled(
                "Press Ctrl+Q to quit and try again.",
                Style::default().fg(theme::DIM),
            )),
        ];
        frame.render_widget(Paragraph::new(lines), geometry.padded);
        return;
    }

    // Render prompt icon
    let prompt = Line::from(Span::styled(
        format!("{} ", theme::PROMPT_CHAR),
        Style::default().fg(theme::RUST_ORANGE),
    ));
    frame.render_widget(Paragraph::new(prompt), geometry.prompt);

    if geometry.text.width == 0 {
        return;
    }

    configure_input_textarea(app);
    app.rendered_input_area = geometry.text;
    if app.selection.is_some_and(|selection| selection.kind == crate::app::SelectionKind::Input) {
        refresh_selection_snapshot(app);
    }
    frame.render_widget(app.input.editor(), geometry.text);

    if let Some(sel) = app.selection
        && sel.kind == crate::app::SelectionKind::Input
    {
        frame.render_widget(SelectionOverlay { selection: sel }, geometry.text);
    }
}

pub(super) fn refresh_selection_snapshot(app: &mut App) {
    if !app.selection.is_some_and(|selection| selection.kind == crate::app::SelectionKind::Input) {
        return;
    }

    let area = app.rendered_input_area;
    if area.width == 0 || area.height == 0 {
        return;
    }

    configure_input_textarea(app);
    app.rendered_input_lines = render_lines_from_textarea(app.input.editor(), area);
}

fn configure_input_textarea(app: &mut App) {
    let needs_highlight_update = app.input.highlight_version != app.input.content_version;

    {
        let textarea = app.input.editor_mut();
        textarea.set_placeholder_text("Type a message...");
        textarea.set_placeholder_style(Style::default().fg(theme::DIM));
        textarea.set_cursor_line_style(Style::default());
        textarea.set_cursor_style(Style::default().add_modifier(Modifier::REVERSED));
    }

    if needs_highlight_update {
        let lines = app.input.lines().to_vec();
        let textarea = app.input.editor_mut();
        textarea.clear_custom_highlight();
        apply_textarea_highlights(textarea, &lines);
        app.input.highlight_version = app.input.content_version;
    }
}

fn apply_textarea_highlights(textarea: &mut TextArea<'_>, lines: &[String]) {
    let slash_style = Style::default().fg(theme::SLASH_COMMAND);
    let mention_style = Style::default().fg(Color::Cyan);
    let subagent_style = Style::default().fg(theme::SUBAGENT_TOKEN);
    let paste_style = Style::default().fg(Color::Green);

    for (row, line) in lines.iter().enumerate() {
        if let Some((start, end)) = slash_command_range(line) {
            textarea.custom_highlight(
                ((row, start), (row, end)),
                slash_style,
                HIGHLIGHT_SLASH_PRIORITY,
            );
        }

        for (start, end, _) in mention::find_mention_spans(line) {
            textarea.custom_highlight(
                ((row, start), (row, end)),
                mention_style,
                HIGHLIGHT_MENTION_PRIORITY,
            );
        }

        for (start, end, _) in subagent::find_subagent_spans(line) {
            textarea.custom_highlight(
                ((row, start), (row, end)),
                subagent_style,
                HIGHLIGHT_SUBAGENT_PRIORITY,
            );
        }

        for (start, end) in parse_paste_placeholder_ranges(line) {
            textarea.custom_highlight(
                ((row, start), (row, end)),
                paste_style,
                HIGHLIGHT_PASTE_PRIORITY,
            );
        }
    }
}

fn slash_command_range(line: &str) -> Option<(usize, usize)> {
    let start = line.find(|c: char| !c.is_whitespace())?;
    if line.as_bytes().get(start).copied() != Some(b'/') {
        return None;
    }
    let rel_end =
        line[start..].find(char::is_whitespace).unwrap_or_else(|| line.len().saturating_sub(start));
    let end = start + rel_end;
    if end <= start + 1 {
        return None;
    }
    Some((start, end))
}

struct SelectionOverlay {
    selection: crate::app::SelectionState,
}

impl Widget for SelectionOverlay {
    #[allow(clippy::cast_possible_truncation)]
    fn render(self, area: Rect, buf: &mut Buffer) {
        let (start, end) =
            crate::app::normalize_selection(self.selection.start, self.selection.end);
        for row in start.row..=end.row {
            let y = area.y.saturating_add(row as u16);
            if y >= area.bottom() {
                break;
            }
            let row_start = if row == start.row { start.col } else { 0 };
            let row_end = if row == end.row { end.col } else { area.width as usize };
            for col in row_start..row_end {
                let x = area.x.saturating_add(col as u16);
                if x >= area.right() {
                    break;
                }
                if let Some(cell) = buf.cell_mut((x, y)) {
                    cell.set_style(cell.style().add_modifier(Modifier::REVERSED));
                }
            }
        }
    }
}

fn render_lines_from_textarea(textarea: &TextArea<'_>, area: Rect) -> Vec<String> {
    let mut buf = Buffer::empty(area);
    textarea.render(area, &mut buf);
    let mut lines = Vec::with_capacity(area.height as usize);
    for y in 0..area.height {
        let mut line = String::new();
        for x in 0..area.width {
            if let Some(cell) = buf.cell((area.x + x, area.y + y)) {
                line.push_str(cell.symbol());
            }
        }
        lines.push(line.trim_end().to_owned());
    }
    lines
}

/// Total visual height for the input area: input lines + hint banners.
/// Called by the layout to allocate the correct input area height.
pub fn visual_line_count(app: &mut App, area_width: u16) -> u16 {
    let hint = hint_line_count(app);
    let content_width =
        area_width.saturating_sub(INPUT_PAD * 2 + INPUT_RIGHT_PAD).saturating_sub(PROMPT_WIDTH);
    let input_lines = app.input.measure_visual_lines(content_width, MAX_INPUT_HEIGHT);
    hint + input_lines
}

#[cfg(test)]
mod tests {
    use super::{
        CANCEL_HINT_LINES, LOGIN_HINT_LINES, MAX_INPUT_HEIGHT, slash_command_range,
        visual_line_count,
    };
    use crate::app::subagent::find_subagent_spans;
    use crate::app::{App, CancelOrigin, LoginHint};

    #[test]
    fn slash_range_matches_leading_command_token() {
        assert_eq!(slash_command_range("/mode plan"), Some((0, 5)));
        assert_eq!(slash_command_range("  /mode plan"), Some((2, 7)));
    }

    #[test]
    fn slash_range_ignores_non_command_lines() {
        assert_eq!(slash_command_range("hello /mode"), None);
        assert_eq!(slash_command_range("/"), None);
        assert_eq!(slash_command_range("   "), None);
    }

    #[test]
    fn subagent_spans_match_valid_ampersand_tokens() {
        let spans = find_subagent_spans("&reviewer and &explore");
        assert_eq!(spans.len(), 2);
        assert_eq!(spans[0].2, "reviewer");
        assert_eq!(spans[1].2, "explore");
    }

    #[test]
    fn subagent_spans_reject_double_ampersand_shell_syntax() {
        let spans = find_subagent_spans("cargo test && cargo clippy");
        assert!(spans.is_empty());
    }

    #[test]
    fn visual_line_count_uses_textarea_max_rows() {
        let mut app = App::test_default();
        app.input.set_text(&"x".repeat(500));
        assert_eq!(visual_line_count(&mut app, 8), MAX_INPUT_HEIGHT);
    }

    #[test]
    fn visual_line_count_includes_login_hint_rows() {
        let mut app = App::test_default();
        app.login_hint = Some(LoginHint {
            method_name: "oauth".to_owned(),
            method_description: "Sign in".to_owned(),
        });
        assert_eq!(visual_line_count(&mut app, 80), LOGIN_HINT_LINES + 1);
    }

    #[test]
    fn visual_line_count_includes_cancel_hint_row() {
        let mut app = App::test_default();
        app.pending_cancel_origin = Some(CancelOrigin::AutoQueue);
        assert_eq!(visual_line_count(&mut app, 80), CANCEL_HINT_LINES + 1);
    }
}
