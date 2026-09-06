use crate::app::auth;
use crate::app::{ExtraUsage, UsageSnapshot, UsageSourceKind, UsageWindow};
use reqwest::header::{ACCEPT, AUTHORIZATION, CONTENT_TYPE, HeaderMap, HeaderValue, USER_AGENT};
use serde::Deserialize;
use std::time::{Duration, SystemTime, UNIX_EPOCH};

const OAUTH_USAGE_URL: &str = "https://api.anthropic.com/api/oauth/usage";
const OAUTH_BETA_HEADER: &str = "oauth-2025-04-20";
const OAUTH_TIMEOUT: Duration = Duration::from_secs(8);
const FALLBACK_USER_AGENT: &str = "claude-code/unknown";

#[derive(Debug)]
pub(super) enum OauthFetchError {
    Unavailable(String),
    Unauthorized(String),
    Failed(String),
}

impl OauthFetchError {
    pub(super) fn should_fallback_to_cli(&self) -> bool {
        matches!(self, Self::Unavailable(_) | Self::Unauthorized(_))
    }

    pub(super) fn into_message(self) -> String {
        match self {
            Self::Unavailable(message) | Self::Unauthorized(message) | Self::Failed(message) => {
                message
            }
        }
    }
}

#[derive(Debug, Deserialize)]
struct OAuthUsagePayload {
    five_hour: Option<OAuthUsageWindowPayload>,
    seven_day: Option<OAuthUsageWindowPayload>,
    seven_day_oauth_apps: Option<OAuthUsageWindowPayload>,
    seven_day_opus: Option<OAuthUsageWindowPayload>,
    seven_day_sonnet: Option<OAuthUsageWindowPayload>,
    iguana_necktie: Option<OAuthUsageWindowPayload>,
    extra_usage: Option<OAuthExtraUsagePayload>,
}

#[derive(Debug, Deserialize)]
struct OAuthUsageWindowPayload {
    utilization: Option<f64>,
    resets_at: Option<serde_json::Value>,
}

#[derive(Debug, Deserialize)]
struct OAuthExtraUsagePayload {
    is_enabled: Option<bool>,
    monthly_limit: Option<f64>,
    used_credits: Option<f64>,
    utilization: Option<f64>,
    currency: Option<String>,
}

pub(super) async fn fetch_snapshot() -> Result<UsageSnapshot, OauthFetchError> {
    let credentials = auth::load_oauth_credentials().ok_or_else(|| {
        OauthFetchError::Unavailable(
            "No Claude OAuth credentials found. Run /login to authenticate.".to_owned(),
        )
    })?;

    if credentials.expires_at.is_some_and(|expires_at| expires_at <= SystemTime::now()) {
        return Err(OauthFetchError::Unavailable(
            "Claude OAuth credentials expired. Run /login to refresh them.".to_owned(),
        ));
    }

    let client = reqwest::Client::builder()
        .timeout(OAUTH_TIMEOUT)
        .default_headers(oauth_headers(&credentials.access_token)?)
        .build()
        .map_err(|error| {
            OauthFetchError::Failed(format!("Failed to create OAuth client: {error}"))
        })?;

    let response =
        client.get(OAUTH_USAGE_URL).send().await.map_err(|error| {
            OauthFetchError::Failed(format!("Claude OAuth network error: {error}"))
        })?;

    let status = response.status();
    let body = response.bytes().await.map_err(|error| {
        OauthFetchError::Failed(format!("Failed to read Claude OAuth usage response: {error}"))
    })?;

    match status.as_u16() {
        200 => decode_usage_payload(&body),
        401 | 403 => Err(OauthFetchError::Unauthorized(
            "Claude OAuth usage request was rejected. Run /login to refresh Claude credentials."
                .to_owned(),
        )),
        _ => Err(OauthFetchError::Failed(format!(
            "Claude OAuth usage request failed with HTTP {}{}",
            status.as_u16(),
            truncated_body_suffix(&body),
        ))),
    }
}

fn oauth_headers(access_token: &str) -> Result<HeaderMap, OauthFetchError> {
    let mut headers = HeaderMap::new();
    headers.insert(ACCEPT, HeaderValue::from_static("application/json"));
    headers.insert(CONTENT_TYPE, HeaderValue::from_static("application/json"));
    headers.insert("anthropic-beta", HeaderValue::from_static(OAUTH_BETA_HEADER));
    headers.insert(USER_AGENT, HeaderValue::from_static(FALLBACK_USER_AGENT));
    let token = HeaderValue::from_str(&format!("Bearer {access_token}")).map_err(|error| {
        OauthFetchError::Failed(format!("Invalid OAuth bearer token header: {error}"))
    })?;
    headers.insert(AUTHORIZATION, token);
    Ok(headers)
}

fn decode_usage_payload(body: &[u8]) -> Result<UsageSnapshot, OauthFetchError> {
    let payload = serde_json::from_slice::<OAuthUsagePayload>(body).map_err(|error| {
        OauthFetchError::Failed(format!("Failed to decode Claude OAuth usage response: {error}"))
    })?;

    let five_hour = map_window(payload.five_hour, "5-hour");
    if five_hour.is_none() {
        return Err(OauthFetchError::Failed(
            "Claude OAuth usage response did not include the current session window.".to_owned(),
        ));
    }

    let _ = payload.seven_day_oauth_apps;
    let _ = payload.iguana_necktie;

    Ok(UsageSnapshot {
        source: UsageSourceKind::Oauth,
        fetched_at: SystemTime::now(),
        five_hour,
        seven_day: map_window(payload.seven_day, "7-day"),
        seven_day_opus: map_window(payload.seven_day_opus, "7-day Opus"),
        seven_day_sonnet: map_window(payload.seven_day_sonnet, "7-day Sonnet"),
        extra_usage: map_extra_usage(payload.extra_usage),
    })
}

fn map_window(
    payload: Option<OAuthUsageWindowPayload>,
    label: &'static str,
) -> Option<UsageWindow> {
    let payload = payload?;
    let utilization = payload.utilization?;
    Some(UsageWindow {
        label,
        utilization: utilization.clamp(0.0, 100.0),
        resets_at: payload.resets_at.as_ref().and_then(parse_timestamp_value),
        reset_description: None,
    })
}

fn map_extra_usage(payload: Option<OAuthExtraUsagePayload>) -> Option<ExtraUsage> {
    let payload = payload?;
    if payload.is_enabled == Some(false) {
        return None;
    }

    Some(ExtraUsage {
        monthly_limit: payload.monthly_limit.map(|value| value / 100.0),
        used_credits: payload.used_credits.map(|value| value / 100.0),
        utilization: payload.utilization.map(|value| value.clamp(0.0, 100.0)),
        currency: payload.currency,
    })
}

fn truncated_body_suffix(body: &[u8]) -> String {
    let text = String::from_utf8_lossy(body).trim().replace('\n', " ");
    if text.is_empty() {
        return String::new();
    }

    let shortened = if text.chars().count() > 200 {
        let mut out = text.chars().take(200).collect::<String>();
        out.push_str("...");
        out
    } else {
        text
    };
    format!(": {shortened}")
}

fn parse_timestamp_value(value: &serde_json::Value) -> Option<SystemTime> {
    match value {
        serde_json::Value::Number(number) => number
            .as_i64()
            .or_else(|| number.as_u64().and_then(|raw| i64::try_from(raw).ok()))
            .and_then(system_time_from_epoch),
        serde_json::Value::String(raw) => parse_iso8601_timestamp(raw)
            .or_else(|| raw.trim().parse::<i64>().ok().and_then(system_time_from_epoch)),
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

fn parse_iso8601_timestamp(raw: &str) -> Option<SystemTime> {
    let trimmed = raw.trim();
    let (date_part, time_part) = trimmed.split_once('T').or_else(|| trimmed.split_once(' '))?;

    let mut date_iter = date_part.split('-');
    let year = date_iter.next()?.parse::<i32>().ok()?;
    let month = date_iter.next()?.parse::<u32>().ok()?;
    let day = date_iter.next()?.parse::<u32>().ok()?;

    let (time_only, offset_seconds) = split_time_and_offset(time_part)?;
    let mut time_iter = time_only.split(':');
    let hour = time_iter.next()?.parse::<u32>().ok()?;
    let minute = time_iter.next()?.parse::<u32>().ok()?;
    let second_and_fraction = time_iter.next().unwrap_or("0");
    let (second_raw, fraction_raw) =
        second_and_fraction.split_once('.').unwrap_or((second_and_fraction, ""));
    let second = second_raw.parse::<u32>().ok()?;

    let mut nanos = 0u32;
    let mut factor = 100_000_000u32;
    for ch in fraction_raw.chars().take(9) {
        let digit = ch.to_digit(10)?;
        nanos = nanos.saturating_add(digit.saturating_mul(factor));
        if factor == 0 {
            break;
        }
        factor /= 10;
    }

    let days = days_from_civil(year, month, day)?;
    let day_seconds =
        i64::from(hour) * 60 * 60 + i64::from(minute) * 60 + i64::from(second) - offset_seconds;
    let unix_seconds = days.checked_mul(86_400)?.checked_add(day_seconds)?;
    if unix_seconds < 0 {
        return None;
    }

    Some(
        UNIX_EPOCH
            + Duration::from_secs(u64::try_from(unix_seconds).ok()?)
            + Duration::from_nanos(u64::from(nanos)),
    )
}

fn split_time_and_offset(raw: &str) -> Option<(&str, i64)> {
    if let Some(time_only) = raw.strip_suffix('Z') {
        return Some((time_only, 0));
    }

    let sign_index = raw
        .char_indices()
        .skip(1)
        .find(|(_, ch)| *ch == '+' || *ch == '-')
        .map(|(index, _)| index)?;
    let (time_only, offset_raw) = raw.split_at(sign_index);
    let sign = if offset_raw.starts_with('-') { -1 } else { 1 };
    let offset_raw = &offset_raw[1..];
    let mut parts = offset_raw.split(':');
    let hours = parts.next()?.parse::<i64>().ok()?;
    let minutes = parts.next().unwrap_or("0").parse::<i64>().ok()?;
    Some((time_only, sign * (hours * 60 * 60 + minutes * 60)))
}

fn days_from_civil(year: i32, month: u32, day: u32) -> Option<i64> {
    if !(1..=12).contains(&month) || !(1..=31).contains(&day) {
        return None;
    }

    let mut year = i64::from(year);
    let month = i64::from(month);
    let day = i64::from(day);
    year -= i64::from(month <= 2);
    let era = if year >= 0 { year } else { year - 399 } / 400;
    let yoe = year - era * 400;
    let doy = (153 * (month + if month > 2 { -3 } else { 9 }) + 2) / 5 + day - 1;
    let doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    Some(era * 146_097 + doe - 719_468)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn decodes_sparse_oauth_payload() {
        let snapshot = decode_usage_payload(
            br#"{
                "five_hour": { "utilization": 12.5, "resets_at": "2025-12-25T12:00:00.000Z" },
                "seven_day_sonnet": { "utilization": 5 },
                "unknown_field": true
            }"#,
        )
        .expect("snapshot");

        assert_eq!(snapshot.five_hour.as_ref().map(|window| window.utilization), Some(12.5));
        assert_eq!(snapshot.seven_day_sonnet.as_ref().map(|window| window.utilization), Some(5.0));
        assert!(snapshot.seven_day.is_none());
    }

    #[test]
    fn decodes_extra_usage_amounts_in_major_units() {
        let snapshot = decode_usage_payload(
            br#"{
                "five_hour": { "utilization": 1, "resets_at": "2025-12-25T12:00:00.000Z" },
                "extra_usage": {
                    "is_enabled": true,
                    "monthly_limit": 2000,
                    "used_credits": 1240,
                    "utilization": 62,
                    "currency": "USD"
                }
            }"#,
        )
        .expect("snapshot");

        let extra = snapshot.extra_usage.expect("extra usage");
        assert_eq!(extra.monthly_limit, Some(20.0));
        assert_eq!(extra.used_credits, Some(12.4));
        assert_eq!(extra.utilization, Some(62.0));
        assert_eq!(extra.currency.as_deref(), Some("USD"));
    }

    #[test]
    fn parses_iso8601_timestamp() {
        let parsed = parse_iso8601_timestamp("2025-12-25T12:00:00.000Z").expect("timestamp");
        assert!(parsed > UNIX_EPOCH);
    }

    #[test]
    fn parses_numeric_millisecond_timestamp() {
        let parsed =
            parse_timestamp_value(&serde_json::json!(1_735_128_000_000_i64)).expect("timestamp");
        assert!(parsed > UNIX_EPOCH);
    }
}
