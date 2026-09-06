export type Json = null | boolean | number | string | Json[] | { [key: string]: Json };

export interface PromptChunk {
  kind: string;
  value: Json;
}

export interface ModeInfo {
  id: string;
  name: string;
  description?: string;
}

export interface ModeState {
  current_mode_id: string;
  current_mode_name: string;
  available_modes: ModeInfo[];
}

export interface AvailableCommand {
  name: string;
  description: string;
  input_hint?: string;
}

export interface AvailableAgent {
  name: string;
  description: string;
  model?: string;
}

export type EffortLevel = "low" | "medium" | "high";

export interface AvailableModel {
  id: string;
  display_name: string;
  description?: string;
  supports_effort: boolean;
  supported_effort_levels: EffortLevel[];
  supports_adaptive_thinking?: boolean;
  supports_fast_mode?: boolean;
  supports_auto_mode?: boolean;
}

export type FastModeState = "off" | "cooldown" | "on";
export type RateLimitStatus = "allowed" | "allowed_warning" | "rejected";

export interface RateLimitUpdate {
  status: RateLimitStatus;
  resets_at?: number;
  utilization?: number;
  rate_limit_type?: string;
  overage_status?: RateLimitStatus;
  overage_resets_at?: number;
  overage_disabled_reason?: string;
  is_using_overage?: boolean;
  surpassed_threshold?: number;
}

export type ContentBlock =
  | { type: "text"; text: string }
  | { type: "image"; mime_type?: string; uri?: string; data?: string };

export type ToolCallContent =
  | { type: "content"; content: ContentBlock }
  | {
      type: "diff";
      old_path: string;
      new_path: string;
      old: string;
      new: string;
      repository?: string;
    }
  | {
      type: "mcp_resource";
      uri: string;
      mime_type?: string;
      text?: string;
      blob_saved_to?: string;
    };

export interface ExitPlanModeOutputMetadata {
  is_ultraplan?: boolean;
}

export interface TodoWriteOutputMetadata {
  verification_nudge_needed?: boolean;
}

export interface BashOutputMetadata {
  assistant_auto_backgrounded?: boolean;
  token_saver_active?: boolean;
}

export interface ToolOutputMetadata {
  bash?: BashOutputMetadata;
  exit_plan_mode?: ExitPlanModeOutputMetadata;
  todo_write?: TodoWriteOutputMetadata;
}

export interface ToolLocation {
  path: string;
  line?: number;
}

export interface ToolCall {
  tool_call_id: string;
  title: string;
  kind: string;
  status: string;
  content: ToolCallContent[];
  raw_input?: Json;
  raw_output?: string;
  output_metadata?: ToolOutputMetadata;
  locations: ToolLocation[];
  meta?: Json;
}

export interface ToolCallUpdateFields {
  title?: string;
  kind?: string;
  status?: string;
  content?: ToolCallContent[];
  raw_input?: Json;
  raw_output?: string;
  output_metadata?: ToolOutputMetadata;
  locations?: ToolLocation[];
  meta?: Json;
}

export interface ToolCallUpdate {
  tool_call_id: string;
  fields: ToolCallUpdateFields;
}

export interface PlanEntry {
  content: string;
  status: string;
  active_form: string;
}

export type SessionUpdate =
  | { type: "agent_message_chunk"; content: ContentBlock }
  | { type: "user_message_chunk"; content: ContentBlock }
  | { type: "agent_thought_chunk"; content: ContentBlock }
  | { type: "tool_call"; tool_call: ToolCall }
  | { type: "tool_call_update"; tool_call_update: ToolCallUpdate }
  | { type: "plan"; entries: PlanEntry[] }
  | { type: "available_commands_update"; commands: AvailableCommand[] }
  | { type: "available_agents_update"; agents: AvailableAgent[] }
  | { type: "mode_state_update"; mode: ModeState }
  | { type: "current_mode_update"; current_mode_id: string }
  | { type: "config_option_update"; option_id: string; value: Json }
  | { type: "fast_mode_update"; fast_mode_state: FastModeState }
  | ({ type: "rate_limit_update" } & RateLimitUpdate)
  | { type: "session_status_update"; status: "compacting" | "idle" }
  | { type: "compaction_boundary"; trigger: "manual" | "auto"; pre_tokens: number };

export interface PermissionOption {
  option_id: string;
  name: string;
  description?: string;
  kind: string;
}

export interface PermissionRequest {
  tool_call: ToolCall;
  options: PermissionOption[];
}

export type ElicitationMode = "form" | "url";

export type ElicitationAction = "accept" | "decline" | "cancel";

export interface QuestionOption {
  option_id: string;
  label: string;
  description?: string;
  preview?: string;
}

export interface QuestionPrompt {
  question: string;
  header: string;
  multi_select: boolean;
  options: QuestionOption[];
}

export interface QuestionRequest {
  tool_call: ToolCall;
  prompt: QuestionPrompt;
  question_index: number;
  total_questions: number;
}

export interface QuestionAnnotation {
  preview?: string;
  notes?: string;
}

export interface ElicitationRequest {
  request_id: string;
  server_name: string;
  message: string;
  mode: ElicitationMode;
  url?: string;
  elicitation_id?: string;
  requested_schema?: Record<string, Json>;
}

export interface ElicitationComplete {
  elicitation_id: string;
  server_name?: string;
}

export interface McpAuthRedirect {
  server_name: string;
  auth_url: string;
  requires_user_action: boolean;
}

export interface McpOperationError {
  server_name?: string;
  operation: string;
  message: string;
}

export type PermissionOutcome =
  | { outcome: "selected"; option_id: string }
  | { outcome: "cancelled" };

export type QuestionOutcome =
  | {
      outcome: "answered";
      selected_option_ids: string[];
      annotation?: QuestionAnnotation;
    }
  | { outcome: "cancelled" };

export interface SessionListEntry {
  session_id: string;
  summary: string;
  last_modified_ms: number;
  file_size_bytes: number;
  cwd?: string;
  git_branch?: string;
  custom_title?: string;
  first_prompt?: string;
}

export interface AccountInfo {
  email?: string;
  organization?: string;
  subscription_type?: string;
  token_source?: string;
  api_key_source?: string;
}

export type McpServerConnectionStatus =
  | "connected"
  | "failed"
  | "needs-auth"
  | "pending"
  | "disabled";

export interface McpServerInfo {
  name: string;
  version: string;
}

export interface McpToolAnnotations {
  read_only?: boolean;
  destructive?: boolean;
  open_world?: boolean;
}

export interface McpTool {
  name: string;
  description?: string;
  annotations?: McpToolAnnotations;
}

export type McpServerConfig =
  | {
      type: "stdio";
      command: string;
      args?: string[];
      env?: Record<string, string>;
    }
  | {
      type: "sse";
      url: string;
      headers?: Record<string, string>;
    }
  | {
      type: "http";
      url: string;
      headers?: Record<string, string>;
    };

export type McpServerStatusConfig =
  | McpServerConfig
  | {
      type: "sdk";
      name: string;
    }
  | {
      type: "claudeai-proxy";
      url: string;
      id: string;
    };

export interface McpServerStatus {
  name: string;
  status: McpServerConnectionStatus;
  server_info?: McpServerInfo;
  error?: string;
  config?: McpServerStatusConfig;
  scope?: string;
  tools: McpTool[];
}

export interface McpSetServersResult {
  added: string[];
  removed: string[];
  errors: Record<string, string>;
}

export interface SessionLaunchSettings {
  language?: string;
  settings?: { [key: string]: Json };
  agent_progress_summaries?: boolean;
}

export interface BridgeCommandEnvelope {
  request_id?: string;
  command: string;
  [key: string]: unknown;
}

export type BridgeCommand =
  | {
      command: "initialize";
      cwd: string;
      metadata?: Record<string, Json>;
    }
  | {
      command: "create_session";
      cwd: string;
      resume?: string;
      launch_settings: SessionLaunchSettings;
      metadata?: Record<string, Json>;
    }
  | {
      command: "resume_session";
      session_id: string;
      launch_settings: SessionLaunchSettings;
      metadata?: Record<string, Json>;
    }
  | {
      command: "prompt";
      session_id: string;
      chunks: PromptChunk[];
    }
  | {
      command: "cancel_turn";
      session_id: string;
    }
  | {
      command: "set_model";
      session_id: string;
      model: string;
    }
  | {
      command: "set_mode";
      session_id: string;
      mode: string;
    }
  | {
      command: "generate_session_title";
      session_id: string;
      description: string;
    }
  | {
      command: "rename_session";
      session_id: string;
      title: string;
    }
  | {
      command: "new_session";
      cwd: string;
      launch_settings: SessionLaunchSettings;
    }
  | {
      command: "permission_response";
      session_id: string;
      tool_call_id: string;
      outcome: PermissionOutcome;
    }
  | {
      command: "question_response";
      session_id: string;
      tool_call_id: string;
      outcome: QuestionOutcome;
    }
  | {
      command: "elicitation_response";
      session_id: string;
      elicitation_request_id: string;
      action: ElicitationAction;
      content?: Record<string, Json>;
    }
  | {
      command: "get_status_snapshot";
      session_id: string;
    }
  | {
      command: "mcp_status";
      session_id: string;
    }
  | {
      command: "mcp_reconnect";
      session_id: string;
      server_name: string;
    }
  | {
      command: "mcp_toggle";
      session_id: string;
      server_name: string;
      enabled: boolean;
    }
  | {
      command: "mcp_set_servers";
      session_id: string;
      servers: Record<string, McpServerConfig>;
    }
  | {
      command: "mcp_authenticate";
      session_id: string;
      server_name: string;
    }
  | {
      command: "mcp_clear_auth";
      session_id: string;
      server_name: string;
    }
  | {
      command: "mcp_oauth_callback_url";
      session_id: string;
      server_name: string;
      callback_url: string;
    }
  | {
      command: "shutdown";
    };

export interface BridgeEventEnvelope {
  request_id?: string;
  event: string;
  [key: string]: unknown;
}

export interface InitializeResult {
  agent_name: string;
  agent_version: string;
  auth_methods: Array<{ id: string; name: string; description: string }>;
  capabilities: {
    prompt_image: boolean;
    prompt_embedded_context: boolean;
    supports_session_listing: boolean;
    supports_resume_session: boolean;
  };
}

export type TurnErrorKind = "plan_limit" | "auth_required" | "internal" | "other";

export type BridgeEvent =
  | {
      event: "connected";
      session_id: string;
      cwd: string;
      model_name: string;
      available_models: AvailableModel[];
      mode: ModeState | null;
      history_updates?: SessionUpdate[];
    }
  | { event: "auth_required"; method_name: string; method_description: string }
  | { event: "connection_failed"; message: string }
  | { event: "session_update"; session_id: string; update: SessionUpdate }
  | { event: "permission_request"; session_id: string; request: PermissionRequest }
  | { event: "question_request"; session_id: string; request: QuestionRequest }
  | { event: "elicitation_request"; session_id: string; request: ElicitationRequest }
  | { event: "elicitation_complete"; session_id: string; completion: ElicitationComplete }
  | { event: "mcp_auth_redirect"; session_id: string; redirect: McpAuthRedirect }
  | { event: "mcp_operation_error"; session_id: string; error: McpOperationError }
  | { event: "turn_complete"; session_id: string }
  | {
      event: "turn_error";
      session_id: string;
      message: string;
      error_kind?: TurnErrorKind;
      sdk_result_subtype?: string;
      assistant_error?: string;
    }
  | { event: "slash_error"; session_id: string; message: string }
  | {
      event: "session_replaced";
      session_id: string;
      cwd: string;
      model_name: string;
      available_models: AvailableModel[];
      mode: ModeState | null;
      history_updates?: SessionUpdate[];
    }
  | { event: "initialized"; result: InitializeResult }
  | { event: "sessions_listed"; sessions: SessionListEntry[] }
  | { event: "status_snapshot"; session_id: string; account: AccountInfo }
  | {
      event: "mcp_snapshot";
      session_id: string;
      servers: McpServerStatus[];
      error?: string;
    };
