// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use crate::agent::model;
use crate::app::{App, MessageBlock, MessageRole};
use ratatui::Frame;
use ratatui::layout::{Alignment, Constraint, Layout, Rect};
use ratatui::style::{Color, Style};
use ratatui::text::{Line, Span};
use ratatui::widgets::Paragraph;
use unicode_width::{UnicodeWidthChar, UnicodeWidthStr};

use super::theme;

const FOOTER_PAD: u16 = 2;
const FOOTER_COLUMN_GAP: u16 = 1;
const PRIMARY_ROW_LEFT_MIN_WIDTH: u16 = 24;
const SECONDARY_ROW_LEFT_MIN_WIDTH: u16 = 28;
const MIN_CONTEXT_LOCATION_WIDTH: usize = 10;
const MIN_CONTEXT_BRANCH_WIDTH: usize = 4;
type FooterItem = Option<(String, Color)>;
const FOOTER_CONTEXT_VALUE: Color = Color::Gray;

pub fn render(frame: &mut Frame, area: Rect, app: &mut App) {
    if area.height == 0 {
        return;
    }

    let padded = Rect {
        x: area.x.saturating_add(FOOTER_PAD),
        y: area.y,
        width: area.width.saturating_sub(FOOTER_PAD * 2),
        height: area.height,
    };

    let [first_row, second_row] =
        Layout::vertical([Constraint::Length(1), Constraint::Length(1)]).areas(padded);

    let first_line = build_primary_line(app);
    render_footer_row(
        frame,
        first_row,
        first_line,
        footer_update_hint(app),
        PRIMARY_ROW_LEFT_MIN_WIDTH,
    );

    let second_hint = footer_mcp_auth_hint(app);
    let (second_left, second_right) = split_footer_columns_hint(
        second_row,
        second_hint.as_ref().map(|(text, _)| text.as_str()),
        SECONDARY_ROW_LEFT_MIN_WIDTH,
    );
    frame.render_widget(
        Paragraph::new(build_context_line(app, usize::from(second_left.width))),
        second_left,
    );
    if let Some((hint_text, hint_color)) = second_hint {
        render_footer_right_info(frame, second_right, &hint_text, hint_color);
    }
}

fn footer_update_hint(app: &App) -> FooterItem {
    let permission_count = pending_permission_request_count(app);
    if permission_count > 0 {
        return Some((format!("{permission_count} PEND. PERM."), Color::Yellow));
    }
    app.update_check_hint.as_ref().map(|hint| (hint.clone(), theme::RUST_ORANGE))
}

fn footer_mcp_auth_hint(app: &App) -> FooterItem {
    let needs_auth_count = mcp_needs_auth_count(app);
    (needs_auth_count > 0 && should_show_startup_mcp_hint(app))
        .then(|| (format!("{needs_auth_count} MCP NEEDS AUTH"), Color::Yellow))
}

fn render_footer_row(
    frame: &mut Frame,
    area: Rect,
    left_line: Line<'static>,
    right_hint: FooterItem,
    left_min_width: u16,
) {
    let (left_area, right_area) = split_footer_columns_hint(
        area,
        right_hint.as_ref().map(|(text, _)| text.as_str()),
        left_min_width,
    );
    frame.render_widget(Paragraph::new(left_line), left_area);
    if let Some((hint_text, hint_color)) = right_hint {
        render_footer_right_info(frame, right_area, &hint_text, hint_color);
    }
}

fn split_footer_columns_hint(
    area: Rect,
    right_text: Option<&str>,
    left_min_width: u16,
) -> (Rect, Rect) {
    if area.width == 0 {
        return (area, zero_width_rect(area));
    }

    let Some(right_text) = right_text else {
        return (area, zero_width_rect(area));
    };

    let left_min_width = left_min_width.min(area.width);
    let available_right =
        area.width.saturating_sub(left_min_width).saturating_sub(FOOTER_COLUMN_GAP);
    if available_right == 0 {
        return (area, zero_width_rect(area));
    }

    let natural_right_width = u16::try_from(UnicodeWidthStr::width(right_text)).unwrap_or(u16::MAX);
    let right_width = natural_right_width.min(available_right);
    if right_width == 0 {
        return (area, zero_width_rect(area));
    }

    let left_width = area.width.saturating_sub(right_width).saturating_sub(FOOTER_COLUMN_GAP);
    let left = Rect { width: left_width, ..area };
    let right = Rect {
        x: left.x.saturating_add(left_width).saturating_add(FOOTER_COLUMN_GAP),
        width: right_width,
        ..area
    };
    (left, right)
}

fn zero_width_rect(area: Rect) -> Rect {
    Rect { x: area.x.saturating_add(area.width), width: 0, ..area }
}

fn build_primary_line(app: &App) -> Line<'static> {
    if let Some(ref mode) = app.mode {
        let color = mode_color(&mode.current_mode_id);
        let (fast_mode_text, fast_mode_color) = fast_mode_badge(app.fast_mode_state);
        Line::from(vec![
            Span::styled("[", Style::default().fg(color)),
            Span::styled(mode.current_mode_name.clone(), Style::default().fg(color)),
            Span::styled("]", Style::default().fg(color)),
            Span::raw("  "),
            Span::styled("[", Style::default().fg(fast_mode_color)),
            Span::styled(fast_mode_text, Style::default().fg(fast_mode_color)),
            Span::styled("]", Style::default().fg(fast_mode_color)),
            Span::raw("  "),
            Span::styled("?", Style::default().fg(Color::White)),
            Span::styled(" : Help", Style::default().fg(theme::DIM)),
        ])
    } else {
        Line::from(vec![
            Span::styled("?", Style::default().fg(Color::White)),
            Span::styled(" : Help", Style::default().fg(theme::DIM)),
        ])
    }
}

fn fit_footer_right_text(text: &str, max_width: usize) -> Option<String> {
    if max_width == 0 || text.trim().is_empty() {
        return None;
    }

    if UnicodeWidthStr::width(text) <= max_width {
        return Some(text.to_owned());
    }

    if max_width <= 3 {
        return Some(".".repeat(max_width));
    }

    let mut fitted = String::new();
    let mut width: usize = 0;
    for ch in text.chars() {
        let ch_width = UnicodeWidthChar::width(ch).unwrap_or(0);
        if width.saturating_add(ch_width).saturating_add(3) > max_width {
            break;
        }
        fitted.push(ch);
        width = width.saturating_add(ch_width);
    }

    if fitted.is_empty() {
        return Some("...".to_owned());
    }
    fitted.push_str("...");
    Some(fitted)
}

fn render_footer_right_info(frame: &mut Frame, area: Rect, right_text: &str, right_color: Color) {
    if area.width == 0 {
        return;
    }
    let Some(fitted) = fit_footer_right_text(right_text, usize::from(area.width)) else {
        return;
    };

    let line = Line::from(Span::styled(fitted, Style::default().fg(right_color)));
    frame.render_widget(Paragraph::new(line).alignment(Alignment::Right), area);
}

fn build_context_line(app: &App, max_width: usize) -> Line<'static> {
    let Some((location_value, branch_value)) = context_values(app, max_width) else {
        return Line::default();
    };

    let mut spans = vec![
        Span::styled("Loc: ", Style::default().fg(theme::DIM)),
        Span::styled(location_value, Style::default().fg(FOOTER_CONTEXT_VALUE)),
    ];

    if let Some(branch_value) = branch_value {
        spans.push(Span::styled("  |  ", Style::default().fg(theme::DIM)));
        spans.push(Span::styled("Branch: ", Style::default().fg(theme::DIM)));
        spans.push(Span::styled(branch_value, Style::default().fg(FOOTER_CONTEXT_VALUE)));
    }

    Line::from(spans)
}

fn context_values(app: &App, max_width: usize) -> Option<(String, Option<String>)> {
    const LOCATION_LABEL_WIDTH: usize = 5;
    const CONTEXT_SEPARATOR_WIDTH: usize = 5;
    const BRANCH_LABEL_WIDTH: usize = 8;

    let location_only_width = max_width.saturating_sub(LOCATION_LABEL_WIDTH);
    let branch = app.git_branch.as_deref().filter(|branch| !branch.is_empty());

    if let Some(branch) = branch {
        let fixed_width = LOCATION_LABEL_WIDTH + CONTEXT_SEPARATOR_WIDTH + BRANCH_LABEL_WIDTH;
        let available_values = max_width.saturating_sub(fixed_width);
        if available_values >= MIN_CONTEXT_LOCATION_WIDTH + MIN_CONTEXT_BRANCH_WIDTH {
            let branch_width = UnicodeWidthStr::width(branch)
                .min(available_values.saturating_sub(MIN_CONTEXT_LOCATION_WIDTH));
            let branch_value = fit_footer_right_text(branch, branch_width);
            let branch_display_width =
                branch_value.as_ref().map_or(0, |value| UnicodeWidthStr::width(value.as_str()));
            let location_width = available_values.saturating_sub(branch_display_width);
            if let Some(location_value) = fit_location_value(&app.cwd, location_width) {
                return Some((location_value, branch_value));
            }
        }
    }

    fit_location_value(&app.cwd, location_only_width).map(|location_value| (location_value, None))
}

fn fit_location_value(cwd: &str, max_width: usize) -> Option<String> {
    if max_width == 0 {
        return None;
    }

    for candidate in location_candidates(cwd) {
        if UnicodeWidthStr::width(candidate.as_str()) <= max_width {
            return Some(candidate);
        }
    }

    fit_footer_suffix_text(cwd, max_width)
}

fn location_candidates(cwd: &str) -> Vec<String> {
    let mut candidates = Vec::new();
    push_unique(&mut candidates, Some(cwd.to_owned()));
    push_unique(&mut candidates, trailing_path_components(cwd, 2));
    push_unique(&mut candidates, trailing_path_components(cwd, 1));
    candidates
}

fn trailing_path_components(path: &str, count: usize) -> Option<String> {
    let separator = if path.contains('\\') { "\\" } else { "/" };
    let components: Vec<&str> = path
        .split(['/', '\\'])
        .filter(|component| !component.is_empty() && *component != "~")
        .collect();
    if components.is_empty() {
        return None;
    }
    let start = components.len().saturating_sub(count);
    Some(components[start..].join(separator))
}

fn push_unique(candidates: &mut Vec<String>, candidate: Option<String>) {
    let Some(candidate) = candidate else {
        return;
    };
    if !candidate.is_empty() && !candidates.iter().any(|existing| existing == &candidate) {
        candidates.push(candidate);
    }
}

fn fit_footer_suffix_text(text: &str, max_width: usize) -> Option<String> {
    if max_width == 0 || text.trim().is_empty() {
        return None;
    }

    if UnicodeWidthStr::width(text) <= max_width {
        return Some(text.to_owned());
    }

    if max_width <= 3 {
        return Some(".".repeat(max_width));
    }

    let mut fitted = String::new();
    let mut width = 0usize;
    for ch in text.chars().rev() {
        let ch_width = UnicodeWidthChar::width(ch).unwrap_or(0);
        if width.saturating_add(ch_width).saturating_add(3) > max_width {
            break;
        }
        fitted.insert(0, ch);
        width = width.saturating_add(ch_width);
    }

    if fitted.is_empty() {
        return Some("...".to_owned());
    }

    Some(format!("...{fitted}"))
}

fn pending_permission_request_count(app: &App) -> usize {
    app.pending_interaction_ids
        .iter()
        .filter(|tool_id| {
            let Some((mi, bi)) = app.lookup_tool_call(tool_id) else {
                return false;
            };
            matches!(
                app.messages.get(mi).and_then(|msg| msg.blocks.get(bi)),
                Some(MessageBlock::ToolCall(tc)) if tc.pending_permission.is_some()
            )
        })
        .count()
}

fn mcp_needs_auth_count(app: &App) -> usize {
    app.mcp
        .servers
        .iter()
        .filter(|server| {
            matches!(server.status, crate::agent::types::McpServerConnectionStatus::NeedsAuth)
        })
        .count()
}

fn should_show_startup_mcp_hint(app: &App) -> bool {
    !app.messages
        .iter()
        .any(|message| matches!(message.role, MessageRole::User | MessageRole::Assistant))
}

fn mode_color(mode_id: &str) -> Color {
    match mode_id {
        "default" => theme::DIM,
        "plan" => Color::Blue,
        "acceptEdits" => Color::Yellow,
        "bypassPermissions" | "dontAsk" => Color::Red,
        _ => Color::Magenta,
    }
}

fn fast_mode_badge(state: model::FastModeState) -> (&'static str, Color) {
    match state {
        model::FastModeState::Off => ("FAST:OFF", theme::DIM),
        model::FastModeState::Cooldown => ("FAST:CD", Color::Yellow),
        model::FastModeState::On => ("FAST:ON", theme::RUST_ORANGE),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::agent::model;
    use crate::agent::types::{McpServerConnectionStatus, McpServerStatus};
    use crate::app::{
        App, BlockCache, ChatMessage, InlinePermission, MessageBlock, MessageRole,
        TerminalSnapshotMode, TextBlock, ToolCallInfo,
    };
    use tokio::sync::oneshot;

    #[test]
    fn split_footer_columns_hint_left_gets_its_minimum() {
        let area = Rect::new(0, 0, 80, 1);
        let left_min = 24u16;
        let (left, right) = split_footer_columns_hint(area, Some("1 PEND. PERM."), left_min);
        assert_eq!(left.width + FOOTER_COLUMN_GAP + right.width, 80);
        assert!(left.width >= left_min);
    }

    #[test]
    fn split_footer_columns_hint_reserves_natural_right_width() {
        let area = Rect::new(0, 0, 80, 1);
        let left_min = 24u16;
        let right_text = "1 PEND. PERM.";
        let (left, right) = split_footer_columns_hint(area, Some(right_text), left_min);
        assert_eq!(right.width, u16::try_from(UnicodeWidthStr::width(right_text)).unwrap());
        assert_eq!(left.width + FOOTER_COLUMN_GAP + right.width, 80);
    }

    #[test]
    fn split_footer_columns_hint_zero_width() {
        let area = Rect::new(0, 0, 0, 1);
        let (left, right) = split_footer_columns_hint(area, Some("hint"), 24);
        assert_eq!(left.width, 0);
        assert_eq!(right.width, 0);
    }

    #[test]
    fn split_footer_columns_hint_drops_right_when_left_min_cannot_be_preserved() {
        let area = Rect::new(0, 0, 24, 1);
        let (left, right) = split_footer_columns_hint(area, Some("1 MCP NEEDS AUTH"), 24);
        assert_eq!(left.width, 24);
        assert_eq!(right.width, 0);
    }

    #[test]
    fn fit_footer_right_text_truncates_when_needed() {
        let text = "Update available: v9.9.9 (current v0.2.0)";
        let fitted = fit_footer_right_text(text, 12).expect("fitted text");
        assert!(fitted.ends_with("..."));
        assert!(UnicodeWidthStr::width(fitted.as_str()) <= 12);
    }

    #[test]
    fn fit_footer_right_text_keeps_prefix() {
        let text = "Compacting context now and applying update hint";
        let fitted = fit_footer_right_text(text, 20).expect("fitted text");
        assert!(fitted.starts_with("Compacting"));
        assert!(UnicodeWidthStr::width(fitted.as_str()) <= 20);
    }

    #[test]
    fn fit_footer_suffix_text_keeps_path_tail() {
        let text = "~/work/company/claude_rust";
        let fitted = fit_footer_suffix_text(text, 14).expect("fitted text");
        assert!(fitted.starts_with("..."));
        assert!(fitted.ends_with("claude_rust"));
        assert!(UnicodeWidthStr::width(fitted.as_str()) <= 14);
    }

    #[test]
    fn footer_update_hint_none_without_hint() {
        let app = App::test_default();
        assert_eq!(footer_update_hint(&app), None);
    }

    #[test]
    fn footer_update_hint_returns_text_when_present() {
        let mut app = App::test_default();
        app.update_check_hint = Some("Update available".to_owned());
        assert_eq!(
            footer_update_hint(&app),
            Some(("Update available".to_owned(), theme::RUST_ORANGE))
        );
    }

    #[test]
    fn footer_update_hint_prefers_pending_permission_count() {
        let mut app = App::test_default();
        app.update_check_hint = Some("Update available".to_owned());
        let (response_tx, _response_rx) = oneshot::channel();
        app.messages.push(ChatMessage {
            role: MessageRole::Assistant,
            blocks: vec![MessageBlock::ToolCall(Box::new(ToolCallInfo {
                id: "perm-1".into(),
                title: "Read".into(),
                sdk_tool_name: "Read".into(),
                raw_input: None,
                raw_input_bytes: 0,
                output_metadata: None,
                status: model::ToolCallStatus::Pending,
                content: vec![],
                hidden: false,
                terminal_id: None,
                terminal_command: None,
                terminal_output: None,
                terminal_output_len: 0,
                terminal_bytes_seen: 0,
                terminal_snapshot_mode: TerminalSnapshotMode::AppendOnly,
                render_epoch: 0,
                layout_epoch: 0,
                last_measured_width: 0,
                last_measured_height: 0,
                last_measured_layout_epoch: 0,
                last_measured_layout_generation: 0,
                cache: BlockCache::default(),
                pending_permission: Some(InlinePermission {
                    options: vec![],
                    response_tx,
                    selected_index: 0,
                    focused: true,
                }),
                pending_question: None,
            }))],
            usage: None,
        });
        app.index_tool_call("perm-1".into(), 0, 0);
        app.pending_interaction_ids.push("perm-1".into());

        assert_eq!(footer_update_hint(&app), Some(("1 PEND. PERM.".to_owned(), Color::Yellow)));
    }

    #[test]
    fn fast_mode_badge_maps_cooldown_to_cd() {
        let (label, _) = fast_mode_badge(model::FastModeState::Cooldown);
        assert_eq!(label, "FAST:CD");
    }

    #[test]
    fn context_line_includes_loc_only_without_branch() {
        let mut app = App::test_default();
        app.cwd = "~/repo".into();

        let text: String =
            build_context_line(&app, 80).spans.iter().map(|span| span.content.as_ref()).collect();
        assert_eq!(text, "Loc: ~/repo");
    }

    #[test]
    fn context_line_includes_branch_when_present() {
        let mut app = App::test_default();
        app.cwd = "~/repo".into();
        app.git_branch = Some("main".into());

        let text: String =
            build_context_line(&app, 80).spans.iter().map(|span| span.content.as_ref()).collect();
        assert_eq!(text, "Loc: ~/repo  |  Branch: main");
    }

    #[test]
    fn context_line_shortens_location_before_dropping_branch() {
        let mut app = App::test_default();
        app.cwd = "~/work/company/claude_rust".into();
        app.git_branch = Some("feature/footer".into());

        let text: String =
            build_context_line(&app, 46).spans.iter().map(|span| span.content.as_ref()).collect();
        assert!(text.contains("Branch:"));
        assert!(text.starts_with("Loc: "));
        assert!(!text.contains("~/work/company/claude_rust"));
    }

    #[test]
    fn context_line_drops_branch_when_width_is_too_tight() {
        let mut app = App::test_default();
        app.cwd = "~/work/company/claude_rust".into();
        app.git_branch = Some("feature/footer".into());

        let text: String =
            build_context_line(&app, 24).spans.iter().map(|span| span.content.as_ref()).collect();
        assert!(text.starts_with("Loc: "));
        assert!(!text.contains("Branch:"));
    }

    #[test]
    fn mcp_auth_hint_shows_needs_auth_count_before_real_chat() {
        let mut app = App::test_default();
        app.messages.push(ChatMessage {
            role: MessageRole::Welcome,
            blocks: vec![MessageBlock::Text(TextBlock::from_complete("welcome"))],
            usage: None,
        });
        app.mcp.servers.push(McpServerStatus {
            name: "calendar".into(),
            status: McpServerConnectionStatus::NeedsAuth,
            server_info: None,
            error: None,
            config: None,
            scope: None,
            tools: vec![],
        });

        assert_eq!(
            footer_mcp_auth_hint(&app),
            Some(("1 MCP NEEDS AUTH".to_owned(), Color::Yellow))
        );
    }

    #[test]
    fn mcp_auth_hint_hides_after_assistant_message() {
        let mut app = App::test_default();
        app.messages.push(ChatMessage {
            role: MessageRole::Assistant,
            blocks: vec![MessageBlock::Text(TextBlock::from_complete("hello"))],
            usage: None,
        });
        app.mcp.servers.push(McpServerStatus {
            name: "calendar".into(),
            status: McpServerConnectionStatus::NeedsAuth,
            server_info: None,
            error: None,
            config: None,
            scope: None,
            tools: vec![],
        });

        assert_eq!(footer_mcp_auth_hint(&app), None);
    }
}
