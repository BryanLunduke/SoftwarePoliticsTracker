// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use std::path::{Path, PathBuf};
use std::time::{Duration, SystemTime, UNIX_EPOCH};

#[derive(Debug, Clone, PartialEq, Eq)]
pub(crate) struct ClaudeOAuthCredentials {
    pub access_token: String,
    pub expires_at: Option<SystemTime>,
}

/// Resolved path to `~/.claude/.credentials.json`.
pub(crate) fn credentials_path() -> Option<PathBuf> {
    dirs::home_dir().map(|h| h.join(".claude").join(".credentials.json"))
}

pub(crate) fn load_oauth_credentials() -> Option<ClaudeOAuthCredentials> {
    let path = credentials_path()?;
    load_oauth_credentials_at(&path)
}

/// Returns `true` when valid OAuth credentials exist on disk.
///
/// Reads `~/.claude/.credentials.json` and checks that
/// `claudeAiOauth.accessToken` is a non-empty string.
pub fn has_credentials() -> bool {
    load_oauth_credentials().is_some()
}

fn load_oauth_credentials_at(path: &Path) -> Option<ClaudeOAuthCredentials> {
    let contents = std::fs::read_to_string(path).ok()?;
    let json = serde_json::from_str::<serde_json::Value>(&contents).ok()?;
    parse_oauth_credentials(&json)
}

fn parse_oauth_credentials(json: &serde_json::Value) -> Option<ClaudeOAuthCredentials> {
    let oauth = json.get("claudeAiOauth")?;
    let access_token = oauth.get("accessToken")?.as_str()?.trim();
    if access_token.is_empty() {
        return None;
    }

    Some(ClaudeOAuthCredentials {
        access_token: access_token.to_owned(),
        expires_at: oauth.get("expiresAt").and_then(parse_timestamp_value),
    })
}

fn parse_timestamp_value(value: &serde_json::Value) -> Option<SystemTime> {
    match value {
        serde_json::Value::Number(number) => number
            .as_i64()
            .or_else(|| number.as_u64().and_then(|raw| i64::try_from(raw).ok()))
            .and_then(system_time_from_epoch),
        serde_json::Value::String(raw) => {
            raw.trim().parse::<i64>().ok().and_then(system_time_from_epoch)
        }
        _ => None,
    }
}

fn system_time_from_epoch(raw: i64) -> Option<SystemTime> {
    if raw < 0 {
        return None;
    }

    let raw = u64::try_from(raw).ok()?;
    if raw >= 1_000_000_000_000 {
        Some(UNIX_EPOCH + Duration::from_millis(raw))
    } else {
        Some(UNIX_EPOCH + Duration::from_secs(raw))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Write;

    #[test]
    fn returns_false_for_nonexistent_file() {
        let path = std::path::Path::new("/tmp/claude_test_nonexistent_credentials.json");
        assert!(load_oauth_credentials_at(path).is_none());
    }

    #[test]
    fn returns_false_for_empty_json() {
        let mut tmp = tempfile::NamedTempFile::new().unwrap();
        write!(tmp, "{{}}").unwrap();
        assert!(load_oauth_credentials_at(tmp.path()).is_none());
    }

    #[test]
    fn returns_false_for_empty_access_token() {
        let mut tmp = tempfile::NamedTempFile::new().unwrap();
        write!(tmp, r#"{{"claudeAiOauth":{{"accessToken":"","refreshToken":"tok"}}}}"#).unwrap();
        assert!(load_oauth_credentials_at(tmp.path()).is_none());
    }

    #[test]
    fn returns_true_for_valid_oauth() {
        let mut tmp = tempfile::NamedTempFile::new().unwrap();
        write!(
            tmp,
            r#"{{"claudeAiOauth":{{"accessToken":"sk-ant-oat01-test","refreshToken":"sk-ant-ort01-test","expiresAt":9999999999999}}}}"#
        )
        .unwrap();
        assert!(load_oauth_credentials_at(tmp.path()).is_some());
    }

    #[test]
    fn parses_expiry_timestamp_in_milliseconds() {
        let mut tmp = tempfile::NamedTempFile::new().unwrap();
        write!(tmp, r#"{{"claudeAiOauth":{{"accessToken":"token","expiresAt":1}}}}"#).unwrap();

        let credentials = load_oauth_credentials_at(tmp.path()).unwrap();
        assert_eq!(credentials.access_token, "token");
        assert_eq!(credentials.expires_at, Some(UNIX_EPOCH + Duration::from_secs(1)));
    }

    #[test]
    fn returns_false_for_malformed_json() {
        let mut tmp = tempfile::NamedTempFile::new().unwrap();
        write!(tmp, "not json at all").unwrap();
        assert!(load_oauth_credentials_at(tmp.path()).is_none());
    }
}
