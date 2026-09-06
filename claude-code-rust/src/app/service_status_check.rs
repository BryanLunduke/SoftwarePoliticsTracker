// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::App;
use crate::agent::events::{ClientEvent, ServiceStatusSeverity};
use serde::Deserialize;
use std::time::Duration;

const SERVICE_STATUS_TIMEOUT: Duration = Duration::from_secs(4);
const STATUSPAGE_SUMMARY_URL: &str = "https://status.claude.com/api/v2/summary.json";

/// Component names we care about. "Claude Code" is the primary component;
/// "Claude API" is included because Claude Code depends on it.
const RELEVANT_COMPONENTS: &[&str] = &["Claude Code", "Claude API (api.anthropic.com)"];

#[derive(Debug, Clone, PartialEq, Eq)]
struct ServiceStatusIssue {
    severity: ServiceStatusSeverity,
    message: String,
}

#[derive(Debug, Clone, Deserialize)]
struct SummaryResponse {
    components: Vec<Component>,
    incidents: Vec<Incident>,
}

#[derive(Debug, Clone, Deserialize)]
struct Component {
    name: String,
    status: String,
}

#[derive(Debug, Clone, Deserialize)]
struct Incident {
    name: String,
    components: Vec<IncidentComponent>,
}

#[derive(Debug, Clone, Deserialize)]
struct IncidentComponent {
    name: String,
}

pub fn start_service_status_check(app: &App) {
    let event_tx = app.event_tx.clone();

    tokio::task::spawn_local(async move {
        let Some(issue) = resolve_service_status_issue().await else {
            return;
        };
        let _ = event_tx
            .send(ClientEvent::ServiceStatus { severity: issue.severity, message: issue.message });
    });
}

async fn resolve_service_status_issue() -> Option<ServiceStatusIssue> {
    let client = reqwest::Client::builder().timeout(SERVICE_STATUS_TIMEOUT).build().ok()?;
    let response = client.get(STATUSPAGE_SUMMARY_URL).send().await.ok()?;
    if !response.status().is_success() {
        tracing::debug!("service-status request failed with status {}", response.status());
        return None;
    }

    let payload = response.json::<SummaryResponse>().await.ok()?;
    classify_summary(&payload)
}

fn is_relevant_component(name: &str) -> bool {
    RELEVANT_COMPONENTS.contains(&name)
}

fn classify_component_status(status: &str) -> Option<ServiceStatusSeverity> {
    match status {
        "operational" | "under_maintenance" => None,
        "major_outage" => Some(ServiceStatusSeverity::Error),
        // degraded_performance, partial_outage, or unknown
        _ => Some(ServiceStatusSeverity::Warning),
    }
}

fn classify_summary(summary: &SummaryResponse) -> Option<ServiceStatusIssue> {
    // Check if any relevant component is degraded
    let worst_severity = summary
        .components
        .iter()
        .filter(|c| is_relevant_component(&c.name))
        .filter_map(|c| classify_component_status(&c.status))
        .max_by_key(|s| match s {
            ServiceStatusSeverity::Warning => 0,
            ServiceStatusSeverity::Error => 1,
        });

    let severity = worst_severity?;

    // Find incidents that affect our relevant components for a better message
    let relevant_incident = summary
        .incidents
        .iter()
        .find(|incident| incident.components.iter().any(|c| is_relevant_component(&c.name)));

    let message = if let Some(incident) = relevant_incident {
        format!("Claude Code status: {}.", incident.name.trim())
    } else {
        "Claude Code status indicates a service disruption.".to_owned()
    };

    Some(ServiceStatusIssue { severity, message })
}

#[cfg(test)]
mod tests {
    use super::*;

    fn component(name: &str, status: &str) -> Component {
        Component { name: name.to_owned(), status: status.to_owned() }
    }

    fn incident(name: &str, component_names: &[&str]) -> Incident {
        Incident {
            name: name.to_owned(),
            components: component_names
                .iter()
                .map(|n| IncidentComponent { name: n.to_string() })
                .collect(),
        }
    }

    fn summary(components: Vec<Component>, incidents: Vec<Incident>) -> SummaryResponse {
        SummaryResponse { components, incidents }
    }

    #[test]
    fn all_operational_is_healthy() {
        let s = summary(
            vec![component("Claude Code", "operational"), component("claude.ai", "operational")],
            vec![],
        );
        assert!(classify_summary(&s).is_none());
    }

    #[test]
    fn only_unrelated_component_degraded_is_healthy() {
        let s = summary(
            vec![
                component("Claude Code", "operational"),
                component("claude.ai", "degraded_performance"),
                component("Claude for Government", "major_outage"),
            ],
            vec![],
        );
        assert!(classify_summary(&s).is_none());
    }

    #[test]
    fn claude_code_degraded_is_warning() {
        let s = summary(vec![component("Claude Code", "degraded_performance")], vec![]);
        let issue = classify_summary(&s).expect("expected issue");
        assert_eq!(issue.severity, ServiceStatusSeverity::Warning);
    }

    #[test]
    fn claude_code_major_outage_is_error() {
        let s = summary(vec![component("Claude Code", "major_outage")], vec![]);
        let issue = classify_summary(&s).expect("expected issue");
        assert_eq!(issue.severity, ServiceStatusSeverity::Error);
    }

    #[test]
    fn claude_api_degraded_triggers_warning() {
        let s = summary(
            vec![
                component("Claude Code", "operational"),
                component("Claude API (api.anthropic.com)", "partial_outage"),
            ],
            vec![],
        );
        let issue = classify_summary(&s).expect("expected issue");
        assert_eq!(issue.severity, ServiceStatusSeverity::Warning);
    }

    #[test]
    fn worst_severity_wins() {
        let s = summary(
            vec![
                component("Claude Code", "degraded_performance"),
                component("Claude API (api.anthropic.com)", "major_outage"),
            ],
            vec![],
        );
        let issue = classify_summary(&s).expect("expected issue");
        assert_eq!(issue.severity, ServiceStatusSeverity::Error);
    }

    #[test]
    fn uses_incident_name_in_message() {
        let s = summary(
            vec![component("Claude Code", "degraded_performance")],
            vec![incident("Elevated errors on Claude Opus 4", &["Claude Code", "claude.ai"])],
        );
        let issue = classify_summary(&s).expect("expected issue");
        assert_eq!(issue.message, "Claude Code status: Elevated errors on Claude Opus 4.");
    }

    #[test]
    fn fallback_message_without_relevant_incident() {
        let s = summary(
            vec![component("Claude Code", "partial_outage")],
            vec![incident("API issue", &["claude.ai"])],
        );
        let issue = classify_summary(&s).expect("expected issue");
        assert_eq!(issue.message, "Claude Code status indicates a service disruption.");
    }

    #[test]
    fn ignores_irrelevant_incident() {
        let s = summary(
            vec![
                component("Claude Code", "operational"),
                component("claude.ai", "degraded_performance"),
            ],
            vec![incident("claude.ai degraded", &["claude.ai"])],
        );
        assert!(classify_summary(&s).is_none());
    }
}
