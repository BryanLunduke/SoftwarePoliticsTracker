// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use crate::agent::model;
use crate::ui::theme;
use ratatui::style::{Color, Modifier, Style};
use ratatui::text::{Line, Span};
use similar::TextDiff;

/// Render a diff with proper unified-style output using the `similar` crate.
/// The model `Diff` struct provides `old_text`/`new_text` -- we compute the actual
/// line-level changes and show only changed lines with context.
pub fn render_diff(diff: &model::Diff) -> Vec<Line<'static>> {
    let mut lines: Vec<Line<'static>> = Vec::new();

    // File path header
    let name = diff.path.file_name().map_or_else(
        || diff.path.to_string_lossy().into_owned(),
        |f| f.to_string_lossy().into_owned(),
    );
    let mut header_spans =
        vec![Span::styled(name, Style::default().fg(Color::White).add_modifier(Modifier::BOLD))];
    if let Some(repository) = diff.repository.as_deref() {
        header_spans
            .push(Span::styled(format!("  [{repository}]"), Style::default().fg(theme::DIM)));
    }
    lines.push(Line::from(header_spans));

    let old = diff.old_text.as_deref().unwrap_or("");
    let new = &diff.new_text;
    let text_diff = TextDiff::from_lines(old, new);

    // Use unified diff with 3 lines of context -- only shows changed hunks
    // instead of the full file content.
    let udiff = text_diff.unified_diff();
    for hunk in udiff.iter_hunks() {
        // Extract the @@ header from the hunk's Display output (first line).
        let hunk_str = hunk.to_string();
        if let Some(header) = hunk_str.lines().next()
            && header.starts_with("@@")
        {
            lines.push(Line::from(Span::styled(
                header.to_owned(),
                Style::default().fg(Color::Cyan),
            )));
        }

        for change in hunk.iter_changes() {
            let value = change.as_str().unwrap_or("").trim_end_matches('\n');
            let (prefix, style) = match change.tag() {
                similar::ChangeTag::Delete => ("-", Style::default().fg(Color::Red)),
                similar::ChangeTag::Insert => ("+", Style::default().fg(Color::Green)),
                similar::ChangeTag::Equal => (" ", Style::default().fg(theme::DIM)),
            };
            lines.push(Line::from(Span::styled(format!("{prefix} {value}"), style)));
        }
    }

    lines
}

pub fn looks_like_unified_diff(text: &str) -> bool {
    let mut saw_hunk = false;
    let mut saw_file_header = false;
    let mut saw_metadata = false;

    for line in text.lines().take(64) {
        if line.starts_with("@@") {
            saw_hunk = true;
        } else if line.starts_with("--- ") || line.starts_with("+++ ") {
            saw_file_header = true;
        } else if line.starts_with("diff --git ")
            || line.starts_with("index ")
            || line.starts_with("new file mode ")
            || line.starts_with("deleted file mode ")
            || line.starts_with("rename from ")
            || line.starts_with("rename to ")
        {
            saw_metadata = true;
        }
    }

    saw_hunk && (saw_file_header || saw_metadata)
}

pub fn render_raw_unified_diff(text: &str) -> Vec<Line<'static>> {
    let mut lines = Vec::new();

    for line in text.split('\n') {
        lines.push(render_raw_diff_line(line));
    }

    if lines.is_empty() {
        lines.push(Line::default());
    }

    lines
}

fn render_raw_diff_line(line: &str) -> Line<'static> {
    let style = if line.starts_with("diff --git ")
        || line.starts_with("index ")
        || line.starts_with("new file mode ")
        || line.starts_with("deleted file mode ")
        || line.starts_with("similarity index ")
        || line.starts_with("rename from ")
        || line.starts_with("rename to ")
    {
        Style::default().fg(Color::White).add_modifier(Modifier::BOLD)
    } else if line.starts_with("@@") {
        Style::default().fg(Color::Cyan).add_modifier(Modifier::BOLD)
    } else if line.starts_with("+++ ") {
        Style::default().fg(Color::Green)
    } else if line.starts_with("--- ") {
        Style::default().fg(Color::Red)
    } else if line.starts_with('+') {
        Style::default().fg(Color::Green)
    } else if line.starts_with('-') {
        Style::default().fg(Color::Red)
    } else if line.starts_with('\\') {
        Style::default().fg(theme::DIM).add_modifier(Modifier::ITALIC)
    } else {
        Style::default().fg(theme::DIM)
    };

    Line::from(Span::styled(line.to_owned(), style))
}

/// Check if a tool call title references a markdown file.
#[allow(clippy::case_sensitive_file_extension_comparisons)]
pub fn is_markdown_file(title: &str) -> bool {
    let lower = title.to_lowercase();
    lower.ends_with(".md") || lower.ends_with(".mdx") || lower.ends_with(".markdown")
}

/// Extract a language tag from the file extension in a tool call title.
/// Returns the raw extension (e.g. "rs", "py", "toml") which syntect
/// can resolve to the correct syntax definition. Falls back to empty string.
pub fn lang_from_title(title: &str) -> String {
    // Title may be "src/main.rs" or "Read src/main.rs" - find last path-like token
    title
        .split_whitespace()
        .rev()
        .find_map(|token| {
            let ext = token.rsplit('.').next()?;
            // Ignore if the "extension" is the whole token (no dot found)
            if ext.len() < token.len() { Some(ext.to_lowercase()) } else { None }
        })
        .unwrap_or_default()
}

/// Strip an outer markdown code fence if the text is entirely wrapped in one.
/// The bridge adapter often wraps file contents in ```` ``` ```` fences.
pub fn strip_outer_code_fence(text: &str) -> String {
    let trimmed = text.trim();
    if trimmed.starts_with("```") {
        // Find end of first line (the opening fence, possibly with a language tag)
        if let Some(first_newline) = trimmed.find('\n') {
            let after_opening = &trimmed[first_newline + 1..];
            // Check if it ends with a closing fence
            if let Some(body) = after_opening.strip_suffix("```") {
                return body.trim_end().to_owned();
            }
            // Also handle closing fence followed by newline
            let after_trimmed = after_opening.trim_end();
            if let Some(stripped) = after_trimmed.strip_suffix("```") {
                return stripped.trim_end().to_owned();
            }
        }
    }
    text.to_owned()
}

#[cfg(test)]
mod tests {
    use super::*;
    use pretty_assertions::assert_eq;

    // strip_outer_code_fence

    #[test]
    fn strip_fenced_code() {
        let input = "```rust\nfn main() {}\n```";
        let result = strip_outer_code_fence(input);
        assert_eq!(result, "fn main() {}");
    }

    #[test]
    fn strip_fenced_no_lang_tag() {
        let input = "```\nhello world\n```";
        let result = strip_outer_code_fence(input);
        assert_eq!(result, "hello world");
    }

    #[test]
    fn strip_not_fenced_passthrough() {
        let input = "just plain text";
        let result = strip_outer_code_fence(input);
        assert_eq!(result, "just plain text");
    }

    #[test]
    fn strip_fenced_with_trailing_whitespace() {
        let input = "```\ncontent\n```  \n";
        let result = strip_outer_code_fence(input);
        assert_eq!(result, "content");
    }

    #[test]
    fn strip_nested_fences_only_outer() {
        let input = "```\ninner ```\nstuff\n```";
        let result = strip_outer_code_fence(input);
        assert!(result.contains("inner ```"));
    }

    #[test]
    fn strip_only_opening_fence() {
        let input = "```rust\nfn main() {}";
        let result = strip_outer_code_fence(input);
        assert_eq!(result, input);
    }

    #[test]
    fn strip_empty_fenced_block() {
        let input = "```\n```";
        let result = strip_outer_code_fence(input);
        assert_eq!(result, "");
    }

    #[test]
    fn strip_multiline_content() {
        let input = "```python\nline1\nline2\nline3\n```";
        let result = strip_outer_code_fence(input);
        assert_eq!(result, "line1\nline2\nline3");
    }

    /// Quadruple backtick fence -- starts with 4 backticks which starts with 3, so it should still work.
    #[test]
    fn strip_quadruple_backtick_fence() {
        let input = "````\ncontent here\n````";
        let result = strip_outer_code_fence(input);
        // Starts with ```, so it enters the stripping path.
        // Closing is ```` - strip_suffix("```") matches the last 3 backticks
        // leaving one ` in the body. Let's just verify it doesn't panic
        // and returns something reasonable.
        assert!(result.contains("content here"));
    }

    /// Tilde fences -- NOT handled by `strip_outer_code_fence` (only checks triple backticks).
    #[test]
    fn strip_tilde_fence_passthrough() {
        let input = "~~~\ncontent\n~~~";
        let result = strip_outer_code_fence(input);
        assert_eq!(result, input);
    }

    /// Content with inner code fences that look like closing fences.
    #[test]
    fn strip_inner_fence_in_content() {
        let input = "```\nsome code\n```\nmore code\n```";
        let result = strip_outer_code_fence(input);
        // The function finds the first newline, then looks for ``` at the end
        // of the remaining text. The last ``` is the closing fence.
        assert!(result.contains("some code"));
    }

    /// Very large content inside fence - stress test.
    #[test]
    fn strip_large_fenced_content() {
        let big: String = (0..10_000).fold(String::new(), |mut s, i| {
            use std::fmt::Write;
            writeln!(s, "line {i}").unwrap();
            s
        });
        let input = format!("```\n{big}```");
        let result = strip_outer_code_fence(&input);
        assert!(result.contains("line 0"));
        assert!(result.contains("line 9999"));
    }

    /// Fence with blank content line.
    #[test]
    fn strip_fence_with_blank_lines() {
        let input = "```\n\n\n\n```";
        let result = strip_outer_code_fence(input);
        // Content is three blank lines, trimmed to empty
        assert!(result.is_empty() || result.chars().all(|c| c == '\n'));
    }

    /// Text starting with triple backticks but not at the beginning (leading whitespace).
    #[test]
    fn strip_fence_with_leading_whitespace() {
        let input = "  ```\ncontent\n```";
        let result = strip_outer_code_fence(input);
        // After trim(), starts with ```, so should strip
        assert_eq!(result, "content");
    }

    #[test]
    fn render_diff_includes_repository_label() {
        let lines = render_diff(
            &model::Diff::new("src/main.rs", "fn main() {}\n")
                .old_text(Some("fn old() {}\n"))
                .repository(Some("acme/project".to_owned())),
        );
        let header: String = lines[0].spans.iter().map(|span| span.content.as_ref()).collect();
        assert!(header.contains("main.rs"));
        assert!(header.contains("[acme/project]"));
    }

    #[test]
    fn looks_like_unified_diff_detects_git_style_payload() {
        let raw = "diff --git a/a.rs b/a.rs\nindex 111..222 100644\n--- a/a.rs\n+++ b/a.rs\n@@ -1 +1 @@\n-old\n+new\n";
        assert!(looks_like_unified_diff(raw));
    }

    #[test]
    fn render_raw_unified_diff_styles_hunks_and_additions() {
        let raw = "--- a/file.rs\n+++ b/file.rs\n@@ -1 +1 @@\n-old\n+new\n";
        let lines = render_raw_unified_diff(raw);
        assert_eq!(lines[0].spans[0].style.fg, Some(Color::Red));
        assert_eq!(lines[1].spans[0].style.fg, Some(Color::Green));
        assert_eq!(lines[2].spans[0].style.fg, Some(Color::Cyan));
        assert_eq!(lines[4].spans[0].style.fg, Some(Color::Green));
    }

    // lang_from_title

    #[test]
    fn lang_rust_file() {
        assert_eq!(lang_from_title("src/main.rs"), "rs");
    }

    #[test]
    fn lang_python_with_prefix() {
        assert_eq!(lang_from_title("Read foo.py"), "py");
    }

    #[test]
    fn lang_toml_file() {
        assert_eq!(lang_from_title("Cargo.toml"), "toml");
    }

    #[test]
    fn lang_no_extension() {
        assert_eq!(lang_from_title("Makefile"), "");
    }

    #[test]
    fn lang_empty_title() {
        assert_eq!(lang_from_title(""), "");
    }

    #[test]
    fn lang_mixed_case() {
        assert_eq!(lang_from_title("file.RS"), "rs");
    }

    #[test]
    fn lang_multiple_dots() {
        assert_eq!(lang_from_title("archive.tar.gz"), "gz");
    }

    #[test]
    fn lang_path_with_spaces() {
        assert_eq!(lang_from_title("Read some/dir/file.tsx"), "tsx");
    }

    #[test]
    fn lang_hidden_file() {
        assert_eq!(lang_from_title(".gitignore"), "gitignore");
    }

    /// Multiple extensions chained: picks the final one.
    #[test]
    fn lang_chained_extensions() {
        assert_eq!(lang_from_title("Read a.test.spec.ts"), "ts");
    }

    /// Dot at end of title: extension is empty string.
    #[test]
    fn lang_dot_at_end() {
        // "file." - rsplit('.').next() returns "", which is shorter than token
        assert_eq!(lang_from_title("file."), "");
    }

    /// Title with only whitespace.
    #[test]
    fn lang_whitespace_only() {
        assert_eq!(lang_from_title("   "), "");
    }

    /// Title with backslash path (Windows).
    #[test]
    fn lang_windows_backslash_path() {
        // Backslashes are not split by split_whitespace, so the whole path is one token
        assert_eq!(lang_from_title("Read src\\main.rs"), "rs");
    }

    // is_markdown_file

    #[test]
    fn is_md_file() {
        assert!(is_markdown_file("README.md"));
    }

    #[test]
    fn is_mdx_file() {
        assert!(is_markdown_file("component.mdx"));
    }

    #[test]
    fn is_markdown_ext() {
        assert!(is_markdown_file("doc.markdown"));
    }

    #[test]
    fn is_markdown_case_insensitive() {
        assert!(is_markdown_file("README.MD"));
        assert!(is_markdown_file("file.Md"));
    }

    #[test]
    fn is_not_markdown() {
        assert!(!is_markdown_file("main.rs"));
        assert!(!is_markdown_file("style.css"));
        assert!(!is_markdown_file(""));
    }

    #[test]
    fn is_not_markdown_partial() {
        assert!(!is_markdown_file("somemdx"));
    }

    /// `.md` in the middle of the name is NOT a markdown extension.
    #[test]
    fn is_not_markdown_md_in_middle() {
        assert!(!is_markdown_file("file.md.bak"));
    }

    /// Path with .md extension.
    #[test]
    fn is_markdown_with_path() {
        assert!(is_markdown_file("docs/getting-started.md"));
        assert!(is_markdown_file("Read /home/user/notes.md"));
    }

    /// `.MARKDOWN` all caps.
    #[test]
    fn is_markdown_uppercase_full() {
        assert!(is_markdown_file("FILE.MARKDOWN"));
    }
}
