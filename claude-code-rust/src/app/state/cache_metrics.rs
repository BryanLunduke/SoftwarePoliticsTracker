// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

//! Cache observability: accumulation, snapshots, and rate-limited structured logging.
//!
//! `CacheMetrics` lives on `App` and accumulates cross-cutting counters (enforcement
//! counts, watermarks, rate-limit cooldown state). `CacheMetricsSnapshot` is a
//! computed on-demand view that pulls from `RenderCacheBudget`, `HistoryRetentionStats`,
//! `CacheMetrics`, and `ChatViewport`.
//!
//! All structured tracing uses `target: "cache"` so it can be enabled via
//! `--log-filter "cache=debug"` without affecting other log targets.

use super::types::{
    CacheBudgetEnforceStats, HistoryRetentionPolicy, HistoryRetentionStats, RenderCacheBudget,
};
use super::viewport::ChatViewport;

// ---------------------------------------------------------------------------
// Rate-limit constants
// ---------------------------------------------------------------------------

/// Emit render-cache debug log every N enforcement calls (~1/sec at 60 FPS).
const RENDER_LOG_INTERVAL: u64 = 60;

/// Emit history-retention debug log every N enforcement calls.
const HISTORY_LOG_INTERVAL: u64 = 10;

/// Suppress repeated warn-level emissions for this many enforcement calls (~5 sec).
const WARN_COOLDOWN_CALLS: u64 = 300;

/// Utilization percentage that triggers a warn-level log.
const HIGH_UTILIZATION_THRESHOLD: f32 = 90.0;

/// Number of blocks evicted in a single pass that triggers a warn-level log.
const EVICTION_SPIKE_THRESHOLD: usize = 10;

// ---------------------------------------------------------------------------
// Persistent accumulator (lives on App)
// ---------------------------------------------------------------------------

/// Cross-cutting cache metrics accumulated over the session lifetime.
///
/// Updated by `record_render_enforcement` and `record_history_enforcement`
/// after each budget enforcement pass. Rate-limit cooldown state is internal
/// and not meaningful to external consumers.
#[derive(Debug, Clone, Copy)]
pub struct CacheMetrics {
    // -- Render cache --
    pub render_enforcement_count: u64,
    pub render_peak_bytes: usize,

    // -- History retention --
    pub history_enforcement_count: u64,
    pub history_peak_bytes: usize,

    // -- Viewport --
    pub resize_count: u64,

    // -- Rate-limit cooldown (private) --
    render_log_countdown: u64,
    history_log_countdown: u64,
    warn_cooldown_remaining: u64,
}

impl Default for CacheMetrics {
    fn default() -> Self {
        Self {
            render_enforcement_count: 0,
            render_peak_bytes: 0,
            history_enforcement_count: 0,
            history_peak_bytes: 0,
            resize_count: 0,
            // Fire on the very first call (countdown starts at 1 so it
            // decrements to 0 and triggers immediately).
            render_log_countdown: 1,
            history_log_countdown: 1,
            warn_cooldown_remaining: 0,
        }
    }
}

impl CacheMetrics {
    /// Record one render-cache enforcement pass.
    ///
    /// Returns `true` when a debug-level log should be emitted (every
    /// `RENDER_LOG_INTERVAL` calls).
    pub fn record_render_enforcement(
        &mut self,
        stats: &CacheBudgetEnforceStats,
        _budget: &RenderCacheBudget,
    ) -> bool {
        self.render_enforcement_count += 1;
        if stats.total_before_bytes > self.render_peak_bytes {
            self.render_peak_bytes = stats.total_before_bytes;
        }

        self.render_log_countdown -= 1;
        if self.render_log_countdown == 0 {
            self.render_log_countdown = RENDER_LOG_INTERVAL;
            true
        } else {
            false
        }
    }

    /// Record one history-retention enforcement pass.
    ///
    /// Returns `true` when a debug-level log should be emitted (every
    /// `HISTORY_LOG_INTERVAL` calls).
    pub fn record_history_enforcement(
        &mut self,
        stats: &HistoryRetentionStats,
        _policy: HistoryRetentionPolicy,
    ) -> bool {
        self.history_enforcement_count += 1;
        if stats.total_before_bytes > self.history_peak_bytes {
            self.history_peak_bytes = stats.total_before_bytes;
        }

        self.history_log_countdown -= 1;
        if self.history_log_countdown == 0 {
            self.history_log_countdown = HISTORY_LOG_INTERVAL;
            true
        } else {
            false
        }
    }

    /// Record a viewport resize event.
    pub fn record_resize(&mut self) {
        self.resize_count += 1;
    }

    /// Check whether a warn-level log should fire based on current utilization
    /// and eviction counts. Returns `Some(kind)` and resets the cooldown, or
    /// `None` if suppressed.
    pub fn check_warn_condition(
        &mut self,
        render_util_pct: f32,
        history_util_pct: f32,
        evicted_blocks: usize,
    ) -> Option<CacheWarnKind> {
        if self.warn_cooldown_remaining > 0 {
            self.warn_cooldown_remaining -= 1;
            return None;
        }

        let kind = if render_util_pct >= HIGH_UTILIZATION_THRESHOLD {
            Some(CacheWarnKind::HighRenderUtilization(render_util_pct))
        } else if history_util_pct >= HIGH_UTILIZATION_THRESHOLD {
            Some(CacheWarnKind::HighHistoryUtilization(history_util_pct))
        } else if evicted_blocks >= EVICTION_SPIKE_THRESHOLD {
            Some(CacheWarnKind::EvictionSpike(evicted_blocks))
        } else {
            None
        };

        if kind.is_some() {
            self.warn_cooldown_remaining = WARN_COOLDOWN_CALLS;
        }
        kind
    }
}

// ---------------------------------------------------------------------------
// Warn kind
// ---------------------------------------------------------------------------

/// Classification of cache warning conditions for structured logging.
#[derive(Debug, Clone, Copy)]
pub enum CacheWarnKind {
    HighRenderUtilization(f32),
    HighHistoryUtilization(f32),
    EvictionSpike(usize),
}

// ---------------------------------------------------------------------------
// On-demand snapshot (not stored)
// ---------------------------------------------------------------------------

/// Point-in-time view of all cache subsystems, computed on demand.
#[derive(Debug, Clone, Copy)]
pub struct CacheMetricsSnapshot {
    // Render cache
    pub render_bytes: usize,
    pub render_max_bytes: usize,
    pub render_utilization_pct: f32,
    pub render_entry_count: usize,
    pub render_evictions_this_frame: usize,
    pub render_total_evictions: usize,
    pub render_enforcement_count: u64,
    pub render_peak_bytes: usize,
    /// Bytes in protected (non-evictable) blocks excluded from the budget comparison.
    pub render_protected_bytes: usize,

    // History retention
    pub history_bytes: usize,
    pub history_max_bytes: usize,
    pub history_utilization_pct: f32,
    pub history_dropped_messages_this_pass: usize,
    pub history_total_dropped_messages: usize,
    pub history_total_dropped_bytes: usize,
    pub history_enforcement_count: u64,
    pub history_peak_bytes: usize,

    // Viewport dirtiness
    pub viewport_prefix_dirty_from: Option<usize>,
    pub viewport_stale_messages: usize,
    pub viewport_remeasure_active: bool,
    pub viewport_width_valid: bool,
    pub viewport_prefix_sums_valid: bool,
    pub resize_count: u64,
}

/// Build a snapshot from all cache subsystem state.
///
/// Only called on log cadence (not every frame), so the cost of collecting
/// fields is negligible.
#[must_use]
#[allow(clippy::too_many_arguments, clippy::cast_precision_loss)]
pub fn build_snapshot(
    budget: &RenderCacheBudget,
    retention_stats: &HistoryRetentionStats,
    retention_policy: HistoryRetentionPolicy,
    metrics: &CacheMetrics,
    viewport: &ChatViewport,
    render_entry_count: usize,
    evictions_this_frame: usize,
    dropped_this_pass: usize,
    protected_bytes: usize,
) -> CacheMetricsSnapshot {
    let render_util = if budget.max_bytes > 0 {
        (budget.last_total_bytes as f32 / budget.max_bytes as f32) * 100.0
    } else {
        0.0
    };
    let history_util = if retention_policy.max_bytes > 0 {
        (retention_stats.total_after_bytes as f32 / retention_policy.max_bytes as f32) * 100.0
    } else {
        0.0
    };

    CacheMetricsSnapshot {
        render_bytes: budget.last_total_bytes,
        render_max_bytes: budget.max_bytes,
        render_utilization_pct: render_util,
        render_entry_count,
        render_evictions_this_frame: evictions_this_frame,
        render_total_evictions: budget.total_evictions,
        render_enforcement_count: metrics.render_enforcement_count,
        render_peak_bytes: metrics.render_peak_bytes,
        render_protected_bytes: protected_bytes,

        history_bytes: retention_stats.total_after_bytes,
        history_max_bytes: retention_policy.max_bytes,
        history_utilization_pct: history_util,
        history_dropped_messages_this_pass: dropped_this_pass,
        history_total_dropped_messages: retention_stats.total_dropped_messages,
        history_total_dropped_bytes: retention_stats.total_dropped_bytes,
        history_enforcement_count: metrics.history_enforcement_count,
        history_peak_bytes: metrics.history_peak_bytes,

        viewport_prefix_dirty_from: viewport.prefix_dirty_from(),
        viewport_stale_messages: viewport.stale_message_count(),
        viewport_remeasure_active: viewport.remeasure_active(),
        viewport_width_valid: viewport.message_heights_width == viewport.width
            && viewport.width > 0,
        viewport_prefix_sums_valid: viewport.prefix_sums_width == viewport.width
            && viewport.width > 0,
        resize_count: metrics.resize_count,
    }
}

// ---------------------------------------------------------------------------
// Structured tracing emitters
// ---------------------------------------------------------------------------

/// Emit a debug-level structured log summarizing render cache state.
pub fn emit_render_metrics(snap: &CacheMetricsSnapshot) {
    tracing::debug!(
        target: "cache",
        render_bytes = snap.render_bytes,
        render_max = snap.render_max_bytes,
        render_util_pct = format_args!("{:.1}", snap.render_utilization_pct),
        render_entries = snap.render_entry_count,
        render_protected = snap.render_protected_bytes,
        render_evictions_frame = snap.render_evictions_this_frame,
        render_evictions_total = snap.render_total_evictions,
        render_peak = snap.render_peak_bytes,
        render_enforcements = snap.render_enforcement_count,
        viewport_prefix_dirty_from = ?snap.viewport_prefix_dirty_from,
        viewport_stale_messages = snap.viewport_stale_messages,
        viewport_remeasure_active = snap.viewport_remeasure_active,
        viewport_width_valid = snap.viewport_width_valid,
        viewport_prefix_sums_valid = snap.viewport_prefix_sums_valid,
        resize_count = snap.resize_count,
        "render cache metrics"
    );
}

/// Emit a debug-level structured log summarizing history retention state.
pub fn emit_history_metrics(snap: &CacheMetricsSnapshot) {
    tracing::debug!(
        target: "cache",
        history_bytes = snap.history_bytes,
        history_max = snap.history_max_bytes,
        history_util_pct = format_args!("{:.1}", snap.history_utilization_pct),
        history_dropped_pass = snap.history_dropped_messages_this_pass,
        history_dropped_total = snap.history_total_dropped_messages,
        history_dropped_bytes_total = snap.history_total_dropped_bytes,
        history_peak = snap.history_peak_bytes,
        history_enforcements = snap.history_enforcement_count,
        "history retention metrics"
    );
}

/// Emit a warn-level structured log for a cache warning condition.
pub fn emit_cache_warning(kind: &CacheWarnKind) {
    match kind {
        CacheWarnKind::HighRenderUtilization(pct) => {
            tracing::warn!(
                target: "cache",
                util_pct = format_args!("{:.1}", pct),
                "render cache utilization high"
            );
        }
        CacheWarnKind::HighHistoryUtilization(pct) => {
            tracing::warn!(
                target: "cache",
                util_pct = format_args!("{:.1}", pct),
                "history retention utilization high"
            );
        }
        CacheWarnKind::EvictionSpike(blocks) => {
            tracing::warn!(
                target: "cache",
                evicted_blocks = blocks,
                "render cache eviction spike"
            );
        }
    }
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    fn make_render_stats(before_bytes: usize, evicted_blocks: usize) -> CacheBudgetEnforceStats {
        CacheBudgetEnforceStats {
            total_before_bytes: before_bytes,
            total_after_bytes: before_bytes,
            evicted_bytes: 0,
            evicted_blocks,
            protected_bytes: 0,
        }
    }

    fn make_history_stats(before_bytes: usize, dropped: usize) -> HistoryRetentionStats {
        HistoryRetentionStats {
            total_before_bytes: before_bytes,
            total_after_bytes: before_bytes,
            dropped_messages: dropped,
            dropped_bytes: 0,
            total_dropped_messages: dropped,
            total_dropped_bytes: 0,
        }
    }

    #[test]
    fn render_log_fires_on_first_call_and_at_interval() {
        let mut m = CacheMetrics::default();
        let stats = make_render_stats(1000, 0);
        let budget = RenderCacheBudget::default();

        // First call should fire (countdown starts at 1).
        assert!(m.record_render_enforcement(&stats, &budget));

        // Next RENDER_LOG_INTERVAL - 1 calls should not fire.
        for _ in 1..RENDER_LOG_INTERVAL {
            assert!(!m.record_render_enforcement(&stats, &budget));
        }

        // The interval-th call fires again.
        assert!(m.record_render_enforcement(&stats, &budget));
        assert_eq!(m.render_enforcement_count, RENDER_LOG_INTERVAL + 1);
    }

    #[test]
    fn history_log_fires_on_first_call_and_at_interval() {
        let mut m = CacheMetrics::default();
        let stats = make_history_stats(2000, 0);
        let policy = HistoryRetentionPolicy::default();

        assert!(m.record_history_enforcement(&stats, policy));

        for _ in 1..HISTORY_LOG_INTERVAL {
            assert!(!m.record_history_enforcement(&stats, policy));
        }

        assert!(m.record_history_enforcement(&stats, policy));
        assert_eq!(m.history_enforcement_count, HISTORY_LOG_INTERVAL + 1);
    }

    #[test]
    fn peak_bytes_tracks_maximum() {
        let mut m = CacheMetrics::default();
        let budget = RenderCacheBudget::default();

        m.record_render_enforcement(&make_render_stats(5000, 0), &budget);
        assert_eq!(m.render_peak_bytes, 5000);

        m.record_render_enforcement(&make_render_stats(3000, 0), &budget);
        assert_eq!(m.render_peak_bytes, 5000); // unchanged

        m.record_render_enforcement(&make_render_stats(8000, 0), &budget);
        assert_eq!(m.render_peak_bytes, 8000); // updated
    }

    #[test]
    fn history_peak_bytes_tracks_maximum() {
        let mut m = CacheMetrics::default();
        let policy = HistoryRetentionPolicy::default();

        m.record_history_enforcement(&make_history_stats(10_000, 0), policy);
        assert_eq!(m.history_peak_bytes, 10_000);

        m.record_history_enforcement(&make_history_stats(5_000, 0), policy);
        assert_eq!(m.history_peak_bytes, 10_000);
    }

    #[test]
    fn warn_fires_then_cooldown_suppresses() {
        let mut m = CacheMetrics::default();

        // High render utilization should fire.
        let kind = m.check_warn_condition(95.0, 50.0, 0);
        assert!(matches!(kind, Some(CacheWarnKind::HighRenderUtilization(_))));

        // Immediately again: suppressed by cooldown.
        assert!(m.check_warn_condition(95.0, 50.0, 0).is_none());

        // Drain cooldown.
        for _ in 0..WARN_COOLDOWN_CALLS - 1 {
            assert!(m.check_warn_condition(95.0, 50.0, 0).is_none());
        }

        // After cooldown, should fire again.
        assert!(m.check_warn_condition(95.0, 50.0, 0).is_some());
    }

    #[test]
    fn warn_does_not_fire_below_threshold() {
        let mut m = CacheMetrics::default();
        assert!(m.check_warn_condition(80.0, 80.0, 5).is_none());
    }

    #[test]
    fn eviction_spike_triggers_warn() {
        let mut m = CacheMetrics::default();
        let kind = m.check_warn_condition(50.0, 50.0, EVICTION_SPIKE_THRESHOLD);
        assert!(matches!(kind, Some(CacheWarnKind::EvictionSpike(_))));
    }

    #[test]
    fn resize_count_increments() {
        let mut m = CacheMetrics::default();
        assert_eq!(m.resize_count, 0);
        m.record_resize();
        m.record_resize();
        m.record_resize();
        assert_eq!(m.resize_count, 3);
    }

    #[test]
    fn snapshot_utilization_computed_correctly() {
        let budget = RenderCacheBudget {
            max_bytes: 1000,
            last_total_bytes: 500,
            last_evicted_bytes: 0,
            total_evictions: 0,
        };
        let retention_stats =
            HistoryRetentionStats { total_after_bytes: 750, ..Default::default() };
        let policy = HistoryRetentionPolicy { max_bytes: 1000 };
        let metrics = CacheMetrics::default();
        let viewport = ChatViewport::new();

        let snap =
            build_snapshot(&budget, &retention_stats, policy, &metrics, &viewport, 10, 2, 1, 0);

        assert!((snap.render_utilization_pct - 50.0).abs() < 0.01);
        assert!((snap.history_utilization_pct - 75.0).abs() < 0.01);
        assert_eq!(snap.render_entry_count, 10);
        assert_eq!(snap.render_evictions_this_frame, 2);
        assert_eq!(snap.history_dropped_messages_this_pass, 1);
        assert_eq!(snap.render_protected_bytes, 0);
    }

    #[test]
    fn snapshot_zero_budget_no_panic() {
        let budget = RenderCacheBudget {
            max_bytes: 0,
            last_total_bytes: 0,
            last_evicted_bytes: 0,
            total_evictions: 0,
        };
        let policy = HistoryRetentionPolicy { max_bytes: 0 };
        let metrics = CacheMetrics::default();
        let viewport = ChatViewport::new();

        let snap = build_snapshot(
            &budget,
            &HistoryRetentionStats::default(),
            policy,
            &metrics,
            &viewport,
            0,
            0,
            0,
            0,
        );
        assert!(snap.render_utilization_pct.abs() < f32::EPSILON);
        assert!(snap.history_utilization_pct.abs() < f32::EPSILON);
    }
}
