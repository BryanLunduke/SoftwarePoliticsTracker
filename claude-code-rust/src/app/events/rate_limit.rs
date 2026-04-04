// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::super::{App, SystemSeverity};
use crate::agent::model;

fn format_rate_limit_type(raw: &str) -> &str {
    match raw {
        "five_hour" => "5-hour",
        "daily" => "daily",
        "minute" => "per-minute",
        "seven_day" => "7-day",
        "seven_day_opus" => "7-day Opus",
        "seven_day_sonnet" => "7-day Sonnet",
        "overage" => "overage",
        other => other,
    }
}

/// Format an epoch timestamp as a countdown and UTC wall-clock: "4h 23m at 14:30 UTC".
fn format_resets_at(epoch_secs: f64) -> String {
    use std::time::{Duration, UNIX_EPOCH};

    let now = std::time::SystemTime::now();

    let countdown = match (UNIX_EPOCH + Duration::from_secs_f64(epoch_secs)).duration_since(now) {
        Ok(d) => {
            let total_secs = d.as_secs();
            if total_secs < 60 {
                "< 1 minute".to_owned()
            } else {
                let hours = total_secs / 3600;
                let minutes = (total_secs % 3600) / 60;
                if hours > 0 { format!("{hours}h {minutes}m") } else { format!("{minutes}m") }
            }
        }
        Err(_) => "now".to_owned(),
    };

    #[allow(clippy::cast_possible_truncation, clippy::cast_sign_loss)]
    let epoch_u64 = epoch_secs.max(0.0) as u64;
    let h = (epoch_u64 % 86400) / 3600;
    let m = (epoch_u64 % 3600) / 60;

    format!("{countdown} at {h:02}:{m:02} UTC")
}

pub(super) fn format_rate_limit_summary(update: &model::RateLimitUpdate) -> String {
    let is_rejected = matches!(update.status, model::RateLimitStatus::Rejected);

    // Intro
    let intro = if is_rejected { "Rate limit reached" } else { "Approaching rate limit" };

    // "you've used 91% of your 5-hour rate limit"
    let usage_part = match (update.utilization, &update.rate_limit_type) {
        (Some(util), Some(rlt)) => {
            format!(
                "you've used {:.0}% of your {} rate limit",
                util * 100.0,
                format_rate_limit_type(rlt),
            )
        }
        (Some(util), None) => format!("you've used {:.0}% of your rate limit", util * 100.0),
        (None, Some(rlt)) => {
            format!("you've hit your {} rate limit", format_rate_limit_type(rlt))
        }
        (None, None) => "you've hit your rate limit".to_owned(),
    };

    let mut message = format!("{intro}, {usage_part}.");

    // Overage hint
    if is_rejected {
        // Rejected: state if overage is in use
        if update.is_using_overage == Some(true) {
            message.push_str(" You are using your overage allowance.");
        }
    } else {
        // Warning: hint that overage is available
        if update.is_using_overage == Some(false) || update.overage_status.is_some() {
            message.push_str(" You can continue using your overage allowance.");
        }
    }

    // Resets in X at HH:MM
    if let Some(resets_at) = update.resets_at {
        use std::fmt::Write;
        let _ = write!(message, " Resets in {}.", format_resets_at(resets_at));
    }

    message
}

pub(super) fn handle_rate_limit_update(app: &mut App, update: &model::RateLimitUpdate) {
    let previous_status = app.last_rate_limit_update.as_ref().map(|existing| existing.status);
    app.last_rate_limit_update = Some(update.clone());

    match update.status {
        model::RateLimitStatus::Allowed => {}
        model::RateLimitStatus::AllowedWarning => {
            if previous_status == Some(model::RateLimitStatus::AllowedWarning) {
                return;
            }
            let summary = format_rate_limit_summary(update);
            super::push_system_message_with_severity(app, Some(SystemSeverity::Warning), &summary);
        }
        model::RateLimitStatus::Rejected => {
            let summary = format_rate_limit_summary(update);
            super::push_system_message_with_severity(app, None, &summary);
        }
    }
}

pub(super) fn handle_compaction_boundary_update(
    app: &mut App,
    boundary: model::CompactionBoundary,
) {
    app.is_compacting = true;
    if matches!(boundary.trigger, model::CompactionTrigger::Manual) {
        app.pending_compact_clear = true;
    }
    app.session_usage.last_compaction_trigger = Some(boundary.trigger);
    app.session_usage.last_compaction_pre_tokens = Some(boundary.pre_tokens);
    tracing::debug!(
        "CompactionBoundary: trigger={:?} pre_tokens={}",
        boundary.trigger,
        boundary.pre_tokens
    );
}
