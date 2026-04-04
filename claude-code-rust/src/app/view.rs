// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use crate::app::App;
use std::time::Instant;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ActiveView {
    Chat,
    Config,
    Trusted,
}

pub fn set_active_view(app: &mut App, next: ActiveView) {
    if app.active_view == next {
        return;
    }

    clear_transient_view_state(app);
    app.active_view = next;
    if next == ActiveView::Chat {
        app.rebuild_chat_focus_from_state();
    }
    app.needs_redraw = true;
}

fn clear_transient_view_state(app: &mut App) {
    app.selection = None;
    app.scrollbar_drag = None;
    app.active_paste_session = None;
    app.pending_paste_session = None;
    app.pending_paste_text.clear();
    app.pending_submit = None;
    app.help_open = false;
    app.help_view = crate::app::HelpView::default();
    app.help_dialog = crate::app::dialog::DialogState::default();
    app.help_visible_count = 0;
    app.mention = None;
    app.slash = None;
    app.subagent = None;
    if app.active_view == ActiveView::Config {
        app.config.overlay = None;
    }
    app.release_focus_target(crate::app::FocusTarget::Help);
    app.release_focus_target(crate::app::FocusTarget::Mention);
    app.paste_burst.on_non_char_key(Instant::now());
}

#[cfg(test)]
mod tests;
