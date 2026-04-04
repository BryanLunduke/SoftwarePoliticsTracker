// =====
// TESTS: 14
// =====
//
// Caching pipeline integration tests.
// Validates the full pipeline: streaming -> block splitting -> cache storage ->
// budget enforcement -> viewport invalidation -> height measurement -> prefix sums.
//
// Covers Phase 5 test groups from notes/caching.md:
//   Group 2: Streaming + budget behavior
//   Group 3: History retention + estimator validation
//   Group 5: Regression / full pipeline

use claude_code_rust::agent::events::ClientEvent;
use claude_code_rust::agent::model;
use claude_code_rust::app::{
    App, AppStatus, BlockCache, ChatMessage, DEFAULT_CACHE_SPLIT_HARD_LIMIT_BYTES,
    DEFAULT_CACHE_SPLIT_SOFT_LIMIT_BYTES, MessageBlock, MessageRole, TextBlock, TextBlockSpacing,
};
use claude_code_rust::ui::{SpinnerState, measure_message_height_cached};
use ratatui::text::{Line, Span};
use std::fmt::Write as _;

use crate::helpers::{send_client_event, test_app};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

fn inactive_spinner() -> SpinnerState {
    SpinnerState {
        frame: 0,
        is_active_turn_assistant: false,
        show_empty_thinking: false,
        show_thinking: false,
        show_subagent_thinking: false,
        show_compacting: false,
    }
}

fn stream_text(app: &mut App, text: &str) {
    let chunk = model::ContentChunk::new(model::ContentBlock::Text(model::TextContent::new(text)));
    send_client_event(
        app,
        ClientEvent::SessionUpdate(model::SessionUpdate::AgentMessageChunk(chunk)),
    );
}

fn complete_turn(app: &mut App) {
    send_client_event(app, ClientEvent::TurnComplete);
}

/// Build a `ChatMessage` with a single text block for direct insertion.
fn user_text_message(text: &str) -> ChatMessage {
    ChatMessage {
        role: MessageRole::User,
        blocks: vec![MessageBlock::Text(TextBlock::from_complete(text))],
        usage: None,
    }
}

/// Build an assistant message with a text block and pre-stored cache lines.
/// Returns the message with `cached_bytes > 0`.
fn assistant_message_with_cache(text: &str) -> ChatMessage {
    let lines: Vec<Line<'static>> =
        text.lines().map(|l| Line::from(Span::raw(l.to_owned()))).collect();
    let mut cache = BlockCache::default();
    cache.store(lines);
    ChatMessage {
        role: MessageRole::Assistant,
        blocks: vec![MessageBlock::Text(TextBlock {
            text: text.to_owned(),
            cache,
            markdown: claude_code_rust::app::IncrementalMarkdown::from_complete(text),
            trailing_spacing: TextBlockSpacing::None,
        })],
        usage: None,
    }
}

/// Extract the text content of all text blocks in a message.
fn collect_block_texts(msg: &ChatMessage) -> Vec<&str> {
    msg.blocks
        .iter()
        .filter_map(|b| match b {
            MessageBlock::Text(block) => Some(block.text.as_str()),
            _ => None,
        })
        .collect()
}

// ===========================================================================
// Group 2: Streaming + Budget
// ===========================================================================

#[tokio::test]
async fn streaming_creates_single_block_under_soft_limit() {
    let mut app = test_app();
    stream_text(&mut app, "Hello world.");
    complete_turn(&mut app);

    assert_eq!(app.messages.len(), 1);
    let texts = collect_block_texts(&app.messages[0]);
    assert_eq!(texts.len(), 1);
    assert_eq!(texts[0], "Hello world.");
}

#[tokio::test]
async fn streaming_accumulates_multiple_chunks() {
    let mut app = test_app();
    stream_text(&mut app, "Part one. ");
    stream_text(&mut app, "Part two.");
    complete_turn(&mut app);

    assert_eq!(app.messages.len(), 1);
    let texts = collect_block_texts(&app.messages[0]);
    assert_eq!(texts.len(), 1);
    assert_eq!(texts[0], "Part one. Part two.");
}

#[tokio::test]
async fn streaming_splits_at_soft_limit() {
    let mut app = test_app();

    // Build text that exceeds soft limit (1536 bytes) with paragraph breaks.
    // "line N\n" is 7+ bytes per iteration; 250 iterations = ~1750 bytes.
    let mut text = String::new();
    for i in 0..250 {
        writeln!(&mut text, "line {i}").expect("writing to String should never fail");
    }
    assert!(
        text.len() > DEFAULT_CACHE_SPLIT_SOFT_LIMIT_BYTES,
        "test text {} bytes should exceed soft limit {}",
        text.len(),
        DEFAULT_CACHE_SPLIT_SOFT_LIMIT_BYTES,
    );

    stream_text(&mut app, &text);
    complete_turn(&mut app);

    assert_eq!(app.messages.len(), 1);
    let block_count =
        app.messages[0].blocks.iter().filter(|b| matches!(b, MessageBlock::Text(..))).count();
    assert!(block_count >= 2, "expected split, got {block_count} text blocks");

    // First block should not exceed the hard limit.
    if let MessageBlock::Text(first_block) = &app.messages[0].blocks[0] {
        assert!(
            first_block.text.len() <= DEFAULT_CACHE_SPLIT_HARD_LIMIT_BYTES,
            "first block {} bytes exceeds hard limit {}",
            first_block.text.len(),
            DEFAULT_CACHE_SPLIT_HARD_LIMIT_BYTES,
        );
        assert_eq!(first_block.trailing_spacing, TextBlockSpacing::None);
    }
}

#[tokio::test]
async fn streaming_splits_at_hard_limit() {
    let mut app = test_app();

    // Build text that exceeds hard limit (4096 bytes) with sentence boundaries
    // but NO newlines — so the soft-limit paragraph split cannot fire.
    // Sentence boundaries (". ") give the hard-limit fallback something to pick.
    let mut text = String::new();
    for i in 0..250 {
        write!(&mut text, "Sentence number {i}. ").expect("writing to String should never fail");
    }
    assert!(
        text.len() > DEFAULT_CACHE_SPLIT_HARD_LIMIT_BYTES,
        "test text {} bytes should exceed hard limit {}",
        text.len(),
        DEFAULT_CACHE_SPLIT_HARD_LIMIT_BYTES,
    );
    assert!(!text.contains('\n'), "text must have no newlines for hard-limit test");

    stream_text(&mut app, &text);
    complete_turn(&mut app);

    assert_eq!(app.messages.len(), 1);
    let block_count =
        app.messages[0].blocks.iter().filter(|b| matches!(b, MessageBlock::Text(..))).count();
    assert!(block_count >= 2, "expected hard split, got {block_count} text blocks");

    if let MessageBlock::Text(first_block) = &app.messages[0].blocks[0] {
        assert!(
            first_block.text.len() <= DEFAULT_CACHE_SPLIT_HARD_LIMIT_BYTES,
            "first block {} bytes exceeds hard limit {}",
            first_block.text.len(),
            DEFAULT_CACHE_SPLIT_HARD_LIMIT_BYTES,
        );
        assert_eq!(first_block.trailing_spacing, TextBlockSpacing::None);
    }
}

#[tokio::test]
async fn budget_enforcement_no_eviction_under_budget() {
    let mut app = test_app();

    // Create a message with a small cache.
    app.messages.push(assistant_message_with_cache("short line"));
    complete_turn(&mut app);

    let stats = app.enforce_render_cache_budget();
    assert_eq!(stats.evicted_blocks, 0);
    assert_eq!(stats.evicted_bytes, 0);
}

#[tokio::test]
async fn budget_enforcement_evicts_lru_when_over_budget() {
    let mut app = test_app();
    app.render_cache_budget.max_bytes = 100; // tiny budget

    // Insert 3 messages with caches that each exceed 50 bytes.
    // Stored in order: msg 0 (oldest tick), msg 1, msg 2 (newest tick).
    let big_text = "x".repeat(80);
    app.messages.push(assistant_message_with_cache(&big_text));
    app.messages.push(assistant_message_with_cache(&big_text));
    app.messages.push(assistant_message_with_cache(&big_text));

    let stats = app.enforce_render_cache_budget();
    assert!(stats.evicted_blocks > 0, "expected evictions, got 0");
    assert!(
        stats.total_after_bytes <= 100 + stats.protected_bytes,
        "total_after_bytes {} should be <= budget {} + protected {}",
        stats.total_after_bytes,
        100,
        stats.protected_bytes,
    );
}

#[tokio::test]
async fn budget_enforcement_protects_streaming_tail() {
    let mut app = test_app();
    app.render_cache_budget.max_bytes = 100;

    let big_text = "x".repeat(80);
    app.messages.push(assistant_message_with_cache(&big_text));
    app.messages.push(assistant_message_with_cache(&big_text));

    // Set streaming state -- last message is protected.
    app.status = AppStatus::Running;

    let stats = app.enforce_render_cache_budget();
    assert!(stats.protected_bytes > 0, "tail message should be protected during streaming");

    // Last message's cache should still have bytes.
    let last_msg = app.messages.last().expect("should have messages");
    let last_cached: usize = last_msg
        .blocks
        .iter()
        .map(|b| match b {
            MessageBlock::Text(block) => block.cache.cached_bytes(),
            _ => 0,
        })
        .sum();
    assert!(last_cached > 0, "last message cache should not be evicted");
}

// ===========================================================================
// Group 3: History Retention + Estimator
// ===========================================================================

#[tokio::test]
async fn history_retention_drops_oldest_under_pressure() {
    let mut app = test_app();

    // Push 5 messages with ~2KB text each.
    let text = "y".repeat(2000);
    for _ in 0..5 {
        app.messages.push(user_text_message(&text));
    }

    // Set a budget that can hold ~2 messages.
    app.history_retention.max_bytes = 4_000;

    let stats = app.enforce_history_retention();
    assert!(stats.dropped_messages >= 1, "expected drops, got {}", stats.dropped_messages);
    assert!(
        app.measure_history_bytes() <= app.history_retention.max_bytes,
        "remaining {} should be <= budget {}",
        app.measure_history_bytes(),
        app.history_retention.max_bytes,
    );
}

#[tokio::test]
async fn history_retention_inserts_hidden_marker() {
    let mut app = test_app();

    let text = "y".repeat(2000);
    for _ in 0..5 {
        app.messages.push(user_text_message(&text));
    }
    app.history_retention.max_bytes = 4_000;

    let _ = app.enforce_history_retention();

    // The marker is a system message whose text starts with "Older messages hidden".
    let has_marker = app.messages.iter().any(|msg| {
        matches!(msg.role, MessageRole::System(_))
            && msg.blocks.iter().any(|b| match b {
                MessageBlock::Text(block) => block.text.starts_with("Older messages hidden"),
                _ => false,
            })
    });
    assert!(has_marker, "expected a history-hidden marker message");
}

#[tokio::test]
async fn history_estimator_bytes_reasonable() {
    let text = "z".repeat(1000);
    let msg = user_text_message(&text);

    let estimated = App::measure_message_bytes(&msg);
    // The estimate should be in a reasonable range around the payload size.
    // The text is 1000 bytes, but the estimate includes struct overhead,
    // String capacity, and IncrementalMarkdown internal storage.
    assert!(estimated >= 500, "estimate {estimated} is unreasonably low for 1000-byte text");
    assert!(estimated <= 5000, "estimate {estimated} is unreasonably high for 1000-byte text");
}

// ===========================================================================
// Group 5: Regression / Full Pipeline
// ===========================================================================

#[tokio::test]
async fn full_pipeline_stream_split_measure_scroll() {
    let mut app = test_app();

    // Stream enough text with paragraph breaks to trigger splitting.
    let paragraph_words = "word ".repeat(80);
    let mut text = String::new();
    for i in 0..8 {
        write!(&mut text, "Paragraph {i}. {paragraph_words}\n\n")
            .expect("writing to String should never fail");
    }
    assert!(
        text.len() > DEFAULT_CACHE_SPLIT_SOFT_LIMIT_BYTES,
        "pipeline text {} bytes should trigger split",
        text.len(),
    );

    stream_text(&mut app, &text);
    complete_turn(&mut app);

    // Verify blocks were split.
    assert_eq!(app.messages.len(), 1);
    let block_count =
        app.messages[0].blocks.iter().filter(|b| matches!(b, MessageBlock::Text(..))).count();
    assert!(block_count >= 2, "expected split, got {block_count} blocks");
    if let MessageBlock::Text(first_block) = &app.messages[0].blocks[0] {
        assert_eq!(first_block.trailing_spacing, TextBlockSpacing::ParagraphBreak);
    }

    // Set viewport width.
    let _ = app.viewport.on_frame(80, 24);

    // Measure height.
    let spinner = inactive_spinner();
    let (height, _wrapped) = measure_message_height_cached(
        &mut app.messages[0],
        &spinner,
        80,
        app.viewport.layout_generation,
    );
    assert!(height > 0, "measured height should be > 0");

    // Set height + rebuild prefix sums.
    app.viewport.set_message_height(0, height);
    app.viewport.mark_heights_valid();
    app.viewport.rebuild_prefix_sums();

    assert_eq!(app.viewport.total_message_height(), height);
    assert_eq!(app.viewport.find_first_visible(0), 0);
    assert_eq!(app.viewport.cumulative_height_before(0), 0);
}

#[tokio::test]
async fn invalidation_from_streaming_preserves_fast_path() {
    let mut app = test_app();

    // Stream first message and complete.
    stream_text(&mut app, "First message content.");
    complete_turn(&mut app);
    assert_eq!(app.messages.len(), 1);

    // Set up viewport with valid heights and prefix sums.
    let _ = app.viewport.on_frame(80, 24);
    app.viewport.set_message_height(0, 5);
    app.viewport.mark_heights_valid();
    app.viewport.rebuild_prefix_sums();
    assert_eq!(app.viewport.prefix_sums_width, 80);

    // Insert a user message so the next streaming chunk creates a new assistant message
    // (consecutive assistant chunks merge into the last assistant message by design).
    app.messages.push(user_text_message("follow-up prompt"));

    // Stream second assistant message.
    stream_text(&mut app, "Second message content.");
    assert_eq!(app.messages.len(), 3); // msg 0: assistant, msg 1: user, msg 2: assistant

    // The new assistant message dirties itself, and the previous tail user message
    // may also be dirtied because it stops being the last message and regains its
    // trailing separator row. Earlier non-tail messages should remain untouched.
    if let Some(stale) = app.viewport.oldest_stale_index() {
        assert!(
            stale >= 1,
            "oldest stale message should be >= 1 (previous tail or new assistant), got {stale}"
        );
    }
    // The key invariant: msg 0's height cache is not dirtied by streaming msg 2.
}

#[tokio::test]
async fn resize_invalidates_all_heights() {
    let mut app = test_app();

    // Stream 2 assistant messages with a user message in between.
    stream_text(&mut app, "Message one.");
    complete_turn(&mut app);
    app.messages.push(user_text_message("next prompt"));
    stream_text(&mut app, "Message two.");
    complete_turn(&mut app);
    assert_eq!(app.messages.len(), 3); // assistant, user, assistant

    // Set up viewport at width 80 with valid caches.
    let _ = app.viewport.on_frame(80, 24);
    app.viewport.set_message_height(0, 5);
    app.viewport.set_message_height(1, 3);
    app.viewport.set_message_height(2, 10);
    app.viewport.mark_heights_valid();
    app.viewport.rebuild_prefix_sums();
    assert_eq!(app.viewport.message_heights_width, 80);
    assert_eq!(app.viewport.prefix_sums_width, 80);

    // Resize to 120.
    let resized = app.viewport.on_frame(120, 24);
    assert!(resized.width_changed, "should detect resize");
    assert_eq!(
        app.viewport.message_heights_width, 0,
        "resize should invalidate message heights width"
    );
    assert_eq!(app.viewport.prefix_sums_width, 0, "resize should invalidate prefix sums width");
}

#[tokio::test]
async fn multi_turn_message_accumulation() {
    let mut app = test_app();

    // Turn 1: assistant response.
    stream_text(&mut app, "Turn one response.");
    complete_turn(&mut app);
    assert_eq!(app.messages.len(), 1);

    // Insert user message so next stream creates a new assistant message.
    app.messages.push(user_text_message("next prompt"));

    // Turn 2: new assistant response.
    stream_text(&mut app, "Turn two response.");
    complete_turn(&mut app);
    assert_eq!(app.messages.len(), 3); // assistant, user, assistant

    // Set viewport and measure all 3 messages.
    let _ = app.viewport.on_frame(80, 24);
    let spinner = inactive_spinner();
    let (h0, _) = measure_message_height_cached(
        &mut app.messages[0],
        &spinner,
        80,
        app.viewport.layout_generation,
    );
    let (h1, _) = measure_message_height_cached(
        &mut app.messages[1],
        &spinner,
        80,
        app.viewport.layout_generation,
    );
    let (h2, _) = measure_message_height_cached(
        &mut app.messages[2],
        &spinner,
        80,
        app.viewport.layout_generation,
    );
    assert!(h0 > 0);
    assert!(h1 > 0);
    assert!(h2 > 0);

    app.viewport.set_message_height(0, h0);
    app.viewport.set_message_height(1, h1);
    app.viewport.set_message_height(2, h2);
    app.viewport.mark_heights_valid();
    app.viewport.rebuild_prefix_sums();

    assert_eq!(app.viewport.total_message_height(), h0 + h1 + h2);
    assert_eq!(app.viewport.cumulative_height_before(1), h0);
    assert_eq!(app.viewport.cumulative_height_before(2), h0 + h1);
}
