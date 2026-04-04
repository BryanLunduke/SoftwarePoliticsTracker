// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

pub mod agent;
pub mod app;
pub mod error;
pub mod perf;
pub mod ui;

use clap::Parser;

#[derive(Parser, Debug)]
#[command(name = "claude-rs", about = "Native Rust terminal for Claude Code")]
#[allow(clippy::struct_excessive_bools)]
pub struct Cli {
    /// Resume a previous session by ID
    #[arg(long)]
    pub resume: Option<String>,

    /// Disable startup update checks.
    #[arg(long)]
    pub no_update_check: bool,

    /// Working directory (defaults to cwd)
    #[arg(long, short = 'C')]
    pub dir: Option<std::path::PathBuf>,

    /// Path to the agent bridge script (defaults to agent-sdk/dist/bridge.js).
    #[arg(long)]
    pub bridge_script: Option<std::path::PathBuf>,

    /// Write tracing diagnostics to a file (disabled unless explicitly set).
    #[arg(long, value_name = "PATH")]
    pub log_file: Option<std::path::PathBuf>,

    /// Tracing filter directives (example: `info,claude_code_rust::ui=trace`).
    /// Falls back to `RUST_LOG` when omitted.
    #[arg(long, value_name = "FILTER")]
    pub log_filter: Option<String>,

    /// Append to `--log-file` instead of truncating on startup.
    #[arg(long)]
    pub log_append: bool,

    /// Write frame performance events to a file (requires `--features perf` build).
    #[arg(long, value_name = "PATH")]
    pub perf_log: Option<std::path::PathBuf>,

    /// Append to `--perf-log` instead of truncating on startup.
    #[arg(long)]
    pub perf_append: bool,
}
