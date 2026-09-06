// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::diff;
use ratatui::style::{Color, Modifier, Style};
use ratatui::text::{Line, Span};
use std::sync::LazyLock;
use syntect::easy::HighlightLines;
use syntect::highlighting::{Color as SyntectColor, FontStyle, Theme, ThemeSet};
use syntect::parsing::{SyntaxReference, SyntaxSet};
use syntect::util::LinesWithEndings;

static SYNTAX_SET: LazyLock<SyntaxSet> = LazyLock::new(SyntaxSet::load_defaults_newlines);
static THEME_SET: LazyLock<ThemeSet> = LazyLock::new(ThemeSet::load_defaults);
static FALLBACK_THEME: LazyLock<Theme> = LazyLock::new(Theme::default);

pub(crate) fn strip_ansi(text: &str) -> String {
    enum State {
        Normal,
        Escape,
        Csi,
        Osc,
        OscEscape,
    }

    let mut out = String::with_capacity(text.len());
    let mut state = State::Normal;

    for ch in text.chars() {
        state = match state {
            State::Normal => {
                if ch == '\u{1b}' {
                    State::Escape
                } else {
                    out.push(ch);
                    State::Normal
                }
            }
            State::Escape => match ch {
                '[' => State::Csi,
                ']' => State::Osc,
                _ => State::Normal,
            },
            State::Csi => {
                if ('\u{40}'..='\u{7e}').contains(&ch) {
                    State::Normal
                } else {
                    State::Csi
                }
            }
            State::Osc => match ch {
                '\u{07}' => State::Normal,
                '\u{1b}' => State::OscEscape,
                _ => State::Osc,
            },
            State::OscEscape => {
                if ch == '\\' {
                    State::Normal
                } else {
                    State::Osc
                }
            }
        };
    }

    out
}

pub(crate) fn render_terminal_output(text: &str) -> Vec<Line<'static>> {
    let stripped = strip_ansi(text);
    if diff::looks_like_unified_diff(&stripped) {
        return diff::render_raw_unified_diff(&stripped);
    }
    plain_text_lines(&stripped)
}

pub(crate) fn highlight_code(text: &str, language: Option<&str>) -> Vec<Line<'static>> {
    let syntax =
        language.and_then(find_syntax).unwrap_or_else(|| SYNTAX_SET.find_syntax_plain_text());
    highlight_with_syntax(text, syntax)
}

pub(crate) fn highlight_shell_command(text: &str) -> Vec<Span<'static>> {
    let syntax = find_syntax("bash")
        .or_else(|| find_syntax("sh"))
        .unwrap_or_else(|| SYNTAX_SET.find_syntax_plain_text());
    highlight_single_line(text, syntax)
}

fn highlight_with_syntax(text: &str, syntax: &SyntaxReference) -> Vec<Line<'static>> {
    if text.is_empty() {
        return vec![Line::default()];
    }

    let mut highlighter = HighlightLines::new(syntax, highlight_theme());
    let mut lines = Vec::new();

    for raw_line in LinesWithEndings::from(text) {
        lines.push(highlight_line(raw_line, &mut highlighter));
    }

    if text.ends_with('\n') {
        lines.push(Line::default());
    }

    lines
}

fn highlight_single_line(text: &str, syntax: &SyntaxReference) -> Vec<Span<'static>> {
    let line = text.lines().next().unwrap_or("");
    let mut highlighter = HighlightLines::new(syntax, highlight_theme());
    highlight_line(line, &mut highlighter).spans
}

fn highlight_line(line: &str, highlighter: &mut HighlightLines<'_>) -> Line<'static> {
    match highlighter.highlight_line(line, &SYNTAX_SET) {
        Ok(ranges) => {
            let spans = ranges
                .into_iter()
                .filter_map(|(style, segment)| {
                    let content = segment.strip_suffix('\n').unwrap_or(segment);
                    if content.is_empty() {
                        None
                    } else {
                        Some(Span::styled(
                            content.to_owned(),
                            ratatui_style(style.foreground, style.font_style),
                        ))
                    }
                })
                .collect::<Vec<_>>();
            if spans.is_empty() { Line::default() } else { Line::from(spans) }
        }
        Err(err) => {
            tracing::warn!("syntect highlight failed: {err}");
            Line::from(line.trim_end_matches('\n').to_owned())
        }
    }
}

fn plain_text_lines(text: &str) -> Vec<Line<'static>> {
    if text.is_empty() {
        return vec![Line::default()];
    }
    let mut lines: Vec<Line<'static>> =
        text.split('\n').map(|line| Line::from(line.to_owned())).collect();
    if lines.is_empty() {
        lines.push(Line::default());
    }
    lines
}

fn find_syntax(language: &str) -> Option<&'static SyntaxReference> {
    let token = language.trim();
    if token.is_empty() {
        return None;
    }
    SYNTAX_SET
        .find_syntax_by_token(token)
        .or_else(|| SYNTAX_SET.find_syntax_by_extension(token))
        .or_else(|| SYNTAX_SET.find_syntax_by_name(token))
}

fn highlight_theme() -> &'static Theme {
    THEME_SET
        .themes
        .get("base16-ocean.dark")
        .or_else(|| THEME_SET.themes.values().next())
        .unwrap_or(&FALLBACK_THEME)
}

fn ratatui_style(color: SyntectColor, font_style: FontStyle) -> Style {
    let mut style = Style::default().fg(Color::Rgb(color.r, color.g, color.b));

    if font_style.contains(FontStyle::BOLD) {
        style = style.add_modifier(Modifier::BOLD);
    }
    if font_style.contains(FontStyle::ITALIC) {
        style = style.add_modifier(Modifier::ITALIC);
    }
    if font_style.contains(FontStyle::UNDERLINE) {
        style = style.add_modifier(Modifier::UNDERLINED);
    }

    style
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn strip_ansi_removes_csi_sequences() {
        let input = "\u{1b}[31mred\u{1b}[0m plain";
        assert_eq!(strip_ansi(input), "red plain");
    }

    #[test]
    fn strip_ansi_removes_osc_sequences() {
        let input = "prefix\u{1b}]0;title\u{07}suffix";
        assert_eq!(strip_ansi(input), "prefixsuffix");
    }

    #[test]
    fn highlight_code_preserves_text() {
        let rendered = highlight_code("fn main() {}\n", Some("rs"));
        let text: String = rendered[0].spans.iter().map(|span| span.content.as_ref()).collect();
        assert!(text.contains("fn"));
        assert!(text.contains("main"));
    }

    #[test]
    fn highlight_shell_command_preserves_command() {
        let spans = highlight_shell_command("git diff --stat");
        let text: String = spans.iter().map(|span| span.content.as_ref()).collect();
        assert_eq!(text, "git diff --stat");
    }
}
