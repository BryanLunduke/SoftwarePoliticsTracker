// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct ModeInfo {
    pub id: String,
    pub name: String,
    pub description: Option<String>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct ModeState {
    pub current_mode_id: String,
    pub current_mode_name: String,
    pub available_modes: Vec<ModeInfo>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct AvailableCommand {
    pub name: String,
    pub description: String,
    pub input_hint: Option<String>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct AvailableAgent {
    pub name: String,
    pub description: String,
    pub model: Option<String>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum EffortLevel {
    Low,
    Medium,
    High,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct AvailableModel {
    pub id: String,
    pub display_name: String,
    pub description: Option<String>,
    pub supports_effort: bool,
    #[serde(default)]
    pub supported_effort_levels: Vec<EffortLevel>,
    pub supports_adaptive_thinking: Option<bool>,
    pub supports_fast_mode: Option<bool>,
    pub supports_auto_mode: Option<bool>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum FastModeState {
    Off,
    Cooldown,
    On,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum RateLimitStatus {
    Allowed,
    AllowedWarning,
    Rejected,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct RateLimitUpdate {
    pub status: RateLimitStatus,
    pub resets_at: Option<f64>,
    pub utilization: Option<f64>,
    pub rate_limit_type: Option<String>,
    pub overage_status: Option<RateLimitStatus>,
    pub overage_resets_at: Option<f64>,
    pub overage_disabled_reason: Option<String>,
    pub is_using_overage: Option<bool>,
    pub surpassed_threshold: Option<f64>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum SessionStatus {
    Compacting,
    Idle,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum CompactionTrigger {
    Manual,
    Auto,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum ContentBlock {
    Text { text: String },
    Image { mime_type: Option<String>, uri: Option<String>, data: Option<String> },
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[allow(clippy::struct_field_names)]
pub struct ToolCall {
    pub tool_call_id: String,
    pub title: String,
    pub kind: String,
    pub status: String,
    pub content: Vec<ToolCallContent>,
    pub raw_input: Option<serde_json::Value>,
    pub raw_output: Option<String>,
    pub output_metadata: Option<ToolOutputMetadata>,
    pub locations: Vec<ToolLocation>,
    pub meta: Option<serde_json::Value>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct ToolCallUpdate {
    pub tool_call_id: String,
    pub fields: ToolCallUpdateFields,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize, Default)]
pub struct ToolCallUpdateFields {
    pub title: Option<String>,
    pub kind: Option<String>,
    pub status: Option<String>,
    pub content: Option<Vec<ToolCallContent>>,
    pub raw_input: Option<serde_json::Value>,
    pub raw_output: Option<String>,
    pub output_metadata: Option<ToolOutputMetadata>,
    pub locations: Option<Vec<ToolLocation>>,
    pub meta: Option<serde_json::Value>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct ToolLocation {
    pub path: String,
    pub line: Option<u64>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize, Default)]
pub struct ExitPlanModeOutputMetadata {
    pub is_ultraplan: Option<bool>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize, Default)]
pub struct TodoWriteOutputMetadata {
    pub verification_nudge_needed: Option<bool>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize, Default)]
pub struct BashOutputMetadata {
    pub assistant_auto_backgrounded: Option<bool>,
    pub token_saver_active: Option<bool>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize, Default)]
pub struct ToolOutputMetadata {
    pub bash: Option<BashOutputMetadata>,
    pub exit_plan_mode: Option<ExitPlanModeOutputMetadata>,
    pub todo_write: Option<TodoWriteOutputMetadata>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum ToolCallContent {
    Content {
        content: ContentBlock,
    },
    Diff {
        old_path: String,
        new_path: String,
        old: String,
        new: String,
        repository: Option<String>,
    },
    McpResource {
        uri: String,
        mime_type: Option<String>,
        text: Option<String>,
        blob_saved_to: Option<String>,
    },
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct PlanEntry {
    pub content: String,
    pub status: String,
    pub active_form: String,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum SessionUpdate {
    AgentMessageChunk {
        content: ContentBlock,
    },
    UserMessageChunk {
        content: ContentBlock,
    },
    AgentThoughtChunk {
        content: ContentBlock,
    },
    ToolCall {
        tool_call: ToolCall,
    },
    ToolCallUpdate {
        tool_call_update: ToolCallUpdate,
    },
    Plan {
        entries: Vec<PlanEntry>,
    },
    AvailableCommandsUpdate {
        commands: Vec<AvailableCommand>,
    },
    AvailableAgentsUpdate {
        agents: Vec<AvailableAgent>,
    },
    ModeStateUpdate {
        mode: ModeState,
    },
    CurrentModeUpdate {
        current_mode_id: String,
    },
    ConfigOptionUpdate {
        option_id: String,
        value: serde_json::Value,
    },
    FastModeUpdate {
        fast_mode_state: FastModeState,
    },
    RateLimitUpdate {
        status: RateLimitStatus,
        resets_at: Option<f64>,
        utilization: Option<f64>,
        rate_limit_type: Option<String>,
        overage_status: Option<RateLimitStatus>,
        overage_resets_at: Option<f64>,
        overage_disabled_reason: Option<String>,
        is_using_overage: Option<bool>,
        surpassed_threshold: Option<f64>,
    },
    SessionStatusUpdate {
        status: SessionStatus,
    },
    CompactionBoundary {
        trigger: CompactionTrigger,
        pre_tokens: u64,
    },
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct PermissionOption {
    pub option_id: String,
    pub name: String,
    pub description: Option<String>,
    pub kind: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct PermissionRequest {
    pub tool_call: ToolCall,
    pub options: Vec<PermissionOption>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct QuestionOption {
    pub option_id: String,
    pub label: String,
    pub description: Option<String>,
    pub preview: Option<String>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct QuestionPrompt {
    pub question: String,
    pub header: String,
    pub multi_select: bool,
    pub options: Vec<QuestionOption>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct QuestionRequest {
    pub tool_call: ToolCall,
    pub prompt: QuestionPrompt,
    pub question_index: u64,
    pub total_questions: u64,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct QuestionAnnotation {
    pub preview: Option<String>,
    pub notes: Option<String>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(tag = "outcome", rename_all = "snake_case")]
pub enum PermissionOutcome {
    Selected { option_id: String },
    Cancelled,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(tag = "outcome", rename_all = "snake_case")]
pub enum QuestionOutcome {
    Answered { selected_option_ids: Vec<String>, annotation: Option<QuestionAnnotation> },
    Cancelled,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum ElicitationMode {
    Form,
    Url,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum ElicitationAction {
    Accept,
    Decline,
    Cancel,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct ElicitationRequest {
    pub request_id: String,
    pub server_name: String,
    pub message: String,
    pub mode: ElicitationMode,
    pub url: Option<String>,
    pub elicitation_id: Option<String>,
    pub requested_schema: Option<serde_json::Value>,
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct ElicitationResponse {
    pub action: ElicitationAction,
    pub content: Option<serde_json::Value>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct McpAuthRedirect {
    pub server_name: String,
    pub auth_url: String,
    pub requires_user_action: bool,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct McpOperationError {
    pub server_name: Option<String>,
    pub operation: String,
    pub message: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct AuthMethod {
    pub id: String,
    pub name: String,
    pub description: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[allow(clippy::struct_excessive_bools)]
pub struct AgentCapabilities {
    pub prompt_image: bool,
    pub prompt_embedded_context: bool,
    pub supports_session_listing: bool,
    pub supports_resume_session: bool,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct InitializeResult {
    pub agent_name: String,
    pub agent_version: String,
    pub auth_methods: Vec<AuthMethod>,
    pub capabilities: AgentCapabilities,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct SessionListEntry {
    pub session_id: String,
    pub summary: String,
    pub last_modified_ms: u64,
    pub file_size_bytes: u64,
    pub cwd: Option<String>,
    pub git_branch: Option<String>,
    pub custom_title: Option<String>,
    pub first_prompt: Option<String>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct SessionInit {
    pub session_id: String,
    pub model_name: String,
    pub mode: Option<ModeState>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct PromptChunk {
    pub kind: String,
    pub value: serde_json::Value,
}

#[derive(Debug, Clone, Default, PartialEq, Eq, Serialize, Deserialize)]
pub struct AccountInfo {
    pub email: Option<String>,
    pub organization: Option<String>,
    pub subscription_type: Option<String>,
    pub token_source: Option<String>,
    pub api_key_source: Option<String>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub enum McpServerConnectionStatus {
    Connected,
    Failed,
    NeedsAuth,
    Pending,
    Disabled,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct McpServerInfo {
    pub name: String,
    pub version: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize, Default)]
pub struct McpToolAnnotations {
    pub read_only: Option<bool>,
    pub destructive: Option<bool>,
    pub open_world: Option<bool>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct McpTool {
    pub name: String,
    pub description: Option<String>,
    pub annotations: Option<McpToolAnnotations>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum McpServerConfig {
    Stdio {
        command: String,
        #[serde(default)]
        args: Vec<String>,
        #[serde(default)]
        env: BTreeMap<String, String>,
    },
    Sse {
        url: String,
        #[serde(default)]
        headers: BTreeMap<String, String>,
    },
    Http {
        url: String,
        #[serde(default)]
        headers: BTreeMap<String, String>,
    },
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum McpServerStatusConfig {
    Stdio {
        command: String,
        #[serde(default)]
        args: Vec<String>,
        #[serde(default)]
        env: BTreeMap<String, String>,
    },
    Sse {
        url: String,
        #[serde(default)]
        headers: BTreeMap<String, String>,
    },
    Http {
        url: String,
        #[serde(default)]
        headers: BTreeMap<String, String>,
    },
    Sdk {
        name: String,
    },
    #[serde(rename = "claudeai-proxy")]
    ClaudeaiProxy {
        url: String,
        id: String,
    },
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct McpServerStatus {
    pub name: String,
    pub status: McpServerConnectionStatus,
    pub server_info: Option<McpServerInfo>,
    pub error: Option<String>,
    pub config: Option<McpServerStatusConfig>,
    pub scope: Option<String>,
    #[serde(default)]
    pub tools: Vec<McpTool>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize, Default)]
pub struct McpSetServersResult {
    #[serde(default)]
    pub added: Vec<String>,
    #[serde(default)]
    pub removed: Vec<String>,
    #[serde(default)]
    pub errors: BTreeMap<String, String>,
}
