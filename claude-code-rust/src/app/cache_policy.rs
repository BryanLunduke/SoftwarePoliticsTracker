// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

pub const DEFAULT_CACHE_SPLIT_SOFT_LIMIT_BYTES: usize = 1536;
pub const DEFAULT_CACHE_SPLIT_HARD_LIMIT_BYTES: usize = 4096;
pub const DEFAULT_TOOL_PREVIEW_LIMIT_BYTES: usize = 2048;

#[allow(clippy::struct_field_names)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct CacheSplitPolicy {
    pub soft_limit_bytes: usize,
    pub hard_limit_bytes: usize,
    pub preview_limit_bytes: usize,
}

impl Default for CacheSplitPolicy {
    fn default() -> Self {
        Self {
            soft_limit_bytes: DEFAULT_CACHE_SPLIT_SOFT_LIMIT_BYTES,
            hard_limit_bytes: DEFAULT_CACHE_SPLIT_HARD_LIMIT_BYTES,
            preview_limit_bytes: DEFAULT_TOOL_PREVIEW_LIMIT_BYTES,
        }
    }
}

#[must_use]
pub fn default_cache_split_policy() -> &'static CacheSplitPolicy {
    static POLICY: CacheSplitPolicy = CacheSplitPolicy {
        soft_limit_bytes: DEFAULT_CACHE_SPLIT_SOFT_LIMIT_BYTES,
        hard_limit_bytes: DEFAULT_CACHE_SPLIT_HARD_LIMIT_BYTES,
        preview_limit_bytes: DEFAULT_TOOL_PREVIEW_LIMIT_BYTES,
    };
    &POLICY
}

#[must_use]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TextSplitKind {
    Generic,
    ParagraphBoundary,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct TextSplitDecision {
    pub split_at: usize,
    pub kind: TextSplitKind,
}

#[must_use]
pub fn find_text_split(text: &str, policy: CacheSplitPolicy) -> Option<TextSplitDecision> {
    let bytes = text.as_bytes();
    let mut in_fence = false;
    let mut i = 0usize;

    let mut soft_newline = None;
    let mut soft_sentence = None;
    let mut hard_newline = None;
    let mut hard_sentence = None;
    let mut post_hard_newline = None;
    let mut post_hard_sentence = None;

    while i < bytes.len() {
        if (i == 0 || bytes[i - 1] == b'\n') && bytes[i..].starts_with(b"```") {
            in_fence = !in_fence;
        }

        if !in_fence {
            if i + 1 < bytes.len() && bytes[i] == b'\n' && bytes[i + 1] == b'\n' {
                let split_at = i + 2;
                if split_at < bytes.len() {
                    return Some(TextSplitDecision {
                        split_at,
                        kind: TextSplitKind::ParagraphBoundary,
                    });
                }
                return None;
            }

            if bytes[i] == b'\n' {
                track_text_split_candidate(
                    i + 1,
                    &policy,
                    &mut soft_newline,
                    &mut hard_newline,
                    &mut post_hard_newline,
                );
            }

            if is_sentence_boundary(bytes, i) {
                track_text_split_candidate(
                    i + 1,
                    &policy,
                    &mut soft_sentence,
                    &mut hard_sentence,
                    &mut post_hard_sentence,
                );
            }
        }
        i += 1;
    }

    if bytes.len() >= policy.soft_limit_bytes
        && let Some(split_at) = pick_text_split_candidate(soft_newline, soft_sentence)
        && split_at < bytes.len()
    {
        return Some(TextSplitDecision { split_at, kind: TextSplitKind::Generic });
    }

    if bytes.len() >= policy.hard_limit_bytes
        && let Some(split_at) =
            hard_newline.or(post_hard_newline).or(hard_sentence).or(post_hard_sentence)
        && split_at < bytes.len()
    {
        return Some(TextSplitDecision { split_at, kind: TextSplitKind::Generic });
    }

    None
}

#[must_use]
pub fn find_text_split_index(text: &str, policy: CacheSplitPolicy) -> Option<usize> {
    find_text_split(text, policy).map(|decision| decision.split_at)
}

fn track_text_split_candidate(
    split_at: usize,
    policy: &CacheSplitPolicy,
    soft_slot: &mut Option<usize>,
    hard_slot: &mut Option<usize>,
    post_hard_slot: &mut Option<usize>,
) {
    if split_at <= policy.soft_limit_bytes {
        *soft_slot = Some(split_at);
    }
    if split_at <= policy.hard_limit_bytes {
        *hard_slot = Some(split_at);
    } else if post_hard_slot.is_none() {
        *post_hard_slot = Some(split_at);
    }
}

fn pick_text_split_candidate(newline: Option<usize>, sentence: Option<usize>) -> Option<usize> {
    newline.or(sentence)
}

fn is_sentence_boundary(bytes: &[u8], i: usize) -> bool {
    matches!(bytes[i], b'.' | b'!' | b'?')
        && (i + 1 == bytes.len() || matches!(bytes[i + 1], b' ' | b'\t' | b'\r' | b'\n'))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn split_prefers_double_newline() {
        let text = "first\n\nsecond";
        let split_at = find_text_split_index(text, *default_cache_split_policy());
        assert_eq!(split_at, Some("first\n\n".len()));
    }

    #[test]
    fn split_respects_soft_limit() {
        let policy = *default_cache_split_policy();
        let prefix = "a".repeat(policy.soft_limit_bytes - 1);
        let text = format!("{prefix}\nsecond line");
        let split_at = find_text_split_index(&text, policy).expect("expected split");
        assert_eq!(&text[..split_at], format!("{prefix}\n"));
    }

    #[test]
    fn split_ignores_double_newline_inside_fence() {
        let text = "```rust\nfirst\n\nsecond\n```";
        assert!(find_text_split_index(text, *default_cache_split_policy()).is_none());
    }
}
