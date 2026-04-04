// Permission grant/deny flow integration tests.
// Validates that PermissionRequest events are correctly attached to tool calls,
// that the pending_interaction_ids queue is maintained, and that responses
// are sent through the oneshot channel.

use claude_code_rust::agent::events::ClientEvent;
use claude_code_rust::agent::model;
use claude_code_rust::app::{AppStatus, MessageBlock};
use pretty_assertions::assert_eq;
use tokio::sync::oneshot;

use crate::helpers::{send_client_event, test_app};

/// Helper: create a tool call, send it, then send a permission request for it.
/// Returns the oneshot receiver so the test can verify the response.
fn setup_permission(
    app: &mut claude_code_rust::app::App,
    tool_id: &str,
    options: Vec<model::PermissionOption>,
) -> oneshot::Receiver<model::RequestPermissionResponse> {
    // First create the tool call so it exists in the index
    let id = tool_id.to_owned();
    let tc = model::ToolCall::new(id, "Write file").status(model::ToolCallStatus::InProgress);
    send_client_event(app, ClientEvent::SessionUpdate(model::SessionUpdate::ToolCall(tc)));

    let (response_tx, response_rx) = oneshot::channel();
    let tool_call_update =
        model::ToolCallUpdate::new(tool_id.to_owned(), model::ToolCallUpdateFields::new());
    let request = model::RequestPermissionRequest::new("test-session", tool_call_update, options);
    send_client_event(app, ClientEvent::PermissionRequest { request, response_tx });
    response_rx
}

fn allow_deny_options() -> Vec<model::PermissionOption> {
    vec![
        model::PermissionOption::new("allow", "Allow", model::PermissionOptionKind::AllowOnce),
        model::PermissionOption::new("deny", "Deny", model::PermissionOptionKind::RejectOnce),
    ]
}

// --- PermissionRequest attaches to tool call ---

#[tokio::test]
async fn permission_request_attaches_to_tool_call() {
    let mut app = test_app();
    let _rx = setup_permission(&mut app, "tc-perm-1", allow_deny_options());

    assert_eq!(app.pending_interaction_ids.len(), 1);
    assert_eq!(app.pending_interaction_ids[0], "tc-perm-1");

    // The tool call should have a pending_permission
    let (mi, bi) = app.tool_call_index["tc-perm-1"];
    if let MessageBlock::ToolCall(tc) = &app.messages[mi].blocks[bi] {
        assert!(tc.pending_permission.is_some());
        let perm = tc.pending_permission.as_ref().unwrap();
        assert_eq!(perm.options.len(), 2);
        assert_eq!(perm.selected_index, 0);
        assert!(perm.focused, "first permission should be focused");
    } else {
        panic!("expected ToolCall block");
    }
}

#[tokio::test]
async fn permission_request_enables_auto_scroll() {
    let mut app = test_app();
    app.viewport.auto_scroll = false;
    let _rx = setup_permission(&mut app, "tc-scroll", allow_deny_options());
    assert!(app.viewport.auto_scroll, "permission request should enable auto_scroll");
}

// --- Permission for unknown tool call auto-rejects ---

#[tokio::test]
async fn permission_for_unknown_tool_call_auto_rejects() {
    let mut app = test_app();

    let (response_tx, mut response_rx) = oneshot::channel();
    let tool_call_update =
        model::ToolCallUpdate::new("nonexistent", model::ToolCallUpdateFields::new());
    let options = allow_deny_options();
    let request = model::RequestPermissionRequest::new("test-session", tool_call_update, options);
    send_client_event(&mut app, ClientEvent::PermissionRequest { request, response_tx });

    // Should NOT be in pending queue
    assert!(app.pending_interaction_ids.is_empty());

    // The response should have been sent (auto-reject with last option = "deny")
    let response = response_rx.try_recv();
    assert!(response.is_ok(), "auto-reject should send response immediately");
    let resp = response.unwrap();
    if let model::RequestPermissionOutcome::Selected(selected) = resp.outcome {
        assert_eq!(selected.option_id.clone(), "deny", "auto-reject should pick last option");
    } else {
        panic!("expected Selected outcome from auto-reject");
    }
}

// --- Multiple permissions queue correctly ---

#[tokio::test]
async fn multiple_permissions_queue_in_order() {
    let mut app = test_app();
    let _rx1 = setup_permission(&mut app, "tc-q1", allow_deny_options());
    let _rx2 = setup_permission(&mut app, "tc-q2", allow_deny_options());

    assert_eq!(app.pending_interaction_ids.len(), 2);
    assert_eq!(app.pending_interaction_ids[0], "tc-q1");
    assert_eq!(app.pending_interaction_ids[1], "tc-q2");

    // First should be focused, second should not
    let (mi1, bi1) = app.tool_call_index["tc-q1"];
    if let MessageBlock::ToolCall(tc) = &app.messages[mi1].blocks[bi1] {
        assert!(tc.pending_permission.as_ref().unwrap().focused);
    }
    let (mi2, bi2) = app.tool_call_index["tc-q2"];
    if let MessageBlock::ToolCall(tc) = &app.messages[mi2].blocks[bi2] {
        assert!(!tc.pending_permission.as_ref().unwrap().focused);
    }
}

#[tokio::test]
async fn duplicate_permission_request_is_rejected_without_duplicate_queue_entry() {
    let mut app = test_app();
    let mut first_rx = setup_permission(&mut app, "tc-dup", allow_deny_options());

    let (response_tx, mut duplicate_rx) = oneshot::channel();
    let tool_call_update = model::ToolCallUpdate::new("tc-dup", model::ToolCallUpdateFields::new());
    let request = model::RequestPermissionRequest::new(
        "test-session",
        tool_call_update,
        allow_deny_options(),
    );
    send_client_event(&mut app, ClientEvent::PermissionRequest { request, response_tx });

    assert_eq!(app.pending_interaction_ids, vec!["tc-dup"]);
    assert!(matches!(first_rx.try_recv(), Err(tokio::sync::oneshot::error::TryRecvError::Empty)));

    let resp = duplicate_rx.try_recv().expect("duplicate permission should be auto-rejected");
    let model::RequestPermissionOutcome::Selected(selected) = resp.outcome else {
        panic!("expected Selected outcome from duplicate auto-reject");
    };
    assert_eq!(selected.option_id.clone(), "deny");
}

// --- Scroll interaction during streaming ---

#[tokio::test]
async fn scroll_target_preserved_across_text_chunks() {
    let mut app = test_app();
    app.viewport.scroll_target = 42;
    app.viewport.auto_scroll = false;

    let chunk =
        model::ContentChunk::new(model::ContentBlock::Text(model::TextContent::new("Some text")));
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::AgentMessageChunk(chunk)),
    );

    // Text chunks should NOT reset scroll when auto_scroll is off
    assert_eq!(app.viewport.scroll_target, 42, "scroll_target should be preserved");
    assert!(!app.viewport.auto_scroll, "auto_scroll should stay off");
}

#[tokio::test]
async fn tool_call_does_not_change_scroll_when_auto_scroll_off() {
    let mut app = test_app();
    app.viewport.scroll_target = 10;
    app.viewport.auto_scroll = false;

    let tc =
        model::ToolCall::new("tc-scroll", "Read file").status(model::ToolCallStatus::InProgress);
    send_client_event(&mut app, ClientEvent::SessionUpdate(model::SessionUpdate::ToolCall(tc)));

    assert_eq!(app.viewport.scroll_target, 10, "tool calls shouldn't touch scroll_target");
    assert!(!app.viewport.auto_scroll);
}

// --- TurnComplete transient state reset ---

#[tokio::test]
async fn turn_complete_resets_transient_state() {
    let mut app = test_app();
    app.status = AppStatus::Running;
    app.files_accessed = 5;
    app.spinner_frame = 42;

    send_client_event(&mut app, ClientEvent::TurnComplete);

    assert!(matches!(app.status, AppStatus::Ready));
    assert_eq!(app.files_accessed, 0, "files_accessed should reset");
    // spinner_frame is a UI detail, not reset by TurnComplete (it's driven by tick)
    // pending_interaction_ids should be empty (no permissions were pending)
    assert!(app.pending_interaction_ids.is_empty());
}

#[tokio::test]
async fn turn_complete_does_not_clear_messages() {
    let mut app = test_app();

    let chunk =
        model::ContentChunk::new(model::ContentBlock::Text(model::TextContent::new("hello")));
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::AgentMessageChunk(chunk)),
    );
    assert_eq!(app.messages.len(), 1);

    send_client_event(&mut app, ClientEvent::TurnComplete);

    assert_eq!(app.messages.len(), 1, "messages should persist across turns");
}

#[tokio::test]
async fn turn_complete_does_not_clear_tool_call_index() {
    let mut app = test_app();

    let tc =
        model::ToolCall::new("tc-persist", "Read file").status(model::ToolCallStatus::InProgress);
    send_client_event(&mut app, ClientEvent::SessionUpdate(model::SessionUpdate::ToolCall(tc)));
    assert!(app.tool_call_index.contains_key("tc-persist"));

    send_client_event(&mut app, ClientEvent::TurnComplete);

    assert!(
        app.tool_call_index.contains_key("tc-persist"),
        "tool_call_index should persist across turns"
    );
}

#[tokio::test]
async fn turn_complete_does_not_clear_todos() {
    let mut app = test_app();

    // Simulate a TodoWrite by directly setting todos
    app.todos = vec![claude_code_rust::app::TodoItem {
        content: "Test task".into(),
        status: claude_code_rust::app::TodoStatus::InProgress,
        active_form: "Testing".into(),
    }];
    app.show_todo_panel = true;

    send_client_event(&mut app, ClientEvent::TurnComplete);

    assert_eq!(app.todos.len(), 1, "todos should persist across turns");
    assert!(app.show_todo_panel, "todo panel state should persist");
}

#[tokio::test]
async fn turn_complete_does_not_affect_mode() {
    let mut app = test_app();

    app.mode = Some(claude_code_rust::app::ModeState {
        current_mode_id: "plan".into(),
        current_mode_name: "Plan".into(),
        available_modes: vec![claude_code_rust::app::ModeInfo {
            id: "plan".into(),
            name: "Plan".into(),
        }],
    });

    send_client_event(&mut app, ClientEvent::TurnComplete);

    assert!(app.mode.is_some(), "mode should persist across turns");
}
