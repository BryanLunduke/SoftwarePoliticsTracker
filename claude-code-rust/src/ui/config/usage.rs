use super::theme;
use crate::app::usage;
use crate::app::{App, ExtraUsage, UsageWindow};
use ratatui::Frame;
use ratatui::layout::{Constraint, Direction, Layout, Margin, Rect};
use ratatui::style::{Color, Modifier, Style};
use ratatui::text::{Line, Span, Text};
use ratatui::widgets::{Block, Borders, Gauge, Paragraph, Wrap};

pub(super) fn render(frame: &mut Frame, area: Rect, app: &App) {
    let content_area = area.inner(Margin { vertical: 1, horizontal: 2 });
    if content_area.width == 0 || content_area.height == 0 {
        return;
    }

    let windows = app.usage.snapshot.as_ref().map_or_else(Vec::new, usage::visible_windows);

    let mut constraints = vec![Constraint::Length(1)];
    if windows.is_empty() {
        constraints.push(Constraint::Min(3));
    } else {
        for window in &windows {
            constraints.push(Constraint::Length(window_height(window, content_area.width)));
            constraints.push(Constraint::Length(1));
        }
        if let Some(extra_usage) =
            app.usage.snapshot.as_ref().and_then(|snapshot| snapshot.extra_usage.as_ref())
        {
            constraints
                .push(Constraint::Length(extra_usage_height(extra_usage, content_area.width)));
            constraints.push(Constraint::Length(1));
        }
        if let Some(error) = app.usage.last_error.as_deref() {
            constraints.push(Constraint::Length(error_height(error, content_area.width)));
        }
        constraints.push(Constraint::Min(0));
    }

    let sections = Layout::default()
        .direction(Direction::Vertical)
        .constraints(constraints)
        .split(content_area);

    render_spacer(frame, sections[0]);

    if windows.is_empty() {
        render_empty_state(frame, sections[1], app);
        return;
    }

    let Some(snapshot) = app.usage.snapshot.as_ref() else {
        return;
    };
    let mut section_index = 1usize;
    for window in &windows {
        render_window(frame, sections[section_index], window);
        render_spacer(frame, sections[section_index + 1]);
        section_index += 2;
    }

    if let Some(extra_usage) = snapshot.extra_usage.as_ref() {
        render_extra_usage(frame, sections[section_index], extra_usage);
        render_spacer(frame, sections[section_index + 1]);
        section_index += 2;
    }

    if let Some(error) = app.usage.last_error.as_deref() {
        render_error(frame, sections[section_index], error);
    }
}

fn render_spacer(frame: &mut Frame, area: Rect) {
    frame.render_widget(Paragraph::new(Line::default()), area);
}

fn render_empty_state(frame: &mut Frame, area: Rect, app: &App) {
    if app.usage.in_flight {
        frame.render_widget(
            Paragraph::new(Line::from(Span::styled(
                "Loading usage data...",
                Style::default().fg(theme::DIM),
            ))),
            area,
        );
        return;
    }

    let (title, body, color) = if let Some(error) = app.usage.last_error.as_deref() {
        ("Unable to load usage", error, theme::STATUS_ERROR)
    } else {
        (
            "No usage snapshot yet",
            "Press r to fetch Claude usage for the current account.",
            theme::DIM,
        )
    };

    let block = Block::default()
        .borders(Borders::ALL)
        .title(title)
        .border_style(Style::default().fg(color));
    frame.render_widget(block.clone(), area);
    frame.render_widget(
        Paragraph::new(body).wrap(Wrap { trim: false }),
        area.inner(Margin { vertical: 1, horizontal: 2 }),
    );
}

fn render_window(frame: &mut Frame, area: Rect, window: &UsageWindow) {
    let label_line = Line::from(vec![
        Span::styled(window.label.to_owned(), Style::default().add_modifier(Modifier::BOLD)),
        Span::styled(format!("   {}", window_detail_text(window)), Style::default().fg(theme::DIM)),
    ]);
    let label_height = wrapped_height(Text::from(vec![label_line.clone()]), area.width);
    let reset_line = usage::format_window_reset(window).unwrap_or_default();
    let reset_height = wrapped_height(
        Text::from(vec![Line::from(Span::styled(
            reset_line.clone(),
            Style::default().fg(theme::DIM),
        ))]),
        area.width,
    );
    let sections = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(label_height),
            Constraint::Length(1),
            Constraint::Length(reset_height),
        ])
        .split(area);
    frame.render_widget(Paragraph::new(label_line).wrap(Wrap { trim: false }), sections[0]);

    let gauge_area = sections[1];
    let gauge_style = gauge_style(window.utilization);
    frame.render_widget(
        Gauge::default()
            .gauge_style(gauge_style)
            .label("")
            .ratio((window.utilization / 100.0).clamp(0.0, 1.0)),
        gauge_area,
    );

    frame.render_widget(
        Paragraph::new(Line::from(Span::styled(reset_line, Style::default().fg(theme::DIM))))
            .wrap(Wrap { trim: false }),
        sections[2],
    );
}

fn render_extra_usage(frame: &mut Frame, area: Rect, extra_usage: &ExtraUsage) {
    let detail = format_extra_usage(extra_usage);
    let block = Block::default()
        .borders(Borders::ALL)
        .title("Extra credits")
        .border_style(Style::default().fg(theme::DIM));
    frame.render_widget(block.clone(), area);
    let inner = area.inner(Margin { vertical: 1, horizontal: 2 });
    frame.render_widget(
        Paragraph::new(Line::from(Span::styled(detail, Style::default().fg(Color::White))))
            .wrap(Wrap { trim: false }),
        inner,
    );
}

fn render_error(frame: &mut Frame, area: Rect, error: &str) {
    let block = Block::default()
        .borders(Borders::ALL)
        .title("Latest refresh error")
        .border_style(Style::default().fg(theme::STATUS_ERROR));
    frame.render_widget(block.clone(), area);
    frame.render_widget(
        Paragraph::new(error).wrap(Wrap { trim: false }),
        area.inner(Margin { vertical: 1, horizontal: 2 }),
    );
}

fn window_detail_text(window: &UsageWindow) -> String {
    format!("{:.0}% used", window.utilization)
}

fn gauge_style(utilization: f64) -> Style {
    let color = if utilization >= 85.0 {
        theme::STATUS_ERROR
    } else if utilization >= 65.0 {
        theme::STATUS_WARNING
    } else {
        theme::RUST_ORANGE
    };
    Style::default().fg(color).bg(Color::DarkGray)
}

fn format_extra_usage(extra_usage: &ExtraUsage) -> String {
    let currency = extra_usage.currency.as_deref().unwrap_or("USD");
    match (extra_usage.used_credits, extra_usage.monthly_limit) {
        (Some(used), Some(limit)) => format!("{used:.2} of {limit:.2} {currency} used"),
        (Some(used), None) => format!("{used:.2} {currency} used"),
        (None, Some(limit)) => format!("{limit:.2} {currency} limit"),
        (None, None) => match extra_usage.utilization {
            Some(utilization) => format!("{utilization:.0}% of monthly budget"),
            None => "Usage available".to_owned(),
        },
    }
}

fn window_height(window: &UsageWindow, width: u16) -> u16 {
    let label_line = Line::from(vec![
        Span::styled(window.label.to_owned(), Style::default().add_modifier(Modifier::BOLD)),
        Span::styled(format!("   {}", window_detail_text(window)), Style::default().fg(theme::DIM)),
    ]);
    let reset_line = Line::from(Span::styled(
        usage::format_window_reset(window).unwrap_or_default(),
        Style::default().fg(theme::DIM),
    ));
    wrapped_height(Text::from(vec![label_line]), width)
        .saturating_add(1)
        .saturating_add(wrapped_height(Text::from(vec![reset_line]), width))
}

fn extra_usage_height(extra_usage: &ExtraUsage, width: u16) -> u16 {
    let inner_width = width.saturating_sub(4);
    wrapped_height(
        Text::from(vec![Line::from(Span::styled(
            format_extra_usage(extra_usage),
            Style::default().fg(Color::White),
        ))]),
        inner_width,
    )
    .saturating_add(2)
}

fn error_height(error: &str, width: u16) -> u16 {
    let inner_width = width.saturating_sub(4);
    wrapped_height(Text::from(error.to_owned()), inner_width).saturating_add(2)
}

fn wrapped_height(text: Text<'static>, width: u16) -> u16 {
    u16::try_from(Paragraph::new(text).wrap(Wrap { trim: false }).line_count(width))
        .unwrap_or(u16::MAX)
        .max(1)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::app::{UsageSnapshot, UsageSourceKind, UsageSourceMode, UsageState};
    use ratatui::Terminal;
    use ratatui::backend::TestBackend;
    use std::time::SystemTime;

    fn render_usage_rows(app: &App, width: u16, height: u16) -> Vec<String> {
        let backend = TestBackend::new(width, height);
        let mut terminal = Terminal::new(backend).expect("terminal");
        terminal
            .draw(|frame| {
                super::render(frame, frame.area(), app);
            })
            .expect("draw");
        let buffer = terminal.backend().buffer().clone();
        buffer
            .content
            .chunks(usize::from(buffer.area.width))
            .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
            .collect()
    }

    fn render_usage(app: &App) -> String {
        render_usage_rows(app, 100, 24).join("\n")
    }

    fn usage_app() -> App {
        let mut app = App::test_default();
        app.usage = UsageState {
            snapshot: None,
            in_flight: false,
            last_error: None,
            active_source: UsageSourceMode::Auto,
            last_attempted_source: None,
        };
        app
    }

    #[test]
    fn renders_idle_state() {
        let app = usage_app();
        let rendered = render_usage(&app);
        assert!(rendered.contains("No usage snapshot yet"));
    }

    #[test]
    fn renders_loading_state() {
        let mut app = usage_app();
        app.usage.in_flight = true;
        let rendered = render_usage(&app);
        assert!(rendered.contains("Loading usage data..."));
    }

    #[test]
    fn renders_snapshot_with_extra_usage_and_error() {
        let mut app = usage_app();
        app.usage.snapshot = Some(UsageSnapshot {
            source: UsageSourceKind::Oauth,
            fetched_at: SystemTime::now(),
            five_hour: Some(UsageWindow {
                label: "5-hour",
                utilization: 47.0,
                resets_at: None,
                reset_description: Some("resets in 2h 14m".to_owned()),
            }),
            seven_day: Some(UsageWindow {
                label: "7-day",
                utilization: 62.0,
                resets_at: None,
                reset_description: Some("resets in 4d 11h".to_owned()),
            }),
            seven_day_opus: None,
            seven_day_sonnet: None,
            extra_usage: Some(ExtraUsage {
                monthly_limit: Some(20.0),
                used_credits: Some(12.4),
                utilization: Some(62.0),
                currency: Some("USD".to_owned()),
            }),
        });
        app.usage.last_error = Some("Network timeout while refreshing cached data.".to_owned());

        let rendered = render_usage(&app);
        assert!(rendered.contains("5-hour"));
        assert_eq!(rendered.matches("47%").count(), 1);
        assert!(rendered.contains("12.40"));
        assert!(rendered.contains("20.00"));
        assert!(rendered.contains("USD"));
        assert!(rendered.contains("Extra credits"));
        assert!(rendered.contains("Latest refresh error"));
        assert!(!rendered.contains("source:"));

        let rendered_lines = rendered.lines().collect::<Vec<_>>();
        let first_reset_index = rendered_lines
            .iter()
            .position(|line| line.contains("resets in 2h 14m"))
            .expect("reset line");
        assert!(rendered_lines[first_reset_index + 1].trim().is_empty());
    }

    #[test]
    fn extra_usage_wraps_inside_card_on_narrow_widths() {
        let mut app = usage_app();
        app.usage.snapshot = Some(UsageSnapshot {
            source: UsageSourceKind::Oauth,
            fetched_at: SystemTime::now(),
            five_hour: Some(UsageWindow {
                label: "5-hour",
                utilization: 47.0,
                resets_at: None,
                reset_description: Some("resets soon".to_owned()),
            }),
            seven_day: None,
            seven_day_opus: None,
            seven_day_sonnet: None,
            extra_usage: Some(ExtraUsage {
                monthly_limit: Some(100.0),
                used_credits: Some(99.99),
                utilization: Some(99.0),
                currency: Some("USD".to_owned()),
            }),
        });

        let rows = render_usage_rows(&app, 20, 18);
        assert!(rows.iter().any(|row| row.contains("Extra credits")));
        assert!(rows.iter().any(|row| row.contains("99.99")));
        assert!(rows.iter().any(|row| row.contains("USD")));
        assert!(rows.iter().any(|row| row.contains("used")));
    }
}
