// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use crate::app::{
    BlockCache, ChatMessage, IncrementalMarkdown, MarkdownRenderKey, MessageBlock, MessageRole,
    SystemSeverity, TextBlock, WelcomeBlock,
};
use crate::ui::tables;
use crate::ui::theme;
use crate::ui::tool_call;
use ratatui::style::{Color, Modifier, Style};
use ratatui::text::{Line, Span, Text};
use ratatui::widgets::{Paragraph, Wrap};

const SPINNER_FRAMES: &[char] = &[
    '\u{280B}', '\u{2819}', '\u{2839}', '\u{2838}', '\u{283C}', '\u{2834}', '\u{2826}', '\u{2827}',
    '\u{2807}', '\u{280F}',
];

const FERRIS_SAYS: &[&str] = &[
    r" --------------------------------- ",
    r"< Welcome back to Claude, in Rust! >",
    r" --------------------------------- ",
    r"        \             ",
    r"         \            ",
    r"            _~^~^~_  ",
    r"        \) /  o o  \ (/",
    r"          '_   -   _' ",
    r"          / '-----' \ ",
];

/// Snapshot of the app state needed by the spinner -- extracted before
/// the message loop so we don't need `&App` (which conflicts with `&mut msg`).
#[derive(Clone, Copy)]
#[allow(clippy::struct_excessive_bools)]
pub struct SpinnerState {
    pub frame: usize,
    /// True when this message owns the currently active assistant turn.
    pub is_active_turn_assistant: bool,
    /// True when this message should show the initial empty-turn thinking indicator.
    pub show_empty_thinking: bool,
    /// True when this message should show the thinking indicator.
    pub show_thinking: bool,
    /// True when this message should show the subagent-thinking indicator.
    pub show_subagent_thinking: bool,
    /// True when this message should show the compaction indicator.
    pub show_compacting: bool,
}

struct MessageLayout {
    segments: Vec<MessageLayoutSegment>,
    height: usize,
    wrapped_lines: usize,
}

impl MessageLayout {
    fn new() -> Self {
        Self { segments: Vec::new(), height: 0, wrapped_lines: 0 }
    }

    fn push_blank(&mut self) {
        self.segments.push(MessageLayoutSegment::Blank);
        self.height += 1;
    }

    fn push_wrapped_line(&mut self, line: Line<'static>, width: u16) {
        self.push_wrapped_lines(vec![line], width);
    }

    fn push_wrapped_lines(&mut self, lines: Vec<Line<'static>>, width: u16) {
        let height = rendered_lines_height(&lines, width);
        self.push_lines(lines, height, height);
    }

    fn push_lines(&mut self, lines: Vec<Line<'static>>, height: usize, wrapped_lines: usize) {
        if height == 0 {
            return;
        }
        self.segments.push(MessageLayoutSegment::Lines { lines, height });
        self.height += height;
        self.wrapped_lines += wrapped_lines;
    }

    fn render_into(self, out: &mut Vec<Line<'static>>) {
        for segment in self.segments {
            segment.render_into(out);
        }
    }
}

enum MessageLayoutSegment {
    Blank,
    Lines { lines: Vec<Line<'static>>, height: usize },
}

impl MessageLayoutSegment {
    fn render_into(self, out: &mut Vec<Line<'static>>) {
        match self {
            Self::Blank => out.push(Line::default()),
            Self::Lines { lines, .. } => out.extend(lines),
        }
    }
}

struct RenderedBlockLayout {
    lines: Vec<Line<'static>>,
    height: usize,
    wrapped_lines: usize,
}

fn assistant_role_label_line() -> Line<'static> {
    let spans = vec![Span::styled(
        "Claude",
        Style::default().fg(theme::ROLE_ASSISTANT).add_modifier(Modifier::BOLD),
    )];

    Line::from(spans)
}

/// Render a single chat message into a `Vec<Line>`, using per-block caches.
/// Takes `&mut` so block caches can be updated.
/// `spinner` is only used for the "Thinking..." animation on empty assistant messages.
#[allow(dead_code)]
pub fn render_message(
    msg: &mut ChatMessage,
    spinner: &SpinnerState,
    width: u16,
    out: &mut Vec<Line<'static>>,
) {
    render_message_with_tools_collapsed(msg, spinner, width, false, out);
}

pub fn render_message_with_tools_collapsed(
    msg: &mut ChatMessage,
    spinner: &SpinnerState,
    width: u16,
    tools_collapsed: bool,
    out: &mut Vec<Line<'static>>,
) {
    render_message_internal(msg, spinner, width, tools_collapsed, true, out);
}

pub fn render_message_with_tools_collapsed_and_separator(
    msg: &mut ChatMessage,
    spinner: &SpinnerState,
    width: u16,
    tools_collapsed: bool,
    include_trailing_separator: bool,
    out: &mut Vec<Line<'static>>,
) {
    render_message_internal(msg, spinner, width, tools_collapsed, include_trailing_separator, out);
}

fn render_message_internal(
    msg: &mut ChatMessage,
    spinner: &SpinnerState,
    width: u16,
    tools_collapsed: bool,
    include_trailing_separator: bool,
    out: &mut Vec<Line<'static>>,
) {
    build_message_layout(
        msg,
        spinner,
        width,
        MessageRenderOptions { tools_collapsed, include_trailing_separator },
        None,
    )
    .render_into(out);
}

fn build_message_layout(
    msg: &mut ChatMessage,
    spinner: &SpinnerState,
    width: u16,
    options: MessageRenderOptions,
    layout_generation: Option<u64>,
) -> MessageLayout {
    let mut layout = MessageLayout::new();
    layout.push_wrapped_line(role_label_line(&msg.role), width);

    match msg.role {
        MessageRole::Welcome => append_welcome_blocks(msg, width, &mut layout),
        MessageRole::User => append_user_blocks(msg, width, &mut layout),
        MessageRole::Assistant => append_assistant_blocks(
            msg,
            spinner,
            width,
            options.tools_collapsed,
            layout_generation,
            &mut layout,
        ),
        MessageRole::System(_) => append_system_blocks(msg, width, &mut layout),
    }

    if options.include_trailing_separator {
        layout.push_blank();
    }

    layout
}

fn append_welcome_blocks(msg: &mut ChatMessage, width: u16, layout: &mut MessageLayout) {
    for block in &mut msg.blocks {
        if let MessageBlock::Welcome(welcome) = block {
            let rendered = welcome_block_layout(welcome, width);
            layout.push_lines(rendered.lines, rendered.height, rendered.wrapped_lines);
        }
    }
}

fn append_user_blocks(msg: &mut ChatMessage, width: u16, layout: &mut MessageLayout) {
    for block in &mut msg.blocks {
        if let MessageBlock::Text(block) = block {
            let trailing_gap = block.trailing_blank_lines();
            let rendered = text_block_layout(block, width, Some(theme::USER_MSG_BG), true);
            layout.push_lines(rendered.lines, rendered.height, rendered.wrapped_lines);
            for _ in 0..trailing_gap {
                layout.push_blank();
            }
        }
    }
}

fn append_assistant_blocks(
    msg: &mut ChatMessage,
    spinner: &SpinnerState,
    width: u16,
    tools_collapsed: bool,
    layout_generation: Option<u64>,
    layout: &mut MessageLayout,
) {
    if msg.blocks.is_empty() && spinner.show_compacting {
        layout.push_wrapped_line(compacting_line(spinner.frame), width);
        return;
    }
    if msg.blocks.is_empty() && spinner.show_empty_thinking {
        layout.push_wrapped_line(thinking_line(spinner.frame), width);
        return;
    }

    let show_compacting = spinner.show_compacting;
    let show_subagent_thinking = spinner.show_subagent_thinking && !show_compacting;
    let mut prev_was_tool = false;
    let mut has_body_content = false;
    let mut has_visible_content = false;
    for block in &mut msg.blocks {
        match block {
            MessageBlock::Text(block) => {
                if prev_was_tool {
                    layout.push_blank();
                }
                let rendered = assistant_text_block_layout(block, width, !has_visible_content);
                let trailing_gap = if !has_visible_content && rendered.height == 0 {
                    0
                } else {
                    block.trailing_blank_lines()
                };
                layout.push_lines(rendered.lines, rendered.height, rendered.wrapped_lines);
                for _ in 0..trailing_gap {
                    layout.push_blank();
                }
                if rendered.height > 0 {
                    has_body_content = true;
                    has_visible_content = true;
                }
                prev_was_tool = false;
            }
            MessageBlock::ToolCall(tc) => {
                let tc = tc.as_mut();
                if tc.hidden {
                    continue;
                }
                if !prev_was_tool && has_body_content {
                    layout.push_blank();
                }
                let mut lines = Vec::new();
                tool_call::render_tool_call_cached_with_tools_collapsed(
                    tc,
                    width,
                    spinner.frame,
                    tools_collapsed,
                    &mut lines,
                );
                let (height, wrapped_lines) = if let Some(layout_generation) = layout_generation {
                    tool_call::measure_tool_call_height_cached_with_tools_collapsed(
                        tc,
                        width,
                        spinner.frame,
                        layout_generation,
                        tools_collapsed,
                    )
                } else {
                    (rendered_lines_height(&lines, width), 0)
                };
                layout.push_lines(lines, height, wrapped_lines);
                if height > 0 {
                    has_body_content = true;
                }
                has_visible_content = true;
                prev_was_tool = true;
            }
            MessageBlock::Welcome(_) => {}
        }
    }

    if show_compacting {
        if has_body_content {
            layout.push_blank();
        }
        layout.push_wrapped_line(compacting_line(spinner.frame), width);
    } else if show_subagent_thinking {
        if has_body_content {
            layout.push_blank();
        }
        layout.push_wrapped_line(subagent_thinking_line(spinner.frame), width);
    }
    if spinner.show_thinking && !show_subagent_thinking && !show_compacting {
        if has_body_content {
            layout.push_blank();
        }
        layout.push_wrapped_line(thinking_line(spinner.frame), width);
    }
}

fn append_system_blocks(msg: &mut ChatMessage, width: u16, layout: &mut MessageLayout) {
    let color = system_severity_color(system_severity_from_role(&msg.role));
    for block in &mut msg.blocks {
        if let MessageBlock::Text(block) = block {
            let trailing_gap = block.trailing_blank_lines();
            let mut rendered = text_block_layout(block, width, None, false);
            tint_lines(&mut rendered.lines, color);
            layout.push_lines(rendered.lines, rendered.height, rendered.wrapped_lines);
            for _ in 0..trailing_gap {
                layout.push_blank();
            }
        }
    }
}

fn system_severity_color(severity: SystemSeverity) -> Color {
    match severity {
        SystemSeverity::Info => theme::DIM,
        SystemSeverity::Warning => theme::STATUS_WARNING,
        SystemSeverity::Error => theme::STATUS_ERROR,
    }
}

fn system_severity_from_role(role: &MessageRole) -> SystemSeverity {
    match role {
        MessageRole::System(level) => level.unwrap_or(SystemSeverity::Error),
        _ => SystemSeverity::Error,
    }
}

/// Measure message height from block caches + width-aware wrapped heights.
/// Returns `(visual_height_rows, lines_wrapped_for_height_updates)`.
///
/// Accuracy is preserved because each block height is computed with
/// `Paragraph::line_count(width)` on the exact rendered `Vec<Line>`.
pub fn measure_message_height_cached(
    msg: &mut ChatMessage,
    spinner: &SpinnerState,
    width: u16,
    layout_generation: u64,
) -> (usize, usize) {
    measure_message_height_cached_with_tools_collapsed(
        msg,
        spinner,
        width,
        layout_generation,
        false,
    )
}

pub fn measure_message_height_cached_with_tools_collapsed(
    msg: &mut ChatMessage,
    spinner: &SpinnerState,
    width: u16,
    layout_generation: u64,
    tools_collapsed: bool,
) -> (usize, usize) {
    measure_message_height_cached_with_tools_collapsed_and_separator(
        msg,
        spinner,
        width,
        layout_generation,
        tools_collapsed,
        true,
    )
}

pub fn measure_message_height_cached_with_tools_collapsed_and_separator(
    msg: &mut ChatMessage,
    spinner: &SpinnerState,
    width: u16,
    layout_generation: u64,
    tools_collapsed: bool,
    include_trailing_separator: bool,
) -> (usize, usize) {
    let layout = build_message_layout(
        msg,
        spinner,
        width,
        MessageRenderOptions { tools_collapsed, include_trailing_separator },
        Some(layout_generation),
    );
    (layout.height, layout.wrapped_lines)
}

/// Render a message while consuming as many whole leading rows as possible.
///
/// `skip_rows` is measured in wrapped visual rows. We skip entire structural parts
/// (label/separators/full blocks) without rendering them. If skipping lands inside
/// a block, that block is rendered in full and the remaining skip is returned so
/// the caller can apply `Paragraph::scroll()` for exact intra-block offset.
#[allow(dead_code)]
pub fn render_message_from_offset(
    msg: &mut ChatMessage,
    spinner: &SpinnerState,
    width: u16,
    layout_generation: u64,
    skip_rows: usize,
    out: &mut Vec<Line<'static>>,
) -> usize {
    render_message_from_offset_with_tools_collapsed(
        msg,
        spinner,
        width,
        layout_generation,
        false,
        skip_rows,
        out,
    )
}

pub fn render_message_from_offset_with_tools_collapsed(
    msg: &mut ChatMessage,
    spinner: &SpinnerState,
    width: u16,
    layout_generation: u64,
    tools_collapsed: bool,
    skip_rows: usize,
    out: &mut Vec<Line<'static>>,
) -> usize {
    render_message_from_offset_internal(
        msg,
        spinner,
        width,
        layout_generation,
        MessageRenderOptions { tools_collapsed, include_trailing_separator: true },
        skip_rows,
        out,
    )
}

pub(crate) fn render_message_from_offset_internal(
    msg: &mut ChatMessage,
    spinner: &SpinnerState,
    width: u16,
    _layout_generation: u64,
    options: MessageRenderOptions,
    skip_rows: usize,
    out: &mut Vec<Line<'static>>,
) -> usize {
    let mut remaining_skip = skip_rows;
    let mut can_consume_skip = true;

    let layout = build_message_layout(msg, spinner, width, options, None);
    render_message_layout_from_offset(layout, out, &mut remaining_skip, &mut can_consume_skip);
    remaining_skip
}

fn render_message_layout_from_offset(
    layout: MessageLayout,
    out: &mut Vec<Line<'static>>,
    remaining_skip: &mut usize,
    can_consume_skip: &mut bool,
) {
    for segment in layout.segments {
        match segment {
            MessageLayoutSegment::Blank => {
                if *can_consume_skip && *remaining_skip > 0 {
                    *remaining_skip -= 1;
                } else {
                    out.push(Line::default());
                }
            }
            MessageLayoutSegment::Lines { lines, height, .. } => {
                if should_skip_whole_block(height, remaining_skip, can_consume_skip) {
                    continue;
                }
                out.extend(lines);
            }
        }
    }
}

#[derive(Clone, Copy)]
pub(crate) struct MessageRenderOptions {
    pub tools_collapsed: bool,
    pub include_trailing_separator: bool,
}

fn rendered_lines_height(lines: &[Line<'static>], width: u16) -> usize {
    if lines.is_empty() {
        return 0;
    }
    Paragraph::new(Text::from(lines.to_vec())).wrap(Wrap { trim: false }).line_count(width)
}

fn welcome_block_layout(block: &mut WelcomeBlock, width: u16) -> RenderedBlockLayout {
    let had_height = block.cache.height_at(width).is_some();
    let mut lines = Vec::new();
    render_welcome_cached(block, width, &mut lines);
    let height = block.cache.height_at(width).unwrap_or_else(|| {
        let height = rendered_lines_height(&lines, width);
        block.cache.set_height(height, width);
        height
    });
    let wrapped_lines = if had_height { 0 } else { lines.len() };
    RenderedBlockLayout { lines, height, wrapped_lines }
}

fn text_block_layout(
    block: &mut TextBlock,
    width: u16,
    bg: Option<Color>,
    preserve_newlines: bool,
) -> RenderedBlockLayout {
    let had_height = block.cache.height_at(width).is_some();
    let mut lines = Vec::new();
    render_text_block_cached(block, width, bg, preserve_newlines, &mut lines);
    let height = block.cache.height_at(width).unwrap_or_else(|| {
        let height = rendered_lines_height(&lines, width);
        block.cache.set_height(height, width);
        height
    });
    let wrapped_lines = if had_height { 0 } else { lines.len() };
    RenderedBlockLayout { lines, height, wrapped_lines }
}

fn assistant_text_block_layout(
    block: &mut TextBlock,
    width: u16,
    trim_leading_blank_lines: bool,
) -> RenderedBlockLayout {
    let mut rendered = text_block_layout(block, width, None, false);

    if trim_leading_blank_lines {
        let leading_blank_lines = count_leading_blank_lines(&rendered.lines);
        if leading_blank_lines > 0 {
            rendered.lines.drain(..leading_blank_lines);
            rendered.height = rendered.height.saturating_sub(leading_blank_lines);
            rendered.wrapped_lines = rendered.wrapped_lines.saturating_sub(leading_blank_lines);
        }
    }

    rendered
}

fn count_leading_blank_lines(lines: &[Line<'static>]) -> usize {
    lines.iter().take_while(|line| line_is_blank(line)).count()
}

fn line_is_blank(line: &Line<'_>) -> bool {
    line.spans.iter().all(|span| span.content.as_ref().chars().all(char::is_whitespace))
}

fn should_skip_whole_block(
    block_h: usize,
    remaining_skip: &mut usize,
    can_consume_skip: &mut bool,
) -> bool {
    if !*can_consume_skip {
        return false;
    }
    if *remaining_skip >= block_h {
        *remaining_skip -= block_h;
        return true;
    }
    if *remaining_skip > 0 {
        // We have to render this block, but keep the remaining intra-block skip
        // for Paragraph::scroll().
        *can_consume_skip = false;
    }
    false
}

fn role_label_line(role: &MessageRole) -> Line<'static> {
    match role {
        MessageRole::Welcome => Line::from(Span::styled(
            "Overview",
            Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD),
        )),
        MessageRole::User => Line::from(Span::styled(
            "User",
            Style::default().fg(theme::DIM).add_modifier(Modifier::BOLD),
        )),
        MessageRole::Assistant => assistant_role_label_line(),
        MessageRole::System(_) => system_role_label_line(system_severity_from_role(role)),
    }
}

fn system_role_label_line(severity: SystemSeverity) -> Line<'static> {
    let (label, color) = match severity {
        SystemSeverity::Info => ("Info", theme::DIM),
        SystemSeverity::Warning => ("Warning", theme::STATUS_WARNING),
        SystemSeverity::Error => ("Error", theme::STATUS_ERROR),
    };
    Line::from(Span::styled(label, Style::default().fg(color).add_modifier(Modifier::BOLD)))
}

fn thinking_line(frame: usize) -> Line<'static> {
    let ch = SPINNER_FRAMES[frame % SPINNER_FRAMES.len()];
    Line::from(Span::styled(format!("{ch} Thinking..."), Style::default().fg(theme::DIM)))
}

fn compacting_line(frame: usize) -> Line<'static> {
    let ch = SPINNER_FRAMES[frame % SPINNER_FRAMES.len()];
    Line::from(Span::styled(
        format!("{ch} Compacting context..."),
        Style::default().fg(theme::RUST_ORANGE),
    ))
}

fn subagent_thinking_line(frame: usize) -> Line<'static> {
    let ch = SPINNER_FRAMES[frame % SPINNER_FRAMES.len()];
    Line::from(vec![
        Span::styled("  \u{2514}\u{2500} ", Style::default().fg(theme::DIM)),
        Span::styled(format!("{ch} Thinking..."), Style::default().fg(theme::DIM)),
    ])
}

fn welcome_lines(block: &WelcomeBlock, _width: u16) -> Vec<Line<'static>> {
    let pad = "  ";
    let mut lines = Vec::new();
    for art_line in FERRIS_SAYS {
        lines.push(Line::from(Span::styled(
            format!("{pad}{art_line}"),
            Style::default().fg(theme::RUST_ORANGE),
        )));
    }

    lines.push(Line::default());
    lines.push(Line::default());

    lines.push(Line::from(vec![
        Span::styled(format!("{pad}Model: "), Style::default().fg(theme::DIM)),
        Span::styled(
            block.model_name.clone(),
            Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD),
        ),
    ]));
    lines.push(Line::from(Span::styled(
        format!("{pad}cwd:   {}", block.cwd),
        Style::default().fg(theme::DIM),
    )));

    lines.push(Line::default());
    lines.push(Line::from(Span::styled(
        format!(
            "{pad}Tips: Enter to send, Shift+Enter for newline, Ctrl+C copies selection or quits"
        ),
        Style::default().fg(theme::DIM),
    )));
    lines.push(Line::default());

    lines
}

fn render_welcome_cached(block: &mut WelcomeBlock, width: u16, out: &mut Vec<Line<'static>>) {
    if let Some(cached_lines) = block.cache.get() {
        out.extend_from_slice(cached_lines);
        return;
    }

    let fresh = welcome_lines(block, width);
    let h = {
        let _t = crate::perf::start_with("msg::wrap_height", "lines", fresh.len());
        Paragraph::new(Text::from(fresh.clone())).wrap(Wrap { trim: false }).line_count(width)
    };
    block.cache.store(fresh);
    block.cache.set_height(h, width);
    if let Some(stored) = block.cache.get() {
        out.extend_from_slice(stored);
    }
}

fn tint_lines(lines: &mut [Line<'static>], color: Color) {
    for line in lines {
        for span in &mut line.spans {
            span.style = span.style.fg(color);
        }
    }
}

/// Preprocess markdown that `tui_markdown` doesn't handle well.
/// Headings (`# Title`) become `**Title**` (bold) with a blank line before.
/// Handles variations: `#Title`, `#  Title`, `  ## Title  `, etc.
/// Links are left as-is -- `tui_markdown` handles `[title](url)` natively.
fn preprocess_markdown(text: &str) -> String {
    let mut result = String::with_capacity(text.len());
    for line in text.lines() {
        let trimmed = line.trim();
        if trimmed.starts_with('#') {
            // Strip all leading '#' characters
            let after_hashes = trimmed.trim_start_matches('#');
            // Extract heading content (trim spaces between # and text, and trailing)
            let content = after_hashes.trim();
            if !content.is_empty() {
                // Blank line before heading for visual separation
                if !result.is_empty() && !result.ends_with("\n\n") {
                    result.push('\n');
                }
                result.push_str("**");
                result.push_str(content);
                result.push_str("**\n");
                continue;
            }
        }
        result.push_str(line);
        result.push('\n');
    }
    if !text.ends_with('\n') {
        result.pop();
    }
    result
}

/// Render a text block with caching. Uses paragraph-level incremental markdown
/// during streaming to avoid re-parsing the entire text every frame.
///
/// Cache hierarchy:
/// 1. `BlockCache` (full block) -- hit for completed messages (no changes).
/// 2. `IncrementalMarkdown` (per-paragraph) -- only tail paragraph re-parsed during streaming.
pub(super) fn render_text_cached(
    text: &str,
    cache: &mut BlockCache,
    incr: &mut IncrementalMarkdown,
    width: u16,
    bg: Option<Color>,
    preserve_newlines: bool,
    out: &mut Vec<Line<'static>>,
) {
    // Fast path: full block cache is valid (completed message, no changes)
    if let Some(cached_lines) = cache.get() {
        crate::perf::mark_with("msg::cache_hit", "lines", cached_lines.len());
        out.extend_from_slice(cached_lines);
        return;
    }
    crate::perf::mark("msg::cache_miss");

    let _t = crate::perf::start("msg::render_text");

    // Build a render function that handles preprocessing + tui_markdown
    let render_fn = |src: &str| -> Vec<Line<'static>> {
        let mut preprocessed = preprocess_markdown(src);
        if preserve_newlines {
            preprocessed = force_markdown_line_breaks(&preprocessed);
        }
        tables::render_markdown_with_tables(&preprocessed, width, bg)
    };
    let render_key = MarkdownRenderKey { width, bg, preserve_newlines };

    // Ensure any previously invalidated paragraph caches are re-rendered
    let _ = text;
    incr.ensure_rendered(render_key, &render_fn);

    // Render: cached paragraphs + fresh tail
    let fresh = incr.lines(render_key, &render_fn);

    // Store in the full block cache with wrapped height.
    // For streaming messages this will be invalidated on the next chunk,
    // but for completed messages it persists.
    let h = {
        let _t = crate::perf::start_with("msg::wrap_height", "lines", fresh.len());
        Paragraph::new(Text::from(fresh.clone())).wrap(Wrap { trim: false }).line_count(width)
    };
    cache.store(fresh);
    cache.set_height(h, width);
    if let Some(stored) = cache.get() {
        out.extend_from_slice(stored);
    }
}

fn render_text_block_cached(
    block: &mut TextBlock,
    width: u16,
    bg: Option<Color>,
    preserve_newlines: bool,
    out: &mut Vec<Line<'static>>,
) {
    render_text_cached(
        &block.text,
        &mut block.cache,
        &mut block.markdown,
        width,
        bg,
        preserve_newlines,
        out,
    );
}

/// Convert single line breaks into hard breaks so user-entered newlines persist.
fn force_markdown_line_breaks(text: &str) -> String {
    let lines: Vec<&str> = text.lines().collect();
    let mut out = String::with_capacity(text.len());
    for (i, line) in lines.iter().enumerate() {
        if !line.is_empty() {
            out.push_str(line);
            out.push_str("  ");
        }
        if i + 1 < lines.len() || text.ends_with('\n') {
            out.push('\n');
        }
    }
    if text.ends_with('\n') {
        // preserve trailing newline
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::app::{ChatMessage, MessageBlock, TextBlock, TextBlockSpacing};
    use pretty_assertions::assert_eq;
    use ratatui::widgets::{Paragraph, Wrap};

    // preprocess_markdown

    #[test]
    fn preprocess_h1_heading() {
        let result = preprocess_markdown("# Hello");
        assert!(result.contains("**Hello**"));
        assert!(!result.contains('#'));
    }

    #[test]
    fn preprocess_h3_heading() {
        let result = preprocess_markdown("### Deeply Nested");
        assert!(result.contains("**Deeply Nested**"));
    }

    #[test]
    fn preprocess_non_heading_passthrough() {
        let input = "Just normal text\nwith multiple lines";
        let result = preprocess_markdown(input);
        assert_eq!(result, input);
    }

    #[test]
    fn preprocess_mixed_headings_and_text() {
        let input = "# Title\nSome text\n## Subtitle\nMore text";
        let result = preprocess_markdown(input);
        assert!(result.contains("**Title**"));
        assert!(result.contains("Some text"));
        assert!(result.contains("**Subtitle**"));
        assert!(result.contains("More text"));
    }

    #[test]
    fn preprocess_heading_no_space() {
        let result = preprocess_markdown("#Title");
        assert!(result.contains("**Title**"));
    }

    #[test]
    fn preprocess_heading_extra_spaces() {
        let result = preprocess_markdown("#   Spaced Out   ");
        assert!(result.contains("**Spaced Out**"));
    }

    #[test]
    fn preprocess_indented_heading() {
        let result = preprocess_markdown("  ## Indented");
        assert!(result.contains("**Indented**"));
    }

    #[test]
    fn preprocess_empty_heading() {
        let result = preprocess_markdown("# ");
        assert_eq!(result, "# ");
    }

    #[test]
    fn preprocess_empty_string() {
        assert_eq!(preprocess_markdown(""), "");
    }

    #[test]
    fn preprocess_preserves_trailing_newline() {
        let result = preprocess_markdown("hello\n");
        assert!(result.ends_with('\n'));
    }

    #[test]
    fn preprocess_no_trailing_newline() {
        let result = preprocess_markdown("hello");
        assert!(!result.ends_with('\n'));
    }

    #[test]
    fn preprocess_blank_line_before_heading() {
        let input = "text\n\n# Heading";
        let result = preprocess_markdown(input);
        assert!(!result.contains("\n\n\n"));
        assert!(result.contains("**Heading**"));
    }

    #[test]
    fn preprocess_consecutive_headings() {
        let input = "# First\n# Second";
        let result = preprocess_markdown(input);
        assert!(result.contains("**First**"));
        assert!(result.contains("**Second**"));
    }

    #[test]
    fn preprocess_hash_in_code_not_heading() {
        let result = preprocess_markdown("# actual heading");
        assert!(result.contains("**actual heading**"));
    }

    /// H6 heading (6 `#` chars).
    #[test]
    fn preprocess_h6_heading() {
        let result = preprocess_markdown("###### Deep H6");
        assert!(result.contains("**Deep H6**"));
        assert!(!result.contains('#'));
    }

    /// Heading with markdown formatting inside.
    #[test]
    fn preprocess_heading_with_bold_inside() {
        let result = preprocess_markdown("# **bold** and *italic*");
        assert!(result.contains("****bold** and *italic***"));
    }

    /// Heading at end of file with no trailing newline.
    #[test]
    fn preprocess_heading_at_eof_no_newline() {
        let result = preprocess_markdown("text\n# Final");
        assert!(result.contains("**Final**"));
        assert!(!result.ends_with('\n'));
    }

    /// Only hashes with no text: `###` - content after stripping is empty, passthrough.
    #[test]
    fn preprocess_only_hashes() {
        let result = preprocess_markdown("###");
        assert_eq!(result, "###");
    }

    /// Very long heading.
    #[test]
    fn preprocess_very_long_heading() {
        let long_text = "A".repeat(1000);
        let input = format!("# {long_text}");
        let result = preprocess_markdown(&input);
        assert!(result.starts_with("**"));
        assert!(result.contains(&long_text));
    }

    /// Unicode emoji in heading.
    #[test]
    fn preprocess_unicode_heading() {
        let result = preprocess_markdown("# \u{1F680} Launch \u{4F60}\u{597D}");
        assert!(result.contains("**\u{1F680} Launch \u{4F60}\u{597D}**"));
    }

    /// Quoted heading: `> # Heading` - starts with `>` not `#`, so passthrough.
    #[test]
    fn preprocess_blockquote_heading_passthrough() {
        let result = preprocess_markdown("> # Quoted heading");
        // Line starts with `>`, not `#`, so trimmed starts with `>` not `#`
        assert!(!result.contains("**"));
        assert!(result.contains("> # Quoted heading"));
    }

    /// All heading levels in sequence.
    #[test]
    fn preprocess_all_heading_levels() {
        let input = "# H1\n## H2\n### H3\n#### H4\n##### H5\n###### H6";
        let result = preprocess_markdown(input);
        for label in ["H1", "H2", "H3", "H4", "H5", "H6"] {
            assert!(result.contains(&format!("**{label}**")), "missing {label}");
        }
    }

    #[test]
    fn welcome_lines_do_not_render_recent_sessions_section() {
        let message = ChatMessage::welcome_with_recent(
            "claude-sonnet-4-5",
            "/cwd",
            &[crate::app::RecentSessionInfo {
                session_id: "11111111-1111-1111-1111-111111111111".to_owned(),
                summary: "Title".to_owned(),
                last_modified_ms: 0,
                file_size_bytes: 0,
                cwd: Some("/a".to_owned()),
                git_branch: None,
                custom_title: Some("Title".to_owned()),
                first_prompt: None,
            }],
        );
        let MessageBlock::Welcome(block) = &message.blocks[0] else {
            panic!("expected welcome block");
        };
        let rendered = welcome_lines(block, 120);
        let lines: Vec<String> = rendered
            .into_iter()
            .map(|line| line.spans.into_iter().map(|s| s.content).collect())
            .collect();
        assert!(!lines.iter().any(|line| line.contains("Recent sessions")));
    }

    // force_markdown_line_breaks

    #[test]
    fn force_breaks_adds_trailing_spaces() {
        let result = force_markdown_line_breaks("line1\nline2");
        assert!(result.contains("line1  \n"));
        assert!(result.contains("line2  "));
    }

    #[test]
    fn force_breaks_preserves_trailing_newline() {
        let result = force_markdown_line_breaks("hello\n");
        assert!(result.ends_with('\n'));
    }

    #[test]
    fn force_breaks_empty_lines_no_trailing_spaces() {
        let result = force_markdown_line_breaks("a\n\nb");
        let lines: Vec<&str> = result.lines().collect();
        assert_eq!(lines.len(), 3);
        assert!(lines[0].ends_with("  "));
        assert_eq!(lines[1], "");
        assert!(lines[2].ends_with("  "));
    }

    #[test]
    fn force_breaks_single_line_no_trailing_newline() {
        let result = force_markdown_line_breaks("hello");
        assert_eq!(result, "hello  ");
    }

    #[test]
    fn force_breaks_many_consecutive_empty_lines() {
        let result = force_markdown_line_breaks("a\n\n\nb");
        let lines: Vec<&str> = result.lines().collect();
        assert_eq!(lines.len(), 4);
    }

    /// Empty input.
    #[test]
    fn force_breaks_empty_input() {
        let result = force_markdown_line_breaks("");
        assert_eq!(result, "");
    }

    /// Only empty lines.
    #[test]
    fn force_breaks_only_empty_lines() {
        let result = force_markdown_line_breaks("\n\n\n");
        let lines: Vec<&str> = result.lines().collect();
        // All lines are empty, so no trailing spaces added
        for line in &lines {
            assert!(line.is_empty(), "empty line got content: {line:?}");
        }
    }

    /// Line already ending with two spaces - gets two more.
    #[test]
    fn force_breaks_already_has_trailing_spaces() {
        let result = force_markdown_line_breaks("hello  \nworld");
        // "hello  " + "  " = "hello    "
        assert!(result.starts_with("hello    "));
    }

    /// Single newline (no content).
    #[test]
    fn force_breaks_single_newline() {
        let result = force_markdown_line_breaks("\n");
        // One empty line, should stay empty with trailing newline
        assert_eq!(result, "\n");
    }

    fn make_text_message(role: MessageRole, text: &str) -> ChatMessage {
        ChatMessage {
            role,
            blocks: vec![MessageBlock::Text(TextBlock::from_complete(text))],
            usage: None,
        }
    }

    fn make_assistant_split_message(first: &str, second: &str) -> ChatMessage {
        ChatMessage {
            role: MessageRole::Assistant,
            blocks: vec![
                MessageBlock::Text(
                    TextBlock::from_complete(first)
                        .with_trailing_spacing(TextBlockSpacing::ParagraphBreak),
                ),
                MessageBlock::Text(TextBlock::from_complete(second)),
            ],
            usage: None,
        }
    }

    fn make_tool_call_info(
        id: &str,
        sdk_tool_name: &str,
        status: crate::agent::model::ToolCallStatus,
        text: &str,
    ) -> crate::app::ToolCallInfo {
        crate::app::ToolCallInfo {
            id: id.to_owned(),
            title: id.to_owned(),
            sdk_tool_name: sdk_tool_name.to_owned(),
            raw_input: None,
            raw_input_bytes: 0,
            output_metadata: None,
            status,
            content: if text.is_empty() {
                Vec::new()
            } else {
                vec![crate::agent::model::ToolCallContent::from(text.to_owned())]
            },
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
            cache: BlockCache::default(),
            pending_permission: None,
            pending_question: None,
        }
    }

    fn render_lines_to_strings(lines: &[Line<'static>]) -> Vec<String> {
        lines
            .iter()
            .map(|line| line.spans.iter().map(|span| span.content.as_ref()).collect())
            .collect()
    }

    fn make_welcome_message(model_name: &str, cwd: &str) -> ChatMessage {
        ChatMessage::welcome(model_name, cwd)
    }

    fn idle_spinner() -> SpinnerState {
        SpinnerState {
            frame: 0,
            is_active_turn_assistant: false,
            show_empty_thinking: false,
            show_thinking: false,
            show_subagent_thinking: false,
            show_compacting: false,
        }
    }

    fn ground_truth_height(msg: &mut ChatMessage, spinner: &SpinnerState, width: u16) -> usize {
        let mut lines = Vec::new();
        render_message(msg, spinner, width, &mut lines);
        Paragraph::new(Text::from(lines)).wrap(Wrap { trim: false }).line_count(width)
    }

    #[test]
    fn measure_height_matches_ground_truth_for_long_soft_wrap() {
        let text = "A".repeat(500);
        let spinner = idle_spinner();

        let mut measured_msg = make_text_message(MessageRole::User, &text);
        let mut truth_msg = make_text_message(MessageRole::User, &text);

        let (h, _) = measure_message_height_cached(&mut measured_msg, &spinner, 32, 1);
        let truth = ground_truth_height(&mut truth_msg, &spinner, 32);

        assert_eq!(h, truth);
    }

    #[test]
    fn user_role_label_wrap_height_matches_ground_truth() {
        let spinner = idle_spinner();
        let mut measured_msg = make_text_message(MessageRole::User, "ok");
        let mut truth_msg = make_text_message(MessageRole::User, "ok");

        let (h, _) = measure_message_height_cached(&mut measured_msg, &spinner, 2, 1);
        let truth = ground_truth_height(&mut truth_msg, &spinner, 2);

        assert_eq!(h, truth);
        assert!(h >= 3);
    }

    #[test]
    fn system_role_label_wrap_height_matches_ground_truth() {
        let spinner = idle_spinner();
        let mut measured_msg =
            make_text_message(MessageRole::System(Some(SystemSeverity::Warning)), "rate limit");
        let mut truth_msg =
            make_text_message(MessageRole::System(Some(SystemSeverity::Warning)), "rate limit");

        let (h, _) = measure_message_height_cached(&mut measured_msg, &spinner, 4, 1);
        let truth = ground_truth_height(&mut truth_msg, &spinner, 4);

        assert_eq!(h, truth);
        assert!(h >= 4);
    }

    #[test]
    fn welcome_role_label_wrap_height_matches_ground_truth() {
        let spinner = idle_spinner();
        let mut measured_msg = make_welcome_message("claude-sonnet-4-5", "~/project");
        let mut truth_msg = make_welcome_message("claude-sonnet-4-5", "~/project");

        let (h, _) = measure_message_height_cached(&mut measured_msg, &spinner, 4, 1);
        let truth = ground_truth_height(&mut truth_msg, &spinner, 4);

        assert_eq!(h, truth);
    }

    #[test]
    fn assistant_split_paragraph_renders_visible_blank_line() {
        let spinner = idle_spinner();
        let mut msg = make_assistant_split_message("First paragraph", "Second paragraph");
        let mut lines = Vec::new();
        render_message(&mut msg, &spinner, 80, &mut lines);

        assert_eq!(
            render_lines_to_strings(&lines),
            vec![
                "Claude".to_owned(),
                "First paragraph".to_owned(),
                String::new(),
                "Second paragraph".to_owned(),
                String::new(),
            ]
        );
    }

    #[test]
    fn assistant_split_paragraph_height_matches_rendered_gap() {
        let spinner = idle_spinner();
        let mut measured = make_assistant_split_message("First paragraph", "Second paragraph");
        let mut truth = make_assistant_split_message("First paragraph", "Second paragraph");

        let (h, _) = measure_message_height_cached(&mut measured, &spinner, 80, 1);
        let truth_h = ground_truth_height(&mut truth, &spinner, 80);
        assert_eq!(h, truth_h);
        assert_eq!(h, 5);
    }

    #[test]
    fn assistant_message_can_render_without_trailing_separator() {
        let spinner = idle_spinner();
        let mut msg = make_text_message(MessageRole::Assistant, "hello");
        let mut lines = Vec::new();

        render_message_with_tools_collapsed_and_separator(
            &mut msg, &spinner, 80, false, false, &mut lines,
        );

        assert_eq!(render_lines_to_strings(&lines), vec!["Claude".to_owned(), "hello".to_owned()]);

        let (h, _) = measure_message_height_cached_with_tools_collapsed_and_separator(
            &mut msg, &spinner, 80, 1, false, false,
        );
        assert_eq!(h, 2);
    }

    #[test]
    fn empty_last_assistant_thinking_omits_trailing_separator() {
        let spinner = SpinnerState {
            is_active_turn_assistant: true,
            show_empty_thinking: true,
            ..idle_spinner()
        };
        let mut msg = ChatMessage { role: MessageRole::Assistant, blocks: Vec::new(), usage: None };
        let mut lines = Vec::new();

        render_message_with_tools_collapsed_and_separator(
            &mut msg, &spinner, 80, false, false, &mut lines,
        );

        let rendered = render_lines_to_strings(&lines);
        assert_eq!(rendered.len(), 2);
        assert_eq!(rendered[0], "Claude");
        assert!(rendered[1].contains("Thinking..."));

        let (h, _) = measure_message_height_cached_with_tools_collapsed_and_separator(
            &mut msg, &spinner, 80, 1, false, false,
        );
        assert_eq!(h, 2);
    }

    #[test]
    fn empty_last_assistant_thinking_wrap_height_matches_ground_truth() {
        let spinner = SpinnerState {
            is_active_turn_assistant: true,
            show_empty_thinking: true,
            ..idle_spinner()
        };
        let mut measured_msg =
            ChatMessage { role: MessageRole::Assistant, blocks: Vec::new(), usage: None };
        let mut truth_msg =
            ChatMessage { role: MessageRole::Assistant, blocks: Vec::new(), usage: None };

        let (h, _) = measure_message_height_cached_with_tools_collapsed_and_separator(
            &mut measured_msg,
            &spinner,
            6,
            1,
            false,
            false,
        );
        let mut truth_lines = Vec::new();
        render_message_with_tools_collapsed_and_separator(
            &mut truth_msg,
            &spinner,
            6,
            false,
            false,
            &mut truth_lines,
        );
        let truth =
            Paragraph::new(Text::from(truth_lines)).wrap(Wrap { trim: false }).line_count(6);

        assert_eq!(h, truth);
        assert!(h > 2);
    }

    #[test]
    fn empty_last_assistant_compacting_omits_trailing_separator() {
        let spinner = SpinnerState {
            is_active_turn_assistant: true,
            show_compacting: true,
            ..idle_spinner()
        };
        let mut msg = ChatMessage { role: MessageRole::Assistant, blocks: Vec::new(), usage: None };
        let mut lines = Vec::new();

        render_message_with_tools_collapsed_and_separator(
            &mut msg, &spinner, 80, false, false, &mut lines,
        );

        let rendered = render_lines_to_strings(&lines);
        assert_eq!(rendered.len(), 2);
        assert_eq!(rendered[0], "Claude");
        assert!(rendered[1].contains("Compacting context..."));

        let (h, _) = measure_message_height_cached_with_tools_collapsed_and_separator(
            &mut msg, &spinner, 80, 1, false, false,
        );
        assert_eq!(h, 2);
    }

    #[test]
    fn empty_last_assistant_thinking_offset_render_omits_trailing_separator() {
        let spinner = SpinnerState {
            is_active_turn_assistant: true,
            show_empty_thinking: true,
            ..idle_spinner()
        };
        let mut msg = ChatMessage { role: MessageRole::Assistant, blocks: Vec::new(), usage: None };
        let mut out = Vec::new();

        let remaining = render_message_from_offset_internal(
            &mut msg,
            &spinner,
            80,
            1,
            MessageRenderOptions { tools_collapsed: false, include_trailing_separator: false },
            0,
            &mut out,
        );

        assert_eq!(remaining, 0);
        let rendered = render_lines_to_strings(&out);
        assert_eq!(rendered.len(), 2);
        assert_eq!(rendered[0], "Claude");
        assert!(rendered[1].contains("Thinking..."));
    }

    #[test]
    fn empty_last_assistant_compacting_offset_render_omits_trailing_separator() {
        let spinner = SpinnerState {
            is_active_turn_assistant: true,
            show_compacting: true,
            ..idle_spinner()
        };
        let mut msg = ChatMessage { role: MessageRole::Assistant, blocks: Vec::new(), usage: None };
        let mut out = Vec::new();

        let remaining = render_message_from_offset_internal(
            &mut msg,
            &spinner,
            80,
            1,
            MessageRenderOptions { tools_collapsed: false, include_trailing_separator: false },
            0,
            &mut out,
        );

        assert_eq!(remaining, 0);
        let rendered = render_lines_to_strings(&out);
        assert_eq!(rendered.len(), 2);
        assert_eq!(rendered[0], "Claude");
        assert!(rendered[1].contains("Compacting context..."));
    }

    #[test]
    fn render_from_offset_handles_paragraph_gap_as_structural_rows() {
        let spinner = idle_spinner();
        let mut msg = make_assistant_split_message("First paragraph", "Second paragraph");
        let mut out = Vec::new();

        let remaining = render_message_from_offset(&mut msg, &spinner, 80, 1, 2, &mut out);

        assert_eq!(remaining, 0);
        assert_eq!(
            render_lines_to_strings(&out),
            vec![String::new(), "Second paragraph".to_owned(), String::new()]
        );
    }

    #[test]
    fn measure_height_matches_ground_truth_after_resize() {
        let text =
            "This is a single very long line without explicit line breaks to stress soft wrapping."
                .repeat(20);
        let spinner = idle_spinner();

        let mut measured_msg = make_text_message(MessageRole::Assistant, &text);
        let mut truth_wide = make_text_message(MessageRole::Assistant, &text);
        let mut truth_narrow = make_text_message(MessageRole::Assistant, &text);

        let (h_wide, _) = measure_message_height_cached(&mut measured_msg, &spinner, 100, 1);
        let wide_truth = ground_truth_height(&mut truth_wide, &spinner, 100);
        assert_eq!(h_wide, wide_truth);

        // Reuse the same message to hit width-mismatch cache path.
        let (h_narrow, _) = measure_message_height_cached(&mut measured_msg, &spinner, 28, 2);
        let narrow_truth = ground_truth_height(&mut truth_narrow, &spinner, 28);
        assert_eq!(h_narrow, narrow_truth);
    }

    #[test]
    fn render_from_offset_can_skip_entire_message() {
        let spinner = idle_spinner();
        let mut msg = make_text_message(MessageRole::User, "hello\nworld");
        let mut truth_msg = make_text_message(MessageRole::User, "hello\nworld");
        let total = ground_truth_height(&mut truth_msg, &spinner, 120);

        let mut out = Vec::new();
        let rem = render_message_from_offset(&mut msg, &spinner, 120, 1, total + 3, &mut out);

        assert!(out.is_empty());
        assert_eq!(rem, 3);
    }

    #[test]
    fn welcome_height_matches_ground_truth() {
        let spinner = idle_spinner();
        let mut measured_msg = make_welcome_message("claude-sonnet-4-5", "~/project");
        let mut truth_msg = make_welcome_message("claude-sonnet-4-5", "~/project");

        let (h, _) = measure_message_height_cached(&mut measured_msg, &spinner, 52, 1);
        let truth = ground_truth_height(&mut truth_msg, &spinner, 52);
        assert_eq!(h, truth);
    }

    #[test]
    fn system_warning_severity_renders_warning_label() {
        let spinner = idle_spinner();
        let mut msg = make_text_message(
            MessageRole::System(Some(SystemSeverity::Warning)),
            "Rate limit warning",
        );
        let mut lines = Vec::new();
        render_message(&mut msg, &spinner, 120, &mut lines);
        let rendered = render_lines_to_strings(&lines);

        assert!(rendered.iter().any(|line| line.contains("Warning")));
        assert!(rendered.iter().any(|line| line.contains("Rate limit warning")));
    }

    #[test]
    fn assistant_message_shows_subagent_indicator_when_enabled() {
        let spinner = SpinnerState { show_subagent_thinking: true, ..idle_spinner() };
        let mut msg = ChatMessage {
            role: MessageRole::Assistant,
            blocks: vec![MessageBlock::ToolCall(Box::new(make_tool_call_info(
                "task-only",
                "Task",
                crate::agent::model::ToolCallStatus::InProgress,
                "Research project",
            )))],
            usage: None,
        };

        let mut lines = Vec::new();
        render_message(&mut msg, &spinner, 120, &mut lines);
        let rendered = render_lines_to_strings(&lines);

        assert!(rendered.iter().any(|line| line.contains("Thinking...")));
    }

    #[test]
    fn assistant_heading_at_start_does_not_render_blank_line_after_label() {
        let spinner = idle_spinner();
        let mut msg = make_text_message(MessageRole::Assistant, "\n# Heading\nBody");

        let mut lines = Vec::new();
        render_message(&mut msg, &spinner, 80, &mut lines);
        let rendered = render_lines_to_strings(&lines);

        assert_eq!(rendered[0], "Claude");
        assert!(rendered[1].contains("Heading"));
        assert!(!rendered[1].is_empty());
    }

    #[test]
    fn assistant_heading_at_start_height_matches_rendered_output() {
        let spinner = idle_spinner();
        let mut measured = make_text_message(MessageRole::Assistant, "\n# Heading\nBody");
        let mut truth = make_text_message(MessageRole::Assistant, "\n# Heading\nBody");

        let (h, _) = measure_message_height_cached(&mut measured, &spinner, 80, 1);
        let truth_h = ground_truth_height(&mut truth, &spinner, 80);

        assert_eq!(h, truth_h);
    }

    #[test]
    fn assistant_heading_at_start_offset_render_omits_leading_blank_row() {
        let spinner = idle_spinner();
        let mut msg = make_text_message(MessageRole::Assistant, "\n# Heading\nBody");
        let mut out = Vec::new();

        let remaining = render_message_from_offset(&mut msg, &spinner, 80, 1, 0, &mut out);
        let rendered = render_lines_to_strings(&out);

        assert_eq!(remaining, 0);
        assert_eq!(rendered[0], "Claude");
        assert!(rendered[1].contains("Heading"));
        assert!(!rendered[1].is_empty());
    }

    #[test]
    fn assistant_message_hides_subagent_indicator_when_disabled() {
        let spinner = idle_spinner();
        let mut msg = ChatMessage {
            role: MessageRole::Assistant,
            blocks: vec![
                MessageBlock::ToolCall(Box::new(make_tool_call_info(
                    "task-main",
                    "Task",
                    crate::agent::model::ToolCallStatus::InProgress,
                    "Research project",
                ))),
                MessageBlock::ToolCall(Box::new(make_tool_call_info(
                    "bash-child",
                    "Bash",
                    crate::agent::model::ToolCallStatus::InProgress,
                    "",
                ))),
            ],
            usage: None,
        };

        let mut lines = Vec::new();
        render_message(&mut msg, &spinner, 120, &mut lines);
        let rendered = render_lines_to_strings(&lines);

        assert!(!rendered.iter().any(|line| line.contains("Thinking...")));
    }

    #[test]
    fn assistant_message_places_subagent_indicator_after_visible_tool_blocks() {
        let spinner = SpinnerState { show_subagent_thinking: true, ..idle_spinner() };
        let mut msg = ChatMessage {
            role: MessageRole::Assistant,
            blocks: vec![
                MessageBlock::ToolCall(Box::new(make_tool_call_info(
                    "task-main",
                    "Task",
                    crate::agent::model::ToolCallStatus::InProgress,
                    "Research project",
                ))),
                MessageBlock::ToolCall(Box::new(make_tool_call_info(
                    "bash-done",
                    "Bash",
                    crate::agent::model::ToolCallStatus::Completed,
                    "",
                ))),
            ],
            usage: None,
        };

        let mut lines = Vec::new();
        render_message(&mut msg, &spinner, 120, &mut lines);
        let rendered = render_lines_to_strings(&lines);

        let bash_idx = rendered.iter().position(|line| line.contains("Bash")).expect("bash line");
        let thinking_idx = rendered
            .iter()
            .position(|line| line.contains("Thinking..."))
            .expect("subagent thinking line");

        assert!(thinking_idx > bash_idx);
    }

    #[test]
    fn assistant_message_does_not_show_empty_turn_thinking_after_content_exists() {
        let spinner = SpinnerState {
            is_active_turn_assistant: true,
            show_empty_thinking: true,
            ..idle_spinner()
        };
        let mut msg = make_text_message(MessageRole::Assistant, "done");

        let mut lines = Vec::new();
        render_message(&mut msg, &spinner, 120, &mut lines);
        let rendered = render_lines_to_strings(&lines);

        assert!(!rendered.iter().any(|line| line.contains("Thinking...")));
    }

    #[test]
    fn assistant_message_suppresses_thinking_line_while_compacting() {
        let spinner = SpinnerState {
            is_active_turn_assistant: true,
            show_thinking: true,
            show_compacting: true,
            ..idle_spinner()
        };
        let mut msg = make_text_message(MessageRole::Assistant, "done");

        let mut lines = Vec::new();
        render_message(&mut msg, &spinner, 120, &mut lines);
        let rendered = render_lines_to_strings(&lines);

        assert!(rendered.iter().any(|line| line.contains("Compacting context...")));
        assert!(!rendered.iter().any(|line| line.contains("Thinking...")));
    }

    #[test]
    fn assistant_offset_render_suppresses_thinking_line_while_compacting() {
        let spinner = SpinnerState {
            is_active_turn_assistant: true,
            show_thinking: true,
            show_compacting: true,
            ..idle_spinner()
        };
        let mut msg = make_text_message(MessageRole::Assistant, "done");

        let mut lines = Vec::new();
        let remaining = render_message_from_offset(&mut msg, &spinner, 120, 1, 0, &mut lines);
        let rendered = render_lines_to_strings(&lines);

        assert_eq!(remaining, 0);
        assert!(rendered.iter().any(|line| line.contains("Compacting context...")));
        assert!(!rendered.iter().any(|line| line.contains("Thinking...")));
    }
}
