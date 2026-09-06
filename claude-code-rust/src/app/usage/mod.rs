mod cli;
mod oauth;

use crate::agent::events::ClientEvent;
use crate::app::{App, UsageSnapshot, UsageSourceKind, UsageSourceMode, UsageWindow};
use std::time::{Duration, SystemTime};

const USAGE_REFRESH_TTL: Duration = Duration::from_secs(30);

struct UsageRefreshFailure {
    source: UsageSourceKind,
    message: String,
}

pub(crate) fn request_refresh_if_needed(app: &mut App) {
    if app.usage.in_flight {
        return;
    }
    if app.usage.snapshot.as_ref().is_some_and(is_snapshot_fresh) {
        return;
    }
    request_refresh(app);
}

pub(crate) fn request_refresh(app: &mut App) {
    if app.usage.in_flight || tokio::runtime::Handle::try_current().is_err() {
        return;
    }

    apply_refresh_started(app);

    let event_tx = app.event_tx.clone();
    let epoch = app.session_scope_epoch;
    let source_mode = app.usage.active_source;
    let cwd_raw = app.cwd_raw.clone();

    tokio::task::spawn_local(async move {
        let _ = event_tx.send(ClientEvent::UsageRefreshStarted { epoch });
        match refresh_snapshot(source_mode, cwd_raw).await {
            Ok(snapshot) => {
                let _ = event_tx.send(ClientEvent::UsageSnapshotReceived { epoch, snapshot });
            }
            Err(error) => {
                let _ = event_tx.send(ClientEvent::UsageRefreshFailed {
                    epoch,
                    message: error.message,
                    source: error.source,
                });
            }
        }
    });
}

pub(crate) fn apply_refresh_started(app: &mut App) {
    app.usage.in_flight = true;
    app.usage.last_error = None;
    app.usage.last_attempted_source = None;
}

pub(crate) fn apply_refresh_success(app: &mut App, snapshot: UsageSnapshot) {
    app.usage.last_attempted_source = Some(snapshot.source);
    app.usage.snapshot = Some(snapshot);
    app.usage.in_flight = false;
    app.usage.last_error = None;
}

pub(crate) fn apply_refresh_failure(app: &mut App, message: String, source: UsageSourceKind) {
    app.usage.in_flight = false;
    app.usage.last_error = Some(message);
    app.usage.last_attempted_source = Some(source);
}

pub(crate) fn reset_for_session_change(app: &mut App) {
    app.usage.snapshot = None;
    app.usage.in_flight = false;
    app.usage.last_error = None;
    app.usage.last_attempted_source = None;
}

pub(crate) fn visible_windows(snapshot: &UsageSnapshot) -> Vec<&UsageWindow> {
    let mut windows = Vec::new();
    if let Some(window) = snapshot.five_hour.as_ref() {
        windows.push(window);
    }
    if let Some(window) = snapshot.seven_day.as_ref() {
        windows.push(window);
    }
    if let Some(window) = snapshot.seven_day_sonnet.as_ref() {
        windows.push(window);
    }
    if let Some(window) = snapshot.seven_day_opus.as_ref() {
        windows.push(window);
    }
    windows
}

pub(crate) fn format_window_reset(window: &UsageWindow) -> Option<String> {
    if let Some(resets_at) = window.resets_at {
        return Some(format!("resets in {}", format_remaining_until(resets_at)));
    }

    let description = window.reset_description.as_deref()?.trim();
    if description.is_empty() { None } else { Some(description.to_owned()) }
}

fn is_snapshot_fresh(snapshot: &UsageSnapshot) -> bool {
    snapshot.fetched_at.elapsed().is_ok_and(|age| age < USAGE_REFRESH_TTL)
}

fn format_remaining_until(target: SystemTime) -> String {
    let Ok(remaining) = target.duration_since(SystemTime::now()) else {
        return "< 1 minute".to_owned();
    };

    if remaining < Duration::from_secs(60) {
        return "< 1 minute".to_owned();
    }

    let total_minutes = remaining.as_secs() / 60;
    let days = total_minutes / (24 * 60);
    let hours = (total_minutes % (24 * 60)) / 60;
    let minutes = total_minutes % 60;

    if days > 0 {
        return format!("{days}d {hours}h");
    }
    if hours > 0 {
        if minutes == 0 {
            return format!("{hours}h");
        }
        return format!("{hours}h {minutes}m");
    }
    format!("{minutes}m")
}

async fn refresh_snapshot(
    source_mode: UsageSourceMode,
    cwd_raw: String,
) -> Result<UsageSnapshot, UsageRefreshFailure> {
    match source_mode {
        UsageSourceMode::Oauth => oauth::fetch_snapshot().await.map_err(|error| {
            UsageRefreshFailure { source: UsageSourceKind::Oauth, message: error.into_message() }
        }),
        UsageSourceMode::Cli => cli::fetch_snapshot(cwd_raw)
            .await
            .map_err(|message| UsageRefreshFailure { source: UsageSourceKind::Cli, message }),
        UsageSourceMode::Auto => refresh_snapshot_auto(cwd_raw).await,
    }
}

async fn refresh_snapshot_auto(cwd_raw: String) -> Result<UsageSnapshot, UsageRefreshFailure> {
    match oauth::fetch_snapshot().await {
        Ok(snapshot) => Ok(snapshot),
        Err(error) if error.should_fallback_to_cli() => {
            let oauth_message = error.into_message();
            cli::fetch_snapshot(cwd_raw).await.map_err(|message| UsageRefreshFailure {
                source: UsageSourceKind::Cli,
                message: format!(
                    "OAuth unavailable ({oauth_message}). CLI fallback failed: {message}"
                ),
            })
        }
        Err(error) => Err(UsageRefreshFailure {
            source: UsageSourceKind::Oauth,
            message: error.into_message(),
        }),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::app::UsageSourceKind;

    #[test]
    fn formats_day_scale_reset() {
        let target = SystemTime::now() + Duration::from_secs(4 * 24 * 60 * 60 + 12 * 60 * 60);
        let formatted = format_window_reset(&UsageWindow {
            label: "7-day",
            utilization: 50.0,
            resets_at: Some(target),
            reset_description: None,
        })
        .expect("formatted reset");
        assert!(formatted.starts_with("resets in 4d "));
    }

    #[test]
    fn prefers_reset_description_when_no_timestamp_exists() {
        let window = UsageWindow {
            label: "7-day",
            utilization: 40.0,
            resets_at: None,
            reset_description: Some("Resets Feb 12 at 1:30pm (Asia/Calcutta)".to_owned()),
        };
        assert_eq!(
            format_window_reset(&window),
            Some("Resets Feb 12 at 1:30pm (Asia/Calcutta)".to_owned())
        );
    }

    #[test]
    fn collects_only_present_windows() {
        let snapshot = UsageSnapshot {
            source: UsageSourceKind::Oauth,
            fetched_at: SystemTime::now(),
            five_hour: Some(UsageWindow {
                label: "5-hour",
                utilization: 10.0,
                resets_at: None,
                reset_description: None,
            }),
            seven_day: None,
            seven_day_opus: Some(UsageWindow {
                label: "7-day Opus",
                utilization: 30.0,
                resets_at: None,
                reset_description: None,
            }),
            seven_day_sonnet: None,
            extra_usage: None,
        };

        let labels =
            visible_windows(&snapshot).into_iter().map(|window| window.label).collect::<Vec<_>>();
        assert_eq!(labels, vec!["5-hour", "7-day Opus"]);
    }
}
