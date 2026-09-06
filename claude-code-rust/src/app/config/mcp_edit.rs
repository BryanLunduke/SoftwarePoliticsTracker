use super::ConfigOverlayState;
use super::edit::{
    TextInputOverlay, accepts_text_input, delete_text_at_cursor, delete_text_before_cursor,
    insert_text_char, insert_text_str, move_text_cursor_left, move_text_cursor_right,
    move_text_cursor_to_end, set_text_cursor, step_index_clamped,
};
use super::mcp::{
    McpCallbackUrlOverlayState, McpServerActionKind, authenticate_mcp_server,
    available_mcp_actions, clear_mcp_server_auth, copy_text_to_clipboard, is_mcp_action_available,
    open_mcp_server_details, reconnect_mcp_server, refresh_mcp_snapshot,
    send_mcp_elicitation_response, set_mcp_server_enabled, submit_mcp_oauth_callback_url,
};
use crate::app::App;
use crossterm::event::{KeyCode, KeyEvent, KeyModifiers};

pub(super) fn handle_overlay_key(app: &mut App, key: KeyEvent) -> bool {
    match app.config.overlay.clone() {
        Some(ConfigOverlayState::McpDetails(_)) => {
            handle_mcp_details_overlay_key(app, key);
            true
        }
        Some(ConfigOverlayState::McpCallbackUrl(_)) => {
            handle_mcp_callback_url_overlay_key(app, key);
            true
        }
        Some(ConfigOverlayState::McpAuthRedirect(_)) => {
            handle_mcp_auth_redirect_overlay_key(app, key);
            true
        }
        Some(ConfigOverlayState::McpElicitation(_)) => {
            handle_mcp_elicitation_overlay_key(app, key);
            true
        }
        _ => false,
    }
}

pub(super) fn handle_overlay_paste(app: &mut App, text: &str) -> bool {
    match app.config.overlay {
        Some(ConfigOverlayState::McpCallbackUrl(_)) => {
            insert_text_str(app.config.mcp_callback_url_overlay_mut(), text);
            true
        }
        _ => false,
    }
}

fn handle_mcp_details_overlay_key(app: &mut App, key: KeyEvent) {
    match (key.code, key.modifiers) {
        (KeyCode::Esc, KeyModifiers::NONE) => app.config.overlay = None,
        (KeyCode::Up, KeyModifiers::NONE) => move_mcp_details_overlay_selection(app, -1),
        (KeyCode::Down, KeyModifiers::NONE) => move_mcp_details_overlay_selection(app, 1),
        (KeyCode::Enter, KeyModifiers::NONE) => execute_selected_mcp_overlay_action(app),
        _ => {}
    }
}

fn move_mcp_details_overlay_selection(app: &mut App, delta: isize) {
    let Some(overlay) = app.config.mcp_details_overlay().cloned() else {
        return;
    };
    let Some(server) = app.mcp.servers.iter().find(|server| server.name == overlay.server_name)
    else {
        return;
    };
    let actions = available_mcp_actions(server);
    if actions.is_empty() {
        return;
    }

    let next_index = step_index_clamped(overlay.selected_index, delta, actions.len());
    if let Some(state) = app.config.mcp_details_overlay_mut() {
        state.selected_index = next_index;
    }
}

fn execute_selected_mcp_overlay_action(app: &mut App) {
    let Some(overlay) = app.config.mcp_details_overlay().cloned() else {
        return;
    };
    let Some(server) = app.mcp.servers.iter().find(|server| server.name == overlay.server_name)
    else {
        app.config.overlay = None;
        return;
    };
    let actions = available_mcp_actions(server);
    let Some(action) = actions.get(overlay.selected_index).copied() else {
        return;
    };
    if !is_mcp_action_available(server, action) {
        return;
    }

    match action {
        McpServerActionKind::RefreshSnapshot => refresh_mcp_snapshot(app),
        McpServerActionKind::Authenticate => {
            authenticate_mcp_server(app, &overlay.server_name);
        }
        McpServerActionKind::ClearAuth => {
            clear_mcp_server_auth(app, &overlay.server_name);
        }
        McpServerActionKind::Reconnect => {
            reconnect_mcp_server(app, &overlay.server_name);
        }
        McpServerActionKind::Enable => {
            set_mcp_server_enabled(app, &overlay.server_name, true);
        }
        McpServerActionKind::Disable => {
            set_mcp_server_enabled(app, &overlay.server_name, false);
        }
    }

    app.config.overlay = None;
}

fn handle_mcp_callback_url_overlay_key(app: &mut App, key: KeyEvent) {
    match (key.code, key.modifiers) {
        (KeyCode::Enter, KeyModifiers::NONE) => confirm_mcp_callback_url_overlay(app),
        (KeyCode::Esc, KeyModifiers::NONE) => cancel_mcp_callback_url_overlay(app),
        (KeyCode::Left, KeyModifiers::NONE) => {
            move_text_cursor_left(app.config.mcp_callback_url_overlay_mut());
        }
        (KeyCode::Right, KeyModifiers::NONE) => {
            move_text_cursor_right(app.config.mcp_callback_url_overlay_mut());
        }
        (KeyCode::Home, KeyModifiers::NONE) => {
            set_text_cursor(app.config.mcp_callback_url_overlay_mut(), 0);
        }
        (KeyCode::End, KeyModifiers::NONE) => {
            move_text_cursor_to_end(app.config.mcp_callback_url_overlay_mut());
        }
        (KeyCode::Backspace, KeyModifiers::NONE) => {
            delete_text_before_cursor(app.config.mcp_callback_url_overlay_mut());
        }
        (KeyCode::Delete, KeyModifiers::NONE) => {
            delete_text_at_cursor(app.config.mcp_callback_url_overlay_mut());
        }
        (KeyCode::Char(ch), modifiers) if accepts_text_input(modifiers) => {
            insert_text_char(app.config.mcp_callback_url_overlay_mut(), ch);
        }
        _ => {}
    }
}

fn cancel_mcp_callback_url_overlay(app: &mut App) {
    let Some(server_name) =
        app.config.mcp_callback_url_overlay().map(|overlay| overlay.server_name.clone())
    else {
        app.config.overlay = None;
        return;
    };
    open_mcp_server_details(app, server_name, Some(McpServerActionKind::Authenticate));
}

fn confirm_mcp_callback_url_overlay(app: &mut App) {
    let Some(overlay) = app.config.mcp_callback_url_overlay().cloned() else {
        return;
    };
    let callback_url = overlay.draft.trim().to_owned();
    if callback_url.is_empty() {
        app.config.last_error = Some("Callback URL cannot be empty".to_owned());
        app.config.status_message = None;
        return;
    }

    submit_mcp_oauth_callback_url(app, &overlay.server_name, callback_url);
    app.config.overlay = None;
}

fn handle_mcp_elicitation_overlay_key(app: &mut App, key: KeyEvent) {
    match (key.code, key.modifiers) {
        (KeyCode::Esc, KeyModifiers::NONE) => cancel_mcp_elicitation_overlay(app),
        (KeyCode::Up, KeyModifiers::NONE) => move_mcp_elicitation_overlay_selection(app, -1),
        (KeyCode::Down, KeyModifiers::NONE) => move_mcp_elicitation_overlay_selection(app, 1),
        (KeyCode::Enter, KeyModifiers::NONE) => execute_mcp_elicitation_overlay_action(app),
        _ => {}
    }
}

fn cancel_mcp_elicitation_overlay(app: &mut App) {
    let Some(request_id) =
        app.config.mcp_elicitation_overlay().map(|overlay| overlay.request.request_id.clone())
    else {
        app.config.overlay = None;
        return;
    };
    send_mcp_elicitation_response(
        app,
        &request_id,
        crate::agent::types::ElicitationAction::Cancel,
        None,
    );
    app.config.overlay = None;
}

#[derive(Clone, Copy)]
enum McpAuthRedirectAction {
    Refresh,
    CopyUrl,
    Close,
}

fn mcp_auth_redirect_actions() -> [McpAuthRedirectAction; 3] {
    [McpAuthRedirectAction::Refresh, McpAuthRedirectAction::CopyUrl, McpAuthRedirectAction::Close]
}

fn handle_mcp_auth_redirect_overlay_key(app: &mut App, key: KeyEvent) {
    match (key.code, key.modifiers) {
        (KeyCode::Up, KeyModifiers::NONE) => move_mcp_auth_redirect_overlay_selection(app, -1),
        (KeyCode::Down, KeyModifiers::NONE) => move_mcp_auth_redirect_overlay_selection(app, 1),
        (KeyCode::Enter, KeyModifiers::NONE) => execute_mcp_auth_redirect_overlay_action(app),
        (KeyCode::Esc, KeyModifiers::NONE) => app.config.overlay = None,
        _ => {}
    }
}

fn move_mcp_auth_redirect_overlay_selection(app: &mut App, delta: isize) {
    let Some(overlay) = app.config.mcp_auth_redirect_overlay().cloned() else {
        return;
    };
    let actions = mcp_auth_redirect_actions();
    let next_index = step_index_clamped(overlay.selected_index, delta, actions.len());
    if let Some(state) = app.config.mcp_auth_redirect_overlay_mut() {
        state.selected_index = next_index;
    }
}

fn execute_mcp_auth_redirect_overlay_action(app: &mut App) {
    let Some(overlay) = app.config.mcp_auth_redirect_overlay().cloned() else {
        return;
    };
    let actions = mcp_auth_redirect_actions();
    let Some(action) = actions.get(overlay.selected_index).copied() else {
        return;
    };
    match action {
        McpAuthRedirectAction::Refresh => {
            refresh_mcp_snapshot(app);
            app.config.overlay = None;
        }
        McpAuthRedirectAction::CopyUrl => {
            match copy_text_to_clipboard(&overlay.redirect.auth_url) {
                Ok(()) => {
                    app.config.status_message = Some("Copied auth URL to clipboard.".to_owned());
                    app.config.last_error = None;
                }
                Err(error) => {
                    app.config.last_error = Some(error);
                    app.config.status_message = None;
                }
            }
        }
        McpAuthRedirectAction::Close => {
            app.config.overlay = None;
        }
    }
}

fn move_mcp_elicitation_overlay_selection(app: &mut App, delta: isize) {
    let Some(overlay) = app.config.mcp_elicitation_overlay().cloned() else {
        return;
    };
    let actions = mcp_elicitation_actions(&overlay.request);
    if actions.is_empty() {
        return;
    }
    let next_index = step_index_clamped(overlay.selected_index, delta, actions.len());
    if let Some(state) = app.config.mcp_elicitation_overlay_mut() {
        state.selected_index = next_index;
    }
}

fn execute_mcp_elicitation_overlay_action(app: &mut App) {
    let Some(overlay) = app.config.mcp_elicitation_overlay().cloned() else {
        return;
    };
    let actions = mcp_elicitation_actions(&overlay.request);
    let Some(action) = actions.get(overlay.selected_index).copied() else {
        return;
    };
    send_mcp_elicitation_response(app, &overlay.request.request_id, action, None);
    app.config.overlay = None;
}

fn mcp_elicitation_actions(
    request: &crate::agent::types::ElicitationRequest,
) -> Vec<crate::agent::types::ElicitationAction> {
    match request.mode {
        crate::agent::types::ElicitationMode::Url => vec![
            crate::agent::types::ElicitationAction::Accept,
            crate::agent::types::ElicitationAction::Decline,
            crate::agent::types::ElicitationAction::Cancel,
        ],
        crate::agent::types::ElicitationMode::Form => vec![
            crate::agent::types::ElicitationAction::Decline,
            crate::agent::types::ElicitationAction::Cancel,
        ],
    }
}

impl TextInputOverlay for McpCallbackUrlOverlayState {
    fn draft(&self) -> &str {
        &self.draft
    }

    fn draft_mut(&mut self) -> &mut String {
        &mut self.draft
    }

    fn cursor(&self) -> usize {
        self.cursor
    }

    fn cursor_mut(&mut self) -> &mut usize {
        &mut self.cursor
    }
}
