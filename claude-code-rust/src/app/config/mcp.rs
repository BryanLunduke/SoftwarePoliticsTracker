use super::{ConfigOverlayState, ConfigState, ConfigTab};
use crate::app::App;
use crate::app::view::{self, ActiveView};
use crossterm::event::{KeyCode, KeyEvent, KeyModifiers};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum McpServerActionKind {
    RefreshSnapshot,
    Authenticate,
    ClearAuth,
    Reconnect,
    Enable,
    Disable,
}

impl McpServerActionKind {
    #[must_use]
    pub const fn label(self) -> &'static str {
        match self {
            Self::RefreshSnapshot => "Refresh",
            Self::Authenticate => "Authenticate",
            Self::ClearAuth => "Clear auth",
            Self::Reconnect => "Reconnect server",
            Self::Enable => "Enable server",
            Self::Disable => "Disable server",
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct McpDetailsOverlayState {
    pub server_name: String,
    pub selected_index: usize,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct McpCallbackUrlOverlayState {
    pub server_name: String,
    pub draft: String,
    pub cursor: usize,
}

#[derive(Debug, Clone, PartialEq)]
pub struct McpElicitationOverlayState {
    pub request: crate::agent::types::ElicitationRequest,
    pub selected_index: usize,
    pub browser_opened: bool,
    pub browser_open_error: Option<String>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct McpAuthRedirectOverlayState {
    pub redirect: crate::agent::types::McpAuthRedirect,
    pub selected_index: usize,
    pub browser_opened: bool,
    pub browser_open_error: Option<String>,
}

impl ConfigState {
    #[must_use]
    pub fn mcp_details_overlay(&self) -> Option<&McpDetailsOverlayState> {
        if let Some(ConfigOverlayState::McpDetails(overlay)) = &self.overlay {
            Some(overlay)
        } else {
            None
        }
    }

    pub fn mcp_details_overlay_mut(&mut self) -> Option<&mut McpDetailsOverlayState> {
        if let Some(ConfigOverlayState::McpDetails(overlay)) = &mut self.overlay {
            Some(overlay)
        } else {
            None
        }
    }

    #[must_use]
    pub fn mcp_callback_url_overlay(&self) -> Option<&McpCallbackUrlOverlayState> {
        if let Some(ConfigOverlayState::McpCallbackUrl(overlay)) = &self.overlay {
            Some(overlay)
        } else {
            None
        }
    }

    pub fn mcp_callback_url_overlay_mut(&mut self) -> Option<&mut McpCallbackUrlOverlayState> {
        if let Some(ConfigOverlayState::McpCallbackUrl(overlay)) = &mut self.overlay {
            Some(overlay)
        } else {
            None
        }
    }

    #[must_use]
    pub fn mcp_elicitation_overlay(&self) -> Option<&McpElicitationOverlayState> {
        if let Some(ConfigOverlayState::McpElicitation(overlay)) = &self.overlay {
            Some(overlay)
        } else {
            None
        }
    }

    pub fn mcp_elicitation_overlay_mut(&mut self) -> Option<&mut McpElicitationOverlayState> {
        if let Some(ConfigOverlayState::McpElicitation(overlay)) = &mut self.overlay {
            Some(overlay)
        } else {
            None
        }
    }

    #[must_use]
    pub fn mcp_auth_redirect_overlay(&self) -> Option<&McpAuthRedirectOverlayState> {
        if let Some(ConfigOverlayState::McpAuthRedirect(overlay)) = &self.overlay {
            Some(overlay)
        } else {
            None
        }
    }

    pub fn mcp_auth_redirect_overlay_mut(&mut self) -> Option<&mut McpAuthRedirectOverlayState> {
        if let Some(ConfigOverlayState::McpAuthRedirect(overlay)) = &mut self.overlay {
            Some(overlay)
        } else {
            None
        }
    }
}

pub(super) fn handle_mcp_key(app: &mut App, key: KeyEvent) -> bool {
    if app.config.active_tab != ConfigTab::Mcp {
        return false;
    }

    match (key.code, key.modifiers) {
        (KeyCode::Char(ch), modifiers)
            if matches!(ch, 'r' | 'R')
                && (modifiers.is_empty() || modifiers == KeyModifiers::SHIFT) =>
        {
            refresh_mcp_snapshot(app);
            true
        }
        (KeyCode::Enter, KeyModifiers::NONE) => {
            open_selected_mcp_server_details(app);
            true
        }
        (KeyCode::Up, KeyModifiers::NONE) => {
            app.config.mcp_selected_server_index =
                app.config.mcp_selected_server_index.saturating_sub(1);
            true
        }
        (KeyCode::Down, KeyModifiers::NONE) => {
            let last_index = app.mcp.servers.len().saturating_sub(1);
            app.config.mcp_selected_server_index =
                (app.config.mcp_selected_server_index + 1).min(last_index);
            true
        }
        _ => false,
    }
}

pub(crate) fn refresh_mcp_snapshot_if_needed(app: &mut App) {
    if app.config.active_tab != ConfigTab::Mcp {
        tracing::debug!("skipping MCP refresh request: active_tab={:?}", app.config.active_tab);
        return;
    }
    refresh_mcp_snapshot(app);
}

pub(crate) fn refresh_mcp_snapshot(app: &mut App) {
    app.mcp.servers.clear();
    app.mcp.last_error = None;
    request_mcp_snapshot(app);
}

pub(crate) fn request_mcp_snapshot(app: &mut App) {
    let Some(conn) = app.conn.as_ref() else {
        app.mcp.in_flight = false;
        return;
    };
    let Some(ref sid) = app.session_id else {
        app.mcp.in_flight = false;
        return;
    };
    tracing::debug!("requesting MCP snapshot: session_id={sid}");
    app.mcp.in_flight = true;
    app.mcp.last_error = None;
    if let Err(err) = conn.get_mcp_snapshot(sid.to_string()) {
        app.mcp.in_flight = false;
        app.mcp.last_error = Some(err.to_string());
        tracing::warn!("failed to request MCP snapshot: {err}");
    }
}

pub(crate) fn reconnect_mcp_server(app: &mut App, server_name: &str) {
    let Some(conn) = app.conn.as_ref() else {
        return;
    };
    let Some(ref sid) = app.session_id else {
        return;
    };
    if conn.reconnect_mcp_server(sid.to_string(), server_name.to_owned()).is_ok() {
        refresh_mcp_snapshot(app);
    }
}

pub(crate) fn set_mcp_server_enabled(app: &mut App, server_name: &str, enabled: bool) {
    let Some(conn) = app.conn.as_ref() else {
        return;
    };
    let Some(ref sid) = app.session_id else {
        return;
    };
    if conn.toggle_mcp_server(sid.to_string(), server_name.to_owned(), enabled).is_ok() {
        refresh_mcp_snapshot(app);
    }
}

pub(crate) fn authenticate_mcp_server(app: &mut App, server_name: &str) {
    let Some(conn) = app.conn.as_ref() else {
        return;
    };
    let Some(ref sid) = app.session_id else {
        return;
    };
    if conn.authenticate_mcp_server(sid.to_string(), server_name.to_owned()).is_ok() {
        app.config.status_message = Some(format!("Starting MCP auth for {server_name}..."));
        app.config.last_error = None;
        refresh_mcp_snapshot(app);
    }
}

pub(crate) fn clear_mcp_server_auth(app: &mut App, server_name: &str) {
    let Some(conn) = app.conn.as_ref() else {
        return;
    };
    let Some(ref sid) = app.session_id else {
        return;
    };
    if conn.clear_mcp_auth(sid.to_string(), server_name.to_owned()).is_ok() {
        refresh_mcp_snapshot(app);
    }
}

pub(crate) fn submit_mcp_oauth_callback_url(
    app: &mut App,
    server_name: &str,
    callback_url: String,
) {
    let Some(conn) = app.conn.as_ref() else {
        return;
    };
    let Some(ref sid) = app.session_id else {
        return;
    };
    if conn
        .submit_mcp_oauth_callback_url(sid.to_string(), server_name.to_owned(), callback_url)
        .is_ok()
    {
        refresh_mcp_snapshot(app);
    }
}

pub(crate) fn send_mcp_elicitation_response(
    app: &mut App,
    request_id: &str,
    action: crate::agent::types::ElicitationAction,
    content: Option<serde_json::Value>,
) {
    let Some(conn) = app.conn.as_ref() else {
        return;
    };
    let Some(ref sid) = app.session_id else {
        return;
    };
    if conn.respond_to_elicitation(sid.to_string(), request_id.to_owned(), action, content).is_ok()
    {
        app.mcp.pending_elicitation = None;
        refresh_mcp_snapshot(app);
    }
}

fn open_selected_mcp_server_details(app: &mut App) {
    let Some(server_name) =
        app.mcp.servers.get(app.config.mcp_selected_server_index).map(|server| server.name.clone())
    else {
        return;
    };
    open_mcp_server_details(app, server_name, None);
}

pub(crate) fn open_mcp_server_details(
    app: &mut App,
    server_name: String,
    preferred_action: Option<McpServerActionKind>,
) {
    let selected_index =
        app.mcp.servers.iter().find(|server| server.name == server_name).map_or(0, |server| {
            preferred_action
                .and_then(|action| {
                    available_mcp_actions(server).iter().position(|candidate| *candidate == action)
                })
                .unwrap_or(0)
        });
    app.config.overlay = Some(ConfigOverlayState::McpDetails(McpDetailsOverlayState {
        server_name,
        selected_index,
    }));
    app.config.last_error = None;
}

#[must_use]
pub(crate) fn available_mcp_actions(
    server: &crate::agent::types::McpServerStatus,
) -> Vec<McpServerActionKind> {
    let mut actions = vec![McpServerActionKind::RefreshSnapshot];
    if matches!(server.status, crate::agent::types::McpServerConnectionStatus::Disabled) {
        actions.push(McpServerActionKind::Enable);
    } else {
        if matches!(
            server.status,
            crate::agent::types::McpServerConnectionStatus::NeedsAuth
                | crate::agent::types::McpServerConnectionStatus::Failed
                | crate::agent::types::McpServerConnectionStatus::Pending
        ) {
            actions.push(McpServerActionKind::Authenticate);
        }
        actions.push(McpServerActionKind::ClearAuth);
        actions.push(McpServerActionKind::Reconnect);
        actions.push(McpServerActionKind::Disable);
    }
    actions
}

#[must_use]
pub(crate) fn is_mcp_action_available(
    server: &crate::agent::types::McpServerStatus,
    action: McpServerActionKind,
) -> bool {
    !matches!(
        (action, server.config.as_ref()),
        (
            McpServerActionKind::Authenticate,
            Some(crate::agent::types::McpServerStatusConfig::ClaudeaiProxy { .. })
        )
    )
}

pub(crate) fn present_mcp_elicitation_request(
    app: &mut App,
    request: crate::agent::types::ElicitationRequest,
) {
    app.mcp.pending_elicitation = Some(request.clone());
    view::set_active_view(app, ActiveView::Config);
    app.config.active_tab = ConfigTab::Mcp;
    refresh_mcp_snapshot(app);
    let (browser_opened, browser_open_error) =
        if matches!(request.mode, crate::agent::types::ElicitationMode::Url) {
            request.url.as_deref().map_or(
                (false, Some("SDK did not provide an auth URL".to_owned())),
                |url| match open_url_in_browser(url) {
                    Ok(()) => (true, None),
                    Err(error) => (false, Some(error)),
                },
            )
        } else {
            (false, None)
        };
    app.config.overlay = Some(ConfigOverlayState::McpElicitation(McpElicitationOverlayState {
        request,
        selected_index: 0,
        browser_opened,
        browser_open_error,
    }));
    app.config.last_error = None;
}

pub(crate) fn present_mcp_auth_redirect(
    app: &mut App,
    redirect: crate::agent::types::McpAuthRedirect,
) {
    view::set_active_view(app, ActiveView::Config);
    app.config.active_tab = ConfigTab::Mcp;
    refresh_mcp_snapshot(app);
    let (browser_opened, browser_open_error) = match open_url_in_browser(&redirect.auth_url) {
        Ok(()) => (true, None),
        Err(error) => (false, Some(error)),
    };
    app.config.overlay = Some(ConfigOverlayState::McpAuthRedirect(McpAuthRedirectOverlayState {
        redirect,
        selected_index: 0,
        browser_opened,
        browser_open_error,
    }));
    app.config.last_error = None;
}

pub(crate) fn handle_mcp_elicitation_completed(
    app: &mut App,
    elicitation_id: &str,
    _server_name: Option<String>,
) {
    let should_clear = app
        .mcp
        .pending_elicitation
        .as_ref()
        .and_then(|request| request.elicitation_id.as_deref())
        .is_some_and(|current| current == elicitation_id);
    if should_clear {
        app.mcp.pending_elicitation = None;
        if matches!(app.config.overlay, Some(ConfigOverlayState::McpElicitation(_))) {
            app.config.overlay = None;
        }
        refresh_mcp_snapshot(app);
    }
}

pub(crate) fn handle_mcp_operation_error(
    app: &mut App,
    error: &crate::agent::types::McpOperationError,
) {
    app.mcp.in_flight = false;
    let formatted = format_mcp_operation_error(error);
    app.mcp.last_error = Some(formatted.clone());
    app.config.last_error = Some(formatted);
    app.config.status_message = None;
}

fn format_mcp_operation_error(error: &crate::agent::types::McpOperationError) -> String {
    let action = match error.operation.as_str() {
        "authenticate" => "authenticate",
        "clear-auth" => "clear auth for",
        "reconnect" => "reconnect",
        "toggle" => "update",
        "submit-callback-url" => "submit callback URL for",
        other => other,
    };
    match error.server_name.as_deref() {
        Some(server_name) => {
            format!("Failed to {action} MCP server {server_name}: {}", error.message)
        }
        None => format!("MCP operation failed ({action}): {}", error.message),
    }
}

fn open_url_in_browser(url: &str) -> Result<(), String> {
    #[cfg(target_os = "windows")]
    let mut command = {
        let mut cmd = std::process::Command::new("rundll32.exe");
        cmd.args(["url.dll,FileProtocolHandler", url]);
        cmd
    };
    #[cfg(target_os = "macos")]
    let mut command = {
        let mut cmd = std::process::Command::new("open");
        cmd.arg(url);
        cmd
    };
    #[cfg(all(unix, not(target_os = "macos")))]
    let mut command = {
        let mut cmd = std::process::Command::new("xdg-open");
        cmd.arg(url);
        cmd
    };

    command
        .spawn()
        .map(|_| ())
        .map_err(|error| format!("Failed to open browser automatically: {error}"))
}

pub(crate) fn copy_text_to_clipboard(text: &str) -> Result<(), String> {
    let mut clipboard = arboard::Clipboard::new()
        .map_err(|error| format!("Failed to access clipboard: {error}"))?;
    clipboard
        .set_text(text.to_owned())
        .map_err(|error| format!("Failed to copy to clipboard: {error}"))
}
