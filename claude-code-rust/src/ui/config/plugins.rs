use super::theme;
use crate::app::App;
use crate::app::plugins::{
    PluginCapability, PluginsViewTab, display_label, filtered_installed,
    filtered_marketplace_plugins, ordered_installed, relevant_installed_count, search_enabled,
    visible_marketplaces,
};
use ratatui::Frame;
use ratatui::layout::{Constraint, Direction, Layout, Margin, Rect};
use ratatui::style::{Color, Modifier, Style};
use ratatui::text::{Line, Span};
use ratatui::widgets::{Block, Borders, Paragraph, Wrap};
use unicode_width::UnicodeWidthStr;

pub(super) fn render(frame: &mut Frame, area: Rect, app: &App) {
    let body = area.inner(Margin { vertical: 1, horizontal: 1 });
    let top_height = top_region_height(app, body.width);
    let sections = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(1),
            Constraint::Length(1),
            Constraint::Length(top_height),
            Constraint::Min(1),
        ])
        .split(body);

    frame.render_widget(Paragraph::new(tab_header_line(app)), sections[0]);
    render_top_region(frame, sections[2], app);
    render_list_region(frame, sections[3], app);
}

fn render_top_region(frame: &mut Frame, area: Rect, app: &App) {
    if search_enabled(app.plugins.active_tab) {
        frame.render_widget(
            Paragraph::new(search_field_line(app))
                .block(
                    Block::default()
                        .borders(Borders::ALL)
                        .title(if app.plugins.search_focused {
                            " Search "
                        } else {
                            " Search (Up to focus) "
                        })
                        .border_style(if app.plugins.search_focused {
                            Style::default().fg(theme::RUST_ORANGE)
                        } else {
                            Style::default().fg(theme::DIM)
                        }),
                )
                .wrap(Wrap { trim: false }),
            area,
        );
        return;
    }

    frame.render_widget(
        Paragraph::new(Line::from(vec![
            Span::raw(" "),
            Span::styled(
                "Configured marketplaces",
                Style::default().fg(Color::White).add_modifier(Modifier::BOLD),
            ),
        ])),
        area,
    );
}

fn render_list_region(frame: &mut Frame, area: Rect, app: &App) {
    let list_area =
        if area.width > 1 { area.inner(Margin { vertical: 0, horizontal: 1 }) } else { area };
    let rendered = match app.plugins.active_tab {
        PluginsViewTab::Installed => installed_list(app, list_area.width, list_area.height),
        PluginsViewTab::Plugins => plugins_list(app, list_area.width, list_area.height),
        PluginsViewTab::Marketplace => marketplace_list(app, list_area.width, list_area.height),
    };
    frame.render_widget(
        Paragraph::new(rendered.lines).scroll((rendered.scroll, 0)).wrap(Wrap { trim: false }),
        list_area,
    );
}

fn top_region_height(app: &App, width: u16) -> u16 {
    if !search_enabled(app.plugins.active_tab) {
        return 1;
    }

    let content_height = Paragraph::new(search_field_line(app))
        .wrap(Wrap { trim: false })
        .line_count(width.saturating_sub(2).max(1));
    u16::try_from(content_height).unwrap_or(u16::MAX).max(1).saturating_add(2)
}

fn tab_header_line(app: &App) -> Line<'static> {
    let spans = PluginsViewTab::ALL
        .into_iter()
        .enumerate()
        .flat_map(|(index, tab)| {
            let active = tab == app.plugins.active_tab;
            let count = match tab {
                PluginsViewTab::Installed => filtered_installed(&app.plugins).len(),
                PluginsViewTab::Plugins => filtered_marketplace_plugins(&app.plugins).len(),
                PluginsViewTab::Marketplace => visible_marketplaces(&app.plugins).len(),
            };
            let label = format!(" {} ({count}) ", tab.title());
            let mut spans = vec![Span::styled(
                label,
                if active {
                    Style::default()
                        .fg(Color::Black)
                        .bg(theme::RUST_ORANGE)
                        .add_modifier(Modifier::BOLD)
                } else {
                    Style::default().fg(Color::White).add_modifier(Modifier::BOLD)
                },
            )];
            if index + 1 < PluginsViewTab::ALL.len() {
                spans.push(Span::styled("  ", Style::default().fg(theme::DIM)));
            }
            spans
        })
        .collect::<Vec<_>>();
    Line::from(spans)
}

fn search_field_line(app: &App) -> Line<'static> {
    let cursor_style =
        Style::default().fg(Color::Black).bg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD);
    let text_style = Style::default().fg(Color::White);
    let hint_style = Style::default().fg(theme::DIM);
    let query = app.plugins.search_query_for(app.plugins.active_tab);

    if query.is_empty() {
        if app.plugins.search_focused {
            return Line::from(vec![
                Span::styled(" ".to_owned(), cursor_style),
                Span::styled("Type to filter this list".to_owned(), hint_style),
            ]);
        }
        return Line::from(Span::styled("Type to filter this list", hint_style));
    }

    if app.plugins.search_focused {
        return Line::from(vec![
            Span::styled(query.to_owned(), text_style),
            Span::styled(" ".to_owned(), cursor_style),
        ]);
    }

    Line::from(Span::styled(query.to_owned(), text_style))
}

fn installed_list(app: &App, viewport_width: u16, viewport_height: u16) -> RenderedList {
    let entries = ordered_installed(&app.plugins, &app.cwd_raw);
    if entries.is_empty() {
        return RenderedList::single(
            if app.plugins.loading {
                "Loading installed plugins..."
            } else if app.plugins.search_query_for(PluginsViewTab::Installed).is_empty() {
                "No installed plugins found."
            } else {
                "No installed plugins match the current search."
            },
            viewport_height,
        );
    }

    let blocks = entries
        .iter()
        .enumerate()
        .map(|(index, entry)| {
            let selected =
                index == app.plugins.installed_selected_index && !app.plugins.search_focused;
            let mut lines = vec![
                title_line_with_badge(&display_label(&entry.id), Some(entry.capability), selected),
                meta_line(
                    &format!(
                        "{} | {}{}",
                        if entry.enabled { "enabled" } else { "disabled" },
                        entry.scope,
                        entry
                            .version
                            .as_deref()
                            .map_or_else(String::new, |version| format!(" | {version}"))
                    ),
                    selected,
                ),
            ];
            if let Some(project_path) = entry.project_path.as_deref() {
                lines.push(meta_line(&format!("project | {project_path}"), selected));
            }
            lines
        })
        .collect::<Vec<_>>();
    let relevant_count = relevant_installed_count(&app.plugins, &app.cwd_raw);
    let divider_after = if relevant_count > 0 && relevant_count < blocks.len() {
        Some(relevant_count.saturating_sub(1))
    } else {
        None
    };
    let top_label = divider_after.map(|_| section_label_line("Available here"));
    let divider = divider_line(viewport_width, "Installed elsewhere");

    RenderedList::from_blocks_with_sections(
        &blocks,
        app.plugins.installed_selected_index,
        viewport_width,
        viewport_height,
        top_label,
        divider_after,
        &divider,
    )
}

fn plugins_list(app: &App, viewport_width: u16, viewport_height: u16) -> RenderedList {
    let entries = filtered_marketplace_plugins(&app.plugins);
    if entries.is_empty() {
        return RenderedList::single(
            if app.plugins.loading {
                "Loading marketplace plugins..."
            } else if app.plugins.search_query_for(PluginsViewTab::Plugins).is_empty() {
                "No plugins are available from the configured marketplaces."
            } else {
                "No marketplace plugins match the current search."
            },
            viewport_height,
        );
    }

    let blocks = entries
        .iter()
        .enumerate()
        .map(|(index, entry)| {
            let selected =
                index == app.plugins.plugins_selected_index && !app.plugins.search_focused;
            let mut lines = vec![title_line(&display_label(&entry.name), selected)];
            lines.push(meta_line(&format!("Plugin: {}", entry.plugin_id), selected));
            if let Some(description) = entry.description.as_deref() {
                lines.push(meta_line(description, selected));
            }
            if let Some(marketplace_name) = entry.marketplace_name.as_deref() {
                lines.push(meta_line(&format!("Marketplace: {marketplace_name}"), selected));
            }
            if let Some(version) = entry.version.as_deref() {
                lines.push(meta_line(&format!("Version: {version}"), selected));
            }
            lines
        })
        .collect::<Vec<_>>();
    RenderedList::from_blocks(
        &blocks,
        app.plugins.plugins_selected_index,
        viewport_width,
        viewport_height,
    )
}

fn marketplace_list(app: &App, viewport_width: u16, viewport_height: u16) -> RenderedList {
    let entries = visible_marketplaces(&app.plugins);
    if entries.is_empty() && app.plugins.loading {
        return RenderedList::single("Loading configured marketplaces...", viewport_height);
    }
    let mut blocks = entries
        .iter()
        .enumerate()
        .map(|(index, marketplace)| {
            let selected = index == app.plugins.marketplace_selected_index;
            let mut lines = vec![title_line(&display_label(&marketplace.name), selected)];
            if let Some(source) = marketplace.source.as_deref() {
                lines.push(meta_line(&format!("Source: {source}"), selected));
            }
            if let Some(repo) = marketplace.repo.as_deref() {
                lines.push(meta_line(&format!("Repo: {repo}"), selected));
            }
            lines
        })
        .collect::<Vec<_>>();

    blocks.push(vec![
        title_line("Add marketplace", app.plugins.marketplace_selected_index == entries.len()),
        meta_line(
            "Add a marketplace from a GitHub repo, URL, or local path.",
            app.plugins.marketplace_selected_index == entries.len(),
        ),
    ]);

    RenderedList::from_blocks(
        &blocks,
        app.plugins.marketplace_selected_index,
        viewport_width,
        viewport_height,
    )
}

fn title_line(text: &str, selected: bool) -> Line<'static> {
    title_line_with_badge(text, None, selected)
}

fn title_line_with_badge(
    text: &str,
    capability: Option<PluginCapability>,
    selected: bool,
) -> Line<'static> {
    let mut spans = vec![Span::styled(
        text.to_owned(),
        if selected {
            Style::default().fg(Color::Black).bg(theme::RUST_ORANGE).add_modifier(Modifier::BOLD)
        } else {
            Style::default().fg(Color::White).add_modifier(Modifier::BOLD)
        },
    )];
    if let Some(capability) = capability {
        spans.push(Span::styled("  ", Style::default().fg(theme::DIM)));
        let (fg, bg) = capability_badge_colors(capability);
        spans.push(Span::styled(
            format!(" {} ", capability.label()),
            Style::default().fg(fg).bg(bg).add_modifier(Modifier::BOLD),
        ));
    }
    Line::from(spans)
}

fn meta_line(text: &str, selected: bool) -> Line<'static> {
    Line::from(Span::styled(
        format!("  {text}"),
        if selected { Style::default().fg(Color::White) } else { Style::default().fg(theme::DIM) },
    ))
}

struct RenderedList {
    lines: Vec<Line<'static>>,
    scroll: u16,
}

impl RenderedList {
    fn single(message: &str, _viewport_height: u16) -> Self {
        Self {
            lines: vec![Line::from(Span::styled(
                message.to_owned(),
                Style::default().fg(theme::DIM),
            ))],
            scroll: 0,
        }
    }

    fn from_blocks(
        blocks: &[Vec<Line<'static>>],
        selected_index: usize,
        viewport_width: u16,
        viewport_height: u16,
    ) -> Self {
        let mut lines = Vec::new();
        let mut selected_start = 0usize;
        let mut selected_height = 1usize;
        let mut offset = 0usize;

        for (index, block) in blocks.iter().enumerate() {
            let block_height = visual_block_height(block, viewport_width).saturating_add(1);
            if index == selected_index {
                selected_start = offset;
                selected_height = block_height;
            }
            lines.extend(block.iter().cloned());
            lines.push(Line::default());
            offset = offset.saturating_add(block_height);
        }

        Self { lines, scroll: selected_scroll(selected_start, selected_height, viewport_height) }
    }

    fn from_blocks_with_sections(
        blocks: &[Vec<Line<'static>>],
        selected_index: usize,
        viewport_width: u16,
        viewport_height: u16,
        top_label: Option<Line<'static>>,
        divider_after: Option<usize>,
        divider: &Line<'static>,
    ) -> Self {
        let mut lines = Vec::new();
        let mut selected_start = 0usize;
        let mut selected_height = 1usize;
        let mut offset = 0usize;
        let divider_height = visual_line_height(divider, viewport_width).saturating_add(1);
        let top_label_height = top_label
            .as_ref()
            .map_or(0, |line| visual_line_height(line, viewport_width).saturating_add(1));

        if let Some(label) = top_label {
            lines.push(label);
            lines.push(Line::default());
            offset = offset.saturating_add(top_label_height);
        }

        for (index, block) in blocks.iter().enumerate() {
            let block_height = visual_block_height(block, viewport_width).saturating_add(1);
            if index == selected_index {
                selected_start = offset;
                selected_height = block_height;
            }
            lines.extend(block.iter().cloned());
            lines.push(Line::default());
            offset = offset.saturating_add(block_height);

            if divider_after == Some(index) {
                lines.push(divider.clone());
                lines.push(Line::default());
                offset = offset.saturating_add(divider_height);
            }
        }

        Self { lines, scroll: selected_scroll(selected_start, selected_height, viewport_height) }
    }
}

fn selected_scroll(selected_start: usize, selected_height: usize, viewport_height: u16) -> u16 {
    let viewport_height = usize::from(viewport_height.max(1));
    if selected_start.saturating_add(selected_height) <= viewport_height {
        0
    } else {
        u16::try_from(
            selected_start.saturating_add(selected_height).saturating_sub(viewport_height),
        )
        .unwrap_or(u16::MAX)
    }
}

fn visual_block_height(block: &[Line<'static>], viewport_width: u16) -> usize {
    block.iter().map(|line| visual_line_height(line, viewport_width)).sum::<usize>()
}

fn visual_line_height(line: &Line<'static>, viewport_width: u16) -> usize {
    let width = usize::from(viewport_width.max(1));
    let content = line.spans.iter().map(|span| span.content.as_ref()).collect::<String>();
    let visual_width = UnicodeWidthStr::width(content.as_str()).max(1);
    visual_width.div_ceil(width)
}

fn section_label_line(text: &str) -> Line<'static> {
    Line::from(Span::styled(
        text.to_owned(),
        Style::default().fg(Color::White).add_modifier(Modifier::BOLD),
    ))
}

fn capability_badge_colors(capability: PluginCapability) -> (Color, Color) {
    match capability {
        PluginCapability::Skill => (Color::White, Color::Rgb(64, 64, 64)),
        PluginCapability::Mcp => (Color::White, Color::Rgb(34, 92, 124)),
    }
}

fn divider_line(viewport_width: u16, label: &str) -> Line<'static> {
    let min_width = usize::from(viewport_width.max(20));
    let label_text = format!(" {label} ");
    let label_width = UnicodeWidthStr::width(label_text.as_str());
    let fill_width = min_width.saturating_sub(label_width).max(4);
    let left_width = fill_width / 2;
    let right_width = fill_width.saturating_sub(left_width);

    Line::from(vec![
        Span::styled("─".repeat(left_width), Style::default().fg(theme::DIM)),
        Span::styled(label_text, Style::default().fg(Color::White).add_modifier(Modifier::BOLD)),
        Span::styled("─".repeat(right_width), Style::default().fg(theme::DIM)),
    ])
}

#[cfg(test)]
mod tests {
    use super::{search_field_line, top_region_height};
    use crate::app::App;
    use crate::app::plugins::PluginsViewTab;
    use ratatui::widgets::{Paragraph, Wrap};

    #[test]
    fn top_region_height_grows_for_wrapped_search_query() {
        let mut app = App::test_default();
        app.plugins.active_tab = PluginsViewTab::Installed;
        app.plugins.search_focused = true;
        app.plugins.installed_search_query =
            "search query that should wrap across multiple lines".to_owned();

        let expected = Paragraph::new(search_field_line(&app))
            .wrap(Wrap { trim: false })
            .line_count(10)
            .max(1)
            .saturating_add(2);

        assert_eq!(usize::from(top_region_height(&app, 12)), expected);
        assert!(top_region_height(&app, 12) > 3);
    }
}
