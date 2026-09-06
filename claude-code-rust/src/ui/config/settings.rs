use super::theme;
use crate::app::App;
use crate::app::config::{
    resolved_setting, setting_detail_options, setting_display_value, setting_invalid_hint,
    setting_specs,
};
use ratatui::Frame;
use ratatui::layout::{Constraint, Direction, Layout, Margin, Rect};
use ratatui::style::{Color, Modifier, Style};
use ratatui::text::{Line, Span};
use ratatui::widgets::{Block, Borders, List, ListItem, ListState, Paragraph, Wrap};
use std::fmt::Write as _;

pub(super) fn render(frame: &mut Frame, area: Rect, app: &mut App) {
    if compact_settings_layout(area) {
        let sections = Layout::default()
            .direction(Direction::Vertical)
            .constraints([
                Constraint::Min(super::MIN_SETTINGS_PANEL_HEIGHT),
                Constraint::Length(reserved_hint_height(area)),
            ])
            .split(area);

        frame.render_widget(
            Block::default()
                .borders(Borders::ALL)
                .title("Settings")
                .border_style(Style::default().fg(theme::DIM)),
            sections[0],
        );
        render_settings_list(frame, panel_body(sections[0]), app, true);
        render_settings_limitation_hint(frame, hint_area(area, sections[1]));
        return;
    }

    let content = padded_body_area(area);
    let sections = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Min(super::MIN_SETTINGS_PANEL_HEIGHT),
            Constraint::Length(reserved_hint_height(content)),
        ])
        .split(content);
    let columns = Layout::default()
        .direction(Direction::Horizontal)
        .constraints([Constraint::Percentage(44), Constraint::Percentage(56)])
        .spacing(1)
        .split(sections[0]);

    frame.render_widget(
        Block::default()
            .borders(Borders::ALL)
            .title("Settings")
            .border_style(Style::default().fg(theme::DIM)),
        columns[0],
    );
    frame.render_widget(
        Block::default()
            .borders(Borders::ALL)
            .title("Details")
            .border_style(Style::default().fg(theme::DIM)),
        columns[1],
    );

    render_settings_list(frame, panel_body(columns[0]), app, false);
    frame.render_widget(
        Paragraph::new(setting_detail_lines(app)).wrap(Wrap { trim: false }),
        panel_body(columns[1]),
    );
    render_settings_limitation_hint(frame, hint_area(content, sections[1]));
}

pub(super) fn compact_settings_layout(area: Rect) -> bool {
    area.width < COMPACT_SETTINGS_MIN_WIDTH || area.height < COMPACT_SETTINGS_MIN_HEIGHT
}

pub(super) fn settings_hint_height(viewport_width: u16) -> u16 {
    if viewport_width == 0 {
        return 0;
    }

    let line_count = Paragraph::new(super::SETTINGS_LIMITATION_HINT)
        .wrap(Wrap { trim: false })
        .line_count(viewport_width);
    u16::try_from(line_count).unwrap_or(u16::MAX)
}

pub(super) fn setting_detail_lines(app: &App) -> Vec<Line<'static>> {
    let Some(spec) = app.config.selected_setting_spec() else {
        return vec![detail_text("No setting selected.")];
    };
    let resolved = resolved_setting(app, spec);

    let mut lines = vec![detail_title(spec.label), detail_text(spec.description)];

    if !spec.supported {
        lines.push(Line::default());
        lines.push(unsupported_hint());
    }

    let options = setting_detail_options(app, spec);
    if !options.is_empty() {
        lines.push(Line::default());
        lines.push(detail_section_title("Options"));
        lines.extend(options.into_iter().map(detail_option));
    }

    if let Some(hint) = setting_invalid_hint(spec, resolved.validation) {
        lines.push(Line::default());
        lines.push(Line::from(Span::styled(
            format!(
                "Invalid persisted value detected. Runtime uses the fallback until you save a valid selection. {hint}"
            ),
            Style::default().fg(theme::STATUS_ERROR),
        )));
    }

    lines
}

fn render_settings_list(frame: &mut Frame, area: Rect, app: &mut App, compact: bool) {
    let mut state = ListState::default()
        .with_selected(Some(app.config.selected_setting_index))
        .with_offset(app.config.settings_scroll_offset);
    let list = List::new(setting_items(app, compact, area.width));
    frame.render_stateful_widget(list, area, &mut state);
    app.config.settings_scroll_offset = state.offset();
}

fn padded_body_area(area: Rect) -> Rect {
    area.inner(Margin { vertical: 1, horizontal: 2 })
}

fn panel_body(area: Rect) -> Rect {
    area.inner(Margin { vertical: 1, horizontal: 2 })
}

fn reserved_hint_height(base_area: Rect) -> u16 {
    if base_area.height == 0 {
        return 0;
    }

    let desired = settings_hint_height(hint_text_width(base_area)).max(1);
    let max_hint = base_area.height.saturating_sub(super::MIN_SETTINGS_PANEL_HEIGHT).max(1);
    desired.min(max_hint)
}

fn hint_text_width(base_area: Rect) -> u16 {
    base_area.width.saturating_sub(4)
}

fn hint_area(base_area: Rect, hint_row: Rect) -> Rect {
    Rect {
        x: base_area.x.saturating_add(2),
        y: hint_row.y,
        width: hint_text_width(base_area),
        height: hint_row.height,
    }
}

fn render_settings_limitation_hint(frame: &mut Frame, area: Rect) {
    frame.render_widget(
        Paragraph::new(super::SETTINGS_LIMITATION_HINT)
            .style(Style::default().fg(Color::White))
            .wrap(Wrap { trim: false }),
        area,
    );
}

fn setting_items(app: &App, compact: bool, viewport_width: u16) -> Vec<ListItem<'static>> {
    let mut items = Vec::new();
    for (index, spec) in setting_specs().iter().enumerate() {
        let resolved = resolved_setting(app, spec);
        let mut lines = vec![config_line(
            app.config.selected_setting_index == index,
            spec.label,
            &setting_display_value(app, spec, &resolved),
            resolved.validation.is_invalid(),
        )];
        if let Some(hint) = setting_invalid_hint(spec, resolved.validation) {
            lines.extend(wrap_styled_text(
                &format!("  {hint}"),
                Style::default().fg(theme::STATUS_ERROR),
                viewport_width,
            ));
        }
        if compact && !spec.supported {
            lines.extend(unsupported_hint_lines(viewport_width));
        }
        if index + 1 < setting_specs().len() {
            lines.push(Line::default());
        }
        items.push(ListItem::new(lines));
    }
    items
}

fn detail_title(text: &str) -> Line<'static> {
    Line::from(Span::styled(
        text.to_owned(),
        Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD),
    ))
}

fn detail_text(text: &str) -> Line<'static> {
    Line::from(Span::styled(text.to_owned(), Style::default().fg(Color::White)))
}

fn detail_section_title(text: &str) -> Line<'static> {
    Line::from(Span::styled(
        text.to_owned(),
        Style::default().fg(theme::DIM).add_modifier(Modifier::BOLD),
    ))
}

fn detail_option(text: String) -> Line<'static> {
    Line::from(vec![
        Span::styled("- ", Style::default().fg(theme::DIM)),
        Span::styled(text, Style::default().fg(Color::White)),
    ])
}

fn unsupported_hint() -> Line<'static> {
    Line::from(Span::styled(
        "  Warning: not supported yet; this setting will not affect sessions.".to_owned(),
        Style::default().fg(Color::Yellow),
    ))
}

fn unsupported_hint_lines(viewport_width: u16) -> Vec<Line<'static>> {
    wrap_styled_text(
        "  Warning: not supported yet; this setting will not affect sessions.",
        Style::default().fg(Color::Yellow),
        viewport_width,
    )
}

fn wrap_styled_text(text: &str, style: Style, viewport_width: u16) -> Vec<Line<'static>> {
    let width = usize::from(viewport_width.max(1));
    if width == 0 {
        return Vec::new();
    }

    let indent_len = text.chars().take_while(|ch| ch.is_whitespace()).count();
    let indent = text.chars().take(indent_len).collect::<String>();
    let content = text.chars().skip(indent_len).collect::<String>();
    let indent_width = Line::raw(indent.as_str()).width();
    let available_width = width.saturating_sub(indent_width).max(1);

    let mut wrapped = Vec::new();
    let mut current = String::new();

    for word in content.split_whitespace() {
        push_wrapped_word(&mut wrapped, &mut current, word, available_width);
    }

    if !current.is_empty() {
        wrapped.push(format!("{indent}{current}"));
    }

    if wrapped.is_empty() {
        wrapped.push(indent);
    }

    wrapped.into_iter().map(|line| Line::from(Span::styled(line, style))).collect()
}

fn push_wrapped_word(
    wrapped: &mut Vec<String>,
    current: &mut String,
    word: &str,
    available_width: usize,
) {
    let mut remaining = word;
    while !remaining.is_empty() {
        let candidate = if current.is_empty() {
            remaining.to_owned()
        } else {
            format!("{current} {remaining}")
        };

        if Line::raw(candidate.as_str()).width() <= available_width {
            current.clear();
            current.push_str(&candidate);
            break;
        }

        if current.is_empty() {
            let split = split_to_width(remaining, available_width);
            let head = remaining[..split].to_owned();
            wrapped.push(head);
            remaining = &remaining[split..];
        } else {
            wrapped.push(std::mem::take(current));
        }
    }
}

fn split_to_width(text: &str, available_width: usize) -> usize {
    let mut width = 0;
    let mut split = 0;
    for (byte_index, ch) in text.char_indices() {
        let ch_width = Line::raw(ch.to_string()).width();
        if byte_index > 0 && width + ch_width > available_width {
            break;
        }
        width += ch_width;
        split = byte_index + ch.len_utf8();
    }
    split.max(1)
}

fn config_line(selected: bool, label: &str, value: &str, invalid: bool) -> Line<'static> {
    let mut line = String::new();
    line.push(if selected { '>' } else { ' ' });
    line.push(' ');
    line.push_str(label);
    let marker = if invalid { " !" } else { "" };
    let _ = write!(&mut line, ": {value}{marker}");
    Line::from(Span::styled(line, Style::default().fg(Color::White)))
}

const COMPACT_SETTINGS_MIN_WIDTH: u16 = 90;
const COMPACT_SETTINGS_MIN_HEIGHT: u16 = 20;
