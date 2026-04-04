// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TurnErrorClass {
    PlanLimit,
    AuthRequired,
    Internal,
    Other,
}

pub fn parse_turn_error_class(tag: &str) -> Option<TurnErrorClass> {
    match tag {
        "plan_limit" => Some(TurnErrorClass::PlanLimit),
        "auth_required" => Some(TurnErrorClass::AuthRequired),
        "internal" => Some(TurnErrorClass::Internal),
        "other" => Some(TurnErrorClass::Other),
        _ => None,
    }
}

pub fn classify_turn_error(input: &str) -> TurnErrorClass {
    let lower = input.to_ascii_lowercase();
    if looks_like_plan_limit_error_lower(&lower) {
        TurnErrorClass::PlanLimit
    } else if looks_like_auth_required_error_lower(&lower) {
        TurnErrorClass::AuthRequired
    } else if looks_like_internal_error_lower(&lower) {
        TurnErrorClass::Internal
    } else {
        TurnErrorClass::Other
    }
}

pub fn looks_like_internal_error(input: &str) -> bool {
    looks_like_internal_error_lower(&input.to_ascii_lowercase())
}

pub fn summarize_internal_error(input: &str) -> String {
    if let Some(summary) = summarize_permission_schema_error(input) {
        return truncate_for_log(&summary);
    }
    if let Some(msg) = extract_xml_tag_value(input, "message") {
        return truncate_for_log(msg);
    }
    if let Some(msg) = extract_json_string_field(input, "message") {
        return truncate_for_log(&msg);
    }
    let fallback = input.lines().find(|line| !line.trim().is_empty()).unwrap_or(input);
    truncate_for_log(fallback.trim())
}

fn looks_like_plan_limit_error_lower(lower: &str) -> bool {
    [
        "rate limit",
        "rate-limit",
        "max turns",
        "max turn",
        "max budget",
        "quota",
        "plan limit",
        "plan-limit",
        "429",
        "too many requests",
        "usage limit",
        "insufficient quota",
    ]
    .iter()
    .any(|needle| lower.contains(needle))
}

fn looks_like_auth_required_error_lower(lower: &str) -> bool {
    [
        "/login",
        "auth required",
        "authentication failed",
        "please log in",
        "login required",
        "not authenticated",
        "unauthorized",
    ]
    .iter()
    .any(|needle| lower.contains(needle))
}

fn looks_like_internal_error_lower(lower: &str) -> bool {
    has_internal_error_keywords(lower)
        || looks_like_json_rpc_error_shape(lower)
        || looks_like_xml_error_shape(lower)
}

fn has_internal_error_keywords(lower: &str) -> bool {
    [
        "internal error",
        "agent sdk",
        "claude-agent-sdk",
        "adapter",
        "bridge",
        "json-rpc",
        "rpc",
        "protocol error",
        "transport",
        "handshake failed",
        "session creation failed",
        "connection closed",
        "event channel closed",
        "tool permission request failed",
        "zoderror",
        "invalid_union",
        "bridge command failed",
        "agent stream failed",
        "agent initialization failed",
    ]
    .iter()
    .any(|needle| lower.contains(needle))
}

fn looks_like_json_rpc_error_shape(lower: &str) -> bool {
    (lower.contains("\"jsonrpc\"") && lower.contains("\"error\""))
        || lower.contains("\"code\":-32603")
        || lower.contains("\"code\": -32603")
}

fn looks_like_xml_error_shape(lower: &str) -> bool {
    let has_error_node = lower.contains("<error") || lower.contains("<fault");
    let has_detail_node = lower.contains("<message>") || lower.contains("<code>");
    has_error_node && has_detail_node
}

fn summarize_permission_schema_error(input: &str) -> Option<String> {
    let lower = input.to_ascii_lowercase();
    if !lower.contains("tool permission request failed") {
        return None;
    }

    let detail = if let Some(msg) = extract_json_string_field(input, "message") {
        msg
    } else {
        input.lines().find(|line| !line.trim().is_empty()).unwrap_or(input).trim().to_owned()
    };

    Some(format!("Tool permission request failed: {detail}"))
}

fn truncate_for_log(input: &str) -> String {
    const LIMIT: usize = 240;
    let mut out = String::new();
    for (i, ch) in input.chars().enumerate() {
        if i >= LIMIT {
            out.push_str("...");
            break;
        }
        out.push(ch);
    }
    out.replace('\n', "\\n")
}

fn extract_xml_tag_value<'a>(input: &'a str, tag: &str) -> Option<&'a str> {
    let lower = input.to_ascii_lowercase();
    let open = format!("<{tag}>");
    let close = format!("</{tag}>");
    let start = lower.find(&open)? + open.len();
    let end = start + lower[start..].find(&close)?;
    let value = input[start..end].trim();
    (!value.is_empty()).then_some(value)
}

fn extract_json_string_field(input: &str, field: &str) -> Option<String> {
    let needle = format!("\"{field}\"");
    let start = input.find(&needle)? + needle.len();
    let rest = input[start..].trim_start();
    let colon_idx = rest.find(':')?;
    let mut chars = rest[colon_idx + 1..].trim_start().chars();
    if chars.next()? != '"' {
        return None;
    }

    let mut escaped = false;
    let mut out = String::new();
    for ch in chars {
        if escaped {
            let mapped = match ch {
                'n' => '\n',
                'r' => '\r',
                't' => '\t',
                '"' => '"',
                '\\' => '\\',
                _ => ch,
            };
            out.push(mapped);
            escaped = false;
            continue;
        }
        match ch {
            '\\' => escaped = true,
            '"' => return Some(out),
            _ => out.push(ch),
        }
    }
    None
}

#[cfg(test)]
mod tests {
    use super::{
        TurnErrorClass, classify_turn_error, looks_like_internal_error, parse_turn_error_class,
        summarize_internal_error,
    };

    #[test]
    fn classifies_plan_limit_errors() {
        assert_eq!(classify_turn_error("HTTP 429 Too Many Requests"), TurnErrorClass::PlanLimit);
        assert_eq!(
            classify_turn_error("turn failed: max budget exceeded"),
            TurnErrorClass::PlanLimit
        );
    }

    #[test]
    fn classifies_auth_required_errors() {
        assert_eq!(
            classify_turn_error("authentication failed: please log in"),
            TurnErrorClass::AuthRequired
        );
    }

    #[test]
    fn classifies_internal_errors() {
        assert_eq!(
            classify_turn_error(
                r#"{"jsonrpc":"2.0","error":{"code":-32603,"message":"internal rpc fault"}}"#
            ),
            TurnErrorClass::Internal
        );
        assert!(looks_like_internal_error(
            "<error><code>-32603</code><message>Adapter process crashed</message></error>"
        ));
    }

    #[test]
    fn classifies_other_errors() {
        assert_eq!(classify_turn_error("turn failed: timeout"), TurnErrorClass::Other);
    }

    #[test]
    fn parses_bridge_turn_error_kind_tags() {
        assert_eq!(parse_turn_error_class("plan_limit"), Some(TurnErrorClass::PlanLimit));
        assert_eq!(parse_turn_error_class("auth_required"), Some(TurnErrorClass::AuthRequired));
        assert_eq!(parse_turn_error_class("internal"), Some(TurnErrorClass::Internal));
        assert_eq!(parse_turn_error_class("other"), Some(TurnErrorClass::Other));
        assert_eq!(parse_turn_error_class("unexpected"), None);
    }

    #[test]
    fn summarize_prefers_permission_schema_error_message() {
        let payload = "Tool permission request failed: ZodError: [{\"message\":\"Invalid input: expected record, received undefined\"}]";
        assert_eq!(
            summarize_internal_error(payload),
            "Tool permission request failed: Invalid input: expected record, received undefined"
        );
    }
}
