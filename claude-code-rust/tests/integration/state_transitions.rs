// =====
// TESTS: 19
// =====
//
// State transition integration tests.
// Validates multi-event sequences and App state consistency.

use claude_code_rust::agent::events::ClientEvent;
use claude_code_rust::agent::model;
use claude_code_rust::app::{AppStatus, MessageBlock, MessageRole};
use pretty_assertions::assert_eq;

use crate::helpers::{send_client_event, test_app};

// --- Full turn lifecycle ---

#[tokio::test]
async fn full_turn_lifecycle_text_only() {
    let mut app = test_app();
    assert!(matches!(app.status, AppStatus::Ready));

    // Agent starts thinking (thought chunk)
    let thought =
        model::ContentChunk::new(model::ContentBlock::Text(model::TextContent::new("Planning...")));
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::AgentThoughtChunk(thought)),
    );
    assert!(matches!(app.status, AppStatus::Thinking));

    // Agent streams text
    let chunk = model::ContentChunk::new(model::ContentBlock::Text(model::TextContent::new(
        "Here is my answer.",
    )));
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::AgentMessageChunk(chunk)),
    );
    assert!(matches!(app.status, AppStatus::Running));

    // Turn completes
    send_client_event(&mut app, ClientEvent::TurnComplete);
    assert!(matches!(app.status, AppStatus::Ready));
    assert_eq!(app.messages.len(), 1);
}

#[tokio::test]
async fn full_turn_lifecycle_with_tool_calls() {
    let mut app = test_app();

    // Text chunk
    let chunk = model::ContentChunk::new(model::ContentBlock::Text(model::TextContent::new(
        "Let me check.",
    )));
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::AgentMessageChunk(chunk)),
    );

    // Tool call
    let tc = model::ToolCall::new("tc-flow", "Read src/lib.rs")
        .kind(model::ToolKind::Read)
        .status(model::ToolCallStatus::InProgress);
    send_client_event(&mut app, ClientEvent::SessionUpdate(model::SessionUpdate::ToolCall(tc)));

    // Tool completes
    let fields = model::ToolCallUpdateFields::new().status(model::ToolCallStatus::Completed);
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::ToolCallUpdate(
            model::ToolCallUpdate::new("tc-flow", fields),
        )),
    );
    assert!(matches!(app.status, AppStatus::Thinking));

    // More text
    let chunk2 = model::ContentChunk::new(model::ContentBlock::Text(model::TextContent::new(
        " The file looks good.",
    )));
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::AgentMessageChunk(chunk2)),
    );

    // Turn completes
    send_client_event(&mut app, ClientEvent::TurnComplete);
    assert!(matches!(app.status, AppStatus::Ready));
}

// --- TodoWrite handling ---

#[tokio::test]
async fn todowrite_tool_call_updates_todo_list() {
    let mut app = test_app();

    let raw_input = serde_json::json!({
        "todos": [
            {"content": "Fix bug", "status": "in_progress", "activeForm": "Fixing bug"},
            {"content": "Write tests", "status": "pending", "activeForm": "Writing tests"},
        ]
    });

    let mut meta = serde_json::Map::new();
    meta.insert("claudeCode".into(), serde_json::json!({"toolName": "TodoWrite"}));
    let tc = model::ToolCall::new("todo-1", "TodoWrite")
        .kind(model::ToolKind::Other)
        .status(model::ToolCallStatus::InProgress)
        .raw_input(raw_input)
        .meta(meta);
    send_client_event(&mut app, ClientEvent::SessionUpdate(model::SessionUpdate::ToolCall(tc)));

    assert_eq!(app.todos.len(), 2);
    assert_eq!(app.todos[0].content, "Fix bug");
    assert_eq!(app.todos[1].content, "Write tests");
    // show_todo_panel is user-toggled (Ctrl+T), not auto-shown on TodoWrite
    assert!(!app.show_todo_panel);
}

#[tokio::test]
async fn todowrite_all_completed_hides_panel() {
    let mut app = test_app();

    let raw_input = serde_json::json!({
        "todos": [
            {"content": "Done task", "status": "completed", "activeForm": "Done"},
        ]
    });

    let mut meta = serde_json::Map::new();
    meta.insert("claudeCode".into(), serde_json::json!({"toolName": "TodoWrite"}));
    let tc = model::ToolCall::new("todo-done", "TodoWrite")
        .kind(model::ToolKind::Other)
        .status(model::ToolCallStatus::InProgress)
        .raw_input(raw_input)
        .meta(meta);
    send_client_event(&mut app, ClientEvent::SessionUpdate(model::SessionUpdate::ToolCall(tc)));

    assert!(app.todos.is_empty(), "all-completed clears the list");
    assert!(!app.show_todo_panel, "panel hidden when all done");
}

// --- Error recovery ---

#[tokio::test]
async fn error_then_new_turn_recovers() {
    let mut app = test_app();

    send_client_event(&mut app, ClientEvent::TurnError("timeout".into()));
    assert!(matches!(app.status, AppStatus::Error));

    // New text chunk (simulates user retry) starts fresh
    let chunk = model::ContentChunk::new(model::ContentBlock::Text(model::TextContent::new(
        "Retry answer",
    )));
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::AgentMessageChunk(chunk)),
    );
    assert!(matches!(app.status, AppStatus::Running));
}

// --- Message accumulation ---

#[tokio::test]
async fn chunks_across_turns_append_to_last_assistant_message() {
    let mut app = test_app();

    // First turn
    let c1 = model::ContentChunk::new(model::ContentBlock::Text(model::TextContent::new("Turn 1")));
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::AgentMessageChunk(c1)),
    );
    send_client_event(&mut app, ClientEvent::TurnComplete);
    assert_eq!(app.messages.len(), 1);

    // Second turn: chunks append to the last assistant message (no user message between turns)
    let c2 = model::ContentChunk::new(model::ContentBlock::Text(model::TextContent::new("Turn 2")));
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::AgentMessageChunk(c2)),
    );

    // Still one message - consecutive assistant chunks always merge
    assert_eq!(app.messages.len(), 1);
    if let MessageBlock::Text(block) =
        &app.messages.last().expect("message").blocks.last().expect("block")
    {
        assert!(block.text.contains("Turn 1"), "first turn text present");
        assert!(block.text.contains("Turn 2"), "second turn text appended");
    }
}

#[tokio::test]
async fn tool_call_content_update() {
    let mut app = test_app();

    let tc =
        model::ToolCall::new("tc-content", "Read file").status(model::ToolCallStatus::InProgress);
    send_client_event(&mut app, ClientEvent::SessionUpdate(model::SessionUpdate::ToolCall(tc)));

    // Update with content
    let content = vec![model::ToolCallContent::from("file contents here")];
    let fields = model::ToolCallUpdateFields::new()
        .content(content)
        .status(model::ToolCallStatus::Completed);
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::ToolCallUpdate(
            model::ToolCallUpdate::new("tc-content", fields),
        )),
    );

    let (mi, bi) = app.tool_call_index["tc-content"];
    if let MessageBlock::ToolCall(tc) = &app.messages[mi].blocks[bi] {
        assert!(!tc.content.is_empty(), "content should be set");
    } else {
        panic!("expected ToolCall block");
    }
}

// --- Auto-scroll ---

#[tokio::test]
async fn auto_scroll_maintained_during_streaming() {
    let mut app = test_app();
    assert!(app.viewport.auto_scroll);

    for _ in 0..20 {
        let chunk = model::ContentChunk::new(model::ContentBlock::Text(model::TextContent::new(
            "More text. ",
        )));
        send_client_event(
            &mut app,
            ClientEvent::SessionUpdate(model::SessionUpdate::AgentMessageChunk(chunk)),
        );
    }

    assert!(app.viewport.auto_scroll, "auto_scroll should stay true during streaming");
}

// --- Stress: many tool calls in one turn ---

#[tokio::test]
async fn stress_many_tool_calls_in_one_turn() {
    let mut app = test_app();
    app.status = AppStatus::Running;

    for i in 0..50 {
        let tc = model::ToolCall::new(format!("stress-{i}"), format!("Op {i}"))
            .status(model::ToolCallStatus::InProgress);
        send_client_event(&mut app, ClientEvent::SessionUpdate(model::SessionUpdate::ToolCall(tc)));
    }

    assert_eq!(app.tool_call_index.len(), 50);

    // Complete all
    for i in 0..50 {
        let fields = model::ToolCallUpdateFields::new().status(model::ToolCallStatus::Completed);
        send_client_event(
            &mut app,
            ClientEvent::SessionUpdate(model::SessionUpdate::ToolCallUpdate(
                model::ToolCallUpdate::new(format!("stress-{i}"), fields),
            )),
        );
    }

    assert!(matches!(app.status, AppStatus::Thinking));
}

// --- CurrentModeUpdate ---

#[tokio::test]
async fn mode_update_switches_active_mode() {
    let mut app = test_app();

    // Initialize with two modes, "code" active
    app.mode = Some(claude_code_rust::app::ModeState {
        current_mode_id: "code".into(),
        current_mode_name: "Code".into(),
        available_modes: vec![
            claude_code_rust::app::ModeInfo { id: "code".into(), name: "Code".into() },
            claude_code_rust::app::ModeInfo { id: "plan".into(), name: "Plan".into() },
        ],
    });

    // CurrentModeUpdate switches to "plan"
    let update = model::CurrentModeUpdate::new("plan");
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::CurrentModeUpdate(update)),
    );

    let mode = app.mode.as_ref().expect("mode should still exist");
    assert_eq!(mode.current_mode_id, "plan");
    assert_eq!(mode.current_mode_name, "Plan", "name resolved from available_modes");
    assert_eq!(mode.available_modes.len(), 2, "available_modes unchanged");
}

#[tokio::test]
async fn mode_update_unknown_id_uses_id_as_name() {
    let mut app = test_app();

    app.mode = Some(claude_code_rust::app::ModeState {
        current_mode_id: "code".into(),
        current_mode_name: "Code".into(),
        available_modes: vec![claude_code_rust::app::ModeInfo {
            id: "code".into(),
            name: "Code".into(),
        }],
    });

    // Update with an ID not in available_modes
    let update = model::CurrentModeUpdate::new("unknown-mode");
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::CurrentModeUpdate(update)),
    );

    let mode = app.mode.as_ref().unwrap();
    assert_eq!(mode.current_mode_id, "unknown-mode");
    assert_eq!(mode.current_mode_name, "unknown-mode", "falls back to ID as name");
}

#[tokio::test]
async fn mode_update_without_mode_state_is_noop() {
    let mut app = test_app();
    assert!(app.mode.is_none());

    let update = model::CurrentModeUpdate::new("plan-mode");
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::CurrentModeUpdate(update)),
    );

    // No crash, mode stays None since no ModeState was initialized
    assert!(app.mode.is_none());
}

// --- Edge cases: interleaved events ---

#[tokio::test]
async fn text_between_tool_calls_creates_separate_blocks() {
    let mut app = test_app();

    let c1 =
        model::ContentChunk::new(model::ContentBlock::Text(model::TextContent::new("Before tool")));
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::AgentMessageChunk(c1)),
    );

    let tc =
        model::ToolCall::new("tc-inter", "Read file").status(model::ToolCallStatus::InProgress);
    send_client_event(&mut app, ClientEvent::SessionUpdate(model::SessionUpdate::ToolCall(tc)));

    let c2 =
        model::ContentChunk::new(model::ContentBlock::Text(model::TextContent::new("After tool")));
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::AgentMessageChunk(c2)),
    );

    let tc2 =
        model::ToolCall::new("tc-inter2", "Write file").status(model::ToolCallStatus::InProgress);
    send_client_event(&mut app, ClientEvent::SessionUpdate(model::SessionUpdate::ToolCall(tc2)));

    let c3 =
        model::ContentChunk::new(model::ContentBlock::Text(model::TextContent::new("Final text")));
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::AgentMessageChunk(c3)),
    );

    // Should be: Text, ToolCall, Text, ToolCall, Text = 5 blocks
    assert_eq!(app.messages.len(), 1);
    assert_eq!(app.messages[0].blocks.len(), 5);
    assert!(matches!(app.messages[0].blocks[0], MessageBlock::Text(..)));
    assert!(matches!(app.messages[0].blocks[1], MessageBlock::ToolCall(_)));
    assert!(matches!(app.messages[0].blocks[2], MessageBlock::Text(..)));
    assert!(matches!(app.messages[0].blocks[3], MessageBlock::ToolCall(_)));
    assert!(matches!(app.messages[0].blocks[4], MessageBlock::Text(..)));
}

#[tokio::test]
async fn rapid_turn_complete_then_new_streaming() {
    let mut app = test_app();

    // First turn
    let c1 = model::ContentChunk::new(model::ContentBlock::Text(model::TextContent::new("Turn 1")));
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::AgentMessageChunk(c1)),
    );
    send_client_event(&mut app, ClientEvent::TurnComplete);
    assert!(matches!(app.status, AppStatus::Ready));
    assert_eq!(app.files_accessed, 0);

    // Immediately start second turn
    let c2 = model::ContentChunk::new(model::ContentBlock::Text(model::TextContent::new("Turn 2")));
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::AgentMessageChunk(c2)),
    );
    assert!(matches!(app.status, AppStatus::Running));

    let tc = model::ToolCall::new("tc-t2", "Read file").status(model::ToolCallStatus::InProgress);
    send_client_event(&mut app, ClientEvent::SessionUpdate(model::SessionUpdate::ToolCall(tc)));
    assert_eq!(app.files_accessed, 1);

    send_client_event(&mut app, ClientEvent::TurnComplete);
    assert!(matches!(app.status, AppStatus::Ready));
    assert_eq!(app.files_accessed, 0, "reset again on second TurnComplete");
}

#[tokio::test]
async fn todowrite_replaces_previous_todos() {
    let mut app = test_app();

    // First TodoWrite with 2 items
    let raw1 = serde_json::json!({"todos": [
        {"content": "Task A", "status": "in_progress", "activeForm": "Doing A"},
        {"content": "Task B", "status": "pending", "activeForm": "Doing B"},
    ]});
    let mut meta1 = serde_json::Map::new();
    meta1.insert("claudeCode".into(), serde_json::json!({"toolName": "TodoWrite"}));
    let tc1 = model::ToolCall::new("todo-r1", "TodoWrite")
        .status(model::ToolCallStatus::InProgress)
        .raw_input(raw1)
        .meta(meta1);
    send_client_event(&mut app, ClientEvent::SessionUpdate(model::SessionUpdate::ToolCall(tc1)));
    assert_eq!(app.todos.len(), 2);

    // Second TodoWrite replaces with 1 item
    let raw2 = serde_json::json!({"todos": [
        {"content": "Task C", "status": "pending", "activeForm": "Doing C"},
    ]});
    let mut meta2 = serde_json::Map::new();
    meta2.insert("claudeCode".into(), serde_json::json!({"toolName": "TodoWrite"}));
    let tc2 = model::ToolCall::new("todo-r2", "TodoWrite")
        .status(model::ToolCallStatus::InProgress)
        .raw_input(raw2)
        .meta(meta2);
    send_client_event(&mut app, ClientEvent::SessionUpdate(model::SessionUpdate::ToolCall(tc2)));

    assert_eq!(app.todos.len(), 1, "second TodoWrite replaces first");
    assert_eq!(app.todos[0].content, "Task C");
}

#[tokio::test]
async fn available_commands_update_replaces_previous() {
    let mut app = test_app();

    let cmd1 = model::AvailableCommand::new("/help", "Help");
    let cmd2 = model::AvailableCommand::new("/clear", "Clear");
    let update1 = model::AvailableCommandsUpdate::new(vec![cmd1, cmd2]);
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::AvailableCommandsUpdate(update1)),
    );
    assert_eq!(app.available_commands.len(), 2);

    // New update replaces, not appends
    let cmd3 = model::AvailableCommand::new("/commit", "Commit");
    let update2 = model::AvailableCommandsUpdate::new(vec![cmd3]);
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::AvailableCommandsUpdate(update2)),
    );
    assert_eq!(app.available_commands.len(), 1, "replaced, not appended");
}

#[tokio::test]
async fn empty_todowrite_clears_todos() {
    let mut app = test_app();

    // Set up some todos first
    let raw1 = serde_json::json!({"todos": [
        {"content": "Task A", "status": "pending", "activeForm": "Doing A"},
    ]});
    let mut meta1 = serde_json::Map::new();
    meta1.insert("claudeCode".into(), serde_json::json!({"toolName": "TodoWrite"}));
    let tc1 = model::ToolCall::new("todo-e1", "TodoWrite")
        .status(model::ToolCallStatus::InProgress)
        .raw_input(raw1)
        .meta(meta1);
    send_client_event(&mut app, ClientEvent::SessionUpdate(model::SessionUpdate::ToolCall(tc1)));
    assert_eq!(app.todos.len(), 1);

    // Empty TodoWrite clears
    let raw2 = serde_json::json!({"todos": []});
    let mut meta2 = serde_json::Map::new();
    meta2.insert("claudeCode".into(), serde_json::json!({"toolName": "TodoWrite"}));
    let tc2 = model::ToolCall::new("todo-e2", "TodoWrite")
        .status(model::ToolCallStatus::InProgress)
        .raw_input(raw2)
        .meta(meta2);
    send_client_event(&mut app, ClientEvent::SessionUpdate(model::SessionUpdate::ToolCall(tc2)));

    assert!(app.todos.is_empty(), "empty todo list clears");
}

#[tokio::test]
async fn error_during_tool_calls_leaves_tool_calls_intact() {
    let mut app = test_app();

    let c = model::ContentChunk::new(model::ContentBlock::Text(model::TextContent::new("working")));
    send_client_event(
        &mut app,
        ClientEvent::SessionUpdate(model::SessionUpdate::AgentMessageChunk(c)),
    );

    let tc = model::ToolCall::new("tc-err", "Read file").status(model::ToolCallStatus::InProgress);
    send_client_event(&mut app, ClientEvent::SessionUpdate(model::SessionUpdate::ToolCall(tc)));

    send_client_event(&mut app, ClientEvent::TurnError("crashed".into()));

    assert!(matches!(app.status, AppStatus::Error));
    // Tool call should remain indexed and preserved in the original assistant message.
    assert!(app.tool_call_index.contains_key("tc-err"));
    assert_eq!(app.messages.len(), 2, "assistant message + system error message");
    assert!(matches!(app.messages[0].role, MessageRole::Assistant));
    assert_eq!(app.messages[0].blocks.len(), 2, "text + tool call preserved");
    let Some(MessageBlock::ToolCall(tc)) = app.messages[0].blocks.get(1) else {
        panic!("expected preserved tool call block");
    };
    assert_eq!(tc.id, "tc-err");
    assert_eq!(tc.status, model::ToolCallStatus::Failed, "in-progress tool should be failed");

    assert!(matches!(app.messages[1].role, MessageRole::System(_)));
    let Some(MessageBlock::Text(block)) = app.messages[1].blocks.first() else {
        panic!("expected system error text block");
    };
    assert!(block.text.contains("Turn failed: crashed"));
}

#[tokio::test]
async fn files_accessed_accumulates_across_tool_calls_in_one_turn() {
    let mut app = test_app();

    for i in 0..3 {
        let tc = model::ToolCall::new(format!("tc-acc-{i}"), format!("Read {i}"))
            .status(model::ToolCallStatus::InProgress);
        send_client_event(&mut app, ClientEvent::SessionUpdate(model::SessionUpdate::ToolCall(tc)));
    }

    assert_eq!(app.files_accessed, 3, "one per tool call");
    send_client_event(&mut app, ClientEvent::TurnComplete);
    assert_eq!(app.files_accessed, 0, "reset on turn complete");
}
