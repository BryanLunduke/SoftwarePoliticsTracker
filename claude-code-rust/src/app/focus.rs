// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

/// Logical focus target that can claim directional key navigation.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FocusTarget {
    TodoList,
    Mention,
    Permission,
    Help,
}

/// Effective owner of directional/navigation keys.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FocusOwner {
    Input,
    TodoList,
    Mention,
    Permission,
    Help,
}

#[derive(Debug, Clone, Copy)]
#[allow(clippy::struct_excessive_bools)]
pub struct FocusContext {
    pub todo_focus_available: bool,
    pub mention_active: bool,
    pub permission_active: bool,
    pub help_active: bool,
}

impl FocusContext {
    #[must_use]
    pub const fn new(
        todo_focus_available: bool,
        mention_active: bool,
        permission_active: bool,
    ) -> Self {
        Self { todo_focus_available, mention_active, permission_active, help_active: false }
    }

    #[must_use]
    pub const fn with_help(mut self, help_active: bool) -> Self {
        self.help_active = help_active;
        self
    }

    #[must_use]
    pub const fn supports(self, target: FocusTarget) -> bool {
        match target {
            FocusTarget::TodoList => self.todo_focus_available,
            FocusTarget::Mention => self.mention_active,
            FocusTarget::Permission => self.permission_active,
            FocusTarget::Help => self.help_active,
        }
    }
}

impl From<FocusTarget> for FocusOwner {
    fn from(value: FocusTarget) -> Self {
        match value {
            FocusTarget::TodoList => Self::TodoList,
            FocusTarget::Mention => Self::Mention,
            FocusTarget::Permission => Self::Permission,
            FocusTarget::Help => Self::Help,
        }
    }
}

/// Focus claim manager:
/// latest valid claim wins; invalid claims are dropped during normalization.
#[derive(Debug, Clone, Default)]
pub struct FocusManager {
    stack: Vec<FocusTarget>,
}

impl FocusManager {
    /// Resolve the current focus owner for key routing.
    #[must_use]
    pub fn owner(&self, context: FocusContext) -> FocusOwner {
        for target in self.stack.iter().rev().copied() {
            if context.supports(target) {
                return target.into();
            }
        }
        FocusOwner::Input
    }

    /// Claim focus for the target. Latest valid claim wins.
    pub fn claim(&mut self, target: FocusTarget, context: FocusContext) {
        self.stack.retain(|t| *t != target);
        self.stack.push(target);
        self.normalize(context);
    }

    /// Release focus claim for the target.
    pub fn release(&mut self, target: FocusTarget, context: FocusContext) {
        if let Some(idx) = self.stack.iter().rposition(|t| *t == target) {
            self.stack.remove(idx);
        }
        self.normalize(context);
    }

    /// Remove claims no longer valid in the current context.
    pub fn normalize(&mut self, context: FocusContext) {
        self.stack.retain(|target| context.supports(*target));
    }
}

#[cfg(test)]
mod tests {
    use super::{FocusContext, FocusManager, FocusOwner, FocusTarget};

    #[test]
    fn owner_defaults_to_input_without_claims() {
        let mgr = FocusManager::default();
        let ctx = FocusContext::new(false, false, false);
        assert_eq!(mgr.owner(ctx), FocusOwner::Input);
    }

    #[test]
    fn latest_valid_claim_wins() {
        let mut mgr = FocusManager::default();
        let ctx = FocusContext::new(true, true, true);
        mgr.claim(FocusTarget::TodoList, ctx);
        mgr.claim(FocusTarget::Permission, ctx);
        mgr.claim(FocusTarget::Mention, ctx);
        assert_eq!(mgr.owner(ctx), FocusOwner::Mention);
    }

    #[test]
    fn invalid_claims_are_normalized_out() {
        let mut mgr = FocusManager::default();
        let valid_ctx = FocusContext::new(true, false, false);
        let invalid_ctx = FocusContext::new(false, false, false);
        mgr.claim(FocusTarget::TodoList, valid_ctx);
        assert_eq!(mgr.owner(valid_ctx), FocusOwner::TodoList);
        mgr.normalize(invalid_ctx);
        assert_eq!(mgr.owner(invalid_ctx), FocusOwner::Input);
    }

    #[test]
    fn help_focus_target_works_when_enabled() {
        let mut mgr = FocusManager::default();
        let ctx = FocusContext::new(false, false, false).with_help(true);
        mgr.claim(FocusTarget::Help, ctx);
        assert_eq!(mgr.owner(ctx), FocusOwner::Help);
    }
}
