// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

//! Tool-call rendering: entry points, caching, and shared helpers.
//!
//! Submodules handle specific rendering concerns:
//! - [`standard`] -- non-Execute tool calls (Read, Write, Glob, etc.)
//! - [`execute`] -- Execute/Bash two-layer bordered rendering
//! - [`interactions`] -- inline permissions, questions, and plan approvals
//! - [`errors`] -- error rendering and tool-use error extraction

mod errors;
mod execute;
mod interactions;
mod standard;

use crate::agent::model;
use crate::app::ToolCallInfo;
use crate::ui::markdown;
use crate::ui::theme;
use ratatui::style::{Color, Modifier, Style};
use ratatui::text::{Line, Span, Text};
use ratatui::widgets::{Paragraph, Wrap};
use unicode_width::{UnicodeWidthChar, UnicodeWidthStr};

// Re-export submodule items used by tests.
#[cfg(test)]
use errors::{
    extract_tool_use_error_message, looks_like_internal_error, render_tool_use_error_content,
    summarize_internal_error,
};

#[cfg(test)]
use standard::{cap_write_diff_lines, content_summary};

/// Spinner frames as `&'static str` for use in `status_icon` return type.
const SPINNER_STRS: &[&str] = &[
    "\u{280B}", "\u{2819}", "\u{2839}", "\u{2838}", "\u{283C}", "\u{2834}", "\u{2826}", "\u{2827}",
    "\u{2807}", "\u{280F}",
];

pub fn status_icon(status: model::ToolCallStatus, spinner_frame: usize) -> (&'static str, Color) {
    match status {
        model::ToolCallStatus::Pending => ("\u{25CB}", theme::RUST_ORANGE),
        model::ToolCallStatus::InProgress => {
            let s = SPINNER_STRS[spinner_frame % SPINNER_STRS.len()];
            (s, theme::RUST_ORANGE)
        }
        model::ToolCallStatus::Completed => (theme::ICON_COMPLETED, theme::RUST_ORANGE),
        model::ToolCallStatus::Failed => (theme::ICON_FAILED, theme::STATUS_ERROR),
    }
}

// ---------------------------------------------------------------------------
// Public entry points (delegating to submodules)
// ---------------------------------------------------------------------------

/// Render a tool call with caching. Only re-renders when cache is stale.
///
/// For Execute/Bash tool calls, the cache stores **content only** (command, output,
/// permissions) without border decoration. Borders are applied at render time using
/// the current width, so they always fill the terminal correctly after resize.
/// Height for Execute = `content_lines + 2` (title border + bottom border).
///
/// For other tool calls, the title is rendered live and the expanded body is cached
/// independently, so session collapse preference can change without invalidating
/// every completed tool-call cache.
#[allow(dead_code)]
pub fn render_tool_call_cached(
    tc: &mut ToolCallInfo,
    width: u16,
    spinner_frame: usize,
    out: &mut Vec<Line<'static>>,
) {
    render_tool_call_cached_with_tools_collapsed(tc, width, spinner_frame, false, out);
}

pub fn render_tool_call_cached_with_tools_collapsed(
    tc: &mut ToolCallInfo,
    width: u16,
    spinner_frame: usize,
    tools_collapsed: bool,
    out: &mut Vec<Line<'static>>,
) {
    let is_execute = tc.is_execute_tool();

    // Execute/Bash: two-layer rendering (cache content, apply borders at render time)
    if is_execute {
        if tc.cache.get().is_none() {
            crate::perf::mark("tc::cache_miss_execute");
            let _t = crate::perf::start("tc::render_exec");
            let content = execute::render_execute_content(tc);
            tc.cache.store(content);
        } else {
            crate::perf::mark("tc::cache_hit_execute");
        }
        if let Some(content) = tc.cache.get() {
            let bordered = execute::render_execute_with_borders(tc, content, width, spinner_frame);
            out.extend(bordered);
        }
        return;
    }

    let title = standard::render_tool_call_title(tc, width, spinner_frame);
    out.push(title);

    let has_body = !(tc.content.is_empty()
        && tc.pending_permission.is_none()
        && tc.pending_question.is_none());
    if !has_body {
        return;
    }

    if standard::tool_call_effectively_collapsed(tc, tools_collapsed) {
        standard::render_collapsed_tool_call_summary(tc, out);
        return;
    }

    // Expanded body: use cache if valid, otherwise render and cache.
    if let Some(cached_body) = tc.cache.get() {
        crate::perf::mark_with("tc::cache_hit_body", "lines", cached_body.len());
        out.extend_from_slice(cached_body);
    } else {
        crate::perf::mark("tc::cache_miss_body");
        let _t = crate::perf::start("tc::render_body");
        let body = standard::render_tool_call_body(tc);
        tc.cache.store(body);
        if let Some(stored) = tc.cache.get() {
            out.extend_from_slice(stored);
        }
    }
}

/// Ensure tool call caches are up-to-date and return visual wrapped height at `width`.
/// Returns `(height, lines_wrapped_for_measurement)`.
#[allow(dead_code)]
pub fn measure_tool_call_height_cached(
    tc: &mut ToolCallInfo,
    width: u16,
    spinner_frame: usize,
    layout_generation: u64,
) -> (usize, usize) {
    measure_tool_call_height_cached_with_tools_collapsed(
        tc,
        width,
        spinner_frame,
        layout_generation,
        false,
    )
}

pub fn measure_tool_call_height_cached_with_tools_collapsed(
    tc: &mut ToolCallInfo,
    width: u16,
    spinner_frame: usize,
    layout_generation: u64,
    tools_collapsed: bool,
) -> (usize, usize) {
    if tc.cache_measurement_key_matches(width, layout_generation) {
        crate::perf::mark("tc_measure_fast_path_hits");
        return (tc.last_measured_height, 0);
    }
    crate::perf::mark("tc_measure_recompute_count");

    let is_execute = tc.is_execute_tool();
    if is_execute {
        if tc.cache.get().is_none() {
            let content = execute::render_execute_content(tc);
            tc.cache.store(content);
        }
        if let Some(content) = tc.cache.get() {
            let bordered = execute::render_execute_with_borders(tc, content, width, spinner_frame);
            let h = Paragraph::new(Text::from(bordered.clone()))
                .wrap(Wrap { trim: false })
                .line_count(width);
            tc.cache.set_height(h, width);
            tc.record_measured_height(width, h, layout_generation);
            return (h, bordered.len());
        }
        tc.record_measured_height(width, 0, layout_generation);
        return (0, 0);
    }

    let title = standard::render_tool_call_title(tc, width, spinner_frame);
    let title_h =
        Paragraph::new(Text::from(vec![title])).wrap(Wrap { trim: false }).line_count(width);
    let has_body = !(tc.content.is_empty()
        && tc.pending_permission.is_none()
        && tc.pending_question.is_none());

    if !has_body {
        tc.record_measured_height(width, title_h, layout_generation);
        return (title_h, 1);
    }

    if standard::tool_call_effectively_collapsed(tc, tools_collapsed) {
        let mut summary = Vec::new();
        standard::render_collapsed_tool_call_summary(tc, &mut summary);
        let summary_h = Paragraph::new(Text::from(summary.clone()))
            .wrap(Wrap { trim: false })
            .line_count(width);
        let total = title_h + summary_h;
        tc.record_measured_height(width, total, layout_generation);
        return (total, 1 + summary.len());
    }

    if let Some(body_h) = tc.cache.height_at(width) {
        let total = title_h + body_h;
        tc.record_measured_height(width, total, layout_generation);
        return (total, 1);
    }
    if let Some(body_h) = tc.cache.measure_and_set_height(width) {
        let total = title_h + body_h;
        tc.record_measured_height(width, total, layout_generation);
        return (total, tc.cache.get().map_or(1, |b| b.len() + 1));
    }

    let body = standard::render_tool_call_body(tc);
    let body_h =
        Paragraph::new(Text::from(body.clone())).wrap(Wrap { trim: false }).line_count(width);
    tc.cache.store(body);
    tc.cache.set_height(body_h, width);
    let total = title_h + body_h;
    tc.record_measured_height(width, total, layout_generation);
    (total, tc.cache.get().map_or(1, |b| b.len() + 1))
}

// ---------------------------------------------------------------------------
// Shared helpers (used by multiple submodules)
// ---------------------------------------------------------------------------

fn markdown_inline_spans(input: &str) -> Vec<Span<'static>> {
    markdown::render_markdown_safe(input, None).into_iter().next().map_or_else(Vec::new, |line| {
        line.spans.into_iter().map(|s| Span::styled(s.content.into_owned(), s.style)).collect()
    })
}

fn spans_width(spans: &[Span<'static>]) -> usize {
    spans.iter().map(|s| UnicodeWidthStr::width(s.content.as_ref())).sum()
}

fn truncate_spans_to_width(spans: Vec<Span<'static>>, max_width: usize) -> Vec<Span<'static>> {
    if max_width == 0 {
        return Vec::new();
    }
    if spans_width(&spans) <= max_width {
        return spans;
    }

    let keep_width = max_width.saturating_sub(1);
    let mut used = 0usize;
    let mut out: Vec<Span<'static>> = Vec::new();

    for span in spans {
        if used >= keep_width {
            break;
        }
        let mut chunk = String::new();
        for ch in span.content.chars() {
            let w = UnicodeWidthChar::width(ch).unwrap_or(0);
            if used + w > keep_width {
                break;
            }
            chunk.push(ch);
            used += w;
        }
        if !chunk.is_empty() {
            out.push(Span::styled(chunk, span.style));
        }
    }
    out.push(Span::styled("\u{2026}", Style::default().fg(theme::DIM)));
    out
}

fn tool_output_badge_spans(tc: &ToolCallInfo) -> Vec<Span<'static>> {
    let mut badges = Vec::new();

    if tc.assistant_auto_backgrounded() {
        badges.push(Span::styled(
            "  [assistant backgrounded]",
            Style::default().fg(Color::Yellow).add_modifier(Modifier::BOLD),
        ));
    }

    if tc.is_ultraplan() {
        badges.push(Span::styled(
            "  [ultraplan]",
            Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD),
        ));
    }

    if tc.verification_nudge_needed() {
        badges.push(Span::styled(
            "  [verification needed]",
            Style::default().fg(Color::Yellow).add_modifier(Modifier::BOLD),
        ));
    }

    badges
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;
    use crate::app::BlockCache;
    use pretty_assertions::assert_eq;

    fn test_tool_call(
        id: &str,
        sdk_tool_name: &str,
        status: model::ToolCallStatus,
    ) -> ToolCallInfo {
        ToolCallInfo {
            id: id.to_owned(),
            title: id.to_owned(),
            sdk_tool_name: sdk_tool_name.to_owned(),
            raw_input: None,
            raw_input_bytes: 0,
            output_metadata: None,
            status,
            content: Vec::new(),
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

    // status_icon

    #[test]
    fn status_icon_pending() {
        let (icon, color) = status_icon(model::ToolCallStatus::Pending, 0);
        assert!(!icon.is_empty());
        assert_eq!(color, theme::RUST_ORANGE);
    }

    #[test]
    fn status_icon_in_progress() {
        let (icon, color) = status_icon(model::ToolCallStatus::InProgress, 3);
        assert!(!icon.is_empty());
        assert_eq!(color, theme::RUST_ORANGE);
    }

    #[test]
    fn status_icon_completed() {
        let (icon, color) = status_icon(model::ToolCallStatus::Completed, 0);
        assert_eq!(icon, theme::ICON_COMPLETED);
        assert_eq!(color, theme::RUST_ORANGE);
    }

    #[test]
    fn status_icon_failed() {
        let (icon, color) = status_icon(model::ToolCallStatus::Failed, 0);
        assert_eq!(icon, theme::ICON_FAILED);
        assert_eq!(color, theme::STATUS_ERROR);
    }

    #[test]
    fn status_icon_spinner_wraps() {
        let (icon_a, _) = status_icon(model::ToolCallStatus::InProgress, 0);
        let (icon_b, _) = status_icon(model::ToolCallStatus::InProgress, SPINNER_STRS.len());
        assert_eq!(icon_a, icon_b);
    }

    #[test]
    fn status_icon_all_spinner_frames_valid() {
        for i in 0..SPINNER_STRS.len() {
            let (icon, _) = status_icon(model::ToolCallStatus::InProgress, i);
            assert!(!icon.is_empty());
        }
    }

    /// Spinner frames are all distinct.
    #[test]
    fn status_icon_spinner_frames_distinct() {
        let frames: Vec<&str> = (0..SPINNER_STRS.len())
            .map(|i| status_icon(model::ToolCallStatus::InProgress, i).0)
            .collect();
        for i in 0..frames.len() {
            for j in (i + 1)..frames.len() {
                assert_ne!(frames[i], frames[j], "frames {i} and {j} are identical");
            }
        }
    }

    /// Large spinner frame number wraps correctly.
    #[test]
    fn status_icon_spinner_large_frame() {
        let (icon, _) = status_icon(model::ToolCallStatus::Pending, 999_999);
        assert!(!icon.is_empty());
    }

    #[test]
    fn truncate_spans_adds_ellipsis_when_needed() {
        let spans = vec![Span::raw("abcdefghijklmnopqrstuvwxyz")];
        let out = truncate_spans_to_width(spans, 8);
        let rendered: String = out.iter().map(|s| s.content.as_ref()).collect();
        assert_eq!(rendered, "abcdefg\u{2026}");
        assert!(spans_width(&out) <= 8);
    }

    #[test]
    fn markdown_inline_spans_removes_markdown_syntax() {
        let spans = markdown_inline_spans("**Allow** _once_");
        let rendered: String = spans.iter().map(|s| s.content.as_ref()).collect();
        assert!(rendered.contains("Allow"));
        assert!(rendered.contains("once"));
        assert!(!rendered.contains('*'));
        assert!(!rendered.contains('_'));
    }

    #[test]
    fn execute_top_border_does_not_wrap_for_long_title() {
        let tc = ToolCallInfo {
            id: "tc-1".into(),
            title: "echo very long command title with markdown **bold** and path /a/b/c/d/e/f"
                .into(),
            sdk_tool_name: "Bash".into(),
            raw_input: None,
            raw_input_bytes: 0,
            output_metadata: None,
            status: model::ToolCallStatus::Pending,
            content: Vec::new(),
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
        };

        let rendered = execute::render_execute_with_borders(&tc, &[], 80, 0);
        let top = rendered.first().expect("top border line");
        assert!(spans_width(&top.spans) <= 80);
    }

    #[test]
    fn execute_title_renders_assistant_backgrounded_badge() {
        let mut tc = test_tool_call("tc-bash-bg", "Bash", model::ToolCallStatus::Completed);
        tc.output_metadata =
            Some(model::ToolOutputMetadata::new().bash(Some(
                model::BashOutputMetadata::new().assistant_auto_backgrounded(Some(true)),
            )));

        let rendered = execute::render_execute_with_borders(&tc, &[], 100, 0);
        let top = rendered.first().expect("top border line");
        let text: String = top.spans.iter().map(|span| span.content.as_ref()).collect();
        assert!(text.contains("[assistant backgrounded]"));
    }

    #[test]
    fn execute_measure_fast_path_reuses_cached_height() {
        let mut tc = test_tool_call("tc-fast", "Bash", model::ToolCallStatus::InProgress);
        tc.terminal_command = Some("echo hi".to_owned());
        tc.terminal_output = Some("hello\nworld".to_owned());

        let (h1, lines1) = measure_tool_call_height_cached(&mut tc, 80, 0, 1);
        assert!(h1 > 0);
        assert!(lines1 > 0);

        let (h2, lines2) = measure_tool_call_height_cached(&mut tc, 80, 4, 1);
        assert_eq!(h2, h1);
        assert_eq!(lines2, 0);
    }

    #[test]
    fn execute_measure_recomputes_on_layout_generation_change() {
        let mut tc = test_tool_call("tc-layout-gen", "Bash", model::ToolCallStatus::InProgress);
        tc.terminal_command = Some("echo hi".to_owned());
        tc.terminal_output = Some("hello".to_owned());

        let (_, first_lines) = measure_tool_call_height_cached(&mut tc, 80, 0, 1);
        assert!(first_lines > 0);
        let (_, second_lines) = measure_tool_call_height_cached(&mut tc, 80, 0, 2);
        assert!(second_lines > 0);
    }

    #[test]
    fn layout_dirty_invalidates_measure_fast_path() {
        let mut tc = test_tool_call("tc-dirty", "Read", model::ToolCallStatus::Completed);
        tc.content = vec![model::ToolCallContent::from("one line")];

        let (_, first_lines) = measure_tool_call_height_cached(&mut tc, 80, 0, 1);
        assert!(first_lines > 0);
        let (_, fast_lines) = measure_tool_call_height_cached(&mut tc, 80, 0, 1);
        assert_eq!(fast_lines, 0);

        tc.mark_tool_call_layout_dirty();
        let (_, recompute_lines) = measure_tool_call_height_cached(&mut tc, 80, 0, 1);
        assert!(recompute_lines > 0);
    }

    #[test]
    fn exit_plan_mode_title_renders_ultraplan_badge() {
        let mut tc = test_tool_call("tc-plan", "ExitPlanMode", model::ToolCallStatus::Completed);
        tc.output_metadata =
            Some(model::ToolOutputMetadata::new().exit_plan_mode(Some(
                model::ExitPlanModeOutputMetadata::new().ultraplan(Some(true)),
            )));

        let rendered = standard::render_tool_call_title(&tc, 80, 0);
        let text: String = rendered.spans.iter().map(|span| span.content.as_ref()).collect();
        assert!(text.contains("[ultraplan]"));
    }

    #[test]
    fn todo_write_title_renders_verification_badge() {
        let mut tc = test_tool_call("tc-todo", "TodoWrite", model::ToolCallStatus::Completed);
        tc.output_metadata = Some(model::ToolOutputMetadata::new().todo_write(Some(
            model::TodoWriteOutputMetadata::new().verification_nudge_needed(Some(true)),
        )));

        let rendered = standard::render_tool_call_title(&tc, 80, 0);
        let text: String = rendered.spans.iter().map(|span| span.content.as_ref()).collect();
        assert!(text.contains("[verification needed]"));
    }

    #[test]
    fn mcp_resource_body_renders_saved_path_hint_when_text_omits_it() {
        let mut tc =
            test_tool_call("tc-mcp-resource", "ReadMcpResource", model::ToolCallStatus::Completed);
        tc.content = vec![model::ToolCallContent::McpResource(
            model::McpResource::new("file://manual.pdf")
                .mime_type(Some("application/pdf".to_owned()))
                .text(Some("Binary resource downloaded successfully.".to_owned()))
                .blob_saved_to(Some("C:\\tmp\\manual.pdf".to_owned())),
        )];

        let body = standard::render_tool_call_body(&tc);
        let rendered: Vec<String> = body
            .iter()
            .map(|line| line.spans.iter().map(|span| span.content.as_ref()).collect())
            .collect();

        assert!(
            rendered.iter().any(|line| line.contains("Binary resource downloaded successfully."))
        );
        assert!(rendered.iter().any(|line| line.contains("Saved to: C:\\tmp\\manual.pdf")));
    }

    #[test]
    fn mcp_resource_body_avoids_duplicate_saved_path_hint_when_text_already_mentions_it() {
        let mut tc = test_tool_call(
            "tc-mcp-resource-dupe",
            "ReadMcpResource",
            model::ToolCallStatus::Completed,
        );
        tc.content = vec![model::ToolCallContent::McpResource(
            model::McpResource::new("file://manual.pdf")
                .mime_type(Some("application/pdf".to_owned()))
                .text(Some(
                    "[Resource from docs at file://manual.pdf] Saved to C:\\tmp\\manual.pdf"
                        .to_owned(),
                ))
                .blob_saved_to(Some("C:\\tmp\\manual.pdf".to_owned())),
        )];

        let body = standard::render_tool_call_body(&tc);
        let rendered: Vec<String> = body
            .iter()
            .map(|line| line.spans.iter().map(|span| span.content.as_ref()).collect())
            .collect();

        assert_eq!(
            rendered.iter().filter(|line| line.contains("Saved to: C:\\tmp\\manual.pdf")).count(),
            0
        );
    }

    #[test]
    fn completed_non_execute_collapse_uses_session_preference_without_mutating_cache() {
        let mut tc = test_tool_call("tc-collapse", "Read", model::ToolCallStatus::Completed);
        tc.content = vec![model::ToolCallContent::from("alpha\nbeta".to_owned())];

        let mut expanded = Vec::new();
        render_tool_call_cached_with_tools_collapsed(&mut tc, 80, 0, false, &mut expanded);
        let expanded_text: Vec<String> = expanded
            .iter()
            .map(|line| line.spans.iter().map(|span| span.content.as_ref()).collect())
            .collect();
        assert!(expanded_text.iter().any(|line| line.contains("alpha")));
        assert!(!expanded_text.iter().any(|line| line.contains("ctrl+o to expand")));

        let mut collapsed = Vec::new();
        render_tool_call_cached_with_tools_collapsed(&mut tc, 80, 0, true, &mut collapsed);
        let collapsed_text: Vec<String> = collapsed
            .iter()
            .map(|line| line.spans.iter().map(|span| span.content.as_ref()).collect())
            .collect();
        assert!(collapsed_text.iter().any(|line| line.contains("ctrl+o to expand")));
        assert!(!collapsed_text.iter().any(|line| line.contains("beta")));
    }

    #[test]
    fn completed_non_execute_measurement_changes_with_session_collapse_preference() {
        let mut tc =
            test_tool_call("tc-measure-collapse", "Read", model::ToolCallStatus::Completed);
        tc.content = vec![model::ToolCallContent::from("alpha\nbeta\ngamma\ndelta".to_owned())];

        let (expanded_h, _) =
            measure_tool_call_height_cached_with_tools_collapsed(&mut tc, 24, 0, 1, false);
        let (collapsed_h, _) =
            measure_tool_call_height_cached_with_tools_collapsed(&mut tc, 24, 0, 2, true);

        assert!(collapsed_h < expanded_h);
    }

    #[test]
    fn diff_tool_stays_expanded_when_session_prefers_collapsed() {
        let mut tc = test_tool_call("tc-diff", "Write", model::ToolCallStatus::Completed);
        tc.content = vec![model::ToolCallContent::Diff(
            model::Diff::new("src/main.rs", "new".to_owned()).old_text(Some("old".to_owned())),
        )];

        let mut rendered = Vec::new();
        render_tool_call_cached_with_tools_collapsed(&mut tc, 80, 0, true, &mut rendered);
        let text: Vec<String> = rendered
            .iter()
            .map(|line| line.spans.iter().map(|span| span.content.as_ref()).collect())
            .collect();

        assert!(!text.iter().any(|line| line.contains("ctrl+o to expand")));
        assert!(text.len() > 2, "expanded diff should render more than title plus summary");
    }

    #[test]
    fn internal_error_detection_accepts_xml_payload() {
        let payload =
            "<error><code>-32603</code><message>Adapter process crashed</message></error>";
        assert!(looks_like_internal_error(payload));
    }

    #[test]
    fn internal_error_detection_rejects_plain_bash_failure() {
        let payload = "bash: unknown_command: command not found";
        assert!(!looks_like_internal_error(payload));
    }

    #[test]
    fn summarize_internal_error_prefers_xml_message() {
        let payload =
            "<error><code>-32603</code><message>Adapter process crashed</message></error>";
        assert_eq!(summarize_internal_error(payload), "Adapter process crashed");
    }

    #[test]
    fn summarize_internal_error_reads_json_rpc_message() {
        let payload = r#"{"jsonrpc":"2.0","error":{"code":-32603,"message":"internal rpc fault"}}"#;
        assert_eq!(summarize_internal_error(payload), "internal rpc fault");
    }

    #[test]
    fn extract_tool_use_error_message_reads_inner_text() {
        let payload = "<tool_use_error>Sibling tool call errored</tool_use_error>";
        assert_eq!(
            extract_tool_use_error_message(payload).as_deref(),
            Some("Sibling tool call errored")
        );
    }

    #[test]
    fn render_tool_use_error_content_shows_only_inner_text_lines() {
        let lines = render_tool_use_error_content("Line A\nLine B");
        let rendered: Vec<String> = lines
            .iter()
            .map(|line| line.spans.iter().map(|s| s.content.as_ref()).collect())
            .collect();
        assert_eq!(rendered, vec!["Line A", "Line B"]);
    }

    #[test]
    fn content_summary_only_extracts_tool_use_error_for_failed_execute() {
        let tc = ToolCallInfo {
            id: "tc-1".into(),
            title: "Bash".into(),
            sdk_tool_name: "Bash".into(),
            raw_input: None,
            raw_input_bytes: 0,
            output_metadata: None,
            status: model::ToolCallStatus::Completed,
            content: Vec::new(),
            hidden: false,
            terminal_id: Some("term-1".into()),
            terminal_command: Some("echo done".into()),
            terminal_output: Some("<tool_use_error>bad</tool_use_error>\ndone".into()),
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
        };
        assert_eq!(content_summary(&tc), "done");
    }

    #[test]
    fn content_summary_extracts_tool_use_error_for_failed_execute() {
        let tc = ToolCallInfo {
            id: "tc-1".into(),
            title: "Bash".into(),
            sdk_tool_name: "Bash".into(),
            raw_input: None,
            raw_input_bytes: 0,
            output_metadata: None,
            status: model::ToolCallStatus::Failed,
            content: Vec::new(),
            hidden: false,
            terminal_id: Some("term-1".into()),
            terminal_command: Some("echo done".into()),
            terminal_output: Some("<tool_use_error>bad</tool_use_error>\ndone".into()),
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
        };
        assert_eq!(content_summary(&tc), "bad");
    }

    #[test]
    fn content_summary_uses_first_terminal_line_for_failed_execute() {
        let tc = ToolCallInfo {
            id: "tc-2".into(),
            title: "Bash".into(),
            sdk_tool_name: "Bash".into(),
            raw_input: None,
            raw_input_bytes: 0,
            output_metadata: None,
            status: model::ToolCallStatus::Failed,
            content: Vec::new(),
            hidden: false,
            terminal_id: Some("term-2".into()),
            terminal_command: Some("cd path with spaces".into()),
            terminal_output: Some(
                "Exit code 1\n/usr/bin/bash: line 1: cd: too many arguments\nmore detail".into(),
            ),
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
        };
        assert_eq!(content_summary(&tc), "Exit code 1");
    }

    #[test]
    fn render_execute_content_failed_keeps_single_output_line() {
        let tc = ToolCallInfo {
            id: "tc-3".into(),
            title: "Bash".into(),
            sdk_tool_name: "Bash".into(),
            raw_input: None,
            raw_input_bytes: 0,
            output_metadata: None,
            status: model::ToolCallStatus::Failed,
            content: Vec::new(),
            hidden: false,
            terminal_id: Some("term-3".into()),
            terminal_command: Some("cd path with spaces".into()),
            terminal_output: Some(
                "Exit code 1\n/usr/bin/bash: line 1: cd: too many arguments\nmore detail".into(),
            ),
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
        };

        let lines = execute::render_execute_content(&tc);
        let rendered: Vec<String> = lines
            .iter()
            .map(|line| line.spans.iter().map(|s| s.content.as_ref()).collect())
            .collect();
        assert_eq!(rendered.len(), 2);
        assert_eq!(rendered[1], "Exit code 1");
    }

    #[test]
    fn write_diff_cap_keeps_head_and_tail_with_omission_marker() {
        use standard::WRITE_DIFF_HEAD_LINES;
        use standard::WRITE_DIFF_MAX_LINES;

        let lines: Vec<Line<'static>> =
            (0..120).map(|idx| Line::from(format!("line {idx}"))).collect();
        let capped = cap_write_diff_lines(lines);
        let rendered: Vec<String> = capped
            .iter()
            .map(|line| line.spans.iter().map(|s| s.content.as_ref()).collect())
            .collect();

        assert_eq!(rendered.len(), WRITE_DIFF_MAX_LINES);
        assert_eq!(rendered[0], "line 0");
        assert_eq!(rendered[WRITE_DIFF_HEAD_LINES - 1], "line 9");
        assert_eq!(rendered[WRITE_DIFF_HEAD_LINES], "");
        assert!(rendered[WRITE_DIFF_HEAD_LINES + 1].contains("73 diff lines omitted"));
        assert_eq!(rendered[WRITE_DIFF_HEAD_LINES + 2], "");
        assert_eq!(rendered[WRITE_DIFF_HEAD_LINES + 3], "line 83");
        assert_eq!(rendered.last().map(String::as_str), Some("line 119"));
    }
}
