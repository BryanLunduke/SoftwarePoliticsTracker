// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

mod autocomplete;
mod chat;
mod chat_view;
mod config;
mod diff;
mod footer;
pub(crate) mod help;
mod highlight;
mod input;
mod layout;
mod markdown;
mod message;
mod tables;
pub mod theme;
mod todo;
mod tool_call;
mod trusted;

pub use message::{SpinnerState, measure_message_height_cached};

use crate::app::ActiveView;
use crate::app::App;
use ratatui::Frame;

pub fn render(frame: &mut Frame, app: &mut App) {
    match app.active_view {
        ActiveView::Chat => chat_view::render(frame, app),
        ActiveView::Config => config::render(frame, app),
        ActiveView::Trusted => trusted::render(frame, app),
    }
}

pub(crate) fn refresh_selection_snapshot(app: &mut App) {
    let Some(selection) = app.selection else {
        return;
    };

    match (app.active_view, selection.kind) {
        (ActiveView::Chat, crate::app::SelectionKind::Chat) => {
            chat::refresh_selection_snapshot(app);
        }
        (ActiveView::Chat, crate::app::SelectionKind::Input) => {
            input::refresh_selection_snapshot(app);
        }
        _ => {}
    }
}
