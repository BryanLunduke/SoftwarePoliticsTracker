// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::App;
use crate::Cli;
use crate::agent::events::ClientEvent;
use reqwest::header::{ACCEPT, HeaderMap, HeaderValue, USER_AGENT};
use serde::{Deserialize, Serialize};
use std::path::{Path, PathBuf};
use std::time::{Duration, SystemTime, UNIX_EPOCH};

const UPDATE_CHECK_DISABLE_ENV: &str = "CLAUDE_RUST_NO_UPDATE_CHECK";
const UPDATE_CHECK_TTL_SECS: u64 = 24 * 60 * 60;
const UPDATE_CHECK_TIMEOUT: Duration = Duration::from_secs(4);
const GITHUB_LATEST_RELEASE_API_URL: &str =
    "https://api.github.com/repos/srothgan/claude-code-rust/releases/latest";
const GITHUB_API_ACCEPT_VALUE: &str = "application/vnd.github+json";
const GITHUB_API_VERSION_VALUE: &str = "2022-11-28";
const GITHUB_USER_AGENT_VALUE: &str = "claude-code-rust-update-check";
const CACHE_FILE: &str = "update-check.json";
const CACHE_DIR_NAME: &str = "claude-code-rust";

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
struct SimpleVersion {
    major: u64,
    minor: u64,
    patch: u64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct UpdateCheckCache {
    checked_at_unix_secs: u64,
    latest_version: String,
}

#[derive(Debug, Clone, Deserialize)]
struct GithubLatestRelease {
    tag_name: String,
}

pub fn start_update_check(app: &App, cli: &Cli) {
    if update_check_disabled(cli.no_update_check) {
        tracing::debug!("Skipping update check (disabled by flag/env)");
        return;
    }

    let event_tx = app.event_tx.clone();
    let current_version = env!("CARGO_PKG_VERSION").to_owned();

    tokio::task::spawn_local(async move {
        let latest_version = resolve_latest_version().await;
        let Some(latest_version) = latest_version else {
            return;
        };

        if is_newer_version(&latest_version, &current_version) {
            let _ = event_tx.send(ClientEvent::UpdateAvailable { latest_version, current_version });
        }
    });
}

fn update_check_disabled(no_update_check_flag: bool) -> bool {
    if no_update_check_flag {
        return true;
    }
    std::env::var(UPDATE_CHECK_DISABLE_ENV)
        .ok()
        .is_some_and(|v| matches!(v.trim().to_ascii_lowercase().as_str(), "1" | "true" | "yes"))
}

async fn resolve_latest_version() -> Option<String> {
    let cache_path = update_cache_path()?;
    let now = unix_now_secs()?;
    let cached = read_cache(&cache_path).await;

    if let Some(cache) = cached.as_ref()
        && now.saturating_sub(cache.checked_at_unix_secs) <= UPDATE_CHECK_TTL_SECS
        && is_valid_version(&cache.latest_version)
    {
        return Some(cache.latest_version.clone());
    }

    match fetch_latest_release_tag().await {
        Some(latest_version) => {
            let cache = UpdateCheckCache { checked_at_unix_secs: now, latest_version };
            if let Err(err) = write_cache(&cache_path, &cache).await {
                tracing::debug!("update-check cache write failed: {err}");
            }
            Some(cache.latest_version)
        }
        None => cached.and_then(|cache| {
            is_valid_version(&cache.latest_version).then_some(cache.latest_version)
        }),
    }
}

fn update_cache_path() -> Option<PathBuf> {
    dirs::cache_dir().map(|dir| dir.join(CACHE_DIR_NAME).join(CACHE_FILE))
}

fn unix_now_secs() -> Option<u64> {
    SystemTime::now().duration_since(UNIX_EPOCH).ok().map(|d| d.as_secs())
}

async fn read_cache(path: &Path) -> Option<UpdateCheckCache> {
    let content = tokio::fs::read_to_string(path).await.ok()?;
    serde_json::from_str::<UpdateCheckCache>(&content).ok()
}

async fn write_cache(path: &Path, cache: &UpdateCheckCache) -> anyhow::Result<()> {
    if let Some(parent) = path.parent() {
        tokio::fs::create_dir_all(parent).await?;
    }
    let content = serde_json::to_vec(cache)?;
    tokio::fs::write(path, content).await?;
    Ok(())
}

async fn fetch_latest_release_tag() -> Option<String> {
    let client = reqwest::Client::builder().timeout(UPDATE_CHECK_TIMEOUT).build().ok()?;

    let response = client
        .get(GITHUB_LATEST_RELEASE_API_URL)
        .headers(github_api_headers())
        .send()
        .await
        .ok()?;

    if !response.status().is_success() {
        tracing::debug!("update-check request failed with status {}", response.status());
        return None;
    }

    let release = response.json::<GithubLatestRelease>().await.ok()?;
    normalize_version_string(&release.tag_name)
}

fn github_api_headers() -> HeaderMap {
    let mut headers = HeaderMap::new();
    headers.insert(ACCEPT, HeaderValue::from_static(GITHUB_API_ACCEPT_VALUE));
    headers.insert("X-GitHub-Api-Version", HeaderValue::from_static(GITHUB_API_VERSION_VALUE));
    headers.insert(USER_AGENT, HeaderValue::from_static(GITHUB_USER_AGENT_VALUE));
    headers
}

fn normalize_version_string(raw: &str) -> Option<String> {
    parse_simple_version(raw).map(|v| format!("{}.{}.{}", v.major, v.minor, v.patch))
}

fn parse_simple_version(raw: &str) -> Option<SimpleVersion> {
    let trimmed = raw.trim();
    let without_prefix = trimmed.strip_prefix('v').unwrap_or(trimmed);
    let core = without_prefix.split_once('-').map_or(without_prefix, |(c, _)| c);

    let mut parts = core.split('.');
    let major = parts.next()?.parse().ok()?;
    let minor = parts.next()?.parse().ok()?;
    let patch = parts.next()?.parse().ok()?;
    if parts.next().is_some() {
        return None;
    }
    Some(SimpleVersion { major, minor, patch })
}

fn is_valid_version(version: &str) -> bool {
    parse_simple_version(version).is_some()
}

fn is_newer_version(candidate: &str, current: &str) -> bool {
    let Some(candidate) = parse_simple_version(candidate) else {
        return false;
    };
    let Some(current) = parse_simple_version(current) else {
        return false;
    };
    candidate > current
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_simple_version_accepts_v_prefix() {
        assert_eq!(
            parse_simple_version("v1.2.3"),
            Some(SimpleVersion { major: 1, minor: 2, patch: 3 })
        );
    }

    #[test]
    fn parse_simple_version_rejects_invalid_shapes() {
        assert_eq!(parse_simple_version("1.2"), None);
        assert_eq!(parse_simple_version("1.2.3.4"), None);
        assert_eq!(parse_simple_version("v1.two.3"), None);
    }

    #[test]
    fn parse_simple_version_ignores_prerelease_suffix() {
        assert_eq!(
            parse_simple_version("v2.4.6-rc1"),
            Some(SimpleVersion { major: 2, minor: 4, patch: 6 })
        );
    }

    #[test]
    fn normalize_version_string_accepts_release_tag() {
        assert_eq!(normalize_version_string("v0.10.0").as_deref(), Some("0.10.0"));
    }

    #[test]
    fn github_release_payload_parses_tag_name() {
        let payload = r#"{"tag_name":"v0.11.0"}"#;
        let parsed = serde_json::from_str::<GithubLatestRelease>(payload).ok();
        assert_eq!(parsed.map(|r| r.tag_name), Some("v0.11.0".to_owned()));
    }

    #[test]
    fn update_check_disabled_prefers_flag() {
        assert!(update_check_disabled(true));
    }

    #[test]
    fn is_newer_version_compares_semver_triplets() {
        assert!(is_newer_version("0.3.0", "0.2.9"));
        assert!(!is_newer_version("0.2.9", "0.3.0"));
        assert!(!is_newer_version("bad", "0.3.0"));
    }
}
