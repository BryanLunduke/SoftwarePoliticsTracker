use super::*;
use crate::agent::model::AvailableModel;
use crate::agent::wire::BridgeCommand;
use crate::app::AppStatus;
use crossterm::event::{Event, KeyCode, KeyEvent, KeyModifiers};
use std::collections::BTreeMap;
use std::path::PathBuf;
use std::rc::Rc;
use tempfile::TempDir;

fn open_settings_test_app() -> (TempDir, App) {
    let dir = tempfile::tempdir().expect("tempdir");
    std::fs::write(
        dir.path().join(".claude.json"),
        format!(
            "{{\n  \"projects\": {{\n    \"{}\": {{ \"hasTrustDialogAccepted\": true }}\n  }}\n}}\n",
            crate::app::trust::store::normalize_project_key(dir.path())
        ),
    )
    .expect("write trust prefs");
    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().to_string_lossy().to_string();
    open(&mut app).expect("open");
    (dir, app)
}

fn select_setting(app: &mut App, setting_id: SettingId) {
    app.config.selected_setting_index =
        setting_specs().iter().position(|spec| spec.id == setting_id).expect("setting row");
}

fn app_with_status_connection()
-> (App, tokio::sync::mpsc::UnboundedReceiver<crate::agent::wire::CommandEnvelope>) {
    let mut app = App::test_default();
    let (tx, rx) = tokio::sync::mpsc::unbounded_channel();
    app.conn = Some(Rc::new(crate::agent::client::AgentConnection::new(tx)));
    app.session_id = Some(crate::agent::model::SessionId::new("session-1"));
    app.config.active_tab = ConfigTab::Status;
    app.recent_sessions = vec![crate::app::RecentSessionInfo {
        session_id: "session-1".to_owned(),
        summary: "Existing session summary".to_owned(),
        last_modified_ms: 0,
        file_size_bytes: 0,
        cwd: Some("/test".to_owned()),
        git_branch: None,
        custom_title: Some("Current custom title".to_owned()),
        first_prompt: Some("First prompt".to_owned()),
    }];
    (app, rx)
}

#[test]
fn open_loads_document_and_switches_view() {
    let dir = tempfile::tempdir().expect("tempdir");
    let path = dir.path().join(".claude").join("settings.json");
    std::fs::create_dir_all(path.parent().expect("settings parent")).expect("create dir");
    std::fs::write(&path, r#"{"fastMode":true}"#).expect("write");
    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().to_string_lossy().to_string();

    open(&mut app).expect("open");

    assert_eq!(app.active_view, ActiveView::Config);
    assert!(matches!(
        resolve_setting_document(&app.config.committed_settings_document, SettingId::FastMode, &[])
            .value,
        ResolvedSettingValue::Bool(true)
    ));
    assert!(app.config.settings_path.is_some());
    assert!(app.config.local_settings_path.is_some());
    assert!(app.config.preferences_path.is_some());
}

#[test]
fn open_does_not_force_stop_active_turn() {
    let dir = tempfile::tempdir().expect("tempdir");
    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().to_string_lossy().to_string();
    app.status = AppStatus::Running;

    open(&mut app).expect("open");

    assert_eq!(app.active_view, ActiveView::Config);
    assert!(matches!(app.status, AppStatus::Running));
    assert!(app.pending_cancel_origin.is_none());
}

#[test]
fn initialize_shared_state_reconciles_trust_from_preferences() {
    let dir = tempfile::tempdir().expect("tempdir");
    let prefs_path = dir.path().join(".claude.json");
    std::fs::write(&prefs_path, r#"{"projects":{}}"#).expect("write prefs");

    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().join("project").to_string_lossy().to_string();
    app.trust.status = crate::app::trust::TrustStatus::Trusted;

    initialize_shared_state(&mut app).expect("initialize");

    assert_eq!(app.trust.status, crate::app::trust::TrustStatus::Untrusted);
    assert_eq!(
        app.trust.project_key,
        crate::app::trust::store::normalize_project_key(std::path::Path::new(&app.cwd_raw))
    );
}

#[test]
fn activate_tab_clears_status_and_error_feedback() {
    let (_dir, mut app) = open_settings_test_app();
    app.config.status_message = Some("saved".into());
    app.config.last_error = Some("failed".into());

    activate_tab(&mut app, ConfigTab::Plugins);

    assert!(app.config.status_message.is_none());
    assert!(app.config.last_error.is_none());
}

#[test]
fn reopen_reload_picks_up_external_settings_changes() {
    let dir = tempfile::tempdir().expect("tempdir");
    let path = dir.path().join(".claude").join("settings.json");
    std::fs::create_dir_all(path.parent().expect("settings parent")).expect("create dir");
    std::fs::write(&path, r#"{"fastMode":false}"#).expect("write");
    std::fs::write(
        dir.path().join(".claude.json"),
        format!(
            "{{\n  \"projects\": {{\n    \"{}\": {{ \"hasTrustDialogAccepted\": true }}\n  }}\n}}\n",
            crate::app::trust::store::normalize_project_key(dir.path())
        ),
    )
    .expect("write trust prefs");
    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().to_string_lossy().to_string();

    open(&mut app).expect("open");
    assert!(!app.config.fast_mode_effective());

    close(&mut app);
    std::fs::write(&path, r#"{"fastMode":true}"#).expect("rewrite");

    open(&mut app).expect("reopen");

    assert!(app.config.fast_mode_effective());
}

#[test]
fn reopen_clears_stale_transient_feedback() {
    let (_dir, mut app) = open_settings_test_app();
    app.config.status_message = Some("stale status".to_owned());
    app.config.last_error = Some("stale error".to_owned());

    close(&mut app);
    open(&mut app).expect("reopen");

    assert!(app.config.status_message.is_none());
    assert!(app.config.last_error.is_none());
}

#[test]
fn space_persists_toggled_fast_mode_immediately() {
    let dir = tempfile::tempdir().expect("tempdir");
    let path = dir.path().join(".claude").join("settings.json");
    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().to_string_lossy().to_string();

    open(&mut app).expect("open");
    select_setting(&mut app, SettingId::FastMode);
    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));

    let raw = std::fs::read_to_string(path).expect("read");
    assert!(raw.contains("\"fastMode\": true"));
    assert!(app.config.last_error.is_none());
}

#[test]
fn handle_key_moves_between_config_rows() {
    let mut app = App::test_default();
    app.active_view = ActiveView::Config;
    let last_index = setting_specs().len().saturating_sub(1);

    handle_key(&mut app, KeyEvent::new(KeyCode::Down, KeyModifiers::NONE));
    assert_eq!(app.config.selected_setting_index, 1);

    for _ in 0..setting_specs().len().saturating_add(4) {
        handle_key(&mut app, KeyEvent::new(KeyCode::Down, KeyModifiers::NONE));
    }

    assert_eq!(app.config.selected_setting_index, last_index);
}

#[test]
fn open_rejects_untrusted_projects() {
    let dir = tempfile::tempdir().expect("tempdir");
    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().to_string_lossy().to_string();
    app.trust.status = crate::app::trust::TrustStatus::Untrusted;

    let err = open(&mut app).expect_err("open should be blocked");

    assert!(err.contains("Project trust"));
    assert_eq!(app.active_view, ActiveView::Chat);
}

#[test]
fn tab_navigation_wraps_and_clears_status_message() {
    let (_dir, mut app) = open_settings_test_app();
    app.config.status_message = Some("saved".to_owned());

    handle_key(&mut app, KeyEvent::new(KeyCode::BackTab, KeyModifiers::SHIFT));

    assert_eq!(app.config.active_tab, ConfigTab::Mcp);
    assert!(app.config.status_message.is_none());

    handle_key(&mut app, KeyEvent::new(KeyCode::Tab, KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Tab, KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Tab, KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Tab, KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Tab, KeyModifiers::NONE));

    assert_eq!(app.config.active_tab, ConfigTab::Mcp);
}

#[test]
fn plugins_tab_uses_arrow_keys_for_inner_navigation() {
    let (_dir, mut app) = open_settings_test_app();
    app.config.active_tab = ConfigTab::Plugins;
    app.config.selected_setting_index = 3;
    app.plugins.installed = vec![
        crate::app::plugins::InstalledPluginEntry {
            id: "frontend-design@claude-plugins-official".to_owned(),
            version: Some("1.0.0".to_owned()),
            scope: "user".to_owned(),
            enabled: true,
            installed_at: None,
            last_updated: None,
            project_path: None,
            capability: crate::app::plugins::PluginCapability::Skill,
        },
        crate::app::plugins::InstalledPluginEntry {
            id: "rust-analyzer-lsp@claude-plugins-official".to_owned(),
            version: Some("1.0.0".to_owned()),
            scope: "user".to_owned(),
            enabled: true,
            installed_at: None,
            last_updated: None,
            project_path: None,
            capability: crate::app::plugins::PluginCapability::Skill,
        },
    ];

    handle_key(&mut app, KeyEvent::new(KeyCode::Down, KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Right, KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Char('r'), KeyModifiers::NONE));

    assert_eq!(app.config.selected_setting_index, 3);
    assert_eq!(app.config.active_tab, ConfigTab::Plugins);
    assert_eq!(app.plugins.installed_selected_index, 1);
    assert_eq!(app.plugins.active_tab, crate::app::plugins::PluginsViewTab::Plugins);
    assert_eq!(app.plugins.installed_search_query, "");
    assert_eq!(app.plugins.plugins_search_query, "");
    assert!(app.config.overlay.is_none());
}

#[test]
fn plugins_inner_tab_switch_does_not_trigger_refresh() {
    let (_dir, mut app) = open_settings_test_app();
    app.config.active_tab = ConfigTab::Plugins;
    app.plugins.loading = false;
    app.plugins.last_inventory_refresh_at = None;

    handle_key(&mut app, KeyEvent::new(KeyCode::Right, KeyModifiers::NONE));

    assert_eq!(app.plugins.active_tab, crate::app::plugins::PluginsViewTab::Plugins);
    assert!(!app.plugins.loading);
}

#[test]
fn installed_plugin_enter_opens_actions_overlay() {
    let (_dir, mut app) = open_settings_test_app();
    app.config.active_tab = ConfigTab::Plugins;
    app.plugins.installed = vec![crate::app::plugins::InstalledPluginEntry {
        id: "frontend-design@claude-plugins-official".to_owned(),
        version: Some("1.0.0".to_owned()),
        scope: "local".to_owned(),
        enabled: true,
        installed_at: None,
        last_updated: None,
        project_path: Some("C:\\work\\project-a".to_owned()),
        capability: crate::app::plugins::PluginCapability::Skill,
    }];
    app.plugins.marketplace = vec![crate::app::plugins::MarketplaceEntry {
        plugin_id: "frontend-design@claude-plugins-official".to_owned(),
        name: "frontend-design".to_owned(),
        description: Some("Create distinctive interfaces".to_owned()),
        marketplace_name: Some("claude-plugins-official".to_owned()),
        version: Some("1.0.0".to_owned()),
        install_count: Some(42),
        source: None,
    }];

    handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));

    let overlay = app.config.installed_plugin_actions_overlay().expect("installed actions overlay");
    assert_eq!(overlay.title, "Frontend Design From Claude Plugins Official");
    assert_eq!(overlay.description, "Create distinctive interfaces");
    assert_eq!(
        overlay.actions,
        vec![
            InstalledPluginActionKind::Disable,
            InstalledPluginActionKind::Update,
            InstalledPluginActionKind::InstallInCurrentProject,
            InstalledPluginActionKind::Uninstall,
        ]
    );
}

#[test]
fn installed_plugin_overlay_uses_up_down_and_escape() {
    let (_dir, mut app) = open_settings_test_app();
    app.config.active_tab = ConfigTab::Plugins;
    app.plugins.installed = vec![crate::app::plugins::InstalledPluginEntry {
        id: "frontend-design@claude-plugins-official".to_owned(),
        version: Some("1.0.0".to_owned()),
        scope: "user".to_owned(),
        enabled: false,
        installed_at: None,
        last_updated: None,
        project_path: None,
        capability: crate::app::plugins::PluginCapability::Skill,
    }];

    handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Down, KeyModifiers::NONE));

    assert_eq!(
        app.config.installed_plugin_actions_overlay().map(|overlay| overlay.selected_index),
        Some(1)
    );

    handle_key(&mut app, KeyEvent::new(KeyCode::Up, KeyModifiers::NONE));

    assert_eq!(
        app.config.installed_plugin_actions_overlay().map(|overlay| overlay.selected_index),
        Some(0)
    );

    handle_key(&mut app, KeyEvent::new(KeyCode::Esc, KeyModifiers::NONE));

    assert!(app.config.overlay.is_none());
}

#[test]
fn plugin_enter_opens_install_overlay() {
    let (_dir, mut app) = open_settings_test_app();
    app.config.active_tab = ConfigTab::Plugins;
    app.plugins.active_tab = crate::app::plugins::PluginsViewTab::Plugins;
    app.plugins.marketplace = vec![crate::app::plugins::MarketplaceEntry {
        plugin_id: "frontend-design@claude-plugins-official".to_owned(),
        name: "frontend-design".to_owned(),
        description: Some("Create distinctive interfaces".to_owned()),
        marketplace_name: Some("claude-plugins-official".to_owned()),
        version: Some("1.0.0".to_owned()),
        install_count: Some(42),
        source: None,
    }];

    handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));

    let overlay = app.config.plugin_install_overlay().expect("Plugin install overlay");
    assert_eq!(overlay.title, "Frontend Design");
    assert_eq!(overlay.description, "Create distinctive interfaces");
    assert_eq!(
        overlay.actions,
        vec![
            PluginInstallActionKind::User,
            PluginInstallActionKind::Project,
            PluginInstallActionKind::Local,
        ]
    );
}

#[test]
fn plugin_install_overlay_uses_up_down_and_escape() {
    let (_dir, mut app) = open_settings_test_app();
    app.config.active_tab = ConfigTab::Plugins;
    app.plugins.active_tab = crate::app::plugins::PluginsViewTab::Plugins;
    app.plugins.marketplace = vec![crate::app::plugins::MarketplaceEntry {
        plugin_id: "frontend-design@claude-plugins-official".to_owned(),
        name: "frontend-design".to_owned(),
        description: Some("Create distinctive interfaces".to_owned()),
        marketplace_name: Some("claude-plugins-official".to_owned()),
        version: Some("1.0.0".to_owned()),
        install_count: Some(42),
        source: None,
    }];

    handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Down, KeyModifiers::NONE));

    assert_eq!(app.config.plugin_install_overlay().map(|overlay| overlay.selected_index), Some(1));

    handle_key(&mut app, KeyEvent::new(KeyCode::Up, KeyModifiers::NONE));

    assert_eq!(app.config.plugin_install_overlay().map(|overlay| overlay.selected_index), Some(0));

    handle_key(&mut app, KeyEvent::new(KeyCode::Esc, KeyModifiers::NONE));

    assert!(app.config.overlay.is_none());
}

#[test]
fn marketplace_enter_opens_actions_overlay_for_configured_marketplace() {
    let (_dir, mut app) = open_settings_test_app();
    app.config.active_tab = ConfigTab::Plugins;
    app.plugins.active_tab = crate::app::plugins::PluginsViewTab::Marketplace;
    app.plugins.marketplaces = vec![crate::app::plugins::MarketplaceSourceEntry {
        name: "claude-plugins-official".to_owned(),
        source: Some("github".to_owned()),
        repo: Some("anthropics/claude-plugins-official".to_owned()),
    }];

    handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));

    let overlay = app.config.marketplace_actions_overlay().expect("marketplace actions overlay");
    assert_eq!(overlay.title, "Claude Plugins Official");
    assert!(overlay.description.contains("Source: github"));
    assert!(overlay.description.contains("Repo: anthropics/claude-plugins-official"));
    assert_eq!(
        overlay.actions,
        vec![
            crate::app::config::MarketplaceActionKind::Update,
            crate::app::config::MarketplaceActionKind::Remove,
        ]
    );
}

#[test]
fn marketplace_add_row_opens_text_input_overlay() {
    let (_dir, mut app) = open_settings_test_app();
    app.config.active_tab = ConfigTab::Plugins;
    app.plugins.active_tab = crate::app::plugins::PluginsViewTab::Marketplace;
    app.plugins.marketplaces = vec![crate::app::plugins::MarketplaceSourceEntry {
        name: "claude-plugins-official".to_owned(),
        source: Some("github".to_owned()),
        repo: Some("anthropics/claude-plugins-official".to_owned()),
    }];

    handle_key(&mut app, KeyEvent::new(KeyCode::Down, KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));

    let overlay = app.config.add_marketplace_overlay().expect("add marketplace overlay");
    assert_eq!(overlay.draft, "");
    assert_eq!(overlay.cursor, 0);
}

#[test]
fn add_marketplace_overlay_supports_editing_and_escape() {
    let (_dir, mut app) = open_settings_test_app();
    app.config.active_tab = ConfigTab::Plugins;
    app.plugins.active_tab = crate::app::plugins::PluginsViewTab::Marketplace;

    handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Char('o'), KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Char('w'), KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Char('n'), KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Left, KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Backspace, KeyModifiers::NONE));

    let overlay = app.config.add_marketplace_overlay().expect("add marketplace overlay");
    assert_eq!(overlay.draft, "on");
    assert_eq!(overlay.cursor, 1);

    handle_key(&mut app, KeyEvent::new(KeyCode::Esc, KeyModifiers::NONE));

    assert!(app.config.overlay.is_none());
}

#[test]
fn add_marketplace_overlay_accepts_paste() {
    let (_dir, mut app) = open_settings_test_app();
    app.config.active_tab = ConfigTab::Plugins;
    app.plugins.active_tab = crate::app::plugins::PluginsViewTab::Marketplace;

    handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));
    crate::app::events::handle_terminal_event(
        &mut app,
        Event::Paste("anthropics/claude-plugins-official".into()),
    );

    let overlay = app.config.add_marketplace_overlay().expect("add marketplace overlay");
    assert_eq!(overlay.draft, "anthropics/claude-plugins-official");
}

#[test]
fn plugins_search_accepts_paste_when_focused() {
    let (_dir, mut app) = open_settings_test_app();
    app.config.active_tab = ConfigTab::Plugins;
    app.plugins.active_tab = crate::app::plugins::PluginsViewTab::Plugins;
    app.plugins.search_focused = true;

    crate::app::events::handle_terminal_event(
        &mut app,
        Event::Paste("frontend-design\nsupabase".into()),
    );

    assert_eq!(app.plugins.plugins_search_query, "frontend-design supabase");
}

#[test]
fn left_and_right_adjust_selected_setting_without_switching_tabs() {
    let (_dir, mut app) = open_settings_test_app();
    select_setting(&mut app, SettingId::FastMode);

    handle_key(&mut app, KeyEvent::new(KeyCode::Right, KeyModifiers::NONE));

    assert_eq!(app.config.active_tab, ConfigTab::Settings);
    assert!(app.config.fast_mode_effective());

    handle_key(&mut app, KeyEvent::new(KeyCode::Left, KeyModifiers::NONE));

    assert_eq!(app.config.active_tab, ConfigTab::Settings);
    assert!(!app.config.fast_mode_effective());
}

#[test]
fn status_tab_r_opens_session_rename_overlay() {
    let (mut app, _rx) = app_with_status_connection();

    handle_key(&mut app, KeyEvent::new(KeyCode::Char('r'), KeyModifiers::NONE));

    assert_eq!(
        app.config.session_rename_overlay().map(|overlay| overlay.draft.as_str()),
        Some("Current custom title")
    );
    assert_eq!(app.config.session_rename_overlay().map(|overlay| overlay.cursor), Some(20));
}

#[tokio::test(flavor = "current_thread")]
async fn activating_usage_tab_starts_refresh_lifecycle() {
    tokio::task::LocalSet::new()
        .run_until(async {
            let (_dir, mut app) = open_settings_test_app();

            activate_tab(&mut app, ConfigTab::Usage);

            assert_eq!(app.config.active_tab, ConfigTab::Usage);
            assert!(app.usage.in_flight);
        })
        .await;
}

#[tokio::test(flavor = "current_thread")]
async fn usage_tab_r_triggers_manual_refresh() {
    tokio::task::LocalSet::new()
        .run_until(async {
            let (_dir, mut app) = open_settings_test_app();
            app.config.active_tab = ConfigTab::Usage;

            handle_key(&mut app, KeyEvent::new(KeyCode::Char('r'), KeyModifiers::NONE));

            assert!(app.usage.in_flight);
        })
        .await;
}

#[test]
fn status_tab_rename_confirm_sends_bridge_command() {
    let (mut app, mut rx) = app_with_status_connection();

    handle_key(&mut app, KeyEvent::new(KeyCode::Char('r'), KeyModifiers::NONE));
    for _ in 0.."Current custom title".chars().count() {
        handle_key(&mut app, KeyEvent::new(KeyCode::Backspace, KeyModifiers::NONE));
    }
    for ch in "Renamed session".chars() {
        handle_key(&mut app, KeyEvent::new(KeyCode::Char(ch), KeyModifiers::NONE));
    }
    handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));

    let envelope = rx.try_recv().expect("rename command");
    assert_eq!(
        envelope.command,
        BridgeCommand::RenameSession {
            session_id: "session-1".to_owned(),
            title: "Renamed session".to_owned(),
        }
    );
    assert!(app.config.overlay.is_none());
    assert_eq!(app.config.status_message.as_deref(), Some("Renaming session..."));
    assert!(app.config.last_error.is_none());
    assert!(matches!(
        app.config.pending_session_title_change.as_ref(),
        Some(pending)
            if pending.session_id == "session-1"
                && matches!(
                    pending.kind,
                    PendingSessionTitleChangeKind::Rename {
                        requested_title: Some(ref requested_title)
                    } if requested_title == "Renamed session"
                )
    ));
}

#[test]
fn status_tab_rename_empty_confirm_clears_custom_title() {
    let (mut app, mut rx) = app_with_status_connection();

    handle_key(&mut app, KeyEvent::new(KeyCode::Char('r'), KeyModifiers::NONE));
    for _ in 0.."Current custom title".chars().count() {
        handle_key(&mut app, KeyEvent::new(KeyCode::Backspace, KeyModifiers::NONE));
    }
    handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));

    let envelope = rx.try_recv().expect("rename command");
    assert_eq!(
        envelope.command,
        BridgeCommand::RenameSession { session_id: "session-1".to_owned(), title: String::new() }
    );
    assert_eq!(app.config.status_message.as_deref(), Some("Clearing session name..."));
    assert!(matches!(
        app.config.pending_session_title_change.as_ref(),
        Some(pending)
            if matches!(
                pending.kind,
                PendingSessionTitleChangeKind::Rename { requested_title: None }
            )
    ));
}

#[test]
fn status_tab_rename_escape_cancels_without_command() {
    let (mut app, mut rx) = app_with_status_connection();

    handle_key(&mut app, KeyEvent::new(KeyCode::Char('r'), KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Char('X'), KeyModifiers::SHIFT));
    handle_key(&mut app, KeyEvent::new(KeyCode::Esc, KeyModifiers::NONE));

    assert!(app.config.overlay.is_none());
    assert!(rx.try_recv().is_err());
    assert!(app.config.pending_session_title_change.is_none());
}

#[test]
fn status_tab_g_generates_session_title_from_current_title_fallback() {
    let (mut app, mut rx) = app_with_status_connection();

    handle_key(&mut app, KeyEvent::new(KeyCode::Char('g'), KeyModifiers::NONE));

    let envelope = rx.try_recv().expect("generate command");
    assert_eq!(
        envelope.command,
        BridgeCommand::GenerateSessionTitle {
            session_id: "session-1".to_owned(),
            description: "Current custom title".to_owned(),
        }
    );
    assert_eq!(app.config.status_message.as_deref(), Some("Generating session title..."));
    assert!(matches!(
        app.config.pending_session_title_change.as_ref(),
        Some(pending)
            if pending.session_id == "session-1"
                && matches!(pending.kind, PendingSessionTitleChangeKind::Generate)
    ));
}

#[test]
fn status_tab_g_requires_existing_session_metadata() {
    let (mut app, mut rx) = app_with_status_connection();
    app.recent_sessions[0].custom_title = None;
    app.recent_sessions[0].summary.clear();
    app.recent_sessions[0].first_prompt = None;

    handle_key(&mut app, KeyEvent::new(KeyCode::Char('g'), KeyModifiers::NONE));

    assert!(rx.try_recv().is_err());
    assert_eq!(
        app.config.last_error.as_deref(),
        Some("No session summary is available to generate a title")
    );
    assert!(app.config.pending_session_title_change.is_none());
}

#[test]
fn overlay_enter_confirms_without_closing_config_screen() {
    let (_dir, mut app) = open_settings_test_app();
    select_setting(&mut app, SettingId::OutputStyle);

    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Down, KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));

    assert_eq!(app.active_view, ActiveView::Config);
    assert!(app.config.overlay.is_none());
    assert_eq!(
        store::output_style(&app.config.committed_local_settings_document),
        Ok(OutputStyle::Explanatory)
    );
}

#[test]
fn always_thinking_toggles_in_settings_document() {
    let (_dir, mut app) = open_settings_test_app();
    select_setting(&mut app, SettingId::AlwaysThinking);

    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));

    assert_eq!(store::always_thinking_enabled(&app.config.committed_settings_document), Ok(true));
}

#[test]
fn reduce_motion_toggles_in_local_settings_document() {
    let (_dir, mut app) = open_settings_test_app();
    select_setting(&mut app, SettingId::ReduceMotion);

    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));

    assert_eq!(
        store::prefers_reduced_motion(&app.config.committed_local_settings_document),
        Ok(true)
    );
}

#[test]
fn show_tips_toggles_in_local_settings_document() {
    let (_dir, mut app) = open_settings_test_app();
    select_setting(&mut app, SettingId::ShowTips);

    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));

    assert_eq!(
        store::spinner_tips_enabled(&app.config.committed_local_settings_document),
        Ok(false)
    );
}

#[test]
fn handle_key_cycles_default_permission_mode() {
    let (_dir, mut app) = open_settings_test_app();
    select_setting(&mut app, SettingId::DefaultPermissionMode);

    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));

    assert_eq!(
        store::default_permission_mode(&app.config.committed_settings_document),
        Ok(DefaultPermissionMode::AcceptEdits)
    );
}

#[test]
fn respect_gitignore_toggles_in_preferences_document() {
    let (_dir, mut app) = open_settings_test_app();
    select_setting(&mut app, SettingId::RespectGitignore);

    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));

    assert_eq!(store::respect_gitignore(&app.config.committed_preferences_document), Ok(false));
}

#[test]
fn terminal_progress_bar_toggles_in_preferences_document() {
    let (_dir, mut app) = open_settings_test_app();
    select_setting(&mut app, SettingId::TerminalProgressBar);

    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));

    assert_eq!(
        store::terminal_progress_bar_enabled(&app.config.committed_preferences_document),
        Ok(false)
    );
}

#[test]
fn immediate_save_respect_gitignore_invalidates_active_mention_session_cache() {
    let dir = tempfile::tempdir().expect("tempdir");
    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().to_string_lossy().to_string();

    open(&mut app).expect("open");
    app.mention = Some(crate::app::mention::MentionState::new(0, 0, "rs".to_owned(), vec![]));
    select_setting(&mut app, SettingId::RespectGitignore);
    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));

    let mention = app.mention.as_ref().expect("mention should stay active");
    assert!(mention.candidates.is_empty());
    assert_eq!(mention.placeholder_message().as_deref(), Some("Searching files..."));
    assert!(!app.config.respect_gitignore_effective());
}

#[test]
fn save_preserves_invalid_unedited_values() {
    let dir = tempfile::tempdir().expect("tempdir");
    let path = dir.path().join(".claude").join("settings.json");
    std::fs::create_dir_all(path.parent().expect("settings parent")).expect("create dir");
    std::fs::write(&path, r#"{"permissions":{"defaultMode":"broken"},"fastMode":false}"#)
        .expect("write");
    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().to_string_lossy().to_string();

    open(&mut app).expect("open");
    select_setting(&mut app, SettingId::FastMode);
    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));

    let raw = std::fs::read_to_string(path).expect("read");
    assert!(raw.contains("\"defaultMode\": \"broken\""));
    assert!(raw.contains("\"fastMode\": true"));
}

#[test]
fn resolved_model_uses_runtime_fallback_when_catalog_rejects_value() {
    let mut app = App::test_default();
    app.available_models = vec![AvailableModel::new("sonnet", "Claude Sonnet")];
    store::set_model(&mut app.config.committed_settings_document, Some("unknown"));

    let resolved = resolved_setting(&app, setting_spec(SettingId::Model));

    assert_eq!(resolved.validation, SettingValidation::UnavailableOption);
    assert_eq!(setting_display_value(&app, setting_spec(SettingId::Model), &resolved), "Default");
}

#[test]
fn model_overlay_options_are_sorted_alphabetically() {
    let mut app = App::test_default();
    app.available_models = vec![
        AvailableModel::new("sonnet", "Sonnet"),
        AvailableModel::new("haiku", "Haiku"),
        AvailableModel::new("opus", "Opus"),
    ];

    let labels = model_overlay_options(&app)
        .into_iter()
        .map(|option| option.display_name)
        .collect::<Vec<_>>();

    assert_eq!(labels, vec!["Default", "Haiku", "Opus", "Sonnet"]);
}

#[test]
fn notifications_cycle_in_preferences_document() {
    let (_dir, mut app) = open_settings_test_app();
    select_setting(&mut app, SettingId::Notifications);

    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));

    assert_eq!(
        store::preferred_notification_channel(&app.config.committed_preferences_document),
        Ok(PreferredNotifChannel::Iterm2WithBell)
    );
}

#[test]
fn theme_cycles_in_preferences_document() {
    let (_dir, mut app) = open_settings_test_app();
    select_setting(&mut app, SettingId::Theme);

    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));

    let stored = store::read_persisted_setting(
        &app.config.committed_preferences_document,
        setting_spec(SettingId::Theme),
    );
    assert_eq!(stored, Ok(store::PersistedSettingValue::String("light".to_owned())));
}

#[test]
fn editor_mode_cycles_in_preferences_document() {
    let (_dir, mut app) = open_settings_test_app();
    select_setting(&mut app, SettingId::EditorMode);

    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));

    let stored = store::read_persisted_setting(
        &app.config.committed_preferences_document,
        setting_spec(SettingId::EditorMode),
    );
    assert_eq!(stored, Ok(store::PersistedSettingValue::String("vim".to_owned())));
}

#[test]
fn output_style_resolves_existing_project_value() {
    let mut app = App::test_default();
    store::set_output_style(
        &mut app.config.committed_local_settings_document,
        OutputStyle::Explanatory,
    );

    let resolved = resolved_setting(&app, setting_spec(SettingId::OutputStyle));

    assert_eq!(resolved.validation, SettingValidation::Valid);
    assert_eq!(
        setting_display_value(&app, setting_spec(SettingId::OutputStyle), &resolved),
        "Explanatory"
    );
}

#[test]
fn output_style_missing_value_falls_back_to_default() {
    let app = App::test_default();

    let resolved = resolved_setting(&app, setting_spec(SettingId::OutputStyle));

    assert_eq!(resolved.validation, SettingValidation::Valid);
    assert_eq!(
        setting_display_value(&app, setting_spec(SettingId::OutputStyle), &resolved),
        "Default"
    );
}

#[test]
fn output_style_invalid_value_uses_default_with_invalid_state() {
    let mut app = App::test_default();
    app.config.committed_local_settings_document = serde_json::json!({ "outputStyle": "Verbose" });

    let resolved = resolved_setting(&app, setting_spec(SettingId::OutputStyle));

    assert_eq!(resolved.validation, SettingValidation::InvalidValue);
    assert_eq!(
        setting_display_value(&app, setting_spec(SettingId::OutputStyle), &resolved),
        "Default"
    );
}

#[test]
fn language_resolves_existing_project_value() {
    let mut app = App::test_default();
    store::set_language(&mut app.config.committed_settings_document, Some("German"));

    let resolved = resolved_setting(&app, setting_spec(SettingId::Language));

    assert_eq!(resolved.validation, SettingValidation::Valid);
    assert_eq!(setting_display_value(&app, setting_spec(SettingId::Language), &resolved), "German");
}

#[test]
fn language_missing_value_displays_not_set() {
    let app = App::test_default();

    let resolved = resolved_setting(&app, setting_spec(SettingId::Language));

    assert_eq!(resolved.validation, SettingValidation::Valid);
    assert_eq!(
        setting_display_value(&app, setting_spec(SettingId::Language), &resolved),
        "Not set"
    );
}

#[test]
fn language_invalid_persisted_length_uses_not_set_with_invalid_state() {
    let mut app = App::test_default();
    app.config.committed_settings_document = serde_json::json!({ "language": "E" });

    let resolved = resolved_setting(&app, setting_spec(SettingId::Language));

    assert_eq!(resolved.validation, SettingValidation::InvalidValue);
    assert_eq!(
        setting_display_value(&app, setting_spec(SettingId::Language), &resolved),
        "Not set"
    );
}

#[test]
fn language_whitespace_only_persisted_value_uses_not_set_with_invalid_state() {
    let mut app = App::test_default();
    app.config.committed_settings_document = serde_json::json!({ "language": "   " });

    let resolved = resolved_setting(&app, setting_spec(SettingId::Language));

    assert_eq!(resolved.validation, SettingValidation::InvalidValue);
    assert_eq!(
        setting_display_value(&app, setting_spec(SettingId::Language), &resolved),
        "Not set"
    );
}

#[test]
fn space_opens_output_style_overlay() {
    let (_dir, mut app) = open_settings_test_app();
    select_setting(&mut app, SettingId::OutputStyle);

    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));

    assert_eq!(
        app.config.output_style_overlay().map(|overlay| overlay.selected),
        Some(OutputStyle::Default)
    );
}

#[test]
fn space_opens_language_overlay_with_existing_value() {
    let (_dir, mut app) = open_settings_test_app();
    store::set_language(&mut app.config.committed_settings_document, Some("German"));
    select_setting(&mut app, SettingId::Language);

    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));

    assert_eq!(app.config.language_overlay().map(|overlay| overlay.draft.as_str()), Some("German"));
    assert_eq!(app.config.language_overlay().map(|overlay| overlay.cursor), Some(6));
}

#[test]
fn space_persists_local_project_settings_immediately() {
    let dir = tempfile::tempdir().expect("tempdir");
    let path = dir.path().join(".claude").join("settings.local.json");
    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().to_string_lossy().to_string();

    open(&mut app).expect("open");
    select_setting(&mut app, SettingId::ReduceMotion);
    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));
    select_setting(&mut app, SettingId::ShowTips);
    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));

    let raw = std::fs::read_to_string(path).expect("read");
    assert!(raw.contains("\"prefersReducedMotion\": true"));
    assert!(raw.contains("\"spinnerTipsEnabled\": false"));
}

#[test]
fn output_style_overlay_confirm_persists_local_setting() {
    let dir = tempfile::tempdir().expect("tempdir");
    let path = dir.path().join(".claude").join("settings.local.json");
    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().to_string_lossy().to_string();

    open(&mut app).expect("open");
    select_setting(&mut app, SettingId::OutputStyle);
    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Down, KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Down, KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));

    let raw = std::fs::read_to_string(path).expect("read");
    assert!(raw.contains("\"outputStyle\": \"Learning\""));
    assert_eq!(
        store::output_style(&app.config.committed_local_settings_document),
        Ok(OutputStyle::Learning)
    );
    assert!(app.config.overlay.is_none());
}

#[test]
fn output_style_overlay_escape_cancels_without_persisting() {
    let dir = tempfile::tempdir().expect("tempdir");
    let path = dir.path().join(".claude").join("settings.local.json");
    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().to_string_lossy().to_string();

    open(&mut app).expect("open");
    select_setting(&mut app, SettingId::OutputStyle);
    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Down, KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Esc, KeyModifiers::NONE));

    assert!(!path.exists());
    assert!(app.config.overlay.is_none());
    assert_eq!(
        store::output_style(&app.config.committed_local_settings_document),
        Ok(OutputStyle::Default)
    );
}

#[test]
fn language_overlay_confirm_persists_project_setting() {
    let dir = tempfile::tempdir().expect("tempdir");
    let path = dir.path().join(".claude").join("settings.json");
    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().to_string_lossy().to_string();

    open(&mut app).expect("open");
    select_setting(&mut app, SettingId::Language);
    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));
    for ch in "German".chars() {
        handle_key(&mut app, KeyEvent::new(KeyCode::Char(ch), KeyModifiers::NONE));
    }
    handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));

    let raw = std::fs::read_to_string(path).expect("read");
    assert!(raw.contains("\"language\": \"German\""));
    assert_eq!(
        store::language(&app.config.committed_settings_document),
        Ok(Some("German".to_owned()))
    );
    assert!(app.config.overlay.is_none());
}

#[test]
fn language_overlay_confirm_trims_project_setting() {
    let dir = tempfile::tempdir().expect("tempdir");
    let path = dir.path().join(".claude").join("settings.json");
    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().to_string_lossy().to_string();

    open(&mut app).expect("open");
    select_setting(&mut app, SettingId::Language);
    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));
    for ch in "German".chars() {
        handle_key(&mut app, KeyEvent::new(KeyCode::Char(ch), KeyModifiers::NONE));
    }
    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));

    let raw = std::fs::read_to_string(path).expect("read");
    assert!(raw.contains("\"language\": \"German\""));
    assert_eq!(
        store::language(&app.config.committed_settings_document),
        Ok(Some("German".to_owned()))
    );
    assert!(app.config.overlay.is_none());
}

#[test]
fn language_overlay_empty_confirm_clears_existing_setting() {
    let dir = tempfile::tempdir().expect("tempdir");
    let path = dir.path().join(".claude").join("settings.json");
    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().to_string_lossy().to_string();
    std::fs::create_dir_all(path.parent().expect("settings parent")).expect("create dir");
    std::fs::write(&path, r#"{"language":"German"}"#).expect("write");

    open(&mut app).expect("open");
    select_setting(&mut app, SettingId::Language);
    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));
    for _ in 0..6 {
        handle_key(&mut app, KeyEvent::new(KeyCode::Backspace, KeyModifiers::NONE));
    }
    handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));

    let raw = std::fs::read_to_string(path).expect("read");
    assert!(!raw.contains("\"language\""));
    assert_eq!(store::language(&app.config.committed_settings_document), Ok(None));
    assert!(app.config.overlay.is_none());
}

#[test]
fn language_overlay_escape_cancels_without_persisting() {
    let dir = tempfile::tempdir().expect("tempdir");
    let path = dir.path().join(".claude").join("settings.json");
    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().to_string_lossy().to_string();

    open(&mut app).expect("open");
    select_setting(&mut app, SettingId::Language);
    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Char('G'), KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Esc, KeyModifiers::NONE));

    assert!(!path.exists());
    assert!(app.config.overlay.is_none());
    assert_eq!(store::language(&app.config.committed_settings_document), Ok(None));
}

#[test]
fn language_overlay_blocks_too_short_input() {
    let dir = tempfile::tempdir().expect("tempdir");
    let path = dir.path().join(".claude").join("settings.json");
    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().to_string_lossy().to_string();

    open(&mut app).expect("open");
    select_setting(&mut app, SettingId::Language);
    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Char('E'), KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));

    assert!(app.config.language_overlay().is_some());
    assert!(!path.exists());
    assert_eq!(store::language(&app.config.committed_settings_document), Ok(None));
}

#[test]
fn language_overlay_blocks_too_long_input() {
    let dir = tempfile::tempdir().expect("tempdir");
    let path = dir.path().join(".claude").join("settings.json");
    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().to_string_lossy().to_string();

    open(&mut app).expect("open");
    select_setting(&mut app, SettingId::Language);
    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));
    for ch in "abcdefghijklmnopqrstuvwxyzabcde".chars() {
        handle_key(&mut app, KeyEvent::new(KeyCode::Char(ch), KeyModifiers::NONE));
    }
    handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));

    assert!(app.config.language_overlay().is_some());
    assert!(!path.exists());
    assert_eq!(store::language(&app.config.committed_settings_document), Ok(None));
}

#[test]
fn language_overlay_supports_cursor_aware_editing() {
    let dir = tempfile::tempdir().expect("tempdir");
    let path = dir.path().join(".claude").join("settings.json");
    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().to_string_lossy().to_string();
    std::fs::create_dir_all(path.parent().expect("settings parent")).expect("create dir");
    std::fs::write(&path, r#"{"language":"German"}"#).expect("write");

    open(&mut app).expect("open");
    select_setting(&mut app, SettingId::Language);
    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Left, KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Left, KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Backspace, KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Char('i'), KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::End, KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Delete, KeyModifiers::NONE));
    handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));

    assert_eq!(
        store::language(&app.config.committed_settings_document),
        Ok(Some("Gerian".to_owned()))
    );
}

#[test]
fn space_persists_always_thinking_in_user_settings() {
    let dir = tempfile::tempdir().expect("tempdir");
    let path = dir.path().join(".claude").join("settings.json");
    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().to_string_lossy().to_string();

    open(&mut app).expect("open");
    select_setting(&mut app, SettingId::AlwaysThinking);
    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));

    let raw = std::fs::read_to_string(path).expect("read");
    assert!(raw.contains("\"alwaysThinkingEnabled\": true"));
}

#[test]
fn space_persists_terminal_progress_bar_in_preferences() {
    let dir = tempfile::tempdir().expect("tempdir");
    let path = dir.path().join(".claude.json");
    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().to_string_lossy().to_string();

    open(&mut app).expect("open");
    select_setting(&mut app, SettingId::TerminalProgressBar);
    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));

    let raw = std::fs::read_to_string(path).expect("read");
    assert!(raw.contains("\"terminalProgressBarEnabled\": false"));
}

#[test]
fn enter_closes_settings_without_editing_selected_row() {
    let (_dir, mut app) = open_settings_test_app();
    select_setting(&mut app, SettingId::FastMode);

    handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));

    assert_eq!(app.active_view, ActiveView::Chat);
    assert!(!app.config.fast_mode_effective());
}

#[test]
fn mcp_enter_opens_details_overlay_instead_of_closing_config() {
    let (_dir, mut app) = open_settings_test_app();
    app.config.active_tab = ConfigTab::Mcp;
    app.session_id = Some(crate::agent::model::SessionId::new("session-1"));
    app.mcp.servers = vec![crate::agent::types::McpServerStatus {
        name: "filesystem".to_owned(),
        status: crate::agent::types::McpServerConnectionStatus::Connected,
        server_info: None,
        error: None,
        config: Some(crate::agent::types::McpServerStatusConfig::Stdio {
            command: "npx".to_owned(),
            args: vec!["@modelcontextprotocol/server-filesystem".to_owned()],
            env: BTreeMap::new(),
        }),
        scope: Some("project".to_owned()),
        tools: vec![],
    }];

    handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));

    assert_eq!(app.active_view, ActiveView::Config);
    assert_eq!(
        app.config.mcp_details_overlay().map(|overlay| overlay.server_name.as_str()),
        Some("filesystem")
    );
}

#[test]
fn mcp_details_overlay_enter_closes_overlay() {
    let (_dir, mut app) = open_settings_test_app();
    app.config.active_tab = ConfigTab::Mcp;
    app.config.overlay = Some(ConfigOverlayState::McpDetails(McpDetailsOverlayState {
        server_name: "filesystem".to_owned(),
        selected_index: 0,
    }));

    handle_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE));

    assert!(app.config.overlay.is_none());
    assert_eq!(app.active_view, ActiveView::Config);
}

#[test]
fn mcp_tab_refresh_key_requests_snapshot() {
    let (_dir, mut app) = open_settings_test_app();
    let (tx, mut rx) = tokio::sync::mpsc::unbounded_channel();
    app.conn = Some(Rc::new(crate::agent::client::AgentConnection::new(tx)));
    app.session_id = Some(crate::agent::model::SessionId::new("session-1"));
    app.config.active_tab = ConfigTab::Mcp;
    app.mcp.servers.push(crate::agent::types::McpServerStatus {
        name: "stale".to_owned(),
        status: crate::agent::types::McpServerConnectionStatus::NeedsAuth,
        server_info: None,
        error: None,
        config: None,
        scope: None,
        tools: Vec::new(),
    });

    handle_key(&mut app, KeyEvent::new(KeyCode::Char('r'), KeyModifiers::NONE));

    let envelope = rx.try_recv().expect("mcp snapshot command");
    assert_eq!(
        envelope.command,
        BridgeCommand::GetMcpSnapshot { session_id: "session-1".to_owned() }
    );
    assert!(app.mcp.in_flight);
    assert!(app.mcp.servers.is_empty());
}

#[test]
fn request_mcp_snapshot_sends_outside_mcp_tab() {
    let (_dir, mut app) = open_settings_test_app();
    let (tx, mut rx) = tokio::sync::mpsc::unbounded_channel();
    app.conn = Some(Rc::new(crate::agent::client::AgentConnection::new(tx)));
    app.session_id = Some(crate::agent::model::SessionId::new("session-1"));
    app.config.active_tab = ConfigTab::Status;

    super::mcp::request_mcp_snapshot(&mut app);

    let envelope = rx.try_recv().expect("mcp snapshot command");
    assert_eq!(
        envelope.command,
        BridgeCommand::GetMcpSnapshot { session_id: "session-1".to_owned() }
    );
    assert!(app.mcp.in_flight);
}

#[test]
fn refresh_mcp_snapshot_clears_existing_servers_before_request() {
    let (_dir, mut app) = open_settings_test_app();
    let (tx, mut rx) = tokio::sync::mpsc::unbounded_channel();
    app.conn = Some(Rc::new(crate::agent::client::AgentConnection::new(tx)));
    app.session_id = Some(crate::agent::model::SessionId::new("session-1"));
    app.mcp.servers.push(crate::agent::types::McpServerStatus {
        name: "stale".to_owned(),
        status: crate::agent::types::McpServerConnectionStatus::Connected,
        server_info: None,
        error: None,
        config: None,
        scope: None,
        tools: Vec::new(),
    });

    refresh_mcp_snapshot(&mut app);

    let envelope = rx.try_recv().expect("mcp snapshot command");
    assert_eq!(
        envelope.command,
        BridgeCommand::GetMcpSnapshot { session_id: "session-1".to_owned() }
    );
    assert!(app.mcp.servers.is_empty());
    assert!(app.mcp.in_flight);
}

#[test]
fn refresh_mcp_snapshot_if_needed_skips_outside_mcp_tab() {
    let (_dir, mut app) = open_settings_test_app();
    let (tx, mut rx) = tokio::sync::mpsc::unbounded_channel();
    app.conn = Some(Rc::new(crate::agent::client::AgentConnection::new(tx)));
    app.session_id = Some(crate::agent::model::SessionId::new("session-1"));
    app.config.active_tab = ConfigTab::Status;

    super::mcp::refresh_mcp_snapshot_if_needed(&mut app);

    assert!(rx.try_recv().is_err());
    assert!(!app.mcp.in_flight);
}

#[test]
fn claudeai_proxy_server_shows_disabled_authenticate_action() {
    let server = crate::agent::types::McpServerStatus {
        name: "claude.ai Google Calendar".to_owned(),
        status: crate::agent::types::McpServerConnectionStatus::NeedsAuth,
        server_info: None,
        error: Some(
            "MCP server requires authentication but no OAuth token is configured.".to_owned(),
        ),
        config: Some(crate::agent::types::McpServerStatusConfig::ClaudeaiProxy {
            url: "https://mcp-proxy.anthropic.com/v1/mcp/server".to_owned(),
            id: "mcpsrv_test".to_owned(),
        }),
        scope: Some("session".to_owned()),
        tools: Vec::new(),
    };

    let actions = available_mcp_actions(&server);

    assert!(actions.contains(&super::mcp::McpServerActionKind::Authenticate));
    assert!(!super::mcp::is_mcp_action_available(
        &server,
        super::mcp::McpServerActionKind::Authenticate
    ));
    assert!(actions.contains(&super::mcp::McpServerActionKind::Reconnect));
}

#[test]
fn esc_closes_settings_without_editing_selected_row() {
    let (_dir, mut app) = open_settings_test_app();
    select_setting(&mut app, SettingId::FastMode);

    handle_key(&mut app, KeyEvent::new(KeyCode::Esc, KeyModifiers::NONE));

    assert_eq!(app.active_view, ActiveView::Chat);
    assert!(!app.config.fast_mode_effective());
}

#[test]
fn save_failure_keeps_previous_value_and_surfaces_error() {
    let dir = tempfile::tempdir().expect("tempdir");
    let mut app = App::test_default();
    app.settings_home_override = Some(dir.path().to_path_buf());
    app.cwd_raw = dir.path().to_string_lossy().to_string();

    open(&mut app).expect("open");
    app.config.settings_path = Some(PathBuf::new());
    select_setting(&mut app, SettingId::FastMode);

    handle_key(&mut app, KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE));

    assert_eq!(app.active_view, ActiveView::Config);
    assert!(!app.config.fast_mode_effective());
    assert!(app.config.last_error.is_some());
    assert!(app.config.status_message.is_none());
}
