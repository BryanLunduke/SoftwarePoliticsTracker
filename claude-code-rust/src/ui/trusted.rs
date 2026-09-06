use crate::app::App;
use crate::app::TrustSelection;
use ratatui::Frame;
use ratatui::layout::{Constraint, Direction, Layout, Margin};
use ratatui::style::{Modifier, Style};
use ratatui::text::{Line, Span, Text};
use ratatui::widgets::{Block, Borders, Paragraph, Wrap};

use super::theme;

pub fn render(frame: &mut Frame, app: &mut App) {
    let area = frame.area();
    app.cached_frame_area = area;

    let outer = Block::default()
        .borders(Borders::ALL)
        .title("Unknown Project")
        .border_style(Style::default().fg(theme::DIM));
    frame.render_widget(outer, area);

    let inner = area.inner(Margin { vertical: 1, horizontal: 2 });
    let body_intro = vec![
        Line::from("Claude Rust will wait here until you choose whether to trust this workspace."),
        Line::default(),
        Line::from("Review the project before continuing if you are unsure."),
    ];
    let message = app
        .trust
        .last_error
        .clone()
        .unwrap_or_else(|| "Choose Yes to continue or No to close Claude Rust.".to_owned());
    let title_height = wrapped_line_count(
        Text::from(vec![Line::from(vec![Span::styled(
            "Trust this project directory?",
            Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD),
        )])]),
        inner.width,
    );
    let body_height = wrapped_line_count(Text::from(body_intro.clone()), inner.width);
    let message_height = wrapped_line_count(
        Text::from(vec![Line::from(Span::styled(message.clone(), Style::default()))]),
        inner.width,
    );
    let actions_height = wrapped_line_count(Text::from(action_lines(app)), inner.width);
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(title_height),
            Constraint::Length(body_height),
            Constraint::Length(message_height),
            Constraint::Min(3),
        ])
        .split(inner);

    frame.render_widget(
        Paragraph::new(Line::from(vec![Span::styled(
            "Trust this project directory?",
            Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD),
        )]))
        .wrap(Wrap { trim: false }),
        chunks[0],
    );

    frame.render_widget(Paragraph::new(body_intro).wrap(Wrap { trim: false }), chunks[1]);

    let message_style = if app.trust.last_error.is_some() {
        Style::default().fg(theme::STATUS_ERROR)
    } else {
        Style::default().fg(theme::DIM)
    };
    frame.render_widget(
        Paragraph::new(Line::from(Span::styled(message, message_style))).wrap(Wrap { trim: false }),
        chunks[2],
    );

    let action_constraints = [Constraint::Length(actions_height), Constraint::Min(0)];
    let action_chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints(action_constraints)
        .split(chunks[3]);
    frame.render_widget(
        Paragraph::new(action_lines(app)).wrap(Wrap { trim: false }),
        action_chunks[0],
    );
}

fn wrapped_line_count(text: Text<'static>, width: u16) -> u16 {
    u16::try_from(Paragraph::new(text).wrap(Wrap { trim: false }).line_count(width))
        .unwrap_or(u16::MAX)
        .max(1)
}

fn action_lines(app: &App) -> Vec<Line<'static>> {
    vec![
        action_line("Yes", app.trust.selection == TrustSelection::Yes),
        action_line("No", app.trust.selection == TrustSelection::No),
    ]
}

fn action_line(label: &str, selected: bool) -> Line<'static> {
    let marker = if selected { ">" } else { " " };
    let style = if selected {
        Style::default().fg(ratatui::style::Color::White).bg(theme::RUST_ORANGE)
    } else {
        Style::default().fg(theme::DIM)
    }
    .add_modifier(Modifier::BOLD);

    Line::from(Span::styled(format!("{marker} {label}"), style))
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::app::App;
    use ratatui::Terminal;
    use ratatui::backend::TestBackend;

    fn draw_rows(app: &mut App, width: u16, height: u16) -> Vec<String> {
        let backend = TestBackend::new(width, height);
        let mut terminal = Terminal::new(backend).expect("terminal");
        terminal.draw(|frame| render(frame, app)).expect("draw");
        terminal
            .backend()
            .buffer()
            .content
            .chunks(usize::from(width))
            .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
            .collect()
    }

    fn draw_text(app: &mut App) -> String {
        draw_rows(app, 70, 14).join("\n")
    }

    #[test]
    fn trusted_view_shows_selection_at_top_without_storage_details() {
        let mut app = App::test_default();
        app.trust.selection = TrustSelection::Yes;
        app.cwd_raw = r"C:\work\project".to_owned();

        let text = draw_text(&mut app);

        assert!(text.contains("Unknown Project"));
        assert!(!text.contains("~/.claude.json"));
        assert!(!text.contains("hasTrustDialogAccepted"));
        assert!(!text.contains("Directory:"));
        assert!(!text.contains(r"C:\work\project"));
    }

    #[test]
    fn trusted_view_highlights_no_when_selected() {
        let mut app = App::test_default();
        app.trust.selection = TrustSelection::No;

        let text = draw_text(&mut app);

        assert!(text.contains("  Yes"));
        assert!(text.contains("> No"));
    }

    #[test]
    fn trusted_view_renders_actions_below_body_text() {
        let mut app = App::test_default();
        app.trust.selection = TrustSelection::Yes;

        let text = draw_text(&mut app);
        let body_idx = text.find("Claude Rust will wait here").expect("body text");
        let action_idx = text.find("> Yes").expect("yes action");

        assert!(action_idx > body_idx);
    }

    #[test]
    fn trusted_view_wraps_title_on_narrow_widths() {
        let mut app = App::test_default();
        let rows = draw_rows(&mut app, 18, 16);

        assert!(rows.iter().any(|row| row.contains("Trust this")));
        assert!(rows.iter().any(|row| row.contains("directory?")));
    }
}
