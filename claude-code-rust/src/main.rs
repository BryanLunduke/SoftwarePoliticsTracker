// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use clap::Parser;
use claude_code_rust::Cli;
use claude_code_rust::error::AppError;
use std::fs::OpenOptions;
use std::time::Instant;

#[allow(clippy::exit)]
fn main() {
    if let Err(err) = run() {
        if let Some(app_error) = extract_app_error(&err) {
            eprintln!("{}", app_error.user_message());
            std::process::exit(app_error.exit_code());
        }
        eprintln!("{err}");
        std::process::exit(1);
    }
}

fn run() -> anyhow::Result<()> {
    let cli = Cli::parse();
    init_tracing(&cli)?;

    #[cfg(not(feature = "perf"))]
    if cli.perf_log.is_some() {
        return Err(anyhow::anyhow!("`--perf-log` requires a binary built with `--features perf`"));
    }

    let resolve_started = Instant::now();
    let bridge_launcher =
        claude_code_rust::agent::bridge::resolve_bridge_launcher(cli.bridge_script.as_deref())?;
    tracing::info!(
        "Resolved agent bridge launcher in {:?}: {}",
        resolve_started.elapsed(),
        bridge_launcher.describe()
    );

    let rt = tokio::runtime::Runtime::new()?;
    let local_set = tokio::task::LocalSet::new();

    rt.block_on(local_set.run_until(async move {
        // Phase 1: create app in Connecting state (instant, no I/O)
        let mut app = claude_code_rust::app::create_app(&cli);

        // Phase 2: start non-session startup work + TUI.
        // The bridge itself is started from the TUI loop only after trust is accepted.
        claude_code_rust::app::start_update_check(&app, &cli);
        claude_code_rust::app::start_service_status_check(&app);
        let result = claude_code_rust::app::run_tui(&mut app).await;
        maybe_print_resume_hint(&app, result.is_ok());

        // Kill any spawned terminal child processes before exiting
        claude_code_rust::agent::events::kill_all_terminals(&app.terminals);

        if let Some(app_error) = app.exit_error.take() {
            return Err(anyhow::Error::new(app_error));
        }

        result
    }))
}

fn extract_app_error(err: &anyhow::Error) -> Option<AppError> {
    err.chain().find_map(|cause| cause.downcast_ref::<AppError>().cloned())
}

fn init_tracing(cli: &Cli) -> anyhow::Result<()> {
    let Some(path) = cli.log_file.as_ref() else {
        if std::env::var_os("RUST_LOG").is_some() {
            eprintln!(
                "RUST_LOG is set, but tracing is disabled without --log-file <PATH>. \
Use --log-file to enable diagnostics."
            );
        }
        return Ok(());
    };

    let mut directives = cli
        .log_filter
        .clone()
        .or_else(|| std::env::var("RUST_LOG").ok())
        .unwrap_or_else(|| "info".to_owned());
    if !directives.contains("tui_markdown=") {
        directives.push_str(",tui_markdown=info");
    }
    let filter = tracing_subscriber::EnvFilter::try_new(directives.as_str())
        .map_err(|e| anyhow::anyhow!("invalid tracing filter `{directives}`: {e}"))?;

    let mut options = OpenOptions::new();
    options.create(true).write(true);
    if cli.log_append {
        options.append(true);
    } else {
        options.truncate(true);
    }
    let file = options
        .open(path)
        .map_err(|e| anyhow::anyhow!("failed to open log file {}: {e}", path.display()))?;

    tracing_subscriber::fmt()
        .with_env_filter(filter)
        .with_writer(file)
        .with_ansi(false)
        .with_file(true)
        .with_line_number(true)
        .with_target(true)
        .try_init()
        .map_err(|e| anyhow::anyhow!("failed to initialize tracing subscriber: {e}"))?;

    tracing::info!(
        target: "diagnostics",
        version = env!("CARGO_PKG_VERSION"),
        log_file = %path.display(),
        log_filter = %directives,
        log_append = cli.log_append,
        "tracing enabled"
    );

    Ok(())
}

fn maybe_print_resume_hint(app: &claude_code_rust::app::App, success: bool) {
    if !success {
        return;
    }
    let Some(session_id) = app.session_id.as_ref() else {
        return;
    };
    eprintln!("Resume this session: claude-rs --resume {session_id}");
}
