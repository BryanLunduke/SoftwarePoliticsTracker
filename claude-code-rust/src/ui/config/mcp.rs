use super::input::render_text_input_field;
use super::overlay::{
    OverlayChrome, OverlayLayoutSpec, overlay_line_style, render_overlay_separator,
    render_overlay_shell,
};
use super::theme;
use crate::agent::types::{
    ElicitationAction, ElicitationMode, McpServerConnectionStatus, McpServerStatus,
    McpServerStatusConfig,
};
use crate::app::App;
use crate::app::config::{available_mcp_actions, is_mcp_action_available};
use ratatui::Frame;
use ratatui::layout::{Constraint, Direction, Layout, Margin, Rect};
use ratatui::style::{Color, Modifier, Style};
use ratatui::text::{Line, Span, Text};
use ratatui::widgets::{List, ListItem, ListState, Paragraph, Wrap};

pub(super) fn render(frame: &mut Frame, area: Rect, app: &App) {
    let content_area = area.inner(Margin { vertical: 1, horizontal: 2 });
    if content_area.width == 0 || content_area.height == 0 {
        return;
    }

    let summary = summary_lines(app);
    let summary_height =
        wrapped_height(Text::from(summary.clone()), content_area.width).min(content_area.height);
    let sections = Layout::default()
        .direction(Direction::Vertical)
        .constraints([Constraint::Length(summary_height), Constraint::Min(1)])
        .split(content_area);
    frame.render_widget(Paragraph::new(summary).wrap(Wrap { trim: false }), sections[0]);

    if app.session_id.is_none() {
        render_message(
            frame,
            sections[1],
            "No active session",
            "Open or resume a session to inspect MCP servers from the live SDK session.",
        );
        return;
    }

    if app.mcp.in_flight && app.mcp.servers.is_empty() {
        render_message(
            frame,
            sections[1],
            "Loading MCP status",
            "Waiting for the current session to return MCP server state.",
        );
        return;
    }

    if app.mcp.servers.is_empty() {
        let body = app.mcp.last_error.as_deref().unwrap_or(
            "The current session did not report any MCP servers. This view only shows live session-backed MCP state.",
        );
        render_message(frame, sections[1], "No MCP servers", body);
        return;
    }

    render_server_list(frame, sections[1], app);
}

pub(super) fn render_details_overlay(frame: &mut Frame, area: Rect, app: &App) {
    let Some(overlay) = app.config.mcp_details_overlay() else {
        return;
    };

    let server = app.mcp.servers.iter().find(|server| server.name == overlay.server_name);
    let action_lines = server.map_or_else(Vec::new, |server| mcp_action_lines(server, overlay));
    let rendered = render_overlay_shell(
        frame,
        area,
        OverlayLayoutSpec {
            min_width: 72,
            min_height: 12,
            width_percent: 78,
            height_percent: 82,
            preferred_height: 24,
            fullscreen_below: Some((80, 18)),
            inner_margin: Margin { vertical: 1, horizontal: 2 },
        },
        OverlayChrome {
            title: overlay.server_name.as_str(),
            subtitle: None,
            help: Some("Up/Down select | Enter run | Esc cancel"),
        },
    );

    if action_lines.is_empty() {
        let body = server.map_or_else(server_missing_lines, server_detail_lines);
        frame.render_widget(Paragraph::new(body).wrap(Wrap { trim: false }), rendered.body_area);
        return;
    }

    let action_height = wrapped_height(Text::from(action_lines.clone()), rendered.body_area.width);
    let sections = Layout::default()
        .direction(Direction::Vertical)
        .constraints([Constraint::Min(1), Constraint::Length(1), Constraint::Length(action_height)])
        .split(rendered.body_area);

    let body = server.map_or_else(server_missing_lines, server_detail_lines);
    frame.render_widget(Paragraph::new(body).wrap(Wrap { trim: false }), sections[0]);
    render_overlay_separator(frame, sections[1]);
    frame.render_widget(Paragraph::new(action_lines).wrap(Wrap { trim: false }), sections[2]);
}

pub(super) fn render_callback_url_overlay(frame: &mut Frame, area: Rect, app: &App) {
    let Some(overlay) = app.config.mcp_callback_url_overlay() else {
        return;
    };
    let rendered = render_overlay_shell(
        frame,
        area,
        OverlayLayoutSpec {
            min_width: 68,
            min_height: 12,
            width_percent: 72,
            height_percent: 42,
            preferred_height: 14,
            fullscreen_below: Some((80, 18)),
            inner_margin: Margin { vertical: 1, horizontal: 2 },
        },
        OverlayChrome {
            title: "Submit callback URL",
            subtitle: Some(overlay.server_name.as_str()),
            help: Some("Enter submit | Esc back"),
        },
    );
    let header_lines = vec![
        section_heading("Callback"),
        Line::from(Span::styled(
            "Paste the OAuth callback URL returned by the provider.",
            Style::default().fg(theme::DIM),
        )),
        Line::default(),
        detail_kv("Server", &overlay.server_name, Color::White),
    ];
    let footer_lines = vec![Line::from(Span::styled(
        "The URL is sent to the SDK exactly as pasted.",
        Style::default().fg(theme::DIM),
    ))];
    let sections = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(wrapped_height(
                Text::from(header_lines.clone()),
                rendered.body_area.width,
            )),
            Constraint::Length(1),
            Constraint::Length(wrapped_height(
                Text::from(footer_lines.clone()),
                rendered.body_area.width,
            )),
            Constraint::Min(0),
        ])
        .split(rendered.body_area);
    frame.render_widget(Paragraph::new(header_lines).wrap(Wrap { trim: false }), sections[0]);
    render_text_input_field(
        frame,
        sections[1],
        &overlay.draft,
        overlay.cursor,
        "https://callback.example/...",
    );
    frame.render_widget(Paragraph::new(footer_lines).wrap(Wrap { trim: false }), sections[2]);
}

pub(super) fn render_elicitation_overlay(frame: &mut Frame, area: Rect, app: &App) {
    let Some(overlay) = app.config.mcp_elicitation_overlay() else {
        return;
    };
    let action_lines = elicitation_action_lines(overlay);
    let rendered = render_overlay_shell(
        frame,
        area,
        OverlayLayoutSpec {
            min_width: 72,
            min_height: 16,
            width_percent: 80,
            height_percent: 78,
            preferred_height: 22,
            fullscreen_below: Some((90, 20)),
            inner_margin: Margin { vertical: 1, horizontal: 2 },
        },
        OverlayChrome {
            title: "MCP authentication",
            subtitle: Some(overlay.request.server_name.as_str()),
            help: Some("Up/Down select | Enter respond | Esc cancel"),
        },
    );
    let action_height = wrapped_height(Text::from(action_lines.clone()), rendered.body_area.width);
    let sections = Layout::default()
        .direction(Direction::Vertical)
        .constraints([Constraint::Min(1), Constraint::Length(1), Constraint::Length(action_height)])
        .split(rendered.body_area);
    frame.render_widget(
        Paragraph::new(elicitation_body_lines(overlay)).wrap(Wrap { trim: false }),
        sections[0],
    );
    render_overlay_separator(frame, sections[1]);
    frame.render_widget(Paragraph::new(action_lines).wrap(Wrap { trim: false }), sections[2]);
}

pub(super) fn render_auth_redirect_overlay(frame: &mut Frame, area: Rect, app: &App) {
    let Some(overlay) = app.config.mcp_auth_redirect_overlay() else {
        return;
    };
    let action_lines = auth_redirect_action_lines(overlay);
    let rendered = render_overlay_shell(
        frame,
        area,
        OverlayLayoutSpec {
            min_width: 72,
            min_height: 16,
            width_percent: 80,
            height_percent: 78,
            preferred_height: 22,
            fullscreen_below: Some((90, 20)),
            inner_margin: Margin { vertical: 1, horizontal: 2 },
        },
        OverlayChrome {
            title: "MCP authentication",
            subtitle: Some(overlay.redirect.server_name.as_str()),
            help: Some("Up/Down select | Enter run | Esc cancel"),
        },
    );
    let action_height = wrapped_height(Text::from(action_lines.clone()), rendered.body_area.width);
    let sections = Layout::default()
        .direction(Direction::Vertical)
        .constraints([Constraint::Min(1), Constraint::Length(1), Constraint::Length(action_height)])
        .split(rendered.body_area);
    frame.render_widget(
        Paragraph::new(auth_redirect_body_lines(overlay)).wrap(Wrap { trim: false }),
        sections[0],
    );
    render_overlay_separator(frame, sections[1]);
    frame.render_widget(Paragraph::new(action_lines).wrap(Wrap { trim: false }), sections[2]);
}

fn render_server_list(frame: &mut Frame, area: Rect, app: &App) {
    let items = app.mcp.servers.iter().enumerate().map(|(index, server)| {
        let selected = index == app.config.mcp_selected_server_index;
        ListItem::new(server_list_lines(server, selected)).style(server_row_style(selected))
    });

    let mut state = ListState::default().with_selected(Some(app.config.mcp_selected_server_index));
    frame.render_stateful_widget(List::new(items).highlight_symbol(""), area, &mut state);
}

fn summary_lines(app: &App) -> Vec<Line<'static>> {
    let counts = status_counts(app);
    let mut stats_spans = vec![
        badge_span(&format!("total {}", app.mcp.servers.len()), Color::Black, Color::White),
        Span::styled(" ", Style::default()),
        badge_span(&format!("connected {}", counts.connected), Color::Black, theme::RUST_ORANGE),
        Span::styled(" ", Style::default()),
        badge_span(
            &format!("needs auth {}", counts.needs_auth),
            Color::Black,
            theme::STATUS_WARNING,
        ),
        Span::styled(" ", Style::default()),
        badge_span(&format!("pending {}", counts.pending), Color::Black, Color::Cyan),
        Span::styled(" ", Style::default()),
        badge_span(&format!("disabled {}", counts.disabled), Color::White, Color::DarkGray),
        Span::styled(" ", Style::default()),
        badge_span(&format!("failed {}", counts.failed), Color::White, theme::STATUS_ERROR),
    ];
    if app.mcp.in_flight {
        stats_spans.push(Span::styled(" ", Style::default()));
        stats_spans.push(badge_span("refreshing", Color::Black, Color::Cyan));
    }

    let mut lines = vec![Line::default(), Line::from(stats_spans), Line::default()];

    if let Some(error) = app.mcp.last_error.as_deref() {
        lines.push(Line::from(Span::styled(
            format!("Last MCP error: {error}"),
            Style::default().fg(theme::STATUS_ERROR),
        )));
        lines.push(Line::default());
    }

    lines
}

fn server_list_lines(server: &McpServerStatus, selected: bool) -> Vec<Line<'static>> {
    let marker = if selected { ">" } else { " " };
    vec![
        Line::from(vec![
            Span::styled(format!("{marker} {}", server.name), list_title_style(selected)),
            Span::styled("  ", Style::default()),
            badge_span(
                status_label(server.status),
                status_badge_fg(server.status),
                status_color(server.status),
            ),
            Span::styled(" ", Style::default()),
            badge_span(server.scope.as_deref().unwrap_or("session"), Color::White, Color::DarkGray),
            Span::styled(" ", Style::default()),
            badge_span(transport_label(server.config.as_ref()), Color::Black, Color::White),
        ]),
        Line::from(Span::styled(
            format!("  {}", server_summary_line(server)),
            server_secondary_style(server),
        )),
        Line::default(),
    ]
}

fn server_detail_lines(server: &McpServerStatus) -> Vec<Line<'static>> {
    let mut lines = vec![
        section_heading("Status"),
        detail_kv("Status", status_label(server.status), status_color(server.status)),
        detail_kv(
            "Enabled",
            if matches!(server.status, McpServerConnectionStatus::Disabled) { "No" } else { "Yes" },
            Color::White,
        ),
        detail_kv("Scope", server.scope.as_deref().unwrap_or("session"), Color::White),
        detail_kv("Transport", transport_label(server.config.as_ref()), Color::White),
        detail_kv("Tools", &tool_summary(server.tools.len()), Color::White),
    ];

    if let Some(info) = server.server_info.as_ref() {
        lines.push(detail_kv("Server name", &info.name, Color::White));
        lines.push(detail_kv("Version", &info.version, Color::White));
    }

    if let Some(config) = server.config.as_ref() {
        lines.push(Line::default());
        lines.push(section_heading("Configuration"));
        lines.extend(config_lines(config));
    }

    if let Some(error) = server.error.as_deref() {
        lines.push(Line::default());
        lines.push(section_heading("Error"));
        lines.push(detail_value(error, theme::STATUS_ERROR));
    }

    lines
}

fn server_missing_lines() -> Vec<Line<'static>> {
    vec![
        Line::from(Span::styled(
            "The selected server is no longer present in the latest MCP snapshot.",
            Style::default().fg(theme::DIM),
        )),
        Line::default(),
        Line::from(Span::styled(
            "Close this overlay and refresh the MCP list.",
            Style::default().fg(theme::DIM),
        )),
    ]
}

fn mcp_action_lines(
    server: &McpServerStatus,
    overlay: &crate::app::config::McpDetailsOverlayState,
) -> Vec<Line<'static>> {
    let actions = available_mcp_actions(server);
    if actions.is_empty() {
        return vec![detail_value("No actions available.", theme::DIM)];
    }

    let mut lines = vec![section_heading("Actions"), Line::default()];
    for (index, action) in actions.into_iter().enumerate() {
        let selected = index == overlay.selected_index;
        let mut spans = vec![Span::styled(
            format!("{} {}", if selected { ">" } else { " " }, action.label()),
            overlay_line_style(selected, true),
        )];
        if !is_mcp_action_available(server, action) {
            spans.push(Span::styled("  ", Style::default()));
            spans.push(badge_span("not available", Color::Black, theme::STATUS_WARNING));
        }
        lines.push(Line::from(spans));
    }
    lines
}

fn elicitation_body_lines(
    overlay: &crate::app::config::McpElicitationOverlayState,
) -> Vec<Line<'static>> {
    let request = &overlay.request;
    let mut lines = vec![
        section_heading("Request"),
        detail_kv("Mode", elicitation_mode_label(request.mode), Color::White),
        Line::from(Span::styled(request.message.clone(), Style::default().fg(Color::White))),
    ];
    if let Some(url) = request.url.as_deref() {
        lines.push(Line::default());
        lines.push(section_heading("URL"));
        lines.push(detail_value(url, Color::White));
    }
    if overlay.browser_opened {
        lines.push(Line::default());
        lines.push(detail_value(
            "Opened your browser automatically. Finish auth there, then accept below.",
            theme::DIM,
        ));
    }
    if let Some(error) = overlay.browser_open_error.as_deref() {
        lines.push(Line::default());
        lines.push(detail_value(error, theme::STATUS_ERROR));
    }
    if matches!(request.mode, ElicitationMode::Form) {
        lines.push(Line::default());
        lines.push(section_heading("Form"));
        lines.push(detail_value(
            "Structured MCP forms are not editable yet in claude-rs.",
            theme::DIM,
        ));
        if let Some(schema) = request.requested_schema.as_ref() {
            let schema_text =
                serde_json::to_string_pretty(schema).unwrap_or_else(|_| schema.to_string());
            for line in schema_text.lines() {
                lines.push(detail_value(line, theme::DIM));
            }
        }
    }
    lines
}

fn elicitation_action_lines(
    overlay: &crate::app::config::McpElicitationOverlayState,
) -> Vec<Line<'static>> {
    let actions = elicitation_actions(&overlay.request);
    let mut lines = vec![section_heading("Actions"), Line::default()];
    let last_index = actions.len().saturating_sub(1);
    for (index, action) in actions.iter().enumerate() {
        let selected = index == overlay.selected_index;
        lines.push(Line::from(Span::styled(
            format!("{} {}", if selected { ">" } else { " " }, elicitation_action_label(*action)),
            overlay_line_style(selected, true),
        )));
        if index < last_index {
            lines.push(Line::default());
        }
    }
    if !lines.is_empty() {
        lines.push(Line::default());
    }
    lines
}

fn auth_redirect_body_lines(
    overlay: &crate::app::config::McpAuthRedirectOverlayState,
) -> Vec<Line<'static>> {
    let redirect = &overlay.redirect;
    let mut lines = vec![
        section_heading("Request"),
        detail_value(
            "Claude Code returned a browser auth redirect for this MCP server.",
            Color::White,
        ),
        Line::default(),
        section_heading("URL"),
        detail_value(&redirect.auth_url, Color::White),
    ];
    if overlay.browser_opened {
        lines.push(Line::default());
        lines.push(detail_value(
            "Opened your browser automatically. Finish auth there, then refresh.",
            theme::DIM,
        ));
    }
    if let Some(error) = overlay.browser_open_error.as_deref() {
        lines.push(Line::default());
        lines.push(detail_value(error, theme::STATUS_ERROR));
    }
    lines
}

fn auth_redirect_action_lines(
    overlay: &crate::app::config::McpAuthRedirectOverlayState,
) -> Vec<Line<'static>> {
    const ACTIONS: [&str; 3] = ["Refresh", "Copy URL", "Close"];
    let mut lines = vec![section_heading("Actions"), Line::default()];
    for (index, label) in ACTIONS.iter().enumerate() {
        let selected = index == overlay.selected_index;
        lines.push(Line::from(Span::styled(
            format!("{} {}", if selected { ">" } else { " " }, label),
            overlay_line_style(selected, true),
        )));
        if index + 1 < ACTIONS.len() {
            lines.push(Line::default());
        }
    }
    lines.push(Line::default());
    lines
}

fn config_lines(config: &McpServerStatusConfig) -> Vec<Line<'static>> {
    match config {
        McpServerStatusConfig::Stdio { command, args, env } => {
            let args_label = if args.is_empty() { "(none)".to_owned() } else { args.join(" ") };
            vec![
                detail_kv("Command", command, Color::White),
                detail_kv("Args", &args_label, Color::White),
                detail_kv("Env", &format!("{} variable(s)", env.len()), Color::White),
            ]
        }
        McpServerStatusConfig::Sse { url, headers }
        | McpServerStatusConfig::Http { url, headers } => vec![
            detail_kv("URL", url, Color::White),
            detail_kv("Headers", &format!("{} configured", headers.len()), Color::White),
        ],
        McpServerStatusConfig::Sdk { name } => vec![detail_kv("SDK server", name, Color::White)],
        McpServerStatusConfig::ClaudeaiProxy { url, id } => {
            vec![detail_kv("Proxy URL", url, Color::White), detail_kv("Proxy ID", id, Color::White)]
        }
    }
}

fn render_message(frame: &mut Frame, area: Rect, title: &str, body: &str) {
    frame.render_widget(
        Paragraph::new(vec![
            Line::from(Span::styled(
                title,
                Style::default().fg(Color::White).add_modifier(Modifier::BOLD),
            )),
            Line::default(),
            Line::from(Span::styled(body, Style::default().fg(theme::DIM))),
        ])
        .wrap(Wrap { trim: false }),
        area,
    );
}

fn detail_kv(key: &str, value: &str, value_color: Color) -> Line<'static> {
    Line::from(vec![
        Span::styled(format!("{key}: "), Style::default().fg(theme::DIM)),
        Span::styled(value.to_owned(), Style::default().fg(value_color)),
    ])
}

fn detail_value(value: &str, color: Color) -> Line<'static> {
    Line::from(Span::styled(value.to_owned(), Style::default().fg(color)))
}

fn section_heading(title: &str) -> Line<'static> {
    Line::from(Span::styled(
        title.to_owned(),
        Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD),
    ))
}

fn badge_span(label: &str, fg: Color, bg: Color) -> Span<'static> {
    Span::styled(format!(" {label} "), Style::default().fg(fg).bg(bg).add_modifier(Modifier::BOLD))
}

fn wrapped_height(text: Text<'static>, width: u16) -> u16 {
    u16::try_from(Paragraph::new(text).wrap(Wrap { trim: false }).line_count(width))
        .unwrap_or(u16::MAX)
        .max(1)
}

fn list_title_style(selected: bool) -> Style {
    let base = Style::default().fg(Color::White);
    if selected {
        base.fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD)
    } else {
        base.add_modifier(Modifier::BOLD)
    }
}

fn server_row_style(selected: bool) -> Style {
    if selected { Style::default().bg(theme::USER_MSG_BG) } else { Style::default() }
}

fn server_secondary_style(server: &McpServerStatus) -> Style {
    if server.error.as_deref().is_some_and(|error| !error.trim().is_empty()) {
        Style::default().fg(theme::STATUS_ERROR)
    } else {
        Style::default().fg(theme::DIM)
    }
}

fn server_summary_line(server: &McpServerStatus) -> String {
    if let Some(error) = server.error.as_deref()
        && !error.trim().is_empty()
    {
        return error.to_owned();
    }

    let mut parts = Vec::new();
    if let Some(info) = server.server_info.as_ref() {
        parts.push(format!("{} {}", info.name, info.version));
    }
    parts.push(tool_summary(server.tools.len()));
    match server.config.as_ref() {
        Some(McpServerStatusConfig::Stdio { command, .. }) => parts.push(format!("cmd {command}")),
        Some(
            McpServerStatusConfig::Sse { url, .. }
            | McpServerStatusConfig::Http { url, .. }
            | McpServerStatusConfig::ClaudeaiProxy { url, .. },
        ) => parts.push(url.clone()),
        Some(McpServerStatusConfig::Sdk { name }) => parts.push(format!("sdk {name}")),
        None => {}
    }
    parts.join("  |  ")
}

fn tool_summary(tool_count: usize) -> String {
    match tool_count {
        0 => "no tools".to_owned(),
        1 => "1 tool".to_owned(),
        count => format!("{count} tools"),
    }
}

fn status_color(status: McpServerConnectionStatus) -> Color {
    match status {
        McpServerConnectionStatus::Connected => theme::RUST_ORANGE,
        McpServerConnectionStatus::NeedsAuth => theme::STATUS_WARNING,
        McpServerConnectionStatus::Pending => Color::Cyan,
        McpServerConnectionStatus::Disabled => Color::DarkGray,
        McpServerConnectionStatus::Failed => theme::STATUS_ERROR,
    }
}

fn status_badge_fg(status: McpServerConnectionStatus) -> Color {
    match status {
        McpServerConnectionStatus::Connected
        | McpServerConnectionStatus::NeedsAuth
        | McpServerConnectionStatus::Pending => Color::Black,
        McpServerConnectionStatus::Disabled | McpServerConnectionStatus::Failed => Color::White,
    }
}

fn status_label(status: McpServerConnectionStatus) -> &'static str {
    match status {
        McpServerConnectionStatus::Connected => "connected",
        McpServerConnectionStatus::Failed => "failed",
        McpServerConnectionStatus::NeedsAuth => "needs auth",
        McpServerConnectionStatus::Pending => "pending",
        McpServerConnectionStatus::Disabled => "disabled",
    }
}

fn transport_label(config: Option<&McpServerStatusConfig>) -> &'static str {
    match config {
        Some(McpServerStatusConfig::Stdio { .. }) => "stdio",
        Some(McpServerStatusConfig::Sse { .. }) => "sse",
        Some(McpServerStatusConfig::Http { .. }) => "http",
        Some(McpServerStatusConfig::Sdk { .. }) => "sdk",
        Some(McpServerStatusConfig::ClaudeaiProxy { .. }) => "claudeai-proxy",
        None => "unknown",
    }
}

fn status_counts(app: &App) -> StatusCounts {
    app.mcp.servers.iter().fold(StatusCounts::default(), |mut counts, server| {
        match server.status {
            McpServerConnectionStatus::Connected => counts.connected += 1,
            McpServerConnectionStatus::NeedsAuth => counts.needs_auth += 1,
            McpServerConnectionStatus::Pending => counts.pending += 1,
            McpServerConnectionStatus::Disabled => counts.disabled += 1,
            McpServerConnectionStatus::Failed => counts.failed += 1,
        }
        counts
    })
}

fn elicitation_actions(
    request: &crate::agent::types::ElicitationRequest,
) -> Vec<ElicitationAction> {
    match request.mode {
        ElicitationMode::Url => {
            vec![ElicitationAction::Accept, ElicitationAction::Decline, ElicitationAction::Cancel]
        }
        ElicitationMode::Form => vec![ElicitationAction::Decline, ElicitationAction::Cancel],
    }
}

fn elicitation_mode_label(mode: ElicitationMode) -> &'static str {
    match mode {
        ElicitationMode::Form => "form",
        ElicitationMode::Url => "url",
    }
}

fn elicitation_action_label(action: ElicitationAction) -> &'static str {
    match action {
        ElicitationAction::Accept => "Accept",
        ElicitationAction::Decline => "Decline",
        ElicitationAction::Cancel => "Cancel",
    }
}

#[derive(Default)]
struct StatusCounts {
    connected: usize,
    needs_auth: usize,
    pending: usize,
    disabled: usize,
    failed: usize,
}

#[cfg(test)]
mod tests {
    use super::*;
    use ratatui::Terminal;
    use ratatui::backend::TestBackend;
    use std::collections::BTreeMap;

    fn render_mcp(app: &App, width: u16, height: u16) -> String {
        let backend = TestBackend::new(width, height);
        let mut terminal = Terminal::new(backend).expect("terminal");
        terminal
            .draw(|frame| {
                super::render(frame, frame.area(), app);
            })
            .expect("draw");
        let buffer = terminal.backend().buffer().clone();
        buffer
            .content
            .chunks(usize::from(buffer.area.width))
            .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
            .collect::<Vec<_>>()
            .join("\n")
    }

    #[test]
    fn renders_no_session_state_without_session_id_header() {
        let app = App::test_default();
        let rendered = render_mcp(&app, 120, 28);
        assert!(rendered.contains("MCP servers"));
        assert!(rendered.contains("No active session"));
        assert!(!rendered.contains("Session:"));
    }

    #[test]
    fn renders_live_server_snapshot_as_list_only() {
        let mut app = App::test_default();
        app.session_id = Some(crate::agent::model::SessionId::new("session-1"));
        app.mcp.servers = vec![
            McpServerStatus {
                name: "notion".to_owned(),
                status: McpServerConnectionStatus::NeedsAuth,
                server_info: None,
                error: None,
                config: Some(McpServerStatusConfig::Http {
                    url: "https://mcp.notion.com/mcp".to_owned(),
                    headers: BTreeMap::new(),
                }),
                scope: Some("user".to_owned()),
                tools: vec![],
            },
            McpServerStatus {
                name: "filesystem".to_owned(),
                status: McpServerConnectionStatus::Connected,
                server_info: Some(crate::agent::types::McpServerInfo {
                    name: "Filesystem".to_owned(),
                    version: "1.2.3".to_owned(),
                }),
                error: None,
                config: Some(McpServerStatusConfig::Stdio {
                    command: "npx".to_owned(),
                    args: vec![
                        "-y".to_owned(),
                        "@modelcontextprotocol/server-filesystem".to_owned(),
                    ],
                    env: BTreeMap::new(),
                }),
                scope: Some("project".to_owned()),
                tools: vec![crate::agent::types::McpTool {
                    name: "read_file".to_owned(),
                    description: Some("Read a file".to_owned()),
                    annotations: Some(crate::agent::types::McpToolAnnotations {
                        read_only: Some(true),
                        destructive: Some(false),
                        open_world: Some(false),
                    }),
                }],
            },
        ];
        app.config.mcp_selected_server_index = 1;

        let rendered = render_mcp(&app, 120, 30);
        assert!(rendered.contains("total 2"));
        assert!(rendered.contains("connected 1"));
        assert!(rendered.contains("needs auth 1"));
        assert!(rendered.contains("filesystem"));
        assert!(rendered.contains("project"));
        assert!(rendered.contains("Filesystem 1.2.3"));
        assert!(rendered.contains("1 tool"));
        assert!(!rendered.contains("Details"));
        assert!(!rendered.contains("Servers"));
    }
}
