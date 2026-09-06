// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

/// Shared list-dialog navigation state used by autocomplete/dropdown UIs.
#[derive(Debug, Clone, Copy, Default)]
pub struct DialogState {
    /// Index of the currently selected item.
    pub selected: usize,
    /// First visible item index in the scroll window.
    pub scroll_offset: usize,
}

impl DialogState {
    /// Clamp selection + scroll to the current item count and viewport size.
    pub fn clamp(&mut self, item_count: usize, max_visible: usize) {
        if item_count == 0 || max_visible == 0 {
            self.selected = 0;
            self.scroll_offset = 0;
            return;
        }

        self.selected = self.selected.min(item_count - 1);
        if self.selected < self.scroll_offset {
            self.scroll_offset = self.selected;
        } else if self.selected >= self.scroll_offset + max_visible {
            self.scroll_offset = self.selected + 1 - max_visible;
        }

        let max_start = item_count.saturating_sub(max_visible);
        self.scroll_offset = self.scroll_offset.min(max_start);
    }

    /// Move selection one item up (with wrap-around).
    pub fn move_up(&mut self, item_count: usize, max_visible: usize) {
        if item_count == 0 {
            self.selected = 0;
            self.scroll_offset = 0;
            return;
        }
        if self.selected == 0 {
            self.selected = item_count - 1;
        } else {
            self.selected -= 1;
        }
        self.clamp(item_count, max_visible);
    }

    /// Move selection one item down (with wrap-around).
    pub fn move_down(&mut self, item_count: usize, max_visible: usize) {
        if item_count == 0 {
            self.selected = 0;
            self.scroll_offset = 0;
            return;
        }
        self.selected = (self.selected + 1) % item_count;
        self.clamp(item_count, max_visible);
    }

    /// Compute the `[start, end)` visible slice for rendering.
    #[must_use]
    pub fn visible_range(&self, item_count: usize, max_visible: usize) -> (usize, usize) {
        if item_count == 0 || max_visible == 0 {
            return (0, 0);
        }
        let max_start = item_count.saturating_sub(max_visible);
        let start = self.scroll_offset.min(max_start);
        let end = (start + max_visible).min(item_count);
        (start, end)
    }
}

#[cfg(test)]
mod tests {
    use super::DialogState;

    #[test]
    fn clamp_resets_when_empty() {
        let mut d = DialogState { selected: 5, scroll_offset: 2 };
        d.clamp(0, 8);
        assert_eq!(d.selected, 0);
        assert_eq!(d.scroll_offset, 0);
    }

    #[test]
    fn move_down_wraps_and_updates_scroll() {
        let mut d = DialogState { selected: 7, scroll_offset: 0 };
        d.move_down(8, 4);
        assert_eq!(d.selected, 0);
        assert_eq!(d.scroll_offset, 0);
    }

    #[test]
    fn visible_range_clamps_scroll_offset() {
        let d = DialogState { selected: 0, scroll_offset: 10 };
        assert_eq!(d.visible_range(6, 4), (2, 6));
    }
}
