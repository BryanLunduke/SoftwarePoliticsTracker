// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::markdown;
use pulldown_cmark::{Alignment, Event, Options, Parser, Tag, TagEnd};
use ratatui::style::{Color, Modifier, Style};
use ratatui::text::{Line, Span};
use unicode_width::{UnicodeWidthChar, UnicodeWidthStr};

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
enum ColumnAlignment {
    Left,
    Center,
    Right,
}

#[derive(Clone, Debug, Default)]
struct TableRowAst {
    cells: Vec<TableCellAst>,
}

#[derive(Clone, Debug)]
struct TableCellAst {
    chunks: Vec<StyledChunk>,
    preferred_width: usize,
    soft_min_width: usize,
}

#[derive(Clone, Debug, Default)]
struct TableAst {
    header: TableRowAst,
    rows: Vec<TableRowAst>,
    alignments: Vec<ColumnAlignment>,
}

#[derive(Clone, Debug)]
struct StyledChunk {
    text: String,
    style: Style,
}

enum MarkdownBlock {
    Text(String),
    Table(TableAst),
}

#[derive(Clone, Copy)]
struct ColumnMetrics {
    preferred: usize,
    soft_min: usize,
}

impl TableAst {
    fn column_count(&self) -> usize {
        let body_cols = self.rows.iter().map(|row| row.cells.len()).max().unwrap_or(0);
        self.header.cells.len().max(self.alignments.len()).max(body_cols)
    }
}

impl TableCellAst {
    fn empty() -> Self {
        Self { chunks: Vec::new(), preferred_width: 1, soft_min_width: 1 }
    }
}

impl Default for TableCellAst {
    fn default() -> Self {
        Self::empty()
    }
}

pub fn render_markdown_with_tables(
    text: &str,
    width: u16,
    bg: Option<Color>,
) -> Vec<Line<'static>> {
    let blocks = split_markdown_tables(text);
    let mut out = Vec::new();
    for block in blocks {
        match block {
            MarkdownBlock::Text(chunk) => {
                if chunk.trim().is_empty() {
                    continue;
                }
                out.extend(markdown::render_markdown_safe(&chunk, bg));
            }
            MarkdownBlock::Table(table) => {
                if !out.is_empty() {
                    out.push(Line::default());
                }
                out.extend(render_table_lines(&table, width, bg));
                out.push(Line::default());
            }
        }
    }
    out
}

fn parser_options() -> Options {
    let mut options = Options::ENABLE_STRIKETHROUGH;
    options.insert(Options::ENABLE_TABLES);
    options
}

fn split_markdown_tables(text: &str) -> Vec<MarkdownBlock> {
    let mut blocks = Vec::new();
    let mut parser = Parser::new_ext(text, parser_options()).into_offset_iter().peekable();
    let mut text_start = 0usize;

    loop {
        let Some((event, range)) = parser.next() else {
            break;
        };
        if let Event::Start(Tag::Table(alignments)) = event {
            if text_start < range.start {
                blocks.push(MarkdownBlock::Text(text[text_start..range.start].to_owned()));
            }

            let mut table_end = range.end;
            let table = parse_table_ast(alignments, &mut parser, &mut table_end);
            blocks.push(MarkdownBlock::Table(table));
            text_start = table_end;
        }
    }

    if text_start < text.len() {
        blocks.push(MarkdownBlock::Text(text[text_start..].to_owned()));
    }

    blocks
}

fn parse_table_ast<'input, I>(
    alignments: Vec<Alignment>,
    parser: &mut std::iter::Peekable<I>,
    table_end: &mut usize,
) -> TableAst
where
    I: Iterator<Item = (Event<'input>, std::ops::Range<usize>)>,
{
    let mut header = None;
    let mut rows = Vec::new();
    let mut current_row: Option<TableRowAst> = None;
    let mut current_cell: Option<CellBuilder> = None;
    let mut in_header = false;

    for (event, range) in parser.by_ref() {
        *table_end = (*table_end).max(range.end);
        match event {
            Event::Start(Tag::TableHead) => in_header = true,
            Event::End(TagEnd::TableHead) => {
                if let Some(row) = current_row.take()
                    && !row.cells.is_empty()
                    && header.is_none()
                {
                    header = Some(row);
                }
                in_header = false;
            }
            Event::Start(Tag::TableRow) => current_row = Some(TableRowAst::default()),
            Event::End(TagEnd::TableRow) => {
                let row = current_row.take().unwrap_or_default();
                if in_header && header.is_none() {
                    header = Some(row);
                } else {
                    rows.push(row);
                }
            }
            Event::Start(Tag::TableCell) => {
                current_row.get_or_insert_with(TableRowAst::default);
                current_cell = Some(CellBuilder::new());
            }
            Event::End(TagEnd::TableCell) => {
                if let Some(cell) = current_cell.take() {
                    current_row.get_or_insert_with(TableRowAst::default).cells.push(cell.finish());
                }
            }
            Event::End(TagEnd::Table) => break,
            Event::Start(tag) => {
                if let Some(cell) = current_cell.as_mut() {
                    cell.start_tag(&tag);
                }
            }
            Event::End(tag) => {
                if let Some(cell) = current_cell.as_mut() {
                    cell.end_tag(tag);
                }
            }
            Event::Text(text) => {
                if let Some(cell) = current_cell.as_mut() {
                    cell.push_text(text.as_ref());
                }
            }
            Event::Code(code) => {
                if let Some(cell) = current_cell.as_mut() {
                    cell.push_code(code.as_ref());
                }
            }
            Event::SoftBreak => {
                if let Some(cell) = current_cell.as_mut() {
                    cell.push_text(" ");
                }
            }
            Event::HardBreak => {
                if let Some(cell) = current_cell.as_mut() {
                    cell.push_text("\n");
                }
            }
            Event::Html(raw) | Event::InlineHtml(raw) => {
                if let Some(cell) = current_cell.as_mut() {
                    cell.push_text(raw.as_ref());
                }
            }
            Event::InlineMath(math) | Event::DisplayMath(math) => {
                if let Some(cell) = current_cell.as_mut() {
                    cell.push_text(math.as_ref());
                }
            }
            Event::FootnoteReference(reference) => {
                if let Some(cell) = current_cell.as_mut() {
                    cell.push_text(reference.as_ref());
                }
            }
            Event::TaskListMarker(done) => {
                if let Some(cell) = current_cell.as_mut() {
                    cell.push_text(if done { "[x] " } else { "[ ] " });
                }
            }
            Event::Rule => {}
        }
    }

    TableAst {
        header: header.unwrap_or_default(),
        rows,
        alignments: alignments.into_iter().map(ColumnAlignment::from).collect(),
    }
}

struct CellBuilder {
    chunks: Vec<StyledChunk>,
    current_text: String,
    style_stack: Vec<Style>,
    current_style: Style,
}

impl CellBuilder {
    fn new() -> Self {
        Self {
            chunks: Vec::new(),
            current_text: String::new(),
            style_stack: vec![Style::default()],
            current_style: Style::default(),
        }
    }

    fn push_text(&mut self, text: &str) {
        self.current_text.push_str(text);
    }

    fn push_code(&mut self, text: &str) {
        self.flush_current();
        let style = self.current_style.add_modifier(Modifier::REVERSED);
        self.chunks.push(StyledChunk { text: text.to_owned(), style });
    }

    fn start_tag(&mut self, tag: &Tag<'_>) {
        let next = match tag {
            Tag::Strong => self.current_style.add_modifier(Modifier::BOLD),
            Tag::Emphasis => self.current_style.add_modifier(Modifier::ITALIC),
            Tag::Strikethrough => self.current_style.add_modifier(Modifier::CROSSED_OUT),
            Tag::Link { .. } => self.current_style.add_modifier(Modifier::UNDERLINED),
            _ => return,
        };

        self.flush_current();
        self.style_stack.push(next);
        self.current_style = next;
    }

    fn end_tag(&mut self, tag: TagEnd) {
        let styled =
            matches!(tag, TagEnd::Strong | TagEnd::Emphasis | TagEnd::Strikethrough | TagEnd::Link);
        if !styled {
            return;
        }

        self.flush_current();
        let _ = self.style_stack.pop();
        self.current_style = self.style_stack.last().copied().unwrap_or_default();
    }

    fn flush_current(&mut self) {
        if self.current_text.is_empty() {
            return;
        }
        self.chunks.push(StyledChunk {
            text: std::mem::take(&mut self.current_text),
            style: self.current_style,
        });
    }

    fn finish(mut self) -> TableCellAst {
        self.flush_current();
        let plain: String = self.chunks.iter().map(|chunk| chunk.text.as_str()).collect();
        let (preferred_width, soft_min_width) = measure_cell_widths(&plain);
        TableCellAst {
            chunks: self.chunks,
            preferred_width: preferred_width.max(1),
            soft_min_width: soft_min_width.max(1),
        }
    }
}

fn measure_cell_widths(text: &str) -> (usize, usize) {
    let preferred = text.lines().map(UnicodeWidthStr::width).max().unwrap_or(0);
    let soft_min = text
        .lines()
        .flat_map(|line| line.split_whitespace())
        .map(UnicodeWidthStr::width)
        .max()
        .unwrap_or(preferred);
    (preferred, soft_min)
}

fn render_table_lines(table: &TableAst, width: u16, bg: Option<Color>) -> Vec<Line<'static>> {
    let cols = table.column_count();
    if cols == 0 || width == 0 {
        return Vec::new();
    }

    let spacing = 3usize;
    let available = (width as usize).saturating_sub(spacing.saturating_mul(cols.saturating_sub(1)));
    if available == 0 {
        return Vec::new();
    }

    let metrics = collect_column_metrics(table, cols);
    let widths = solve_column_widths(&metrics, available);

    let header_style = bg.map_or_else(
        || Style::default().add_modifier(Modifier::BOLD),
        |bg_color| Style::default().bg(bg_color).add_modifier(Modifier::BOLD),
    );
    let row_style = bg.map_or_else(Style::default, |bg_color| Style::default().bg(bg_color));

    let mut lines =
        render_row_lines(&table.header, &widths, &table.alignments, header_style, spacing);
    if !lines.is_empty() {
        lines.push(render_separator_line(&widths, spacing, row_style));
    }
    for row in &table.rows {
        lines.extend(render_row_lines(row, &widths, &table.alignments, row_style, spacing));
    }
    lines
}

fn collect_column_metrics(table: &TableAst, cols: usize) -> Vec<ColumnMetrics> {
    let mut metrics = vec![ColumnMetrics { preferred: 1, soft_min: 1 }; cols];
    for row in std::iter::once(&table.header).chain(table.rows.iter()) {
        for (idx, cell) in row.cells.iter().enumerate() {
            metrics[idx].preferred = metrics[idx].preferred.max(cell.preferred_width);
            metrics[idx].soft_min = metrics[idx].soft_min.max(cell.soft_min_width);
        }
    }
    metrics
}

fn solve_column_widths(metrics: &[ColumnMetrics], available: usize) -> Vec<usize> {
    if metrics.is_empty() {
        return Vec::new();
    }

    let mut widths: Vec<usize> = metrics.iter().map(|metric| metric.preferred.max(1)).collect();
    let soft_floor: Vec<usize> = metrics
        .iter()
        .zip(widths.iter())
        .map(|(metric, width)| metric.soft_min.clamp(1, *width))
        .collect();

    reduce_widths(&mut widths, available, &soft_floor);
    if widths.iter().sum::<usize>() > available {
        let hard_floor = vec![1; widths.len()];
        reduce_widths(&mut widths, available, &hard_floor);
    }
    widths
}

fn reduce_widths(widths: &mut [usize], available: usize, floor: &[usize]) {
    while widths.iter().sum::<usize>() > available {
        let candidate = widths
            .iter()
            .enumerate()
            .filter(|(idx, width)| **width > floor[*idx])
            .max_by_key(|(idx, width)| (*width - floor[*idx], *width, std::cmp::Reverse(*idx)));
        let Some((idx, _)) = candidate else {
            break;
        };
        widths[idx] -= 1;
    }
}

fn render_row_lines(
    row: &TableRowAst,
    widths: &[usize],
    alignments: &[ColumnAlignment],
    base_style: Style,
    spacing: usize,
) -> Vec<Line<'static>> {
    let mut cell_lines = Vec::with_capacity(widths.len());
    let mut row_height = 1usize;

    for (idx, width) in widths.iter().copied().enumerate() {
        let alignment = alignments.get(idx).copied().unwrap_or(ColumnAlignment::Left);
        let cell = row.cells.get(idx).cloned().unwrap_or_default();
        let rendered = render_cell_lines(&cell, width, alignment, base_style);
        row_height = row_height.max(rendered.len());
        cell_lines.push(rendered);
    }

    for (idx, width) in widths.iter().copied().enumerate() {
        while cell_lines[idx].len() < row_height {
            cell_lines[idx].push(blank_cell_line(width, base_style));
        }
    }

    let mut lines = Vec::with_capacity(row_height);
    for line_idx in 0..row_height {
        let mut spans = Vec::new();
        for (idx, cell) in cell_lines.iter().enumerate() {
            if idx > 0 {
                spans.push(Span::styled(" ".repeat(spacing), base_style));
            }
            spans.extend(cell[line_idx].spans.clone());
        }
        lines.push(Line::from(spans));
    }
    lines
}

fn render_separator_line(widths: &[usize], spacing: usize, base_style: Style) -> Line<'static> {
    let separator_style = base_style.add_modifier(Modifier::DIM);
    let mut spans = Vec::new();
    for (idx, width) in widths.iter().copied().enumerate() {
        if idx > 0 {
            spans.push(Span::styled(" ".repeat(spacing), base_style));
        }
        spans.push(Span::styled("─".repeat(width), separator_style));
    }
    Line::from(spans)
}

fn render_cell_lines(
    cell: &TableCellAst,
    width: usize,
    alignment: ColumnAlignment,
    base_style: Style,
) -> Vec<Line<'static>> {
    if width == 0 {
        return Vec::new();
    }
    if cell.chunks.is_empty() {
        return vec![blank_cell_line(width, base_style)];
    }

    let tokens = tokenize_chunks(&cell.chunks, base_style);
    let mut lines = Vec::new();
    let mut spans = Vec::new();
    let mut line_width = 0usize;
    let mut pending_spaces = Vec::<StyledToken>::new();

    for token in tokens {
        match token {
            WrapToken::Newline => {
                finish_wrapped_line(
                    &mut lines,
                    &mut spans,
                    &mut line_width,
                    width,
                    alignment,
                    base_style,
                );
                pending_spaces.clear();
            }
            WrapToken::Space(space) => {
                if line_width > 0 {
                    pending_spaces.push(space);
                }
            }
            WrapToken::Text(text) => {
                let pending_width: usize = pending_spaces.iter().map(|space| space.width).sum();
                if line_width > 0 && line_width + pending_width + text.width > width {
                    finish_wrapped_line(
                        &mut lines,
                        &mut spans,
                        &mut line_width,
                        width,
                        alignment,
                        base_style,
                    );
                    pending_spaces.clear();
                }

                if line_width > 0 {
                    for space in pending_spaces.drain(..) {
                        push_styled_text(&mut spans, &space.text, space.style);
                        line_width += space.width;
                    }
                }

                if text.width <= width.saturating_sub(line_width) {
                    push_styled_text(&mut spans, &text.text, text.style);
                    line_width += text.width;
                    continue;
                }

                wrap_long_token(
                    &text,
                    width,
                    alignment,
                    base_style,
                    &mut lines,
                    &mut spans,
                    &mut line_width,
                );
            }
        }
    }

    finish_wrapped_line(&mut lines, &mut spans, &mut line_width, width, alignment, base_style);
    if lines.is_empty() {
        lines.push(blank_cell_line(width, base_style));
    }
    lines
}

#[derive(Clone)]
struct StyledToken {
    text: String,
    style: Style,
    width: usize,
}

enum WrapToken {
    Text(StyledToken),
    Space(StyledToken),
    Newline,
}

fn tokenize_chunks(chunks: &[StyledChunk], base_style: Style) -> Vec<WrapToken> {
    let mut tokens = Vec::new();
    for chunk in chunks {
        let mut current = String::new();
        let mut is_space = None;
        let style = base_style.patch(chunk.style);

        let flush_current = |tokens: &mut Vec<WrapToken>,
                             current: &mut String,
                             is_space: &mut Option<bool>,
                             style: Style| {
            if current.is_empty() {
                return;
            }
            let text = std::mem::take(current);
            let width = UnicodeWidthStr::width(text.as_str());
            let token = StyledToken { text, style, width };
            if is_space.unwrap_or(false) {
                tokens.push(WrapToken::Space(token));
            } else {
                tokens.push(WrapToken::Text(token));
            }
        };

        for ch in chunk.text.chars() {
            if ch == '\n' {
                flush_current(&mut tokens, &mut current, &mut is_space, style);
                is_space = None;
                tokens.push(WrapToken::Newline);
                continue;
            }

            let ch_is_space = ch.is_whitespace();
            if is_space.is_some_and(|value| value != ch_is_space) {
                flush_current(&mut tokens, &mut current, &mut is_space, style);
            }

            is_space = Some(ch_is_space);
            current.push(ch);
        }

        flush_current(&mut tokens, &mut current, &mut is_space, style);
    }
    tokens
}

fn wrap_long_token(
    token: &StyledToken,
    width: usize,
    alignment: ColumnAlignment,
    base_style: Style,
    lines: &mut Vec<Line<'static>>,
    spans: &mut Vec<Span<'static>>,
    line_width: &mut usize,
) {
    let mut segment = String::new();
    let mut segment_width = 0usize;

    for ch in token.text.chars() {
        let ch_width = UnicodeWidthChar::width(ch).unwrap_or(0);
        if *line_width > 0 && *line_width + segment_width + ch_width > width {
            if !segment.is_empty() {
                push_styled_text(spans, &segment, token.style);
                *line_width += segment_width;
                segment.clear();
                segment_width = 0;
            }
            finish_wrapped_line(lines, spans, line_width, width, alignment, base_style);
        }

        if segment_width + ch_width > width && !segment.is_empty() {
            push_styled_text(spans, &segment, token.style);
            *line_width += segment_width;
            segment.clear();
            segment_width = 0;
            finish_wrapped_line(lines, spans, line_width, width, alignment, base_style);
        }

        segment.push(ch);
        segment_width += ch_width;
    }

    if !segment.is_empty() {
        push_styled_text(spans, &segment, token.style);
        *line_width += segment_width;
    }
}

fn finish_wrapped_line(
    lines: &mut Vec<Line<'static>>,
    spans: &mut Vec<Span<'static>>,
    line_width: &mut usize,
    width: usize,
    alignment: ColumnAlignment,
    base_style: Style,
) {
    let line =
        align_spans_to_width(std::mem::take(spans), *line_width, width, alignment, base_style);
    lines.push(Line::from(line));
    *line_width = 0;
}

fn align_spans_to_width(
    mut spans: Vec<Span<'static>>,
    content_width: usize,
    width: usize,
    alignment: ColumnAlignment,
    base_style: Style,
) -> Vec<Span<'static>> {
    if content_width >= width {
        return spans;
    }

    let padding = width - content_width;
    let (left_pad, right_pad) = match alignment {
        ColumnAlignment::Left => (0, padding),
        ColumnAlignment::Center => (padding / 2, padding - (padding / 2)),
        ColumnAlignment::Right => (padding, 0),
    };

    if left_pad > 0 {
        spans.insert(0, Span::styled(" ".repeat(left_pad), base_style));
    }
    if right_pad > 0 {
        spans.push(Span::styled(" ".repeat(right_pad), base_style));
    }
    spans
}

fn blank_cell_line(width: usize, style: Style) -> Line<'static> {
    Line::from(Span::styled(" ".repeat(width), style))
}

fn push_styled_text(spans: &mut Vec<Span<'static>>, text: &str, style: Style) {
    if text.is_empty() {
        return;
    }
    if let Some(last) = spans.last_mut()
        && last.style == style
    {
        last.content.to_mut().push_str(text);
        return;
    }
    spans.push(Span::styled(text.to_owned(), style));
}

impl From<Alignment> for ColumnAlignment {
    fn from(value: Alignment) -> Self {
        match value {
            Alignment::Center => Self::Center,
            Alignment::Right => Self::Right,
            Alignment::Left | Alignment::None => Self::Left,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use pretty_assertions::assert_eq;

    fn render_strings(text: &str, width: u16) -> Vec<String> {
        render_markdown_with_tables(text, width, None)
            .into_iter()
            .map(|line| line.spans.into_iter().map(|span| span.content.into_owned()).collect())
            .collect()
    }

    #[test]
    fn structural_parser_extracts_markdown_tables() {
        let blocks =
            split_markdown_tables("before\n\n| a | b |\n| --- | --- |\n| 1 | 2 |\n\nafter");
        assert_eq!(blocks.len(), 3);
        assert!(matches!(blocks[0], MarkdownBlock::Text(_)));
        assert!(matches!(blocks[1], MarkdownBlock::Table(_)));
        assert!(matches!(blocks[2], MarkdownBlock::Text(_)));
    }

    #[test]
    fn structural_parser_does_not_treat_code_fences_as_tables() {
        let input = "```text\n| not | a table |\n| --- | --- |\n```\n";
        let blocks = split_markdown_tables(input);
        assert_eq!(blocks.len(), 1);
        assert!(matches!(blocks[0], MarkdownBlock::Text(_)));
    }

    #[test]
    fn table_render_keeps_text_before_and_after() {
        let input = "Intro\n\n| A | B |\n| --- | --- |\n| 1 | 2 |\n\nOutro";
        let rendered = render_strings(input, 40);
        assert!(rendered.iter().any(|line| line.contains("Intro")));
        assert!(rendered.iter().any(|line| line.contains("Outro")));
        assert!(rendered.iter().any(|line| line.contains('A')));
        assert!(rendered.iter().any(|line| line.contains("─")));
    }

    #[test]
    fn alignment_markers_affect_output_padding() {
        let input = "| left | center | right |\n| :--- | :----: | ----: |\n| a | bb | c |\n";
        let rendered = render_strings(input, 32);
        assert_eq!(rendered[0], "left   center   right");
        assert_eq!(rendered[1], "────   ──────   ─────");
        assert_eq!(rendered[2], "a        bb         c");
    }

    #[test]
    fn width_solver_wraps_cells_across_multiple_widths() {
        let input = "| feature | details |\n| --- | --- |\n| wrapping | this sentence should wrap cleanly |\n";
        let wide = render_strings(input, 40);
        let narrow = render_strings(input, 22);

        assert!(wide.len() < narrow.len());
        assert!(narrow.iter().any(|line| line.contains("sentence")));
        assert!(narrow.iter().any(|line| line.contains("cleanly")));
    }

    #[test]
    fn unicode_width_is_accounted_for() {
        let input = "| col | value |\n| --- | --- |\n| 你好 | 宽字符 |\n";
        let rendered = render_strings(input, 20);
        assert!(rendered.iter().any(|line| line.contains("你好")));
        assert!(rendered.iter().any(|line| line.contains("宽字符")));
    }

    #[test]
    fn malformed_markdown_falls_back_to_text_block() {
        let blocks = split_markdown_tables("| a | b |\n| this is not a separator |\n| 1 | 2 |\n");
        assert_eq!(blocks.len(), 1);
        assert!(matches!(blocks[0], MarkdownBlock::Text(_)));
    }

    #[test]
    fn inline_markdown_spans_render_inside_cells() {
        let input = "| col |\n| --- |\n| **bold** and `code` |\n";
        let rendered = render_markdown_with_tables(input, 24, None);
        let body = &rendered[2];
        assert!(body.spans.iter().any(|span| span.style.add_modifier.contains(Modifier::BOLD)));
        assert!(body.spans.iter().any(|span| span.style.add_modifier.contains(Modifier::REVERSED)));
    }
}
