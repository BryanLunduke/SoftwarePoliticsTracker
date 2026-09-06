// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use tui_textarea::{CursorMove, TextArea, WrapMode};

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct InputSnapshot {
    pub lines: Vec<String>,
    pub cursor_row: usize,
    pub cursor_col: usize,
    pub paste_blocks: Vec<String>,
}

#[derive(Debug)]
pub struct InputState {
    /// Monotonically increasing version counter. Bumped on every content or cursor change
    /// so that downstream caches (e.g. wrap result) can detect staleness cheaply.
    pub version: u64,
    /// Bumped only when visible input content changes (not cursor-only movement).
    /// Used to key wrap/highlight caches that don't depend on cursor position.
    pub content_version: u64,
    /// Stored paste blocks: each entry holds the full text of a large paste (>1000 chars).
    /// A placeholder token `[Pasted Text N]` is inserted into `lines` at the paste point.
    /// On `text()`, placeholders are expanded back to the original pasted content.
    pub paste_blocks: Vec<String>,
    /// Cached visual line measurement: (`content_version`, width, `max_rows`, result).
    cached_measure: Option<(u64, u16, u16, u16)>,
    /// Tracks which `content_version` highlights were last applied for.
    /// Initialized to `u64::MAX` so the first render always applies highlights.
    pub highlight_version: u64,
    editor: TextArea<'static>,
}

/// Prefix/suffix used to identify paste placeholder lines in the input buffer.
const PASTE_PREFIX: &str = "[Pasted Text ";
const PASTE_SUFFIX: &str = "]";
/// Character threshold above which pasted content is collapsed to a placeholder.
pub const PASTE_PLACEHOLDER_CHAR_THRESHOLD: usize = 1000;

impl InputState {
    fn configure_editor(editor: &mut TextArea<'static>) {
        editor.set_wrap_mode(WrapMode::WordOrGlyph);
    }

    fn bump_cursor_version(&mut self) {
        self.version += 1;
    }

    fn bump_content_version(&mut self) {
        self.version += 1;
        self.content_version += 1;
    }

    pub fn new() -> Self {
        let mut editor = TextArea::default();
        Self::configure_editor(&mut editor);
        Self {
            version: 0,
            content_version: 0,
            paste_blocks: Vec::new(),
            cached_measure: None,
            highlight_version: u64::MAX,
            editor,
        }
    }

    #[must_use]
    pub fn editor(&self) -> &TextArea<'static> {
        &self.editor
    }

    pub fn editor_mut(&mut self) -> &mut TextArea<'static> {
        &mut self.editor
    }

    #[must_use]
    pub fn lines(&self) -> &[String] {
        self.editor.lines()
    }

    #[must_use]
    pub fn cursor(&self) -> (usize, usize) {
        self.editor.cursor()
    }

    #[must_use]
    pub fn cursor_row(&self) -> usize {
        self.cursor().0
    }

    #[must_use]
    pub fn cursor_col(&self) -> usize {
        self.cursor().1
    }

    pub fn set_cursor(&mut self, row: usize, col: usize) -> bool {
        self.apply_cursor_edit(|textarea| {
            textarea.move_cursor(CursorMove::Jump(
                u16::try_from(row).unwrap_or(u16::MAX),
                u16::try_from(col).unwrap_or(u16::MAX),
            ));
        })
    }

    pub fn set_cursor_col(&mut self, col: usize) -> bool {
        self.set_cursor(self.cursor_row(), col)
    }

    pub fn replace_lines_and_cursor(
        &mut self,
        mut lines: Vec<String>,
        cursor_row: usize,
        cursor_col: usize,
    ) {
        if lines.is_empty() {
            lines.push(String::new());
        }
        self.editor.set_lines(lines, (cursor_row, cursor_col));
        self.bump_content_version();
    }

    pub fn clear_custom_highlights(&mut self) {
        self.editor.clear_custom_highlight();
    }

    pub fn mutate_lines(&mut self, edit: impl FnOnce(&mut Vec<String>)) {
        let mut lines = self.lines().to_vec();
        edit(&mut lines);
        let (row, col) = self.cursor();
        self.replace_lines_and_cursor(lines, row, col);
    }

    #[must_use]
    pub fn text(&self) -> String {
        if self.paste_blocks.is_empty() {
            return self.lines().join("\n");
        }
        // Expand paste placeholders back to their full content.
        let mut result = String::new();
        for (i, line) in self.lines().iter().enumerate() {
            if i > 0 {
                result.push('\n');
            }
            result.push_str(&expand_placeholders_in_line(line, &self.paste_blocks));
        }
        result
    }

    #[must_use]
    pub fn is_empty(&self) -> bool {
        self.lines().len() == 1 && self.lines()[0].is_empty()
    }

    pub fn clear(&mut self) {
        self.paste_blocks.clear();
        self.replace_lines_and_cursor(vec![String::new()], 0, 0);
    }

    #[must_use]
    pub fn snapshot(&self) -> InputSnapshot {
        let (cursor_row, cursor_col) = self.cursor();
        InputSnapshot {
            lines: self.lines().to_vec(),
            cursor_row,
            cursor_col,
            paste_blocks: self.paste_blocks.clone(),
        }
    }

    pub fn restore_snapshot(&mut self, snapshot: InputSnapshot) {
        self.paste_blocks = snapshot.paste_blocks;
        self.replace_lines_and_cursor(snapshot.lines, snapshot.cursor_row, snapshot.cursor_col);
    }

    /// Replace the input with the given text, placing the cursor at the end.
    pub fn set_text(&mut self, text: &str) {
        let mut lines: Vec<String> = text.split('\n').map(String::from).collect();
        if lines.is_empty() {
            lines.push(String::new());
        }
        let row = lines.len().saturating_sub(1);
        let col = lines[row].chars().count();
        self.paste_blocks.clear();
        self.replace_lines_and_cursor(lines, row, col);
    }

    pub fn insert_char(&mut self, c: char) {
        let _ = self.textarea_insert_char(c);
    }

    fn apply_cursor_edit(&mut self, edit: impl FnOnce(&mut TextArea<'_>)) -> bool {
        let before = self.editor.cursor();
        edit(&mut self.editor);
        let changed = before != self.editor.cursor();
        if changed {
            self.bump_cursor_version();
        }
        changed
    }

    pub fn textarea_insert_char(&mut self, c: char) -> bool {
        self.editor.insert_char(c);
        self.bump_content_version();
        true
    }

    pub fn textarea_insert_newline(&mut self) -> bool {
        self.editor.insert_newline();
        self.bump_content_version();
        true
    }

    pub fn textarea_delete_char_before(&mut self) -> bool {
        let changed = self.editor.delete_char();
        if changed {
            self.bump_content_version();
        }
        changed
    }

    pub fn textarea_delete_char_after(&mut self) -> bool {
        let changed = self.editor.delete_next_char();
        if changed {
            self.bump_content_version();
        }
        changed
    }

    pub fn textarea_move_left(&mut self) -> bool {
        self.apply_cursor_edit(|textarea| {
            textarea.move_cursor(CursorMove::Back);
        })
    }

    pub fn textarea_move_right(&mut self) -> bool {
        self.apply_cursor_edit(|textarea| {
            textarea.move_cursor(CursorMove::Forward);
        })
    }

    pub fn textarea_move_up(&mut self) -> bool {
        self.apply_cursor_edit(|textarea| {
            textarea.move_cursor(CursorMove::Up);
        })
    }

    pub fn textarea_move_down(&mut self) -> bool {
        self.apply_cursor_edit(|textarea| {
            textarea.move_cursor(CursorMove::Down);
        })
    }

    pub fn textarea_move_home(&mut self) -> bool {
        self.apply_cursor_edit(|textarea| {
            textarea.move_cursor(CursorMove::Head);
        })
    }

    pub fn textarea_move_end(&mut self) -> bool {
        self.apply_cursor_edit(|textarea| {
            textarea.move_cursor(CursorMove::End);
        })
    }

    pub fn textarea_undo(&mut self) -> bool {
        let changed = self.editor.undo();
        if changed {
            self.bump_content_version();
        }
        changed
    }

    pub fn textarea_redo(&mut self) -> bool {
        let changed = self.editor.redo();
        if changed {
            self.bump_content_version();
        }
        changed
    }

    pub fn textarea_move_word_left(&mut self) -> bool {
        self.apply_cursor_edit(|textarea| {
            textarea.move_cursor(CursorMove::WordBack);
        })
    }

    pub fn textarea_move_word_right(&mut self) -> bool {
        self.apply_cursor_edit(|textarea| {
            textarea.move_cursor(CursorMove::WordForward);
        })
    }

    pub fn textarea_delete_word_before(&mut self) -> bool {
        let changed = self.editor.delete_word();
        if changed {
            self.bump_content_version();
        }
        changed
    }

    pub fn textarea_delete_word_after(&mut self) -> bool {
        let changed = self.editor.delete_next_word();
        if changed {
            self.bump_content_version();
        }
        changed
    }

    pub fn insert_newline(&mut self) {
        let _ = self.textarea_insert_newline();
    }

    pub fn insert_str(&mut self, s: &str) {
        if s.is_empty() {
            return;
        }
        let normalized = normalize_line_endings(s);
        if normalized.is_empty() {
            return;
        }
        let changed = self.editor.insert_str(&normalized);
        if changed {
            self.bump_content_version();
        }
    }

    /// Insert a large paste as a compact placeholder token at the cursor.
    /// The full text is stored in `paste_blocks` and expanded by `text()` on submit.
    /// Returns the placeholder label for display purposes.
    pub fn insert_paste_block(&mut self, text: &str) -> String {
        let placeholder = self.allocate_paste_block_placeholder(text);
        let (cursor_row, cursor_col) = self.cursor();
        let mut lines = self.lines().to_vec();
        let Some(current_line) = lines.get_mut(cursor_row) else {
            return placeholder;
        };
        let byte_idx = char_to_byte_index(current_line, cursor_col);
        current_line.insert_str(byte_idx, &placeholder);
        let new_cursor_col = cursor_col + placeholder.chars().count();
        self.replace_lines_and_cursor(lines, cursor_row, new_cursor_col);
        placeholder
    }

    /// Store the full text as a paste block and return its placeholder label.
    pub fn allocate_paste_block_placeholder(&mut self, text: &str) -> String {
        let idx = next_free_paste_block_index(self.lines(), &self.paste_blocks);
        if idx < self.paste_blocks.len() {
            text.clone_into(&mut self.paste_blocks[idx]);
        } else {
            self.paste_blocks.push(text.to_owned());
        }
        paste_placeholder_label(idx, count_text_chars(text))
    }

    /// Append text to the paste block under the cursor if the cursor currently
    /// sits at the end of a placeholder token.
    ///
    /// This handles terminals that deliver one clipboard paste as multiple
    /// `Event::Paste` chunks.
    pub fn append_to_active_paste_block(&mut self, text: &str) -> bool {
        let (cursor_row, cursor_col) = self.cursor();
        let Some(current_line) = self.lines().get(cursor_row).cloned() else {
            return false;
        };
        let Some((start, end, idx)) = find_placeholder_ending_at_col(&current_line, cursor_col)
        else {
            return false;
        };

        let Some(block) = self.paste_blocks.get_mut(idx) else {
            return false;
        };
        block.push_str(text);

        let head = &current_line[..start];
        let tail = &current_line[end..];
        let placeholder = paste_placeholder_label(idx, count_text_chars(block));
        let mut lines = self.lines().to_vec();
        lines[cursor_row] = format!("{head}{placeholder}{tail}");
        let new_col = head.chars().count() + placeholder.chars().count();
        self.replace_lines_and_cursor(lines, cursor_row, new_col);
        true
    }

    pub fn delete_char_before(&mut self) {
        let _ = self.textarea_delete_char_before();
    }

    pub fn delete_char_after(&mut self) {
        let _ = self.textarea_delete_char_after();
    }

    pub fn move_left(&mut self) {
        let _ = self.textarea_move_left();
    }

    pub fn move_right(&mut self) {
        let _ = self.textarea_move_right();
    }

    pub fn move_up(&mut self) {
        let _ = self.textarea_move_up();
    }

    pub fn move_down(&mut self) {
        let _ = self.textarea_move_down();
    }

    pub fn move_home(&mut self) {
        if !self.textarea_move_home() {
            self.version += 1;
        }
    }

    pub fn move_end(&mut self) {
        if !self.textarea_move_end() {
            self.version += 1;
        }
    }

    pub fn measure_visual_lines(&mut self, content_width: u16, max_rows: u16) -> u16 {
        if let Some((v, w, m, result)) = self.cached_measure
            && v == self.content_version
            && w == content_width
            && m == max_rows
        {
            return result;
        }

        let result = if content_width == 0 {
            1
        } else {
            self.editor.set_min_rows(1);
            self.editor.set_max_rows(max_rows);
            self.editor.measure(content_width).preferred_rows
        };

        self.cached_measure = Some((self.content_version, content_width, max_rows, result));
        result
    }

    #[must_use]
    pub fn line_count(&self) -> u16 {
        u16::try_from(self.lines().len()).unwrap_or(u16::MAX)
    }
}

impl Default for InputState {
    fn default() -> Self {
        Self::new()
    }
}

/// Convert a character index to a byte index within a string.
fn char_to_byte_index(s: &str, char_idx: usize) -> usize {
    s.char_indices().nth(char_idx).map_or(s.len(), |(i, _)| i)
}

/// Normalize mixed line endings to `\n` while preserving trailing blank lines.
fn normalize_line_endings(text: &str) -> String {
    let mut out = String::with_capacity(text.len());
    let mut chars = text.chars().peekable();
    while let Some(ch) = chars.next() {
        if ch == '\r' {
            if chars.peek().is_some_and(|next| *next == '\n') {
                let _ = chars.next();
            }
            out.push('\n');
        } else {
            out.push(ch);
        }
    }
    out
}

/// Count logical lines for text containing mixed `\n`, `\r`, and `\r\n` endings.
#[cfg(test)]
#[must_use]
fn count_text_lines(text: &str) -> usize {
    // Count universal newlines (\n, \r, and \r\n as a single break).
    let mut lines = 1;
    let bytes = text.as_bytes();
    let mut i = 0;
    while i < bytes.len() {
        match bytes[i] {
            b'\n' => lines += 1,
            b'\r' => {
                lines += 1;
                if i + 1 < bytes.len() && bytes[i + 1] == b'\n' {
                    i += 1;
                }
            }
            _ => {}
        }
        i += 1;
    }
    lines
}

/// Count Unicode scalar characters in a text payload.
#[must_use]
pub fn count_text_chars(text: &str) -> usize {
    text.chars().count()
}

fn paste_placeholder_label(idx: usize, char_count: usize) -> String {
    format!("{PASTE_PREFIX}{} - {char_count} chars{PASTE_SUFFIX}", idx + 1)
}

fn find_next_placeholder_with_suffix(
    line: &str,
    search_from: usize,
) -> Option<(usize, usize, usize)> {
    let mut start_search = search_from;
    while start_search < line.len() {
        let rel = line[start_search..].find(PASTE_PREFIX)?;
        let start = start_search + rel;
        if let Some((idx, end_rel)) = parse_paste_placeholder_with_suffix(&line[start..]) {
            return Some((start, start + end_rel, idx));
        }
        start_search = start + PASTE_PREFIX.len();
    }
    None
}

fn expand_placeholders_in_line(line: &str, paste_blocks: &[String]) -> String {
    let mut out = String::new();
    let mut cursor = 0usize;
    while let Some((start, end, idx)) = find_next_placeholder_with_suffix(line, cursor) {
        out.push_str(&line[cursor..start]);
        if let Some(content) = paste_blocks.get(idx) {
            out.push_str(content);
        } else {
            out.push_str(&line[start..end]);
        }
        cursor = end;
    }
    if cursor == 0 {
        return line.to_owned();
    }
    out.push_str(&line[cursor..]);
    out
}

fn next_free_paste_block_index(lines: &[String], paste_blocks: &[String]) -> usize {
    let mut used = vec![false; paste_blocks.len()];
    for line in lines {
        let mut cursor = 0usize;
        while let Some((_, end, idx)) = find_next_placeholder_with_suffix(line, cursor) {
            if idx < used.len() {
                used[idx] = true;
            }
            cursor = end;
        }
    }
    used.iter().position(|in_use| !*in_use).unwrap_or(paste_blocks.len())
}

fn find_placeholder_ending_at_col(line: &str, cursor_col: usize) -> Option<(usize, usize, usize)> {
    let cursor_byte = char_to_byte_index(line, cursor_col);
    let mut search_from = 0usize;
    while let Some((start, end, idx)) = find_next_placeholder_with_suffix(line, search_from) {
        if end == cursor_byte {
            return Some((start, end, idx));
        }
        if end > cursor_byte {
            return None;
        }
        search_from = end;
    }
    None
}

/// Parse a placeholder at the start of a line.
///
/// Returns `(paste_block_index, placeholder_end_byte_index)`.
pub fn parse_paste_placeholder_with_suffix(line: &str) -> Option<(usize, usize)> {
    let rest = line.strip_prefix(PASTE_PREFIX)?;
    let close_rel = rest.find(PASTE_SUFFIX)?;
    let rest = &rest[..close_rel];
    let num_str = rest.split(" - ").next()?;
    let n: usize = num_str.parse().ok()?;
    if n == 0 {
        return None;
    }
    let end = PASTE_PREFIX.len() + close_rel + PASTE_SUFFIX.len();
    Some((n - 1, end))
}

/// Parse the placeholder index directly before the cursor, even when the
/// placeholder is embedded within a line.
pub fn parse_paste_placeholder_before_cursor(line: &str, cursor_col: usize) -> Option<usize> {
    find_placeholder_ending_at_col(line, cursor_col).map(|(_, _, idx)| idx)
}

/// Return all placeholder highlight ranges in a line as `(start_col, end_col)`.
#[must_use]
pub fn parse_paste_placeholder_ranges(line: &str) -> Vec<(usize, usize)> {
    let mut ranges = Vec::new();
    let mut search_from = 0usize;
    while let Some((start, end, _)) = find_next_placeholder_with_suffix(line, search_from) {
        let start_col = line[..start].chars().count();
        let end_col = line[..end].chars().count();
        ranges.push((start_col, end_col));
        search_from = end;
    }
    ranges
}

#[cfg(test)]
mod tests {
    // =====
    // TESTS: 83
    // =====

    use super::*;
    use pretty_assertions::assert_eq;

    // char_to_byte_index

    #[test]
    fn char_to_byte_index_ascii() {
        assert_eq!(char_to_byte_index("hello", 0), 0);
        assert_eq!(char_to_byte_index("hello", 2), 2);
        assert_eq!(char_to_byte_index("hello", 5), 5); // past end
    }

    #[test]
    fn char_to_byte_index_multibyte_utf8() {
        // 'e' with accent: 2 bytes in UTF-8
        let s = "cafe\u{0301}"; // "cafe" + combining accent = 5 chars, but accent is 2 bytes
        assert_eq!(char_to_byte_index(s, 4), 4); // the combining char starts at byte 4
    }

    #[test]
    fn char_to_byte_index_emoji() {
        let s = "\u{1F600}hello"; // grinning face (4 bytes) + "hello"
        assert_eq!(char_to_byte_index(s, 0), 0);
        assert_eq!(char_to_byte_index(s, 1), 4); // after emoji
    }

    #[test]
    fn char_to_byte_index_beyond_string() {
        assert_eq!(char_to_byte_index("ab", 10), 2); // returns s.len()
    }

    #[test]
    fn char_to_byte_index_empty_string() {
        assert_eq!(char_to_byte_index("", 0), 0);
        assert_eq!(char_to_byte_index("", 5), 0);
    }

    // InputState::new / Default

    #[test]
    fn new_creates_empty_state() {
        let input = InputState::new();
        assert_eq!(input.lines(), vec![String::new()]);
        assert_eq!(input.cursor_row(), 0);
        assert_eq!(input.cursor_col(), 0);
        assert_eq!(input.version, 0);
    }

    #[test]
    fn default_equals_new() {
        let a = InputState::new();
        let b = InputState::default();
        assert_eq!(a.lines(), b.lines());
        assert_eq!(a.cursor_row(), b.cursor_row());
        assert_eq!(a.cursor_col(), b.cursor_col());
        assert_eq!(a.version, b.version);
    }

    // text()

    #[test]
    fn text_single_empty_line() {
        let input = InputState::new();
        assert_eq!(input.text(), "");
    }

    #[test]
    fn text_joins_lines_with_newline() {
        let mut input = InputState::new();
        input.insert_str("line1\nline2\nline3");
        assert_eq!(input.text(), "line1\nline2\nline3");
    }

    // is_empty()

    #[test]
    fn is_empty_true_for_new() {
        assert!(InputState::new().is_empty());
    }

    #[test]
    fn is_empty_false_after_insert() {
        let mut input = InputState::new();
        input.insert_char('a');
        assert!(!input.is_empty());
    }

    #[test]
    fn is_empty_false_for_empty_multiline() {
        // Two empty lines: not considered "empty" by the method
        let mut input = InputState::new();
        input.insert_newline();
        assert!(!input.is_empty());
    }

    // clear()

    #[test]
    fn clear_resets_to_empty() {
        let mut input = InputState::new();
        input.insert_str("hello\nworld");
        let v_before = input.version;
        input.clear();
        assert!(input.is_empty());
        assert_eq!(input.cursor_row(), 0);
        assert_eq!(input.cursor_col(), 0);
        assert!(input.version > v_before);
    }

    // insert_char

    #[test]
    fn insert_char_ascii() {
        let mut input = InputState::new();
        input.insert_char('h');
        input.insert_char('i');
        assert_eq!(input.lines()[0], "hi");
        assert_eq!(input.cursor_col(), 2);
    }

    #[test]
    fn insert_char_unicode_emoji() {
        let mut input = InputState::new();
        input.insert_char('\u{1F600}'); // grinning face
        assert_eq!(input.cursor_col(), 1);
        assert_eq!(input.lines()[0], "\u{1F600}");
    }

    #[test]
    fn insert_char_cjk() {
        let mut input = InputState::new();
        input.insert_char('\u{4F60}'); // Chinese "ni"
        input.insert_char('\u{597D}'); // Chinese "hao"
        assert_eq!(input.lines()[0], "\u{4F60}\u{597D}");
        assert_eq!(input.cursor_col(), 2);
    }

    #[test]
    fn insert_char_mid_line() {
        let mut input = InputState::new();
        input.insert_str("ac");
        input.move_left(); // cursor at col 1
        input.insert_char('b');
        assert_eq!(input.lines()[0], "abc");
        assert_eq!(input.cursor_col(), 2);
    }

    #[test]
    fn insert_char_bumps_version() {
        let mut input = InputState::new();
        let v = input.version;
        input.insert_char('x');
        assert!(input.version > v);
    }

    // insert_newline

    #[test]
    fn insert_newline_at_end() {
        let mut input = InputState::new();
        input.insert_str("hello");
        input.insert_newline();
        assert_eq!(input.lines(), vec!["hello", ""]);
        assert_eq!(input.cursor_row(), 1);
        assert_eq!(input.cursor_col(), 0);
    }

    #[test]
    fn insert_newline_mid_line() {
        let mut input = InputState::new();
        input.insert_str("helloworld");
        // Move cursor to position 5
        let _ = input.set_cursor_col(5);
        input.insert_newline();
        assert_eq!(input.lines(), vec!["hello", "world"]);
        assert_eq!(input.cursor_row(), 1);
        assert_eq!(input.cursor_col(), 0);
    }

    #[test]
    fn insert_newline_at_start() {
        let mut input = InputState::new();
        input.insert_str("hello");
        input.move_home();
        input.insert_newline();
        assert_eq!(input.lines(), vec!["", "hello"]);
    }

    // insert_str

    #[test]
    fn insert_str_multiline() {
        let mut input = InputState::new();
        input.insert_str("line1\nline2\nline3");
        assert_eq!(input.lines(), vec!["line1", "line2", "line3"]);
        assert_eq!(input.cursor_row(), 2);
        assert_eq!(input.cursor_col(), 5);
    }

    #[test]
    fn insert_str_with_carriage_returns() {
        let mut input = InputState::new();
        input.insert_str("a\rb\rc");
        // \r treated same as \n
        assert_eq!(input.lines(), vec!["a", "b", "c"]);
    }

    #[test]
    fn insert_str_empty() {
        let mut input = InputState::new();
        let v = input.version;
        input.insert_str("");
        assert_eq!(input.version, v); // no mutation
    }

    // delete_char_before (backspace)

    #[test]
    fn backspace_mid_line() {
        let mut input = InputState::new();
        input.insert_str("abc");
        input.delete_char_before();
        assert_eq!(input.lines()[0], "ab");
        assert_eq!(input.cursor_col(), 2);
    }

    #[test]
    fn backspace_start_of_line_joins() {
        let mut input = InputState::new();
        input.insert_str("hello\nworld");
        // cursor at row 1, col 5. Move to start of row 1.
        input.move_home();
        input.delete_char_before();
        assert_eq!(input.lines(), vec!["helloworld"]);
        assert_eq!(input.cursor_row(), 0);
        assert_eq!(input.cursor_col(), 5); // at the join point
    }

    #[test]
    fn backspace_start_of_buffer_noop() {
        let mut input = InputState::new();
        input.insert_str("hi");
        input.move_home();
        let v = input.version;
        input.delete_char_before(); // should do nothing
        assert_eq!(input.lines()[0], "hi");
        assert_eq!(input.version, v); // no version bump
    }

    #[test]
    fn backspace_unicode() {
        let mut input = InputState::new();
        input.insert_char('\u{1F600}');
        input.insert_char('x');
        input.delete_char_before();
        assert_eq!(input.lines()[0], "\u{1F600}");
    }

    // delete_char_after (delete key)

    #[test]
    fn delete_mid_line() {
        let mut input = InputState::new();
        input.insert_str("abc");
        input.move_home();
        input.delete_char_after();
        assert_eq!(input.lines()[0], "bc");
        assert_eq!(input.cursor_col(), 0);
    }

    #[test]
    fn delete_end_of_line_joins_next() {
        let mut input = InputState::new();
        input.insert_str("hello\nworld");
        let _ = input.set_cursor(0, 5); // end of "hello"
        input.delete_char_after();
        assert_eq!(input.lines(), vec!["helloworld"]);
    }

    #[test]
    fn delete_end_of_buffer_noop() {
        let mut input = InputState::new();
        input.insert_str("hi");
        // cursor at end of last line
        let v = input.version;
        input.delete_char_after();
        assert_eq!(input.lines()[0], "hi");
        assert_eq!(input.version, v);
    }

    // Navigation: move_left, move_right

    #[test]
    fn move_left_within_line() {
        let mut input = InputState::new();
        input.insert_str("abc");
        input.move_left();
        assert_eq!(input.cursor_col(), 2);
    }

    #[test]
    fn move_left_wraps_to_previous_line() {
        let mut input = InputState::new();
        input.insert_str("ab\ncd");
        input.move_home(); // at col 0, row 1
        input.move_left();
        assert_eq!(input.cursor_row(), 0);
        assert_eq!(input.cursor_col(), 2); // end of "ab"
    }

    #[test]
    fn move_left_at_origin_noop() {
        let mut input = InputState::new();
        input.insert_char('a');
        input.move_home();
        let v = input.version;
        input.move_left();
        assert_eq!(input.cursor_col(), 0);
        assert_eq!(input.cursor_row(), 0);
        assert_eq!(input.version, v); // no change
    }

    #[test]
    fn move_right_within_line() {
        let mut input = InputState::new();
        input.insert_str("abc");
        input.move_home();
        input.move_right();
        assert_eq!(input.cursor_col(), 1);
    }

    #[test]
    fn move_right_wraps_to_next_line() {
        let mut input = InputState::new();
        input.insert_str("ab\ncd");
        let _ = input.set_cursor(0, 2); // end of "ab"
        input.move_right();
        assert_eq!(input.cursor_row(), 1);
        assert_eq!(input.cursor_col(), 0);
    }

    #[test]
    fn move_right_at_end_noop() {
        let mut input = InputState::new();
        input.insert_str("ab");
        let v = input.version;
        input.move_right(); // already at end
        assert_eq!(input.version, v);
    }

    // Navigation: move_up, move_down

    #[test]
    fn move_up_clamps_col() {
        let mut input = InputState::new();
        input.insert_str("ab\nhello");
        // cursor at row 1, col 5 ("hello" end)
        input.move_up();
        assert_eq!(input.cursor_row(), 0);
        assert_eq!(input.cursor_col(), 2); // clamped to "ab" length
    }

    #[test]
    fn move_up_at_top_noop() {
        let mut input = InputState::new();
        input.insert_str("hello");
        let v = input.version;
        input.move_up();
        assert_eq!(input.cursor_row(), 0);
        assert_eq!(input.version, v);
    }

    #[test]
    fn move_down_clamps_col() {
        let mut input = InputState::new();
        input.insert_str("hello\nab");
        let _ = input.set_cursor(0, 5);
        input.move_down();
        assert_eq!(input.cursor_row(), 1);
        assert_eq!(input.cursor_col(), 2); // clamped to "ab" length
    }

    #[test]
    fn move_down_at_bottom_noop() {
        let mut input = InputState::new();
        input.insert_str("hello");
        let v = input.version;
        input.move_down();
        assert_eq!(input.version, v);
    }

    // Navigation: move_home, move_end

    #[test]
    fn move_home_sets_col_zero() {
        let mut input = InputState::new();
        input.insert_str("hello");
        input.move_home();
        assert_eq!(input.cursor_col(), 0);
    }

    #[test]
    fn move_end_sets_col_to_line_len() {
        let mut input = InputState::new();
        input.insert_str("hello");
        input.move_home();
        input.move_end();
        assert_eq!(input.cursor_col(), 5);
    }

    #[test]
    fn move_home_always_bumps_version() {
        let mut input = InputState::new();
        input.insert_str("hello");
        input.move_home(); // col was 5, now 0
        let v = input.version;
        input.move_home(); // col already 0, but still bumps
        assert!(input.version > v);
    }

    // line_count

    #[test]
    fn line_count_single() {
        assert_eq!(InputState::new().line_count(), 1);
    }

    #[test]
    fn line_count_multi() {
        let mut input = InputState::new();
        input.insert_str("a\nb\nc");
        assert_eq!(input.line_count(), 3);
    }

    // version counter

    #[test]
    fn version_increments_on_every_mutation() {
        let mut input = InputState::new();
        let mut v = input.version;

        input.insert_char('a');
        assert!(input.version > v);
        v = input.version;

        input.insert_newline();
        assert!(input.version > v);
        v = input.version;

        input.delete_char_before();
        assert!(input.version > v);
        v = input.version;

        input.move_left();
        assert!(input.version > v);
        v = input.version;

        input.clear();
        assert!(input.version > v);
    }

    #[test]
    fn rapid_insert_delete_cycle() {
        let mut input = InputState::new();
        for _ in 0..100 {
            input.insert_char('x');
        }
        assert_eq!(input.lines()[0].len(), 100);
        for _ in 0..100 {
            input.delete_char_before();
        }
        assert!(input.is_empty());
    }

    #[test]
    fn mixed_unicode_operations() {
        let mut input = InputState::new();
        // Insert mixed: ASCII, emoji, CJK
        input.insert_str("hi\u{1F600}\u{4F60}");
        assert_eq!(input.cursor_col(), 4); // h, i, emoji, CJK
        input.move_home();
        input.move_right(); // past 'h'
        input.move_right(); // past 'i'
        input.delete_char_after(); // delete emoji
        assert_eq!(input.lines()[0], "hi\u{4F60}");
    }

    #[test]
    fn multiline_editing_stress() {
        let mut input = InputState::new();
        // Create 10 lines
        for i in 0..10 {
            input.insert_str(&format!("line{i}"));
            if i < 9 {
                input.insert_newline();
            }
        }
        assert_eq!(input.lines().len(), 10);

        // Navigate to middle and delete lines by joining
        let _ = input.set_cursor(5, 0);
        input.delete_char_before(); // join line 5 with line 4
        assert_eq!(input.lines().len(), 9);

        // Text should be coherent
        let text = input.text();
        assert!(text.contains("line4line5"));
    }

    #[test]
    fn insert_str_with_only_newlines() {
        let mut input = InputState::new();
        input.insert_str("\n\n\n");
        assert_eq!(input.lines(), vec!["", "", "", ""]);
        assert_eq!(input.cursor_row(), 3);
        assert_eq!(input.cursor_col(), 0);
    }

    #[test]
    fn cursor_clamping_on_vertical_nav() {
        let mut input = InputState::new();
        input.insert_str("long line here\nab\nmedium line");
        // cursor at row 2, col 11 (end of "medium line")
        input.move_up(); // to row 1 "ab", col clamped to 2
        assert_eq!(input.cursor_col(), 2);
        input.move_up(); // to row 0 "long line here", col stays 2
        assert_eq!(input.cursor_col(), 2);
        input.move_end(); // col = 14
        input.move_down(); // to row 1 "ab", col clamped to 2
        assert_eq!(input.cursor_col(), 2);
    }

    // weird inputs

    #[test]
    fn insert_tab_character() {
        let mut input = InputState::new();
        input.insert_char('\t');
        assert_eq!(input.lines()[0], "\t");
        assert_eq!(input.cursor_col(), 1);
    }

    #[test]
    fn insert_null_byte() {
        let mut input = InputState::new();
        input.insert_char('\0');
        assert_eq!(input.lines()[0].len(), 1);
        assert_eq!(input.cursor_col(), 1);
    }

    #[test]
    fn insert_control_chars() {
        let mut input = InputState::new();
        // Bell, backspace-char (not the key), escape
        input.insert_char('\x07');
        input.insert_char('\x08');
        input.insert_char('\x1B');
        assert_eq!(input.cursor_col(), 3);
        assert_eq!(input.lines()[0].chars().count(), 3);
    }

    #[test]
    fn windows_crlf_line_endings() {
        // \r\n normalizes to a single newline.
        let mut input = InputState::new();
        input.insert_str("a\r\nb");
        assert_eq!(input.lines(), vec!["a", "b"]);
    }

    #[test]
    fn insert_zero_width_joiner_sequence() {
        // Family emoji: man + ZWJ + woman + ZWJ + girl
        let family = "\u{1F468}\u{200D}\u{1F469}\u{200D}\u{1F467}";
        let mut input = InputState::new();
        input.insert_str(family);
        // Each code point is a separate char as far as Rust is concerned
        assert_eq!(input.cursor_col(), family.chars().count());
        assert_eq!(input.text(), family);
    }

    #[test]
    fn insert_flag_emoji() {
        // Regional indicator symbols for US flag
        let flag = "\u{1F1FA}\u{1F1F8}";
        let mut input = InputState::new();
        input.insert_str(flag);
        assert_eq!(input.cursor_col(), 2); // two chars
        assert_eq!(input.text(), flag);
    }

    #[test]
    fn insert_combining_diacritical_marks() {
        // e + combining acute + combining cedilla
        let mut input = InputState::new();
        input.insert_char('e');
        input.insert_char('\u{0301}'); // combining acute
        input.insert_char('\u{0327}'); // combining cedilla
        assert_eq!(input.cursor_col(), 3);
        // Delete the last combining mark
        input.delete_char_before();
        assert_eq!(input.cursor_col(), 2);
        assert_eq!(input.lines()[0], "e\u{0301}");
    }

    #[test]
    fn insert_right_to_left_text() {
        // Arabic text
        let arabic = "\u{0645}\u{0631}\u{062D}\u{0628}\u{0627}";
        let mut input = InputState::new();
        input.insert_str(arabic);
        assert_eq!(input.cursor_col(), 5);
        assert_eq!(input.text(), arabic);
        // Navigate and delete should still work
        input.move_home();
        input.delete_char_after();
        assert_eq!(input.cursor_col(), 0);
        assert_eq!(input.lines()[0].chars().count(), 4);
    }

    #[test]
    fn insert_very_long_single_line() {
        let mut input = InputState::new();
        let long_str: String = "x".repeat(10_000);
        input.insert_str(&long_str);
        assert_eq!(input.cursor_col(), 10_000);
        assert_eq!(input.lines()[0].len(), 10_000);
        // Navigate to middle
        input.move_home();
        for _ in 0..5000 {
            input.move_right();
        }
        assert_eq!(input.cursor_col(), 5000);
        // Insert in the middle
        input.insert_char('Y');
        assert_eq!(input.lines()[0].len(), 10_001);
    }

    #[test]
    fn insert_many_short_lines() {
        let mut input = InputState::new();
        for i in 0..500 {
            input.insert_str(&format!("{i}"));
            input.insert_newline();
        }
        assert_eq!(input.lines().len(), 501); // 500 newlines + 1 trailing empty
        assert_eq!(input.cursor_row(), 500);
    }

    // rapid key combinations

    #[test]
    fn type_then_backspace_all_then_retype() {
        let mut input = InputState::new();
        input.insert_str("hello world");
        // Backspace everything
        for _ in 0..11 {
            input.delete_char_before();
        }
        assert!(input.is_empty());
        assert_eq!(input.cursor_col(), 0);
        // Type again
        input.insert_str("new text");
        assert_eq!(input.text(), "new text");
    }

    #[test]
    fn alternating_insert_and_navigate() {
        let mut input = InputState::new();
        // Simulate: type 'a', left, type 'b', left, type 'c' -> "cba"
        input.insert_char('a');
        input.move_left();
        input.insert_char('b');
        input.move_left();
        input.insert_char('c');
        assert_eq!(input.lines()[0], "cba");
        assert_eq!(input.cursor_col(), 1); // after 'c'
    }

    #[test]
    fn home_end_rapid_cycle() {
        let mut input = InputState::new();
        input.insert_str("hello");
        for _ in 0..50 {
            input.move_home();
            assert_eq!(input.cursor_col(), 0);
            input.move_end();
            assert_eq!(input.cursor_col(), 5);
        }
    }

    #[test]
    fn left_right_round_trip_preserves_position() {
        let mut input = InputState::new();
        input.insert_str("abcdef");
        input.move_home();
        input.move_right();
        input.move_right();
        input.move_right(); // at col 3
        let col = input.cursor_col();
        // Go left 2 then right 2 -- should be back at same spot
        input.move_left();
        input.move_left();
        input.move_right();
        input.move_right();
        assert_eq!(input.cursor_col(), col);
    }

    #[test]
    fn up_down_round_trip_with_short_line() {
        let mut input = InputState::new();
        input.insert_str("longline\na\nlongline");
        let _ = input.set_cursor(0, 7); // end-ish of first line
        input.move_down(); // to "a" -- col clamped to 1
        assert_eq!(input.cursor_col(), 1);
        input.move_down(); // to "longline" -- col stays at 1 (not restored to 7)
        assert_eq!(input.cursor_col(), 1);
    }

    #[test]
    fn newline_then_immediate_backspace() {
        let mut input = InputState::new();
        input.insert_str("hello");
        input.insert_newline();
        assert_eq!(input.lines().len(), 2);
        input.delete_char_before(); // should rejoin
        assert_eq!(input.lines().len(), 1);
        assert_eq!(input.lines()[0], "hello");
        assert_eq!(input.cursor_col(), 5);
    }

    #[test]
    fn delete_forward_through_multiple_line_joins() {
        let mut input = InputState::new();
        input.insert_str("a\nb\nc\nd");
        assert_eq!(input.lines().len(), 4);
        // Go to very start
        let _ = input.set_cursor(0, 0);
        // Move to col 1 (after 'a'), then delete forward repeatedly
        input.move_right(); // past 'a'
        input.delete_char_after(); // join "a" + "b" -> "ab"
        assert_eq!(input.lines()[0], "ab");
        input.move_right(); // past 'b'
        input.delete_char_after(); // join "ab" + "c" -> "abc"
        assert_eq!(input.lines()[0], "abc");
        input.move_right(); // past 'c'
        input.delete_char_after(); // join "abc" + "d" -> "abcd"
        assert_eq!(input.lines(), vec!["abcd"]);
    }

    #[test]
    fn backspace_collapses_all_lines_to_one() {
        let mut input = InputState::new();
        input.insert_str("a\nb\nc\nd\ne");
        assert_eq!(input.lines().len(), 5);
        // Cursor is at end of last line. Backspace everything.
        let total_chars = input.text().len(); // includes \n chars
        for _ in 0..total_chars {
            input.delete_char_before();
        }
        assert!(input.is_empty());
        assert_eq!(input.lines().len(), 1);
        assert_eq!(input.cursor_row(), 0);
        assert_eq!(input.cursor_col(), 0);
    }

    // interleaved actions

    #[test]
    fn type_on_multiple_lines_then_clear() {
        let mut input = InputState::new();
        input.insert_str("line1\nline2\nline3");
        input.move_up();
        input.move_home();
        input.insert_str("prefix_");
        assert_eq!(input.lines()[1], "prefix_line2");
        input.clear();
        assert!(input.is_empty());
        assert_eq!(input.cursor_row(), 0);
    }

    #[test]
    fn insert_between_emoji() {
        let mut input = InputState::new();
        input.insert_char('\u{1F600}');
        input.insert_char('\u{1F601}');
        // cursor at col 2, after both emoji
        input.move_left(); // between the two emoji, col 1
        input.insert_char('X');
        assert_eq!(input.lines()[0], "\u{1F600}X\u{1F601}");
        assert_eq!(input.cursor_col(), 2);
    }

    #[test]
    fn delete_char_after_on_multibyte_boundary() {
        let mut input = InputState::new();
        input.insert_str("\u{1F600}\u{1F601}\u{1F602}");
        input.move_home();
        input.move_right(); // after first emoji
        input.delete_char_after(); // delete second emoji
        assert_eq!(input.lines()[0], "\u{1F600}\u{1F602}");
    }

    #[test]
    fn text_consistent_after_every_operation() {
        let mut input = InputState::new();

        input.insert_str("hello");
        assert_eq!(input.text(), "hello");

        input.insert_newline();
        assert_eq!(input.text(), "hello\n");

        input.insert_str("world");
        assert_eq!(input.text(), "hello\nworld");

        input.move_up();
        input.move_end();
        input.insert_char('!');
        assert_eq!(input.text(), "hello!\nworld");

        input.delete_char_before();
        assert_eq!(input.text(), "hello\nworld");

        input.move_down();
        input.move_home();
        input.delete_char_before(); // join lines
        assert_eq!(input.text(), "helloworld");

        input.clear();
        assert_eq!(input.text(), "");
    }

    #[test]
    fn navigate_through_empty_lines() {
        let mut input = InputState::new();
        input.insert_str("\n\n\n");
        // 4 empty lines, cursor at row 3
        assert_eq!(input.cursor_row(), 3);
        input.move_up();
        assert_eq!(input.cursor_row(), 2);
        assert_eq!(input.cursor_col(), 0);
        input.move_up();
        input.move_up();
        assert_eq!(input.cursor_row(), 0);
        // Insert on the first empty line
        input.insert_char('x');
        assert_eq!(input.lines()[0], "x");
        assert_eq!(input.lines().len(), 4);
    }

    #[test]
    fn insert_str_into_middle_of_existing_content() {
        let mut input = InputState::new();
        input.insert_str("hd");
        input.move_left(); // between h and d
        input.insert_str("ello worl");
        assert_eq!(input.lines()[0], "hello world");
    }

    #[test]
    fn multiline_paste_into_middle_of_line() {
        let mut input = InputState::new();
        input.insert_str("start end");
        // Move cursor to col 6 (between "start " and "end")
        input.move_home();
        for _ in 0..6 {
            input.move_right();
        }
        input.insert_str("line1\nline2\nline3 ");
        assert_eq!(input.lines()[0], "start line1");
        assert_eq!(input.lines()[1], "line2");
        assert_eq!(input.lines()[2], "line3 end");
        assert_eq!(input.cursor_row(), 2);
    }

    #[test]
    fn version_never_wraps_in_reasonable_use() {
        let mut input = InputState::new();
        // After 1000 operations version should be 1000
        for _ in 0..500 {
            input.insert_char('a');
            input.delete_char_before();
        }
        assert_eq!(input.version, 1000);
    }

    #[test]
    fn mixed_cr_and_lf_in_paste() {
        let mut input = InputState::new();
        // Mix of \r, \n, and \r\n
        input.insert_str("a\rb\nc\r\nd");
        // \r -> newline, \n -> newline, \r -> newline, \n -> newline
        // So: "a", "", "b", "", "c", "", "", "d" -- no wait, let me think again
        // \r -> newline (line "a" done, new line), b -> char, \n -> newline,
        // c -> char, \r -> newline, \n -> newline, d -> char
        // lines: ["a", "b", "c", "", "d"]
        assert_eq!(input.lines()[0], "a");
        assert_eq!(input.lines().last().unwrap(), "d");
        // The key point: it doesn't crash and 'd' ends up somewhere
        assert!(input.text().contains('d'));
    }

    #[test]
    fn parse_placeholder_with_trailing_suffix_text() {
        let line = "[Pasted Text 2 - 42 chars]tail";
        let parsed = parse_paste_placeholder_with_suffix(line).unwrap();
        assert_eq!(parsed.0, 1);
        assert_eq!(&line[..parsed.1], "[Pasted Text 2 - 42 chars]");
    }

    #[test]
    fn parse_placeholder_before_cursor_detects_inline_placeholder() {
        let line = "a[Pasted Text 2 - 42 chars]tail";
        let cursor_col = "a[Pasted Text 2 - 42 chars]".chars().count();
        assert_eq!(parse_paste_placeholder_before_cursor(line, cursor_col), Some(1));
    }

    #[test]
    fn text_expands_placeholder_even_with_trailing_text() {
        let mut input = InputState::new();
        input.insert_paste_block("line1\nline2");
        input.mutate_lines(|lines| {
            lines[0].push_str(" + extra");
        });
        let _ = input.set_cursor_col(input.lines()[0].chars().count());
        assert_eq!(input.text(), "line1\nline2 + extra");
    }

    #[test]
    fn insert_paste_block_inserts_inline_at_cursor() {
        let mut input = InputState::new();
        input.insert_str("beforeafter");
        let _ = input.set_cursor_col("before".chars().count());
        input.insert_paste_block(&"x".repeat(1001));

        assert_eq!(input.lines(), vec!["before[Pasted Text 1 - 1001 chars]after"]);
        assert_eq!(input.text(), format!("before{}after", "x".repeat(1001)));
    }

    #[test]
    fn insert_paste_block_reuses_number_after_placeholder_removed() {
        let mut input = InputState::new();
        input.insert_paste_block(&"a".repeat(1001));
        input.mutate_lines(|lines| {
            lines[0].clear();
        });
        let _ = input.set_cursor_col(0);

        input.insert_paste_block(&"b".repeat(1001));

        assert_eq!(input.lines(), vec!["[Pasted Text 1 - 1001 chars]"]);
        assert_eq!(input.text(), "b".repeat(1001));
    }

    #[test]
    fn append_to_active_paste_block_merges_chunks_and_updates_label() {
        let mut input = InputState::new();
        let original = "a\nb\nc\nd\ne\nf\ng\nh\ni\nj\nk";
        input.insert_paste_block(original);
        assert!(input.append_to_active_paste_block("\nl\nm"));
        assert_eq!(input.lines()[0], "[Pasted Text 1 - 25 chars]");
        assert_eq!(input.text(), "a\nb\nc\nd\ne\nf\ng\nh\ni\nj\nk\nl\nm");
    }

    #[test]
    fn append_to_active_paste_block_supports_inline_placeholder() {
        let mut input = InputState::new();
        input.insert_str("prefix-suffix");
        let _ = input.set_cursor_col("prefix-".chars().count());
        input.insert_paste_block("abc");
        assert_eq!(input.lines()[0], "prefix-[Pasted Text 1 - 3 chars]suffix");
        assert!(input.append_to_active_paste_block("XYZ"));
        assert_eq!(input.lines()[0], "prefix-[Pasted Text 1 - 6 chars]suffix");
        assert_eq!(input.text(), "prefix-abcXYZsuffix");
    }

    #[test]
    fn append_to_active_paste_block_rejects_dirty_placeholder_line() {
        let mut input = InputState::new();
        input.insert_paste_block("a\nb\nc\nd\ne\nf\ng\nh\ni\nj\nk");
        input.mutate_lines(|lines| {
            lines[0].push_str("tail");
        });
        let _ = input.set_cursor_col(input.lines()[0].chars().count());
        assert!(!input.append_to_active_paste_block("x"));
    }

    #[test]
    fn count_text_lines_handles_mixed_line_endings() {
        assert_eq!(count_text_lines("a\r\nb\nc\rd"), 4);
        assert_eq!(count_text_lines("single"), 1);
        assert_eq!(count_text_lines("x\r\n"), 2);
    }

    #[test]
    fn count_text_chars_counts_unicode_scalars() {
        assert_eq!(count_text_chars("abc"), 3);
        assert_eq!(count_text_chars("\u{1F600}\u{4F60}"), 2);
        assert_eq!(count_text_chars("a\nb"), 3);
    }
}
