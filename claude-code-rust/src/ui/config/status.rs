// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::theme;
use crate::app::App;
use ratatui::Frame;
use ratatui::layout::{Margin, Rect};
use ratatui::style::{Color, Modifier, Style};
use ratatui::text::{Line, Span};
use ratatui::widgets::{Paragraph, Wrap};

pub(super) fn render(frame: &mut Frame, area: Rect, app: &App) {
    let lines = status_lines(app);
    frame.render_widget(
        Paragraph::new(lines).wrap(Wrap { trim: false }),
        area.inner(Margin { vertical: 1, horizontal: 2 }),
    );
}

pub(crate) fn status_lines(app: &App) -> Vec<Line<'static>> {
    let mut lines = Vec::new();

    // ---- Session ----
    section_header(&mut lines, "Session");
    kv_line(&mut lines, "Version", env!("CARGO_PKG_VERSION"));
    kv_line(&mut lines, "Session name", &derive_session_name(app));

    let session_id_str = app
        .session_id
        .as_ref()
        .map_or_else(|| "(none)".to_owned(), std::string::ToString::to_string);
    kv_line(&mut lines, "Session ID", &session_id_str);

    kv_line(&mut lines, "cwd", &app.cwd);

    if let Some(ref branch) = app.git_branch {
        kv_line(&mut lines, "Git branch", branch);
    }

    lines.push(Line::default());

    // ---- Account ----
    if let Some(ref account) = app.account_info {
        section_header(&mut lines, "Account");
        kv_line(&mut lines, "Login method", &login_method_label(account));
        if let Some(ref org) = account.organization
            && !org.is_empty()
        {
            kv_line(&mut lines, "Organization", org);
        }
        if let Some(ref email) = account.email
            && !email.is_empty()
        {
            kv_line(&mut lines, "Email", email);
        }
        if let Some(ref sub) = account.subscription_type
            && !sub.is_empty()
        {
            kv_line(&mut lines, "Subscription", sub);
        }
        lines.push(Line::default());
    }

    // ---- Model ----
    section_header(&mut lines, "Model");
    kv_line(&mut lines, "Model", &model_display(app));

    if let Some(ref mode) = app.mode {
        kv_line(&mut lines, "Mode", &mode.current_mode_name);
    }

    lines.push(Line::default());

    // ---- Settings ----
    section_header(&mut lines, "Settings");

    let memory_path = resolve_memory_path(app);
    kv_line(&mut lines, "Memory", &memory_path);

    let sources = setting_sources(app);
    kv_line(&mut lines, "Setting sources", &sources);

    lines
}

fn section_header(lines: &mut Vec<Line<'static>>, title: &str) {
    lines.push(Line::from(Span::styled(
        title.to_owned(),
        Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD),
    )));
}

fn kv_line(lines: &mut Vec<Line<'static>>, key: &str, value: &str) {
    lines.push(Line::from(vec![
        Span::styled(format!("  {key}: "), Style::default().fg(theme::DIM)),
        Span::styled(value.to_owned(), Style::default().fg(Color::White)),
    ]));
}

fn derive_session_name(app: &App) -> String {
    if let Some(ref sid) = app.session_id {
        let sid_str = sid.to_string();
        if let Some(session) = app.recent_sessions.iter().find(|s| s.session_id == sid_str) {
            if let Some(ref title) = session.custom_title
                && !title.trim().is_empty()
            {
                return title.clone();
            }
            if !session.summary.trim().is_empty() {
                let summary = &session.summary;
                return if summary.len() > 60 {
                    format!("{}...", &summary[..57])
                } else {
                    summary.clone()
                };
            }
            if let Some(ref prompt) = session.first_prompt
                && !prompt.trim().is_empty()
            {
                return if prompt.len() > 60 {
                    format!("{}...", &prompt[..57])
                } else {
                    prompt.clone()
                };
            }
        }
    }
    "(unnamed session)".to_owned()
}

fn model_display(app: &App) -> String {
    if app.model_name.is_empty() {
        return "(not set)".to_owned();
    }
    if let Some(model) = app.available_models.iter().find(|m| m.id == app.model_name) {
        let mut label = model.display_name.clone();
        if let Some(ref desc) = model.description {
            label.push_str(" - ");
            label.push_str(desc);
        }
        return label;
    }
    app.model_name.clone()
}

pub(crate) fn login_method_label(account: &crate::agent::types::AccountInfo) -> String {
    if let Some(ref source) = account.api_key_source {
        match source.as_str() {
            "oauth" => return "Claude Max Account".to_owned(),
            "user" => return "User API key".to_owned(),
            "project" => return "Project API key".to_owned(),
            "org" => return "Organization API key".to_owned(),
            "temporary" => return "Temporary key".to_owned(),
            other if !other.is_empty() => return other.to_owned(),
            _ => {}
        }
    }
    if let Some(ref source) = account.token_source
        && !source.is_empty()
    {
        return source.clone();
    }
    "Unknown".to_owned()
}

fn resolve_memory_path(app: &App) -> String {
    let Some(home) = dirs::home_dir() else {
        return "(unable to resolve home directory)".to_owned();
    };
    let encoded = encode_project_path(&app.cwd_raw);
    let memory_md =
        home.join(".claude").join("projects").join(&encoded).join("memory").join("MEMORY.md");

    if memory_md.exists() {
        format!("auto memory ({})", memory_md.display())
    } else {
        "(no memory file found)".to_owned()
    }
}

pub(crate) fn encode_project_path(cwd: &str) -> String {
    cwd.replace(['/', '\\'], "-").replace(':', "-").trim_start_matches('-').to_owned()
}

fn setting_sources(app: &App) -> String {
    let mut sources = Vec::new();
    if app.config.settings_path.is_some() {
        sources.push("User settings");
    }
    if app.config.local_settings_path.is_some() {
        sources.push("Project local settings");
    }
    if app.config.preferences_path.is_some() {
        sources.push("Preferences");
    }
    if sources.is_empty() { "(none loaded)".to_owned() } else { sources.join(", ") }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn status_lines_contains_version() {
        let app = App::test_default();
        let text = lines_to_string(&status_lines(&app));
        assert!(text.contains(env!("CARGO_PKG_VERSION")));
    }

    #[test]
    fn status_lines_shows_cwd() {
        let mut app = App::test_default();
        app.cwd = "/test/project".to_owned();
        let text = lines_to_string(&status_lines(&app));
        assert!(text.contains("/test/project"));
    }

    #[test]
    fn status_lines_shows_model() {
        let app = App::test_default();
        let text = lines_to_string(&status_lines(&app));
        assert!(text.contains(&app.model_name) || text.contains("(not set)"));
    }

    #[test]
    fn status_lines_unnamed_session_fallback() {
        let app = App::test_default();
        let text = lines_to_string(&status_lines(&app));
        assert!(text.contains("(unnamed session)"));
    }

    #[test]
    fn status_lines_uses_custom_title() {
        let mut app = App::test_default();
        app.session_id = Some(crate::agent::model::SessionId::new("test-sess-1"));
        app.recent_sessions = vec![crate::app::RecentSessionInfo {
            session_id: "test-sess-1".to_owned(),
            summary: String::new(),
            last_modified_ms: 0,
            file_size_bytes: 0,
            cwd: None,
            git_branch: None,
            custom_title: Some("My Custom Title".to_owned()),
            first_prompt: None,
        }];
        let text = lines_to_string(&status_lines(&app));
        assert!(text.contains("My Custom Title"));
    }

    #[test]
    fn section_headers_present() {
        let app = App::test_default();
        let text = lines_to_string(&status_lines(&app));
        assert!(text.contains("Session"));
        assert!(text.contains("Model"));
        assert!(text.contains("Settings"));
    }

    #[test]
    fn encode_project_path_unix() {
        assert_eq!(encode_project_path("/home/user/project"), "home-user-project");
    }

    #[test]
    fn encode_project_path_windows() {
        assert_eq!(
            encode_project_path("C:\\Users\\User\\Desktop\\project"),
            "C--Users-User-Desktop-project"
        );
    }

    #[test]
    fn login_method_maps_oauth() {
        let account = crate::agent::types::AccountInfo {
            api_key_source: Some("oauth".to_owned()),
            ..Default::default()
        };
        assert_eq!(login_method_label(&account), "Claude Max Account");
    }

    #[test]
    fn login_method_maps_user_key() {
        let account = crate::agent::types::AccountInfo {
            api_key_source: Some("user".to_owned()),
            ..Default::default()
        };
        assert_eq!(login_method_label(&account), "User API key");
    }

    #[test]
    fn login_method_falls_back_to_unknown() {
        let account = crate::agent::types::AccountInfo::default();
        assert_eq!(login_method_label(&account), "Unknown");
    }

    fn lines_to_string(lines: &[Line<'_>]) -> String {
        lines.iter().map(std::string::ToString::to_string).collect::<Vec<_>>().join("\n")
    }
}
