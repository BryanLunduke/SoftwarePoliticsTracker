// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use crate::app::{App, FocusOwner, TodoStatus};
use crate::ui::theme;
use ratatui::Frame;
use ratatui::layout::Rect;
use ratatui::style::{Color, Modifier, Style};
use ratatui::text::{Line, Span};
use ratatui::widgets::Paragraph;

/// Maximum visible lines in the expanded todo panel.
const MAX_VISIBLE: usize = 5;

/// Horizontal padding to match header/input/footer inset.
const TODO_PAD: u16 = 2;

/// Compute the height the todo panel needs in the layout.
/// Returns 0 when there are no todos, 1 for the closed compact line,
/// or min(`todo_count`, `MAX_VISIBLE`) for the open panel.
pub fn compute_height(app: &App) -> u16 {
    if app.todos.is_empty() {
        return 0;
    }
    if !app.show_todo_panel {
        // Closed: compact one-line status
        return 1;
    }
    // Open: capped at MAX_VISIBLE
    #[allow(clippy::cast_possible_truncation)]
    {
        app.todos.len().min(MAX_VISIBLE) as u16
    }
}

/// Render the todo panel into the given area.
pub fn render(frame: &mut Frame, area: Rect, app: &mut App) {
    tracing::debug!(
        "todo::render: {} todos, show_panel={}, area={}x{}",
        app.todos.len(),
        app.show_todo_panel,
        area.width,
        area.height
    );
    if app.todos.is_empty() {
        return;
    }

    if app.show_todo_panel {
        render_open(frame, area, app);
    } else {
        render_closed(frame, area, app);
    }
}

/// Closed state: single compact line showing progress and current task.
/// Format: `  [3/7] Running tests`
/// The line is cached on `App` and only rebuilt when `set_todos()` invalidates.
fn render_closed(frame: &mut Frame, area: Rect, app: &mut App) {
    if app.cached_todo_compact.is_none() {
        let completed = app.todos.iter().filter(|t| t.status == TodoStatus::Completed).count();
        let total = app.todos.len();

        let current = app.todos.iter().find(|t| t.status == TodoStatus::InProgress);
        let task_text = match current {
            Some(t) if !t.active_form.is_empty() => t.active_form.clone(),
            Some(t) => t.content.clone(),
            None => {
                if completed == total {
                    "All tasks completed".to_owned()
                } else {
                    app.todos
                        .iter()
                        .find(|t| t.status == TodoStatus::Pending)
                        .map(|t| t.content.clone())
                        .unwrap_or_default()
                }
            }
        };

        app.cached_todo_compact = Some(Line::from(vec![
            Span::styled("[", Style::default().fg(theme::DIM)),
            Span::styled(format!("{completed}/{total}"), Style::default().fg(theme::RUST_ORANGE)),
            Span::styled("] ", Style::default().fg(theme::DIM)),
            Span::styled(task_text, Style::default().fg(Color::White)),
        ]));
    }

    if let Some(line) = &app.cached_todo_compact {
        let padded = Rect {
            x: area.x.saturating_add(TODO_PAD),
            y: area.y,
            width: area.width.saturating_sub(TODO_PAD * 2),
            height: area.height,
        };
        frame.render_widget(Paragraph::new(line.clone()), padded);
    }
}

/// Open state: full list with status icons, scrollable when > `MAX_VISIBLE`.
fn render_open(frame: &mut Frame, area: Rect, app: &mut App) {
    let total = app.todos.len();
    let visible = (area.height as usize).min(total);
    let todo_has_focus = app.focus_owner() == FocusOwner::TodoList;

    // Clamp scroll offset
    let max_scroll = total.saturating_sub(visible);
    if app.todo_scroll > max_scroll {
        app.todo_scroll = max_scroll;
    }

    // Keep the active row visible:
    // - todo focus: selected row
    // - input focus: in-progress row
    let active_idx = if todo_has_focus {
        app.todo_selected.min(total.saturating_sub(1))
    } else {
        app.todos.iter().position(|t| t.status == TodoStatus::InProgress).unwrap_or(0)
    };
    if active_idx < app.todo_scroll {
        app.todo_scroll = active_idx;
    } else if active_idx >= app.todo_scroll + visible {
        app.todo_scroll = active_idx.saturating_sub(visible.saturating_sub(1));
    }

    let mut lines: Vec<Line<'static>> = Vec::with_capacity(visible);

    for (i, todo) in app.todos.iter().enumerate().skip(app.todo_scroll).take(visible) {
        let (icon, icon_color) = match todo.status {
            TodoStatus::Completed => ("\u{2713}", Color::Green), // ✓
            TodoStatus::InProgress => ("\u{25b8}", theme::RUST_ORANGE), // ▸
            TodoStatus::Pending => ("\u{25cb}", theme::DIM),     // ○
        };

        let mut text_style = match todo.status {
            TodoStatus::Completed => {
                Style::default().fg(theme::DIM).add_modifier(Modifier::CROSSED_OUT)
            }
            TodoStatus::InProgress => {
                Style::default().fg(Color::White).add_modifier(Modifier::BOLD)
            }
            TodoStatus::Pending => Style::default().fg(Color::Gray),
        };
        if todo_has_focus && i == app.todo_selected {
            text_style = text_style.add_modifier(Modifier::REVERSED);
        }

        let display_text = if todo.status == TodoStatus::InProgress && !todo.active_form.is_empty()
        {
            &todo.active_form
        } else {
            &todo.content
        };

        lines.push(Line::from(vec![
            Span::styled(format!("{icon} "), Style::default().fg(icon_color)),
            Span::styled(display_text.clone(), text_style),
        ]));
    }

    let padded = Rect {
        x: area.x.saturating_add(TODO_PAD),
        y: area.y,
        width: area.width.saturating_sub(TODO_PAD * 2),
        height: area.height,
    };
    frame.render_widget(Paragraph::new(lines), padded);
}
