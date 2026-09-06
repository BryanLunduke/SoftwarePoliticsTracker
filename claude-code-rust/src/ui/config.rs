// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

mod input;
mod mcp;
mod overlay;
mod plugins;
mod settings;
mod status;
mod usage;

use crate::app::config::{
    OutputStyle, OverlayFocus, language_input_validation_message, model_overlay_options,
    supported_effort_levels_for_model,
};
use crate::app::{App, ConfigTab};
use ratatui::Frame;
use ratatui::layout::{Constraint, Direction, Layout, Margin, Rect};
use ratatui::style::Color;
use ratatui::style::{Modifier, Style};
use ratatui::text::{Line, Span, Text};
use ratatui::widgets::{Block, Borders, Paragraph, Wrap};

use super::theme;
use input::{add_marketplace_example_lines, render_text_input_field};
use overlay::{
    OverlayChrome, OverlayLayoutSpec, overlay_line_style, render_overlay_header,
    render_overlay_separator as shared_render_overlay_separator, render_overlay_shell,
};

const SETTINGS_LIMITATION_HINT: &str = "Currently, not all settings are supported by claude-rs. This project uses the official Anthropic Claude Agent SDK, which limits claude-rs implementing all Claude Code settings.";
const MIN_SETTINGS_PANEL_HEIGHT: u16 = 3;

pub fn render(frame: &mut Frame, app: &mut App) {
    let frame_area = frame.area();
    app.cached_frame_area = frame_area;

    let outer = Block::default()
        .borders(Borders::ALL)
        .title("Config")
        .border_style(Style::default().fg(theme::DIM));
    frame.render_widget(outer, frame_area);

    let inner = frame_area.inner(Margin { vertical: 1, horizontal: 1 });
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(1),
            Constraint::Min(3),
            Constraint::Length(1),
            Constraint::Length(1),
        ])
        .split(inner);

    render_tab_header(frame, chunks[0], app.config.active_tab);

    match app.config.active_tab {
        ConfigTab::Settings => settings::render(frame, chunks[1], app),
        ConfigTab::Plugins => plugins::render(frame, chunks[1], app),
        ConfigTab::Status => status::render(frame, chunks[1], app),
        ConfigTab::Usage => usage::render(frame, chunks[1], app),
        ConfigTab::Mcp => mcp::render(frame, chunks[1], app),
    }

    if app.config.model_and_effort_overlay().is_some() {
        render_model_and_effort_overlay(frame, frame_area, app);
    } else if app.config.output_style_overlay().is_some() {
        render_output_style_overlay(frame, frame_area, app);
    } else if app.config.language_overlay().is_some() {
        render_language_overlay(frame, frame_area, app);
    } else if app.config.session_rename_overlay().is_some() {
        render_session_rename_overlay(frame, frame_area, app);
    } else if app.config.installed_plugin_actions_overlay().is_some() {
        render_installed_plugin_actions_overlay(frame, frame_area, app);
    } else if app.config.plugin_install_overlay().is_some() {
        render_plugin_install_overlay(frame, frame_area, app);
    } else if app.config.marketplace_actions_overlay().is_some() {
        render_marketplace_actions_overlay(frame, frame_area, app);
    } else if app.config.add_marketplace_overlay().is_some() {
        render_add_marketplace_overlay(frame, frame_area, app);
    } else if app.config.mcp_details_overlay().is_some() {
        mcp::render_details_overlay(frame, frame_area, app);
    } else if app.config.mcp_callback_url_overlay().is_some() {
        mcp::render_callback_url_overlay(frame, frame_area, app);
    } else if app.config.mcp_auth_redirect_overlay().is_some() {
        mcp::render_auth_redirect_overlay(frame, frame_area, app);
    } else if app.config.mcp_elicitation_overlay().is_some() {
        mcp::render_elicitation_overlay(frame, frame_area, app);
    }

    let (message, is_error) = if let Some(error) = app.config.last_error.clone() {
        (error, true)
    } else if let Some(status) = app.config.status_message.clone() {
        (status, false)
    } else {
        (String::new(), false)
    };
    frame.render_widget(
        Paragraph::new(Line::from(Span::styled(
            message,
            Style::default().fg(if is_error { theme::STATUS_ERROR } else { theme::DIM }),
        ))),
        chunks[2],
    );

    let help = config_help_text(app);
    frame.render_widget(
        Paragraph::new(Line::from(Span::styled(help, Style::default().fg(theme::RUST_ORANGE)))),
        chunks[3],
    );
}

fn config_help_text(app: &App) -> String {
    if app.config.overlay.is_some() {
        return String::new();
    }

    match app.config.active_tab {
        ConfigTab::Settings => {
            "Left/Right edit | Space edit | Tab next tab | Shift+Tab prev tab | Enter close | Esc close"
                .to_owned()
        }
        ConfigTab::Plugins => {
            if crate::app::plugins::search_enabled(app.plugins.active_tab) {
                if app.plugins.search_focused {
                    "Left/Right switch list | Down list | Type search | Backspace erase | Del clear | Tab next tab | Shift+Tab prev tab | Enter close | Esc close".to_owned()
                } else if matches!(
                    app.plugins.active_tab,
                    crate::app::plugins::PluginsViewTab::Installed
                        | crate::app::plugins::PluginsViewTab::Plugins
                ) {
                    "Left/Right switch list | Up search | Up/Down move | Enter actions | Tab next tab | Shift+Tab prev tab | Esc close".to_owned()
                } else {
                    "Left/Right switch list | Up search | Up/Down move | Tab next tab | Shift+Tab prev tab | Enter close | Esc close".to_owned()
                }
            } else if matches!(
                app.plugins.active_tab,
                crate::app::plugins::PluginsViewTab::Marketplace
            ) {
                "Left/Right switch list | Up/Down move | Enter actions | Tab next tab | Shift+Tab prev tab | Esc close".to_owned()
            } else {
                "Left/Right switch list | Up/Down move | Tab next tab | Shift+Tab prev tab | Enter close | Esc close".to_owned()
            }
        }
        ConfigTab::Usage => {
            "r refresh | Tab next tab | Shift+Tab prev tab | Enter close | Esc close".to_owned()
        }
        ConfigTab::Mcp => {
            "Up/Down select | Enter actions | r refresh | Tab next tab | Shift+Tab prev tab | Esc close"
                .to_owned()
        }
        ConfigTab::Status => {
            if app.session_id.is_some() {
                "g generate | r rename | Tab next tab | Shift+Tab prev tab | Enter close | Esc close"
                    .to_owned()
            } else {
                "Tab next tab | Shift+Tab prev tab | Enter close | Esc close".to_owned()
            }
        }
    }
}

fn render_model_and_effort_overlay(frame: &mut Frame, area: Rect, app: &App) {
    let Some(overlay) = app.config.model_and_effort_overlay() else {
        return;
    };
    let rendered = render_overlay_shell(
        frame,
        area,
        OverlayLayoutSpec {
            min_width: 1,
            min_height: 1,
            width_percent: 90,
            height_percent: 84,
            preferred_height: u16::MAX,
            fullscreen_below: Some((90, 20)),
            inner_margin: Margin { vertical: 1, horizontal: 1 },
        },
        OverlayChrome {
            title: "Model and Thinking Effort",
            subtitle: None,
            help: Some("Tab switches model/effort | Enter confirm | Esc cancel"),
        },
    );
    let model_lines = model_overlay_lines(app);
    let effort_lines = effort_overlay_lines(app);
    let (model_height, effort_height) = model_and_effort_section_heights(rendered.body_area.height);
    let sections = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(1),
            Constraint::Length(model_height),
            Constraint::Length(1),
            Constraint::Length(1),
            Constraint::Length(effort_height),
        ])
        .split(rendered.body_area);

    let model_focused = overlay.focus == OverlayFocus::Model;
    let effort_focused = overlay.focus == OverlayFocus::Effort;
    render_overlay_header(frame, sections[0], "Model", model_focused);
    shared_render_overlay_separator(frame, sections[2]);
    render_overlay_header(frame, sections[3], "Thinking effort", effort_focused);

    let model_scroll = model_overlay_scroll(app, sections[1].height, sections[1].width);
    frame.render_widget(
        Paragraph::new(model_lines).scroll((model_scroll, 0)).wrap(Wrap { trim: false }),
        sections[1],
    );

    frame.render_widget(Paragraph::new(effort_lines).wrap(Wrap { trim: false }), sections[4]);
}

fn model_overlay_lines(app: &App) -> Vec<Line<'static>> {
    let Some(overlay) = app.config.model_and_effort_overlay() else {
        return Vec::new();
    };
    let mut lines = model_overlay_options(app)
        .into_iter()
        .flat_map(|option| {
            let selected = option.id == overlay.selected_model;
            let marker = if selected { ">" } else { " " };
            let mut lines = vec![model_overlay_title_line(
                &option,
                marker,
                selected,
                overlay.focus == OverlayFocus::Model,
            )];
            if let Some(description) = option.description {
                lines.push(Line::from(Span::styled(
                    format!("  {description}"),
                    Style::default().fg(theme::DIM),
                )));
            }
            lines.push(Line::default());
            lines
        })
        .collect::<Vec<_>>();
    if !lines.is_empty() {
        lines.pop();
    }
    lines
}

fn effort_overlay_lines(app: &App) -> Vec<Line<'static>> {
    let Some(overlay) = app.config.model_and_effort_overlay() else {
        return Vec::new();
    };
    let levels = supported_effort_levels_for_model(app, &overlay.selected_model);
    if levels.is_empty() {
        return vec![
            Line::from(Span::styled(
                "  Thinking effort is not available for the selected model.",
                Style::default().fg(theme::DIM),
            )),
            Line::default(),
            Line::from(Span::styled(
                format!("  Saved value: {}", overlay.selected_effort.label()),
                Style::default().fg(Color::White),
            )),
        ];
    }
    let mut lines = levels
        .into_iter()
        .flat_map(|level| {
            let selected = level == overlay.selected_effort;
            vec![
                Line::from(Span::styled(
                    format!("{} {}", if selected { ">" } else { " " }, level.label()),
                    overlay_line_style(selected, overlay.focus == OverlayFocus::Effort),
                )),
                Line::from(Span::styled(
                    format!("  {}", level.description()),
                    Style::default().fg(theme::DIM),
                )),
                Line::default(),
            ]
        })
        .collect::<Vec<_>>();
    if !lines.is_empty() {
        lines.pop();
    }
    lines
}

fn render_output_style_overlay(frame: &mut Frame, area: Rect, app: &App) {
    let rendered = render_overlay_shell(
        frame,
        area,
        OverlayLayoutSpec {
            min_width: 72,
            min_height: 8,
            width_percent: 84,
            height_percent: 80,
            preferred_height: 14,
            fullscreen_below: Some((72, 16)),
            inner_margin: Margin { vertical: 1, horizontal: 2 },
        },
        OverlayChrome {
            title: "Preferred output style",
            subtitle: Some("This changes how Claude Code communicates with you"),
            help: Some("Enter confirm | Esc cancel"),
        },
    );
    frame.render_widget(
        Paragraph::new(output_style_overlay_lines(app)).wrap(Wrap { trim: false }),
        rendered.body_area,
    );
}

fn render_language_overlay(frame: &mut Frame, area: Rect, app: &App) {
    let Some(overlay) = app.config.language_overlay() else {
        return;
    };
    let rendered = render_overlay_shell(
        frame,
        area,
        OverlayLayoutSpec {
            min_width: 56,
            min_height: 8,
            width_percent: 72,
            height_percent: 48,
            preferred_height: 10,
            fullscreen_below: Some((56, 14)),
            inner_margin: Margin { vertical: 1, horizontal: 2 },
        },
        OverlayChrome {
            title: "Language",
            subtitle: Some("Free-text prompt language for Claude sessions"),
            help: Some("Enter confirm | Esc cancel"),
        },
    );
    let sections = Layout::default()
        .direction(Direction::Vertical)
        .constraints([Constraint::Length(1), Constraint::Min(1)])
        .split(rendered.body_area);

    render_text_input_field(
        frame,
        sections[0],
        &overlay.draft,
        overlay.cursor,
        "e.g. en, Greek, Japanese, Pirate",
    );

    let validation = language_input_validation_message(&overlay.draft);
    let (message, style) = match validation {
        Some(message) => (message, Style::default().fg(theme::STATUS_ERROR)),
        None => (
            "Examples: en, Greek, Japanese, Klingon, Pirate. Stored as prompt guidance, not UI language.",
            Style::default().fg(theme::DIM),
        ),
    };
    frame.render_widget(Paragraph::new(Line::from(Span::styled(message, style))), sections[1]);
}

fn render_session_rename_overlay(frame: &mut Frame, area: Rect, app: &App) {
    let Some(overlay) = app.config.session_rename_overlay() else {
        return;
    };
    let rendered = render_overlay_shell(
        frame,
        area,
        OverlayLayoutSpec {
            min_width: 56,
            min_height: 8,
            width_percent: 72,
            height_percent: 48,
            preferred_height: 10,
            fullscreen_below: Some((56, 14)),
            inner_margin: Margin { vertical: 1, horizontal: 2 },
        },
        OverlayChrome {
            title: "Rename session",
            subtitle: Some("Set a custom title for the current session"),
            help: Some("Enter confirm | Esc cancel"),
        },
    );
    let sections = Layout::default()
        .direction(Direction::Vertical)
        .constraints([Constraint::Length(1), Constraint::Min(1)])
        .split(rendered.body_area);

    render_text_input_field(
        frame,
        sections[0],
        &overlay.draft,
        overlay.cursor,
        "Custom session name",
    );
    frame.render_widget(
        Paragraph::new(Line::from(Span::styled(
            "Leave the field empty to clear the custom session name.",
            Style::default().fg(theme::DIM),
        ))),
        sections[1],
    );
}

fn render_installed_plugin_actions_overlay(frame: &mut Frame, area: Rect, app: &App) {
    let Some(overlay) = app.config.installed_plugin_actions_overlay() else {
        return;
    };
    let rendered = render_overlay_shell(
        frame,
        area,
        OverlayLayoutSpec {
            min_width: 56,
            min_height: 10,
            width_percent: 70,
            height_percent: 62,
            preferred_height: 14,
            fullscreen_below: Some((56, 16)),
            inner_margin: Margin { vertical: 1, horizontal: 2 },
        },
        OverlayChrome {
            title: "Installed plugin",
            subtitle: None,
            help: Some("Up/Down select | Enter run | Esc cancel"),
        },
    );
    let sections = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(1),
            Constraint::Length(2),
            Constraint::Length(1),
            Constraint::Min(1),
            Constraint::Length(1),
        ])
        .split(rendered.body_area);

    frame.render_widget(
        Paragraph::new(Line::from(Span::styled(
            overlay.title.clone(),
            Style::default().fg(Color::White).add_modifier(Modifier::BOLD),
        ))),
        sections[0],
    );
    frame.render_widget(
        Paragraph::new(Line::from(Span::styled(
            overlay.description.clone(),
            Style::default().fg(theme::DIM),
        )))
        .wrap(Wrap { trim: false }),
        sections[1],
    );
    shared_render_overlay_separator(frame, sections[2]);
    frame.render_widget(
        Paragraph::new(installed_plugin_action_overlay_lines(app)).wrap(Wrap { trim: false }),
        sections[3],
    );
}

fn render_plugin_install_overlay(frame: &mut Frame, area: Rect, app: &App) {
    let Some(overlay) = app.config.plugin_install_overlay() else {
        return;
    };
    let rendered = render_overlay_shell(
        frame,
        area,
        OverlayLayoutSpec {
            min_width: 56,
            min_height: 10,
            width_percent: 70,
            height_percent: 62,
            preferred_height: 14,
            fullscreen_below: Some((56, 16)),
            inner_margin: Margin { vertical: 1, horizontal: 2 },
        },
        OverlayChrome {
            title: "Install plugin",
            subtitle: None,
            help: Some("Up/Down select | Enter run | Esc cancel"),
        },
    );
    let sections = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(1),
            Constraint::Length(2),
            Constraint::Length(1),
            Constraint::Min(1),
            Constraint::Length(1),
        ])
        .split(rendered.body_area);

    frame.render_widget(
        Paragraph::new(Line::from(Span::styled(
            overlay.title.clone(),
            Style::default().fg(Color::White).add_modifier(Modifier::BOLD),
        ))),
        sections[0],
    );
    frame.render_widget(
        Paragraph::new(Line::from(Span::styled(
            overlay.description.clone(),
            Style::default().fg(theme::DIM),
        )))
        .wrap(Wrap { trim: false }),
        sections[1],
    );
    shared_render_overlay_separator(frame, sections[2]);
    frame.render_widget(
        Paragraph::new(plugin_install_overlay_lines(app)).wrap(Wrap { trim: false }),
        sections[3],
    );
}

fn render_marketplace_actions_overlay(frame: &mut Frame, area: Rect, app: &App) {
    let Some(overlay) = app.config.marketplace_actions_overlay() else {
        return;
    };
    let rendered = render_overlay_shell(
        frame,
        area,
        OverlayLayoutSpec {
            min_width: 56,
            min_height: 10,
            width_percent: 70,
            height_percent: 62,
            preferred_height: 14,
            fullscreen_below: Some((56, 16)),
            inner_margin: Margin { vertical: 1, horizontal: 2 },
        },
        OverlayChrome {
            title: "Marketplace",
            subtitle: None,
            help: Some("Up/Down select | Enter run | Esc cancel"),
        },
    );
    let sections = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(1),
            Constraint::Length(2),
            Constraint::Length(1),
            Constraint::Min(1),
            Constraint::Length(1),
        ])
        .split(rendered.body_area);

    frame.render_widget(
        Paragraph::new(Line::from(Span::styled(
            overlay.title.clone(),
            Style::default().fg(Color::White).add_modifier(Modifier::BOLD),
        ))),
        sections[0],
    );
    frame.render_widget(
        Paragraph::new(Line::from(Span::styled(
            overlay.description.clone(),
            Style::default().fg(theme::DIM),
        )))
        .wrap(Wrap { trim: false }),
        sections[1],
    );
    shared_render_overlay_separator(frame, sections[2]);
    frame.render_widget(
        Paragraph::new(marketplace_action_overlay_lines(app)).wrap(Wrap { trim: false }),
        sections[3],
    );
}

fn render_add_marketplace_overlay(frame: &mut Frame, area: Rect, app: &App) {
    let Some(overlay) = app.config.add_marketplace_overlay() else {
        return;
    };
    let rendered = render_overlay_shell(
        frame,
        area,
        OverlayLayoutSpec {
            min_width: 60,
            min_height: 13,
            width_percent: 72,
            height_percent: 66,
            preferred_height: 15,
            fullscreen_below: Some((60, 18)),
            inner_margin: Margin { vertical: 1, horizontal: 2 },
        },
        OverlayChrome {
            title: "Add Marketplace",
            subtitle: None,
            help: Some("Enter add | Esc cancel"),
        },
    );
    let sections = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(1),
            Constraint::Length(5),
            Constraint::Length(1),
            Constraint::Length(1),
            Constraint::Min(1),
        ])
        .split(rendered.body_area);

    frame.render_widget(
        Paragraph::new(Line::from(Span::styled(
            "Enter marketplace source:",
            Style::default().fg(Color::White).add_modifier(Modifier::BOLD),
        ))),
        sections[0],
    );
    frame.render_widget(
        Paragraph::new(add_marketplace_example_lines()).wrap(Wrap { trim: false }),
        sections[1],
    );
    render_text_input_field(
        frame,
        sections[3],
        &overlay.draft,
        overlay.cursor,
        "owner/repo or URL",
    );
}

fn output_style_overlay_lines(app: &App) -> Vec<Line<'static>> {
    let Some(overlay) = app.config.output_style_overlay() else {
        return Vec::new();
    };

    let mut lines = Vec::new();
    for (index, style) in OutputStyle::ALL.iter().copied().enumerate() {
        let selected = style == overlay.selected;
        let marker = if selected { ">" } else { " " };
        lines.push(Line::from(vec![
            Span::styled(format!("{marker} {}. ", index + 1), overlay_line_style(selected, true)),
            Span::styled(style.label().to_owned(), overlay_line_style(selected, true)),
        ]));
        lines.push(Line::from(Span::styled(
            format!("   {}", style.description()),
            Style::default().fg(theme::DIM),
        )));
        if index + 1 < OutputStyle::ALL.len() {
            lines.push(Line::default());
        }
    }
    lines
}

fn installed_plugin_action_overlay_lines(app: &App) -> Vec<Line<'static>> {
    let Some(overlay) = app.config.installed_plugin_actions_overlay() else {
        return Vec::new();
    };

    let mut lines = Vec::new();
    for (index, action) in overlay.actions.iter().copied().enumerate() {
        let selected = index == overlay.selected_index;
        lines.push(Line::from(Span::styled(
            format!("{} {}", if selected { ">" } else { " " }, action.label()),
            overlay_line_style(selected, true),
        )));
        if index + 1 < overlay.actions.len() {
            lines.push(Line::default());
        }
    }
    lines
}

fn plugin_install_overlay_lines(app: &App) -> Vec<Line<'static>> {
    let Some(overlay) = app.config.plugin_install_overlay() else {
        return Vec::new();
    };

    let mut lines = Vec::new();
    for (index, action) in overlay.actions.iter().copied().enumerate() {
        let selected = index == overlay.selected_index;
        lines.push(Line::from(Span::styled(
            format!("{} {}", if selected { ">" } else { " " }, action.label()),
            overlay_line_style(selected, true),
        )));
        if index + 1 < overlay.actions.len() {
            lines.push(Line::default());
        }
    }
    lines
}

fn marketplace_action_overlay_lines(app: &App) -> Vec<Line<'static>> {
    let Some(overlay) = app.config.marketplace_actions_overlay() else {
        return Vec::new();
    };

    let mut lines = Vec::new();
    for (index, action) in overlay.actions.iter().copied().enumerate() {
        let selected = index == overlay.selected_index;
        lines.push(Line::from(Span::styled(
            format!("{} {}", if selected { ">" } else { " " }, action.label()),
            overlay_line_style(selected, true),
        )));
        if index + 1 < overlay.actions.len() {
            lines.push(Line::default());
        }
    }
    lines
}

fn model_and_effort_section_heights(inner_height: u16) -> (u16, u16) {
    const CHROME_HEIGHT: u16 = 3;
    const DEFAULT_EFFORT_HEIGHT: u16 = 8;

    let content_height = inner_height.saturating_sub(CHROME_HEIGHT);
    match content_height {
        0 => (0, 0),
        1 => (1, 0),
        _ => {
            let effort_height = DEFAULT_EFFORT_HEIGHT.min(content_height.saturating_sub(1));
            let model_height = content_height.saturating_sub(effort_height);
            (model_height, effort_height)
        }
    }
}

fn model_overlay_scroll(app: &App, viewport_height: u16, viewport_width: u16) -> u16 {
    let Some(overlay) = app.config.model_and_effort_overlay() else {
        return 0;
    };
    let options = model_overlay_options(app);
    if options.is_empty() || viewport_height == 0 || viewport_width == 0 {
        return 0;
    }

    let selected_index =
        options.iter().position(|option| option.id == overlay.selected_model).unwrap_or(0);
    let selected_start = options
        .iter()
        .take(selected_index)
        .enumerate()
        .map(|(index, option)| {
            model_overlay_option_height(option, index + 1 == options.len(), viewport_width)
        })
        .sum::<usize>();
    let selected_height = model_overlay_option_height(
        &options[selected_index],
        selected_index + 1 == options.len(),
        viewport_width,
    );
    let viewport_height = usize::from(viewport_height);

    if selected_start + selected_height <= viewport_height {
        0
    } else {
        u16::try_from(selected_start + selected_height - viewport_height).unwrap_or(u16::MAX)
    }
}

fn model_overlay_option_height(
    option: &crate::app::config::OverlayModelOption,
    is_last: bool,
    viewport_width: u16,
) -> usize {
    let title = model_overlay_title_line(option, " ", false, false);
    let mut height = wrapped_text_height(Text::from(vec![title]), viewport_width);
    if let Some(description) = option.description.as_deref() {
        height += wrapped_text_height(
            Text::from(vec![Line::from(Span::styled(
                format!("  {description}"),
                Style::default().fg(theme::DIM),
            ))]),
            viewport_width,
        );
    }
    height + usize::from(!is_last)
}

struct CapabilityBadge {
    label: &'static str,
    bg: Color,
    fg: Color,
}

#[cfg(test)]
fn model_overlay_title_text(
    option: &crate::app::config::OverlayModelOption,
    marker: &str,
) -> String {
    let badges = model_capability_badges(option);
    let mut title = format!("{marker} {}", option.display_name);
    if !badges.is_empty() {
        title.push_str("  ");
        title.push_str(&badges.into_iter().map(|badge| badge.label).collect::<Vec<_>>().join("  "));
    }
    title
}

fn model_overlay_title_line(
    option: &crate::app::config::OverlayModelOption,
    marker: &str,
    selected: bool,
    focused: bool,
) -> Line<'static> {
    Line::from(model_overlay_title_spans(option, marker, selected, focused))
}

fn model_overlay_title_spans(
    option: &crate::app::config::OverlayModelOption,
    marker: &str,
    selected: bool,
    focused: bool,
) -> Vec<Span<'static>> {
    let mut spans = vec![Span::styled(
        format!("{marker} {}", option.display_name),
        overlay_line_style(selected, focused),
    )];
    let badges = model_capability_badges(option);
    if badges.is_empty() {
        return spans;
    }
    spans.push(Span::styled("  ", Style::default().fg(theme::DIM)));
    for (index, badge) in badges.into_iter().enumerate() {
        if index > 0 {
            spans.push(Span::styled("  ", Style::default().fg(theme::DIM)));
        }
        spans.push(Span::styled(
            format!(" {} ", badge.label),
            Style::default().fg(badge.fg).bg(badge.bg).add_modifier(Modifier::BOLD),
        ));
    }
    spans
}

fn model_capability_badges(
    option: &crate::app::config::OverlayModelOption,
) -> Vec<CapabilityBadge> {
    let mut badges = Vec::new();
    if option.supports_effort {
        badges.push(CapabilityBadge {
            label: "Effort",
            bg: Color::Rgb(64, 64, 64),
            fg: Color::White,
        });
    }
    if option.supports_adaptive_thinking == Some(true) {
        badges.push(CapabilityBadge {
            label: "Adaptive thinking",
            bg: Color::Rgb(34, 92, 124),
            fg: Color::White,
        });
    }
    if option.supports_fast_mode == Some(true) {
        badges.push(CapabilityBadge {
            label: "Fast mode",
            bg: Color::Rgb(24, 120, 82),
            fg: Color::White,
        });
    }
    if option.supports_auto_mode == Some(true) {
        badges.push(CapabilityBadge {
            label: "Auto mode",
            bg: Color::Rgb(152, 106, 0),
            fg: Color::Black,
        });
    }
    badges
}

fn wrapped_text_height(text: Text<'static>, viewport_width: u16) -> usize {
    Paragraph::new(text).wrap(Wrap { trim: false }).line_count(viewport_width.max(1)).max(1)
}

fn render_tab_header(frame: &mut Frame, area: Rect, active_tab: ConfigTab) {
    let mut spans = Vec::new();
    for (index, tab) in ConfigTab::ALL.iter().copied().enumerate() {
        if index > 0 {
            spans.push(Span::styled(" | ", Style::default().fg(theme::DIM)));
        }

        let style = if tab == active_tab {
            Style::default().fg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD)
        } else {
            Style::default().fg(Color::White)
        };
        spans.push(Span::styled(tab.title().to_owned(), style));
    }

    let line = Line::from(spans);
    let content_width = u16::try_from(line.width()).unwrap_or(area.width).min(area.width);
    let header_area = centered_line_area(area, content_width);
    frame.render_widget(Paragraph::new(line), header_area);
}

fn centered_line_area(area: Rect, content_width: u16) -> Rect {
    if content_width >= area.width {
        return area;
    }

    let offset = area.width.saturating_sub(content_width) / 2;
    Rect { x: area.x + offset, y: area.y, width: content_width, height: area.height }
}

#[cfg(test)]
mod tests {
    use super::{
        SETTINGS_LIMITATION_HINT, model_overlay_lines, model_overlay_scroll,
        model_overlay_title_line, model_overlay_title_text,
    };
    use crate::agent::model::{AvailableModel, EffortLevel};
    use crate::app::App;
    use crate::app::config::{
        ConfigOverlayState, LanguageOverlayState, ModelAndEffortOverlayState, OutputStyle,
        OutputStyleOverlayState, OverlayFocus, SettingId, setting_specs,
    };
    use ratatui::Terminal;
    use ratatui::backend::TestBackend;
    use ratatui::buffer::Buffer;
    use ratatui::layout::Rect;
    use ratatui::style::Style;
    use ratatui::text::{Line, Span, Text};
    use ratatui::widgets::{Paragraph, Wrap};

    fn rendered_model_option_height(
        option: &crate::app::config::OverlayModelOption,
        is_last: bool,
        viewport_width: u16,
    ) -> usize {
        let mut lines = vec![model_overlay_title_line(option, " ", false, false)];
        if let Some(description) = option.description.as_deref() {
            lines.push(Line::from(Span::styled(
                format!("  {description}"),
                Style::default().fg(super::theme::DIM),
            )));
        }
        if !is_last {
            lines.push(Line::default());
        }
        Paragraph::new(Text::from(lines)).wrap(Wrap { trim: false }).line_count(viewport_width)
    }

    #[test]
    fn model_overlay_scroll_keeps_selected_multiline_model_visible() {
        let mut app = App::test_default();
        app.available_models = vec![
            AvailableModel::new("default", "Default")
                .description("Opus 4.6")
                .supports_effort(true)
                .supported_effort_levels(vec![
                    EffortLevel::Low,
                    EffortLevel::Medium,
                    EffortLevel::High,
                ]),
            AvailableModel::new("opus-1m", "Opus (1M context)")
                .description("Extra usage")
                .supports_effort(true)
                .supported_effort_levels(vec![
                    EffortLevel::Low,
                    EffortLevel::Medium,
                    EffortLevel::High,
                ]),
            AvailableModel::new("sonnet", "Sonnet")
                .description("Everyday tasks")
                .supports_effort(true)
                .supported_effort_levels(vec![
                    EffortLevel::Low,
                    EffortLevel::Medium,
                    EffortLevel::High,
                ]),
            AvailableModel::new("haiku", "Haiku").description("Fastest").supports_effort(false),
        ];
        app.config.overlay = Some(ConfigOverlayState::ModelAndEffort(ModelAndEffortOverlayState {
            focus: OverlayFocus::Model,
            selected_model: "sonnet".to_owned(),
            selected_effort: EffortLevel::High,
        }));

        assert_eq!(model_overlay_scroll(&app, 6, 40), 5);
    }

    #[test]
    fn model_overlay_scroll_accounts_for_wrapped_lines() {
        let mut app = App::test_default();
        app.available_models = vec![
            AvailableModel::new("default", "Default")
                .description("1234567890")
                .supports_effort(true)
                .supported_effort_levels(vec![
                    EffortLevel::Low,
                    EffortLevel::Medium,
                    EffortLevel::High,
                ]),
            AvailableModel::new("haiku", "Haiku").supports_effort(false),
        ];
        app.config.overlay = Some(ConfigOverlayState::ModelAndEffort(ModelAndEffortOverlayState {
            focus: OverlayFocus::Model,
            selected_model: "haiku".to_owned(),
            selected_effort: EffortLevel::Medium,
        }));

        assert_eq!(model_overlay_scroll(&app, 4, 10), 2);
    }

    #[test]
    fn model_overlay_scroll_accounts_for_badge_padding_width() {
        let mut app = App::test_default();
        app.available_models = vec![
            AvailableModel::new("sonnet", "Sonnet")
                .description("Everyday tasks")
                .supports_effort(true)
                .supported_effort_levels(vec![EffortLevel::Low, EffortLevel::Medium])
                .supports_adaptive_thinking(Some(true))
                .supports_fast_mode(Some(true))
                .supports_auto_mode(Some(true)),
            AvailableModel::new("haiku", "Haiku")
                .description("Fastest")
                .supports_effort(false)
                .supports_fast_mode(Some(true)),
        ];
        app.config.overlay = Some(ConfigOverlayState::ModelAndEffort(ModelAndEffortOverlayState {
            focus: OverlayFocus::Model,
            selected_model: "haiku".to_owned(),
            selected_effort: EffortLevel::Medium,
        }));

        let options = crate::app::config::model_overlay_options(&app);
        let selected_index =
            options.iter().position(|option| option.id == "haiku").expect("selected model index");
        let selected_start = options
            .iter()
            .take(selected_index)
            .enumerate()
            .map(|(index, option)| {
                rendered_model_option_height(option, index + 1 == options.len(), 18)
            })
            .sum::<usize>();
        let selected_height = rendered_model_option_height(
            &options[selected_index],
            selected_index + 1 == options.len(),
            18,
        );
        let expected_scroll = selected_start.saturating_add(selected_height).saturating_sub(4);

        assert_eq!(
            model_overlay_scroll(&app, 4, 18),
            u16::try_from(expected_scroll).unwrap_or(u16::MAX)
        );
        assert!(expected_scroll > 0);
    }

    #[test]
    fn model_overlay_lines_show_positive_capability_badges_only() {
        let mut app = App::test_default();
        app.available_models = vec![
            AvailableModel::new("sonnet", "Sonnet")
                .description("Everyday tasks")
                .supports_effort(true)
                .supported_effort_levels(vec![
                    EffortLevel::Low,
                    EffortLevel::Medium,
                    EffortLevel::High,
                ])
                .supports_adaptive_thinking(Some(true))
                .supports_fast_mode(Some(true))
                .supports_auto_mode(Some(true)),
            AvailableModel::new("haiku", "Haiku")
                .description("Fastest")
                .supports_effort(false)
                .supports_adaptive_thinking(Some(false))
                .supports_fast_mode(Some(true))
                .supports_auto_mode(None),
        ];
        app.config.overlay = Some(ConfigOverlayState::ModelAndEffort(ModelAndEffortOverlayState {
            focus: OverlayFocus::Model,
            selected_model: "sonnet".to_owned(),
            selected_effort: EffortLevel::High,
        }));

        let rendered =
            model_overlay_lines(&app).into_iter().map(|line| line.to_string()).collect::<Vec<_>>();

        let sonnet_line =
            rendered.iter().find(|line| line.contains("> Sonnet")).expect("sonnet line");
        assert!(sonnet_line.contains("Effort"));
        assert!(sonnet_line.contains("Adaptive thinking"));
        assert!(sonnet_line.contains("Fast mode"));
        assert!(sonnet_line.contains("Auto mode"));

        let haiku_line = rendered.iter().find(|line| line.contains("  Haiku")).expect("haiku line");
        assert!(haiku_line.contains("Fast mode"));
        assert!(!haiku_line.contains("Auto mode"));
        assert!(rendered.iter().all(|line| !line.contains("no effort")
            && !line.contains("adaptive false")
            && !line.contains('[')));
    }

    #[test]
    fn model_overlay_title_text_uses_human_labels_without_divider() {
        let title = model_overlay_title_text(
            &crate::app::config::OverlayModelOption {
                id: "sonnet".to_owned(),
                display_name: "Sonnet".to_owned(),
                description: None,
                supports_effort: true,
                supported_effort_levels: vec![EffortLevel::Low, EffortLevel::Medium],
                supports_adaptive_thinking: Some(true),
                supports_fast_mode: Some(true),
                supports_auto_mode: Some(false),
            },
            ">",
        );

        assert_eq!(title, "> Sonnet  Effort  Adaptive thinking  Fast mode");
    }

    #[test]
    fn output_style_overlay_lists_expected_options() {
        let mut app = App::test_default();
        app.config.overlay = Some(ConfigOverlayState::OutputStyle(OutputStyleOverlayState {
            selected: OutputStyle::Explanatory,
        }));

        let rendered = super::output_style_overlay_lines(&app)
            .into_iter()
            .map(|line| line.to_string())
            .collect::<Vec<_>>();

        assert!(rendered.iter().any(|line| line.contains("1. Default")));
        assert!(rendered.iter().any(|line| line.contains("2. Explanatory")));
        assert!(rendered.iter().any(|line| line.contains("3. Learning")));
    }

    #[test]
    fn language_overlay_input_uses_placeholder_when_empty() {
        let line =
            super::input::text_input_line("", 0, "e.g. en, Greek, Japanese, Pirate").to_string();

        assert!(line.contains("e.g. en, Greek, Japanese, Pirate"));
    }

    #[test]
    fn session_rename_overlay_input_uses_placeholder_when_empty() {
        let line = super::input::text_input_line("", 0, "Custom session name").to_string();

        assert!(line.contains("Custom session name"));
    }

    #[test]
    fn language_overlay_renders_inline_validation_message() {
        fn buffer_text(buffer: &Buffer) -> String {
            let width = usize::from(buffer.area.width);
            buffer
                .content
                .chunks(width)
                .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
                .collect::<Vec<_>>()
                .join("\n")
        }

        let backend = TestBackend::new(120, 30);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.active_view = crate::app::ActiveView::Config;
        app.config.overlay = Some(ConfigOverlayState::Language(LanguageOverlayState {
            draft: "E".to_owned(),
            cursor: 1,
        }));

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        let rendered = buffer_text(terminal.backend().buffer());
        assert!(rendered.contains("Language must be at least 2 characters."));
    }

    #[test]
    fn output_style_details_do_not_show_unsupported_warning() {
        let mut app = App::test_default();
        app.config.selected_setting_index = setting_specs()
            .iter()
            .position(|spec| spec.id == SettingId::OutputStyle)
            .expect("output style row");

        let rendered = super::settings::setting_detail_lines(&app)
            .into_iter()
            .map(|line| line.to_string())
            .collect::<Vec<_>>();

        assert!(!rendered.iter().any(|line| line.contains("not supported yet")));
    }

    #[test]
    fn compact_settings_layout_triggers_for_small_tuis() {
        assert!(super::settings::compact_settings_layout(Rect::new(0, 0, 89, 25)));
        assert!(super::settings::compact_settings_layout(Rect::new(0, 0, 100, 19)));
        assert!(!super::settings::compact_settings_layout(Rect::new(0, 0, 90, 20)));
    }

    #[test]
    fn compact_settings_list_does_not_inline_warning_for_supported_output_style() {
        fn buffer_text(buffer: &Buffer) -> String {
            let width = usize::from(buffer.area.width);
            buffer
                .content
                .chunks(width)
                .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
                .collect::<Vec<_>>()
                .join("\n")
        }

        let backend = TestBackend::new(80, 16);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.active_view = crate::app::ActiveView::Config;
        app.config.selected_setting_index = setting_specs()
            .iter()
            .position(|spec| spec.id == SettingId::OutputStyle)
            .expect("output style row");

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        let rendered = buffer_text(terminal.backend().buffer());

        assert!(!rendered.contains("not supported yet"));
    }

    #[test]
    fn compact_settings_supported_output_style_does_not_render_warning_lines() {
        fn buffer_lines(buffer: &Buffer) -> Vec<String> {
            let width = usize::from(buffer.area.width);
            buffer
                .content
                .chunks(width)
                .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
                .collect()
        }

        let backend = TestBackend::new(42, 20);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.active_view = crate::app::ActiveView::Config;
        app.config.selected_setting_index = setting_specs()
            .iter()
            .position(|spec| spec.id == SettingId::OutputStyle)
            .expect("output style row");

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        let rendered = buffer_lines(terminal.backend().buffer());

        let warning_lines = rendered
            .iter()
            .filter(|line| {
                line.contains("Warning: not supported yet;")
                    || line.contains("this setting")
                    || line.contains("affect")
                    || line.contains("sessions.")
            })
            .count();

        assert_eq!(warning_lines, 0);
    }

    #[test]
    fn render_updates_settings_scroll_offset_to_keep_selection_visible() {
        let backend = TestBackend::new(80, 16);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.active_view = crate::app::ActiveView::Config;
        app.config.selected_setting_index = setting_specs().len().saturating_sub(1);
        app.config.settings_scroll_offset = 0;

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        assert!(app.config.settings_scroll_offset > 0);
    }

    #[test]
    fn normal_layout_renders_settings_limitation_hint() {
        fn buffer_text(buffer: &Buffer) -> String {
            let width = usize::from(buffer.area.width);
            buffer
                .content
                .chunks(width)
                .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
                .collect::<Vec<_>>()
                .join("\n")
        }

        let backend = TestBackend::new(180, 30);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.active_view = crate::app::ActiveView::Config;

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        let rendered = buffer_text(terminal.backend().buffer());

        assert!(rendered.contains("supported by claude-rs"));
        assert!(rendered.contains("Anthropic Claude Agent SDK"));
    }

    #[test]
    fn compact_layout_renders_settings_limitation_hint() {
        fn buffer_text(buffer: &Buffer) -> String {
            let width = usize::from(buffer.area.width);
            buffer
                .content
                .chunks(width)
                .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
                .collect::<Vec<_>>()
                .join("\n")
        }

        let backend = TestBackend::new(80, 24);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.active_view = crate::app::ActiveView::Config;

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        let rendered = buffer_text(terminal.backend().buffer());

        assert!(rendered.contains("supported by claude-rs"));
        assert!(rendered.contains("Anthropic Claude Agent SDK"));
    }

    #[test]
    fn settings_limitation_hint_wraps_on_narrow_widths() {
        assert_eq!(super::settings::settings_hint_height(200), 1);
        assert!(super::settings::settings_hint_height(40) > 1);
        assert!(
            super::settings::settings_hint_height(20) > super::settings::settings_hint_height(40)
        );
        assert_eq!(super::settings::settings_hint_height(0), 0);
        assert!(!SETTINGS_LIMITATION_HINT.is_empty());
    }

    #[test]
    fn status_tab_renders_session_info() {
        fn buffer_text(buffer: &Buffer) -> String {
            let width = usize::from(buffer.area.width);
            buffer
                .content
                .chunks(width)
                .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
                .collect::<Vec<_>>()
                .join("\n")
        }

        let backend = TestBackend::new(100, 24);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.active_view = crate::app::ActiveView::Config;
        app.config.active_tab = crate::app::ConfigTab::Status;

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        let rendered = buffer_text(terminal.backend().buffer());
        assert!(rendered.contains("Version"), "missing Version");
        assert!(rendered.contains("cwd"), "missing cwd");
        assert!(rendered.contains("Model"), "missing Model");
    }

    #[test]
    fn status_tab_help_omits_space_edit() {
        fn buffer_text(buffer: &Buffer) -> String {
            let width = usize::from(buffer.area.width);
            buffer
                .content
                .chunks(width)
                .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
                .collect::<Vec<_>>()
                .join("\n")
        }

        let backend = TestBackend::new(100, 24);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.active_view = crate::app::ActiveView::Config;
        app.config.active_tab = crate::app::ConfigTab::Status;

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        let rendered = buffer_text(terminal.backend().buffer());
        assert!(!rendered.contains("Space edit"), "Status tab should not show Space edit");
        assert!(rendered.contains("Tab next tab"), "missing tab navigation hint");
        assert!(rendered.contains("Enter close"), "missing Enter close");
    }

    #[test]
    fn usage_tab_help_shows_refresh_hint() {
        fn buffer_text(buffer: &Buffer) -> String {
            let width = usize::from(buffer.area.width);
            buffer
                .content
                .chunks(width)
                .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
                .collect::<Vec<_>>()
                .join("\n")
        }

        let backend = TestBackend::new(100, 24);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.active_view = crate::app::ActiveView::Config;
        app.config.active_tab = crate::app::ConfigTab::Usage;

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        let rendered = buffer_text(terminal.backend().buffer());
        assert!(rendered.contains("r refresh"));
        assert!(rendered.contains("Shift+Tab prev tab"));
    }

    #[test]
    fn settings_tab_help_shows_edit_keys() {
        fn buffer_text(buffer: &Buffer) -> String {
            let width = usize::from(buffer.area.width);
            buffer
                .content
                .chunks(width)
                .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
                .collect::<Vec<_>>()
                .join("\n")
        }

        let backend = TestBackend::new(100, 24);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.active_view = crate::app::ActiveView::Config;
        app.config.active_tab = crate::app::ConfigTab::Settings;

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        let rendered = buffer_text(terminal.backend().buffer());
        assert!(rendered.contains("Left/Right edit"));
        assert!(rendered.contains("Space edit"));
        assert!(rendered.contains("Shift+Tab prev tab"));
    }

    #[test]
    fn plugins_tab_renders_inventory_shell() {
        fn buffer_text(buffer: &Buffer) -> String {
            let width = usize::from(buffer.area.width);
            buffer
                .content
                .chunks(width)
                .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
                .collect::<Vec<_>>()
                .join("\n")
        }

        let backend = TestBackend::new(100, 24);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.active_view = crate::app::ActiveView::Config;
        app.config.active_tab = crate::app::ConfigTab::Plugins;
        app.plugins.installed = vec![crate::app::plugins::InstalledPluginEntry {
            id: "frontend-design@claude-plugins-official".to_owned(),
            version: Some("1.0.0".to_owned()),
            scope: "user".to_owned(),
            enabled: true,
            installed_at: None,
            last_updated: None,
            project_path: None,
            capability: crate::app::plugins::PluginCapability::Skill,
        }];
        app.plugins.marketplace = vec![crate::app::plugins::MarketplaceEntry {
            plugin_id: "frontend-design@claude-plugins-official".to_owned(),
            name: "frontend-design".to_owned(),
            description: Some("Create distinctive interfaces".to_owned()),
            marketplace_name: Some("claude-plugins-official".to_owned()),
            version: Some("1.0.0".to_owned()),
            install_count: Some(42),
            source: None,
        }];
        app.plugins.marketplaces = vec![crate::app::plugins::MarketplaceSourceEntry {
            name: "claude-plugins-official".to_owned(),
            source: Some("github".to_owned()),
            repo: Some("anthropics/claude-plugins-official".to_owned()),
        }];

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        let rendered = buffer_text(terminal.backend().buffer());
        assert!(rendered.contains("Installed (1)"));
        assert!(rendered.contains("Plugins (1)"));
        assert!(rendered.contains("Marketplace (1)"));
        assert!(rendered.contains("Search"));
        assert!(rendered.contains("Type to filter this list"));
        assert!(rendered.contains("Frontend Design From Claude Plugins Official"));
        assert!(rendered.contains("SKILL"));
        assert!(rendered.contains("Left/Right switch list"));
    }

    #[test]
    fn plugins_tab_renders_marketplace_plugin_title_and_plugin_id() {
        fn buffer_text(buffer: &Buffer) -> String {
            let width = usize::from(buffer.area.width);
            buffer
                .content
                .chunks(width)
                .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
                .collect::<Vec<_>>()
                .join("\n")
        }

        let backend = TestBackend::new(100, 24);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.active_view = crate::app::ActiveView::Config;
        app.config.active_tab = crate::app::ConfigTab::Plugins;
        app.plugins.active_tab = crate::app::plugins::PluginsViewTab::Plugins;
        app.plugins.marketplace = vec![crate::app::plugins::MarketplaceEntry {
            plugin_id: "frontend-design@claude-plugins-official".to_owned(),
            name: "frontend-design".to_owned(),
            description: Some("Review UI".to_owned()),
            marketplace_name: Some("claude-plugins-official".to_owned()),
            version: Some("1.0.0".to_owned()),
            install_count: Some(42),
            source: None,
        }];

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        let rendered = buffer_text(terminal.backend().buffer());
        assert!(rendered.contains("Frontend Design"));
        assert!(rendered.contains("Plugin: frontend-design@claude-plugins-official"));
    }

    #[test]
    fn plugins_tab_groups_relevant_installed_plugins_above_other_projects() {
        fn buffer_text(buffer: &Buffer) -> String {
            let width = usize::from(buffer.area.width);
            buffer
                .content
                .chunks(width)
                .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
                .collect::<Vec<_>>()
                .join("\n")
        }

        let backend = TestBackend::new(100, 24);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.cwd_raw = "C:\\work\\project-b".to_owned();
        app.active_view = crate::app::ActiveView::Config;
        app.config.active_tab = crate::app::ConfigTab::Plugins;
        app.plugins.installed = vec![
            crate::app::plugins::InstalledPluginEntry {
                id: "other-local@claude-plugins-official".to_owned(),
                version: Some("1.0.0".to_owned()),
                scope: "local".to_owned(),
                enabled: true,
                installed_at: None,
                last_updated: None,
                project_path: Some("C:\\work\\project-a".to_owned()),
                capability: crate::app::plugins::PluginCapability::Skill,
            },
            crate::app::plugins::InstalledPluginEntry {
                id: "user-plugin@claude-plugins-official".to_owned(),
                version: Some("1.0.0".to_owned()),
                scope: "user".to_owned(),
                enabled: true,
                installed_at: None,
                last_updated: None,
                project_path: None,
                capability: crate::app::plugins::PluginCapability::Skill,
            },
            crate::app::plugins::InstalledPluginEntry {
                id: "current-local@claude-plugins-official".to_owned(),
                version: Some("1.0.0".to_owned()),
                scope: "local".to_owned(),
                enabled: true,
                installed_at: None,
                last_updated: None,
                project_path: Some("C:\\work\\project-b".to_owned()),
                capability: crate::app::plugins::PluginCapability::Skill,
            },
        ];

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        let rendered = buffer_text(terminal.backend().buffer());
        let user_index =
            rendered.find("User Plugin From Claude Plugins Official").expect("user plugin");
        let current_index = rendered
            .find("Current Local From Claude Plugins Official")
            .expect("current project plugin");
        let other_index = rendered
            .find("Other Local From Claude Plugins Official")
            .expect("other project plugin");

        assert!(user_index < other_index);
        assert!(current_index < other_index);
        assert!(rendered.contains("Available here"));
        assert!(rendered.contains("Installed elsewhere"));
    }

    #[test]
    fn plugins_tab_shows_loading_copy_instead_of_empty_state_during_refresh() {
        fn buffer_text(buffer: &Buffer) -> String {
            let width = usize::from(buffer.area.width);
            buffer
                .content
                .chunks(width)
                .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
                .collect::<Vec<_>>()
                .join("\n")
        }

        let backend = TestBackend::new(100, 24);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.active_view = crate::app::ActiveView::Config;
        app.config.active_tab = crate::app::ConfigTab::Plugins;
        app.plugins.loading = true;

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        let rendered = buffer_text(terminal.backend().buffer());
        assert!(rendered.contains("Loading installed plugins..."));
        assert!(!rendered.contains("No installed plugins found."));
    }

    #[test]
    fn marketplace_tab_renders_configured_heading_and_add_placeholder() {
        fn buffer_text(buffer: &Buffer) -> String {
            let width = usize::from(buffer.area.width);
            buffer
                .content
                .chunks(width)
                .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
                .collect::<Vec<_>>()
                .join("\n")
        }

        let backend = TestBackend::new(100, 24);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.active_view = crate::app::ActiveView::Config;
        app.config.active_tab = crate::app::ConfigTab::Plugins;
        app.plugins.active_tab = crate::app::plugins::PluginsViewTab::Marketplace;

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        let rendered = buffer_text(terminal.backend().buffer());
        assert!(rendered.contains("Configured marketplaces"));
        assert!(rendered.contains("Add marketplace"));
    }

    #[test]
    fn installed_plugin_overlay_renders_title_description_and_actions() {
        fn buffer_text(buffer: &Buffer) -> String {
            let width = usize::from(buffer.area.width);
            buffer
                .content
                .chunks(width)
                .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
                .collect::<Vec<_>>()
                .join("\n")
        }

        let backend = TestBackend::new(100, 24);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.active_view = crate::app::ActiveView::Config;
        app.config.active_tab = crate::app::ConfigTab::Plugins;
        app.config.overlay = Some(crate::app::config::ConfigOverlayState::InstalledPluginActions(
            crate::app::config::InstalledPluginActionOverlayState {
                plugin_id: "frontend-design@claude-plugins-official".to_owned(),
                title: "Frontend Design From Claude Plugins Official".to_owned(),
                description: "Create distinctive interfaces".to_owned(),
                scope: "local".to_owned(),
                project_path: Some("C:\\work\\project-a".to_owned()),
                selected_index: 0,
                actions: vec![
                    crate::app::config::InstalledPluginActionKind::Disable,
                    crate::app::config::InstalledPluginActionKind::Update,
                    crate::app::config::InstalledPluginActionKind::InstallInCurrentProject,
                    crate::app::config::InstalledPluginActionKind::Uninstall,
                ],
            },
        ));

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        let rendered = buffer_text(terminal.backend().buffer());
        assert!(rendered.contains("Installed plugin"));
        assert!(rendered.contains("Frontend Design From Claude Plugins Official"));
        assert!(rendered.contains("Create distinctive interfaces"));
        assert!(rendered.contains("Install in current project"));
        assert!(rendered.contains("Up/Down select"));
    }

    #[test]
    fn plugin_install_overlay_renders_title_description_and_actions() {
        fn buffer_text(buffer: &Buffer) -> String {
            let width = usize::from(buffer.area.width);
            buffer
                .content
                .chunks(width)
                .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
                .collect::<Vec<_>>()
                .join("\n")
        }

        let backend = TestBackend::new(100, 24);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.active_view = crate::app::ActiveView::Config;
        app.config.active_tab = crate::app::ConfigTab::Plugins;
        app.config.overlay = Some(crate::app::config::ConfigOverlayState::PluginInstallActions(
            crate::app::config::PluginInstallOverlayState {
                plugin_id: "frontend-design@claude-plugins-official".to_owned(),
                title: "Frontend Design".to_owned(),
                description: "Create distinctive interfaces".to_owned(),
                selected_index: 0,
                actions: vec![
                    crate::app::config::PluginInstallActionKind::User,
                    crate::app::config::PluginInstallActionKind::Project,
                    crate::app::config::PluginInstallActionKind::Local,
                ],
            },
        ));

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        let rendered = buffer_text(terminal.backend().buffer());
        assert!(rendered.contains("Install plugin"));
        assert!(rendered.contains("Frontend Design"));
        assert!(rendered.contains("Create distinctive interfaces"));
        assert!(rendered.contains("Install for project"));
        assert!(rendered.contains("Up/Down select"));
    }

    #[test]
    fn marketplace_actions_overlay_renders_title_description_and_actions() {
        fn buffer_text(buffer: &Buffer) -> String {
            let width = usize::from(buffer.area.width);
            buffer
                .content
                .chunks(width)
                .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
                .collect::<Vec<_>>()
                .join("\n")
        }

        let backend = TestBackend::new(100, 24);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.active_view = crate::app::ActiveView::Config;
        app.config.active_tab = crate::app::ConfigTab::Plugins;
        app.config.overlay = Some(crate::app::config::ConfigOverlayState::MarketplaceActions(
            crate::app::config::MarketplaceActionsOverlayState {
                name: "claude-plugins-official".to_owned(),
                title: "Claude Plugins Official".to_owned(),
                description: "Source: github\nRepo: anthropics/claude-plugins-official".to_owned(),
                selected_index: 0,
                actions: vec![
                    crate::app::config::MarketplaceActionKind::Update,
                    crate::app::config::MarketplaceActionKind::Remove,
                ],
            },
        ));

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        let rendered = buffer_text(terminal.backend().buffer());
        assert!(rendered.contains("Marketplace"));
        assert!(rendered.contains("Claude Plugins Official"));
        assert!(rendered.contains("Source: github"));
        assert!(rendered.contains("Remove"));
    }

    #[test]
    fn add_marketplace_overlay_renders_examples() {
        fn buffer_text(buffer: &Buffer) -> String {
            let width = usize::from(buffer.area.width);
            buffer
                .content
                .chunks(width)
                .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
                .collect::<Vec<_>>()
                .join("\n")
        }

        let backend = TestBackend::new(100, 24);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.active_view = crate::app::ActiveView::Config;
        app.config.active_tab = crate::app::ConfigTab::Plugins;
        app.config.overlay = Some(crate::app::config::ConfigOverlayState::AddMarketplace(
            crate::app::config::AddMarketplaceOverlayState { draft: String::new(), cursor: 0 },
        ));

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        let rendered = buffer_text(terminal.backend().buffer());
        assert!(rendered.contains("Add Marketplace"));
        assert!(rendered.contains("Enter marketplace source:"));
        assert!(rendered.contains("owner/repo (GitHub)"));
        assert!(rendered.contains("Enter add"));
    }

    #[test]
    fn mcp_details_overlay_renders_selected_server_details() {
        use std::collections::BTreeMap;

        fn buffer_text(buffer: &Buffer) -> String {
            let width = usize::from(buffer.area.width);
            buffer
                .content
                .chunks(width)
                .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
                .collect::<Vec<_>>()
                .join("\n")
        }

        let backend = TestBackend::new(100, 24);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.active_view = crate::app::ActiveView::Config;
        app.config.active_tab = crate::app::ConfigTab::Mcp;
        app.config.overlay = Some(crate::app::config::ConfigOverlayState::McpDetails(
            crate::app::config::McpDetailsOverlayState {
                server_name: "filesystem".to_owned(),
                selected_index: 0,
            },
        ));
        app.mcp.servers = vec![crate::agent::types::McpServerStatus {
            name: "filesystem".to_owned(),
            status: crate::agent::types::McpServerConnectionStatus::Connected,
            server_info: Some(crate::agent::types::McpServerInfo {
                name: "Filesystem".to_owned(),
                version: "1.2.3".to_owned(),
            }),
            error: None,
            config: Some(crate::agent::types::McpServerStatusConfig::Stdio {
                command: "npx".to_owned(),
                args: vec!["@modelcontextprotocol/server-filesystem".to_owned()],
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
        }];

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        let rendered = buffer_text(terminal.backend().buffer());
        assert!(rendered.contains("filesystem"));
        assert!(rendered.contains("project"));
        assert!(rendered.contains("stdio"));
        assert!(rendered.contains("Reconnect server"));
        assert!(rendered.contains("Disable server"));
        assert!(rendered.contains("Enter run"));
    }

    #[test]
    fn status_tab_help_shows_generate_and_rename_when_session_is_active() {
        fn buffer_text(buffer: &Buffer) -> String {
            let width = usize::from(buffer.area.width);
            buffer
                .content
                .chunks(width)
                .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
                .collect::<Vec<_>>()
                .join("\n")
        }

        let backend = TestBackend::new(100, 24);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.active_view = crate::app::ActiveView::Config;
        app.config.active_tab = crate::app::ConfigTab::Status;
        app.session_id = Some(crate::agent::model::SessionId::new("session-1"));

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        let rendered = buffer_text(terminal.backend().buffer());
        assert!(rendered.contains("g generate"));
        assert!(rendered.contains("r rename"));
    }

    #[test]
    fn config_footer_renders_status_message_when_present() {
        fn buffer_text(buffer: &Buffer) -> String {
            let width = usize::from(buffer.area.width);
            buffer
                .content
                .chunks(width)
                .map(|row| row.iter().map(ratatui::buffer::Cell::symbol).collect::<String>())
                .collect::<Vec<_>>()
                .join("\n")
        }

        let backend = TestBackend::new(100, 24);
        let mut terminal = Terminal::new(backend).expect("terminal");
        let mut app = App::test_default();
        app.active_view = crate::app::ActiveView::Config;
        app.config.status_message = Some("Renaming session...".to_owned());

        terminal
            .draw(|frame| {
                super::render(frame, &mut app);
            })
            .expect("draw");

        let rendered = buffer_text(terminal.backend().buffer());
        assert!(rendered.contains("Renaming session..."));
    }
}
