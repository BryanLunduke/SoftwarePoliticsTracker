// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use crate::agent::client::AgentConnection;
use crate::agent::wire::SessionLaunchSettings;
use crate::app::App;
use crate::app::config::{language_input_validation_message, store};
use serde_json::{Map, Value, json};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum SessionStartReason {
    Startup,
    NewSession,
    Resume,
    Login,
    Logout,
}

pub(crate) fn session_launch_settings_for_reason(
    app: &App,
    reason: SessionStartReason,
) -> SessionLaunchSettings {
    match reason {
        SessionStartReason::Logout => SessionLaunchSettings::default(),
        SessionStartReason::Startup
        | SessionStartReason::NewSession
        | SessionStartReason::Resume
        | SessionStartReason::Login => {
            let language = store::language(&app.config.committed_settings_document)
                .ok()
                .flatten()
                .map(|value| value.trim().to_owned())
                .filter(|value| !value.is_empty())
                .filter(|value| language_input_validation_message(value).is_none());
            SessionLaunchSettings {
                language,
                settings: Some(build_session_settings_object(app)),
                agent_progress_summaries: Some(true),
            }
        }
    }
}

fn build_session_settings_object(app: &App) -> Value {
    let mut settings = Map::new();

    settings.insert(
        "alwaysThinkingEnabled".to_owned(),
        Value::Bool(app.config.always_thinking_effective()),
    );

    if let Some(model) = store::model(&app.config.committed_settings_document).ok().flatten() {
        settings.insert("model".to_owned(), Value::String(model));
    }

    settings.insert(
        "permissions".to_owned(),
        json!({
            "defaultMode": app.config.default_permission_mode_effective().as_stored()
        }),
    );
    settings.insert("fastMode".to_owned(), Value::Bool(app.config.fast_mode_effective()));
    settings.insert(
        "effortLevel".to_owned(),
        Value::String(app.config.thinking_effort_effective().as_stored().to_owned()),
    );
    settings.insert(
        "outputStyle".to_owned(),
        Value::String(app.config.output_style_effective().as_stored().to_owned()),
    );
    settings.insert(
        "spinnerTipsEnabled".to_owned(),
        Value::Bool(
            store::spinner_tips_enabled(&app.config.committed_local_settings_document)
                .unwrap_or(true),
        ),
    );
    settings.insert(
        "terminalProgressBarEnabled".to_owned(),
        Value::Bool(
            store::terminal_progress_bar_enabled(&app.config.committed_preferences_document)
                .unwrap_or(true),
        ),
    );

    Value::Object(settings)
}

pub(crate) fn start_new_session(
    app: &App,
    conn: &AgentConnection,
    reason: SessionStartReason,
) -> anyhow::Result<()> {
    conn.new_session(app.cwd_raw.clone(), session_launch_settings_for_reason(app, reason))
}

pub(crate) fn resume_session(
    app: &App,
    conn: &AgentConnection,
    session_id: String,
) -> anyhow::Result<()> {
    conn.resume_session(
        session_id,
        session_launch_settings_for_reason(app, SessionStartReason::Resume),
    )
}

#[cfg(test)]
mod tests {
    use super::{SessionStartReason, session_launch_settings_for_reason};
    use crate::agent::model::EffortLevel;
    use crate::app::App;
    use crate::app::config::{DefaultPermissionMode, store};

    #[test]
    fn persisted_launch_settings_include_model_and_permission_mode() {
        let mut app = App::test_default();
        store::set_model(&mut app.config.committed_settings_document, Some("haiku"));
        store::set_default_permission_mode(
            &mut app.config.committed_settings_document,
            DefaultPermissionMode::Plan,
        );
        store::set_language(&mut app.config.committed_settings_document, Some("German"));
        store::set_always_thinking_enabled(&mut app.config.committed_settings_document, true);
        store::set_thinking_effort_level(
            &mut app.config.committed_settings_document,
            EffortLevel::High,
        );

        let launch_settings = session_launch_settings_for_reason(&app, SessionStartReason::Startup);

        assert_eq!(launch_settings.language.as_deref(), Some("German"));
        assert_eq!(
            launch_settings.settings,
            Some(serde_json::json!({
                "alwaysThinkingEnabled": true,
                "model": "haiku",
                "permissions": { "defaultMode": "plan" },
                "fastMode": false,
                "effortLevel": "high",
                "outputStyle": "Default",
                "spinnerTipsEnabled": true,
                "terminalProgressBarEnabled": true
            }))
        );
        assert_eq!(launch_settings.agent_progress_summaries, Some(true));
    }

    #[test]
    fn persisted_launch_settings_trim_language_value() {
        let mut app = App::test_default();
        app.config.committed_settings_document = serde_json::json!({ "language": "  German  " });

        let launch_settings = session_launch_settings_for_reason(&app, SessionStartReason::Startup);

        assert_eq!(launch_settings.language.as_deref(), Some("German"));
    }

    #[test]
    fn persisted_launch_settings_default_permission_mode_when_missing() {
        let app = App::test_default();

        let launch_settings =
            session_launch_settings_for_reason(&app, SessionStartReason::NewSession);

        assert_eq!(launch_settings.language, None);
        assert_eq!(
            launch_settings.settings,
            Some(serde_json::json!({
                "alwaysThinkingEnabled": false,
                "permissions": { "defaultMode": "default" },
                "fastMode": false,
                "effortLevel": "medium",
                "outputStyle": "Default",
                "spinnerTipsEnabled": true,
                "terminalProgressBarEnabled": true
            }))
        );
        assert_eq!(launch_settings.agent_progress_summaries, Some(true));
    }

    #[test]
    fn persisted_launch_settings_include_supported_settings_json_without_model_when_unset() {
        let mut app = App::test_default();
        store::set_always_thinking_enabled(&mut app.config.committed_settings_document, true);
        store::set_thinking_effort_level(
            &mut app.config.committed_settings_document,
            EffortLevel::High,
        );
        store::set_fast_mode(&mut app.config.committed_settings_document, true);
        store::set_output_style(
            &mut app.config.committed_local_settings_document,
            crate::app::config::OutputStyle::Learning,
        );
        store::set_spinner_tips_enabled(&mut app.config.committed_local_settings_document, false);
        store::set_terminal_progress_bar_enabled(
            &mut app.config.committed_preferences_document,
            false,
        );

        let launch_settings = session_launch_settings_for_reason(&app, SessionStartReason::Startup);

        assert_eq!(launch_settings.language, None);
        assert_eq!(
            launch_settings.settings,
            Some(serde_json::json!({
                "alwaysThinkingEnabled": true,
                "permissions": { "defaultMode": "default" },
                "fastMode": true,
                "effortLevel": "high",
                "outputStyle": "Learning",
                "spinnerTipsEnabled": false,
                "terminalProgressBarEnabled": false
            }))
        );
        assert_eq!(launch_settings.agent_progress_summaries, Some(true));
    }

    #[test]
    fn persisted_launch_settings_omit_invalid_language_value() {
        let mut app = App::test_default();
        app.config.committed_settings_document = serde_json::json!({ "language": "E" });

        let launch_settings = session_launch_settings_for_reason(&app, SessionStartReason::Startup);

        assert_eq!(launch_settings.language, None);
    }

    #[test]
    fn persisted_launch_settings_omit_whitespace_only_language_value() {
        let mut app = App::test_default();
        app.config.committed_settings_document = serde_json::json!({ "language": "   " });

        let launch_settings = session_launch_settings_for_reason(&app, SessionStartReason::Startup);

        assert_eq!(launch_settings.language, None);
    }

    #[test]
    fn logout_launch_settings_omit_all_overrides() {
        let mut app = App::test_default();
        store::set_model(&mut app.config.committed_settings_document, Some("haiku"));
        store::set_default_permission_mode(
            &mut app.config.committed_settings_document,
            DefaultPermissionMode::Plan,
        );
        store::set_always_thinking_enabled(&mut app.config.committed_settings_document, true);

        let launch_settings = session_launch_settings_for_reason(&app, SessionStartReason::Logout);

        assert!(launch_settings.is_empty());
    }
}
