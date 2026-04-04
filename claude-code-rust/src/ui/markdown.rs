// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use ratatui::style::{Color, Style};
use ratatui::text::{Line, Span};
use std::panic::{self, AssertUnwindSafe};

pub(super) fn render_markdown_safe(text: &str, bg: Option<Color>) -> Vec<Line<'static>> {
    render_markdown_safe_with(text, bg, render_with_tui_markdown)
}

fn render_markdown_safe_with<F>(text: &str, bg: Option<Color>, renderer: F) -> Vec<Line<'static>>
where
    F: FnOnce(&str, Option<Color>) -> Vec<Line<'static>>,
{
    if let Ok(lines) = panic::catch_unwind(AssertUnwindSafe(|| renderer(text, bg))) {
        lines
    } else {
        tracing::warn!("tui-markdown panic; falling back to plain-text markdown rendering");
        plain_text_fallback(text, bg)
    }
}

fn render_with_tui_markdown(text: &str, bg: Option<Color>) -> Vec<Line<'static>> {
    let rendered = tui_markdown::from_str(text);
    rendered
        .lines
        .into_iter()
        .map(|line| {
            let owned_spans: Vec<Span<'static>> = line
                .spans
                .into_iter()
                .map(|span| {
                    let style =
                        if let Some(bg_color) = bg { span.style.bg(bg_color) } else { span.style };
                    Span::styled(span.content.into_owned(), style)
                })
                .collect();
            let line_style =
                if let Some(bg_color) = bg { line.style.bg(bg_color) } else { line.style };
            Line::from(owned_spans).style(line_style)
        })
        .collect()
}

fn plain_text_fallback(text: &str, bg: Option<Color>) -> Vec<Line<'static>> {
    let style =
        if let Some(bg_color) = bg { Style::default().bg(bg_color) } else { Style::default() };

    text.split('\n').map(|line| Line::from(Span::styled(line.to_owned(), style))).collect()
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::panic::catch_unwind;

    #[test]
    fn render_markdown_safe_handles_checklist_content() {
        let lines = render_markdown_safe("- [ ] one\n- [x] two", None);
        assert!(!lines.is_empty());
    }

    #[test]
    fn render_markdown_safe_handles_requested_task_line() {
        let input = "- [ ] Move todos below input top line";
        let lines = render_markdown_safe(input, None);
        assert!(!lines.is_empty());
    }

    #[test]
    fn render_markdown_safe_does_not_panic_on_weird_inputs() {
        let weird_inputs = [
            "- [ ] Move todos below input top line",
            "- [ ]\n- [x]\n- [ ]",
            "- [x] done\n  - [ ] child",
            "1. [ ] numbered checklist marker",
            "[]()[]()[]()",
            "```md\n- [ ] fenced checklist\n```",
            "> - [ ] blockquote checklist\n>\n> text",
            "# Heading\n- [ ] item\n\n| a | b |\n|---|---|\n| x | y |",
            "- [ ] [link](https://example.com) [",
            "- [ ] \u{200d}\u{200d}\u{200d}",
        ];

        for input in weird_inputs {
            let result = catch_unwind(|| render_markdown_safe(input, None));
            assert!(result.is_ok(), "input triggered panic: {input}");
            assert!(!result.unwrap().is_empty(), "input rendered zero lines: {input}");
        }
    }

    #[test]
    fn render_markdown_safe_falls_back_when_renderer_panics() {
        let lines = render_markdown_safe_with("line1\nline2", None, |_text, _bg| {
            panic!("forced renderer panic for fallback path")
        });
        assert_eq!(lines.len(), 2);
        assert_eq!(lines[0].spans[0].content.as_ref(), "line1");
        assert_eq!(lines[1].spans[0].content.as_ref(), "line2");
    }
}
