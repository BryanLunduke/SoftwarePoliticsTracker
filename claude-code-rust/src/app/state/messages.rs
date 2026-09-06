// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::block_cache::BlockCache;
use super::tool_call_info::ToolCallInfo;
use super::types::{MessageUsage, RecentSessionInfo};
use ratatui::style::Color;
use ratatui::text::Line;
use std::ops::Range;

pub struct ChatMessage {
    pub role: MessageRole,
    pub blocks: Vec<MessageBlock>,
    pub usage: Option<MessageUsage>,
}

impl ChatMessage {
    #[must_use]
    pub fn welcome(model_name: &str, cwd: &str) -> Self {
        Self::welcome_with_recent(model_name, cwd, &[])
    }

    #[must_use]
    pub fn welcome_with_recent(
        model_name: &str,
        cwd: &str,
        recent_sessions: &[RecentSessionInfo],
    ) -> Self {
        Self {
            role: MessageRole::Welcome,
            blocks: vec![MessageBlock::Welcome(WelcomeBlock {
                model_name: model_name.to_owned(),
                cwd: cwd.to_owned(),
                recent_sessions: recent_sessions.to_vec(),
                cache: BlockCache::default(),
            })],
            usage: None,
        }
    }
}

/// Text holder for a single message block's markdown source.
///
/// Block splitting for streaming text is handled at the message construction
/// level. Within a block, this type keeps stable paragraph-sized prefixes cached
/// so only the active tail needs to be re-rendered while streaming continues.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub(crate) struct MarkdownRenderKey {
    pub width: u16,
    pub bg: Option<Color>,
    pub preserve_newlines: bool,
}

#[derive(Default)]
struct MarkdownChunk {
    range: Range<usize>,
    rendered: Option<Vec<Line<'static>>>,
    render_key: Option<MarkdownRenderKey>,
    dirty: bool,
}

impl MarkdownChunk {
    fn new(range: Range<usize>) -> Self {
        Self { range, rendered: None, render_key: None, dirty: true }
    }
}

#[derive(Default)]
pub struct IncrementalMarkdown {
    text: String,
    chunks: Vec<MarkdownChunk>,
}

impl IncrementalMarkdown {
    /// Create from existing full text (e.g. user messages, connection errors).
    /// Treats the entire text as one block source.
    #[must_use]
    pub fn from_complete(text: &str) -> Self {
        let mut markdown = Self::default();
        markdown.append(text);
        markdown
    }

    /// Append a streaming text chunk.
    pub fn append(&mut self, chunk: &str) {
        if chunk.is_empty() {
            return;
        }
        self.text.push_str(chunk);
        if let Some(last) = self.chunks.last_mut() {
            last.range.end = self.text.len();
            last.dirty = true;
            last.rendered = None;
            last.render_key = None;
        } else {
            self.chunks.push(MarkdownChunk::new(0..self.text.len()));
        }
        self.split_tail_chunks();
    }

    /// Get the full source text.
    #[must_use]
    pub fn full_text(&self) -> String {
        self.text.clone()
    }

    /// Allocated capacity of the internal text buffer in bytes.
    #[must_use]
    pub fn text_capacity(&self) -> usize {
        self.text.capacity()
    }

    /// Render this block source via the provided markdown renderer.
    /// `render_fn` converts a markdown source string into `Vec<Line>`.
    pub(crate) fn lines(
        &mut self,
        render_key: MarkdownRenderKey,
        render_fn: &impl Fn(&str) -> Vec<Line<'static>>,
    ) -> Vec<Line<'static>> {
        self.ensure_rendered(render_key, render_fn);

        let mut rendered = Vec::new();
        for chunk in &self.chunks {
            if let Some(lines) = &chunk.rendered {
                rendered.extend(lines.iter().cloned());
            }
        }
        rendered
    }

    pub fn invalidate_renders(&mut self) {
        for chunk in &mut self.chunks {
            chunk.dirty = true;
            chunk.rendered = None;
            chunk.render_key = None;
        }
    }

    pub(crate) fn ensure_rendered(
        &mut self,
        render_key: MarkdownRenderKey,
        render_fn: &impl Fn(&str) -> Vec<Line<'static>>,
    ) {
        for idx in 0..self.chunks.len() {
            let needs_render = {
                let chunk = &self.chunks[idx];
                chunk.dirty || chunk.rendered.is_none() || chunk.render_key != Some(render_key)
            };
            if !needs_render {
                continue;
            }

            let range = self.chunks[idx].range.clone();
            let rendered = render_fn(&self.text[range]);
            let chunk = &mut self.chunks[idx];
            chunk.rendered = Some(rendered);
            chunk.render_key = Some(render_key);
            chunk.dirty = false;
        }
    }

    fn split_tail_chunks(&mut self) {
        loop {
            let Some(last_idx) = self.chunks.len().checked_sub(1) else {
                break;
            };
            let range = self.chunks[last_idx].range.clone();
            let Some(split_at_rel) = find_first_stable_split(&self.text[range.clone()]) else {
                break;
            };
            let split_at = range.start + split_at_rel;
            if split_at <= range.start || split_at >= range.end {
                break;
            }

            self.chunks[last_idx] = MarkdownChunk::new(range.start..split_at);
            self.chunks.push(MarkdownChunk::new(split_at..range.end));
        }
    }
}

fn find_first_stable_split(text: &str) -> Option<usize> {
    let mut in_fenced_code = false;
    let mut saw_nonblank = false;
    let mut blank_run_end = None;
    let mut offset = 0usize;

    for line in text.split_inclusive('\n') {
        offset += line.len();
        let trimmed = line.trim_end_matches('\n').trim();
        let is_fence = trimmed.starts_with("```") || trimmed.starts_with("~~~");
        if is_fence {
            in_fenced_code = !in_fenced_code;
        }

        let is_blank = trimmed.is_empty();
        if !in_fenced_code && is_blank {
            if saw_nonblank {
                blank_run_end = Some(offset);
            }
            continue;
        }

        if let Some(boundary) = blank_run_end.take()
            && boundary < text.len()
        {
            return Some(boundary);
        }

        if !is_blank {
            saw_nonblank = true;
        }
    }

    None
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum TextBlockSpacing {
    #[default]
    None,
    ParagraphBreak,
}

impl TextBlockSpacing {
    #[must_use]
    pub fn blank_lines(self) -> usize {
        match self {
            Self::None => 0,
            Self::ParagraphBreak => 1,
        }
    }
}

pub struct TextBlock {
    pub text: String,
    pub cache: BlockCache,
    pub markdown: IncrementalMarkdown,
    /// Explicit visual spacing after this block.
    ///
    /// This is used when streaming splits one logical assistant message into
    /// multiple cached blocks at paragraph boundaries. Rendering consumes this
    /// metadata directly so spacing, height measurement, and scroll skipping all
    /// agree without mutating source text.
    pub trailing_spacing: TextBlockSpacing,
}

impl TextBlock {
    #[must_use]
    pub fn new(text: String) -> Self {
        Self {
            markdown: IncrementalMarkdown::from_complete(&text),
            text,
            cache: BlockCache::default(),
            trailing_spacing: TextBlockSpacing::None,
        }
    }

    #[must_use]
    pub fn from_complete(text: &str) -> Self {
        Self::new(text.to_owned())
    }

    #[must_use]
    pub fn with_trailing_spacing(mut self, trailing_spacing: TextBlockSpacing) -> Self {
        self.trailing_spacing = trailing_spacing;
        self
    }

    #[must_use]
    pub fn trailing_blank_lines(&self) -> usize {
        self.trailing_spacing.blank_lines()
    }
}

/// Ordered content block - text and tool calls interleaved as they arrive.
pub enum MessageBlock {
    Text(TextBlock),
    ToolCall(Box<ToolCallInfo>),
    Welcome(WelcomeBlock),
}

#[derive(Debug)]
pub enum MessageRole {
    User,
    Assistant,
    System(Option<SystemSeverity>),
    Welcome,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SystemSeverity {
    Info,
    Warning,
    Error,
}

pub struct WelcomeBlock {
    pub model_name: String,
    pub cwd: String,
    pub recent_sessions: Vec<RecentSessionInfo>,
    pub cache: BlockCache,
}
