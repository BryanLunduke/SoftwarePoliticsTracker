use claude_code_rust::agent::events::ClientEvent;
use claude_code_rust::app::App;

/// Build a minimal `App` for integration testing.
/// No real bridge connection, no TUI -- just state.
pub fn test_app() -> App {
    App::test_default()
}

/// Helper: send a client event into the app's event handling pipeline.
pub fn send_client_event(app: &mut App, event: ClientEvent) {
    claude_code_rust::app::handle_client_event(app, event);
}
