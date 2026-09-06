// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::inline_interactions::{
    focus_next_inline_interaction, focused_interaction, focused_interaction_dirty_idx,
    get_focused_interaction_tc, invalidate_if_changed, pop_next_valid_interaction_id,
};
use super::{App, InvalidationLevel, MessageBlock};
use crate::agent::model;
use crossterm::event::{KeyCode, KeyEvent, KeyModifiers};

fn focused_question(app: &App) -> Option<&crate::app::InlineQuestion> {
    focused_interaction(app)?.pending_question.as_ref()
}

pub(super) fn has_focused_question(app: &App) -> bool {
    focused_question(app).is_some()
}

fn focused_question_is_editing_notes(app: &App) -> bool {
    focused_question(app).is_some_and(|question| question.editing_notes)
}

fn focused_question_option_count(app: &App) -> usize {
    focused_question(app).map_or(0, |question| question.prompt.options.len())
}

fn is_printable_question_note_modifiers(modifiers: KeyModifiers) -> bool {
    let ctrl_alt =
        modifiers.contains(KeyModifiers::CONTROL) && modifiers.contains(KeyModifiers::ALT);
    !modifiers.intersects(KeyModifiers::CONTROL | KeyModifiers::ALT) || ctrl_alt
}

fn move_question_option_left(app: &mut App) {
    let dirty_idx = focused_interaction_dirty_idx(app);
    let mut changed = false;
    if let Some(tc) = get_focused_interaction_tc(app)
        && let Some(ref mut question) = tc.pending_question
    {
        let next = question.focused_option_index.saturating_sub(1);
        if next != question.focused_option_index {
            question.focused_option_index = next;
            tc.mark_tool_call_layout_dirty();
            changed = true;
        }
    }
    invalidate_if_changed(app, dirty_idx, changed);
}

fn move_question_option_right(app: &mut App) {
    let dirty_idx = focused_interaction_dirty_idx(app);
    let mut changed = false;
    if let Some(tc) = get_focused_interaction_tc(app)
        && let Some(ref mut question) = tc.pending_question
        && question.focused_option_index + 1 < question.prompt.options.len()
    {
        question.focused_option_index += 1;
        tc.mark_tool_call_layout_dirty();
        changed = true;
    }
    invalidate_if_changed(app, dirty_idx, changed);
}

fn move_question_option_to_start(app: &mut App) {
    let dirty_idx = focused_interaction_dirty_idx(app);
    let mut changed = false;
    if let Some(tc) = get_focused_interaction_tc(app)
        && let Some(ref mut question) = tc.pending_question
        && question.focused_option_index != 0
    {
        question.focused_option_index = 0;
        tc.mark_tool_call_layout_dirty();
        changed = true;
    }
    invalidate_if_changed(app, dirty_idx, changed);
}

fn move_question_option_to_end(app: &mut App) {
    let dirty_idx = focused_interaction_dirty_idx(app);
    let mut changed = false;
    if let Some(tc) = get_focused_interaction_tc(app)
        && let Some(ref mut question) = tc.pending_question
        && let Some(last_idx) = question.prompt.options.len().checked_sub(1)
        && question.focused_option_index != last_idx
    {
        question.focused_option_index = last_idx;
        tc.mark_tool_call_layout_dirty();
        changed = true;
    }
    invalidate_if_changed(app, dirty_idx, changed);
}

fn question_notes_byte_index(notes: &str, cursor: usize) -> usize {
    notes.char_indices().nth(cursor).map_or(notes.len(), |(idx, _)| idx)
}

fn insert_question_note_char(app: &mut App, ch: char) {
    let dirty_idx = focused_interaction_dirty_idx(app);
    let mut changed = false;
    if let Some(tc) = get_focused_interaction_tc(app)
        && let Some(ref mut question) = tc.pending_question
    {
        let idx = question_notes_byte_index(&question.notes, question.notes_cursor);
        question.notes.insert(idx, ch);
        question.notes_cursor += 1;
        tc.mark_tool_call_layout_dirty();
        changed = true;
    }
    invalidate_if_changed(app, dirty_idx, changed);
}

fn delete_question_note_char_before(app: &mut App) {
    let dirty_idx = focused_interaction_dirty_idx(app);
    let mut changed = false;
    if let Some(tc) = get_focused_interaction_tc(app)
        && let Some(ref mut question) = tc.pending_question
        && question.notes_cursor > 0
    {
        let start = question_notes_byte_index(&question.notes, question.notes_cursor - 1);
        let end = question_notes_byte_index(&question.notes, question.notes_cursor);
        question.notes.replace_range(start..end, "");
        question.notes_cursor -= 1;
        tc.mark_tool_call_layout_dirty();
        changed = true;
    }
    invalidate_if_changed(app, dirty_idx, changed);
}

fn delete_question_note_char_after(app: &mut App) {
    let dirty_idx = focused_interaction_dirty_idx(app);
    let mut changed = false;
    if let Some(tc) = get_focused_interaction_tc(app)
        && let Some(ref mut question) = tc.pending_question
        && question.notes_cursor < question.notes.chars().count()
    {
        let start = question_notes_byte_index(&question.notes, question.notes_cursor);
        let end = question_notes_byte_index(&question.notes, question.notes_cursor + 1);
        question.notes.replace_range(start..end, "");
        tc.mark_tool_call_layout_dirty();
        changed = true;
    }
    invalidate_if_changed(app, dirty_idx, changed);
}

fn move_question_notes_cursor(app: &mut App, direction: i32) {
    let dirty_idx = focused_interaction_dirty_idx(app);
    let mut changed = false;
    if let Some(tc) = get_focused_interaction_tc(app)
        && let Some(ref mut question) = tc.pending_question
    {
        let max = question.notes.chars().count();
        let next = if direction < 0 {
            question.notes_cursor.saturating_sub(1)
        } else {
            (question.notes_cursor + 1).min(max)
        };
        if next != question.notes_cursor {
            question.notes_cursor = next;
            tc.mark_tool_call_layout_dirty();
            changed = true;
        }
    }
    invalidate_if_changed(app, dirty_idx, changed);
}

fn move_question_notes_cursor_to_start(app: &mut App) {
    let dirty_idx = focused_interaction_dirty_idx(app);
    let mut changed = false;
    if let Some(tc) = get_focused_interaction_tc(app)
        && let Some(ref mut question) = tc.pending_question
        && question.notes_cursor != 0
    {
        question.notes_cursor = 0;
        tc.mark_tool_call_layout_dirty();
        changed = true;
    }
    invalidate_if_changed(app, dirty_idx, changed);
}

fn move_question_notes_cursor_to_end(app: &mut App) {
    let dirty_idx = focused_interaction_dirty_idx(app);
    let mut changed = false;
    if let Some(tc) = get_focused_interaction_tc(app)
        && let Some(ref mut question) = tc.pending_question
    {
        let next = question.notes.chars().count();
        if question.notes_cursor != next {
            question.notes_cursor = next;
            tc.mark_tool_call_layout_dirty();
            changed = true;
        }
    }
    invalidate_if_changed(app, dirty_idx, changed);
}

fn set_question_notes_editing(app: &mut App, editing_notes: bool) {
    let dirty_idx = focused_interaction_dirty_idx(app);
    let mut changed = false;
    if let Some(tc) = get_focused_interaction_tc(app)
        && let Some(ref mut question) = tc.pending_question
        && question.editing_notes != editing_notes
    {
        question.editing_notes = editing_notes;
        tc.mark_tool_call_layout_dirty();
        changed = true;
    }
    invalidate_if_changed(app, dirty_idx, changed);
}

fn toggle_question_selection(app: &mut App) {
    let dirty_idx = focused_interaction_dirty_idx(app);
    let mut changed = false;
    if let Some(tc) = get_focused_interaction_tc(app)
        && let Some(ref mut question) = tc.pending_question
    {
        let idx = question.focused_option_index;
        if question.prompt.multi_select {
            if !question.selected_option_indices.insert(idx) {
                question.selected_option_indices.remove(&idx);
            }
        } else {
            question.selected_option_indices.clear();
            question.selected_option_indices.insert(idx);
        }
        tc.mark_tool_call_layout_dirty();
        changed = true;
    }
    invalidate_if_changed(app, dirty_idx, changed);
}

fn question_selected_indices(question: &crate::app::InlineQuestion) -> Vec<usize> {
    if question.prompt.multi_select {
        if question.selected_option_indices.is_empty() {
            return vec![question.focused_option_index];
        }
        return question.selected_option_indices.iter().copied().collect();
    }
    vec![question.focused_option_index]
}

fn question_annotation(
    question: &crate::app::InlineQuestion,
    selected_indices: &[usize],
) -> Option<model::QuestionAnnotation> {
    let preview = selected_indices
        .iter()
        .filter_map(|idx| question.prompt.options.get(*idx))
        .filter_map(|option| option.preview.as_deref())
        .map(str::trim)
        .filter(|preview| !preview.is_empty())
        .collect::<Vec<_>>()
        .join("\n\n");
    let notes = question.notes.trim();
    if preview.is_empty() && notes.is_empty() {
        return None;
    }

    Some(
        model::QuestionAnnotation::new()
            .preview((!preview.is_empty()).then_some(preview))
            .notes((!notes.is_empty()).then_some(notes.to_owned())),
    )
}

fn respond_question(app: &mut App) {
    let Some(tool_id) = pop_next_valid_interaction_id(app) else {
        return;
    };

    let Some((mi, bi)) = app.tool_call_index.get(&tool_id).copied() else {
        return;
    };
    let Some(MessageBlock::ToolCall(tc)) =
        app.messages.get_mut(mi).and_then(|m| m.blocks.get_mut(bi))
    else {
        return;
    };
    let tc = tc.as_mut();
    let mut invalidated = false;
    if let Some(pending) = tc.pending_question.take() {
        let selected_indices = question_selected_indices(&pending);
        let selected_option_ids = selected_indices
            .iter()
            .filter_map(|idx| pending.prompt.options.get(*idx))
            .map(|option| option.option_id.clone())
            .collect::<Vec<_>>();
        let annotation = question_annotation(&pending, &selected_indices);

        if selected_option_ids.is_empty() {
            tracing::warn!("question selection had no valid option ids: tool_call_id={tool_id}");
            let _ = pending.response_tx.send(model::RequestQuestionResponse::new(
                model::RequestQuestionOutcome::Cancelled,
            ));
        } else {
            tracing::debug!(
                "question selection: tool_call_id={} selected_option_ids={:?}",
                tool_id,
                selected_option_ids
            );
            let _ = pending.response_tx.send(model::RequestQuestionResponse::new(
                model::RequestQuestionOutcome::Answered(
                    model::AnsweredQuestionOutcome::new(selected_option_ids).annotation(annotation),
                ),
            ));
        }
        tc.mark_tool_call_layout_dirty();
        invalidated = true;
    }
    if invalidated {
        app.sync_render_cache_slot(mi, bi);
        app.recompute_message_retained_bytes(mi);
        app.invalidate_layout(InvalidationLevel::MessageChanged(mi));
    }

    focus_next_inline_interaction(app);
}

fn respond_question_cancel(app: &mut App) {
    let Some(tool_id) = pop_next_valid_interaction_id(app) else {
        return;
    };

    let Some((mi, bi)) = app.tool_call_index.get(&tool_id).copied() else {
        return;
    };
    let Some(MessageBlock::ToolCall(tc)) =
        app.messages.get_mut(mi).and_then(|m| m.blocks.get_mut(bi))
    else {
        return;
    };
    let tc = tc.as_mut();
    if let Some(pending) = tc.pending_question.take() {
        let _ = pending
            .response_tx
            .send(model::RequestQuestionResponse::new(model::RequestQuestionOutcome::Cancelled));
        tc.mark_tool_call_layout_dirty();
        app.sync_render_cache_slot(mi, bi);
        app.recompute_message_retained_bytes(mi);
        app.invalidate_layout(InvalidationLevel::MessageChanged(mi));
    }

    focus_next_inline_interaction(app);
}

pub(super) fn handle_question_key(
    app: &mut App,
    key: KeyEvent,
    interaction_has_focus: bool,
) -> Option<bool> {
    if !has_focused_question(app) || !interaction_has_focus {
        return None;
    }
    let option_count = focused_question_option_count(app);

    if focused_question_is_editing_notes(app) {
        return match key.code {
            KeyCode::Left => {
                move_question_notes_cursor(app, -1);
                Some(true)
            }
            KeyCode::Right => {
                move_question_notes_cursor(app, 1);
                Some(true)
            }
            KeyCode::Home => {
                move_question_notes_cursor_to_start(app);
                Some(true)
            }
            KeyCode::End => {
                move_question_notes_cursor_to_end(app);
                Some(true)
            }
            KeyCode::Backspace => {
                delete_question_note_char_before(app);
                Some(true)
            }
            KeyCode::Delete => {
                delete_question_note_char_after(app);
                Some(true)
            }
            KeyCode::Tab | KeyCode::BackTab => {
                set_question_notes_editing(app, false);
                Some(true)
            }
            KeyCode::Enter => {
                respond_question(app);
                Some(true)
            }
            KeyCode::Esc => {
                respond_question_cancel(app);
                Some(true)
            }
            KeyCode::Up | KeyCode::Down => Some(true),
            KeyCode::Char(ch) if is_printable_question_note_modifiers(key.modifiers) => {
                insert_question_note_char(app, ch);
                Some(true)
            }
            _ => None,
        };
    }

    match key.code {
        KeyCode::Left | KeyCode::Up if option_count > 0 => {
            move_question_option_left(app);
            Some(true)
        }
        KeyCode::Right | KeyCode::Down if option_count > 0 => {
            move_question_option_right(app);
            Some(true)
        }
        KeyCode::Home if option_count > 0 => {
            move_question_option_to_start(app);
            Some(true)
        }
        KeyCode::End if option_count > 0 => {
            move_question_option_to_end(app);
            Some(true)
        }
        KeyCode::Char(' ') if option_count > 0 => {
            toggle_question_selection(app);
            Some(true)
        }
        KeyCode::Tab | KeyCode::BackTab => {
            set_question_notes_editing(app, true);
            Some(true)
        }
        KeyCode::Enter if option_count > 0 => {
            respond_question(app);
            Some(true)
        }
        KeyCode::Esc => {
            respond_question_cancel(app);
            Some(true)
        }
        KeyCode::Backspace | KeyCode::Delete => Some(true),
        KeyCode::Char(_) if is_printable_question_note_modifiers(key.modifiers) => Some(true),
        _ => None,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::app::{
        App, BlockCache, ChatMessage, InlineQuestion, MessageBlock, MessageRole, ToolCallInfo,
    };
    use pretty_assertions::assert_eq;
    use std::collections::BTreeSet;
    use tokio::sync::oneshot;

    fn test_tool_call(id: &str) -> ToolCallInfo {
        ToolCallInfo {
            id: id.to_owned(),
            title: format!("Tool {id}"),
            sdk_tool_name: "AskUserQuestion".to_owned(),
            raw_input: None,
            raw_input_bytes: 0,
            output_metadata: None,
            status: model::ToolCallStatus::InProgress,
            content: Vec::new(),
            hidden: false,
            terminal_id: None,
            terminal_command: None,
            terminal_output: None,
            terminal_output_len: 0,
            terminal_bytes_seen: 0,
            terminal_snapshot_mode: crate::app::TerminalSnapshotMode::AppendOnly,
            render_epoch: 0,
            layout_epoch: 0,
            last_measured_width: 0,
            last_measured_height: 0,
            last_measured_layout_epoch: 0,
            last_measured_layout_generation: 0,
            cache: BlockCache::default(),
            pending_permission: None,
            pending_question: None,
        }
    }

    fn assistant_tool_msg(tc: ToolCallInfo) -> ChatMessage {
        ChatMessage {
            role: MessageRole::Assistant,
            blocks: vec![MessageBlock::ToolCall(Box::new(tc))],
            usage: None,
        }
    }

    fn add_question(
        app: &mut App,
        tool_id: &str,
        prompt: model::QuestionPrompt,
        focused: bool,
    ) -> oneshot::Receiver<model::RequestQuestionResponse> {
        let msg_idx = app.messages.len();
        app.messages.push(assistant_tool_msg(test_tool_call(tool_id)));
        app.index_tool_call(tool_id.to_owned(), msg_idx, 0);

        let (tx, rx) = oneshot::channel();
        if let Some(MessageBlock::ToolCall(tc)) =
            app.messages.get_mut(msg_idx).and_then(|m| m.blocks.get_mut(0))
        {
            tc.pending_question = Some(InlineQuestion {
                prompt,
                response_tx: tx,
                focused_option_index: 0,
                selected_option_indices: BTreeSet::new(),
                notes: String::new(),
                notes_cursor: 0,
                editing_notes: false,
                focused,
                question_index: 0,
                total_questions: 1,
            });
        }
        app.pending_interaction_ids.push(tool_id.to_owned());
        rx
    }

    #[test]
    fn question_prompt_enter_answers_focused_option_with_preview_annotation() {
        let mut app = App::test_default();
        let mut rx = add_question(
            &mut app,
            "question-1",
            model::QuestionPrompt::new(
                "Choose a target",
                "Target",
                false,
                vec![
                    model::QuestionOption::new("question_0", "Staging")
                        .preview(Some("Deploy to staging first.".to_owned())),
                    model::QuestionOption::new("question_1", "Production")
                        .preview(Some("Deploy to production after approval.".to_owned())),
                ],
            ),
            true,
        );

        let consumed_right =
            handle_question_key(&mut app, KeyEvent::new(KeyCode::Right, KeyModifiers::NONE), true);
        let consumed_enter =
            handle_question_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE), true);

        assert_eq!(consumed_right, Some(true));
        assert_eq!(consumed_enter, Some(true));
        assert!(app.pending_interaction_ids.is_empty());

        let resp = rx.try_recv().expect("question should be answered");
        let model::RequestQuestionOutcome::Answered(answered) = resp.outcome else {
            panic!("expected answered question response");
        };
        assert_eq!(answered.selected_option_ids, vec!["question_1"]);
        assert_eq!(
            answered.annotation.and_then(|annotation| annotation.preview),
            Some("Deploy to production after approval.".to_owned())
        );
    }

    #[test]
    fn multi_select_question_collects_toggles_and_notes() {
        let mut app = App::test_default();
        let mut rx = add_question(
            &mut app,
            "question-2",
            model::QuestionPrompt::new(
                "Pick environments",
                "Environments",
                true,
                vec![
                    model::QuestionOption::new("question_0", "Staging")
                        .preview(Some("Deploy to staging first.".to_owned())),
                    model::QuestionOption::new("question_1", "Production")
                        .preview(Some("Deploy to production after approval.".to_owned())),
                ],
            ),
            true,
        );

        assert_eq!(
            handle_question_key(
                &mut app,
                KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE),
                true
            ),
            Some(true)
        );
        assert_eq!(
            handle_question_key(&mut app, KeyEvent::new(KeyCode::Right, KeyModifiers::NONE), true),
            Some(true)
        );
        assert_eq!(
            handle_question_key(
                &mut app,
                KeyEvent::new(KeyCode::Char(' '), KeyModifiers::NONE),
                true
            ),
            Some(true)
        );
        assert_eq!(
            handle_question_key(&mut app, KeyEvent::new(KeyCode::Tab, KeyModifiers::NONE), true),
            Some(true)
        );
        for ch in ['n', 'o', 't', 'e'] {
            assert_eq!(
                handle_question_key(
                    &mut app,
                    KeyEvent::new(KeyCode::Char(ch), KeyModifiers::NONE),
                    true
                ),
                Some(true)
            );
        }
        assert_eq!(
            handle_question_key(&mut app, KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE), true),
            Some(true)
        );

        let resp = rx.try_recv().expect("question should be answered");
        let model::RequestQuestionOutcome::Answered(answered) = resp.outcome else {
            panic!("expected answered question response");
        };
        assert_eq!(answered.selected_option_ids, vec!["question_0", "question_1"]);
        assert_eq!(
            answered.annotation,
            Some(
                model::QuestionAnnotation::new()
                    .preview(Some(
                        "Deploy to staging first.\n\nDeploy to production after approval."
                            .to_owned(),
                    ))
                    .notes(Some("note".to_owned())),
            )
        );
    }
}
