mod cli;

use crate::agent::events::ClientEvent;
use crate::app::App;
use crate::app::config::{
    AddMarketplaceOverlayState, ConfigOverlayState, InstalledPluginActionKind,
    InstalledPluginActionOverlayState, MarketplaceActionKind, MarketplaceActionsOverlayState,
    PluginInstallActionKind, PluginInstallOverlayState,
};
use crossterm::event::{KeyCode, KeyEvent, KeyModifiers};
use serde_json::Value;
use std::path::PathBuf;
use std::time::{Duration, Instant};

const INVENTORY_REFRESH_TTL: Duration = Duration::from_secs(5);

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum PluginsViewTab {
    #[default]
    Installed,
    Plugins,
    Marketplace,
}

impl PluginsViewTab {
    pub const ALL: [Self; 3] = [Self::Installed, Self::Plugins, Self::Marketplace];

    #[must_use]
    pub const fn title(self) -> &'static str {
        match self {
            Self::Installed => "Installed",
            Self::Plugins => "Plugins",
            Self::Marketplace => "Marketplace",
        }
    }

    #[must_use]
    pub const fn next(self) -> Self {
        match self {
            Self::Installed => Self::Plugins,
            Self::Plugins => Self::Marketplace,
            Self::Marketplace => Self::Installed,
        }
    }

    #[must_use]
    pub const fn prev(self) -> Self {
        match self {
            Self::Installed => Self::Marketplace,
            Self::Plugins => Self::Installed,
            Self::Marketplace => Self::Plugins,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PluginCapability {
    Skill,
    Mcp,
}

impl PluginCapability {
    #[must_use]
    pub const fn label(self) -> &'static str {
        match self {
            Self::Skill => "SKILL",
            Self::Mcp => "MCP",
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct InstalledPluginEntry {
    pub id: String,
    pub version: Option<String>,
    pub scope: String,
    pub enabled: bool,
    pub installed_at: Option<String>,
    pub last_updated: Option<String>,
    pub project_path: Option<String>,
    pub capability: PluginCapability,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MarketplaceEntry {
    pub plugin_id: String,
    pub name: String,
    pub description: Option<String>,
    pub marketplace_name: Option<String>,
    pub version: Option<String>,
    pub install_count: Option<u64>,
    pub source: Option<Value>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MarketplaceSourceEntry {
    pub name: String,
    pub source: Option<String>,
    pub repo: Option<String>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct PluginsInventorySnapshot {
    pub installed: Vec<InstalledPluginEntry>,
    pub marketplace: Vec<MarketplaceEntry>,
    pub marketplaces: Vec<MarketplaceSourceEntry>,
}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct PluginsState {
    pub active_tab: PluginsViewTab,
    pub search_focused: bool,
    pub installed_search_query: String,
    pub plugins_search_query: String,
    pub installed_selected_index: usize,
    pub plugins_selected_index: usize,
    pub marketplace_selected_index: usize,
    pub installed: Vec<InstalledPluginEntry>,
    pub marketplace: Vec<MarketplaceEntry>,
    pub marketplaces: Vec<MarketplaceSourceEntry>,
    pub loading: bool,
    pub status_message: Option<String>,
    pub last_error: Option<String>,
    pub last_inventory_refresh_at: Option<Instant>,
    pub claude_path: Option<PathBuf>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct PluginsCliActionSuccess {
    pub snapshot: PluginsInventorySnapshot,
    pub message: String,
    pub claude_path: PathBuf,
}

impl PluginsState {
    #[must_use]
    pub fn selected_index_for(&self, tab: PluginsViewTab) -> usize {
        match tab {
            PluginsViewTab::Installed => self.installed_selected_index,
            PluginsViewTab::Plugins => self.plugins_selected_index,
            PluginsViewTab::Marketplace => self.marketplace_selected_index,
        }
    }

    pub fn set_selected_index_for(&mut self, tab: PluginsViewTab, index: usize) {
        match tab {
            PluginsViewTab::Installed => self.installed_selected_index = index,
            PluginsViewTab::Plugins => self.plugins_selected_index = index,
            PluginsViewTab::Marketplace => self.marketplace_selected_index = index,
        }
    }

    pub fn clear_feedback(&mut self) {
        self.status_message = None;
        self.last_error = None;
    }

    #[must_use]
    pub fn search_query_for(&self, tab: PluginsViewTab) -> &str {
        match tab {
            PluginsViewTab::Installed => &self.installed_search_query,
            PluginsViewTab::Plugins => &self.plugins_search_query,
            PluginsViewTab::Marketplace => "",
        }
    }

    pub fn active_search_query_mut(&mut self) -> Option<&mut String> {
        match self.active_tab {
            PluginsViewTab::Installed => Some(&mut self.installed_search_query),
            PluginsViewTab::Plugins => Some(&mut self.plugins_search_query),
            PluginsViewTab::Marketplace => None,
        }
    }
}

pub(crate) fn handle_paste(app: &mut App, text: &str) -> bool {
    if !search_enabled(app.plugins.active_tab) || !app.plugins.search_focused {
        return false;
    }
    let normalized = normalize_single_line_input(text);
    if normalized.is_empty() {
        return false;
    }
    if let Some(query) = app.plugins.active_search_query_mut() {
        query.push_str(&normalized);
        reset_selection_for_active_tab(app);
        return true;
    }
    false
}

pub(crate) fn handle_key(app: &mut App, key: KeyEvent) -> bool {
    match (key.code, key.modifiers) {
        (KeyCode::Left, KeyModifiers::NONE) => {
            app.plugins.active_tab = app.plugins.active_tab.prev();
            app.plugins.search_focused = false;
            clamp_selection(app);
            true
        }
        (KeyCode::Right, KeyModifiers::NONE) => {
            app.plugins.active_tab = app.plugins.active_tab.next();
            app.plugins.search_focused = false;
            clamp_selection(app);
            true
        }
        (KeyCode::Up, KeyModifiers::NONE) => {
            if search_enabled(app.plugins.active_tab)
                && !app.plugins.search_focused
                && app.plugins.selected_index_for(app.plugins.active_tab) == 0
            {
                app.plugins.search_focused = true;
            } else if !app.plugins.search_focused {
                move_selection(app, -1);
            }
            true
        }
        (KeyCode::Down, KeyModifiers::NONE) => {
            if app.plugins.search_focused {
                app.plugins.search_focused = false;
            } else {
                move_selection(app, 1);
            }
            true
        }
        (KeyCode::Enter, KeyModifiers::NONE) => {
            if app.plugins.search_focused {
                false
            } else {
                match app.plugins.active_tab {
                    PluginsViewTab::Installed => open_installed_actions_overlay(app),
                    PluginsViewTab::Plugins => open_plugin_install_overlay(app),
                    PluginsViewTab::Marketplace => open_marketplace_overlay(app),
                }
            }
        }
        (KeyCode::Backspace, KeyModifiers::NONE) => {
            if search_enabled(app.plugins.active_tab)
                && app.plugins.search_focused
                && let Some(query) = app.plugins.active_search_query_mut()
                && query.pop().is_some()
            {
                reset_selection_for_active_tab(app);
            }
            true
        }
        (KeyCode::Delete, KeyModifiers::NONE) => {
            if search_enabled(app.plugins.active_tab)
                && app.plugins.search_focused
                && let Some(query) = app.plugins.active_search_query_mut()
                && !query.is_empty()
            {
                query.clear();
                reset_selection_for_active_tab(app);
            }
            true
        }
        (KeyCode::Char(ch), modifiers)
            if modifiers.is_empty() || modifiers == KeyModifiers::SHIFT =>
        {
            if search_enabled(app.plugins.active_tab)
                && app.plugins.search_focused
                && let Some(query) = app.plugins.active_search_query_mut()
            {
                query.push(ch);
                reset_selection_for_active_tab(app);
            }
            true
        }
        _ => false,
    }
}

pub(crate) fn request_inventory_refresh_if_needed(app: &mut App) {
    if app.plugins.loading {
        return;
    }
    if app
        .plugins
        .last_inventory_refresh_at
        .is_some_and(|refreshed_at| refreshed_at.elapsed() < INVENTORY_REFRESH_TTL)
    {
        clamp_selection(app);
        return;
    }
    request_inventory_refresh(app);
}

pub(crate) fn request_inventory_refresh(app: &mut App) {
    if tokio::runtime::Handle::try_current().is_err() {
        return;
    }
    app.plugins.loading = true;
    app.plugins.clear_feedback();
    app.plugins.status_message = Some("Refreshing plugin inventory...".to_owned());
    app.needs_redraw = true;
    let event_tx = app.event_tx.clone();
    let cwd_context = app.cwd_raw.clone();
    let cwd_raw = app.cwd_raw.clone();
    let cached_claude_path = app.plugins.claude_path.clone();
    tokio::task::spawn_local(async move {
        match cli::refresh_inventory(cwd_raw, cached_claude_path).await {
            Ok((snapshot, claude_path)) => {
                let _ = event_tx.send(crate::agent::events::ClientEvent::PluginsInventoryUpdated {
                    cwd_raw: cwd_context,
                    snapshot,
                    claude_path,
                });
            }
            Err(message) => {
                let _ = event_tx.send(
                    crate::agent::events::ClientEvent::PluginsInventoryRefreshFailed {
                        cwd_raw: cwd_context,
                        message,
                    },
                );
            }
        }
    });
}

pub(crate) fn apply_inventory_refresh_success(
    app: &mut App,
    snapshot: PluginsInventorySnapshot,
    claude_path: PathBuf,
) {
    app.plugins.installed = snapshot.installed;
    app.plugins.marketplace = snapshot.marketplace;
    app.plugins.marketplaces = snapshot.marketplaces;
    app.plugins.loading = false;
    app.plugins.last_error = None;
    app.plugins.last_inventory_refresh_at = Some(Instant::now());
    app.plugins.claude_path = Some(claude_path);
    app.plugins.status_message = Some("Plugin inventory refreshed".to_owned());
    clamp_selection(app);
}

pub(crate) fn apply_inventory_refresh_failure(app: &mut App, message: String) {
    app.plugins.loading = false;
    app.plugins.status_message = None;
    app.plugins.last_error = Some(message);
}

pub(crate) fn reset_for_session_change(app: &mut App) {
    app.plugins.loading = false;
    app.plugins.status_message = None;
    app.plugins.last_error = None;
    app.plugins.last_inventory_refresh_at = None;
    app.plugins.installed.clear();
    app.plugins.marketplace.clear();
    app.plugins.marketplaces.clear();
    app.plugins.claude_path = None;
    clamp_selection(app);
}

pub(crate) fn clamp_selection(app: &mut App) {
    let installed_len = filtered_installed(&app.plugins).len();
    let plugin_len = filtered_marketplace_plugins(&app.plugins).len();
    let marketplace_len = marketplace_row_count(&app.plugins);
    app.plugins.installed_selected_index =
        clamp_index(app.plugins.installed_selected_index, installed_len);
    app.plugins.plugins_selected_index =
        clamp_index(app.plugins.plugins_selected_index, plugin_len);
    app.plugins.marketplace_selected_index =
        clamp_index(app.plugins.marketplace_selected_index, marketplace_len);
}

#[must_use]
pub(crate) fn filtered_installed(state: &PluginsState) -> Vec<&InstalledPluginEntry> {
    state
        .installed
        .iter()
        .filter(|entry| {
            installed_entry_matches(entry, state.search_query_for(PluginsViewTab::Installed))
        })
        .collect()
}

#[must_use]
pub(crate) fn ordered_installed<'a>(
    state: &'a PluginsState,
    current_project_raw: &str,
) -> Vec<&'a InstalledPluginEntry> {
    let current_project = normalize_project_path(current_project_raw);
    let mut relevant = Vec::new();
    let mut other = Vec::new();

    for entry in filtered_installed(state) {
        if is_relevant_installed_entry(entry, &current_project) {
            relevant.push(entry);
        } else {
            other.push(entry);
        }
    }

    relevant.extend(other);
    relevant
}

#[must_use]
pub(crate) fn relevant_installed_count(state: &PluginsState, current_project_raw: &str) -> usize {
    let current_project = normalize_project_path(current_project_raw);
    filtered_installed(state)
        .into_iter()
        .filter(|entry| is_relevant_installed_entry(entry, &current_project))
        .count()
}

#[must_use]
pub(crate) fn filtered_marketplace_plugins(state: &PluginsState) -> Vec<&MarketplaceEntry> {
    state
        .marketplace
        .iter()
        .filter(|entry| {
            marketplace_plugin_matches(entry, state.search_query_for(PluginsViewTab::Plugins))
        })
        .collect()
}

#[must_use]
pub(crate) fn visible_marketplaces(state: &PluginsState) -> Vec<&MarketplaceSourceEntry> {
    state.marketplaces.iter().collect()
}

#[must_use]
pub(crate) fn display_label(raw: &str) -> String {
    let normalized = raw.replace('@', " from ").replace('-', " ");
    let mut result = String::with_capacity(normalized.len());
    let mut capitalize_next = true;

    for ch in normalized.chars() {
        if ch == ' ' {
            capitalize_next = true;
            result.push(ch);
            continue;
        }

        if capitalize_next {
            result.extend(ch.to_uppercase());
            capitalize_next = false;
        } else {
            result.extend(ch.to_lowercase());
        }
    }

    result
}

pub(crate) fn handle_installed_overlay_key(app: &mut App, key: KeyEvent) {
    match (key.code, key.modifiers) {
        (KeyCode::Esc, KeyModifiers::NONE) => app.config.overlay = None,
        (KeyCode::Up, KeyModifiers::NONE) => move_installed_overlay_selection(app, -1),
        (KeyCode::Down, KeyModifiers::NONE) => move_installed_overlay_selection(app, 1),
        (KeyCode::Enter, KeyModifiers::NONE) => execute_selected_installed_overlay_action(app),
        _ => {}
    }
}

pub(crate) fn handle_plugin_install_overlay_key(app: &mut App, key: KeyEvent) {
    match (key.code, key.modifiers) {
        (KeyCode::Esc, KeyModifiers::NONE) => app.config.overlay = None,
        (KeyCode::Up, KeyModifiers::NONE) => move_plugin_install_overlay_selection(app, -1),
        (KeyCode::Down, KeyModifiers::NONE) => move_plugin_install_overlay_selection(app, 1),
        (KeyCode::Enter, KeyModifiers::NONE) => execute_selected_plugin_install_action(app),
        _ => {}
    }
}

pub(crate) fn handle_marketplace_overlay_key(app: &mut App, key: KeyEvent) {
    match (key.code, key.modifiers) {
        (KeyCode::Esc, KeyModifiers::NONE) => app.config.overlay = None,
        (KeyCode::Up, KeyModifiers::NONE) => move_marketplace_overlay_selection(app, -1),
        (KeyCode::Down, KeyModifiers::NONE) => move_marketplace_overlay_selection(app, 1),
        (KeyCode::Enter, KeyModifiers::NONE) => execute_selected_marketplace_action(app),
        _ => {}
    }
}

pub(crate) fn handle_add_marketplace_overlay_key(app: &mut App, key: KeyEvent) {
    match (key.code, key.modifiers) {
        (KeyCode::Enter, KeyModifiers::NONE) => confirm_add_marketplace_overlay(app),
        (KeyCode::Esc, KeyModifiers::NONE) => app.config.overlay = None,
        (KeyCode::Left, KeyModifiers::NONE) => {
            move_add_marketplace_cursor_left(app);
        }
        (KeyCode::Right, KeyModifiers::NONE) => {
            move_add_marketplace_cursor_right(app);
        }
        (KeyCode::Home, KeyModifiers::NONE) => set_add_marketplace_cursor(app, 0),
        (KeyCode::End, KeyModifiers::NONE) => move_add_marketplace_cursor_to_end(app),
        (KeyCode::Backspace, KeyModifiers::NONE) => delete_add_marketplace_before_cursor(app),
        (KeyCode::Delete, KeyModifiers::NONE) => delete_add_marketplace_at_cursor(app),
        (KeyCode::Char(ch), modifiers)
            if modifiers.is_empty() || modifiers == KeyModifiers::SHIFT =>
        {
            insert_add_marketplace_char(app, ch);
        }
        _ => {}
    }
}

fn open_marketplace_overlay(app: &mut App) -> bool {
    if selected_add_marketplace_row(app) {
        open_add_marketplace_overlay(app)
    } else {
        open_marketplace_actions_overlay(app)
    }
}

fn open_installed_actions_overlay(app: &mut App) -> bool {
    let selected = selected_installed_entry(app).cloned();
    let Some(entry) = selected else {
        return false;
    };

    let title = display_label(&entry.id);
    let description = installed_overlay_description(app, &entry);
    let actions = installed_overlay_actions(app, &entry);
    app.config.overlay =
        Some(ConfigOverlayState::InstalledPluginActions(InstalledPluginActionOverlayState {
            plugin_id: entry.id,
            title,
            description,
            scope: entry.scope,
            project_path: entry.project_path,
            selected_index: 0,
            actions,
        }));
    true
}

fn open_plugin_install_overlay(app: &mut App) -> bool {
    let selected = selected_marketplace_plugin(app).cloned();
    let Some(entry) = selected else {
        return false;
    };

    app.config.overlay =
        Some(ConfigOverlayState::PluginInstallActions(PluginInstallOverlayState {
            plugin_id: entry.plugin_id,
            title: display_label(&entry.name),
            description: entry
                .description
                .unwrap_or_else(|| "Install this plugin into Claude Code.".to_owned()),
            selected_index: 0,
            actions: vec![
                PluginInstallActionKind::User,
                PluginInstallActionKind::Project,
                PluginInstallActionKind::Local,
            ],
        }));
    true
}

fn open_marketplace_actions_overlay(app: &mut App) -> bool {
    let selected = selected_marketplace_source(app).cloned();
    let Some(entry) = selected else {
        return false;
    };

    app.config.overlay =
        Some(ConfigOverlayState::MarketplaceActions(MarketplaceActionsOverlayState {
            name: entry.name.clone(),
            title: display_label(&entry.name),
            description: marketplace_overlay_description(&entry),
            selected_index: 0,
            actions: vec![MarketplaceActionKind::Update, MarketplaceActionKind::Remove],
        }));
    true
}

fn open_add_marketplace_overlay(app: &mut App) -> bool {
    app.config.overlay = Some(ConfigOverlayState::AddMarketplace(
        AddMarketplaceOverlayState::from_text_input(String::new(), 0),
    ));
    app.config.last_error = None;
    true
}

fn move_installed_overlay_selection(app: &mut App, delta: isize) {
    let Some(overlay) = app.config.installed_plugin_actions_overlay_mut() else {
        return;
    };
    let len = overlay.actions.len();
    if len == 0 {
        overlay.selected_index = 0;
        return;
    }
    let current = overlay.selected_index;
    overlay.selected_index = if delta.is_negative() {
        current.saturating_sub(delta.unsigned_abs())
    } else {
        current.saturating_add(delta.cast_unsigned()).min(len.saturating_sub(1))
    };
}

fn move_plugin_install_overlay_selection(app: &mut App, delta: isize) {
    let Some(overlay) = app.config.plugin_install_overlay_mut() else {
        return;
    };
    let len = overlay.actions.len();
    if len == 0 {
        overlay.selected_index = 0;
        return;
    }
    let current = overlay.selected_index;
    overlay.selected_index = if delta.is_negative() {
        current.saturating_sub(delta.unsigned_abs())
    } else {
        current.saturating_add(delta.cast_unsigned()).min(len.saturating_sub(1))
    };
}

fn move_marketplace_overlay_selection(app: &mut App, delta: isize) {
    let Some(overlay) = app.config.marketplace_actions_overlay_mut() else {
        return;
    };
    let len = overlay.actions.len();
    if len == 0 {
        overlay.selected_index = 0;
        return;
    }
    let current = overlay.selected_index;
    overlay.selected_index = if delta.is_negative() {
        current.saturating_sub(delta.unsigned_abs())
    } else {
        current.saturating_add(delta.cast_unsigned()).min(len.saturating_sub(1))
    };
}

fn execute_selected_installed_overlay_action(app: &mut App) {
    let Some(overlay) = app.config.installed_plugin_actions_overlay().cloned() else {
        return;
    };
    let Some(action) = overlay.actions.get(overlay.selected_index).copied() else {
        return;
    };

    let (cwd_raw, args, status_message) = installed_action_command(app, &overlay, action);

    if tokio::runtime::Handle::try_current().is_err() {
        app.config.overlay = None;
        app.config.status_message = None;
        app.config.last_error = Some("No runtime available for plugin action".to_owned());
        return;
    }

    app.config.overlay = None;
    app.config.last_error = None;
    app.config.status_message = Some(status_message);
    app.plugins.loading = true;
    app.plugins.last_inventory_refresh_at = None;
    app.needs_redraw = true;
    let event_tx = app.event_tx.clone();
    let cwd_context = app.cwd_raw.clone();
    let cached_claude_path = app.plugins.claude_path.clone();
    tokio::task::spawn_local(async move {
        match cli::run_cli_command_and_refresh(cwd_raw, cached_claude_path, args).await {
            Ok((snapshot, claude_path)) => {
                let message =
                    installed_action_success_message(action, &overlay.title, &overlay.scope);
                let _ = event_tx.send(ClientEvent::PluginsCliActionSucceeded {
                    cwd_raw: cwd_context,
                    result: PluginsCliActionSuccess { snapshot, message, claude_path },
                });
            }
            Err(message) => {
                let _ = event_tx
                    .send(ClientEvent::PluginsCliActionFailed { cwd_raw: cwd_context, message });
            }
        }
    });
}

fn execute_selected_plugin_install_action(app: &mut App) {
    let Some(overlay) = app.config.plugin_install_overlay().cloned() else {
        return;
    };
    let Some(action) = overlay.actions.get(overlay.selected_index).copied() else {
        return;
    };

    if tokio::runtime::Handle::try_current().is_err() {
        app.config.overlay = None;
        app.config.status_message = None;
        app.config.last_error = Some("No runtime available for plugin action".to_owned());
        return;
    }

    let scope = action.scope();
    let args = vec![
        "plugin".to_owned(),
        "install".to_owned(),
        overlay.plugin_id.clone(),
        "--scope".to_owned(),
        scope.to_owned(),
    ];
    let status_message = match action {
        PluginInstallActionKind::User => format!("Installing {} for user scope...", overlay.title),
        PluginInstallActionKind::Project => {
            format!("Installing {} for project scope...", overlay.title)
        }
        PluginInstallActionKind::Local => {
            format!("Installing {} locally...", overlay.title)
        }
    };

    app.config.overlay = None;
    app.config.last_error = None;
    app.config.status_message = Some(status_message);
    app.plugins.loading = true;
    app.plugins.last_inventory_refresh_at = None;
    app.needs_redraw = true;
    let event_tx = app.event_tx.clone();
    let cwd_raw = app.cwd_raw.clone();
    let cwd_context = app.cwd_raw.clone();
    let cached_claude_path = app.plugins.claude_path.clone();
    tokio::task::spawn_local(async move {
        match cli::run_cli_command_and_refresh(cwd_raw, cached_claude_path, args).await {
            Ok((snapshot, claude_path)) => {
                let message = plugin_install_success_message(action, &overlay.title);
                let _ = event_tx.send(ClientEvent::PluginsCliActionSucceeded {
                    cwd_raw: cwd_context,
                    result: PluginsCliActionSuccess { snapshot, message, claude_path },
                });
            }
            Err(message) => {
                let _ = event_tx
                    .send(ClientEvent::PluginsCliActionFailed { cwd_raw: cwd_context, message });
            }
        }
    });
}

fn execute_selected_marketplace_action(app: &mut App) {
    let Some(overlay) = app.config.marketplace_actions_overlay().cloned() else {
        return;
    };
    let Some(action) = overlay.actions.get(overlay.selected_index).copied() else {
        return;
    };

    if tokio::runtime::Handle::try_current().is_err() {
        app.config.overlay = None;
        app.config.status_message = None;
        app.config.last_error = Some("No runtime available for marketplace action".to_owned());
        return;
    }

    let args = marketplace_action_command(&overlay, action);
    let status_message = marketplace_action_status_message(&overlay.title, action);

    app.config.overlay = None;
    app.config.last_error = None;
    app.config.status_message = Some(status_message);
    app.plugins.loading = true;
    app.plugins.last_inventory_refresh_at = None;
    app.needs_redraw = true;
    let event_tx = app.event_tx.clone();
    let cwd_raw = app.cwd_raw.clone();
    let cwd_context = app.cwd_raw.clone();
    let cached_claude_path = app.plugins.claude_path.clone();
    tokio::task::spawn_local(async move {
        match cli::run_cli_command_and_refresh(cwd_raw, cached_claude_path, args).await {
            Ok((snapshot, claude_path)) => {
                let message = marketplace_action_success_message(&overlay.title, action);
                let _ = event_tx.send(ClientEvent::PluginsCliActionSucceeded {
                    cwd_raw: cwd_context,
                    result: PluginsCliActionSuccess { snapshot, message, claude_path },
                });
            }
            Err(message) => {
                let _ = event_tx
                    .send(ClientEvent::PluginsCliActionFailed { cwd_raw: cwd_context, message });
            }
        }
    });
}

fn confirm_add_marketplace_overlay(app: &mut App) {
    let Some(overlay) = app.config.add_marketplace_overlay().cloned() else {
        return;
    };
    let source = overlay.draft.trim().to_owned();
    if source.is_empty() {
        app.config.last_error = Some("Marketplace source cannot be empty".to_owned());
        app.config.status_message = None;
        return;
    }
    if tokio::runtime::Handle::try_current().is_err() {
        app.config.overlay = None;
        app.config.status_message = None;
        app.config.last_error = Some("No runtime available for marketplace action".to_owned());
        return;
    }

    let args = vec![
        "plugin".to_owned(),
        "marketplace".to_owned(),
        "add".to_owned(),
        source.clone(),
        "--scope".to_owned(),
        "user".to_owned(),
    ];

    app.config.overlay = None;
    app.config.last_error = None;
    app.config.status_message = Some(format!("Adding marketplace {source}..."));
    app.plugins.loading = true;
    app.plugins.last_inventory_refresh_at = None;
    app.needs_redraw = true;
    let event_tx = app.event_tx.clone();
    let cwd_raw = app.cwd_raw.clone();
    let cwd_context = app.cwd_raw.clone();
    let cached_claude_path = app.plugins.claude_path.clone();
    tokio::task::spawn_local(async move {
        match cli::run_cli_command_and_refresh(cwd_raw, cached_claude_path, args).await {
            Ok((snapshot, claude_path)) => {
                let _ = event_tx.send(ClientEvent::PluginsCliActionSucceeded {
                    cwd_raw: cwd_context,
                    result: PluginsCliActionSuccess {
                        snapshot,
                        message: format!("Added marketplace {source}"),
                        claude_path,
                    },
                });
            }
            Err(message) => {
                let _ = event_tx
                    .send(ClientEvent::PluginsCliActionFailed { cwd_raw: cwd_context, message });
            }
        }
    });
}

pub(crate) fn apply_cli_action_success(app: &mut App, result: PluginsCliActionSuccess) {
    apply_inventory_refresh_success(app, result.snapshot, result.claude_path);
    app.config.last_error = None;
    app.config.status_message = Some(result.message);
}

pub(crate) fn apply_cli_action_failure(app: &mut App, message: String) {
    app.plugins.loading = false;
    app.config.status_message = None;
    app.config.last_error = Some(message);
}

fn installed_action_command(
    app: &App,
    overlay: &InstalledPluginActionOverlayState,
    action: InstalledPluginActionKind,
) -> (String, Vec<String>, String) {
    let cwd_raw = action_cwd(app, overlay);
    let plugin_id = overlay.plugin_id.clone();
    let scope = overlay.scope.clone();
    let action_label = display_label(&plugin_id);
    match action {
        InstalledPluginActionKind::Enable => (
            cwd_raw.clone(),
            vec![
                "plugin".to_owned(),
                "enable".to_owned(),
                plugin_id.clone(),
                "--scope".to_owned(),
                scope.clone(),
            ],
            format!("Enabling {action_label}..."),
        ),
        InstalledPluginActionKind::Disable => (
            cwd_raw.clone(),
            vec![
                "plugin".to_owned(),
                "disable".to_owned(),
                plugin_id.clone(),
                "--scope".to_owned(),
                scope.clone(),
            ],
            format!("Disabling {action_label}..."),
        ),
        InstalledPluginActionKind::Update => (
            cwd_raw.clone(),
            vec![
                "plugin".to_owned(),
                "update".to_owned(),
                plugin_id.clone(),
                "--scope".to_owned(),
                scope.clone(),
            ],
            format!("Updating {action_label}..."),
        ),
        InstalledPluginActionKind::InstallInCurrentProject => (
            app.cwd_raw.clone(),
            vec![
                "plugin".to_owned(),
                "install".to_owned(),
                plugin_id.clone(),
                "--scope".to_owned(),
                "local".to_owned(),
            ],
            format!("Installing {action_label} in the current project..."),
        ),
        InstalledPluginActionKind::Uninstall => (
            cwd_raw,
            vec![
                "plugin".to_owned(),
                "uninstall".to_owned(),
                plugin_id,
                "--scope".to_owned(),
                scope,
            ],
            format!("Uninstalling {action_label}..."),
        ),
    }
}

fn installed_action_success_message(
    action: InstalledPluginActionKind,
    title: &str,
    scope: &str,
) -> String {
    match action {
        InstalledPluginActionKind::Enable => format!("Enabled {title} in {scope} scope"),
        InstalledPluginActionKind::Disable => format!("Disabled {title} in {scope} scope"),
        InstalledPluginActionKind::Update => format!("Updated {title} in {scope} scope"),
        InstalledPluginActionKind::InstallInCurrentProject => {
            format!("Installed {title} in the current project")
        }
        InstalledPluginActionKind::Uninstall => format!("Uninstalled {title} from {scope} scope"),
    }
}

fn plugin_install_success_message(action: PluginInstallActionKind, title: &str) -> String {
    match action {
        PluginInstallActionKind::User => format!("Installed {title} for user scope"),
        PluginInstallActionKind::Project => format!("Installed {title} for project scope"),
        PluginInstallActionKind::Local => format!("Installed {title} locally"),
    }
}

fn marketplace_action_command(
    overlay: &MarketplaceActionsOverlayState,
    action: MarketplaceActionKind,
) -> Vec<String> {
    match action {
        MarketplaceActionKind::Update => vec![
            "plugin".to_owned(),
            "marketplace".to_owned(),
            "update".to_owned(),
            overlay.name.clone(),
        ],
        MarketplaceActionKind::Remove => vec![
            "plugin".to_owned(),
            "marketplace".to_owned(),
            "remove".to_owned(),
            overlay.name.clone(),
        ],
    }
}

fn marketplace_action_status_message(title: &str, action: MarketplaceActionKind) -> String {
    match action {
        MarketplaceActionKind::Update => format!("Updating {title} marketplace..."),
        MarketplaceActionKind::Remove => format!("Removing {title} marketplace..."),
    }
}

fn marketplace_action_success_message(title: &str, action: MarketplaceActionKind) -> String {
    match action {
        MarketplaceActionKind::Update => format!("Updated {title} marketplace"),
        MarketplaceActionKind::Remove => format!("Removed {title} marketplace"),
    }
}

fn action_cwd(app: &App, overlay: &InstalledPluginActionOverlayState) -> String {
    match overlay.scope.as_str() {
        "local" | "project" => overlay.project_path.clone().unwrap_or_else(|| app.cwd_raw.clone()),
        _ => app.cwd_raw.clone(),
    }
}

fn installed_overlay_actions(
    app: &App,
    entry: &InstalledPluginEntry,
) -> Vec<InstalledPluginActionKind> {
    let mut actions = Vec::new();
    match entry.scope.as_str() {
        "user" | "project" | "local" => {
            actions.push(if entry.enabled {
                InstalledPluginActionKind::Disable
            } else {
                InstalledPluginActionKind::Enable
            });
        }
        _ => {}
    }
    actions.push(InstalledPluginActionKind::Update);
    if can_install_in_current_project(app, entry) {
        actions.push(InstalledPluginActionKind::InstallInCurrentProject);
    }
    actions.push(InstalledPluginActionKind::Uninstall);
    actions
}

fn installed_overlay_description(app: &App, entry: &InstalledPluginEntry) -> String {
    if let Some(description) = app
        .plugins
        .marketplace
        .iter()
        .find(|candidate| candidate.plugin_id == entry.id)
        .and_then(|candidate| candidate.description.as_deref())
    {
        return description.to_owned();
    }

    match entry.project_path.as_deref() {
        Some(project_path) => format!("Installed in {} scope for {}.", entry.scope, project_path),
        None => format!("Installed in {} scope.", entry.scope),
    }
}

fn can_install_in_current_project(app: &App, entry: &InstalledPluginEntry) -> bool {
    let current_project = normalize_project_path(&app.cwd_raw);
    let selected_project = entry.project_path.as_deref().map(normalize_project_path);
    if matches!(entry.scope.as_str(), "local" | "project")
        && selected_project.as_deref() == Some(current_project.as_str())
    {
        return false;
    }

    !app.plugins.installed.iter().any(|candidate| {
        candidate.id == entry.id
            && matches!(candidate.scope.as_str(), "local" | "project")
            && candidate.project_path.as_deref().map(normalize_project_path).as_deref()
                == Some(current_project.as_str())
    })
}

fn selected_installed_entry(app: &App) -> Option<&InstalledPluginEntry> {
    ordered_installed(&app.plugins, &app.cwd_raw).get(app.plugins.installed_selected_index).copied()
}

fn selected_marketplace_plugin(app: &App) -> Option<&MarketplaceEntry> {
    filtered_marketplace_plugins(&app.plugins).get(app.plugins.plugins_selected_index).copied()
}

fn selected_marketplace_source(app: &App) -> Option<&MarketplaceSourceEntry> {
    visible_marketplaces(&app.plugins).get(app.plugins.marketplace_selected_index).copied()
}

fn selected_add_marketplace_row(app: &App) -> bool {
    app.plugins.marketplace_selected_index >= visible_marketplaces(&app.plugins).len()
}

fn marketplace_row_count(state: &PluginsState) -> usize {
    state.marketplaces.len().saturating_add(1)
}

fn marketplace_overlay_description(entry: &MarketplaceSourceEntry) -> String {
    let mut parts = Vec::new();
    if let Some(source) = entry.source.as_deref() {
        parts.push(format!("Source: {source}"));
    }
    if let Some(repo) = entry.repo.as_deref() {
        parts.push(format!("Repo: {repo}"));
    }
    if parts.is_empty() {
        "Manage this configured marketplace.".to_owned()
    } else {
        parts.join("\n")
    }
}

fn normalize_project_path(path: &str) -> String {
    path.replace('\\', "/").trim_end_matches('/').to_ascii_lowercase()
}

fn normalize_single_line_input(text: &str) -> String {
    text.replace("\r\n", "\n").replace('\r', "\n").replace('\n', " ")
}

fn move_add_marketplace_cursor_left(app: &mut App) {
    let Some(overlay) = app.config.add_marketplace_overlay_mut() else {
        return;
    };
    overlay.cursor = overlay.cursor.saturating_sub(1);
}

fn move_add_marketplace_cursor_right(app: &mut App) {
    let Some(overlay) = app.config.add_marketplace_overlay_mut() else {
        return;
    };
    overlay.cursor = overlay.cursor.saturating_add(1).min(overlay.draft.chars().count());
}

fn move_add_marketplace_cursor_to_end(app: &mut App) {
    let Some(overlay) = app.config.add_marketplace_overlay_mut() else {
        return;
    };
    overlay.cursor = overlay.draft.chars().count();
}

fn set_add_marketplace_cursor(app: &mut App, cursor: usize) {
    let Some(overlay) = app.config.add_marketplace_overlay_mut() else {
        return;
    };
    overlay.cursor = cursor.min(overlay.draft.chars().count());
}

fn insert_add_marketplace_char(app: &mut App, ch: char) {
    let Some(overlay) = app.config.add_marketplace_overlay_mut() else {
        return;
    };
    let byte_index = char_to_byte_index(&overlay.draft, overlay.cursor);
    overlay.draft.insert(byte_index, ch);
    overlay.cursor += 1;
}

fn delete_add_marketplace_before_cursor(app: &mut App) {
    let Some(overlay) = app.config.add_marketplace_overlay_mut() else {
        return;
    };
    if overlay.cursor == 0 {
        return;
    }
    let end = char_to_byte_index(&overlay.draft, overlay.cursor);
    let start = char_to_byte_index(&overlay.draft, overlay.cursor - 1);
    overlay.draft.replace_range(start..end, "");
    overlay.cursor -= 1;
}

fn delete_add_marketplace_at_cursor(app: &mut App) {
    let Some(overlay) = app.config.add_marketplace_overlay_mut() else {
        return;
    };
    let char_count = overlay.draft.chars().count();
    if overlay.cursor >= char_count {
        return;
    }
    let start = char_to_byte_index(&overlay.draft, overlay.cursor);
    let end = char_to_byte_index(&overlay.draft, overlay.cursor + 1);
    overlay.draft.replace_range(start..end, "");
}

fn char_to_byte_index(text: &str, char_index: usize) -> usize {
    text.char_indices().nth(char_index).map_or(text.len(), |(idx, _)| idx)
}

fn reset_selection_for_active_tab(app: &mut App) {
    app.plugins.set_selected_index_for(app.plugins.active_tab, 0);
    clamp_selection(app);
}

fn move_selection(app: &mut App, delta: isize) {
    let tab = app.plugins.active_tab;
    let len = match tab {
        PluginsViewTab::Installed => filtered_installed(&app.plugins).len(),
        PluginsViewTab::Plugins => filtered_marketplace_plugins(&app.plugins).len(),
        PluginsViewTab::Marketplace => marketplace_row_count(&app.plugins),
    };
    if len == 0 {
        app.plugins.set_selected_index_for(tab, 0);
        return;
    }
    let current = app.plugins.selected_index_for(tab);
    let next = if delta.is_negative() {
        current.saturating_sub(delta.unsigned_abs())
    } else {
        current.saturating_add(delta.cast_unsigned()).min(len.saturating_sub(1))
    };
    app.plugins.set_selected_index_for(tab, next);
}

fn clamp_index(current: usize, len: usize) -> usize {
    if len == 0 { 0 } else { current.min(len.saturating_sub(1)) }
}

fn installed_entry_matches(entry: &InstalledPluginEntry, query: &str) -> bool {
    if query.is_empty() {
        return true;
    }
    let query = query.to_ascii_lowercase();
    entry.id.to_ascii_lowercase().contains(&query)
        || entry.scope.to_ascii_lowercase().contains(&query)
        || entry
            .version
            .as_deref()
            .is_some_and(|version| version.to_ascii_lowercase().contains(&query))
}

fn is_relevant_installed_entry(entry: &InstalledPluginEntry, current_project: &str) -> bool {
    match entry.scope.as_str() {
        "user" => true,
        "local" | "project" => entry
            .project_path
            .as_deref()
            .map(normalize_project_path)
            .is_some_and(|project| project == current_project),
        _ => false,
    }
}

fn marketplace_plugin_matches(entry: &MarketplaceEntry, query: &str) -> bool {
    if query.is_empty() {
        return true;
    }
    let query = query.to_ascii_lowercase();
    entry.plugin_id.to_ascii_lowercase().contains(&query)
        || entry.name.to_ascii_lowercase().contains(&query)
        || entry
            .description
            .as_deref()
            .is_some_and(|description| description.to_ascii_lowercase().contains(&query))
        || entry
            .marketplace_name
            .as_deref()
            .is_some_and(|marketplace| marketplace.to_ascii_lowercase().contains(&query))
        || entry
            .version
            .as_deref()
            .is_some_and(|version| version.to_ascii_lowercase().contains(&query))
}

#[must_use]
pub(crate) const fn search_enabled(tab: PluginsViewTab) -> bool {
    !matches!(tab, PluginsViewTab::Marketplace)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn plugins_tabs_wrap_in_both_directions() {
        assert_eq!(PluginsViewTab::Installed.prev(), PluginsViewTab::Marketplace);
        assert_eq!(PluginsViewTab::Marketplace.next(), PluginsViewTab::Installed);
    }

    #[test]
    fn recent_inventory_snapshot_skips_refresh() {
        let mut app = crate::app::App::test_default();
        app.plugins.active_tab = PluginsViewTab::Installed;
        app.plugins.last_inventory_refresh_at = Some(Instant::now());

        request_inventory_refresh_if_needed(&mut app);

        assert!(!app.plugins.loading);
    }

    #[test]
    fn display_label_normalizes_plugin_and_marketplace_names() {
        assert_eq!(
            display_label("frontend-design@claude-plugins-official"),
            "Frontend Design From Claude Plugins Official"
        );
        assert_eq!(display_label("claude-plugins-official"), "Claude Plugins Official");
    }

    #[test]
    fn filtered_marketplace_plugins_match_on_name_description_and_marketplace() {
        let state = PluginsState {
            plugins_search_query: "official".to_owned(),
            marketplace: vec![MarketplaceEntry {
                plugin_id: "frontend-design@claude-plugins-official".to_owned(),
                name: "frontend-design".to_owned(),
                description: Some("Create distinctive interfaces".to_owned()),
                marketplace_name: Some("claude-plugins-official".to_owned()),
                version: Some("1.0.0".to_owned()),
                install_count: Some(42),
                source: None,
            }],
            ..PluginsState::default()
        };

        assert_eq!(filtered_marketplace_plugins(&state).len(), 1);
    }

    #[test]
    fn installed_and_plugins_search_queries_are_independent() {
        let state = PluginsState {
            installed_search_query: "installed".to_owned(),
            plugins_search_query: "plugins".to_owned(),
            ..PluginsState::default()
        };

        assert_eq!(state.search_query_for(PluginsViewTab::Installed), "installed");
        assert_eq!(state.search_query_for(PluginsViewTab::Plugins), "plugins");
    }

    #[test]
    fn install_in_current_project_is_available_for_other_project_local_install() {
        let mut app = crate::app::App::test_default();
        app.cwd_raw = "C:\\work\\project-b".to_owned();
        let entry = InstalledPluginEntry {
            id: "frontend-design@claude-plugins-official".to_owned(),
            version: Some("1.0.0".to_owned()),
            scope: "local".to_owned(),
            enabled: true,
            installed_at: None,
            last_updated: None,
            project_path: Some("C:\\work\\project-a".to_owned()),
            capability: PluginCapability::Skill,
        };

        assert!(can_install_in_current_project(&app, &entry));
    }

    #[test]
    fn install_in_current_project_is_hidden_when_already_installed_here() {
        let mut app = crate::app::App::test_default();
        app.cwd_raw = "C:\\work\\project-b".to_owned();
        app.plugins.installed.push(InstalledPluginEntry {
            id: "frontend-design@claude-plugins-official".to_owned(),
            version: Some("1.0.0".to_owned()),
            scope: "local".to_owned(),
            enabled: true,
            installed_at: None,
            last_updated: None,
            project_path: Some("C:\\work\\project-b".to_owned()),
            capability: PluginCapability::Skill,
        });
        let entry = InstalledPluginEntry {
            id: "frontend-design@claude-plugins-official".to_owned(),
            version: Some("1.0.0".to_owned()),
            scope: "local".to_owned(),
            enabled: true,
            installed_at: None,
            last_updated: None,
            project_path: Some("C:\\work\\project-a".to_owned()),
            capability: PluginCapability::Skill,
        };

        assert!(!can_install_in_current_project(&app, &entry));
    }

    #[test]
    fn ordered_installed_puts_current_project_and_user_entries_first() {
        let state = PluginsState {
            installed: vec![
                InstalledPluginEntry {
                    id: "other-local@claude-plugins-official".to_owned(),
                    version: None,
                    scope: "local".to_owned(),
                    enabled: true,
                    installed_at: None,
                    last_updated: None,
                    project_path: Some("C:\\work\\project-a".to_owned()),
                    capability: PluginCapability::Skill,
                },
                InstalledPluginEntry {
                    id: "user-plugin@claude-plugins-official".to_owned(),
                    version: None,
                    scope: "user".to_owned(),
                    enabled: true,
                    installed_at: None,
                    last_updated: None,
                    project_path: None,
                    capability: PluginCapability::Skill,
                },
                InstalledPluginEntry {
                    id: "current-local@claude-plugins-official".to_owned(),
                    version: None,
                    scope: "local".to_owned(),
                    enabled: true,
                    installed_at: None,
                    last_updated: None,
                    project_path: Some("C:\\work\\project-b".to_owned()),
                    capability: PluginCapability::Skill,
                },
            ],
            ..PluginsState::default()
        };

        let ordered = ordered_installed(&state, "C:\\work\\project-b");
        let ordered_ids = ordered.iter().map(|entry| entry.id.as_str()).collect::<Vec<_>>();

        assert_eq!(
            ordered_ids,
            vec![
                "user-plugin@claude-plugins-official",
                "current-local@claude-plugins-official",
                "other-local@claude-plugins-official",
            ]
        );
    }
}
