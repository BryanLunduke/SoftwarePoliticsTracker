// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::{App, FocusTarget, TodoItem, TodoStatus};
use crate::agent::model;

/// Parse a `TodoWrite` `raw_input` JSON value into a `Vec<TodoItem>`.
/// Expected shape: `{"todos": [{"content": "...", "status": "...", "activeForm": "..."}]}`
#[cfg(test)]
pub(super) fn parse_todos(raw_input: &serde_json::Value) -> Vec<TodoItem> {
    parse_todos_if_present(raw_input).unwrap_or_default()
}

/// Parse todos only when a concrete `todos` array is present in `raw_input`.
/// Returns `None` for transient/incomplete payloads (missing or non-array `todos`).
pub(super) fn parse_todos_if_present(raw_input: &serde_json::Value) -> Option<Vec<TodoItem>> {
    let arr = raw_input.get("todos")?.as_array()?;
    Some(
        arr.iter()
            .filter_map(|item| {
                let content = item.get("content")?.as_str()?.to_owned();
                let status_str = item.get("status")?.as_str()?;
                let active_form =
                    item.get("activeForm").and_then(|v| v.as_str()).unwrap_or("").to_owned();
                let status = match status_str {
                    "in_progress" => TodoStatus::InProgress,
                    "completed" => TodoStatus::Completed,
                    _ => TodoStatus::Pending,
                };
                Some(TodoItem { content, status, active_form })
            })
            .collect(),
    )
}

pub(super) fn set_todos(app: &mut App, todos: Vec<TodoItem>) {
    app.cached_todo_compact = None;
    if todos.is_empty() {
        app.todos.clear();
        app.show_todo_panel = false;
        app.todo_scroll = 0;
        app.todo_selected = 0;
        app.release_focus_target(FocusTarget::TodoList);
        return;
    }

    let all_done = todos.iter().all(|t| t.status == TodoStatus::Completed);
    if all_done {
        app.todos.clear();
        app.show_todo_panel = false;
        app.todo_scroll = 0;
        app.todo_selected = 0;
        app.release_focus_target(FocusTarget::TodoList);
    } else {
        app.todos = todos;
        if app.todo_selected >= app.todos.len() {
            app.todo_selected = app.todos.len().saturating_sub(1);
        }
        if !app.show_todo_panel {
            app.release_focus_target(FocusTarget::TodoList);
        }
    }
}

/// Convert bridge plan entries into the local todo list.
pub(super) fn apply_plan_todos(app: &mut App, plan: &model::Plan) {
    app.cached_todo_compact = None;
    let mut todos = Vec::with_capacity(plan.entries.len());
    for entry in &plan.entries {
        let status_str = format!("{:?}", entry.status);
        let status = match status_str.as_str() {
            "InProgress" => TodoStatus::InProgress,
            "Completed" => TodoStatus::Completed,
            _ => TodoStatus::Pending,
        };
        todos.push(TodoItem { content: entry.content.clone(), status, active_form: String::new() });
    }
    set_todos(app, todos);
}

#[cfg(test)]
mod tests {
    // =====
    // TESTS: 32
    // =====

    use super::*;
    use pretty_assertions::assert_eq;
    use serde_json::json;

    // parse_todos

    #[test]
    fn parse_valid_all_statuses() {
        let input = json!({
            "todos": [
                {"content": "Task A", "status": "pending", "activeForm": "Doing A"},
                {"content": "Task B", "status": "in_progress", "activeForm": "Doing B"},
                {"content": "Task C", "status": "completed", "activeForm": "Done C"},
            ]
        });
        let todos = parse_todos(&input);
        assert_eq!(todos.len(), 3);
        assert_eq!(todos[0].content, "Task A");
        assert_eq!(todos[0].status, TodoStatus::Pending);
        assert_eq!(todos[0].active_form, "Doing A");
        assert_eq!(todos[1].status, TodoStatus::InProgress);
        assert_eq!(todos[2].status, TodoStatus::Completed);
    }

    #[test]
    fn parse_missing_active_form_defaults_empty() {
        let input = json!({
            "todos": [{"content": "Task", "status": "pending"}]
        });
        let todos = parse_todos(&input);
        assert_eq!(todos.len(), 1);
        assert_eq!(todos[0].active_form, "");
    }

    #[test]
    fn parse_empty_array() {
        let input = json!({"todos": []});
        let todos = parse_todos(&input);
        assert!(todos.is_empty());
    }

    #[test]
    fn parse_if_present_missing_todos_returns_none() {
        let input = json!({"other": 1});
        let todos = parse_todos_if_present(&input);
        assert!(todos.is_none());
    }

    #[test]
    fn parse_if_present_non_array_todos_returns_none() {
        let input = json!({"todos": {"not": "array"}});
        let todos = parse_todos_if_present(&input);
        assert!(todos.is_none());
    }

    #[test]
    fn parse_if_present_empty_array_returns_some_empty_vec() {
        let input = json!({"todos": []});
        let todos = parse_todos_if_present(&input);
        assert!(matches!(todos, Some(v) if v.is_empty()));
    }

    // parse_todos

    #[test]
    fn parse_missing_todos_key() {
        let input = json!({"something_else": 42});
        let todos = parse_todos(&input);
        assert!(todos.is_empty());
    }

    #[test]
    fn parse_todos_not_array() {
        let input = json!({"todos": "not an array"});
        let todos = parse_todos(&input);
        assert!(todos.is_empty());
    }

    #[test]
    fn parse_missing_content_skips_item() {
        let input = json!({
            "todos": [
                {"status": "pending", "activeForm": "Missing content"},
                {"content": "Valid", "status": "pending"},
            ]
        });
        let todos = parse_todos(&input);
        assert_eq!(todos.len(), 1);
        assert_eq!(todos[0].content, "Valid");
    }

    #[test]
    fn parse_missing_status_skips_item() {
        let input = json!({
            "todos": [
                {"content": "No status"},
                {"content": "Valid", "status": "pending"},
            ]
        });
        let todos = parse_todos(&input);
        assert_eq!(todos.len(), 1);
    }

    #[test]
    fn parse_unknown_status_maps_to_pending() {
        let input = json!({
            "todos": [{"content": "Task", "status": "banana"}]
        });
        let todos = parse_todos(&input);
        assert_eq!(todos[0].status, TodoStatus::Pending);
    }

    // parse_todos

    #[test]
    fn parse_null_input() {
        let input = serde_json::Value::Null;
        let todos = parse_todos(&input);
        assert!(todos.is_empty());
    }

    #[test]
    fn parse_content_is_number_not_string() {
        let input = json!({
            "todos": [{"content": 42, "status": "pending"}]
        });
        let todos = parse_todos(&input);
        assert!(todos.is_empty()); // content.as_str() returns None for number
    }

    #[test]
    fn parse_large_todo_list() {
        let items: Vec<serde_json::Value> = (0..100)
            .map(|i| json!({"content": format!("Task {i}"), "status": "pending"}))
            .collect();
        let input = json!({"todos": items});
        let todos = parse_todos(&input);
        assert_eq!(todos.len(), 100);
    }

    #[test]
    fn parse_mixed_valid_and_invalid() {
        let input = json!({
            "todos": [
                {"content": "Good", "status": "completed"},
                {},
                {"content": "Also good", "status": "in_progress"},
                {"status": "pending"},
                null,
            ]
        });
        let todos = parse_todos(&input);
        assert_eq!(todos.len(), 2);
        assert_eq!(todos[0].content, "Good");
        assert_eq!(todos[1].content, "Also good");
    }

    // weird JSON inputs

    #[test]
    fn parse_unicode_content_and_active_form() {
        let input = json!({
            "todos": [{"content": "\u{1F680} Deploy to prod", "status": "in_progress", "activeForm": "\u{1F525} Deploying"}]
        });
        let todos = parse_todos(&input);
        assert_eq!(todos[0].content, "\u{1F680} Deploy to prod");
        assert_eq!(todos[0].active_form, "\u{1F525} Deploying");
    }

    #[test]
    fn parse_empty_string_content() {
        let input = json!({
            "todos": [{"content": "", "status": "pending"}]
        });
        let todos = parse_todos(&input);
        assert_eq!(todos.len(), 1);
        assert_eq!(todos[0].content, "");
    }

    #[test]
    fn parse_empty_string_status() {
        let input = json!({
            "todos": [{"content": "Task", "status": ""}]
        });
        let todos = parse_todos(&input);
        // Empty string doesn't match "in_progress" or "completed" -> Pending
        assert_eq!(todos[0].status, TodoStatus::Pending);
    }

    #[test]
    fn parse_status_is_boolean() {
        let input = json!({
            "todos": [{"content": "Task", "status": true}]
        });
        let todos = parse_todos(&input);
        assert!(todos.is_empty()); // status.as_str() returns None for bool
    }

    #[test]
    fn parse_status_is_array() {
        let input = json!({
            "todos": [{"content": "Task", "status": ["pending"]}]
        });
        let todos = parse_todos(&input);
        assert!(todos.is_empty());
    }

    #[test]
    fn parse_status_is_nested_object() {
        let input = json!({
            "todos": [{"content": "Task", "status": {"value": "pending"}}]
        });
        let todos = parse_todos(&input);
        assert!(todos.is_empty());
    }

    #[test]
    fn parse_active_form_is_number() {
        let input = json!({
            "todos": [{"content": "Task", "status": "pending", "activeForm": 42}]
        });
        let todos = parse_todos(&input);
        // activeForm.as_str() returns None -> unwrap_or("") -> ""
        assert_eq!(todos[0].active_form, "");
    }

    #[test]
    fn parse_extra_keys_ignored() {
        let input = json!({
            "todos": [{
                "content": "Task",
                "status": "pending",
                "activeForm": "Doing",
                "extraKey": "should be ignored",
                "priority": 1,
                "nested": {"a": "b"}
            }]
        });
        let todos = parse_todos(&input);
        assert_eq!(todos.len(), 1);
        assert_eq!(todos[0].content, "Task");
    }

    #[test]
    fn parse_todos_key_is_null() {
        let input = json!({"todos": null});
        let todos = parse_todos(&input);
        assert!(todos.is_empty());
    }

    #[test]
    fn parse_todos_key_is_object() {
        let input = json!({"todos": {"not": "an array"}});
        let todos = parse_todos(&input);
        assert!(todos.is_empty());
    }

    #[test]
    fn parse_todos_key_is_number() {
        let input = json!({"todos": 42});
        let todos = parse_todos(&input);
        assert!(todos.is_empty());
    }

    #[test]
    fn parse_very_long_content() {
        let long_content = "A".repeat(10_000);
        let input = json!({
            "todos": [{"content": long_content, "status": "pending"}]
        });
        let todos = parse_todos(&input);
        assert_eq!(todos[0].content.len(), 10_000);
    }

    #[test]
    fn parse_content_with_newlines_and_special_chars() {
        let input = json!({
            "todos": [{"content": "line1\nline2\ttab\r\nwindows", "status": "pending"}]
        });
        let todos = parse_todos(&input);
        assert!(todos[0].content.contains('\n'));
        assert!(todos[0].content.contains('\t'));
    }

    #[test]
    fn parse_deeply_nested_json_value() {
        // The input itself is a deeply nested object -- todos key still works
        let input = json!({
            "metadata": {"nested": {"deep": true}},
            "todos": [{"content": "Found it", "status": "completed"}],
            "other": [1, 2, 3]
        });
        let todos = parse_todos(&input);
        assert_eq!(todos.len(), 1);
        assert_eq!(todos[0].content, "Found it");
    }

    #[test]
    fn parse_duplicate_items() {
        let input = json!({
            "todos": [
                {"content": "Same", "status": "pending"},
                {"content": "Same", "status": "pending"},
                {"content": "Same", "status": "pending"},
            ]
        });
        let todos = parse_todos(&input);
        assert_eq!(todos.len(), 3); // duplicates are fine
    }

    #[test]
    fn parse_status_case_sensitive() {
        // "In_Progress", "COMPLETED" should NOT match -> Pending
        let input = json!({
            "todos": [
                {"content": "A", "status": "In_Progress"},
                {"content": "B", "status": "COMPLETED"},
                {"content": "C", "status": "Pending"},
            ]
        });
        let todos = parse_todos(&input);
        assert_eq!(todos[0].status, TodoStatus::Pending);
        assert_eq!(todos[1].status, TodoStatus::Pending);
        assert_eq!(todos[2].status, TodoStatus::Pending); // "Pending" != "pending"
    }

    #[test]
    fn parse_array_input_not_object() {
        // Top-level is an array, not an object -- no "todos" key
        let input = json!([{"content": "Task", "status": "pending"}]);
        let todos = parse_todos(&input);
        assert!(todos.is_empty());
    }

    #[test]
    fn parse_string_input() {
        let input = json!("just a string");
        let todos = parse_todos(&input);
        assert!(todos.is_empty());
    }

    #[test]
    fn parse_boolean_input() {
        let input = json!(true);
        let todos = parse_todos(&input);
        assert!(todos.is_empty());
    }

    #[test]
    fn parse_number_input() {
        let input = json!(42);
        let todos = parse_todos(&input);
        assert!(todos.is_empty());
    }
}
