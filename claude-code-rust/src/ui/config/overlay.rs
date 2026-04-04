use ratatui::Frame;
use ratatui::layout::{Constraint, Direction, Layout, Margin, Rect};
use ratatui::style::{Modifier, Style};
use ratatui::text::{Line, Span};
use ratatui::widgets::{Block, Borders, Clear, Paragraph};

use crate::ui::theme;

#[derive(Debug, Clone, Copy)]
pub(super) struct OverlayLayoutSpec {
    pub min_width: u16,
    pub min_height: u16,
    pub width_percent: u16,
    pub height_percent: u16,
    pub preferred_height: u16,
    pub fullscreen_below: Option<(u16, u16)>,
    pub inner_margin: Margin,
}

#[derive(Debug, Clone, Copy)]
pub(super) struct OverlayChrome<'a> {
    pub title: &'a str,
    pub subtitle: Option<&'a str>,
    pub help: Option<&'a str>,
}

#[derive(Debug, Clone, Copy)]
pub(super) struct RenderedOverlay {
    pub body_area: Rect,
}

pub(super) fn render_overlay_shell(
    frame: &mut Frame,
    area: Rect,
    layout_spec: OverlayLayoutSpec,
    chrome: OverlayChrome<'_>,
) -> RenderedOverlay {
    let overlay_area = overlay_rect(area, layout_spec);
    frame.render_widget(Clear, overlay_area);
    frame.render_widget(
        Block::default()
            .borders(Borders::ALL)
            .title(chrome.title)
            .border_style(Style::default().fg(theme::RUST_ORANGE)),
        overlay_area,
    );

    let inner = overlay_area.inner(layout_spec.inner_margin);
    let sections = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(u16::from(chrome.subtitle.is_some())),
            Constraint::Min(1),
            Constraint::Length(u16::from(chrome.help.is_some())),
        ])
        .split(inner);

    if let Some(subtitle) = chrome.subtitle {
        frame.render_widget(
            Paragraph::new(Line::from(Span::styled(subtitle, Style::default().fg(theme::DIM)))),
            sections[0],
        );
    }
    if let Some(help) = chrome.help {
        frame.render_widget(
            Paragraph::new(Line::from(Span::styled(help, Style::default().fg(theme::RUST_ORANGE)))),
            sections[2],
        );
    }

    RenderedOverlay { body_area: sections[1] }
}

pub(super) fn overlay_line_style(selected: bool, focused: bool) -> Style {
    if selected && focused {
        Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD)
    } else if selected {
        Style::default().fg(ratatui::style::Color::White).add_modifier(Modifier::BOLD)
    } else {
        Style::default().fg(ratatui::style::Color::White)
    }
}

pub(super) fn render_overlay_header(frame: &mut Frame, area: Rect, title: &str, focused: bool) {
    let prefix = if focused { "> " } else { "  " };
    frame.render_widget(
        Paragraph::new(Line::from(Span::styled(
            format!("{prefix}{title}"),
            if focused {
                Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD)
            } else {
                Style::default().fg(theme::DIM)
            },
        ))),
        area,
    );
}

pub(super) fn render_overlay_separator(frame: &mut Frame, area: Rect) {
    let width = usize::from(area.width.max(1));
    frame.render_widget(
        Paragraph::new(Line::from(Span::styled(
            "-".repeat(width),
            Style::default().fg(theme::DIM),
        ))),
        area,
    );
}

fn overlay_rect(area: Rect, spec: OverlayLayoutSpec) -> Rect {
    if spec
        .fullscreen_below
        .is_some_and(|(min_width, min_height)| area.width < min_width || area.height < min_height)
    {
        return area;
    }

    let overlay_width = ((u32::from(area.width) * u32::from(spec.width_percent)) / 100)
        .try_into()
        .unwrap_or(area.width)
        .max(spec.min_width)
        .clamp(1, area.width);
    let percent_height = ((u32::from(area.height) * u32::from(spec.height_percent)) / 100)
        .try_into()
        .unwrap_or(area.height)
        .clamp(1, area.height);
    let overlay_height = percent_height
        .min(spec.preferred_height.min(area.height))
        .max(spec.min_height.min(area.height));

    centered_rect_with_size(area, overlay_width, overlay_height)
}

fn centered_rect_with_size(area: Rect, width: u16, height: u16) -> Rect {
    let width = width.min(area.width);
    let height = height.min(area.height);
    Rect {
        x: area.x + area.width.saturating_sub(width) / 2,
        y: area.y + area.height.saturating_sub(height) / 2,
        width,
        height,
    }
}
