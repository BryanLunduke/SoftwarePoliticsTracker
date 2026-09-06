// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

/// Describes the intent behind a layout invalidation.
///
/// The viewport now tracks per-message staleness, prefix-sum dirtiness, and
/// queued remeasurement separately. Do not add a bounded range variant unless
/// the underlying state can represent disjoint stale spans directly.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LayoutInvalidation {
    /// One message's content changed (tool status, permission UI, terminal output).
    /// The changed message must be remeasured exactly on the next frame.
    MessageChanged(usize),
    /// Messages from `start` onward may have changed structurally (insert/remove/reindex).
    MessagesFrom(usize),
    /// Terminal geometry changed. Handled internally by `on_frame()`.
    /// Included for completeness; not dispatched through `App::invalidate_layout()`.
    Resize,
    /// Global layout change (for example, tool collapse toggle).
    /// All messages become stale and `layout_generation` is bumped.
    Global,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LayoutRemeasureReason {
    MessageChanged,
    MessagesFrom,
    Resize,
    Global,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct FrameGeometryChange {
    pub width_changed: bool,
    pub height_changed: bool,
}

impl FrameGeometryChange {
    #[must_use]
    pub fn resized(self) -> bool {
        self.width_changed || self.height_changed
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
struct PreservedScrollAnchor {
    reason: LayoutRemeasureReason,
    index: usize,
    offset: usize,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct LayoutRemeasurePlan {
    reason: LayoutRemeasureReason,
    scroll_anchor_index: usize,
    scroll_anchor_offset: usize,
    preserved_scroll_anchor: Option<PreservedScrollAnchor>,
    priority_start: usize,
    priority_end: usize,
    next_above: Option<usize>,
    next_below: usize,
    prefer_above: bool,
}

impl LayoutRemeasurePlan {
    fn new(
        reason: LayoutRemeasureReason,
        scroll_anchor_index: usize,
        scroll_anchor_offset: usize,
        preserved_scroll_anchor: Option<PreservedScrollAnchor>,
        priority_start: usize,
        priority_end: usize,
        message_count: usize,
    ) -> Self {
        let last_idx = message_count.saturating_sub(1);
        let scroll_anchor_index = scroll_anchor_index.min(last_idx);
        let priority_start = priority_start.min(last_idx);
        let priority_end = priority_end.min(last_idx).max(priority_start);
        let preserved_scroll_anchor = preserved_scroll_anchor.map(|anchor| PreservedScrollAnchor {
            reason: anchor.reason,
            index: anchor.index.min(last_idx),
            offset: anchor.offset,
        });
        Self {
            reason,
            scroll_anchor_index,
            scroll_anchor_offset,
            preserved_scroll_anchor,
            priority_start,
            priority_end,
            next_above: priority_start.checked_sub(1),
            next_below: priority_end.saturating_add(1).min(message_count),
            prefer_above: false,
        }
    }

    fn from_scroll_anchor(
        reason: LayoutRemeasureReason,
        scroll_anchor_index: usize,
        scroll_anchor_offset: usize,
        preserved_scroll_anchor: Option<PreservedScrollAnchor>,
        message_count: usize,
    ) -> Self {
        Self::new(
            reason,
            scroll_anchor_index,
            scroll_anchor_offset,
            preserved_scroll_anchor,
            scroll_anchor_index,
            scroll_anchor_index,
            message_count,
        )
    }
}

/// Single owner of all chat layout state: scroll, per-message heights, and prefix sums.
///
/// Consolidates state previously scattered across `App` (scroll fields, prefix sums),
/// `ChatMessage` (`cached_visual_height`/`cached_visual_width`), and `BlockCache`
/// (`wrapped_height`/`wrapped_width`). Per-block heights remain on `BlockCache`
/// via `set_height()` / `height_at()`, but the viewport owns the validity width
/// that governs whether those caches are considered current.
pub struct ChatViewport {
    // --- Scroll ---
    /// Rendered scroll offset (rounded from `scroll_pos`).
    pub scroll_offset: usize,
    /// Target scroll offset requested by user input or auto-scroll.
    pub scroll_target: usize,
    /// Smooth scroll position (fractional) for animation.
    pub scroll_pos: f32,
    /// Smoothed scrollbar thumb top row (fractional) for animation.
    pub scrollbar_thumb_top: f32,
    /// Smoothed scrollbar thumb height (fractional) for animation.
    pub scrollbar_thumb_size: f32,
    /// Whether to auto-scroll to bottom on new content.
    pub auto_scroll: bool,

    // --- Layout ---
    /// Current terminal width. Set by `on_frame()` each render cycle.
    pub width: u16,
    /// Current terminal height. Set by `on_frame()` each render cycle.
    pub height: u16,
    /// Monotonic layout generation for width/global layout-affecting changes.
    /// Tool-call measurement cache keys include this to avoid stale heights.
    pub layout_generation: u64,

    // --- Per-message heights ---
    /// Visual height (in terminal rows) of each message, indexed by message position.
    /// Retained across resize and large invalidations as a temporary estimate until
    /// the queued remeasurement converges back to exact heights.
    pub message_heights: Vec<usize>,
    /// Width at which `message_heights` was last known to be exact for every message.
    pub message_heights_width: u16,
    /// Per-message exactness marker used while queued remeasurement is in flight.
    pub measured_message_widths: Vec<u16>,
    /// Per-message stale marker. `true` means the message must be remeasured.
    pub stale_message_heights: Vec<bool>,
    /// Messages that must be remeasured exactly before the general queued plan continues.
    priority_remeasure: Vec<usize>,
    /// Visible-first queued remeasurement plan.
    pub remeasure_plan: Option<LayoutRemeasurePlan>,

    // --- Prefix sums ---
    /// Cumulative heights: `height_prefix_sums[i]` = sum of heights `0..=i`.
    /// Enables O(log n) binary search for first visible message and O(1) total height.
    pub height_prefix_sums: Vec<usize>,
    /// Width at which prefix sums were last rebuilt against `message_heights`.
    pub prefix_sums_width: u16,
    /// Oldest prefix index whose cumulative values must be rebuilt.
    pub prefix_dirty_from: Option<usize>,
}

impl ChatViewport {
    /// Create a new viewport with default scroll state (auto-scroll enabled).
    #[must_use]
    pub fn new() -> Self {
        Self {
            scroll_offset: 0,
            scroll_target: 0,
            scroll_pos: 0.0,
            scrollbar_thumb_top: 0.0,
            scrollbar_thumb_size: 0.0,
            auto_scroll: true,
            width: 0,
            height: 0,
            layout_generation: 1,
            message_heights: Vec::new(),
            message_heights_width: 0,
            measured_message_widths: Vec::new(),
            stale_message_heights: Vec::new(),
            priority_remeasure: Vec::new(),
            remeasure_plan: None,
            height_prefix_sums: Vec::new(),
            prefix_sums_width: 0,
            prefix_dirty_from: None,
        }
    }

    /// Called at top of each render frame.
    ///
    /// Width and height are tracked through the same entry point, but only width
    /// changes invalidate message layout. Height changes are viewport-only.
    #[must_use]
    pub fn on_frame(&mut self, width: u16, height: u16) -> FrameGeometryChange {
        let change = FrameGeometryChange {
            width_changed: self.width != 0 && self.width != width,
            height_changed: self.height != 0 && self.height != height,
        };
        if change.resized() {
            tracing::debug!(
                "RESIZE: width {} -> {}, height {} -> {}, scroll_target={}, auto_scroll={}",
                self.width,
                width,
                self.height,
                height,
                self.scroll_target,
                self.auto_scroll
            );
        }
        if change.width_changed {
            self.handle_width_resize();
        }
        self.width = width;
        self.height = height;
        change
    }

    /// Invalidate message layout caches when terminal width changes.
    ///
    /// Old message heights remain as approximations so the next frame can keep
    /// using a stable estimated prefix-sum model while queued remeasurement converges.
    fn handle_width_resize(&mut self) {
        if self.message_heights.is_empty() {
            self.message_heights_width = 0;
            self.prefix_sums_width = 0;
            self.prefix_dirty_from = None;
            self.remeasure_plan = None;
            self.priority_remeasure.clear();
            self.layout_generation = self.layout_generation.wrapping_add(1);
            return;
        }

        self.message_heights_width = 0;
        self.measured_message_widths.fill(0);
        self.stale_message_heights.fill(true);
        self.priority_remeasure.clear();
        self.mark_prefix_sums_dirty_from(0);
        self.schedule_remeasure(LayoutRemeasureReason::Resize);
        self.layout_generation = self.layout_generation.wrapping_add(1);
    }

    /// Bump layout generation for non-width global layout-affecting changes.
    pub fn bump_layout_generation(&mut self) {
        self.layout_generation = self.layout_generation.wrapping_add(1);
    }

    // --- Per-message height ---

    /// Get the cached visual height for message `idx`. Returns 0 if not yet computed.
    #[must_use]
    pub fn message_height(&self, idx: usize) -> usize {
        self.message_heights.get(idx).copied().unwrap_or(0)
    }

    /// Return the number of stale messages awaiting remeasurement.
    #[must_use]
    pub fn stale_message_count(&self) -> usize {
        self.stale_message_heights.iter().filter(|stale| **stale).count()
    }

    /// Return the oldest stale message index, if any.
    #[must_use]
    pub fn oldest_stale_index(&self) -> Option<usize> {
        self.stale_message_heights.iter().position(|stale| *stale)
    }

    /// Return the oldest prefix index whose cumulative values still need repair.
    #[must_use]
    pub fn prefix_dirty_from(&self) -> Option<usize> {
        self.prefix_dirty_from
    }

    /// Return the active queued remeasurement reason, if any.
    #[must_use]
    pub fn remeasure_reason(&self) -> Option<LayoutRemeasureReason> {
        self.remeasure_plan.map(|plan| plan.reason)
    }

    /// Ensure per-message height state matches the current message count.
    pub fn sync_message_count(&mut self, count: usize) {
        let old_len = self.message_heights.len();
        if old_len != count {
            self.message_heights.resize(count, 0);
            self.measured_message_widths.resize(count, 0);
            self.stale_message_heights.resize(count, false);
            self.height_prefix_sums.resize(count, 0);
            self.prefix_sums_width = 0;
            self.message_heights_width = 0;

            if old_len < count {
                self.stale_message_heights[old_len..].fill(true);
                self.measured_message_widths[old_len..].fill(0);
                self.mark_prefix_sums_dirty_from(old_len);
                self.schedule_remeasure(LayoutRemeasureReason::MessagesFrom);
                self.queue_priority_remeasure(count.saturating_sub(1));
            } else {
                self.priority_remeasure.retain(|&idx| idx < count);
                self.prefix_dirty_from = self.prefix_dirty_from.map(|idx| idx.min(count));
            }
        }

        if count == 0 {
            self.priority_remeasure.clear();
            self.remeasure_plan = None;
            self.prefix_dirty_from = None;
            self.height_prefix_sums.clear();
            return;
        }

        if let Some(plan) = self.remeasure_plan
            && (plan.scroll_anchor_index >= count
                || plan.preserved_scroll_anchor.is_some_and(|anchor| anchor.index >= count)
                || plan.priority_start >= count
                || plan.priority_end >= count
                || plan.next_below > count)
        {
            self.remeasure_plan = Some(LayoutRemeasurePlan::new(
                plan.reason,
                plan.scroll_anchor_index.min(count.saturating_sub(1)),
                plan.scroll_anchor_offset,
                plan.preserved_scroll_anchor,
                plan.priority_start.min(count.saturating_sub(1)),
                plan.priority_end.min(count.saturating_sub(1)),
                count,
            ));
        }

        if self.has_stale_message_heights() && self.remeasure_plan.is_none() {
            self.schedule_remeasure(LayoutRemeasureReason::MessagesFrom);
        }
    }

    /// Set the visual height for message `idx`, growing the vec if needed.
    pub fn set_message_height(&mut self, idx: usize, h: usize) {
        if idx >= self.message_heights.len() {
            self.sync_message_count(idx + 1);
        }
        if self.message_heights.get(idx).copied().unwrap_or(0) != h {
            self.message_heights[idx] = h;
            self.mark_prefix_sums_dirty_from(idx);
        }
    }

    /// Mark one message height as exact for the current viewport width.
    pub fn mark_message_height_measured(&mut self, idx: usize) {
        if idx >= self.measured_message_widths.len() {
            self.sync_message_count(idx + 1);
        }
        self.measured_message_widths[idx] = self.width;
        self.stale_message_heights[idx] = false;
        self.priority_remeasure.retain(|&pending| pending != idx);
    }

    /// Return whether a message height is exact at the current width.
    #[must_use]
    pub fn message_height_is_current(&self, idx: usize) -> bool {
        if self.stale_message_heights.get(idx).copied().unwrap_or(false) {
            return false;
        }
        if self.message_heights_width == self.width {
            return idx < self.message_heights.len();
        }
        self.measured_message_widths.get(idx).copied().unwrap_or(0) == self.width
    }

    /// Return whether any queued remeasurement is still active.
    #[must_use]
    pub fn remeasure_active(&self) -> bool {
        self.remeasure_plan.is_some()
    }

    /// Return whether any message height is still stale.
    #[must_use]
    pub fn has_stale_message_heights(&self) -> bool {
        self.stale_message_heights.iter().any(|stale| *stale)
    }

    /// Mark one message as stale and queue it for exact remeasurement.
    pub fn invalidate_message(&mut self, idx: usize) {
        if idx >= self.message_heights.len() {
            self.sync_message_count(idx + 1);
        }
        self.message_heights_width = 0;
        self.measured_message_widths[idx] = 0;
        self.stale_message_heights[idx] = true;
        self.queue_priority_remeasure(idx);
        self.mark_prefix_sums_dirty_from(idx);
        self.schedule_remeasure(LayoutRemeasureReason::MessageChanged);
    }

    /// Mark every message from `idx` onward as stale and queue visible-first remeasurement.
    pub fn invalidate_messages_from(&mut self, idx: usize) {
        if self.message_heights.is_empty() {
            return;
        }
        let start = idx.min(self.message_heights.len().saturating_sub(1));
        self.message_heights_width = 0;
        self.measured_message_widths[start..].fill(0);
        self.stale_message_heights[start..].fill(true);
        self.mark_prefix_sums_dirty_from(start);
        self.schedule_remeasure(LayoutRemeasureReason::MessagesFrom);
    }

    /// Mark all messages stale and queue visible-first remeasurement.
    pub fn invalidate_all_messages(&mut self, reason: LayoutRemeasureReason) {
        if self.message_heights.is_empty() {
            return;
        }
        self.message_heights_width = 0;
        self.measured_message_widths.fill(0);
        self.stale_message_heights.fill(true);
        self.priority_remeasure.clear();
        self.mark_prefix_sums_dirty_from(0);
        self.schedule_remeasure(reason);
    }

    fn queue_priority_remeasure(&mut self, idx: usize) {
        if self.priority_remeasure.contains(&idx) {
            return;
        }
        self.priority_remeasure.push(idx);
    }

    /// Pop the next message that must be remeasured before the general queue continues.
    pub fn next_priority_remeasure(&mut self) -> Option<usize> {
        while let Some(idx) = self.priority_remeasure.pop() {
            if self.stale_message_heights.get(idx).copied().unwrap_or(false) {
                return Some(idx);
            }
        }
        None
    }

    fn schedule_remeasure(&mut self, reason: LayoutRemeasureReason) {
        if self.message_heights.is_empty() {
            self.remeasure_plan = None;
            return;
        }
        let (anchor_index, anchor_offset) = self.current_scroll_anchor();
        let preserved_scroll_anchor =
            if matches!(reason, LayoutRemeasureReason::Resize | LayoutRemeasureReason::Global) {
                Some(PreservedScrollAnchor { reason, index: anchor_index, offset: anchor_offset })
            } else {
                self.remeasure_plan.and_then(|plan| plan.preserved_scroll_anchor)
            };
        self.remeasure_plan = Some(LayoutRemeasurePlan::from_scroll_anchor(
            reason,
            anchor_index,
            anchor_offset,
            preserved_scroll_anchor,
            self.message_heights.len(),
        ));
    }

    /// Capture the current message-local scroll anchor when manual scrolling is active.
    #[must_use]
    pub fn capture_manual_scroll_anchor(&self) -> Option<(usize, usize)> {
        (!self.auto_scroll && !self.message_heights.is_empty())
            .then(|| self.current_scroll_anchor())
    }

    /// Preserve a message-local anchor across topology or lifecycle-driven layout changes.
    pub fn preserve_scroll_anchor(
        &mut self,
        reason: LayoutRemeasureReason,
        anchor_index: usize,
        anchor_offset: usize,
    ) {
        if self.message_heights.is_empty() {
            self.remeasure_plan = None;
            return;
        }

        let anchor = PreservedScrollAnchor {
            reason,
            index: anchor_index.min(self.message_heights.len().saturating_sub(1)),
            offset: anchor_offset,
        };

        if let Some(plan) = self.remeasure_plan.as_mut() {
            plan.preserved_scroll_anchor = Some(anchor);
            return;
        }

        self.remeasure_plan = Some(LayoutRemeasurePlan::from_scroll_anchor(
            reason,
            anchor.index,
            anchor.offset,
            Some(anchor),
            self.message_heights.len(),
        ));
    }

    /// Reset the outward expansion frontiers around the current visible window.
    pub fn ensure_remeasure_anchor(
        &mut self,
        visible_start: usize,
        visible_end: usize,
        message_count: usize,
    ) {
        if message_count == 0 || self.remeasure_plan.is_none() {
            return;
        }
        let Some(plan) = self.remeasure_plan else {
            return;
        };
        let next = LayoutRemeasurePlan::new(
            plan.reason,
            plan.scroll_anchor_index,
            plan.scroll_anchor_offset,
            plan.preserved_scroll_anchor,
            visible_start,
            visible_end,
            message_count,
        );
        let needs_reanchor = self.remeasure_plan.is_some_and(|current| {
            current.priority_start != next.priority_start
                || current.priority_end != next.priority_end
        });
        if needs_reanchor {
            self.remeasure_plan = Some(next);
        }
    }

    /// Resume outward remeasurement from the current visible anchor.
    pub fn next_remeasure_index(&mut self, message_count: usize) -> Option<usize> {
        let prioritize_above_for_anchor = self.remeasure_plan.is_some_and(|plan| {
            plan.preserved_scroll_anchor
                .is_some_and(|anchor| !self.prefix_is_exact_through(anchor.index))
        });
        let plan = self.remeasure_plan.as_mut()?;
        let choose_above = match (plan.next_above, plan.next_below < message_count) {
            (Some(_), true) if prioritize_above_for_anchor => true,
            (Some(_), true) => {
                let choose = plan.prefer_above;
                plan.prefer_above = !plan.prefer_above;
                choose
            }
            (Some(_), false) => true,
            (None, true) => false,
            (None, false) => {
                self.remeasure_plan = None;
                return None;
            }
        };
        if choose_above {
            let idx = plan.next_above?;
            plan.next_above = idx.checked_sub(1);
            Some(idx)
        } else {
            let idx = plan.next_below;
            plan.next_below = plan.next_below.saturating_add(1);
            Some(idx)
        }
    }

    /// Return the preserved scroll anchor that should be restored while remeasure
    /// remains in flight.
    #[must_use]
    pub fn scroll_anchor_to_restore(&self) -> Option<(usize, usize)> {
        self.remeasure_plan.and_then(|plan| {
            plan.preserved_scroll_anchor.map(|anchor| (anchor.index, anchor.offset))
        })
    }

    /// Return the preserved scroll anchor only once rows above it are exact.
    #[must_use]
    pub fn ready_scroll_anchor_to_restore(&self) -> Option<(usize, usize)> {
        self.remeasure_plan.and_then(|plan| {
            plan.preserved_scroll_anchor.and_then(|anchor| {
                self.prefix_is_exact_through(anchor.index).then_some((anchor.index, anchor.offset))
            })
        })
    }

    /// Return the preserved pre-width-resize scroll anchor.
    #[must_use]
    pub fn resize_scroll_anchor(&self) -> Option<(usize, usize)> {
        self.remeasure_plan.and_then(|plan| {
            plan.preserved_scroll_anchor.and_then(|anchor| {
                (anchor.reason == LayoutRemeasureReason::Resize)
                    .then_some((anchor.index, anchor.offset))
            })
        })
    }

    /// Derive the priority window from the preserved scroll anchor using current estimates.
    #[must_use]
    pub fn remeasure_anchor_window(&self, viewport_height: usize) -> Option<(usize, usize)> {
        let plan = self.remeasure_plan?;
        if self.message_heights.is_empty() {
            return None;
        }
        let start = plan.scroll_anchor_index.min(self.message_heights.len().saturating_sub(1));
        let mut end = start;
        let needed_rows = plan.scroll_anchor_offset.saturating_add(viewport_height.max(1));
        let mut covered_rows = self.message_height(start);
        while end + 1 < self.message_heights.len() && covered_rows < needed_rows {
            end += 1;
            covered_rows = covered_rows.saturating_add(self.message_height(end));
        }
        Some((start, end))
    }

    /// Restore the absolute scroll position from a preserved message-local anchor.
    #[allow(clippy::cast_precision_loss)]
    pub fn restore_scroll_anchor(&mut self, anchor_index: usize, anchor_offset: usize) {
        if self.auto_scroll || self.message_heights.is_empty() {
            return;
        }
        let anchor_index = anchor_index.min(self.message_heights.len().saturating_sub(1));
        let anchor_height = self.message_height(anchor_index);
        let clamped_offset =
            if anchor_height == 0 { 0 } else { anchor_offset.min(anchor_height.saturating_sub(1)) };
        let scroll = self.cumulative_height_before(anchor_index).saturating_add(clamped_offset);
        self.scroll_target = scroll;
        self.scroll_pos = scroll as f32;
        self.scroll_offset = scroll;
    }

    /// Mark prefix sums dirty from `idx` onward.
    pub fn mark_prefix_sums_dirty_from(&mut self, idx: usize) {
        self.prefix_dirty_from = Some(self.prefix_dirty_from.map_or(idx, |oldest| oldest.min(idx)));
        self.prefix_sums_width = 0;
    }

    /// Finalize queued remeasurement when all messages are current again.
    pub fn finalize_remeasure_if_clean(&mut self) {
        if self.has_stale_message_heights() {
            return;
        }
        self.message_heights_width = self.width;
        self.measured_message_widths.fill(self.width);
        self.priority_remeasure.clear();
        self.remeasure_plan = None;
    }

    /// Mark all message heights exact at the current width.
    ///
    /// This remains available for tests that seed viewport state directly.
    pub fn mark_heights_valid(&mut self) {
        self.stale_message_heights.fill(false);
        self.finalize_remeasure_if_clean();
    }

    /// Compatibility helper for tests and metrics.
    #[must_use]
    pub fn resize_remeasure_active(&self) -> bool {
        self.remeasure_active()
    }

    /// Compatibility helper for tests that seed and advance the queued plan directly.
    pub fn ensure_resize_remeasure_anchor(
        &mut self,
        visible_start: usize,
        visible_end: usize,
        message_count: usize,
    ) {
        self.ensure_remeasure_anchor(visible_start, visible_end, message_count);
    }

    /// Compatibility helper for tests that seed and advance the queued plan directly.
    pub fn next_resize_remeasure_index(&mut self, message_count: usize) -> Option<usize> {
        self.next_remeasure_index(message_count)
    }

    // --- Prefix sums ---

    /// Rebuild prefix sums from `message_heights`, starting from the oldest dirty index.
    pub fn rebuild_prefix_sums(&mut self) {
        let n = self.message_heights.len();
        if n == 0 {
            self.height_prefix_sums.clear();
            self.prefix_dirty_from = None;
            self.prefix_sums_width = self.width;
            return;
        }
        if self.height_prefix_sums.len() != n {
            self.height_prefix_sums.resize(n, 0);
            self.prefix_dirty_from = Some(0);
        }

        let Some(start) = self.prefix_dirty_from else {
            if self.prefix_sums_width == self.width {
                return;
            }
            self.prefix_dirty_from = Some(0);
            return self.rebuild_prefix_sums();
        };

        let start = start.min(n.saturating_sub(1));
        let mut acc = if start == 0 { 0 } else { self.height_prefix_sums[start - 1] };
        for idx in start..n {
            acc = acc.saturating_add(self.message_heights[idx]);
            self.height_prefix_sums[idx] = acc;
        }
        self.prefix_dirty_from = None;
        self.prefix_sums_width = self.width;
    }

    /// Total height of all messages (O(1) via prefix sums).
    #[must_use]
    pub fn total_message_height(&self) -> usize {
        self.height_prefix_sums.last().copied().unwrap_or(0)
    }

    /// Cumulative height of messages `0..idx` (O(1) via prefix sums).
    #[must_use]
    pub fn cumulative_height_before(&self, idx: usize) -> usize {
        if idx == 0 { 0 } else { self.height_prefix_sums.get(idx - 1).copied().unwrap_or(0) }
    }

    /// Binary search for the first message whose cumulative range overlaps `scroll_offset`.
    #[must_use]
    pub fn find_first_visible(&self, scroll_offset: usize) -> usize {
        if self.height_prefix_sums.is_empty() {
            return 0;
        }
        self.height_prefix_sums
            .partition_point(|&h| h <= scroll_offset)
            .min(self.message_heights.len().saturating_sub(1))
    }

    /// Binary search for the last message whose cumulative range overlaps the viewport.
    #[must_use]
    pub fn find_last_visible(&self, scroll_offset: usize, viewport_height: usize) -> usize {
        if self.height_prefix_sums.is_empty() {
            return 0;
        }
        let visible_end = scroll_offset.saturating_add(viewport_height);
        self.height_prefix_sums
            .partition_point(|&h| h < visible_end)
            .min(self.message_heights.len().saturating_sub(1))
    }

    /// Derive the current visible window from the latest estimated prefix sums.
    #[must_use]
    pub fn current_visible_window(&self, viewport_height: usize) -> Option<(usize, usize)> {
        if self.message_heights.is_empty() {
            return None;
        }
        if self.height_prefix_sums.is_empty() {
            return Some((0, 0));
        }
        let start = self.find_first_visible(self.scroll_offset);
        let end = self.find_last_visible(self.scroll_offset, viewport_height.max(1));
        Some((start.min(end), end.max(start)))
    }

    fn current_scroll_anchor(&self) -> (usize, usize) {
        if self.message_heights.is_empty() {
            return (0, 0);
        }
        let first_visible = self.find_first_visible_in_estimates(self.scroll_offset);
        let offset_in_message = self
            .scroll_offset
            .saturating_sub(self.cumulative_height_before_in_estimates(first_visible));
        (first_visible, offset_in_message)
    }

    fn find_first_visible_in_estimates(&self, scroll_offset: usize) -> usize {
        let mut acc = 0usize;
        for (idx, &height) in self.message_heights.iter().enumerate() {
            acc = acc.saturating_add(height);
            if acc > scroll_offset {
                return idx;
            }
        }
        if acc == 0 { 0 } else { self.message_heights.len().saturating_sub(1) }
    }

    fn cumulative_height_before_in_estimates(&self, idx: usize) -> usize {
        self.message_heights.iter().take(idx).copied().sum()
    }

    fn prefix_is_exact_through(&self, idx: usize) -> bool {
        if self.message_heights.is_empty() {
            return false;
        }
        let end = idx.min(self.message_heights.len().saturating_sub(1));
        if self.message_heights_width == self.width {
            return !self.stale_message_heights.iter().take(end + 1).any(|stale| *stale);
        }
        !(0..=end).any(|message_idx| {
            self.stale_message_heights.get(message_idx).copied().unwrap_or(true)
                || self.measured_message_widths.get(message_idx).copied().unwrap_or(0) != self.width
        })
    }

    // --- Scroll ---

    /// Scroll up by `lines`. Disables auto-scroll.
    #[allow(clippy::cast_precision_loss)]
    pub fn scroll_up(&mut self, lines: usize) {
        self.scroll_target = self.scroll_target.saturating_sub(lines);
        self.auto_scroll = false;
        self.scroll_pos = self.scroll_target as f32;
        self.scroll_offset = self.scroll_target;
    }

    /// Scroll down by `lines`. Auto-scroll re-engagement handled by render.
    #[allow(clippy::cast_precision_loss)]
    pub fn scroll_down(&mut self, lines: usize) {
        self.scroll_target = self.scroll_target.saturating_add(lines);
        self.scroll_pos = self.scroll_target as f32;
        self.scroll_offset = self.scroll_target;
    }

    /// Re-engage auto-scroll (stick to bottom).
    pub fn engage_auto_scroll(&mut self) {
        self.auto_scroll = true;
    }
}

impl Default for ChatViewport {
    fn default() -> Self {
        Self::new()
    }
}
