pub(crate) mod store;

use super::App;
use super::view::{self, ActiveView};
use crossterm::event::{KeyCode, KeyEvent, KeyModifiers};

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum TrustStatus {
    #[default]
    Trusted,
    Untrusted,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum TrustSelection {
    #[default]
    Yes,
    No,
}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct TrustState {
    pub status: TrustStatus,
    pub selection: TrustSelection,
    pub project_key: String,
    pub last_error: Option<String>,
}

impl TrustState {
    #[must_use]
    pub fn is_trusted(&self) -> bool {
        matches!(self.status, TrustStatus::Trusted)
    }
}

pub fn initialize(app: &mut App) {
    let lookup = store::read_status(
        &app.config.committed_preferences_document,
        std::path::Path::new(&app.cwd_raw),
    );
    app.trust.project_key = lookup.project_key;
    app.trust.status = if lookup.trusted { TrustStatus::Trusted } else { TrustStatus::Untrusted };
    app.trust.selection = TrustSelection::Yes;
    app.trust.last_error = app.config.preferences_path.is_none().then(|| {
        app.config
            .last_error
            .clone()
            .unwrap_or_else(|| "Trust preferences path is not available".to_owned())
    });
    app.startup_connection_requested = app.trust.is_trusted();
    if app.trust.is_trusted() {
        view::set_active_view(app, ActiveView::Chat);
    } else {
        view::set_active_view(app, ActiveView::Trusted);
    }
}

pub fn handle_key(app: &mut App, key: KeyEvent) {
    if is_ctrl_shortcut(key, 'q') || is_ctrl_shortcut(key, 'c') {
        app.should_quit = true;
        return;
    }

    match (key.code, key.modifiers) {
        (KeyCode::Up, KeyModifiers::NONE) => app.trust.selection = TrustSelection::Yes,
        (KeyCode::Down, KeyModifiers::NONE) => app.trust.selection = TrustSelection::No,
        (KeyCode::Enter, KeyModifiers::NONE) => activate_selection(app),
        (KeyCode::Char('y' | 'Y'), KeyModifiers::NONE) => {
            app.trust.selection = TrustSelection::Yes;
            activate_selection(app);
        }
        (KeyCode::Esc | KeyCode::Char('n' | 'N'), KeyModifiers::NONE) => {
            app.trust.selection = TrustSelection::No;
            activate_selection(app);
        }
        _ => {}
    }
}

pub fn accept(app: &mut App) -> Result<(), String> {
    let Some(path) = app.config.preferences_path.clone() else {
        return Err("Trust preferences path is not available".to_owned());
    };

    let mut next_document = app.config.committed_preferences_document.clone();
    app.trust.project_key =
        store::set_trusted(&mut next_document, std::path::Path::new(&app.cwd_raw));
    crate::app::config::store::save(&path, &next_document)?;

    app.config.committed_preferences_document = next_document;
    app.trust.status = TrustStatus::Trusted;
    app.trust.last_error = None;
    app.startup_connection_requested = true;
    view::set_active_view(app, ActiveView::Chat);
    Ok(())
}

pub fn decline(app: &mut App) {
    app.should_quit = true;
}

fn activate_selection(app: &mut App) {
    match app.trust.selection {
        TrustSelection::Yes => {
            if let Err(err) = accept(app) {
                app.trust.last_error = Some(err);
            }
        }
        TrustSelection::No => decline(app),
    }
}

fn is_ctrl_shortcut(key: KeyEvent, ch: char) -> bool {
    matches!(key.code, KeyCode::Char(candidate) if candidate == ch)
        && key.modifiers == KeyModifiers::CONTROL
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    #[test]
    fn initialize_routes_untrusted_projects_to_trusted_view() {
        let mut app = App::test_default();
        app.cwd_raw = if cfg!(windows) {
            r"C:\work\project".to_owned()
        } else {
            "/home/user/work/project".to_owned()
        };
        app.config.preferences_path = Some(std::path::PathBuf::from("prefs.json"));
        app.config.committed_preferences_document = json!({
            "projects": {}
        });

        initialize(&mut app);

        assert_eq!(app.active_view, ActiveView::Trusted);
        assert!(!app.is_project_trusted());
        assert_eq!(app.trust.selection, TrustSelection::Yes);
        assert!(!app.startup_connection_requested);
    }

    #[test]
    fn initialize_allows_trusted_projects_into_chat() {
        let project_path =
            if cfg!(windows) { "C:/work/project" } else { "/home/user/work/project" };

        let mut app = App::test_default();
        app.cwd_raw = project_path.to_owned();
        app.config.preferences_path = Some(std::path::PathBuf::from("prefs.json"));
        let mut prefs = json!({ "projects": {} });
        prefs["projects"][project_path] = json!({
            "hasTrustDialogAccepted": true
        });
        app.config.committed_preferences_document = prefs;

        initialize(&mut app);

        assert_eq!(app.active_view, ActiveView::Chat);
        assert!(app.is_project_trusted());
        assert!(app.startup_connection_requested);
    }

    #[test]
    fn accept_persists_trust_and_switches_to_chat() {
        let dir = tempfile::tempdir().expect("tempdir");
        let path = dir.path().join(".claude.json");
        std::fs::write(&path, "{\n  \"projects\": {}\n}\n").expect("write");

        let mut app = App::test_default();
        app.active_view = ActiveView::Trusted;
        app.cwd_raw = dir.path().join("project").to_string_lossy().to_string();
        app.config.preferences_path = Some(path.clone());
        app.trust.status = TrustStatus::Untrusted;
        app.trust.project_key = store::normalize_project_key(std::path::Path::new(&app.cwd_raw));

        accept(&mut app).expect("accept");

        let raw = std::fs::read_to_string(path).expect("read");
        assert!(raw.contains("\"hasTrustDialogAccepted\": true"));
        assert_eq!(app.active_view, ActiveView::Chat);
        assert!(app.is_project_trusted());
        assert!(app.startup_connection_requested);
    }

    #[test]
    fn handle_key_declines_with_n() {
        let mut app = App::test_default();
        app.active_view = ActiveView::Trusted;

        handle_key(&mut app, KeyEvent::new(KeyCode::Char('n'), KeyModifiers::NONE));

        assert!(app.should_quit);
    }

    #[test]
    fn handle_key_moves_selection_with_up_and_down() {
        let mut app = App::test_default();
        app.active_view = ActiveView::Trusted;
        app.trust.selection = TrustSelection::Yes;

        handle_key(&mut app, KeyEvent::new(KeyCode::Down, KeyModifiers::NONE));
        assert_eq!(app.trust.selection, TrustSelection::No);

        handle_key(&mut app, KeyEvent::new(KeyCode::Up, KeyModifiers::NONE));
        assert_eq!(app.trust.selection, TrustSelection::Yes);
    }

    #[test]
    fn handle_key_enter_declines_when_no_is_selected() {
        let mut app = App::test_default();
        app.active_view = ActiveView::Trusted;
        app.trust.selection = TrustSelection::No;

        handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));

        assert!(app.should_quit);
    }
}
