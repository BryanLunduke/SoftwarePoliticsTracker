// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use crate::app::cache_metrics;
use crate::app::{App, AppStatus, MessageBlock, MessageRole, SelectionKind, SelectionState};
use crate::ui::message::{self, SpinnerState};
use crate::ui::theme;
use ratatui::Frame;
use ratatui::buffer::Buffer;
use ratatui::layout::Rect;
use ratatui::style::{Modifier, Style};
use ratatui::text::{Line, Text};
use ratatui::widgets::{Paragraph, Widget, Wrap};
use std::time::Instant;

/// Minimum number of messages to render above/below the visible range as a margin.
/// Heights are now exact (block-level wrapped heights), so no safety margin is needed.
const CULLING_MARGIN: usize = 0;
const CULLING_OVERSCAN_ROWS: usize = 100;
const SCROLLBAR_MIN_THUMB_HEIGHT: usize = 1;
const SCROLLBAR_TOP_EASE: f32 = 0.35;
const SCROLLBAR_SIZE_EASE: f32 = 0.2;
const SCROLLBAR_EASE_EPSILON: f32 = 0.01;
const OVERSCROLL_CLAMP_EASE: f32 = 0.2;

#[derive(Clone, Copy, Default)]
struct HeightUpdateStats {
    measured_msgs: usize,
    measured_lines: usize,
    reused_msgs: usize,
}

#[derive(Clone, Copy, Default)]
struct RemeasureBudget {
    remaining_msgs: usize,
    remaining_lines: usize,
}

impl RemeasureBudget {
    fn new(viewport_height: usize) -> Self {
        let viewport_floor = viewport_height.max(12);
        Self {
            remaining_msgs: viewport_floor,
            remaining_lines: viewport_floor.saturating_mul(8).max(256),
        }
    }

    fn exhausted(self) -> bool {
        self.remaining_msgs == 0 || self.remaining_lines == 0
    }

    fn consume(&mut self, wrapped_lines: usize) {
        self.remaining_msgs = self.remaining_msgs.saturating_sub(1);
        self.remaining_lines = self.remaining_lines.saturating_sub(wrapped_lines.max(1));
    }
}

#[derive(Clone, Copy, Default)]
struct CulledRenderStats {
    local_scroll: usize,
    first_visible: usize,
    render_start: usize,
    rendered_msgs: usize,
    last_rendered_idx: Option<usize>,
    rendered_line_count: usize,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
struct ScrollbarGeometry {
    thumb_top: usize,
    thumb_size: usize,
}

struct ScrolledRenderData {
    paragraph: Paragraph<'static>,
    stats: CulledRenderStats,
    max_scroll: usize,
    scroll_offset: usize,
}
/// Build a `SpinnerState` for a specific message index.
fn msg_spinner(
    base: SpinnerState,
    index: usize,
    active_turn_assistant: Option<usize>,
    msg: &crate::app::ChatMessage,
) -> SpinnerState {
    let is_assistant = matches!(msg.role, MessageRole::Assistant);
    let is_active_turn_assistant = is_assistant && active_turn_assistant == Some(index);
    let has_blocks = !msg.blocks.is_empty();
    SpinnerState {
        is_active_turn_assistant,
        show_empty_thinking: is_active_turn_assistant && base.show_empty_thinking,
        show_thinking: is_active_turn_assistant && base.show_thinking && has_blocks,
        show_subagent_thinking: is_active_turn_assistant
            && base.show_subagent_thinking
            && has_blocks,
        show_compacting: is_active_turn_assistant && base.show_compacting,
        ..base
    }
}

/// Ensure every message has an up-to-date height in the viewport at the given width.
/// The last message is always recomputed while streaming (content changes each frame).
///
/// Height is ground truth: each message is rendered into a scratch buffer via
/// `render_message()` and measured with `Paragraph::line_count(width)`. This uses
/// the exact same wrapping algorithm as the actual render path, so heights can
/// never drift from reality.
///
/// Iterates in reverse so we can break early: once we hit a message whose height
/// is already valid at this width, all earlier messages are also valid (content
/// only changes at the tail during streaming). This turns the common case from
/// O(n) to O(1).
fn update_visual_heights(
    app: &mut App,
    base: SpinnerState,
    width: u16,
    viewport_height: usize,
) -> HeightUpdateStats {
    let _t =
        app.perf.as_ref().map(|p| p.start_with("chat::update_heights", "msgs", app.messages.len()));
    app.viewport.sync_message_count(app.messages.len());

    let msg_count = app.messages.len();
    let is_streaming = matches!(app.status, AppStatus::Thinking | AppStatus::Running);
    let active_turn_assistant = app.active_turn_assistant_idx();
    let mut stats = HeightUpdateStats::default();

    if msg_count == 0 {
        app.viewport.finalize_remeasure_if_clean();
        return stats;
    }

    let (visible_start, visible_end) = app
        .viewport
        .remeasure_anchor_window(viewport_height)
        .or_else(|| app.viewport.current_visible_window(viewport_height))
        .unwrap_or((0, 0));
    app.viewport.ensure_remeasure_anchor(visible_start, visible_end, msg_count);

    while let Some(i) = app.viewport.next_priority_remeasure() {
        let is_last = i + 1 == msg_count;
        if !needs_height_measure(app, i, is_last, active_turn_assistant, is_streaming) {
            stats.reused_msgs += 1;
            continue;
        }
        measure_message_height_at(app, base, active_turn_assistant, width, i, &mut stats);
    }

    for i in visible_start..=visible_end {
        let is_last = i + 1 == msg_count;
        if !needs_height_measure(app, i, is_last, active_turn_assistant, is_streaming) {
            stats.reused_msgs += 1;
            continue;
        }
        measure_message_height_at(app, base, active_turn_assistant, width, i, &mut stats);
    }

    if is_streaming {
        let last = msg_count.saturating_sub(1);
        if needs_height_measure(app, last, true, active_turn_assistant, true) {
            measure_message_height_at(app, base, active_turn_assistant, width, last, &mut stats);
        }
    }

    let mut budget = RemeasureBudget::new(viewport_height);
    while app.viewport.remeasure_active() && !budget.exhausted() {
        let Some(i) = app.viewport.next_remeasure_index(msg_count) else {
            break;
        };
        if (visible_start..=visible_end).contains(&i) {
            continue;
        }
        let is_last = i + 1 == msg_count;
        if !needs_height_measure(app, i, is_last, active_turn_assistant, is_streaming) {
            stats.reused_msgs += 1;
            continue;
        }
        let measured_lines_before = stats.measured_lines;
        measure_message_height_at(app, base, active_turn_assistant, width, i, &mut stats);
        budget.consume(stats.measured_lines.saturating_sub(measured_lines_before));
    }

    app.viewport.finalize_remeasure_if_clean();
    stats
}

fn needs_height_measure(
    app: &App,
    idx: usize,
    is_last: bool,
    active_turn_assistant: Option<usize>,
    is_streaming: bool,
) -> bool {
    ((is_last || active_turn_assistant == Some(idx)) && is_streaming)
        || !app.viewport.message_height_is_current(idx)
}

#[allow(clippy::too_many_arguments)]
fn measure_message_height_at(
    app: &mut App,
    base: SpinnerState,
    active_turn_assistant: Option<usize>,
    width: u16,
    idx: usize,
    stats: &mut HeightUpdateStats,
) {
    let msg_count = app.messages.len();
    let is_last_message = idx + 1 == msg_count;
    let sp = msg_spinner(base, idx, active_turn_assistant, &app.messages[idx]);
    let (h, rendered_lines) = measure_message_height(
        &mut app.messages[idx],
        &sp,
        width,
        app.viewport.layout_generation,
        app.tools_collapsed,
        !is_last_message,
    );
    app.sync_render_cache_message(idx);
    stats.measured_msgs += 1;
    stats.measured_lines += rendered_lines;
    app.viewport.set_message_height(idx, h);
    app.viewport.mark_message_height_measured(idx);
}

/// Measure message height using ground truth: render the message into a scratch
/// buffer and call `Paragraph::line_count(width)`.
///
/// This uses the exact same code path as actual rendering (`render_message()`),
/// so heights can never diverge from what appears on screen. The scratch vec is
/// temporary and discarded after measurement. Block-level caches are still
/// populated as a side effect (via `render_text_cached` / `render_tool_call_cached`),
/// so completed blocks remain O(1) on subsequent calls.
fn measure_message_height(
    msg: &mut crate::app::ChatMessage,
    spinner: &SpinnerState,
    width: u16,
    layout_generation: u64,
    tools_collapsed: bool,
    include_trailing_separator: bool,
) -> (usize, usize) {
    let _t = crate::perf::start_with("chat::measure_msg", "blocks", msg.blocks.len());
    let (h, wrapped_lines) =
        message::measure_message_height_cached_with_tools_collapsed_and_separator(
            msg,
            spinner,
            width,
            layout_generation,
            tools_collapsed,
            include_trailing_separator,
        );
    crate::perf::mark_with("chat::measure_msg_wrapped_lines", "lines", wrapped_lines);
    (h, wrapped_lines)
}

fn build_base_spinner(app: &App) -> SpinnerState {
    let show_subagent_thinking = app.should_show_subagent_thinking(Instant::now());
    SpinnerState {
        frame: app.spinner_frame,
        is_active_turn_assistant: false,
        show_empty_thinking: matches!(app.status, AppStatus::Thinking | AppStatus::Running),
        show_thinking: matches!(app.status, AppStatus::Thinking),
        show_subagent_thinking,
        show_compacting: app.is_compacting,
    }
}

fn sync_chat_layout(app: &mut App, area: Rect, base_spinner: SpinnerState) -> usize {
    let width = area.width;
    let viewport_height = usize::from(area.height);

    {
        let _t = app.perf.as_ref().map(|p| p.start("chat::on_frame"));
        if app.viewport.on_frame(width, area.height).resized() {
            app.cache_metrics.record_resize();
        }
    }
    let height_stats = update_visual_heights(app, base_spinner, width, viewport_height);
    crate::perf::mark_with(
        "chat::update_heights_measured_msgs",
        "msgs",
        height_stats.measured_msgs,
    );
    crate::perf::mark_with("chat::update_heights_reused_msgs", "msgs", height_stats.reused_msgs);
    crate::perf::mark_with(
        "chat::update_heights_measured_lines",
        "lines",
        height_stats.measured_lines,
    );

    {
        let _t = app.perf.as_ref().map(|p| p.start("chat::prefix_sums"));
        app.viewport.rebuild_prefix_sums();
    }
    if let Some((anchor_idx, anchor_offset)) = app.viewport.ready_scroll_anchor_to_restore() {
        app.viewport.restore_scroll_anchor(anchor_idx, anchor_offset);
    }

    let content_height = app.viewport.total_message_height();
    crate::perf::mark_with("chat::content_height", "rows", content_height);
    crate::perf::mark_with("chat::viewport_height", "rows", viewport_height);
    crate::perf::mark_with(
        "chat::content_overflow_rows",
        "rows",
        content_height.saturating_sub(viewport_height),
    );
    content_height
}

#[allow(
    clippy::cast_possible_truncation,
    clippy::too_many_arguments,
    clippy::too_many_lines,
    clippy::cast_precision_loss,
    clippy::cast_sign_loss
)]
fn build_scrolled_render_data(
    app: &mut App,
    base: SpinnerState,
    width: u16,
    content_height: usize,
    viewport_height: usize,
) -> ScrolledRenderData {
    let vp = &mut app.viewport;
    let reduced_motion = app.config.prefers_reduced_motion_effective();
    let max_scroll = content_height.saturating_sub(viewport_height);
    if vp.auto_scroll {
        vp.scroll_target = max_scroll;
        // Auto-scroll should stay pinned to the latest content without easing lag.
        vp.scroll_pos = vp.scroll_target as f32;
    }
    vp.scroll_target = vp.scroll_target.min(max_scroll);

    if !vp.auto_scroll {
        let target = vp.scroll_target as f32;
        let delta = target - vp.scroll_pos;
        if reduced_motion || delta.abs() < 0.01 {
            vp.scroll_pos = target;
        } else {
            vp.scroll_pos += delta * 0.3;
        }
    }
    vp.scroll_offset = vp.scroll_pos.round() as usize;
    clamp_scroll_to_content(vp, max_scroll, reduced_motion);

    let scroll_offset = vp.scroll_offset;
    crate::perf::mark_with("chat::max_scroll", "rows", max_scroll);
    crate::perf::mark_with("chat::scroll_offset", "rows", scroll_offset);

    let mut all_lines = Vec::new();
    let stats = {
        let _t = app
            .perf
            .as_ref()
            .map(|p| p.start_with("chat::render_msgs", "msgs", app.messages.len()));
        render_culled_messages(app, base, width, scroll_offset, viewport_height, &mut all_lines)
    };
    crate::perf::mark_with("chat::render_scrolled_lines", "lines", all_lines.len());
    crate::perf::mark_with("chat::render_scrolled_msgs", "msgs", stats.rendered_msgs);
    crate::perf::mark_with("chat::render_scrolled_first_visible", "idx", stats.first_visible);
    crate::perf::mark_with("chat::render_scrolled_start", "idx", stats.render_start);

    let paragraph = {
        let _t = app
            .perf
            .as_ref()
            .map(|p| p.start_with("chat::paragraph_build", "lines", all_lines.len()));
        Paragraph::new(Text::from(all_lines)).wrap(Wrap { trim: false })
    };

    ScrolledRenderData { paragraph, stats, max_scroll, scroll_offset }
}

/// Long content: smooth scroll + viewport culling.
#[allow(
    clippy::cast_possible_truncation,
    clippy::too_many_arguments,
    clippy::too_many_lines,
    clippy::cast_precision_loss,
    clippy::cast_sign_loss
)]
fn render_scrolled(
    frame: &mut Frame,
    area: Rect,
    app: &mut App,
    base: SpinnerState,
    width: u16,
    content_height: usize,
    viewport_height: usize,
) {
    let _t = app.perf.as_ref().map(|p| p.start("chat::render_scrolled"));
    let render_data = build_scrolled_render_data(app, base, width, content_height, viewport_height);
    let pinned_to_bottom = render_data.scroll_offset == render_data.max_scroll;
    if tracing::enabled!(tracing::Level::DEBUG) {
        let last_message_idx = app.messages.len().checked_sub(1);
        let last_message_height = last_message_idx.map(|idx| app.viewport.message_height(idx));
        tracing::debug!(
            "RENDER_SCROLLED: auto_scroll={} pinned_to_bottom={} scroll_target={} scroll_pos={:.2} \
             scroll_offset={} max_scroll={} first_visible={} render_start={} local_scroll={} \
             rendered_msgs={} last_rendered_idx={:?} rendered_line_count={} last_message_idx={:?} \
             last_message_height={:?}",
            app.viewport.auto_scroll,
            pinned_to_bottom,
            app.viewport.scroll_target,
            app.viewport.scroll_pos,
            render_data.scroll_offset,
            render_data.max_scroll,
            render_data.stats.first_visible,
            render_data.stats.render_start,
            render_data.stats.local_scroll,
            render_data.stats.rendered_msgs,
            render_data.stats.last_rendered_idx,
            render_data.stats.rendered_line_count,
            last_message_idx,
            last_message_height,
        );
    }
    if tracing::enabled!(tracing::Level::TRACE) {
        let visible_preview = render_lines_from_paragraph(
            &render_data.paragraph,
            area,
            render_data.stats.local_scroll,
        );
        tracing::trace!(
            "RENDER_VISIBLE_PREVIEW: bottom_lines={:?}",
            preview_tail_lines(&visible_preview, 5),
        );
    }

    app.rendered_chat_area = area;
    if chat_selection_snapshot_needed(app.selection) {
        let _t = app.perf.as_ref().map(|p| p.start("chat::selection_capture"));
        app.rendered_chat_lines = render_lines_from_paragraph(
            &render_data.paragraph,
            area,
            render_data.stats.local_scroll,
        );
    }
    {
        let _t = app
            .perf
            .as_ref()
            .map(|p| p.start_with("chat::render_widget", "scroll", render_data.stats.local_scroll));
        frame.render_widget(
            render_data
                .paragraph
                .scroll((paragraph_scroll_offset(render_data.stats.local_scroll), 0)),
            area,
        );
    }
}

pub(super) fn refresh_selection_snapshot(app: &mut App) {
    if !chat_selection_snapshot_needed(app.selection) {
        return;
    }

    let area = app.rendered_chat_area;
    if area.width == 0 || area.height == 0 {
        return;
    }

    let base_spinner = build_base_spinner(app);
    let content_height = sync_chat_layout(app, area, base_spinner);
    let render_data = build_scrolled_render_data(
        app,
        base_spinner,
        area.width,
        content_height,
        usize::from(area.height),
    );
    app.rendered_chat_area = area;
    app.rendered_chat_lines =
        render_lines_from_paragraph(&render_data.paragraph, area, render_data.stats.local_scroll);
}

#[must_use]
fn chat_selection_snapshot_needed(selection: Option<SelectionState>) -> bool {
    selection.is_some_and(|selection| selection.kind == SelectionKind::Chat)
}

#[must_use]
fn paragraph_scroll_offset(scroll_offset: usize) -> u16 {
    u16::try_from(scroll_offset).unwrap_or_else(|_| {
        tracing::warn!(
            scroll_offset,
            max_scroll = u16::MAX,
            "chat paragraph scroll exceeds ratatui u16 boundary; clamping local paragraph scroll"
        );
        u16::MAX
    })
}

#[allow(clippy::cast_possible_truncation, clippy::cast_precision_loss, clippy::cast_sign_loss)]
fn clamp_scroll_to_content(
    viewport: &mut crate::app::ChatViewport,
    max_scroll: usize,
    reduced_motion: bool,
) {
    viewport.scroll_target = viewport.scroll_target.min(max_scroll);

    // Shrinks can leave the smoothed scroll position beyond new content end.
    // Ease it back toward the valid bound while keeping rendered offset clamped.
    let max_scroll_f = max_scroll as f32;
    if viewport.scroll_pos > max_scroll_f {
        if reduced_motion {
            viewport.scroll_pos = max_scroll_f;
        } else {
            let overshoot = viewport.scroll_pos - max_scroll_f;
            viewport.scroll_pos = max_scroll_f + overshoot * OVERSCROLL_CLAMP_EASE;
            if (viewport.scroll_pos - max_scroll_f).abs() < SCROLLBAR_EASE_EPSILON {
                viewport.scroll_pos = max_scroll_f;
            }
        }
    }

    viewport.scroll_offset = (viewport.scroll_pos.round() as usize).min(max_scroll);
    if viewport.scroll_offset >= max_scroll {
        viewport.auto_scroll = true;
    }
}

/// Compute overlay scrollbar geometry for a single-column track.
///
/// Returns None when content fits in the viewport.
#[allow(clippy::cast_precision_loss, clippy::cast_possible_truncation, clippy::cast_sign_loss)]
fn compute_scrollbar_geometry(
    content_height: usize,
    viewport_height: usize,
    scroll_pos: f32,
) -> Option<ScrollbarGeometry> {
    if viewport_height == 0 || content_height <= viewport_height {
        return None;
    }
    let max_scroll = content_height.saturating_sub(viewport_height) as f32;
    let thumb_size = viewport_height
        .saturating_mul(viewport_height)
        .checked_div(content_height)
        .unwrap_or(0)
        .max(SCROLLBAR_MIN_THUMB_HEIGHT)
        .min(viewport_height);
    let track_space = viewport_height.saturating_sub(thumb_size) as f32;
    let thumb_top = if max_scroll <= f32::EPSILON || track_space <= 0.0 {
        0
    } else {
        ((scroll_pos.clamp(0.0, max_scroll) / max_scroll) * track_space).round() as usize
    };
    Some(ScrollbarGeometry { thumb_top, thumb_size })
}

fn ease_value(current: &mut f32, target: f32, factor: f32) {
    let delta = target - *current;
    if delta.abs() < SCROLLBAR_EASE_EPSILON {
        *current = target;
    } else {
        *current += delta * factor;
    }
}

#[allow(clippy::cast_precision_loss, clippy::cast_possible_truncation, clippy::cast_sign_loss)]
fn smooth_scrollbar_geometry(
    viewport: &mut crate::app::ChatViewport,
    target: ScrollbarGeometry,
    viewport_height: usize,
    reduced_motion: bool,
) -> ScrollbarGeometry {
    let target_top = target.thumb_top as f32;
    let target_size = target.thumb_size as f32;

    if reduced_motion || viewport.scrollbar_thumb_size <= 0.0 {
        viewport.scrollbar_thumb_top = target_top;
        viewport.scrollbar_thumb_size = target_size;
    } else {
        ease_value(&mut viewport.scrollbar_thumb_top, target_top, SCROLLBAR_TOP_EASE);
        ease_value(&mut viewport.scrollbar_thumb_size, target_size, SCROLLBAR_SIZE_EASE);
    }

    let mut thumb_size = viewport.scrollbar_thumb_size.round() as usize;
    thumb_size = thumb_size.max(SCROLLBAR_MIN_THUMB_HEIGHT).min(viewport_height);
    let max_top = viewport_height.saturating_sub(thumb_size);
    let thumb_top = viewport.scrollbar_thumb_top.round().clamp(0.0, max_top as f32) as usize;

    ScrollbarGeometry { thumb_top, thumb_size }
}
#[allow(clippy::cast_possible_truncation)]
fn render_scrollbar_overlay(
    frame: &mut Frame,
    viewport: &mut crate::app::ChatViewport,
    reduced_motion: bool,
    area: Rect,
    content_height: usize,
    viewport_height: usize,
) {
    let Some(target) =
        compute_scrollbar_geometry(content_height, viewport_height, viewport.scroll_pos)
    else {
        viewport.scrollbar_thumb_top = 0.0;
        viewport.scrollbar_thumb_size = 0.0;
        return;
    };
    if area.width == 0 || area.height == 0 {
        return;
    }
    let geometry = smooth_scrollbar_geometry(viewport, target, viewport_height, reduced_motion);
    let rail_style = Style::default().add_modifier(Modifier::DIM);
    let thumb_style = Style::default().fg(theme::ROLE_ASSISTANT);
    let rail_x = area.right().saturating_sub(1);
    let buf = frame.buffer_mut();
    for row in 0..area.height as usize {
        let y = area.y.saturating_add(row as u16);
        if let Some(cell) = buf.cell_mut((rail_x, y)) {
            cell.set_symbol("\u{2595}");
            cell.set_style(rail_style);
        }
    }
    let thumb_top = geometry.thumb_top.min(area.height.saturating_sub(1) as usize);
    let thumb_end = thumb_top.saturating_add(geometry.thumb_size).min(area.height as usize);
    for row in thumb_top..thumb_end {
        let y = area.y.saturating_add(row as u16);
        if let Some(cell) = buf.cell_mut((rail_x, y)) {
            cell.set_symbol("\u{2590}");
            cell.set_style(thumb_style);
        }
    }
}
/// Render only the visible message range into out (viewport culling).
/// Returns the local scroll offset to pass to `Paragraph::scroll()`.
#[allow(clippy::cast_possible_truncation, clippy::too_many_arguments)]
fn render_culled_messages(
    app: &mut App,
    base: SpinnerState,
    width: u16,
    scroll: usize,
    viewport_height: usize,
    out: &mut Vec<Line<'static>>,
) -> CulledRenderStats {
    let msg_count = app.messages.len();
    let active_turn_assistant = app.active_turn_assistant_idx();

    // O(log n) binary search via prefix sums to find first visible message.
    let first_visible = app.viewport.find_first_visible(scroll);

    // Apply margin: render a few extra messages above/below for safety
    let render_start = first_visible.saturating_sub(CULLING_MARGIN);

    // O(1) cumulative height lookup via prefix sums
    let height_before_start = app.viewport.cumulative_height_before(render_start);

    // Render messages from render_start onward, stopping once the exact wrapped
    // height in the output buffer covers the viewport plus a small overscan.
    let mut structural_skip = scroll.saturating_sub(height_before_start);
    let rows_needed = structural_skip + viewport_height + CULLING_OVERSCAN_ROWS;
    crate::perf::mark_with("chat::cull_lines_needed", "lines", rows_needed);
    let mut rendered_msgs = 0usize;
    let mut local_scroll = 0usize;
    let mut rendered_rows = 0usize;
    let mut last_rendered_idx = None;
    for i in render_start..msg_count {
        let sp = msg_spinner(base, i, active_turn_assistant, &app.messages[i]);
        let before = out.len();
        let message_height = app.viewport.message_height(i);
        if structural_skip > 0 {
            let remaining_skip = message::render_message_from_offset_internal(
                &mut app.messages[i],
                &sp,
                width,
                app.viewport.layout_generation,
                message::MessageRenderOptions {
                    tools_collapsed: app.tools_collapsed,
                    include_trailing_separator: i + 1 != msg_count,
                },
                structural_skip,
                out,
            );
            let structural_rows_skipped = structural_skip.saturating_sub(remaining_skip);
            rendered_rows = rendered_rows
                .saturating_add(message_height.saturating_sub(structural_rows_skipped));
            local_scroll = remaining_skip;
            structural_skip = 0;
        } else {
            message::render_message_with_tools_collapsed_and_separator(
                &mut app.messages[i],
                &sp,
                width,
                app.tools_collapsed,
                i + 1 != msg_count,
                out,
            );
            rendered_rows = rendered_rows.saturating_add(message_height);
        }
        app.sync_render_cache_message(i);
        if out.len() > before {
            rendered_msgs += 1;
            last_rendered_idx = Some(i);
        }
        if rendered_rows > rows_needed {
            break;
        }
    }

    let stats = CulledRenderStats {
        local_scroll,
        first_visible,
        render_start,
        rendered_msgs,
        last_rendered_idx,
        rendered_line_count: out.len(),
    };
    if tracing::enabled!(tracing::Level::DEBUG) {
        tracing::debug!(
            "RENDER_CULLED: scroll={} viewport_height={} height_before_start={} lines_needed={} \
             first_visible={} render_start={} local_scroll={} rendered_msgs={} last_rendered_idx={:?} \
             rendered_line_count={}",
            scroll,
            viewport_height,
            height_before_start,
            rows_needed,
            stats.first_visible,
            stats.render_start,
            stats.local_scroll,
            stats.rendered_msgs,
            stats.last_rendered_idx,
            stats.rendered_line_count,
        );
    }
    stats
}

#[allow(clippy::cast_possible_truncation, clippy::cast_precision_loss, clippy::cast_sign_loss)]
pub fn render(frame: &mut Frame, area: Rect, app: &mut App) {
    let _t = app.perf.as_ref().map(|p| p.start("chat::render"));
    crate::perf::mark_with("chat::message_count", "msgs", app.messages.len());
    let width = area.width;
    let viewport_height = area.height as usize;
    let base_spinner = build_base_spinner(app);
    let content_height = sync_chat_layout(app, area, base_spinner);

    tracing::trace!(
        "RENDER: width={}, content_height={}, viewport_height={}, scroll_target={}, auto_scroll={}",
        width,
        content_height,
        viewport_height,
        app.viewport.scroll_target,
        app.viewport.auto_scroll
    );

    if content_height <= viewport_height {
        crate::perf::mark_with("chat::path_short", "active", 1);
    } else {
        crate::perf::mark_with("chat::path_scrolled", "active", 1);
    }

    render_scrolled(frame, area, app, base_spinner, width, content_height, viewport_height);

    if let Some(sel) = app.selection
        && sel.kind == SelectionKind::Chat
    {
        frame.render_widget(SelectionOverlay { selection: sel }, app.rendered_chat_area);
    }

    render_scrollbar_overlay(
        frame,
        &mut app.viewport,
        app.config.prefers_reduced_motion_effective(),
        area,
        content_height,
        viewport_height,
    );

    enforce_and_emit_cache_metrics(app);
}

#[allow(clippy::cast_precision_loss, clippy::cast_possible_truncation, clippy::cast_sign_loss)]
fn enforce_and_emit_cache_metrics(app: &mut App) {
    let budget_stats = app.enforce_render_cache_budget();
    crate::perf::mark_with("cache::bytes_before", "bytes", budget_stats.total_before_bytes);
    crate::perf::mark_with("cache::bytes_after", "bytes", budget_stats.total_after_bytes);
    crate::perf::mark_with("cache::protected_bytes", "bytes", budget_stats.protected_bytes);
    crate::perf::mark_with("cache::evicted_bytes", "bytes", budget_stats.evicted_bytes);
    crate::perf::mark_with("cache::evicted_blocks", "count", budget_stats.evicted_blocks);

    // -- Accumulate and conditionally log render cache metrics --
    let should_log =
        app.cache_metrics.record_render_enforcement(&budget_stats, &app.render_cache_budget);

    let render_utilization_pct = if app.render_cache_budget.max_bytes > 0 {
        (app.render_cache_budget.last_total_bytes as f32 / app.render_cache_budget.max_bytes as f32)
            * 100.0
    } else {
        0.0
    };
    let history_utilization_pct = if app.history_retention.max_bytes > 0 {
        (app.history_retention_stats.total_after_bytes as f32
            / app.history_retention.max_bytes as f32)
            * 100.0
    } else {
        0.0
    };

    if let Some(warn_kind) = app.cache_metrics.check_warn_condition(
        render_utilization_pct,
        history_utilization_pct,
        budget_stats.evicted_blocks,
    ) {
        cache_metrics::emit_cache_warning(&warn_kind);
    }

    if should_log {
        let entry_count = count_populated_cache_slots(&app.messages);
        let snap = cache_metrics::build_snapshot(
            &app.render_cache_budget,
            &app.history_retention_stats,
            app.history_retention,
            &app.cache_metrics,
            &app.viewport,
            entry_count,
            budget_stats.evicted_blocks,
            0,
            budget_stats.protected_bytes,
        );
        cache_metrics::emit_render_metrics(&snap);

        crate::perf::mark_with("cache::entry_count", "count", entry_count);
        #[allow(clippy::cast_possible_truncation, clippy::cast_sign_loss)]
        crate::perf::mark_with(
            "cache::utilization_pct_x10",
            "pct",
            (snap.render_utilization_pct * 10.0) as usize,
        );
        crate::perf::mark_with("cache::peak_bytes", "bytes", snap.render_peak_bytes);
    }
}

/// Count cache slots with non-zero cached bytes across all message blocks.
///
/// Only called on log cadence (~every 60 frames), not per-frame.
fn count_populated_cache_slots(messages: &[crate::app::ChatMessage]) -> usize {
    messages
        .iter()
        .flat_map(|m| m.blocks.iter())
        .filter(|block| match block {
            MessageBlock::Text(block) => block.cache.cached_bytes() > 0,
            MessageBlock::Welcome(w) => w.cache.cached_bytes() > 0,
            MessageBlock::ToolCall(tc) => tc.cache.cached_bytes() > 0,
        })
        .count()
}

struct SelectionOverlay {
    selection: SelectionState,
}

impl Widget for SelectionOverlay {
    #[allow(clippy::cast_possible_truncation)]
    fn render(self, area: Rect, buf: &mut Buffer) {
        let (start, end) =
            crate::app::normalize_selection(self.selection.start, self.selection.end);
        for row in start.row..=end.row {
            let y = area.y.saturating_add(row as u16);
            if y >= area.bottom() {
                break;
            }
            let row_start = if row == start.row { start.col } else { 0 };
            let row_end = if row == end.row { end.col } else { area.width as usize };
            for col in row_start..row_end {
                let x = area.x.saturating_add(col as u16);
                if x >= area.right() {
                    break;
                }
                if let Some(cell) = buf.cell_mut((x, y)) {
                    cell.set_style(cell.style().add_modifier(Modifier::REVERSED));
                }
            }
        }
    }
}

#[allow(clippy::cast_possible_truncation)]
fn render_lines_from_paragraph(
    paragraph: &Paragraph,
    area: Rect,
    scroll_offset: usize,
) -> Vec<String> {
    let mut buf = Buffer::empty(area);
    let widget = paragraph.clone().scroll((paragraph_scroll_offset(scroll_offset), 0));
    widget.render(area, &mut buf);
    let mut lines = Vec::with_capacity(area.height as usize);
    for y in 0..area.height {
        let mut line = String::new();
        for x in 0..area.width {
            if let Some(cell) = buf.cell((area.x + x, area.y + y)) {
                line.push_str(cell.symbol());
            }
        }
        lines.push(line.trim_end().to_owned());
    }
    lines
}

fn preview_tail_lines(lines: &[String], count: usize) -> Vec<String> {
    lines
        .iter()
        .rev()
        .filter(|line| !line.is_empty())
        .take(count)
        .cloned()
        .collect::<Vec<_>>()
        .into_iter()
        .rev()
        .collect()
}

#[cfg(test)]
mod tests {
    use super::{
        SCROLLBAR_MIN_THUMB_HEIGHT, ScrollbarGeometry, clamp_scroll_to_content,
        compute_scrollbar_geometry, paragraph_scroll_offset, render_culled_messages,
        render_lines_from_paragraph, render_scrolled, smooth_scrollbar_geometry,
        update_visual_heights,
    };
    use crate::app::{
        App, AppStatus, ChatMessage, ChatViewport, InvalidationLevel, MessageBlock, MessageRole,
        SelectionKind, SelectionPoint, SelectionState, SystemSeverity, TextBlock,
    };
    use crate::ui::message::{self, SpinnerState};
    use ratatui::Terminal;
    use ratatui::backend::TestBackend;
    use ratatui::layout::Rect;
    use ratatui::text::Text;
    use ratatui::widgets::{Paragraph, Wrap};

    fn assistant_text_message(text: &str) -> ChatMessage {
        ChatMessage {
            role: MessageRole::Assistant,
            blocks: vec![MessageBlock::Text(TextBlock::from_complete(text))],
            usage: None,
        }
    }

    fn user_message(text: &str) -> ChatMessage {
        ChatMessage {
            role: MessageRole::User,
            blocks: vec![MessageBlock::Text(TextBlock::from_complete(text))],
            usage: None,
        }
    }

    fn system_message(text: &str) -> ChatMessage {
        ChatMessage {
            role: MessageRole::System(Some(SystemSeverity::Info)),
            blocks: vec![MessageBlock::Text(TextBlock::from_complete(text))],
            usage: None,
        }
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

    fn render_selected_chat_snapshot(app: &mut App, width: u16, height: u16) {
        let backend = TestBackend::new(width, height);
        let mut terminal = Terminal::new(backend).expect("terminal");
        terminal
            .draw(|frame| {
                let spinner = idle_spinner();
                let _ = app.viewport.on_frame(width, height);
                update_visual_heights(app, spinner, width, usize::from(height));
                app.viewport.rebuild_prefix_sums();
                render_scrolled(
                    frame,
                    Rect::new(0, 0, width, height),
                    app,
                    spinner,
                    width,
                    app.viewport.total_message_height(),
                    usize::from(height),
                );
            })
            .expect("draw");
    }

    #[test]
    fn scrollbar_hidden_when_content_fits() {
        assert_eq!(compute_scrollbar_geometry(10, 10, 0.0), None);
        assert_eq!(compute_scrollbar_geometry(8, 10, 0.0), None);
    }
    #[test]
    fn scrollbar_thumb_positions_are_stable() {
        assert_eq!(
            compute_scrollbar_geometry(50, 10, 0.0),
            Some(ScrollbarGeometry { thumb_top: 0, thumb_size: 2 })
        );
        assert_eq!(
            compute_scrollbar_geometry(50, 10, 20.0),
            Some(ScrollbarGeometry { thumb_top: 4, thumb_size: 2 })
        );
        assert_eq!(
            compute_scrollbar_geometry(50, 10, 40.0),
            Some(ScrollbarGeometry { thumb_top: 8, thumb_size: 2 })
        );
    }
    #[test]
    fn scrollbar_scroll_offset_is_clamped() {
        assert_eq!(
            compute_scrollbar_geometry(50, 10, 999.0),
            Some(ScrollbarGeometry { thumb_top: 8, thumb_size: 2 })
        );
    }
    #[test]
    fn scrollbar_handles_small_overflow() {
        assert_eq!(
            compute_scrollbar_geometry(11, 10, 1.0),
            Some(ScrollbarGeometry { thumb_top: 1, thumb_size: 9 })
        );
    }
    #[test]
    fn scrollbar_respects_min_thumb_height() {
        assert_eq!(
            compute_scrollbar_geometry(10_000, 10, 0.0),
            Some(ScrollbarGeometry { thumb_top: 0, thumb_size: SCROLLBAR_MIN_THUMB_HEIGHT })
        );
    }

    #[test]
    fn update_visual_heights_remeasures_dirty_non_tail_message() {
        let mut app = App::test_default();
        app.status = AppStatus::Ready;
        app.messages =
            vec![assistant_text_message("short"), assistant_text_message("tail stays unchanged")];

        let _ = app.viewport.on_frame(12, 8);
        let spinner = idle_spinner();

        update_visual_heights(&mut app, spinner, 12, 8);
        let base_h = app.viewport.message_height(0);
        assert!(base_h > 0);

        if let Some(MessageBlock::Text(block)) =
            app.messages.get_mut(0).and_then(|m| m.blocks.get_mut(0))
        {
            let extra = " this now wraps across multiple lines";
            block.text.push_str(extra);
            block.markdown.append(extra);
            block.cache.invalidate();
        }
        app.invalidate_layout(InvalidationLevel::MessagesFrom(0));

        update_visual_heights(&mut app, spinner, 12, 8);
        assert!(
            app.viewport.message_height(0) > base_h,
            "dirty non-tail message should be remeasured"
        );
    }

    #[test]
    fn last_message_height_omits_trailing_separator() {
        let mut app = App::test_default();
        app.status = AppStatus::Ready;
        app.messages = vec![assistant_text_message("hello")];

        let _ = app.viewport.on_frame(40, 8);
        let spinner = idle_spinner();

        update_visual_heights(&mut app, spinner, 40, 8);
        app.viewport.rebuild_prefix_sums();

        assert_eq!(app.viewport.message_height(0), 2);
        assert_eq!(app.viewport.total_message_height(), 2);
    }

    #[test]
    fn active_turn_assistant_owns_thinking_when_system_message_trails() {
        let mut app = App::test_default();
        app.status = AppStatus::Thinking;
        app.messages = vec![
            assistant_text_message("older reply"),
            user_message("next prompt"),
            ChatMessage { role: MessageRole::Assistant, blocks: Vec::new(), usage: None },
            system_message("rate limit warning"),
        ];
        app.bind_active_turn_assistant(2);

        assert_eq!(app.active_turn_assistant_idx(), Some(2));

        let _ = app.viewport.on_frame(40, 8);
        let spinner = SpinnerState { show_empty_thinking: true, ..idle_spinner() };

        update_visual_heights(&mut app, spinner, 40, 8);
        app.viewport.rebuild_prefix_sums();

        assert_eq!(
            app.viewport.message_height(2),
            3,
            "active assistant should render label + thinking + separator even when a system row trails"
        );
    }

    #[test]
    fn active_turn_assistant_uses_explicit_owner_without_user_anchor() {
        let mut app = App::test_default();
        app.status = AppStatus::Thinking;
        app.messages = vec![
            assistant_text_message("older reply"),
            ChatMessage { role: MessageRole::Assistant, blocks: Vec::new(), usage: None },
            system_message("status"),
        ];
        app.bind_active_turn_assistant(1);

        assert_eq!(app.active_turn_assistant_idx(), Some(1));
    }

    #[test]
    fn appending_message_remeasures_previous_tail_separator() {
        let mut app = App::test_default();
        app.status = AppStatus::Ready;
        app.push_message_tracked(assistant_text_message("first reply"));

        let _ = app.viewport.on_frame(40, 8);
        let spinner = idle_spinner();

        update_visual_heights(&mut app, spinner, 40, 8);
        app.viewport.rebuild_prefix_sums();
        assert_eq!(app.viewport.message_height(0), 2);
        assert_eq!(app.viewport.total_message_height(), 2);

        app.push_message_tracked(user_message("follow-up"));

        update_visual_heights(&mut app, spinner, 40, 8);
        app.viewport.rebuild_prefix_sums();
        assert_eq!(app.viewport.message_height(0), 3);
        assert_eq!(app.viewport.message_height(1), 2);
        assert_eq!(app.viewport.total_message_height(), 5);
    }

    #[test]
    fn removing_tail_message_remeasures_new_last_separator() {
        let mut app = App::test_default();
        app.status = AppStatus::Ready;
        app.push_message_tracked(assistant_text_message("first reply"));
        app.push_message_tracked(user_message("follow-up"));

        let _ = app.viewport.on_frame(40, 8);
        let spinner = idle_spinner();

        update_visual_heights(&mut app, spinner, 40, 8);
        app.viewport.rebuild_prefix_sums();
        assert_eq!(app.viewport.message_height(0), 3);
        assert_eq!(app.viewport.message_height(1), 2);

        let removed = app.remove_message_tracked(1);
        assert!(removed.is_some());

        update_visual_heights(&mut app, spinner, 40, 8);
        app.viewport.rebuild_prefix_sums();
        assert_eq!(app.viewport.message_height(0), 2);
        assert_eq!(app.viewport.total_message_height(), 2);
    }

    #[allow(clippy::cast_precision_loss)]
    #[test]
    fn resize_remeasure_updates_visible_window_before_far_messages() {
        let mut app = App::test_default();
        let text = "This message should wrap after resize and stay expensive enough to measure. "
            .repeat(6);
        app.messages = (0..32).map(|_| assistant_text_message(&text)).collect();

        let spinner = idle_spinner();

        let _ = app.viewport.on_frame(48, 12);
        update_visual_heights(&mut app, spinner, 48, 12);
        app.viewport.rebuild_prefix_sums();
        let per_message_height = app.viewport.message_height(0);
        assert!(per_message_height > 0);

        let visible_rows = per_message_height * 2;
        app.viewport.scroll_offset = per_message_height * 15;
        app.viewport.scroll_target = app.viewport.scroll_offset;
        app.viewport.scroll_pos = app.viewport.scroll_offset as f32;

        assert!(app.viewport.on_frame(18, 12).width_changed);
        update_visual_heights(&mut app, spinner, 18, visible_rows);

        assert_eq!(app.viewport.message_heights_width, 0);
        assert!(app.viewport.resize_remeasure_active());
        assert!(app.viewport.message_height_is_current(15));
        assert!(app.viewport.message_height_is_current(16));
        assert!(!app.viewport.message_height_is_current(31));
    }

    #[allow(clippy::cast_precision_loss)]
    #[test]
    fn resize_remeasure_converges_over_multiple_frames() {
        let mut app = App::test_default();
        let text = "This message should wrap after resize and stay expensive enough to measure. "
            .repeat(6);
        app.messages = (0..40).map(|_| assistant_text_message(&text)).collect();

        let spinner = idle_spinner();

        let _ = app.viewport.on_frame(48, 12);
        update_visual_heights(&mut app, spinner, 48, 12);
        app.viewport.rebuild_prefix_sums();
        let per_message_height = app.viewport.message_height(0);
        app.viewport.scroll_offset = per_message_height * 12;
        app.viewport.scroll_target = app.viewport.scroll_offset;
        app.viewport.scroll_pos = app.viewport.scroll_offset as f32;

        assert!(app.viewport.on_frame(18, 12).width_changed);
        for _ in 0..8 {
            update_visual_heights(&mut app, spinner, 18, per_message_height * 2);
            app.viewport.rebuild_prefix_sums();
            if !app.viewport.resize_remeasure_active() {
                break;
            }
        }

        assert_eq!(app.viewport.message_heights_width, 18);
        assert!(!app.viewport.resize_remeasure_active());
        assert!(app.viewport.message_height_is_current(0));
        assert!(app.viewport.message_height_is_current(39));
    }

    #[allow(clippy::cast_precision_loss)]
    #[test]
    fn resize_remeasure_does_not_repeat_dirty_suffix_after_measuring_it() {
        let mut app = App::test_default();
        let text = "This message should wrap after resize and stay expensive enough to measure. "
            .repeat(6);
        app.messages = (0..8).map(|_| assistant_text_message(&text)).collect();

        let spinner = idle_spinner();

        let _ = app.viewport.on_frame(48, 12);
        update_visual_heights(&mut app, spinner, 48, 12);
        app.viewport.rebuild_prefix_sums();
        let per_message_height = app.viewport.message_height(0);
        app.viewport.scroll_offset = per_message_height * 2;
        app.viewport.scroll_target = app.viewport.scroll_offset;
        app.viewport.scroll_pos = app.viewport.scroll_offset as f32;

        assert!(app.viewport.on_frame(18, 12).width_changed);
        app.invalidate_layout(InvalidationLevel::MessagesFrom(0));

        let first = update_visual_heights(&mut app, spinner, 18, per_message_height * 2);
        app.viewport.rebuild_prefix_sums();
        let second = update_visual_heights(&mut app, spinner, 18, per_message_height * 2);

        assert!(first.measured_msgs >= app.messages.len());
        assert_eq!(second.measured_msgs, 0);
        assert_eq!(app.viewport.message_heights_width, 18);
    }

    #[test]
    fn render_culled_messages_matches_full_render_when_scrolled_inside_message() {
        let mut app = App::test_default();
        let text = (0..160).map(|i| format!("line {i:03}")).collect::<Vec<_>>().join("\n");
        app.messages = vec![assistant_text_message(&text)];
        let width = 24u16;
        let viewport_height_u16 = 8u16;
        let viewport_height = usize::from(viewport_height_u16);
        let area = Rect::new(0, 0, width, viewport_height_u16);
        let spinner = idle_spinner();

        let _ = app.viewport.on_frame(width, viewport_height_u16);
        update_visual_heights(&mut app, spinner, width, viewport_height);
        app.viewport.rebuild_prefix_sums();

        let scroll = 60;
        let mut full_lines = Vec::new();
        message::render_message_with_tools_collapsed_and_separator(
            &mut app.messages[0],
            &spinner,
            width,
            app.tools_collapsed,
            false,
            &mut full_lines,
        );
        let full_preview = render_lines_from_paragraph(
            &Paragraph::new(Text::from(full_lines.clone())).wrap(Wrap { trim: false }),
            area,
            scroll,
        );

        let mut culled_lines = Vec::new();
        let stats = render_culled_messages(
            &mut app,
            spinner,
            width,
            scroll,
            viewport_height,
            &mut culled_lines,
        );
        let culled_preview = render_lines_from_paragraph(
            &Paragraph::new(Text::from(culled_lines.clone())).wrap(Wrap { trim: false }),
            area,
            stats.local_scroll,
        );

        assert_eq!(culled_preview, full_preview);
        assert!(culled_lines.len() < full_lines.len());
        assert_eq!(stats.rendered_msgs, 1);
    }

    #[test]
    fn render_culled_messages_matches_full_render_when_scrolled_inside_wrapped_role_label() {
        let mut app = App::test_default();
        app.messages = vec![user_message("ok")];
        let width = 2u16;
        let viewport_height_u16 = 4u16;
        let viewport_height = usize::from(viewport_height_u16);
        let area = Rect::new(0, 0, width, viewport_height_u16);
        let spinner = idle_spinner();

        let _ = app.viewport.on_frame(width, viewport_height_u16);
        update_visual_heights(&mut app, spinner, width, viewport_height);
        app.viewport.rebuild_prefix_sums();

        assert!(app.viewport.message_height(0) >= 3);

        let scroll = 1;
        let mut full_lines = Vec::new();
        message::render_message_with_tools_collapsed_and_separator(
            &mut app.messages[0],
            &spinner,
            width,
            app.tools_collapsed,
            false,
            &mut full_lines,
        );
        let full_preview = render_lines_from_paragraph(
            &Paragraph::new(Text::from(full_lines.clone())).wrap(Wrap { trim: false }),
            area,
            scroll,
        );

        let mut culled_lines = Vec::new();
        let stats = render_culled_messages(
            &mut app,
            spinner,
            width,
            scroll,
            viewport_height,
            &mut culled_lines,
        );
        let culled_preview = render_lines_from_paragraph(
            &Paragraph::new(Text::from(culled_lines.clone())).wrap(Wrap { trim: false }),
            area,
            stats.local_scroll,
        );

        assert_eq!(culled_preview, full_preview);
        assert_eq!(stats.rendered_msgs, 1);
        assert_eq!(stats.local_scroll, 1);
    }

    #[test]
    fn render_culled_messages_stops_after_first_wrapped_message_when_viewport_is_covered() {
        let mut app = App::test_default();
        let huge_wrapped = "wrap ".repeat(2_000);
        app.messages = vec![
            assistant_text_message(&huge_wrapped),
            assistant_text_message("this should remain offscreen"),
        ];
        let width = 20u16;
        let viewport_height_u16 = 8u16;
        let viewport_height = usize::from(viewport_height_u16);
        let spinner = idle_spinner();

        let _ = app.viewport.on_frame(width, viewport_height_u16);
        update_visual_heights(&mut app, spinner, width, viewport_height);
        app.viewport.rebuild_prefix_sums();

        assert!(app.viewport.message_height(0) > 200);

        let mut culled_lines = Vec::new();
        let stats = render_culled_messages(
            &mut app,
            spinner,
            width,
            40,
            viewport_height,
            &mut culled_lines,
        );

        assert_eq!(stats.rendered_msgs, 1);
        assert_eq!(stats.last_rendered_idx, Some(0));
    }

    #[test]
    fn paragraph_scroll_offset_clamps_large_local_scroll_explicitly() {
        assert_eq!(paragraph_scroll_offset(42), 42);
        assert_eq!(paragraph_scroll_offset(usize::from(u16::MAX) + 123), u16::MAX);
    }

    #[test]
    fn chat_selection_snapshot_refreshes_without_dragging_after_streaming_change() {
        let mut app = App::test_default();
        app.status = AppStatus::Running;
        app.messages = vec![assistant_text_message("hello")];
        app.bind_active_turn_assistant(0);
        app.selection = Some(SelectionState {
            kind: SelectionKind::Chat,
            start: SelectionPoint { row: 0, col: 0 },
            end: SelectionPoint { row: 0, col: 5 },
            dragging: false,
        });

        render_selected_chat_snapshot(&mut app, 20, 6);
        let first_snapshot = app.rendered_chat_lines.clone();
        assert!(!first_snapshot.is_empty());

        if let Some(MessageBlock::Text(block)) =
            app.messages.get_mut(0).and_then(|message| message.blocks.get_mut(0))
        {
            block.text.push_str("\nworld");
            block.markdown.append("\nworld");
            block.cache.invalidate();
        }
        app.invalidate_layout(InvalidationLevel::MessageChanged(0));

        render_selected_chat_snapshot(&mut app, 20, 6);

        assert_ne!(app.rendered_chat_lines, first_snapshot);
        assert!(app.rendered_chat_lines.iter().any(|line| line.contains("world")));
    }

    #[test]
    fn clamp_scroll_to_content_snaps_overscroll_after_shrink() {
        let mut viewport = ChatViewport::new();
        viewport.auto_scroll = false;
        viewport.scroll_target = 120;
        viewport.scroll_pos = 120.0;
        viewport.scroll_offset = 120;

        clamp_scroll_to_content(&mut viewport, 40, false);

        assert!(viewport.auto_scroll);
        assert_eq!(viewport.scroll_target, 40);
        assert!(viewport.scroll_pos > 40.0);
        assert!(viewport.scroll_pos < 120.0);
        assert_eq!(viewport.scroll_offset, 40);
    }

    #[test]
    fn clamp_scroll_to_content_preserves_in_range_scroll() {
        let mut viewport = ChatViewport::new();
        viewport.auto_scroll = false;
        viewport.scroll_target = 20;
        viewport.scroll_pos = 20.0;
        viewport.scroll_offset = 20;

        clamp_scroll_to_content(&mut viewport, 40, false);

        assert!(!viewport.auto_scroll);
        assert_eq!(viewport.scroll_target, 20);
        assert!((viewport.scroll_pos - 20.0).abs() < f32::EPSILON);
        assert_eq!(viewport.scroll_offset, 20);
    }

    #[test]
    fn clamp_scroll_to_content_settles_to_max_over_frames() {
        let mut viewport = ChatViewport::new();
        viewport.auto_scroll = false;
        viewport.scroll_target = 120;
        viewport.scroll_pos = 120.0;
        viewport.scroll_offset = 120;

        for _ in 0..12 {
            clamp_scroll_to_content(&mut viewport, 40, false);
        }

        assert_eq!(viewport.scroll_target, 40);
        assert_eq!(viewport.scroll_offset, 40);
        assert!(viewport.scroll_pos >= 40.0);
        assert!(viewport.scroll_pos < 40.1);
    }

    #[test]
    fn clamp_scroll_to_content_snaps_overscroll_when_reduced_motion_enabled() {
        let mut viewport = ChatViewport::new();
        viewport.auto_scroll = false;
        viewport.scroll_target = 120;
        viewport.scroll_pos = 120.0;
        viewport.scroll_offset = 120;

        clamp_scroll_to_content(&mut viewport, 40, true);

        assert!(viewport.auto_scroll);
        assert_eq!(viewport.scroll_target, 40);
        assert!((viewport.scroll_pos - 40.0).abs() < f32::EPSILON);
        assert_eq!(viewport.scroll_offset, 40);
    }

    #[test]
    fn smooth_scrollbar_geometry_snaps_when_reduced_motion_enabled() {
        let mut viewport = ChatViewport::new();
        viewport.scrollbar_thumb_top = 2.0;
        viewport.scrollbar_thumb_size = 3.0;

        let geometry = smooth_scrollbar_geometry(
            &mut viewport,
            ScrollbarGeometry { thumb_top: 9, thumb_size: 5 },
            20,
            true,
        );

        assert_eq!(geometry, ScrollbarGeometry { thumb_top: 9, thumb_size: 5 });
        assert!((viewport.scrollbar_thumb_top - 9.0).abs() < f32::EPSILON);
        assert!((viewport.scrollbar_thumb_size - 5.0).abs() < f32::EPSILON);
    }
}
