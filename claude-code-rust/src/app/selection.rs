// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::App;
use super::SelectionState;
use unicode_width::UnicodeWidthChar;

pub(crate) fn normalize_selection(
    a: super::SelectionPoint,
    b: super::SelectionPoint,
) -> (super::SelectionPoint, super::SelectionPoint) {
    if (a.row, a.col) <= (b.row, b.col) { (a, b) } else { (b, a) }
}

pub(super) fn clear_selection(app: &mut App) {
    app.selection = None;
    app.rendered_chat_lines.clear();
    app.rendered_input_lines.clear();
}

pub(crate) fn selection_text_from_rendered_lines(
    lines: &[String],
    selection: SelectionState,
) -> String {
    if lines.is_empty() {
        return String::new();
    }

    let (start, end) = normalize_selection(selection.start, selection.end);
    if start.row >= lines.len() {
        return String::new();
    }
    let last_row = end.row.min(lines.len().saturating_sub(1));

    let mut out = String::new();
    for row in start.row..=last_row {
        let line = lines.get(row).map_or("", String::as_str);
        let start_col = if row == start.row { start.col } else { 0 };
        let end_col = if row == end.row { end.col } else { line_display_width(line) };
        out.push_str(&slice_by_display_cols(line, start_col, end_col));
        if row < last_row {
            out.push('\n');
        }
    }
    out
}

pub(crate) fn slice_by_display_cols(text: &str, start_col: usize, end_col: usize) -> String {
    if start_col >= end_col {
        return String::new();
    }

    let mut out = String::new();
    let mut col = 0usize;
    let mut last_visible_included = false;

    for ch in text.chars() {
        let width = display_width(ch);
        if width == 0 {
            if last_visible_included {
                out.push(ch);
            }
            continue;
        }

        let char_start = col;
        let char_end = col.saturating_add(width);
        let include = char_end > start_col && char_start < end_col;
        if include {
            out.push(ch);
        }
        last_visible_included = include;
        col = char_end;
    }

    out
}

fn line_display_width(text: &str) -> usize {
    text.chars().map(display_width).sum()
}

fn display_width(ch: char) -> usize {
    match ch {
        '\t' | '\n' | '\r' => 1,
        _ => UnicodeWidthChar::width(ch).unwrap_or(0),
    }
}

#[cfg(test)]
fn slice_by_cols(text: &str, start_col: usize, end_col: usize) -> String {
    let mut out = String::new();
    for (i, ch) in text.chars().enumerate() {
        if i >= end_col {
            break;
        }
        if i >= start_col {
            out.push(ch);
        }
    }
    out
}

#[cfg(test)]
mod tests {
    // =====
    // TESTS: 42
    // =====

    use super::*;
    use crate::app::SelectionPoint;
    use pretty_assertions::assert_eq;

    // normalize_selection

    #[test]
    fn normalize_already_ordered() {
        let a = SelectionPoint { row: 0, col: 2 };
        let b = SelectionPoint { row: 1, col: 5 };
        let (start, end) = normalize_selection(a, b);
        assert_eq!(start, a);
        assert_eq!(end, b);
    }

    #[test]
    fn normalize_reversed() {
        let a = SelectionPoint { row: 2, col: 3 };
        let b = SelectionPoint { row: 0, col: 1 };
        let (start, end) = normalize_selection(a, b);
        assert_eq!(start, b);
        assert_eq!(end, a);
    }

    #[test]
    fn normalize_same_point() {
        let p = SelectionPoint { row: 5, col: 10 };
        let (start, end) = normalize_selection(p, p);
        assert_eq!(start, p);
        assert_eq!(end, p);
    }

    // normalize_selection

    #[test]
    fn normalize_same_row_different_cols() {
        let a = SelectionPoint { row: 3, col: 10 };
        let b = SelectionPoint { row: 3, col: 2 };
        let (start, end) = normalize_selection(a, b);
        assert_eq!(start.col, 2);
        assert_eq!(end.col, 10);
    }

    #[test]
    fn normalize_origin() {
        let a = SelectionPoint { row: 0, col: 0 };
        let b = SelectionPoint { row: 0, col: 0 };
        let (start, end) = normalize_selection(a, b);
        assert_eq!(start, a);
        assert_eq!(end, b);
    }

    #[test]
    fn normalize_same_row_same_col_nonzero() {
        let p = SelectionPoint { row: 7, col: 7 };
        let (start, end) = normalize_selection(p, p);
        assert_eq!(start, p);
        assert_eq!(end, p);
    }

    // normalize_selection

    #[test]
    fn normalize_large_coordinates() {
        let a = SelectionPoint { row: usize::MAX, col: usize::MAX };
        let b = SelectionPoint { row: 0, col: 0 };
        let (start, end) = normalize_selection(a, b);
        assert_eq!(start.row, 0);
        assert_eq!(end.row, usize::MAX);
    }

    /// Row takes priority in ordering: higher row always comes second
    /// even if its col is 0 and the other col is MAX.
    #[test]
    fn normalize_row_priority_over_col() {
        let a = SelectionPoint { row: 0, col: usize::MAX };
        let b = SelectionPoint { row: 1, col: 0 };
        let (start, end) = normalize_selection(a, b);
        assert_eq!(start, a, "row 0 must come first regardless of col");
        assert_eq!(end, b);
    }

    /// Adjacent rows, both at col 0 - order by row.
    #[test]
    fn normalize_adjacent_rows_col_zero() {
        let a = SelectionPoint { row: 5, col: 0 };
        let b = SelectionPoint { row: 4, col: 0 };
        let (start, end) = normalize_selection(a, b);
        assert_eq!(start.row, 4);
        assert_eq!(end.row, 5);
    }

    /// Symmetry: normalize(a, b) and normalize(b, a) produce the same result.
    #[test]
    fn normalize_symmetry_many_pairs() {
        let pairs = [
            (SelectionPoint { row: 0, col: 0 }, SelectionPoint { row: 0, col: 1 }),
            (SelectionPoint { row: 3, col: 9 }, SelectionPoint { row: 1, col: 100 }),
            (SelectionPoint { row: 100, col: 0 }, SelectionPoint { row: 0, col: 100 }),
            (SelectionPoint { row: 42, col: 42 }, SelectionPoint { row: 42, col: 42 }),
        ];
        for (a, b) in pairs {
            let (s1, e1) = normalize_selection(a, b);
            let (s2, e2) = normalize_selection(b, a);
            assert_eq!((s1, e1), (s2, e2), "normalize must be symmetric for {a:?} / {b:?}");
        }
    }

    /// Idempotence: normalizing an already-normalized pair doesn't change it.
    #[test]
    fn normalize_idempotent() {
        let a = SelectionPoint { row: 10, col: 20 };
        let b = SelectionPoint { row: 3, col: 50 };
        let (s, e) = normalize_selection(a, b);
        let (s2, e2) = normalize_selection(s, e);
        assert_eq!((s, e), (s2, e2));
    }

    // slice_by_cols

    #[test]
    fn slice_ascii_mid() {
        assert_eq!(slice_by_cols("hello world", 2, 7), "llo w");
    }

    #[test]
    fn slice_full_string() {
        assert_eq!(slice_by_cols("abc", 0, 3), "abc");
    }

    #[test]
    fn slice_single_char() {
        assert_eq!(slice_by_cols("hello", 1, 2), "e");
    }

    #[test]
    fn slice_single_char_string_full() {
        assert_eq!(slice_by_cols("x", 0, 1), "x");
    }

    // slice_by_cols

    #[test]
    fn slice_empty_string() {
        assert_eq!(slice_by_cols("", 0, 5), "");
    }

    #[test]
    fn slice_empty_string_zero_range() {
        assert_eq!(slice_by_cols("", 0, 0), "");
    }

    #[test]
    fn slice_start_equals_end() {
        assert_eq!(slice_by_cols("hello", 3, 3), "");
    }

    #[test]
    fn slice_out_of_bounds() {
        assert_eq!(slice_by_cols("hi", 0, 100), "hi");
    }

    #[test]
    fn slice_start_beyond_string() {
        assert_eq!(slice_by_cols("hi", 50, 100), "");
    }

    /// `start_col` > `end_col` -- no guard in the function, loop condition
    /// `i >= end_col` triggers immediately, so result is empty.
    #[test]
    fn slice_start_greater_than_end() {
        assert_eq!(slice_by_cols("hello world", 7, 2), "");
    }

    /// Tab character is 1 char (col), not 4 or 8.
    #[test]
    fn slice_tab_chars() {
        let s = "a\tb\tc";
        // chars: a(0) \t(1) b(2) \t(3) c(4)
        assert_eq!(slice_by_cols(s, 0, 2), "a\t");
        assert_eq!(slice_by_cols(s, 1, 4), "\tb\t");
    }

    /// Newline embedded in a "line" - treated as one char.
    #[test]
    fn slice_with_embedded_newline() {
        let s = "ab\ncd";
        // a(0) b(1) \n(2) c(3) d(4)
        assert_eq!(slice_by_cols(s, 1, 4), "b\nc");
    }

    /// Carriage return embedded in a "line".
    #[test]
    fn slice_with_embedded_cr() {
        let s = "ab\rcd";
        assert_eq!(slice_by_cols(s, 0, 5), "ab\rcd");
    }

    /// Null byte is a valid char.
    #[test]
    fn slice_with_null_byte() {
        let s = "a\0b";
        assert_eq!(slice_by_cols(s, 0, 3), "a\0b");
        assert_eq!(slice_by_cols(s, 1, 2), "\0");
    }

    // slice_by_cols

    #[test]
    fn slice_unicode_emoji() {
        let s = "a\u{1F600}b\u{1F600}c";
        // chars: a(0), emoji(1), b(2), emoji(3), c(4)
        assert_eq!(slice_by_cols(s, 1, 4), "\u{1F600}b\u{1F600}");
    }

    #[test]
    fn slice_cjk_chars() {
        let s = "\u{4F60}\u{597D}\u{4E16}\u{754C}"; // ni hao shi jie
        assert_eq!(slice_by_cols(s, 1, 3), "\u{597D}\u{4E16}");
    }

    #[test]
    fn slice_mixed_unicode_and_ascii() {
        let s = "hi\u{1F600}world";
        // h(0), i(1), emoji(2), w(3), o(4), r(5), l(6), d(7)
        assert_eq!(slice_by_cols(s, 0, 3), "hi\u{1F600}");
        assert_eq!(slice_by_cols(s, 3, 8), "world");
    }

    /// Combining diacritical mark: e + combining acute = 2 chars, 1 glyph.
    /// Slicing between them splits the glyph - this is the char-based reality.
    #[test]
    fn slice_combining_diacritical_splits_glyph() {
        let s = "e\u{0301}x"; // e + combining acute + x
        // chars: e(0), \u{0301}(1), x(2)
        assert_eq!(slice_by_cols(s, 0, 1), "e"); // bare e, no accent
        assert_eq!(slice_by_cols(s, 1, 2), "\u{0301}"); // orphan combining mark
        assert_eq!(slice_by_cols(s, 0, 2), "e\u{0301}"); // full glyph
        assert_eq!(slice_by_cols(s, 0, 3), "e\u{0301}x"); // everything
    }

    /// Multiple combining marks stacked on one base: a + ring + macron.
    #[test]
    fn slice_stacked_combining_marks() {
        let s = "a\u{030A}\u{0304}z"; // a + combining ring above + combining macron + z
        // chars: a(0) ring(1) macron(2) z(3)
        assert_eq!(slice_by_cols(s, 0, 3), "a\u{030A}\u{0304}");
        assert_eq!(slice_by_cols(s, 1, 3), "\u{030A}\u{0304}"); // orphaned marks
        assert_eq!(slice_by_cols(s, 3, 4), "z");
    }

    /// ZWJ sequence (family emoji): multiple codepoints, one visual glyph.
    /// char-based slicing will split it.
    #[test]
    fn slice_zwj_family_emoji() {
        // man + ZWJ + woman + ZWJ + girl = 5 chars, 1 visible emoji
        let s = "\u{1F468}\u{200D}\u{1F469}\u{200D}\u{1F467}";
        assert_eq!(s.chars().count(), 5);
        // Slicing first 2 chars gives man + ZWJ (broken glyph)
        assert_eq!(slice_by_cols(s, 0, 2), "\u{1F468}\u{200D}");
        // Full sequence
        assert_eq!(slice_by_cols(s, 0, 5), s);
    }

    /// Flag emoji: two regional indicator chars = 1 flag.
    #[test]
    fn slice_flag_emoji_splits() {
        let flag = "\u{1F1FA}\u{1F1F8}"; // US flag = 2 chars
        assert_eq!(flag.chars().count(), 2);
        assert_eq!(slice_by_cols(flag, 0, 1), "\u{1F1FA}"); // half a flag
        assert_eq!(slice_by_cols(flag, 0, 2), flag); // whole flag
        assert_eq!(slice_by_cols(flag, 1, 2), "\u{1F1F8}"); // other half
    }

    /// Arabic RTL text - chars are chars regardless of display direction.
    #[test]
    fn slice_arabic_rtl() {
        let s = "\u{0645}\u{0631}\u{062D}\u{0628}\u{0627}"; // mrhba
        assert_eq!(s.chars().count(), 5);
        assert_eq!(slice_by_cols(s, 1, 4), "\u{0631}\u{062D}\u{0628}");
    }

    /// All-emoji string.
    #[test]
    fn slice_all_emoji() {
        let s = "\u{1F600}\u{1F601}\u{1F602}\u{1F603}\u{1F604}";
        assert_eq!(slice_by_cols(s, 2, 4), "\u{1F602}\u{1F603}");
        assert_eq!(slice_by_cols(s, 0, 5), s);
    }

    /// Stress test: 10K character string, slice a window in the middle.
    #[test]
    fn slice_stress_10k_chars() {
        let s: String = (0..10_000).map(|i| if i % 2 == 0 { 'a' } else { 'b' }).collect();
        let sliced = slice_by_cols(&s, 4000, 4010);
        assert_eq!(sliced.len(), 10);
        assert_eq!(sliced, "ababababab");
    }

    /// Stress test: 10K emoji string.
    #[test]
    fn slice_stress_10k_emoji() {
        let s: String = "\u{1F600}".repeat(10_000);
        let sliced = slice_by_cols(&s, 9990, 10_000);
        assert_eq!(sliced.chars().count(), 10);
    }

    /// Slice exactly the last character.
    #[test]
    fn slice_last_char_only() {
        assert_eq!(slice_by_cols("abcdef", 5, 6), "f");
    }

    /// Slice exactly the first character.
    #[test]
    fn slice_first_char_only() {
        assert_eq!(slice_by_cols("abcdef", 0, 1), "a");
    }

    /// Variation selector: base + VS16 = 2 chars.
    #[test]
    fn slice_variation_selector() {
        let s = "\u{2764}\u{FE0F}x"; // red heart emoji (heart + VS16) + x
        assert_eq!(s.chars().count(), 3);
        assert_eq!(slice_by_cols(s, 0, 2), "\u{2764}\u{FE0F}");
        assert_eq!(slice_by_cols(s, 2, 3), "x");
    }

    /// Mixed script: Latin, CJK, emoji, Arabic all in one string.
    #[test]
    fn slice_mixed_scripts() {
        let s = "Hi\u{4F60}\u{1F600}\u{0645}!";
        // H(0) i(1) ni(2) emoji(3) meem(4) !(5)
        assert_eq!(s.chars().count(), 6);
        assert_eq!(slice_by_cols(s, 0, 6), s);
        assert_eq!(slice_by_cols(s, 2, 5), "\u{4F60}\u{1F600}\u{0645}");
    }

    /// Consecutive zero-range slices always return empty.
    #[test]
    fn slice_zero_width_at_every_position() {
        let s = "hello";
        for i in 0..=5 {
            assert_eq!(slice_by_cols(s, i, i), "", "zero-width at col {i}");
        }
    }

    /// Sliding window: every 3-char window of "abcde".
    #[test]
    fn slice_sliding_window() {
        let s = "abcde";
        let windows: Vec<String> = (0..3).map(|i| slice_by_cols(s, i, i + 3)).collect();
        assert_eq!(windows, vec!["abc", "bcd", "cde"]);
    }

    #[test]
    fn slice_by_display_cols_handles_wide_unicode_cells() {
        let s = "a😀b";
        assert_eq!(slice_by_display_cols(s, 0, 1), "a");
        assert_eq!(slice_by_display_cols(s, 1, 3), "😀");
        assert_eq!(slice_by_display_cols(s, 2, 4), "😀b");
    }

    #[test]
    fn slice_by_display_cols_preserves_combining_suffix_for_selected_base() {
        let s = "e\u{0301}x";
        assert_eq!(slice_by_display_cols(s, 0, 1), "e\u{0301}");
        assert_eq!(slice_by_display_cols(s, 1, 2), "x");
    }

    #[test]
    fn selection_text_uses_display_columns_for_chat_lines() {
        let lines = vec!["a😀b".to_owned()];
        let selection = SelectionState {
            kind: crate::app::SelectionKind::Chat,
            start: SelectionPoint { row: 0, col: 1 },
            end: SelectionPoint { row: 0, col: 3 },
            dragging: false,
        };

        assert_eq!(selection_text_from_rendered_lines(&lines, selection), "😀");
    }
}
