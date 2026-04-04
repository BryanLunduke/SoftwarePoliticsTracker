// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::messages::MessageBlock;
use super::types::{AppStatus, CacheBudgetEnforceStats};
use crate::agent::model;
use std::cmp::Reverse;

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub(crate) struct RenderCacheEvictionKey {
    pub(super) last_access_tick: u64,
    pub(super) bytes_desc: Reverse<usize>,
    pub(super) msg_idx: usize,
    pub(super) block_idx: usize,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub(crate) struct RenderCacheSlotState {
    pub(super) cached_bytes: usize,
    pub(super) last_access_tick: u64,
    pub(super) protected: bool,
}

impl super::App {
    #[must_use]
    fn is_streaming_tail_protected(&self) -> bool {
        matches!(self.status, AppStatus::Thinking | AppStatus::Running)
    }

    #[must_use]
    fn protected_streaming_message_idx(&self) -> Option<usize> {
        if !self.is_streaming_tail_protected() {
            return None;
        }
        self.active_turn_assistant_idx().or_else(|| self.messages.len().checked_sub(1))
    }

    #[must_use]
    fn block_cache(block: &MessageBlock) -> &super::BlockCache {
        match block {
            MessageBlock::Text(block) => &block.cache,
            MessageBlock::Welcome(welcome) => &welcome.cache,
            MessageBlock::ToolCall(tc) => &tc.cache,
        }
    }

    #[must_use]
    fn is_render_cache_block_protected(&self, msg_idx: usize, block_idx: usize) -> bool {
        let tail_protected = self.protected_streaming_message_idx() == Some(msg_idx);
        let Some(block) = self.messages.get(msg_idx).and_then(|msg| msg.blocks.get(block_idx))
        else {
            return false;
        };
        let tool_protected = matches!(
            block,
            MessageBlock::ToolCall(tc)
                if matches!(
                    tc.status,
                    model::ToolCallStatus::Pending | model::ToolCallStatus::InProgress
                )
        );
        tail_protected || tool_protected
    }

    #[must_use]
    fn render_cache_slot_key(
        msg_idx: usize,
        block_idx: usize,
        slot: &RenderCacheSlotState,
    ) -> Option<RenderCacheEvictionKey> {
        (!slot.protected && slot.cached_bytes > 0).then_some(RenderCacheEvictionKey {
            last_access_tick: slot.last_access_tick,
            bytes_desc: Reverse(slot.cached_bytes),
            msg_idx,
            block_idx,
        })
    }

    #[must_use]
    fn render_cache_slots_match_messages(&self) -> bool {
        self.render_cache_slots.len() == self.messages.len()
            && self
                .render_cache_slots
                .iter()
                .zip(&self.messages)
                .all(|(slots, msg)| slots.len() == msg.blocks.len())
    }

    pub(crate) fn rebuild_render_cache_accounting(&mut self) {
        self.render_cache_slots.clear();
        self.render_cache_slots.reserve(self.messages.len());
        self.render_cache_total_bytes = 0;
        self.render_cache_protected_bytes = 0;
        self.render_cache_evictable.clear();

        let protected_tail = self.protected_streaming_message_idx();
        for (msg_idx, msg) in self.messages.iter().enumerate() {
            let mut slots = Vec::with_capacity(msg.blocks.len());
            for (block_idx, block) in msg.blocks.iter().enumerate() {
                let cache = Self::block_cache(block);
                let cached_bytes = cache.cached_bytes();
                let protected = protected_tail == Some(msg_idx)
                    || matches!(
                        block,
                        MessageBlock::ToolCall(tc)
                            if matches!(
                                tc.status,
                                model::ToolCallStatus::Pending | model::ToolCallStatus::InProgress
                            )
                    );
                let slot = RenderCacheSlotState {
                    cached_bytes,
                    last_access_tick: cache.last_access_tick(),
                    protected,
                };
                self.render_cache_total_bytes =
                    self.render_cache_total_bytes.saturating_add(cached_bytes);
                if protected {
                    self.render_cache_protected_bytes =
                        self.render_cache_protected_bytes.saturating_add(cached_bytes);
                } else if let Some(key) = Self::render_cache_slot_key(msg_idx, block_idx, &slot) {
                    self.render_cache_evictable.insert(key);
                }
                slots.push(slot);
            }
            self.render_cache_slots.push(slots);
        }
        self.render_cache_tail_msg_idx = protected_tail;
    }

    pub(crate) fn ensure_render_cache_accounting(&mut self) {
        if !self.render_cache_slots_match_messages() {
            self.rebuild_render_cache_accounting();
        }
    }

    pub(crate) fn sync_render_cache_slot(&mut self, msg_idx: usize, block_idx: usize) {
        self.ensure_render_cache_accounting();
        let Some(old_slot) =
            self.render_cache_slots.get(msg_idx).and_then(|slots| slots.get(block_idx)).copied()
        else {
            self.rebuild_render_cache_accounting();
            return;
        };

        if let Some(old_key) = Self::render_cache_slot_key(msg_idx, block_idx, &old_slot) {
            self.render_cache_evictable.remove(&old_key);
        }
        self.render_cache_total_bytes =
            self.render_cache_total_bytes.saturating_sub(old_slot.cached_bytes);
        if old_slot.protected {
            self.render_cache_protected_bytes =
                self.render_cache_protected_bytes.saturating_sub(old_slot.cached_bytes);
        }

        let Some(block) = self.messages.get(msg_idx).and_then(|msg| msg.blocks.get(block_idx))
        else {
            self.rebuild_render_cache_accounting();
            return;
        };
        let cache = Self::block_cache(block);
        let new_slot = RenderCacheSlotState {
            cached_bytes: cache.cached_bytes(),
            last_access_tick: cache.last_access_tick(),
            protected: self.is_render_cache_block_protected(msg_idx, block_idx),
        };
        if let Some(slots) = self.render_cache_slots.get_mut(msg_idx) {
            if let Some(slot) = slots.get_mut(block_idx) {
                *slot = new_slot;
            } else {
                self.rebuild_render_cache_accounting();
                return;
            }
        } else {
            self.rebuild_render_cache_accounting();
            return;
        }

        self.render_cache_total_bytes =
            self.render_cache_total_bytes.saturating_add(new_slot.cached_bytes);
        if new_slot.protected {
            self.render_cache_protected_bytes =
                self.render_cache_protected_bytes.saturating_add(new_slot.cached_bytes);
        } else if let Some(new_key) = Self::render_cache_slot_key(msg_idx, block_idx, &new_slot) {
            self.render_cache_evictable.insert(new_key);
        }
    }

    pub(crate) fn sync_render_cache_message(&mut self, msg_idx: usize) {
        self.ensure_render_cache_accounting();
        let block_count = self.messages.get(msg_idx).map_or(0, |msg| msg.blocks.len());
        if self.render_cache_slots.get(msg_idx).map_or(usize::MAX, Vec::len) != block_count {
            self.rebuild_render_cache_accounting();
            return;
        }
        for block_idx in 0..block_count {
            self.sync_render_cache_slot(msg_idx, block_idx);
        }
    }

    fn refresh_tail_message_cache_protection(&mut self) {
        self.ensure_render_cache_accounting();
        let next_tail = self.protected_streaming_message_idx();
        if self.render_cache_tail_msg_idx == next_tail {
            return;
        }

        let previous_tail = self.render_cache_tail_msg_idx;
        self.render_cache_tail_msg_idx = next_tail;

        if let Some(msg_idx) = previous_tail {
            self.sync_render_cache_message(msg_idx);
        }
        if let Some(msg_idx) = next_tail
            && Some(msg_idx) != previous_tail
        {
            self.sync_render_cache_message(msg_idx);
        }
    }

    pub(crate) fn note_render_cache_structure_changed(&mut self) {
        self.rebuild_render_cache_accounting();
    }

    fn refresh_render_cache_eviction_order(&mut self) {
        self.ensure_render_cache_accounting();
        self.render_cache_evictable.clear();

        for (msg_idx, msg) in self.messages.iter().enumerate() {
            for (block_idx, block) in msg.blocks.iter().enumerate() {
                let cache = Self::block_cache(block);
                let protected = self.is_render_cache_block_protected(msg_idx, block_idx);
                let slot = RenderCacheSlotState {
                    cached_bytes: cache.cached_bytes(),
                    last_access_tick: cache.last_access_tick(),
                    protected,
                };
                if let Some(slots) = self.render_cache_slots.get_mut(msg_idx)
                    && let Some(existing) = slots.get_mut(block_idx)
                {
                    existing.last_access_tick = slot.last_access_tick;
                    existing.protected = slot.protected;
                }
                if let Some(key) = Self::render_cache_slot_key(msg_idx, block_idx, &slot) {
                    self.render_cache_evictable.insert(key);
                }
            }
        }
    }

    pub fn enforce_render_cache_budget(&mut self) -> CacheBudgetEnforceStats {
        let mut stats = CacheBudgetEnforceStats::default();
        self.refresh_tail_message_cache_protection();
        stats.total_before_bytes = self.render_cache_total_bytes;
        stats.protected_bytes = self.render_cache_protected_bytes;

        // Budget comparison uses only non-protected (evictable) bytes.
        let budgeted_bytes = stats.total_before_bytes.saturating_sub(stats.protected_bytes);

        if budgeted_bytes <= self.render_cache_budget.max_bytes {
            self.render_cache_budget.last_total_bytes = budgeted_bytes;
            self.render_cache_budget.last_evicted_bytes = 0;
            stats.total_after_bytes = stats.total_before_bytes;
            return stats;
        }

        self.refresh_render_cache_eviction_order();
        let mut current_budgeted = budgeted_bytes;
        stats.total_after_bytes = stats.total_before_bytes;

        while let Some(slot) = self.render_cache_evictable.first().copied() {
            if current_budgeted <= self.render_cache_budget.max_bytes {
                break;
            }
            self.render_cache_evictable.remove(&slot);
            let removed = self.evict_cache_slot(slot.msg_idx, slot.block_idx);
            if removed == 0 {
                continue;
            }
            current_budgeted = current_budgeted.saturating_sub(removed);
            stats.total_after_bytes = stats.total_after_bytes.saturating_sub(removed);
            stats.evicted_bytes = stats.evicted_bytes.saturating_add(removed);
            stats.evicted_blocks = stats.evicted_blocks.saturating_add(1);
        }

        self.render_cache_budget.last_total_bytes = current_budgeted;
        self.render_cache_budget.last_evicted_bytes = stats.evicted_bytes;
        self.render_cache_budget.total_evictions =
            self.render_cache_budget.total_evictions.saturating_add(stats.evicted_blocks);

        stats
    }

    fn evict_cache_slot(&mut self, msg_idx: usize, block_idx: usize) -> usize {
        let Some(msg) = self.messages.get_mut(msg_idx) else {
            return 0;
        };
        let Some(block) = msg.blocks.get_mut(block_idx) else {
            return 0;
        };
        let removed = match block {
            MessageBlock::Text(block) => block.cache.evict_cached_render(),
            MessageBlock::Welcome(welcome) => welcome.cache.evict_cached_render(),
            MessageBlock::ToolCall(tc) => tc.cache.evict_cached_render(),
        };
        if removed > 0 {
            self.sync_render_cache_slot(msg_idx, block_idx);
        }
        removed
    }
}
