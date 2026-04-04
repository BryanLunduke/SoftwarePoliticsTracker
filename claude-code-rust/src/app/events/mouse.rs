// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::super::selection::clear_selection;
use super::super::state::ScrollbarDragState;
use super::super::{App, SelectionKind, SelectionPoint};
use crossterm::event::{MouseEvent, MouseEventKind};

pub(super) const MOUSE_SCROLL_LINES: usize = 3;
const SCROLLBAR_MIN_THUMB_HEIGHT: usize = 1;

struct MouseSelectionPoint {
    kind: SelectionKind,
    point: SelectionPoint,
}

pub(super) fn handle_mouse_event(app: &mut App, mouse: MouseEvent) {
    match mouse.kind {
        MouseEventKind::Down(crossterm::event::MouseButton::Left) => {
            if start_scrollbar_drag(app, mouse) {
                return;
            }
            app.scrollbar_drag = None;
            if let Some(pt) = mouse_point_to_selection(app, mouse) {
                app.selection = Some(super::super::SelectionState {
                    kind: pt.kind,
                    start: pt.point,
                    end: pt.point,
                    dragging: true,
                });
            } else {
                clear_selection(app);
            }
        }
        MouseEventKind::Drag(crossterm::event::MouseButton::Left) => {
            if update_scrollbar_drag(app, mouse) {
                return;
            }
            let pt = mouse_point_to_selection(app, mouse);
            if let (Some(sel), Some(pt)) = (&mut app.selection, pt) {
                sel.end = pt.point;
            }
        }
        MouseEventKind::Up(crossterm::event::MouseButton::Left) => {
            app.scrollbar_drag = None;
            if let Some(sel) = &mut app.selection {
                sel.dragging = false;
            }
        }
        _ => {}
    }
    match mouse.kind {
        MouseEventKind::ScrollUp => {
            if app.selection.is_some() {
                clear_selection(app);
            }
            app.viewport.scroll_up(MOUSE_SCROLL_LINES);
        }
        MouseEventKind::ScrollDown => {
            if app.selection.is_some() {
                clear_selection(app);
            }
            app.viewport.scroll_down(MOUSE_SCROLL_LINES);
        }
        _ => {}
    }
}

#[derive(Clone, Copy)]
pub(super) struct ScrollbarMetrics {
    pub viewport_height: usize,
    pub max_scroll: usize,
    pub thumb_size: usize,
    pub track_space: usize,
}

fn start_scrollbar_drag(app: &mut App, mouse: MouseEvent) -> bool {
    if !mouse_on_scrollbar_rail(app, mouse) {
        return false;
    }
    let Some(metrics) = scrollbar_metrics(app) else {
        return false;
    };
    let Some(local_row) = mouse_row_on_chat_track(app, mouse) else {
        return false;
    };

    let (thumb_top, thumb_size) = current_thumb_geometry(app, metrics);
    let thumb_end = thumb_top.saturating_add(thumb_size);
    let grab_offset = if (thumb_top..thumb_end).contains(&local_row) {
        local_row.saturating_sub(thumb_top)
    } else {
        thumb_size / 2
    };

    set_scroll_from_thumb_top(app, local_row.saturating_sub(grab_offset), metrics);
    app.scrollbar_drag = Some(ScrollbarDragState { thumb_grab_offset: grab_offset });
    clear_selection(app);
    true
}

fn update_scrollbar_drag(app: &mut App, mouse: MouseEvent) -> bool {
    let Some(drag) = app.scrollbar_drag else {
        return false;
    };
    let Some(metrics) = scrollbar_metrics(app) else {
        app.scrollbar_drag = None;
        return false;
    };
    let Some(local_row) = mouse_row_on_chat_track(app, mouse) else {
        return false;
    };

    set_scroll_from_thumb_top(app, local_row.saturating_sub(drag.thumb_grab_offset), metrics);
    true
}

fn scrollbar_metrics(app: &App) -> Option<ScrollbarMetrics> {
    let area = app.rendered_chat_area;
    if area.width == 0 || area.height == 0 {
        return None;
    }

    let viewport_height = area.height as usize;
    let content_height = app.viewport.total_message_height();
    if content_height <= viewport_height {
        return None;
    }

    let max_scroll = content_height.saturating_sub(viewport_height);
    let thumb_size = viewport_height
        .saturating_mul(viewport_height)
        .checked_div(content_height)
        .unwrap_or(0)
        .max(SCROLLBAR_MIN_THUMB_HEIGHT)
        .min(viewport_height);
    let track_space = viewport_height.saturating_sub(thumb_size);

    Some(ScrollbarMetrics { viewport_height, max_scroll, thumb_size, track_space })
}

#[allow(clippy::cast_possible_truncation, clippy::cast_precision_loss, clippy::cast_sign_loss)]
fn current_thumb_geometry(app: &App, metrics: ScrollbarMetrics) -> (usize, usize) {
    let mut thumb_size = app.viewport.scrollbar_thumb_size.round() as usize;
    if thumb_size == 0 {
        thumb_size = metrics.thumb_size;
    }
    thumb_size = thumb_size.max(SCROLLBAR_MIN_THUMB_HEIGHT).min(metrics.viewport_height);
    let max_top = metrics.viewport_height.saturating_sub(thumb_size);
    let thumb_top = app.viewport.scrollbar_thumb_top.round().clamp(0.0, max_top as f32) as usize;
    (thumb_top, thumb_size)
}

#[allow(clippy::cast_possible_truncation, clippy::cast_precision_loss, clippy::cast_sign_loss)]
fn set_scroll_from_thumb_top(app: &mut App, thumb_top: usize, metrics: ScrollbarMetrics) {
    let thumb_top = thumb_top.min(metrics.track_space);
    let target = if metrics.track_space == 0 {
        0
    } else {
        ((thumb_top as f32 / metrics.track_space as f32) * metrics.max_scroll as f32).round()
            as usize
    }
    .min(metrics.max_scroll);

    app.viewport.auto_scroll = false;
    app.viewport.scroll_target = target;
    // Keep content movement responsive while dragging the thumb.
    app.viewport.scroll_pos = target as f32;
    app.viewport.scroll_offset = target;
}

fn mouse_on_scrollbar_rail(app: &App, mouse: MouseEvent) -> bool {
    let area = app.rendered_chat_area;
    if area.width == 0 || area.height == 0 {
        return false;
    }
    let rail_x = area.right().saturating_sub(1);
    mouse.column == rail_x && mouse.row >= area.y && mouse.row < area.bottom()
}

fn mouse_row_on_chat_track(app: &App, mouse: MouseEvent) -> Option<usize> {
    let area = app.rendered_chat_area;
    if area.height == 0 {
        return None;
    }
    let max_row = area.height.saturating_sub(1) as usize;
    if mouse.row < area.y {
        return Some(0);
    }
    if mouse.row >= area.bottom() {
        return Some(max_row);
    }
    Some((mouse.row - area.y) as usize)
}

fn mouse_point_to_selection(app: &App, mouse: MouseEvent) -> Option<MouseSelectionPoint> {
    let input_area = app.rendered_input_area;
    if mouse.column >= input_area.x
        && mouse.column < input_area.right()
        && mouse.row >= input_area.y
        && mouse.row < input_area.bottom()
    {
        let row = (mouse.row - input_area.y) as usize;
        let col = (mouse.column - input_area.x) as usize;
        return Some(MouseSelectionPoint {
            kind: SelectionKind::Input,
            point: SelectionPoint { row, col },
        });
    }

    let chat_area = app.rendered_chat_area;
    if mouse.column >= chat_area.x
        && mouse.column < chat_area.right()
        && mouse.row >= chat_area.y
        && mouse.row < chat_area.bottom()
    {
        let row = (mouse.row - chat_area.y) as usize;
        let col = (mouse.column - chat_area.x) as usize;
        return Some(MouseSelectionPoint {
            kind: SelectionKind::Chat,
            point: SelectionPoint { row, col },
        });
    }
    None
}
