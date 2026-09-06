// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use std::cell::Cell;
use std::sync::atomic::{AtomicU64, Ordering};

static CACHE_ACCESS_TICK: AtomicU64 = AtomicU64::new(1);

fn next_cache_access_tick() -> u64 {
    CACHE_ACCESS_TICK.fetch_add(1, Ordering::Relaxed)
}

/// Cached rendered lines for a block. Stores a version counter so the cache
/// is only recomputed when the block content actually changes.
///
/// Fields are private - use `invalidate()` to mark stale, `is_stale()` to check,
/// `get()` to read cached lines, and `store()` to populate.
#[derive(Default)]
pub struct BlockCache {
    version: u64,
    lines: Option<Vec<ratatui::text::Line<'static>>>,
    /// Segmentation metadata for KB-sized cache chunks shared across message/tool caches.
    segments: Vec<CacheLineSegment>,
    /// Approximate UTF-8 byte size of cached rendered lines.
    cached_bytes: usize,
    /// Wrapped line count of the cached lines at `wrapped_width`.
    /// Computed via `Paragraph::line_count(width)` on the same lines stored in `lines`.
    wrapped_height: usize,
    /// The viewport width used to compute `wrapped_height`.
    wrapped_width: u16,
    wrapped_height_valid: bool,
    last_access_tick: Cell<u64>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
struct CacheLineSegment {
    start: usize,
    end: usize,
    wrapped_height: usize,
    wrapped_width: u16,
    wrapped_height_valid: bool,
}

impl CacheLineSegment {
    #[must_use]
    fn new(start: usize, end: usize) -> Self {
        Self { start, end, wrapped_height: 0, wrapped_width: 0, wrapped_height_valid: false }
    }
}

impl BlockCache {
    fn touch(&self) {
        self.last_access_tick.set(next_cache_access_tick());
    }

    /// Bump the version to invalidate cached lines and height.
    pub fn invalidate(&mut self) {
        self.version += 1;
        self.wrapped_height_valid = false;
    }

    /// Get a reference to the cached lines, if fresh.
    #[must_use]
    pub fn get(&self) -> Option<&Vec<ratatui::text::Line<'static>>> {
        if self.version == 0 {
            let lines = self.lines.as_ref();
            if lines.is_some() {
                self.touch();
            }
            lines
        } else {
            None
        }
    }

    /// Store freshly rendered lines, marking the cache as clean.
    /// Height is set separately via `set_height()` after measurement.
    pub fn store(&mut self, lines: Vec<ratatui::text::Line<'static>>) {
        self.store_with_policy(lines, *super::super::default_cache_split_policy());
    }

    /// Store freshly rendered lines using a shared KB split policy.
    pub fn store_with_policy(
        &mut self,
        lines: Vec<ratatui::text::Line<'static>>,
        policy: super::super::CacheSplitPolicy,
    ) {
        let segment_limit = policy.hard_limit_bytes.max(1);
        let (segments, cached_bytes) = build_line_segments(&lines, segment_limit);
        self.lines = Some(lines);
        self.segments = segments;
        self.cached_bytes = cached_bytes;
        self.version = 0;
        self.wrapped_height = 0;
        self.wrapped_width = 0;
        self.wrapped_height_valid = false;
        self.touch();
    }

    /// Set the wrapped height for the cached lines at the given width.
    /// Called by the viewport/chat layer after `Paragraph::line_count(width)`.
    /// Separate from `store()` so height measurement is the viewport's job.
    pub fn set_height(&mut self, height: usize, width: u16) {
        self.wrapped_height = height;
        self.wrapped_width = width;
        self.wrapped_height_valid = true;
        self.touch();
    }

    /// Store lines and set height in one call.
    /// Deprecated: prefer `store()` + `set_height()` to keep concerns separate.
    pub fn store_with_height(
        &mut self,
        lines: Vec<ratatui::text::Line<'static>>,
        height: usize,
        width: u16,
    ) {
        self.store(lines);
        self.set_height(height, width);
    }

    /// Get the cached wrapped height if cache is valid and was computed at the given width.
    #[must_use]
    pub fn height_at(&self, width: u16) -> Option<usize> {
        if self.version == 0 && self.wrapped_height_valid && self.wrapped_width == width {
            self.touch();
            Some(self.wrapped_height)
        } else {
            None
        }
    }

    /// Recompute wrapped height from cached segments and memoize it at `width`.
    /// Returns `None` when the render cache is stale.
    pub fn measure_and_set_height(&mut self, width: u16) -> Option<usize> {
        if self.version != 0 {
            return None;
        }
        if let Some(h) = self.height_at(width) {
            return Some(h);
        }

        let lines = self.lines.as_ref()?;

        if self.segments.is_empty() {
            self.set_height(0, width);
            return Some(0);
        }

        let mut total_height = 0usize;
        for segment in &mut self.segments {
            if segment.wrapped_height_valid && segment.wrapped_width == width {
                total_height = total_height.saturating_add(segment.wrapped_height);
                continue;
            }
            let segment_lines = lines[segment.start..segment.end].to_vec();
            let h = ratatui::widgets::Paragraph::new(ratatui::text::Text::from(segment_lines))
                .wrap(ratatui::widgets::Wrap { trim: false })
                .line_count(width);
            segment.wrapped_height = h;
            segment.wrapped_width = width;
            segment.wrapped_height_valid = true;
            total_height = total_height.saturating_add(h);
        }

        self.set_height(total_height, width);
        Some(total_height)
    }

    #[must_use]
    pub fn segment_count(&self) -> usize {
        self.segments.len()
    }

    #[must_use]
    pub fn cached_bytes(&self) -> usize {
        self.cached_bytes
    }

    #[must_use]
    pub fn last_access_tick(&self) -> u64 {
        self.last_access_tick.get()
    }

    pub fn evict_cached_render(&mut self) -> usize {
        let removed = self.cached_bytes;
        if removed == 0 {
            return 0;
        }
        self.lines = None;
        self.segments.clear();
        self.cached_bytes = 0;
        self.wrapped_height = 0;
        self.wrapped_width = 0;
        self.wrapped_height_valid = false;
        self.version = self.version.wrapping_add(1);
        removed
    }
}

fn build_line_segments(
    lines: &[ratatui::text::Line<'static>],
    segment_limit_bytes: usize,
) -> (Vec<CacheLineSegment>, usize) {
    if lines.is_empty() {
        return (Vec::new(), 0);
    }

    let limit = segment_limit_bytes.max(1);
    let mut segments = Vec::new();
    let mut total_bytes = 0usize;
    let mut start = 0usize;
    let mut acc = 0usize;

    for (idx, line) in lines.iter().enumerate() {
        let line_bytes = line_utf8_bytes(line).max(1);
        total_bytes = total_bytes.saturating_add(line_bytes);

        if idx > start && acc.saturating_add(line_bytes) > limit {
            segments.push(CacheLineSegment::new(start, idx));
            start = idx;
            acc = 0;
        }
        acc = acc.saturating_add(line_bytes);
    }

    segments.push(CacheLineSegment::new(start, lines.len()));
    (segments, total_bytes)
}

fn line_utf8_bytes(line: &ratatui::text::Line<'static>) -> usize {
    let span_bytes =
        line.spans.iter().fold(0usize, |acc, span| acc.saturating_add(span.content.len()));
    span_bytes.saturating_add(1)
}
