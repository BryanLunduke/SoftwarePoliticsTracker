// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use crate::app::{App, AppStatus, FocusOwner, HelpView};
use crate::ui::theme;
use ratatui::Frame;
use ratatui::layout::Constraint;
use ratatui::layout::Rect;
use ratatui::style::{Modifier, Style};
use ratatui::text::{Line, Span, Text};
use ratatui::widgets::{Block, BorderType, Borders, Cell, Row, Table};
use unicode_width::UnicodeWidthStr;

const COLUMN_GAP: usize = 4;
/// Content lines available in the help panel (excluding padding and borders).
const MAX_ROWS: usize = 10;
const HELP_VERTICAL_PADDING_LINES: usize = 1;
const SUBAGENT_NAME_MIN_WIDTH: usize = 12;
const SUBAGENT_NAME_MAX_WIDTH: usize = 28;
const SUBAGENT_NAME_MAX_SHARE_NUM: usize = 2;
const SUBAGENT_NAME_MAX_SHARE_DEN: usize = 5;
const HELP_PANEL_HEIGHT: u16 = 14;

pub fn is_active(app: &App) -> bool {
    app.is_help_active()
}

/// Returns the number of items in the current help tab (for key navigation).
pub fn help_item_count(app: &App) -> usize {
    build_help_items(app).len()
}

#[allow(clippy::cast_possible_truncation)]
pub fn compute_height(app: &App, _area_width: u16) -> u16 {
    if !is_active(app) {
        return 0;
    }
    HELP_PANEL_HEIGHT
}

pub(crate) fn sync_geometry_state(app: &mut App, panel_width: u16) {
    if !is_active(app) || panel_width == 0 {
        app.help_visible_count = 0;
        return;
    }

    let items = build_help_items(app);
    let visible_count = visible_count_for_view(app, &items, panel_width);
    if matches!(app.help_view, HelpView::SlashCommands | HelpView::Subagents) {
        app.help_dialog.clamp(items.len(), visible_count);
    }
    app.help_visible_count = visible_count;
}

#[allow(clippy::cast_possible_truncation)]
pub fn render(frame: &mut Frame, area: Rect, app: &mut App) {
    if area.height == 0 || area.width == 0 || !is_active(app) {
        return;
    }

    let items = build_help_items(app);
    if items.is_empty() {
        return;
    }

    match app.help_view {
        HelpView::Keys => render_keys_help(frame, area, app, &items),
        HelpView::SlashCommands | HelpView::Subagents => {
            render_two_column_help(frame, area, app, &items);
        }
    }
}

#[allow(clippy::cast_possible_truncation)]
fn render_keys_help(frame: &mut Frame, area: Rect, app: &App, items: &[(String, String)]) {
    let rows = items.len().div_ceil(2).min(MAX_ROWS);
    let max_items = rows * 2;
    let items = &items[..items.len().min(max_items)];
    let inner_width = area.width.saturating_sub(2) as usize;
    let col_width = (inner_width.saturating_sub(COLUMN_GAP)) / 2;
    let left_width = col_width;
    let right_width = col_width;

    let mut table_rows: Vec<Row<'static>> =
        Vec::with_capacity(rows + HELP_VERTICAL_PADDING_LINES * 2);

    for _ in 0..HELP_VERTICAL_PADDING_LINES {
        table_rows.push(Row::new(vec![Cell::from(Line::default()), Cell::from(Line::default())]));
    }

    for row in 0..rows {
        let left_idx = row;
        let right_idx = row + rows;

        let left = items.get(left_idx).cloned().unwrap_or_default();
        let right = items.get(right_idx).cloned().unwrap_or_default();

        let left_lines = format_item_cell_lines(&left, left_width);
        let right_lines = format_item_cell_lines(&right, right_width);
        let row_height = left_lines.len().max(right_lines.len()).max(1);

        table_rows.push(
            Row::new(vec![Cell::from(Text::from(left_lines)), Cell::from(Text::from(right_lines))])
                .height(row_height as u16),
        );
    }

    for _ in 0..HELP_VERTICAL_PADDING_LINES {
        table_rows.push(Row::new(vec![Cell::from(Line::default()), Cell::from(Line::default())]));
    }

    let block = Block::default()
        .title(help_title(app.help_view))
        .borders(Borders::ALL)
        .border_type(BorderType::Rounded);

    let table = Table::new(
        table_rows,
        [Constraint::Length(left_width as u16), Constraint::Length(right_width as u16)],
    )
    .column_spacing(COLUMN_GAP as u16)
    .block(block);

    frame.render_widget(table, area);
}

#[allow(clippy::cast_possible_truncation)]
fn render_two_column_help(
    frame: &mut Frame,
    area: Rect,
    app: &mut App,
    items: &[(String, String)],
) {
    let inner_width = area.width.saturating_sub(2) as usize;
    let (name_width, desc_width) = help_item_column_widths(items, inner_width);
    let visible_count = visible_count_for_view(app, items, area.width);

    let start = app.help_dialog.scroll_offset;
    let end = (start + visible_count).min(items.len());
    let visible_items = &items[start..end];
    let selected = app.help_dialog.selected;

    // Capacity: items + spacers between items + vertical padding.
    let mut table_rows: Vec<Row<'static>> = Vec::with_capacity(
        visible_count + visible_count.saturating_sub(1) + HELP_VERTICAL_PADDING_LINES * 2,
    );

    for _ in 0..HELP_VERTICAL_PADDING_LINES {
        table_rows.push(Row::new(vec![Cell::from(Line::default()), Cell::from(Line::default())]));
    }

    for (view_index, (name, description)) in visible_items.iter().enumerate() {
        let abs_index = start + view_index;
        let is_selected = abs_index == selected;

        let name_style = if is_selected {
            Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD)
        } else {
            Style::default().add_modifier(Modifier::BOLD)
        };
        let desc_style =
            if is_selected { Style::default().fg(theme::RUST_ORANGE) } else { Style::default() };

        let name_lines = wrap_text_lines_styled(name, name_width, name_style);
        let desc_lines = wrap_text_lines_styled(description, desc_width, desc_style);
        let row_height = name_lines.len().max(desc_lines.len()).max(1);

        table_rows.push(
            Row::new(vec![Cell::from(Text::from(name_lines)), Cell::from(Text::from(desc_lines))])
                .height(row_height as u16),
        );

        // Spacer row between items for readability.
        if view_index + 1 < visible_count {
            table_rows.push(
                Row::new(vec![Cell::from(Line::default()), Cell::from(Line::default())]).height(1),
            );
        }
    }

    for _ in 0..HELP_VERTICAL_PADDING_LINES {
        table_rows.push(Row::new(vec![Cell::from(Line::default()), Cell::from(Line::default())]));
    }

    let block = Block::default()
        .title(help_title(app.help_view))
        .borders(Borders::ALL)
        .border_type(BorderType::Rounded);

    let table = Table::new(
        table_rows,
        [Constraint::Length(name_width as u16), Constraint::Length(desc_width as u16)],
    )
    .column_spacing(COLUMN_GAP as u16)
    .block(block);

    frame.render_widget(table, area);
}

fn visible_count_for_view(app: &App, items: &[(String, String)], panel_width: u16) -> usize {
    if items.is_empty() || panel_width == 0 {
        return 0;
    }

    match app.help_view {
        HelpView::Keys => items.len().div_ceil(2).min(MAX_ROWS),
        HelpView::SlashCommands | HelpView::Subagents => {
            let inner_width = panel_width.saturating_sub(2) as usize;
            let (name_width, desc_width) = help_item_column_widths(items, inner_width);
            compute_visible_count(
                items,
                app.help_dialog.scroll_offset,
                MAX_ROWS,
                name_width,
                desc_width,
            )
        }
    }
}

fn build_help_items(app: &App) -> Vec<(String, String)> {
    match app.help_view {
        HelpView::Keys => build_key_help_items(app),
        HelpView::SlashCommands => build_slash_help_items(app),
        HelpView::Subagents => build_subagent_help_items(app),
    }
}

fn build_key_help_items(app: &App) -> Vec<(String, String)> {
    if app.status == AppStatus::Connecting {
        let mut items = blocked_input_help_items("Unavailable while connecting");
        if app.update_check_hint.is_some() {
            items.push(("Ctrl+u".to_owned(), "Hide update hint".to_owned()));
        }
        return items;
    }
    if app.status == AppStatus::CommandPending {
        let mut items = blocked_input_help_items(&format!(
            "Unavailable while command runs ({})",
            pending_command_help_label(app)
        ));
        if app.update_check_hint.is_some() {
            items.push(("Ctrl+u".to_owned(), "Hide update hint".to_owned()));
        }
        return items;
    }
    if app.status == AppStatus::Error {
        let mut items = blocked_input_help_items("Unavailable after error");
        if app.update_check_hint.is_some() {
            items.push(("Ctrl+u".to_owned(), "Hide update hint".to_owned()));
        }
        return items;
    }

    let mut items: Vec<(String, String)> = vec![
        // Global
        ("Ctrl+c".to_owned(), "Quit".to_owned()),
        ("Ctrl+q".to_owned(), "Quit".to_owned()),
        ("Ctrl+l".to_owned(), "Redraw screen".to_owned()),
        ("Shift+Tab".to_owned(), "Cycle mode".to_owned()),
        ("Ctrl+o".to_owned(), "Toggle tool collapse".to_owned()),
        ("Ctrl+t".to_owned(), "Toggle todos (when available)".to_owned()),
        // Chat scrolling
        ("Ctrl+Up/Down".to_owned(), "Scroll chat".to_owned()),
        ("Mouse wheel".to_owned(), "Scroll chat".to_owned()),
    ];
    if app.update_check_hint.is_some() {
        items.push(("Ctrl+u".to_owned(), "Hide update hint".to_owned()));
    }
    if app.is_compacting {
        items.push(("Status".to_owned(), "Compacting context".to_owned()));
    }
    let focus_owner = app.focus_owner();

    if app.show_todo_panel && !app.todos.is_empty() {
        items.push(("Tab".to_owned(), "Toggle todo focus".to_owned()));
    }

    // Input + navigation (active outside todo-list and mention focus)
    if focus_owner != FocusOwner::TodoList
        && focus_owner != FocusOwner::Mention
        && focus_owner != FocusOwner::Help
    {
        items.push(("Enter".to_owned(), "Send message".to_owned()));
        items.push(("Shift+Enter".to_owned(), "Insert newline".to_owned()));
        items.push(("Up/Down".to_owned(), "Move cursor / scroll chat".to_owned()));
        items.push(("Left/Right".to_owned(), "Move cursor".to_owned()));
        items.push(("Ctrl+Left/Right".to_owned(), "Word left/right".to_owned()));
        items.push(("Home/End".to_owned(), "Line start/end".to_owned()));
        items.push(("Backspace".to_owned(), "Delete before".to_owned()));
        items.push(("Delete".to_owned(), "Delete after".to_owned()));
        items.push(("Ctrl+Backspace/Delete".to_owned(), "Delete word".to_owned()));
        items.push(("Ctrl+z/y".to_owned(), "Undo/redo".to_owned()));
        items.push(("Paste".to_owned(), "Insert text".to_owned()));
    }

    // Turn control
    if matches!(app.status, crate::app::AppStatus::Thinking | crate::app::AppStatus::Running) {
        items.push(("Esc".to_owned(), "Cancel current turn".to_owned()));
    } else if focus_owner == FocusOwner::TodoList {
        items.push(("Esc".to_owned(), "Exit todo focus".to_owned()));
    } else {
        items.push(("Esc".to_owned(), "No-op (idle)".to_owned()));
    }

    // Inline interactions (permissions or questions)
    if !app.pending_interaction_ids.is_empty() && focus_owner == FocusOwner::Permission {
        if app.pending_interaction_ids.len() > 1 {
            items.push(("Up/Down".to_owned(), "Switch prompt focus".to_owned()));
        }
        if focused_question_prompt(app) {
            items.push(("Left/Right".to_owned(), "Move selection".to_owned()));
            items.push(("Tab".to_owned(), "Toggle notes editor".to_owned()));
            items.push(("Enter".to_owned(), "Confirm answer".to_owned()));
            items.push(("Esc".to_owned(), "Cancel prompt".to_owned()));
        } else {
            items.push(("Left/Right".to_owned(), "Select option".to_owned()));
            items.push(("Enter".to_owned(), "Confirm option".to_owned()));
            items.push(("Ctrl+y/a/n".to_owned(), "Quick select".to_owned()));
            items.push(("Esc".to_owned(), "Reject".to_owned()));
        }
    }
    if focus_owner == FocusOwner::TodoList {
        items.push(("Up/Down".to_owned(), "Select todo (todo focus)".to_owned()));
    }

    items
}

fn focused_question_prompt(app: &App) -> bool {
    let Some(tool_id) = app.pending_interaction_ids.first() else {
        return false;
    };
    let Some((mi, bi)) = app.lookup_tool_call(tool_id) else {
        return false;
    };
    let Some(crate::app::MessageBlock::ToolCall(tc)) =
        app.messages.get(mi).and_then(|message| message.blocks.get(bi))
    else {
        return false;
    };
    tc.pending_question.is_some()
}

fn blocked_input_help_items(input_line: &str) -> Vec<(String, String)> {
    vec![
        ("?".to_owned(), "Toggle help".to_owned()),
        ("Ctrl+c".to_owned(), "Quit".to_owned()),
        ("Ctrl+q".to_owned(), "Quit".to_owned()),
        ("Up/Down".to_owned(), "Scroll chat".to_owned()),
        ("Ctrl+Up/Down".to_owned(), "Scroll chat".to_owned()),
        ("Mouse wheel".to_owned(), "Scroll chat".to_owned()),
        ("Ctrl+l".to_owned(), "Redraw screen".to_owned()),
        ("Input keys".to_owned(), input_line.to_owned()),
    ]
}

fn pending_command_help_label(app: &App) -> String {
    app.pending_command_label.clone().unwrap_or_else(|| "Processing command...".to_owned())
}

fn builtin_slash_help_commands() -> [(&'static str, &'static str); 7] {
    [
        ("/config", "Open settings"),
        ("/login", "Authenticate with Claude"),
        ("/logout", "Sign out of Claude"),
        ("/mcp", "Open MCP"),
        ("/plugins", "Open plugins"),
        ("/status", "Show session status"),
        ("/usage", "Open usage"),
    ]
}

fn build_slash_help_items(app: &App) -> Vec<(String, String)> {
    use std::collections::BTreeMap;

    let mut rows = Vec::new();
    if app.status == AppStatus::Connecting {
        rows.push(("Loading commands...".to_owned(), String::new()));
        return rows;
    }
    if app.status == AppStatus::CommandPending {
        rows.push((pending_command_help_label(app), String::new()));
        return rows;
    }

    let mut commands: BTreeMap<String, String> = builtin_slash_help_commands()
        .into_iter()
        .map(|(name, description)| (name.to_owned(), description.to_owned()))
        .collect();

    for cmd in &app.available_commands {
        let name =
            if cmd.name.starts_with('/') { cmd.name.clone() } else { format!("/{}", cmd.name) };
        match commands.get_mut(&name) {
            Some(existing) if !cmd.description.trim().is_empty() => {
                existing.clone_from(&cmd.description);
            }
            Some(_) => {}
            None => {
                commands.insert(name, cmd.description.clone());
            }
        }
    }

    if commands.is_empty() {
        rows.push((
            "No slash commands advertised".to_owned(),
            "Not advertised in this session".to_owned(),
        ));
        return rows;
    }

    for (name, desc) in commands {
        let description =
            if desc.trim().is_empty() { "No description provided".to_owned() } else { desc };
        rows.push((name, description));
    }

    rows
}

fn build_subagent_help_items(app: &App) -> Vec<(String, String)> {
    let mut rows = Vec::new();
    if app.status == AppStatus::Connecting {
        rows.push(("Loading subagents...".to_owned(), String::new()));
        return rows;
    }
    if app.status == AppStatus::CommandPending {
        rows.push((pending_command_help_label(app), String::new()));
        return rows;
    }

    let mut agents: Vec<(String, String)> = app
        .available_agents
        .iter()
        .filter(|agent| !agent.name.trim().is_empty())
        .map(|agent| {
            let description = if agent.description.trim().is_empty() {
                "No description provided".to_owned()
            } else {
                agent.description.clone()
            };
            let label = match &agent.model {
                Some(model) if !model.trim().is_empty() => {
                    format!("&{}\nModel: {}", agent.name, model.trim())
                }
                _ => format!("&{}", agent.name),
            };
            (label, description)
        })
        .collect();

    agents.sort_by(|a, b| a.0.cmp(&b.0));
    agents.dedup_by(|a, b| a.0 == b.0);
    if agents.is_empty() {
        rows.push((
            "No subagents advertised".to_owned(),
            "Not advertised in this session".to_owned(),
        ));
        return rows;
    }

    rows.extend(agents);
    rows
}

/// Count how many terminal lines `text` wraps into at the given column `width`.
/// Uses the same splitting logic as `take_prefix_by_width` / `wrap_text_lines_styled`.
fn wrapped_line_count(text: &str, width: usize) -> usize {
    if width == 0 || text.is_empty() {
        return 1;
    }
    let mut count = 0;
    for segment in text.split('\n') {
        if segment.is_empty() {
            count += 1;
            continue;
        }
        let mut rest = segment.to_owned();
        while !rest.is_empty() {
            let (chunk, remaining) = take_prefix_by_width(&rest, width);
            if chunk.is_empty() {
                break;
            }
            count += 1;
            rest = remaining;
        }
    }
    count.max(1)
}

/// Compute how many items (starting from `start`) fit within `available_lines`,
/// accounting for each item's actual wrapped height and 1-line spacers between items.
fn compute_visible_count(
    items: &[(String, String)],
    start: usize,
    available_lines: usize,
    name_width: usize,
    desc_width: usize,
) -> usize {
    let mut used = 0;
    let mut count = 0;

    for (name, desc) in items.iter().skip(start) {
        let name_h = wrapped_line_count(name, name_width);
        let desc_h = wrapped_line_count(desc, desc_width);
        let item_h = name_h.max(desc_h).max(1);

        // 1-line spacer before every item except the first.
        let spacer = usize::from(count > 0);

        if used + spacer + item_h > available_lines {
            break;
        }

        used += spacer + item_h;
        count += 1;
    }

    count.max(1)
}

fn help_item_column_widths(items: &[(String, String)], inner_width: usize) -> (usize, usize) {
    if inner_width == 0 {
        return (0, 0);
    }
    if inner_width <= COLUMN_GAP + 1 {
        return (inner_width, 1);
    }

    let max_name_width =
        items.iter().map(|(name, _)| UnicodeWidthStr::width(name.as_str())).max().unwrap_or(0);
    let share_cap =
        inner_width.saturating_mul(SUBAGENT_NAME_MAX_SHARE_NUM) / SUBAGENT_NAME_MAX_SHARE_DEN;
    let min_name_width = SUBAGENT_NAME_MIN_WIDTH.min(share_cap.max(1));
    let preferred_name_width =
        max_name_width.max(min_name_width).min(SUBAGENT_NAME_MAX_WIDTH).min(share_cap.max(1));
    let max_name_fit = inner_width.saturating_sub(COLUMN_GAP + 1);
    let name_width = preferred_name_width.clamp(1, max_name_fit.max(1));
    let desc_width = inner_width.saturating_sub(name_width + COLUMN_GAP).max(1);

    (name_width, desc_width)
}

fn wrap_text_lines_styled(text: &str, width: usize, style: Style) -> Vec<Line<'static>> {
    if width == 0 || text.is_empty() {
        return vec![Line::default()];
    }

    let mut lines = Vec::new();
    for segment in text.split('\n') {
        if segment.is_empty() {
            lines.push(Line::default());
            continue;
        }

        let mut rest = segment.to_owned();
        while !rest.is_empty() {
            let (chunk, remaining) = take_prefix_by_width(&rest, width);
            if chunk.is_empty() {
                break;
            }
            lines.push(Line::from(Span::styled(chunk, style)));
            rest = remaining;
        }
    }

    if lines.is_empty() { vec![Line::default()] } else { lines }
}

fn help_title(view: HelpView) -> Line<'static> {
    let keys_style = if matches!(view, HelpView::Keys) {
        Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD)
    } else {
        Style::default().fg(theme::DIM)
    };
    let slash_style = if matches!(view, HelpView::SlashCommands) {
        Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD)
    } else {
        Style::default().fg(theme::DIM)
    };
    let subagent_style = if matches!(view, HelpView::Subagents) {
        Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD)
    } else {
        Style::default().fg(theme::DIM)
    };

    let hint = if matches!(view, HelpView::SlashCommands | HelpView::Subagents) {
        "  (< > tabs  \u{25b2}\u{25bc} scroll)"
    } else {
        "  (< > switch tabs)"
    };

    Line::from(vec![
        Span::styled(
            " Help ",
            Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD),
        ),
        Span::styled("[", Style::default().fg(theme::DIM)),
        Span::styled("Keys", keys_style),
        Span::styled(" | ", Style::default().fg(theme::DIM)),
        Span::styled("Slash", slash_style),
        Span::styled(" | ", Style::default().fg(theme::DIM)),
        Span::styled("Subagents", subagent_style),
        Span::styled("]", Style::default().fg(theme::DIM)),
        Span::styled(hint, Style::default().fg(theme::DIM)),
    ])
}

fn format_item_cell_lines(item: &(String, String), width: usize) -> Vec<Line<'static>> {
    let (label, desc) = item;
    if width == 0 {
        return vec![Line::default()];
    }
    if label.is_empty() && desc.is_empty() {
        return vec![Line::default()];
    }

    let label = truncate_to_width(label, width);
    let label_width = UnicodeWidthStr::width(label.as_str());
    let sep = " : ";
    let sep_width = UnicodeWidthStr::width(sep);

    if desc.is_empty() {
        return vec![Line::from(Span::styled(
            label,
            Style::default().add_modifier(Modifier::BOLD),
        ))];
    }

    let mut lines: Vec<Line<'static>> = Vec::new();
    let mut rest = desc.to_owned();

    if label_width + sep_width < width {
        let first_desc_width = width - label_width - sep_width;
        let (first_chunk, remaining) = take_prefix_by_width(&rest, first_desc_width);
        lines.push(Line::from(vec![
            Span::styled(label, Style::default().add_modifier(Modifier::BOLD)),
            Span::styled(sep.to_owned(), Style::default().fg(theme::DIM)),
            Span::raw(first_chunk),
        ]));
        rest = remaining;
    } else {
        lines.push(Line::from(Span::styled(label, Style::default().add_modifier(Modifier::BOLD))));
    }

    while !rest.is_empty() {
        let (chunk, remaining) = take_prefix_by_width(&rest, width);
        if chunk.is_empty() {
            break;
        }
        lines.push(Line::raw(chunk));
        rest = remaining;
    }

    if lines.is_empty() { vec![Line::default()] } else { lines }
}

fn take_prefix_by_width(text: &str, width: usize) -> (String, String) {
    if width == 0 || text.is_empty() {
        return (String::new(), text.to_owned());
    }

    let mut used = 0usize;
    let mut split_at = 0usize;
    for (idx, ch) in text.char_indices() {
        let w = unicode_width::UnicodeWidthChar::width(ch).unwrap_or(0);
        if used + w > width {
            break;
        }
        used += w;
        split_at = idx + ch.len_utf8();
    }

    if split_at == 0 {
        return (String::new(), text.to_owned());
    }

    (text[..split_at].to_owned(), text[split_at..].to_owned())
}

fn truncate_to_width(text: &str, width: usize) -> String {
    if width == 0 {
        return String::new();
    }
    if UnicodeWidthStr::width(text) <= width {
        return text.to_owned();
    }
    let mut out = String::new();
    let mut used = 0usize;
    for ch in text.chars() {
        let w = unicode_width::UnicodeWidthChar::width(ch).unwrap_or(0);
        if used + w > width {
            break;
        }
        out.push(ch);
        used += w;
    }
    out
}

#[cfg(test)]
mod tests {
    use super::build_help_items;
    use crate::app::{App, AppStatus, FocusTarget, HelpView, TodoItem, TodoStatus};

    fn has_item(items: &[(String, String)], key: &str, desc: &str) -> bool {
        items.iter().any(|(k, d)| k == key && d == desc)
    }

    #[test]
    fn tab_toggle_only_shown_when_todos_available() {
        let mut app = App::test_default();
        let items = build_help_items(&app);
        assert!(!has_item(&items, "Tab", "Toggle todo focus"));

        app.show_todo_panel = true;
        app.todos.push(TodoItem {
            content: "Task".into(),
            status: TodoStatus::Pending,
            active_form: String::new(),
        });
        let items = build_help_items(&app);
        assert!(has_item(&items, "Tab", "Toggle todo focus"));
    }

    #[test]
    fn key_tab_does_not_show_removed_header_shortcut() {
        let app = App::test_default();
        let items = build_help_items(&app);
        assert!(!has_item(&items, "Ctrl+h", "Toggle header"));
    }

    #[test]
    fn key_tab_shows_ctrl_u_only_when_update_hint_visible() {
        let mut app = App::test_default();
        let items = build_help_items(&app);
        assert!(!has_item(&items, "Ctrl+u", "Hide update hint"));

        app.update_check_hint = Some("Update available".into());
        let items = build_help_items(&app);
        assert!(has_item(&items, "Ctrl+u", "Hide update hint"));
    }

    #[test]
    fn permission_navigation_only_shown_when_permission_has_focus() {
        let mut app = App::test_default();
        app.pending_interaction_ids = vec!["perm-1".into(), "perm-2".into()];

        // Without permission focus claim, do not show permission-only arrows.
        let items = build_help_items(&app);
        assert!(!has_item(&items, "Left/Right", "Select option"));
        assert!(!has_item(&items, "Up/Down", "Switch prompt focus"));

        app.claim_focus_target(FocusTarget::Permission);
        let items = build_help_items(&app);
        assert!(has_item(&items, "Left/Right", "Select option"));
        assert!(has_item(&items, "Up/Down", "Switch prompt focus"));
    }

    #[test]
    fn slash_tab_shows_advertised_commands_with_description() {
        let mut app = App::test_default();
        app.help_view = HelpView::SlashCommands;
        app.available_commands = vec![
            crate::agent::model::AvailableCommand::new("/help", "Open help"),
            crate::agent::model::AvailableCommand::new("memory", ""),
        ];

        let items = build_help_items(&app);
        assert!(has_item(&items, "/help", "Open help"));
        assert!(has_item(&items, "/memory", "No description provided"));
    }

    #[test]
    fn slash_tab_shows_local_auth_and_config_commands_without_advertisement() {
        let mut app = App::test_default();
        app.help_view = HelpView::SlashCommands;

        let items = build_help_items(&app);
        assert!(has_item(&items, "/config", "Open settings"));
        assert!(has_item(&items, "/login", "Authenticate with Claude"));
        assert!(has_item(&items, "/logout", "Sign out of Claude"));
        assert!(has_item(&items, "/mcp", "Open MCP"));
        assert!(has_item(&items, "/usage", "Open usage"));
        assert!(!has_item(
            &items,
            "No slash commands advertised",
            "Not advertised in this session"
        ));
    }

    #[test]
    fn slash_tab_shows_login_logout_when_advertised() {
        let mut app = App::test_default();
        app.help_view = HelpView::SlashCommands;
        app.available_commands = vec![
            crate::agent::model::AvailableCommand::new("/login", "Login"),
            crate::agent::model::AvailableCommand::new("/logout", "Logout"),
        ];

        let items = build_help_items(&app);
        assert!(has_item(&items, "/config", "Open settings"));
        assert!(has_item(&items, "/login", "Login"));
        assert!(has_item(&items, "/logout", "Logout"));
    }

    #[test]
    fn slash_tab_shows_loading_commands_while_connecting() {
        let mut app = App::test_default();
        app.help_view = HelpView::SlashCommands;
        app.status = AppStatus::Connecting;

        let items = build_help_items(&app);
        assert!(has_item(&items, "Loading commands...", ""));
        assert!(!has_item(
            &items,
            "No slash commands advertised",
            "Not advertised in this session"
        ));
    }

    #[test]
    fn slash_tab_does_not_repeat_tab_navigation_hint() {
        let mut app = App::test_default();
        app.help_view = HelpView::SlashCommands;

        let items = build_help_items(&app);
        assert!(!has_item(&items, "Left/Right", "Switch help tab"));
    }

    #[test]
    fn key_tab_connecting_shows_startup_shortcuts_only() {
        let mut app = App::test_default();
        app.status = AppStatus::Connecting;

        let items = build_help_items(&app);
        assert!(has_item(&items, "?", "Toggle help"));
        assert!(has_item(&items, "Ctrl+c", "Quit"));
        assert!(has_item(&items, "Ctrl+q", "Quit"));
        assert!(has_item(&items, "Up/Down", "Scroll chat"));
        assert!(has_item(&items, "Input keys", "Unavailable while connecting"));
        assert!(!has_item(&items, "Enter", "Send message"));
    }

    #[test]
    fn key_tab_error_shows_locked_input_shortcuts() {
        let mut app = App::test_default();
        app.status = AppStatus::Error;

        let items = build_help_items(&app);
        assert!(has_item(&items, "Ctrl+c", "Quit"));
        assert!(has_item(&items, "Ctrl+q", "Quit"));
        assert!(has_item(&items, "Up/Down", "Scroll chat"));
        assert!(has_item(&items, "Input keys", "Unavailable after error"));
        assert!(!has_item(&items, "Enter", "Send message"));
    }

    #[test]
    fn key_tab_does_not_repeat_tab_navigation_hint() {
        let app = App::test_default();
        let items = build_help_items(&app);
        assert!(!has_item(&items, "Left/Right", "Switch help tab"));
    }

    #[test]
    fn subagent_tab_shows_advertised_subagents() {
        let mut app = App::test_default();
        app.help_view = HelpView::Subagents;
        app.available_agents = vec![
            crate::agent::model::AvailableAgent::new("reviewer", "Review code").model("haiku"),
            crate::agent::model::AvailableAgent::new("explore", ""),
        ];

        let items = build_help_items(&app);
        assert!(has_item(&items, "&reviewer\nModel: haiku", "Review code"));
        assert!(has_item(&items, "&explore", "No description provided"));
    }

    #[test]
    fn subagent_tab_shows_loading_while_connecting() {
        let mut app = App::test_default();
        app.help_view = HelpView::Subagents;
        app.status = AppStatus::Connecting;

        let items = build_help_items(&app);
        assert!(has_item(&items, "Loading subagents...", ""));
    }

    #[test]
    fn subagent_tab_does_not_repeat_tab_navigation_hint() {
        let mut app = App::test_default();
        app.help_view = HelpView::Subagents;
        let items = build_help_items(&app);
        assert!(!has_item(&items, "Left/Right", "Switch help tab"));
    }
}
