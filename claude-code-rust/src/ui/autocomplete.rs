// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use crate::app::App;
use crate::app::mention::MAX_VISIBLE;
use crate::app::{mention, slash, subagent};
use crate::ui::input;
use crate::ui::theme;
use ratatui::Frame;
use ratatui::layout::Rect;
use ratatui::style::{Modifier, Style};
use ratatui::text::{Line, Span};
use ratatui::widgets::{Block, BorderType, Borders, Paragraph};
use unicode_width::UnicodeWidthChar;

/// Max dropdown width (characters).
const MAX_WIDTH: u16 = 60;
/// Min dropdown width so list entries stay readable.
const MIN_WIDTH: u16 = 20;
/// Vertical gap (in rows) between the trigger line and the dropdown.
const ANCHOR_VERTICAL_GAP: u16 = 1;

enum Dropdown<'a> {
    Mention(&'a mention::MentionState),
    Slash(&'a slash::SlashState),
    Subagent(&'a subagent::SubagentState),
}

struct DropdownMeta {
    visible_count: usize,
    start: usize,
    end: usize,
    title: String,
}

pub fn is_active(app: &App) -> bool {
    app.mention.is_some()
        || app.slash.as_ref().is_some_and(|s| !s.candidates.is_empty())
        || app.subagent.as_ref().is_some_and(|s| !s.candidates.is_empty())
}

#[allow(clippy::cast_possible_truncation)]
pub fn compute_height(app: &App) -> u16 {
    let count = if let Some(m) = &app.mention {
        m.candidates.len().max(1)
    } else if let Some(s) = &app.slash {
        s.candidates.len()
    } else if let Some(s) = &app.subagent {
        s.candidates.len()
    } else {
        0
    };

    if count == 0 {
        0
    } else {
        let visible = count.min(MAX_VISIBLE) as u16;
        visible.saturating_add(2) // +2 for top/bottom border
    }
}

/// Render the autocomplete dropdown as a floating overlay above the input area.
#[allow(clippy::cast_possible_truncation)]
pub fn render(frame: &mut Frame, input_area: Rect, app: &App) {
    let Some(dropdown) = active_dropdown(app) else {
        return;
    };

    let height = compute_height(app);
    if height == 0 {
        return;
    }

    let text_area = input::compute_render_geometry(input_area, input::hint_line_count(app)).text;
    if text_area.width == 0 || text_area.height == 0 {
        return;
    }

    let (trigger_row, trigger_col) = dropdown_trigger(&dropdown);
    let (anchor_row, anchor_col) =
        wrapped_visual_pos(app.input.lines(), trigger_row, trigger_col, text_area.width);

    let anchor_x = text_area.x.saturating_add(anchor_col).min(text_area.right().saturating_sub(1));
    let (x, width) = choose_dropdown_x(anchor_x, text_area.x, text_area.right(), text_area.width);
    if width == 0 {
        return;
    }

    let anchor_y = text_area.y.saturating_add(anchor_row).min(text_area.bottom().saturating_sub(1));
    let y = choose_dropdown_y(anchor_y, height, frame.area().y, frame.area().bottom());

    let dropdown_area = Rect { x, y, width, height };
    let meta = dropdown_meta(&dropdown);
    let lines = dropdown_lines(&dropdown, &meta);

    let block = Block::default()
        .title(Span::styled(meta.title, Style::default().fg(theme::DIM)))
        .borders(Borders::ALL)
        .border_type(BorderType::Rounded)
        .border_style(Style::default().fg(theme::DIM));

    let paragraph = Paragraph::new(lines).block(block);
    frame.render_widget(ratatui::widgets::Clear, dropdown_area);
    frame.render_widget(paragraph, dropdown_area);
}

fn active_dropdown(app: &App) -> Option<Dropdown<'_>> {
    if let Some(m) = &app.mention {
        return Some(Dropdown::Mention(m));
    }
    if let Some(s) = &app.slash
        && !s.candidates.is_empty()
    {
        return Some(Dropdown::Slash(s));
    }
    if let Some(s) = &app.subagent
        && !s.candidates.is_empty()
    {
        return Some(Dropdown::Subagent(s));
    }
    None
}

fn dropdown_trigger(dropdown: &Dropdown<'_>) -> (usize, usize) {
    match dropdown {
        Dropdown::Mention(m) => (m.trigger_row, m.trigger_col),
        Dropdown::Slash(s) => (s.trigger_row, s.trigger_col),
        Dropdown::Subagent(s) => (s.trigger_row, s.trigger_col),
    }
}

fn dropdown_meta(dropdown: &Dropdown<'_>) -> DropdownMeta {
    match dropdown {
        Dropdown::Mention(m) => {
            let visible_count = m.candidates.len().clamp(1, MAX_VISIBLE);
            let (start, end) = if m.candidates.is_empty() {
                (0, 0)
            } else {
                m.dialog.visible_range(m.candidates.len(), MAX_VISIBLE)
            };
            DropdownMeta { visible_count, start, end, title: " Files & Folders ".to_owned() }
        }
        Dropdown::Slash(s) => {
            let visible_count = s.candidates.len().min(MAX_VISIBLE);
            let (start, end) = s.dialog.visible_range(s.candidates.len(), MAX_VISIBLE);
            let title = match &s.context {
                slash::SlashContext::CommandName => format!(" Commands ({}) ", s.candidates.len()),
                slash::SlashContext::Argument { command, .. } => {
                    format!(" {} Args ({}) ", command, s.candidates.len())
                }
            };
            DropdownMeta { visible_count, start, end, title }
        }
        Dropdown::Subagent(s) => {
            let visible_count = s.candidates.len().min(MAX_VISIBLE);
            let (start, end) = s.dialog.visible_range(s.candidates.len(), MAX_VISIBLE);
            DropdownMeta {
                visible_count,
                start,
                end,
                title: format!(" Subagents ({}) ", s.candidates.len()),
            }
        }
    }
}

fn dropdown_lines(dropdown: &Dropdown<'_>, meta: &DropdownMeta) -> Vec<Line<'static>> {
    let mut lines: Vec<Line<'static>> = Vec::with_capacity(meta.visible_count);
    match dropdown {
        Dropdown::Mention(m) => {
            if m.candidates.is_empty() {
                lines.push(mention_placeholder_line(m));
            } else {
                for (i, candidate) in m.candidates[meta.start..meta.end].iter().enumerate() {
                    lines.push(mention_candidate_line(m, candidate, meta.start + i));
                }
            }
        }
        Dropdown::Slash(s) => {
            for (i, candidate) in s.candidates[meta.start..meta.end].iter().enumerate() {
                lines.push(slash_candidate_line(s, candidate, meta.start + i));
            }
        }
        Dropdown::Subagent(s) => {
            for (i, candidate) in s.candidates[meta.start..meta.end].iter().enumerate() {
                lines.push(subagent_candidate_line(s, candidate, meta.start + i));
            }
        }
    }
    lines
}

fn mention_placeholder_line(mention: &mention::MentionState) -> Line<'static> {
    let message = mention.placeholder_message().unwrap_or_default();
    Line::from(Span::styled(format!("   {message}"), Style::default().fg(theme::DIM)))
}

fn mention_candidate_line(
    mention: &mention::MentionState,
    candidate: &mention::FileCandidate,
    global_idx: usize,
) -> Line<'static> {
    let mut spans: Vec<Span<'static>> = Vec::new();
    push_selection_prefix(&mut spans, global_idx == mention.dialog.selected);

    let path = &candidate.rel_path;
    let query = &mention.query;
    if query.is_empty() {
        spans.push(Span::raw(path.clone()));
    } else if let Some((match_start, match_end)) = find_case_insensitive_range(path, query) {
        push_highlighted_text(&mut spans, path, match_start, match_end);
    } else {
        spans.push(Span::raw(path.clone()));
    }

    Line::from(spans)
}

fn slash_candidate_line(
    slash: &slash::SlashState,
    candidate: &slash::SlashCandidate,
    global_idx: usize,
) -> Line<'static> {
    let mut spans: Vec<Span<'static>> = Vec::new();
    push_selection_prefix(&mut spans, global_idx == slash.dialog.selected);

    if slash.query.is_empty() {
        spans.push(Span::raw(candidate.primary.clone()));
    } else if matches!(slash.context, slash::SlashContext::CommandName) {
        let command_name = &candidate.primary;
        let command_body = command_name.strip_prefix('/').unwrap_or(command_name);
        if let Some((match_start, match_end)) =
            find_case_insensitive_range(command_body, &slash.query)
        {
            let prefix_len = command_name.len().saturating_sub(command_body.len());
            let start_idx = prefix_len + match_start;
            let end_idx = prefix_len + match_end;
            push_highlighted_text(&mut spans, command_name, start_idx, end_idx);
        } else {
            spans.push(Span::raw(command_name.clone()));
        }
    } else if let Some((match_start, match_end)) =
        find_case_insensitive_range(&candidate.primary, &slash.query)
    {
        push_highlighted_text(&mut spans, &candidate.primary, match_start, match_end);
    } else {
        spans.push(Span::raw(candidate.primary.clone()));
    }

    if let Some(secondary) = &candidate.secondary {
        spans.push(Span::styled("  ", Style::default().fg(theme::DIM)));
        spans.push(Span::styled(secondary.clone(), Style::default().fg(theme::DIM)));
    }

    Line::from(spans)
}

fn subagent_candidate_line(
    subagent: &subagent::SubagentState,
    candidate: &subagent::SubagentCandidate,
    global_idx: usize,
) -> Line<'static> {
    let mut spans: Vec<Span<'static>> = Vec::new();
    push_selection_prefix(&mut spans, global_idx == subagent.dialog.selected);

    let primary = format!("&{}", candidate.name);
    if subagent.query.is_empty() {
        spans.push(Span::raw(primary));
    } else if let Some((match_start, match_end)) =
        find_case_insensitive_range(&candidate.name, &subagent.query)
    {
        push_highlighted_text(&mut spans, &primary, match_start + 1, match_end + 1);
    } else {
        spans.push(Span::raw(primary));
    }

    let secondary = match (&candidate.description, &candidate.model) {
        (desc, Some(model)) if !desc.trim().is_empty() => Some(format!("{desc} | model: {model}")),
        (desc, None) if !desc.trim().is_empty() => Some(desc.clone()),
        (_, Some(model)) => Some(format!("model: {model}")),
        _ => None,
    };
    if let Some(secondary) = secondary {
        spans.push(Span::styled("  ", Style::default().fg(theme::DIM)));
        spans.push(Span::styled(secondary, Style::default().fg(theme::DIM)));
    }

    Line::from(spans)
}

fn push_selection_prefix(spans: &mut Vec<Span<'static>>, is_selected: bool) {
    if is_selected {
        spans.push(Span::styled(
            " \u{25b8} ",
            Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD),
        ));
    } else {
        spans.push(Span::raw("   "));
    }
}

#[derive(Clone, Copy)]
struct FoldSegment {
    fold_start: usize,
    fold_end: usize,
    orig_start: usize,
    orig_end: usize,
}

fn find_case_insensitive_range(haystack: &str, needle: &str) -> Option<(usize, usize)> {
    if needle.is_empty() || haystack.is_empty() {
        return None;
    }

    let folded_needle = needle.to_lowercase();
    if folded_needle.is_empty() {
        return None;
    }

    let mut folded_haystack = String::new();
    let mut segments: Vec<FoldSegment> = Vec::with_capacity(haystack.chars().count());
    for (orig_start, ch) in haystack.char_indices() {
        let orig_end = orig_start + ch.len_utf8();
        let fold_start = folded_haystack.len();
        for lower_ch in ch.to_lowercase() {
            folded_haystack.push(lower_ch);
        }
        let fold_end = folded_haystack.len();
        segments.push(FoldSegment { fold_start, fold_end, orig_start, orig_end });
    }

    let folded_match_start = folded_haystack.find(&folded_needle)?;
    let folded_match_end = folded_match_start + folded_needle.len();
    let start_seg = segments
        .iter()
        .find(|seg| seg.fold_start <= folded_match_start && folded_match_start < seg.fold_end)?;
    let end_probe = folded_match_end.saturating_sub(1);
    let end_seg = segments
        .iter()
        .find(|seg| seg.fold_start <= end_probe && end_probe < seg.fold_end)
        .unwrap_or(start_seg);

    Some((start_seg.orig_start, end_seg.orig_end))
}

fn push_highlighted_text(
    spans: &mut Vec<Span<'static>>,
    text: &str,
    match_start: usize,
    match_end: usize,
) {
    let before = &text[..match_start];
    let matched = &text[match_start..match_end];
    let after = &text[match_end..];

    if !before.is_empty() {
        spans.push(Span::raw(before.to_owned()));
    }
    spans.push(Span::styled(
        matched.to_owned(),
        Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD),
    ));
    if !after.is_empty() {
        spans.push(Span::raw(after.to_owned()));
    }
}

fn choose_dropdown_x(
    anchor_x: u16,
    area_left: u16,
    area_right: u16,
    text_area_width: u16,
) -> (u16, u16) {
    if area_right <= area_left || text_area_width == 0 {
        return (area_left, 0);
    }

    let preferred_width = text_area_width.clamp(1, MAX_WIDTH);
    let width =
        if text_area_width >= MIN_WIDTH { preferred_width.max(MIN_WIDTH) } else { preferred_width };

    let anchor_x = anchor_x.clamp(area_left, area_right.saturating_sub(1));
    let mut x = anchor_x;
    if x.saturating_add(width) > area_right {
        x = area_right.saturating_sub(width);
    }
    x = x.max(area_left);

    (x, width)
}

#[allow(clippy::cast_possible_truncation)]
fn wrapped_visual_pos(
    lines: &[String],
    target_row: usize,
    target_col: usize,
    width: u16,
) -> (u16, u16) {
    let width = width as usize;
    if width == 0 {
        return (0, 0);
    }

    let mut visual_row: u16 = 0;
    for (row, line) in lines.iter().enumerate() {
        let mut col_width: usize = 0;
        let mut char_idx: usize = 0;

        if row == target_row && target_col == 0 {
            return (visual_row, 0);
        }

        for ch in line.chars() {
            if row == target_row && char_idx == target_col {
                return (visual_row, col_width as u16);
            }

            let w = UnicodeWidthChar::width(ch).unwrap_or(0);
            if w > 0 && col_width + w > width && col_width > 0 {
                visual_row = visual_row.saturating_add(1);
                col_width = 0;
            }

            if w > width && col_width == 0 {
                visual_row = visual_row.saturating_add(1);
                char_idx += 1;
                continue;
            }

            if w > 0 {
                col_width += w;
            }
            char_idx += 1;
        }

        if row == target_row && char_idx == target_col {
            if col_width >= width {
                return (visual_row.saturating_add(1), 0);
            }
            return (visual_row, col_width as u16);
        }

        visual_row = visual_row.saturating_add(1);
    }

    (visual_row, 0)
}

fn choose_dropdown_y(anchor_y: u16, height: u16, frame_top: u16, frame_bottom: u16) -> u16 {
    if height == 0 || frame_bottom <= frame_top {
        return frame_top;
    }

    let below_y = anchor_y.saturating_add(1).saturating_add(ANCHOR_VERTICAL_GAP);
    let rows_below_with_gap = frame_bottom.saturating_sub(below_y);
    let fits_below_with_gap = height <= rows_below_with_gap;

    let above_y = anchor_y.saturating_sub(height.saturating_add(ANCHOR_VERTICAL_GAP));
    let rows_above_with_gap =
        anchor_y.saturating_sub(frame_top.saturating_add(ANCHOR_VERTICAL_GAP));
    let fits_above_with_gap = height <= rows_above_with_gap;

    let mut y = if fits_below_with_gap {
        below_y
    } else if fits_above_with_gap {
        above_y
    } else if rows_below_with_gap >= rows_above_with_gap {
        anchor_y.saturating_add(1)
    } else {
        anchor_y.saturating_sub(height)
    };

    let max_y = frame_bottom.saturating_sub(height);
    y = y.clamp(frame_top, max_y);

    let overlaps_anchor = y <= anchor_y && anchor_y < y.saturating_add(height);
    if overlaps_anchor {
        let can_place_below = anchor_y.saturating_add(1).saturating_add(height) <= frame_bottom;
        let can_place_above = frame_top.saturating_add(height) <= anchor_y;
        if can_place_below {
            y = anchor_y.saturating_add(1);
        } else if can_place_above {
            y = anchor_y.saturating_sub(height);
        }
    }

    y.clamp(frame_top, max_y)
}

#[cfg(test)]
mod tests {
    use super::{
        choose_dropdown_x, choose_dropdown_y, compute_height, find_case_insensitive_range,
        is_active,
    };
    use crate::app::{App, mention};

    #[test]
    fn dropdown_keeps_preferred_width_and_shifts_left_near_right_edge() {
        let (x, width) = choose_dropdown_x(78, 0, 80, 80);
        assert_eq!((x, width), (20, 60));
    }

    #[test]
    fn dropdown_handles_tiny_area_by_shrinking_width() {
        let (x, width) = choose_dropdown_x(7, 5, 10, 5);
        assert_eq!((x, width), (5, 5));
    }

    #[test]
    fn dropdown_keeps_anchor_when_room_is_available() {
        let (x, width) = choose_dropdown_x(12, 0, 80, 80);
        assert_eq!((x, width), (12, 60));
    }

    #[test]
    fn dropdown_prefers_below_with_gap_when_space_available() {
        let y = choose_dropdown_y(10, 4, 0, 30);
        assert_eq!(y, 12);
    }

    #[test]
    fn dropdown_uses_above_with_gap_when_below_too_small() {
        let y = choose_dropdown_y(9, 6, 0, 12);
        assert_eq!(y, 2);
    }

    #[test]
    fn dropdown_does_not_cover_anchor_row_when_possible() {
        let anchor = 5;
        let height = 5;
        let y = choose_dropdown_y(anchor, height, 0, 11);
        assert!(!(y <= anchor && anchor < y + height));
    }

    #[test]
    fn case_insensitive_range_respects_utf8_boundaries() {
        let haystack = "İstanbul";
        let (start, end) =
            find_case_insensitive_range(haystack, "i").expect("case-insensitive match");
        assert!(haystack.is_char_boundary(start));
        assert!(haystack.is_char_boundary(end));
        assert_eq!(&haystack[start..end], "İ");
    }

    #[test]
    fn empty_mention_still_renders_placeholder_dropdown() {
        let mut app = App::test_default();
        app.input.set_text("@");
        let _ = app.input.set_cursor(0, 1);
        mention::activate(&mut app);

        assert!(is_active(&app));
        assert_eq!(compute_height(&app), 3);
    }
}
