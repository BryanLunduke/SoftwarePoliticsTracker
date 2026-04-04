// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use ratatui::style::Color;

// Accent
pub const RUST_ORANGE: Color = Color::Rgb(244, 118, 0);

// UI chrome
pub const DIM: Color = Color::DarkGray;
pub const PROMPT_CHAR: &str = "\u{276f}";
pub const SEPARATOR_CHAR: &str = "\u{2500}";

// Role header colors
pub const ROLE_ASSISTANT: Color = RUST_ORANGE;

// User message background
pub const USER_MSG_BG: Color = Color::Rgb(40, 44, 52);

// Tool status icons
pub const ICON_COMPLETED: &str = "\u{2713}";
pub const ICON_FAILED: &str = "\u{2717}";

// Status colors
pub const STATUS_ERROR: Color = Color::Red;
pub const STATUS_WARNING: Color = Color::Yellow;
pub const SLASH_COMMAND: Color = Color::LightMagenta;
pub const SUBAGENT_TOKEN: Color = Color::LightBlue;

/// SDK tool icon + label pair. Monochrome Unicode symbols.
/// Unknown tool names fall back to a generic Tool label.
pub fn tool_name_label(sdk_tool_name: &str) -> (&'static str, &'static str) {
    match sdk_tool_name {
        "Read" => ("\u{2b1a}", "Read"),
        "Write" => ("\u{25a3}", "Write"),
        "Edit" => ("\u{25a3}", "Edit"),
        "MultiEdit" => ("\u{25a3}", "MultiEdit"),
        "NotebookEdit" => ("\u{25a3}", "NotebookEdit"),
        "Delete" => ("\u{25a3}", "Delete"),
        "Move" => ("\u{21c4}", "Move"),
        "Glob" => ("\u{2315}", "Glob"),
        "Grep" => ("\u{2315}", "Grep"),
        "LS" => ("\u{2315}", "LS"),
        "Bash" => ("\u{27e9}", "Bash"),
        "Task" | "Agent" => ("\u{25c7}", "Subagent"),
        "WebFetch" => ("\u{2295}", "WebFetch"),
        "WebSearch" => ("\u{2295}", "WebSearch"),
        "ExitPlanMode" => ("\u{2299}", "ExitPlanMode"),
        "TodoWrite" => ("\u{25cc}", "TodoWrite"),
        "Config" => ("\u{2299}", "Config"),
        "EnterWorktree" => ("\u{21c4}", "EnterWorktree"),
        _ => ("\u{25cb}", "Tool"),
    }
}

#[cfg(test)]
mod tests {
    use super::tool_name_label;

    #[test]
    fn task_and_agent_share_subagent_label_and_icon() {
        assert_eq!(tool_name_label("Task"), ("\u{25c7}", "Subagent"));
        assert_eq!(tool_name_label("Agent"), ("\u{25c7}", "Subagent"));
    }
}
