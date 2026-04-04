// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

mod edit;
mod mcp;
mod mcp_edit;
mod resolve;
pub mod store;

use super::view::{self, ActiveView};
use crate::agent::model::EffortLevel;
use crate::app::App;
use crossterm::event::{KeyCode, KeyEvent, KeyModifiers};
use std::path::PathBuf;

pub(crate) use edit::{
    OverlayModelOption, model_overlay_options, supported_effort_levels_for_model,
};
pub(crate) use mcp::{
    McpAuthRedirectOverlayState, McpCallbackUrlOverlayState, McpDetailsOverlayState,
    McpElicitationOverlayState, available_mcp_actions, handle_mcp_elicitation_completed,
    handle_mcp_operation_error, is_mcp_action_available, present_mcp_auth_redirect,
    present_mcp_elicitation_request, refresh_mcp_snapshot,
};
pub(crate) use resolve::language_input_validation_message;
use resolve::resolve_setting_document;
use serde_json::Value;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ConfigTab {
    Settings,
    Plugins,
    Status,
    Usage,
    Mcp,
}

impl ConfigTab {
    pub const ALL: [Self; 5] =
        [Self::Settings, Self::Plugins, Self::Status, Self::Usage, Self::Mcp];

    pub const fn title(self) -> &'static str {
        match self {
            Self::Settings => "Settings",
            Self::Plugins => "Plugins",
            Self::Status => "Status",
            Self::Usage => "Usage",
            Self::Mcp => "MCP",
        }
    }

    const fn next(self) -> Self {
        match self {
            Self::Settings => Self::Plugins,
            Self::Plugins => Self::Status,
            Self::Status => Self::Usage,
            Self::Usage => Self::Mcp,
            Self::Mcp => Self::Settings,
        }
    }

    const fn prev(self) -> Self {
        match self {
            Self::Settings => Self::Mcp,
            Self::Plugins => Self::Settings,
            Self::Status => Self::Plugins,
            Self::Usage => Self::Status,
            Self::Mcp => Self::Usage,
        }
    }
}

#[repr(usize)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum SettingId {
    AlwaysThinking,
    Model,
    DefaultPermissionMode,
    EditorMode,
    FastMode,
    Language,
    Notifications,
    OutputStyle,
    ReduceMotion,
    RespectGitignore,
    ShowTips,
    TerminalProgressBar,
    Theme,
    ThinkingEffort,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SettingKind {
    Bool,
    Enum,
    DynamicEnum,
    Text,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum EditorKind {
    Toggle,
    Cycle,
    Overlay,
    Search,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ValueSource {
    PersistedOnly,
    RuntimeBacked,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SettingFile {
    Settings,
    LocalSettings,
    Preferences,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RuntimeCatalogKind {
    Models,
    PermissionModes,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FallbackPolicy {
    None,
    AppDefault,
    English,
    RuntimeDefault,
    Unset,
}

impl FallbackPolicy {
    #[must_use]
    pub const fn short_label(self) -> &'static str {
        match self {
            Self::None => "current value",
            Self::AppDefault => "default",
            Self::English => "English",
            Self::RuntimeDefault => "runtime default",
            Self::Unset => "unset",
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct SettingOption {
    pub stored: &'static str,
    pub label: &'static str,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SettingOptions {
    None,
    Static(&'static [SettingOption]),
    RuntimeCatalog(RuntimeCatalogKind),
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct SettingSpec {
    pub id: SettingId,
    pub entry_id: &'static str,
    pub label: &'static str,
    pub description: &'static str,
    pub file: SettingFile,
    pub json_path: &'static [&'static str],
    pub kind: SettingKind,
    pub editor: EditorKind,
    pub source: ValueSource,
    pub options: SettingOptions,
    pub fallback: FallbackPolicy,
    pub supported: bool,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum DefaultPermissionMode {
    #[default]
    Default,
    AcceptEdits,
    Plan,
    DontAsk,
    BypassPermissions,
}

impl DefaultPermissionMode {
    #[must_use]
    pub const fn as_stored(self) -> &'static str {
        match self {
            Self::Default => "default",
            Self::AcceptEdits => "acceptEdits",
            Self::Plan => "plan",
            Self::DontAsk => "dontAsk",
            Self::BypassPermissions => "bypassPermissions",
        }
    }

    #[must_use]
    pub const fn label(self) -> &'static str {
        match self {
            Self::Default => "Default",
            Self::AcceptEdits => "Accept Edits",
            Self::Plan => "Plan",
            Self::DontAsk => "Don't Ask",
            Self::BypassPermissions => "Bypass Permissions",
        }
    }

    #[must_use]
    pub fn from_stored(value: &str) -> Option<Self> {
        match value {
            "default" => Some(Self::Default),
            "acceptEdits" => Some(Self::AcceptEdits),
            "plan" => Some(Self::Plan),
            "dontAsk" => Some(Self::DontAsk),
            "bypassPermissions" => Some(Self::BypassPermissions),
            _ => None,
        }
    }

    #[must_use]
    pub const fn next(self) -> Self {
        match self {
            Self::Default => Self::AcceptEdits,
            Self::AcceptEdits => Self::Plan,
            Self::Plan => Self::DontAsk,
            Self::DontAsk => Self::BypassPermissions,
            Self::BypassPermissions => Self::Default,
        }
    }

    #[must_use]
    pub const fn prev(self) -> Self {
        match self {
            Self::Default => Self::BypassPermissions,
            Self::AcceptEdits => Self::Default,
            Self::Plan => Self::AcceptEdits,
            Self::DontAsk => Self::Plan,
            Self::BypassPermissions => Self::DontAsk,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum PreferredNotifChannel {
    #[default]
    Iterm2,
    Iterm2WithBell,
    TerminalBell,
    NotificationsDisabled,
    Ghostty,
}

impl PreferredNotifChannel {
    #[must_use]
    pub const fn as_stored(self) -> &'static str {
        match self {
            Self::Iterm2 => "iterm2",
            Self::Iterm2WithBell => "iterm2_with_bell",
            Self::TerminalBell => "terminal_bell",
            Self::NotificationsDisabled => "notifications_disabled",
            Self::Ghostty => "ghostty",
        }
    }

    #[must_use]
    pub const fn label(self) -> &'static str {
        match self {
            Self::Iterm2 => "Auto / iTerm2",
            Self::Iterm2WithBell => "iTerm2 with Bell",
            Self::TerminalBell => "Terminal Bell",
            Self::NotificationsDisabled => "Disabled",
            Self::Ghostty => "Ghostty",
        }
    }

    #[must_use]
    pub fn from_stored(value: &str) -> Option<Self> {
        match value {
            "iterm2" => Some(Self::Iterm2),
            "iterm2_with_bell" => Some(Self::Iterm2WithBell),
            "terminal_bell" => Some(Self::TerminalBell),
            "notifications_disabled" => Some(Self::NotificationsDisabled),
            "ghostty" => Some(Self::Ghostty),
            _ => None,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum OutputStyle {
    #[default]
    Default,
    Explanatory,
    Learning,
}

impl OutputStyle {
    pub const ALL: [Self; 3] = [Self::Default, Self::Explanatory, Self::Learning];

    #[must_use]
    pub const fn as_stored(self) -> &'static str {
        match self {
            Self::Default => "Default",
            Self::Explanatory => "Explanatory",
            Self::Learning => "Learning",
        }
    }

    #[must_use]
    pub const fn label(self) -> &'static str {
        self.as_stored()
    }

    #[must_use]
    pub const fn description(self) -> &'static str {
        match self {
            Self::Default => {
                "Claude completes coding tasks efficiently and provides concise responses"
            }
            Self::Explanatory => "Claude explains its implementation choices and codebase patterns",
            Self::Learning => {
                "Claude pauses and asks you to write small pieces of code for hands-on practice"
            }
        }
    }

    #[must_use]
    pub fn from_stored(value: &str) -> Option<Self> {
        match value {
            "Default" => Some(Self::Default),
            "Explanatory" => Some(Self::Explanatory),
            "Learning" => Some(Self::Learning),
            _ => None,
        }
    }
}

const DEFAULT_PERMISSION_OPTIONS: &[SettingOption] = &[
    SettingOption { stored: "default", label: "Default" },
    SettingOption { stored: "acceptEdits", label: "Accept Edits" },
    SettingOption { stored: "plan", label: "Plan" },
    SettingOption { stored: "dontAsk", label: "Don't Ask" },
    SettingOption { stored: "bypassPermissions", label: "Bypass Permissions" },
];

const NOTIFICATION_OPTIONS: &[SettingOption] = &[
    SettingOption { stored: "iterm2", label: "Auto / iTerm2" },
    SettingOption { stored: "iterm2_with_bell", label: "iTerm2 with Bell" },
    SettingOption { stored: "terminal_bell", label: "Terminal Bell" },
    SettingOption { stored: "ghostty", label: "Ghostty" },
    SettingOption { stored: "notifications_disabled", label: "Disabled" },
];

const OUTPUT_STYLE_OPTIONS: &[SettingOption] = &[
    SettingOption { stored: "Default", label: "Default" },
    SettingOption { stored: "Explanatory", label: "Explanatory" },
    SettingOption { stored: "Learning", label: "Learning" },
];

const THEME_OPTIONS: &[SettingOption] = &[
    SettingOption { stored: "dark", label: "Dark" },
    SettingOption { stored: "light", label: "Light" },
    SettingOption { stored: "light-daltonized", label: "Light (Daltonized)" },
    SettingOption { stored: "dark-daltonized", label: "Dark (Daltonized)" },
];

const EDITOR_MODE_OPTIONS: &[SettingOption] = &[
    SettingOption { stored: "default", label: "Default" },
    SettingOption { stored: "vim", label: "Vim" },
];
const DEFAULT_MODEL_ID: &str = "default";
const DEFAULT_MODEL_LABEL: &str = "Default";
const DEFAULT_EFFORT_LEVELS: [EffortLevel; 3] =
    [EffortLevel::Low, EffortLevel::Medium, EffortLevel::High];
const LANGUAGE_MIN_CHARS: usize = 2;
const LANGUAGE_MAX_CHARS: usize = 30;

const EFFORT_OPTIONS: &[SettingOption] = &[
    SettingOption { stored: "low", label: "Low" },
    SettingOption { stored: "medium", label: "Medium" },
    SettingOption { stored: "high", label: "High" },
];

const CONFIG_SETTINGS: [SettingSpec; 14] = [
    SettingSpec {
        id: SettingId::AlwaysThinking,
        entry_id: "A04",
        label: "Always Thinking",
        description: "Enable adaptive thinking for new sessions. When off, new sessions start with thinking disabled.",
        file: SettingFile::Settings,
        json_path: &["alwaysThinkingEnabled"],
        kind: SettingKind::Bool,
        editor: EditorKind::Toggle,
        source: ValueSource::PersistedOnly,
        options: SettingOptions::None,
        fallback: FallbackPolicy::AppDefault,
        supported: true,
    },
    SettingSpec {
        id: SettingId::Model,
        entry_id: "A19",
        label: "Default model",
        description: "Sets the default model for new sessions and opens the combined model and thinking effort picker.",
        file: SettingFile::Settings,
        json_path: &["model"],
        kind: SettingKind::DynamicEnum,
        editor: EditorKind::Overlay,
        source: ValueSource::RuntimeBacked,
        options: SettingOptions::RuntimeCatalog(RuntimeCatalogKind::Models),
        fallback: FallbackPolicy::RuntimeDefault,
        supported: true,
    },
    SettingSpec {
        id: SettingId::DefaultPermissionMode,
        entry_id: "A09",
        label: "Default permission mode",
        description: "Sets the default approval behavior for future sessions.",
        file: SettingFile::Settings,
        json_path: &["permissions", "defaultMode"],
        kind: SettingKind::DynamicEnum,
        editor: EditorKind::Cycle,
        source: ValueSource::RuntimeBacked,
        options: SettingOptions::RuntimeCatalog(RuntimeCatalogKind::PermissionModes),
        fallback: FallbackPolicy::RuntimeDefault,
        supported: true,
    },
    SettingSpec {
        id: SettingId::EditorMode,
        entry_id: "A17",
        label: "Editor mode",
        description: "Controls how text editing keys behave in the TUI.",
        file: SettingFile::Preferences,
        json_path: &["editorMode"],
        kind: SettingKind::Enum,
        editor: EditorKind::Cycle,
        source: ValueSource::PersistedOnly,
        options: SettingOptions::Static(EDITOR_MODE_OPTIONS),
        fallback: FallbackPolicy::AppDefault,
        supported: false,
    },
    SettingSpec {
        id: SettingId::FastMode,
        entry_id: "A05",
        label: "Fast mode",
        description: "Controls the persisted fast-mode preference for future sessions.",
        file: SettingFile::Settings,
        json_path: &["fastMode"],
        kind: SettingKind::Bool,
        editor: EditorKind::Toggle,
        source: ValueSource::PersistedOnly,
        options: SettingOptions::None,
        fallback: FallbackPolicy::AppDefault,
        supported: true,
    },
    SettingSpec {
        id: SettingId::Language,
        entry_id: "A16",
        label: "Language",
        description: "Controls the free-text language instruction Claude uses in sessions. Accepts 2 to 30 characters and does not localize the UI.",
        file: SettingFile::Settings,
        json_path: &["language"],
        kind: SettingKind::Text,
        editor: EditorKind::Overlay,
        source: ValueSource::PersistedOnly,
        options: SettingOptions::None,
        fallback: FallbackPolicy::Unset,
        supported: true,
    },
    SettingSpec {
        id: SettingId::Notifications,
        entry_id: "A14",
        label: "Notifications",
        description: "Controls how Claude Code notifies you when attention is needed.",
        file: SettingFile::Preferences,
        json_path: &["preferredNotifChannel"],
        kind: SettingKind::Enum,
        editor: EditorKind::Cycle,
        source: ValueSource::PersistedOnly,
        options: SettingOptions::Static(NOTIFICATION_OPTIONS),
        fallback: FallbackPolicy::AppDefault,
        supported: true,
    },
    SettingSpec {
        id: SettingId::OutputStyle,
        entry_id: "A15",
        label: "Output style",
        description: "Changes how Claude communicates with you in sessions.",
        file: SettingFile::LocalSettings,
        json_path: &["outputStyle"],
        kind: SettingKind::Enum,
        editor: EditorKind::Overlay,
        source: ValueSource::PersistedOnly,
        options: SettingOptions::Static(OUTPUT_STYLE_OPTIONS),
        fallback: FallbackPolicy::AppDefault,
        supported: true,
    },
    SettingSpec {
        id: SettingId::ReduceMotion,
        entry_id: "A03",
        label: "Reduce motion",
        description: "Reduce UI motion by slowing spinners and disabling smooth chat scrolling.",
        file: SettingFile::LocalSettings,
        json_path: &["prefersReducedMotion"],
        kind: SettingKind::Bool,
        editor: EditorKind::Toggle,
        source: ValueSource::PersistedOnly,
        options: SettingOptions::None,
        fallback: FallbackPolicy::AppDefault,
        supported: true,
    },
    SettingSpec {
        id: SettingId::RespectGitignore,
        entry_id: "A10",
        label: "Respect .gitignore",
        description: "Controls whether @ file mentions hide entries ignored by git ignore rules.",
        file: SettingFile::Preferences,
        json_path: &["respectGitignore"],
        kind: SettingKind::Bool,
        editor: EditorKind::Toggle,
        source: ValueSource::PersistedOnly,
        options: SettingOptions::None,
        fallback: FallbackPolicy::AppDefault,
        supported: true,
    },
    SettingSpec {
        id: SettingId::ShowTips,
        entry_id: "A02",
        label: "Show Tips",
        description: "Controls whether Claude shows spinner tips in supported clients.",
        file: SettingFile::LocalSettings,
        json_path: &["spinnerTipsEnabled"],
        kind: SettingKind::Bool,
        editor: EditorKind::Toggle,
        source: ValueSource::PersistedOnly,
        options: SettingOptions::None,
        fallback: FallbackPolicy::AppDefault,
        supported: false,
    },
    SettingSpec {
        id: SettingId::TerminalProgressBar,
        entry_id: "A08",
        label: "Terminal progress bar",
        description: "Controls whether Claude should show its terminal progress bar in supported clients.",
        file: SettingFile::Preferences,
        json_path: &["terminalProgressBarEnabled"],
        kind: SettingKind::Bool,
        editor: EditorKind::Toggle,
        source: ValueSource::PersistedOnly,
        options: SettingOptions::None,
        fallback: FallbackPolicy::AppDefault,
        supported: false,
    },
    SettingSpec {
        id: SettingId::Theme,
        entry_id: "A13",
        label: "Theme",
        description: "Controls the TUI color theme.",
        file: SettingFile::Preferences,
        json_path: &["theme"],
        kind: SettingKind::Enum,
        editor: EditorKind::Cycle,
        source: ValueSource::PersistedOnly,
        options: SettingOptions::Static(THEME_OPTIONS),
        fallback: FallbackPolicy::AppDefault,
        supported: false,
    },
    SettingSpec {
        id: SettingId::ThinkingEffort,
        entry_id: "A20",
        label: "Thinking effort",
        description: "Controls how much effort Claude uses when thinking for new sessions. Only applies when Always Thinking is on and the selected model supports effort.",
        file: SettingFile::Settings,
        json_path: &["effortLevel"],
        kind: SettingKind::Enum,
        editor: EditorKind::Overlay,
        source: ValueSource::PersistedOnly,
        options: SettingOptions::Static(EFFORT_OPTIONS),
        fallback: FallbackPolicy::AppDefault,
        supported: true,
    },
];

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SettingValidation {
    Valid,
    InvalidValue,
    UnavailableOption,
}

impl SettingValidation {
    #[must_use]
    pub const fn is_invalid(self) -> bool {
        !matches!(self, Self::Valid)
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ResolvedChoice {
    Automatic,
    Stored(String),
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ResolvedSettingValue {
    Bool(bool),
    Choice(ResolvedChoice),
    Text(String),
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ResolvedSetting {
    pub value: ResolvedSettingValue,
    pub validation: SettingValidation,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum OverlayFocus {
    Model,
    Effort,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ModelAndEffortOverlayState {
    pub focus: OverlayFocus,
    pub selected_model: String,
    pub selected_effort: EffortLevel,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct OutputStyleOverlayState {
    pub selected: OutputStyle,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LanguageOverlayState {
    pub draft: String,
    pub cursor: usize,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SessionRenameOverlayState {
    pub draft: String,
    pub cursor: usize,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MarketplaceActionKind {
    Update,
    Remove,
}

impl MarketplaceActionKind {
    #[must_use]
    pub const fn label(self) -> &'static str {
        match self {
            Self::Update => "Update",
            Self::Remove => "Remove",
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum InstalledPluginActionKind {
    Enable,
    Disable,
    Update,
    InstallInCurrentProject,
    Uninstall,
}

impl InstalledPluginActionKind {
    #[must_use]
    pub const fn label(self) -> &'static str {
        match self {
            Self::Enable => "Enable",
            Self::Disable => "Disable",
            Self::Update => "Update",
            Self::InstallInCurrentProject => "Install in current project",
            Self::Uninstall => "Uninstall",
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PluginInstallActionKind {
    User,
    Project,
    Local,
}

impl PluginInstallActionKind {
    #[must_use]
    pub const fn label(self) -> &'static str {
        match self {
            Self::User => "Install for user",
            Self::Project => "Install for project",
            Self::Local => "Install locally",
        }
    }

    #[must_use]
    pub const fn scope(self) -> &'static str {
        match self {
            Self::User => "user",
            Self::Project => "project",
            Self::Local => "local",
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct InstalledPluginActionOverlayState {
    pub plugin_id: String,
    pub title: String,
    pub description: String,
    pub scope: String,
    pub project_path: Option<String>,
    pub selected_index: usize,
    pub actions: Vec<InstalledPluginActionKind>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct PluginInstallOverlayState {
    pub plugin_id: String,
    pub title: String,
    pub description: String,
    pub selected_index: usize,
    pub actions: Vec<PluginInstallActionKind>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MarketplaceActionsOverlayState {
    pub name: String,
    pub title: String,
    pub description: String,
    pub selected_index: usize,
    pub actions: Vec<MarketplaceActionKind>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct AddMarketplaceOverlayState {
    pub draft: String,
    pub cursor: usize,
}

#[derive(Debug, Clone, PartialEq)]
pub enum ConfigOverlayState {
    ModelAndEffort(ModelAndEffortOverlayState),
    OutputStyle(OutputStyleOverlayState),
    Language(LanguageOverlayState),
    SessionRename(SessionRenameOverlayState),
    InstalledPluginActions(InstalledPluginActionOverlayState),
    PluginInstallActions(PluginInstallOverlayState),
    MarketplaceActions(MarketplaceActionsOverlayState),
    AddMarketplace(AddMarketplaceOverlayState),
    McpDetails(McpDetailsOverlayState),
    McpCallbackUrl(McpCallbackUrlOverlayState),
    McpElicitation(McpElicitationOverlayState),
    McpAuthRedirect(McpAuthRedirectOverlayState),
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum PendingSessionTitleChangeKind {
    Rename { requested_title: Option<String> },
    Generate,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct PendingSessionTitleChangeState {
    pub session_id: String,
    pub kind: PendingSessionTitleChangeKind,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ConfigState {
    pub active_tab: ConfigTab,
    pub selected_setting_index: usize,
    pub settings_scroll_offset: usize,
    pub mcp_selected_server_index: usize,
    pub overlay: Option<ConfigOverlayState>,
    pub committed_settings_document: Value,
    pub committed_local_settings_document: Value,
    pub committed_preferences_document: Value,
    pub settings_path: Option<PathBuf>,
    pub local_settings_path: Option<PathBuf>,
    pub preferences_path: Option<PathBuf>,
    pub status_message: Option<String>,
    pub last_error: Option<String>,
    pub pending_session_title_change: Option<PendingSessionTitleChangeState>,
}

impl Default for ConfigState {
    fn default() -> Self {
        Self {
            active_tab: ConfigTab::Settings,
            selected_setting_index: 0,
            settings_scroll_offset: 0,
            mcp_selected_server_index: 0,
            overlay: None,
            committed_settings_document: Value::Object(serde_json::Map::new()),
            committed_local_settings_document: Value::Object(serde_json::Map::new()),
            committed_preferences_document: Value::Object(serde_json::Map::new()),
            settings_path: None,
            local_settings_path: None,
            preferences_path: None,
            status_message: None,
            last_error: None,
            pending_session_title_change: None,
        }
    }
}

impl ConfigState {
    #[must_use]
    pub fn fast_mode_effective(&self) -> bool {
        match resolve_setting_document(&self.committed_settings_document, SettingId::FastMode, &[])
            .value
        {
            ResolvedSettingValue::Bool(value) => value,
            ResolvedSettingValue::Choice(_) | ResolvedSettingValue::Text(_) => false,
        }
    }

    #[must_use]
    pub fn always_thinking_effective(&self) -> bool {
        match resolve_setting_document(
            &self.committed_settings_document,
            SettingId::AlwaysThinking,
            &[],
        )
        .value
        {
            ResolvedSettingValue::Bool(value) => value,
            ResolvedSettingValue::Choice(_) | ResolvedSettingValue::Text(_) => false,
        }
    }

    #[must_use]
    pub fn model_effective(&self) -> Option<String> {
        match resolve_setting_document(&self.committed_settings_document, SettingId::Model, &[])
            .value
        {
            ResolvedSettingValue::Choice(ResolvedChoice::Automatic) => {
                Some(DEFAULT_MODEL_ID.to_owned())
            }
            ResolvedSettingValue::Choice(ResolvedChoice::Stored(value)) => Some(value),
            ResolvedSettingValue::Bool(_) | ResolvedSettingValue::Text(_) => None,
        }
    }

    #[must_use]
    pub fn thinking_effort_effective(&self) -> EffortLevel {
        store::thinking_effort_level(&self.committed_settings_document)
            .unwrap_or(EffortLevel::Medium)
    }

    #[must_use]
    pub fn default_permission_mode_effective(&self) -> DefaultPermissionMode {
        match resolve_setting_document(
            &self.committed_settings_document,
            SettingId::DefaultPermissionMode,
            &[],
        )
        .value
        {
            ResolvedSettingValue::Choice(ResolvedChoice::Stored(value)) => {
                DefaultPermissionMode::from_stored(&value).unwrap_or_default()
            }
            ResolvedSettingValue::Bool(_)
            | ResolvedSettingValue::Choice(ResolvedChoice::Automatic)
            | ResolvedSettingValue::Text(_) => DefaultPermissionMode::Default,
        }
    }

    #[must_use]
    pub fn respect_gitignore_effective(&self) -> bool {
        store::respect_gitignore(&self.committed_preferences_document).unwrap_or(true)
    }

    #[must_use]
    pub fn preferred_notification_channel_effective(&self) -> PreferredNotifChannel {
        store::preferred_notification_channel(&self.committed_preferences_document)
            .unwrap_or_default()
    }

    #[must_use]
    pub fn prefers_reduced_motion_effective(&self) -> bool {
        store::prefers_reduced_motion(&self.committed_local_settings_document).unwrap_or(false)
    }

    #[must_use]
    pub fn output_style_effective(&self) -> OutputStyle {
        store::output_style(&self.committed_local_settings_document).unwrap_or_default()
    }

    #[must_use]
    pub fn selected_setting_spec(&self) -> Option<&'static SettingSpec> {
        setting_specs().get(self.selected_setting_index)
    }

    #[must_use]
    pub fn model_and_effort_overlay(&self) -> Option<&ModelAndEffortOverlayState> {
        match &self.overlay {
            Some(ConfigOverlayState::ModelAndEffort(overlay)) => Some(overlay),
            Some(
                ConfigOverlayState::OutputStyle(_)
                | ConfigOverlayState::Language(_)
                | ConfigOverlayState::SessionRename(_)
                | ConfigOverlayState::InstalledPluginActions(_)
                | ConfigOverlayState::PluginInstallActions(_)
                | ConfigOverlayState::MarketplaceActions(_)
                | ConfigOverlayState::AddMarketplace(_)
                | ConfigOverlayState::McpDetails(_)
                | ConfigOverlayState::McpCallbackUrl(_)
                | ConfigOverlayState::McpElicitation(_)
                | ConfigOverlayState::McpAuthRedirect(_),
            )
            | None => None,
        }
    }

    pub fn model_and_effort_overlay_mut(&mut self) -> Option<&mut ModelAndEffortOverlayState> {
        match &mut self.overlay {
            Some(ConfigOverlayState::ModelAndEffort(overlay)) => Some(overlay),
            Some(
                ConfigOverlayState::OutputStyle(_)
                | ConfigOverlayState::Language(_)
                | ConfigOverlayState::SessionRename(_)
                | ConfigOverlayState::InstalledPluginActions(_)
                | ConfigOverlayState::PluginInstallActions(_)
                | ConfigOverlayState::MarketplaceActions(_)
                | ConfigOverlayState::AddMarketplace(_)
                | ConfigOverlayState::McpDetails(_)
                | ConfigOverlayState::McpCallbackUrl(_)
                | ConfigOverlayState::McpElicitation(_)
                | ConfigOverlayState::McpAuthRedirect(_),
            )
            | None => None,
        }
    }

    #[must_use]
    pub fn output_style_overlay(&self) -> Option<&OutputStyleOverlayState> {
        match &self.overlay {
            Some(ConfigOverlayState::OutputStyle(overlay)) => Some(overlay),
            Some(
                ConfigOverlayState::ModelAndEffort(_)
                | ConfigOverlayState::Language(_)
                | ConfigOverlayState::SessionRename(_)
                | ConfigOverlayState::InstalledPluginActions(_)
                | ConfigOverlayState::PluginInstallActions(_)
                | ConfigOverlayState::MarketplaceActions(_)
                | ConfigOverlayState::AddMarketplace(_)
                | ConfigOverlayState::McpDetails(_)
                | ConfigOverlayState::McpCallbackUrl(_)
                | ConfigOverlayState::McpElicitation(_)
                | ConfigOverlayState::McpAuthRedirect(_),
            )
            | None => None,
        }
    }

    pub fn output_style_overlay_mut(&mut self) -> Option<&mut OutputStyleOverlayState> {
        match &mut self.overlay {
            Some(ConfigOverlayState::OutputStyle(overlay)) => Some(overlay),
            Some(
                ConfigOverlayState::ModelAndEffort(_)
                | ConfigOverlayState::Language(_)
                | ConfigOverlayState::SessionRename(_)
                | ConfigOverlayState::InstalledPluginActions(_)
                | ConfigOverlayState::PluginInstallActions(_)
                | ConfigOverlayState::MarketplaceActions(_)
                | ConfigOverlayState::AddMarketplace(_)
                | ConfigOverlayState::McpDetails(_)
                | ConfigOverlayState::McpCallbackUrl(_)
                | ConfigOverlayState::McpElicitation(_)
                | ConfigOverlayState::McpAuthRedirect(_),
            )
            | None => None,
        }
    }

    #[must_use]
    pub fn language_overlay(&self) -> Option<&LanguageOverlayState> {
        match &self.overlay {
            Some(ConfigOverlayState::Language(overlay)) => Some(overlay),
            Some(
                ConfigOverlayState::ModelAndEffort(_)
                | ConfigOverlayState::OutputStyle(_)
                | ConfigOverlayState::SessionRename(_)
                | ConfigOverlayState::InstalledPluginActions(_)
                | ConfigOverlayState::PluginInstallActions(_)
                | ConfigOverlayState::MarketplaceActions(_)
                | ConfigOverlayState::AddMarketplace(_)
                | ConfigOverlayState::McpDetails(_)
                | ConfigOverlayState::McpCallbackUrl(_)
                | ConfigOverlayState::McpElicitation(_)
                | ConfigOverlayState::McpAuthRedirect(_),
            )
            | None => None,
        }
    }

    pub fn language_overlay_mut(&mut self) -> Option<&mut LanguageOverlayState> {
        match &mut self.overlay {
            Some(ConfigOverlayState::Language(overlay)) => Some(overlay),
            Some(
                ConfigOverlayState::ModelAndEffort(_)
                | ConfigOverlayState::OutputStyle(_)
                | ConfigOverlayState::SessionRename(_)
                | ConfigOverlayState::InstalledPluginActions(_)
                | ConfigOverlayState::PluginInstallActions(_)
                | ConfigOverlayState::MarketplaceActions(_)
                | ConfigOverlayState::AddMarketplace(_)
                | ConfigOverlayState::McpDetails(_)
                | ConfigOverlayState::McpCallbackUrl(_)
                | ConfigOverlayState::McpElicitation(_)
                | ConfigOverlayState::McpAuthRedirect(_),
            )
            | None => None,
        }
    }

    #[must_use]
    pub fn session_rename_overlay(&self) -> Option<&SessionRenameOverlayState> {
        match &self.overlay {
            Some(ConfigOverlayState::SessionRename(overlay)) => Some(overlay),
            Some(
                ConfigOverlayState::ModelAndEffort(_)
                | ConfigOverlayState::OutputStyle(_)
                | ConfigOverlayState::Language(_)
                | ConfigOverlayState::InstalledPluginActions(_)
                | ConfigOverlayState::PluginInstallActions(_)
                | ConfigOverlayState::MarketplaceActions(_)
                | ConfigOverlayState::AddMarketplace(_)
                | ConfigOverlayState::McpDetails(_)
                | ConfigOverlayState::McpCallbackUrl(_)
                | ConfigOverlayState::McpElicitation(_)
                | ConfigOverlayState::McpAuthRedirect(_),
            )
            | None => None,
        }
    }

    pub fn session_rename_overlay_mut(&mut self) -> Option<&mut SessionRenameOverlayState> {
        match &mut self.overlay {
            Some(ConfigOverlayState::SessionRename(overlay)) => Some(overlay),
            Some(
                ConfigOverlayState::ModelAndEffort(_)
                | ConfigOverlayState::OutputStyle(_)
                | ConfigOverlayState::Language(_)
                | ConfigOverlayState::InstalledPluginActions(_)
                | ConfigOverlayState::PluginInstallActions(_)
                | ConfigOverlayState::MarketplaceActions(_)
                | ConfigOverlayState::AddMarketplace(_)
                | ConfigOverlayState::McpDetails(_)
                | ConfigOverlayState::McpCallbackUrl(_)
                | ConfigOverlayState::McpElicitation(_)
                | ConfigOverlayState::McpAuthRedirect(_),
            )
            | None => None,
        }
    }

    #[must_use]
    pub fn installed_plugin_actions_overlay(&self) -> Option<&InstalledPluginActionOverlayState> {
        match &self.overlay {
            Some(ConfigOverlayState::InstalledPluginActions(overlay)) => Some(overlay),
            Some(
                ConfigOverlayState::ModelAndEffort(_)
                | ConfigOverlayState::OutputStyle(_)
                | ConfigOverlayState::Language(_)
                | ConfigOverlayState::SessionRename(_)
                | ConfigOverlayState::PluginInstallActions(_)
                | ConfigOverlayState::MarketplaceActions(_)
                | ConfigOverlayState::AddMarketplace(_)
                | ConfigOverlayState::McpDetails(_)
                | ConfigOverlayState::McpCallbackUrl(_)
                | ConfigOverlayState::McpElicitation(_)
                | ConfigOverlayState::McpAuthRedirect(_),
            )
            | None => None,
        }
    }

    pub fn installed_plugin_actions_overlay_mut(
        &mut self,
    ) -> Option<&mut InstalledPluginActionOverlayState> {
        match &mut self.overlay {
            Some(ConfigOverlayState::InstalledPluginActions(overlay)) => Some(overlay),
            Some(
                ConfigOverlayState::ModelAndEffort(_)
                | ConfigOverlayState::OutputStyle(_)
                | ConfigOverlayState::Language(_)
                | ConfigOverlayState::SessionRename(_)
                | ConfigOverlayState::PluginInstallActions(_)
                | ConfigOverlayState::MarketplaceActions(_)
                | ConfigOverlayState::AddMarketplace(_)
                | ConfigOverlayState::McpDetails(_)
                | ConfigOverlayState::McpCallbackUrl(_)
                | ConfigOverlayState::McpElicitation(_)
                | ConfigOverlayState::McpAuthRedirect(_),
            )
            | None => None,
        }
    }

    #[must_use]
    pub fn plugin_install_overlay(&self) -> Option<&PluginInstallOverlayState> {
        match &self.overlay {
            Some(ConfigOverlayState::PluginInstallActions(overlay)) => Some(overlay),
            Some(
                ConfigOverlayState::ModelAndEffort(_)
                | ConfigOverlayState::OutputStyle(_)
                | ConfigOverlayState::Language(_)
                | ConfigOverlayState::SessionRename(_)
                | ConfigOverlayState::InstalledPluginActions(_)
                | ConfigOverlayState::MarketplaceActions(_)
                | ConfigOverlayState::AddMarketplace(_)
                | ConfigOverlayState::McpDetails(_)
                | ConfigOverlayState::McpCallbackUrl(_)
                | ConfigOverlayState::McpElicitation(_)
                | ConfigOverlayState::McpAuthRedirect(_),
            )
            | None => None,
        }
    }

    pub fn plugin_install_overlay_mut(&mut self) -> Option<&mut PluginInstallOverlayState> {
        match &mut self.overlay {
            Some(ConfigOverlayState::PluginInstallActions(overlay)) => Some(overlay),
            Some(
                ConfigOverlayState::ModelAndEffort(_)
                | ConfigOverlayState::OutputStyle(_)
                | ConfigOverlayState::Language(_)
                | ConfigOverlayState::SessionRename(_)
                | ConfigOverlayState::InstalledPluginActions(_)
                | ConfigOverlayState::MarketplaceActions(_)
                | ConfigOverlayState::AddMarketplace(_)
                | ConfigOverlayState::McpDetails(_)
                | ConfigOverlayState::McpCallbackUrl(_)
                | ConfigOverlayState::McpElicitation(_)
                | ConfigOverlayState::McpAuthRedirect(_),
            )
            | None => None,
        }
    }

    #[must_use]
    pub fn marketplace_actions_overlay(&self) -> Option<&MarketplaceActionsOverlayState> {
        match &self.overlay {
            Some(ConfigOverlayState::MarketplaceActions(overlay)) => Some(overlay),
            Some(
                ConfigOverlayState::ModelAndEffort(_)
                | ConfigOverlayState::OutputStyle(_)
                | ConfigOverlayState::Language(_)
                | ConfigOverlayState::SessionRename(_)
                | ConfigOverlayState::InstalledPluginActions(_)
                | ConfigOverlayState::PluginInstallActions(_)
                | ConfigOverlayState::AddMarketplace(_)
                | ConfigOverlayState::McpDetails(_)
                | ConfigOverlayState::McpCallbackUrl(_)
                | ConfigOverlayState::McpElicitation(_)
                | ConfigOverlayState::McpAuthRedirect(_),
            )
            | None => None,
        }
    }

    pub fn marketplace_actions_overlay_mut(
        &mut self,
    ) -> Option<&mut MarketplaceActionsOverlayState> {
        match &mut self.overlay {
            Some(ConfigOverlayState::MarketplaceActions(overlay)) => Some(overlay),
            Some(
                ConfigOverlayState::ModelAndEffort(_)
                | ConfigOverlayState::OutputStyle(_)
                | ConfigOverlayState::Language(_)
                | ConfigOverlayState::SessionRename(_)
                | ConfigOverlayState::InstalledPluginActions(_)
                | ConfigOverlayState::PluginInstallActions(_)
                | ConfigOverlayState::AddMarketplace(_)
                | ConfigOverlayState::McpDetails(_)
                | ConfigOverlayState::McpCallbackUrl(_)
                | ConfigOverlayState::McpElicitation(_)
                | ConfigOverlayState::McpAuthRedirect(_),
            )
            | None => None,
        }
    }

    #[must_use]
    pub fn add_marketplace_overlay(&self) -> Option<&AddMarketplaceOverlayState> {
        match &self.overlay {
            Some(ConfigOverlayState::AddMarketplace(overlay)) => Some(overlay),
            Some(
                ConfigOverlayState::ModelAndEffort(_)
                | ConfigOverlayState::OutputStyle(_)
                | ConfigOverlayState::Language(_)
                | ConfigOverlayState::SessionRename(_)
                | ConfigOverlayState::InstalledPluginActions(_)
                | ConfigOverlayState::PluginInstallActions(_)
                | ConfigOverlayState::MarketplaceActions(_)
                | ConfigOverlayState::McpDetails(_)
                | ConfigOverlayState::McpCallbackUrl(_)
                | ConfigOverlayState::McpElicitation(_)
                | ConfigOverlayState::McpAuthRedirect(_),
            )
            | None => None,
        }
    }

    pub fn add_marketplace_overlay_mut(&mut self) -> Option<&mut AddMarketplaceOverlayState> {
        match &mut self.overlay {
            Some(ConfigOverlayState::AddMarketplace(overlay)) => Some(overlay),
            Some(
                ConfigOverlayState::ModelAndEffort(_)
                | ConfigOverlayState::OutputStyle(_)
                | ConfigOverlayState::Language(_)
                | ConfigOverlayState::SessionRename(_)
                | ConfigOverlayState::InstalledPluginActions(_)
                | ConfigOverlayState::PluginInstallActions(_)
                | ConfigOverlayState::MarketplaceActions(_)
                | ConfigOverlayState::McpDetails(_)
                | ConfigOverlayState::McpCallbackUrl(_)
                | ConfigOverlayState::McpElicitation(_)
                | ConfigOverlayState::McpAuthRedirect(_),
            )
            | None => None,
        }
    }

    #[must_use]
    pub fn path_for(&self, file: SettingFile) -> Option<&PathBuf> {
        match file {
            SettingFile::Settings => self.settings_path.as_ref(),
            SettingFile::LocalSettings => self.local_settings_path.as_ref(),
            SettingFile::Preferences => self.preferences_path.as_ref(),
        }
    }

    #[must_use]
    pub fn document_for(&self, file: SettingFile) -> &Value {
        match file {
            SettingFile::Settings => &self.committed_settings_document,
            SettingFile::LocalSettings => &self.committed_local_settings_document,
            SettingFile::Preferences => &self.committed_preferences_document,
        }
    }

    pub fn committed_document_for_mut(&mut self, file: SettingFile) -> &mut Value {
        match file {
            SettingFile::Settings => &mut self.committed_settings_document,
            SettingFile::LocalSettings => &mut self.committed_local_settings_document,
            SettingFile::Preferences => &mut self.committed_preferences_document,
        }
    }

    fn apply_loaded(
        &mut self,
        loaded: store::LoadedSettingsDocuments,
        notice: Option<String>,
        preserve_status: bool,
    ) {
        self.settings_path = Some(loaded.paths.settings);
        self.local_settings_path = Some(loaded.paths.local_settings);
        self.preferences_path = Some(loaded.paths.preferences);
        self.committed_settings_document = loaded.settings_document;
        self.committed_local_settings_document = loaded.local_settings_document;
        self.committed_preferences_document = loaded.preferences_document;
        self.overlay = None;
        self.selected_setting_index =
            self.selected_setting_index.min(setting_specs().len().saturating_sub(1));
        self.settings_scroll_offset = self.settings_scroll_offset.min(self.selected_setting_index);
        self.mcp_selected_server_index = 0;
        if !preserve_status {
            self.status_message = notice;
            self.last_error = None;
        } else if let Some(notice) = notice {
            self.status_message = Some(notice);
        }
    }
}

#[must_use]
pub const fn setting_specs() -> &'static [SettingSpec] {
    &CONFIG_SETTINGS
}

#[must_use]
pub fn setting_spec(id: SettingId) -> &'static SettingSpec {
    &CONFIG_SETTINGS[id as usize]
}

#[must_use]
pub fn resolved_setting(app: &App, spec: &SettingSpec) -> ResolvedSetting {
    let document = app.config.document_for(spec.file);
    resolve_setting_document(document, spec.id, &app.available_models)
}

#[must_use]
pub fn setting_display_value(app: &App, spec: &SettingSpec, resolved: &ResolvedSetting) -> String {
    match (&resolved.value, spec.id) {
        (ResolvedSettingValue::Bool(value), _) => {
            if *value {
                "On".to_owned()
            } else {
                "Off".to_owned()
            }
        }
        (ResolvedSettingValue::Text(value), _) => {
            if value.is_empty() {
                "Not set".to_owned()
            } else {
                value.clone()
            }
        }
        (ResolvedSettingValue::Choice(ResolvedChoice::Automatic), SettingId::Model) => {
            DEFAULT_MODEL_LABEL.to_owned()
        }
        (ResolvedSettingValue::Choice(ResolvedChoice::Stored(value)), SettingId::Model) => {
            model_status_label(Some(value), app)
        }
        (
            ResolvedSettingValue::Choice(ResolvedChoice::Stored(value)),
            SettingId::ThinkingEffort,
        ) => effort_level_label(value).unwrap_or_else(|| value.clone()),
        (ResolvedSettingValue::Choice(ResolvedChoice::Stored(value)), _) => {
            option_label(spec, value).unwrap_or_else(|| value.clone())
        }
        _ => String::new(),
    }
}

#[must_use]
pub fn setting_invalid_hint(spec: &SettingSpec, validation: SettingValidation) -> Option<String> {
    match validation {
        SettingValidation::Valid => None,
        SettingValidation::InvalidValue => {
            Some(format!("invalid value, using {}", spec.fallback.short_label()))
        }
        SettingValidation::UnavailableOption if spec.id == SettingId::Model => {
            Some("model not advertised by current SDK session".to_owned())
        }
        SettingValidation::UnavailableOption => {
            Some(format!("value unavailable, using {}", spec.fallback.short_label()))
        }
    }
}

#[must_use]
pub fn setting_detail_options(app: &App, spec: &SettingSpec) -> Vec<String> {
    match spec.kind {
        SettingKind::Bool => vec!["Off".to_owned(), "On".to_owned()],
        SettingKind::Text => Vec::new(),
        SettingKind::Enum | SettingKind::DynamicEnum => match spec.options {
            SettingOptions::None => Vec::new(),
            SettingOptions::Static(options) => {
                options.iter().map(|option| option.label.to_owned()).collect()
            }
            SettingOptions::RuntimeCatalog(RuntimeCatalogKind::Models) => {
                if app.available_models.is_empty() {
                    vec![
                        DEFAULT_MODEL_LABEL.to_owned(),
                        "Connect to load available models".to_owned(),
                    ]
                } else {
                    model_overlay_options(app)
                        .into_iter()
                        .map(|option| option.display_name)
                        .collect()
                }
            }
            SettingOptions::RuntimeCatalog(RuntimeCatalogKind::PermissionModes) => {
                DEFAULT_PERMISSION_OPTIONS.iter().map(|option| option.label.to_owned()).collect()
            }
        },
    }
}

pub fn initialize_shared_state(app: &mut App) -> Result<(), String> {
    let loaded = store::load(app.settings_home_override.as_deref(), Some(project_root(app)))?;
    let notice = loaded.notice.clone();
    app.config.apply_loaded(loaded, notice, false);
    app.reconcile_runtime_from_persisted_settings_change();
    Ok(())
}

pub fn open(app: &mut App) -> Result<(), String> {
    if !app.is_project_trusted() {
        return Err("Project trust must be accepted before opening settings".to_owned());
    }

    let loaded = store::load(app.settings_home_override.as_deref(), Some(project_root(app)))?;
    let notice = loaded.notice.clone();
    app.config.apply_loaded(loaded, notice, false);
    app.reconcile_runtime_from_persisted_settings_change();
    view::set_active_view(app, ActiveView::Config);
    request_active_tab_side_effects(app);
    Ok(())
}

pub(crate) fn refresh_runtime_tabs_for_session_change(app: &mut App) {
    if app.active_view != ActiveView::Config {
        return;
    }
    request_status_snapshot_if_needed(app);
    if app.config.active_tab == ConfigTab::Usage {
        crate::app::usage::request_refresh_if_needed(app);
    }
    if app.config.active_tab == ConfigTab::Plugins {
        crate::app::plugins::request_inventory_refresh_if_needed(app);
    }
}

pub fn close(app: &mut App) {
    view::set_active_view(app, ActiveView::Chat);
}

pub(crate) fn activate_tab(app: &mut App, tab: ConfigTab) {
    app.config.active_tab = tab;
    app.config.status_message = None;
    app.config.last_error = None;
    request_active_tab_side_effects(app);
}

pub fn handle_key(app: &mut App, key: KeyEvent) {
    if is_ctrl_shortcut(key, 'q') || is_ctrl_shortcut(key, 'c') {
        app.should_quit = true;
        return;
    }

    if app.config.overlay.is_some() {
        edit::handle_overlay_key(app, key);
        return;
    }

    if app.config.active_tab == ConfigTab::Plugins && crate::app::plugins::handle_key(app, key) {
        return;
    }
    if mcp::handle_mcp_key(app, key) {
        return;
    }

    match (key.code, key.modifiers) {
        (KeyCode::Char(' '), KeyModifiers::NONE)
            if app.config.active_tab == ConfigTab::Settings =>
        {
            if let Some(spec) = app.config.selected_setting_spec() {
                edit::activate_setting(app, spec);
            }
        }
        (KeyCode::Left, KeyModifiers::NONE) if app.config.active_tab == ConfigTab::Settings => {
            if let Some(spec) = app.config.selected_setting_spec() {
                edit::step_setting(app, spec, -1);
            }
        }
        (KeyCode::Right, KeyModifiers::NONE) if app.config.active_tab == ConfigTab::Settings => {
            if let Some(spec) = app.config.selected_setting_spec() {
                edit::step_setting(app, spec, 1);
            }
        }
        (KeyCode::Char(ch), modifiers)
            if app.config.active_tab == ConfigTab::Status
                && matches!(ch, 'r' | 'R')
                && (modifiers.is_empty() || modifiers == KeyModifiers::SHIFT) =>
        {
            edit::open_session_rename_overlay(app);
        }
        (KeyCode::Char(ch), modifiers)
            if app.config.active_tab == ConfigTab::Status
                && matches!(ch, 'g' | 'G')
                && (modifiers.is_empty() || modifiers == KeyModifiers::SHIFT) =>
        {
            edit::generate_session_title(app);
        }
        (KeyCode::Char(ch), modifiers)
            if app.config.active_tab == ConfigTab::Usage
                && matches!(ch, 'r' | 'R')
                && (modifiers.is_empty() || modifiers == KeyModifiers::SHIFT) =>
        {
            crate::app::usage::request_refresh(app);
        }
        (KeyCode::Enter | KeyCode::Esc, KeyModifiers::NONE) => {
            close(app);
        }
        (KeyCode::BackTab, _) => {
            activate_tab(app, app.config.active_tab.prev());
        }
        (KeyCode::Tab, KeyModifiers::NONE) => {
            activate_tab(app, app.config.active_tab.next());
        }
        (KeyCode::Up, KeyModifiers::NONE) => {
            if app.config.active_tab == ConfigTab::Settings {
                app.config.selected_setting_index =
                    app.config.selected_setting_index.saturating_sub(1);
            }
        }
        (KeyCode::Down, KeyModifiers::NONE) => {
            if app.config.active_tab == ConfigTab::Settings {
                let last_index = setting_specs().len().saturating_sub(1);
                app.config.selected_setting_index =
                    (app.config.selected_setting_index + 1).min(last_index);
            }
        }
        _ => {}
    }
}

pub fn handle_paste(app: &mut App, text: &str) -> bool {
    if app.config.overlay.is_some() {
        return edit::handle_overlay_paste(app, text);
    }
    if app.config.active_tab == ConfigTab::Plugins {
        return crate::app::plugins::handle_paste(app, text);
    }
    false
}

fn request_active_tab_side_effects(app: &mut App) {
    request_status_snapshot_if_needed(app);
    mcp::refresh_mcp_snapshot_if_needed(app);
    if app.config.active_tab == ConfigTab::Usage {
        crate::app::usage::request_refresh_if_needed(app);
    }
    if app.config.active_tab == ConfigTab::Plugins {
        crate::app::plugins::request_inventory_refresh_if_needed(app);
    }
}

fn is_ctrl_shortcut(key: KeyEvent, ch: char) -> bool {
    matches!(key.code, KeyCode::Char(candidate) if candidate == ch)
        && key.modifiers == KeyModifiers::CONTROL
}

/// Send a `get_status_snapshot` command when the Status tab is active.
pub fn request_status_snapshot_if_needed(app: &App) {
    if app.config.active_tab != ConfigTab::Status {
        return;
    }
    let Some(conn) = app.conn.as_ref() else {
        return;
    };
    let Some(ref sid) = app.session_id else {
        return;
    };
    let _ = conn.get_status_snapshot(sid.to_string());
}

pub(crate) fn model_status_label(model: Option<&str>, app: &App) -> String {
    match model {
        None => DEFAULT_MODEL_LABEL.to_owned(),
        Some(model_id) => model_overlay_options(app)
            .into_iter()
            .find(|candidate| candidate.id == model_id)
            .map_or_else(
                || {
                    if model_id == DEFAULT_MODEL_ID {
                        DEFAULT_MODEL_LABEL.to_owned()
                    } else {
                        model_id.to_owned()
                    }
                },
                |candidate| candidate.display_name,
            ),
    }
}

fn effort_level_label(value: &str) -> Option<String> {
    EffortLevel::from_stored(value).map(|level| level.label().to_owned())
}

fn project_root(app: &App) -> &std::path::Path {
    std::path::Path::new(&app.cwd_raw)
}

fn option_label(spec: &SettingSpec, value: &str) -> Option<String> {
    match spec.options {
        SettingOptions::Static(options) => options
            .iter()
            .find(|option| option.stored == value)
            .map(|option| option.label.to_owned()),
        SettingOptions::RuntimeCatalog(RuntimeCatalogKind::PermissionModes) => {
            DEFAULT_PERMISSION_OPTIONS
                .iter()
                .find(|option| option.stored == value)
                .map(|option| option.label.to_owned())
        }
        _ => None,
    }
}

#[cfg(test)]
mod tests;
