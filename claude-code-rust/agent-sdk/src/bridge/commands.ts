import type { PermissionMode } from "@anthropic-ai/claude-agent-sdk";
import type {
  BridgeCommand,
  BridgeCommandEnvelope,
  ElicitationAction,
  Json,
  McpServerConfig,
  ModeInfo,
  ModeState,
  PermissionOutcome,
  QuestionOutcome,
  SessionLaunchSettings,
} from "../types.js";

const MODE_NAMES: Record<PermissionMode, string> = {
  default: "Default",
  acceptEdits: "Accept Edits",
  bypassPermissions: "Bypass Permissions",
  plan: "Plan",
  dontAsk: "Don't Ask",
};

const MODE_OPTIONS: ModeInfo[] = [
  { id: "default", name: "Default", description: "Standard permission flow" },
  { id: "acceptEdits", name: "Accept Edits", description: "Auto-approve edit operations" },
  { id: "plan", name: "Plan", description: "No tool execution" },
  { id: "dontAsk", name: "Don't Ask", description: "Reject non-approved tools" },
  { id: "bypassPermissions", name: "Bypass Permissions", description: "Auto-approve all tools" },
];

function asRecord(value: unknown, context: string): Record<string, unknown> {
  if (!value || typeof value !== "object" || Array.isArray(value)) {
    throw new Error(`${context} must be an object`);
  }
  return value as Record<string, unknown>;
}

function expectString(record: Record<string, unknown>, key: string, context: string): string {
  const value = record[key];
  if (typeof value !== "string") {
    throw new Error(`${context}.${key} must be a string`);
  }
  return value;
}

function optionalString(
  record: Record<string, unknown>,
  key: string,
  context: string,
): string | undefined {
  const value = record[key];
  if (value === undefined || value === null) {
    return undefined;
  }
  if (typeof value !== "string") {
    throw new Error(`${context}.${key} must be a string when provided`);
  }
  return value;
}

function optionalMetadata(record: Record<string, unknown>, key: string): Record<string, Json> {
  const value = record[key];
  if (value === undefined || value === null) {
    return {};
  }
  return asRecord(value, `${key} metadata`) as Record<string, Json>;
}

function optionalLaunchSettings(
  record: Record<string, unknown>,
  key: string,
  context: string,
): SessionLaunchSettings {
  const value = record[key];
  if (value === undefined || value === null) {
    return {};
  }
  const parsed = asRecord(value, `${context}.${key}`);
  const language = optionalString(parsed, "language", `${context}.${key}`);
  const settings = optionalJsonObject(parsed, "settings", `${context}.${key}`);
  const agentProgressSummaries = optionalBoolean(
    parsed,
    "agent_progress_summaries",
    `${context}.${key}`,
  );
  return {
    ...(language ? { language } : {}),
    ...(settings ? { settings } : {}),
    ...(agentProgressSummaries !== undefined
      ? { agent_progress_summaries: agentProgressSummaries }
      : {}),
  };
}

function optionalBoolean(
  record: Record<string, unknown>,
  key: string,
  context: string,
): boolean | undefined {
  const value = record[key];
  if (value === undefined || value === null) {
    return undefined;
  }
  if (typeof value !== "boolean") {
    throw new Error(`${context}.${key} must be a boolean when provided`);
  }
  return value;
}

function optionalJsonObject(
  record: Record<string, unknown>,
  key: string,
  context: string,
): { [key: string]: Json } | undefined {
  const value = record[key];
  if (value === undefined || value === null) {
    return undefined;
  }
  if (!value || typeof value !== "object" || Array.isArray(value)) {
    throw new Error(`${context}.${key} must be an object when provided`);
  }
  return value as { [key: string]: Json };
}

function expectElicitationAction(
  record: Record<string, unknown>,
  key: string,
  context: string,
): ElicitationAction {
  const value = expectString(record, key, context);
  if (value === "accept" || value === "decline" || value === "cancel") {
    return value;
  }
  throw new Error(`${context}.${key} must be one of accept, decline, cancel`);
}

function parsePromptChunks(
  record: Record<string, unknown>,
  context: string,
): Array<{ kind: string; value: Json }> {
  const rawChunks = record.chunks;
  if (!Array.isArray(rawChunks)) {
    throw new Error(`${context}.chunks must be an array`);
  }
  return rawChunks.map((chunk, index) => {
    const parsed = asRecord(chunk, `${context}.chunks[${index}]`);
    const kind = expectString(parsed, "kind", `${context}.chunks[${index}]`);
    return { kind, value: (parsed.value ?? null) as Json };
  });
}

function expectBoolean(
  record: Record<string, unknown>,
  key: string,
  context: string,
): boolean {
  const value = record[key];
  if (typeof value !== "boolean") {
    throw new Error(`${context}.${key} must be a boolean`);
  }
  return value;
}

function parseMcpServerConfig(
  value: unknown,
  context: string,
): McpServerConfig {
  const record = asRecord(value, context);
  const type = expectString(record, "type", context);
  switch (type) {
    case "stdio":
      return {
        type,
        command: expectString(record, "command", context),
        ...(record.args === undefined ? {} : { args: expectStringArray(record, "args", context) }),
        ...(record.env === undefined ? {} : { env: expectStringMap(record, "env", context) }),
      };
    case "sse":
    case "http":
      return {
        type,
        url: expectString(record, "url", context),
        ...(record.headers === undefined
          ? {}
          : { headers: expectStringMap(record, "headers", context) }),
      };
    default:
      throw new Error(`${context}.type must be one of stdio, sse, http`);
  }
}

function parseMcpServersRecord(
  value: unknown,
  context: string,
): Record<string, McpServerConfig> {
  const record = asRecord(value, context);
  return Object.fromEntries(
    Object.entries(record).map(([key, entry]) => [key, parseMcpServerConfig(entry, `${context}.${key}`)]),
  );
}

export function parseCommandEnvelope(line: string): { requestId?: string; command: BridgeCommand } {
  const raw = asRecord(JSON.parse(line) as BridgeCommandEnvelope, "command envelope");
  const requestId = typeof raw.request_id === "string" ? raw.request_id : undefined;
  const commandName = expectString(raw, "command", "command envelope");

  const command: BridgeCommand = (() => {
    switch (commandName) {
      case "initialize":
        return {
          command: "initialize",
          cwd: expectString(raw, "cwd", "initialize"),
          metadata: optionalMetadata(raw, "metadata"),
        };
      case "create_session":
        return {
          command: "create_session",
          cwd: expectString(raw, "cwd", "create_session"),
          resume: optionalString(raw, "resume", "create_session"),
          launch_settings: optionalLaunchSettings(raw, "launch_settings", "create_session"),
          metadata: optionalMetadata(raw, "metadata"),
        };
      case "resume_session":
        return {
          command: "resume_session",
          session_id: expectString(raw, "session_id", "resume_session"),
          launch_settings: optionalLaunchSettings(raw, "launch_settings", "resume_session"),
          metadata: optionalMetadata(raw, "metadata"),
        };
      case "new_session":
        return {
          command: "new_session",
          cwd: expectString(raw, "cwd", "new_session"),
          launch_settings: optionalLaunchSettings(raw, "launch_settings", "new_session"),
        };
      case "prompt":
        return {
          command: "prompt",
          session_id: expectString(raw, "session_id", "prompt"),
          chunks: parsePromptChunks(raw, "prompt"),
        };
      case "cancel_turn":
        return {
          command: "cancel_turn",
          session_id: expectString(raw, "session_id", "cancel_turn"),
        };
      case "set_model":
        return {
          command: "set_model",
          session_id: expectString(raw, "session_id", "set_model"),
          model: expectString(raw, "model", "set_model"),
        };
      case "set_mode":
        return {
          command: "set_mode",
          session_id: expectString(raw, "session_id", "set_mode"),
          mode: expectString(raw, "mode", "set_mode"),
        };
      case "generate_session_title":
        return {
          command: "generate_session_title",
          session_id: expectString(raw, "session_id", "generate_session_title"),
          description: expectString(raw, "description", "generate_session_title"),
        };
      case "rename_session":
        return {
          command: "rename_session",
          session_id: expectString(raw, "session_id", "rename_session"),
          title: expectString(raw, "title", "rename_session"),
        };
      case "get_status_snapshot":
        return {
          command: "get_status_snapshot",
          session_id: expectString(raw, "session_id", "get_status_snapshot"),
        };
      case "mcp_status":
      case "get_mcp_snapshot":
        return {
          command: "mcp_status",
          session_id: expectString(raw, "session_id", commandName),
        };
      case "mcp_reconnect":
        return {
          command: "mcp_reconnect",
          session_id: expectString(raw, "session_id", "mcp_reconnect"),
          server_name: expectString(raw, "server_name", "mcp_reconnect"),
        };
      case "mcp_toggle":
        return {
          command: "mcp_toggle",
          session_id: expectString(raw, "session_id", "mcp_toggle"),
          server_name: expectString(raw, "server_name", "mcp_toggle"),
          enabled: expectBoolean(raw, "enabled", "mcp_toggle"),
        };
      case "mcp_set_servers":
        return {
          command: "mcp_set_servers",
          session_id: expectString(raw, "session_id", "mcp_set_servers"),
          servers: parseMcpServersRecord(raw.servers ?? {}, "mcp_set_servers.servers"),
        };
      case "permission_response": {
        const outcome = asRecord(raw.outcome, "permission_response.outcome");
        const outcomeType = expectString(outcome, "outcome", "permission_response.outcome");
        if (outcomeType !== "selected" && outcomeType !== "cancelled") {
          throw new Error("permission_response.outcome.outcome must be 'selected' or 'cancelled'");
        }
        const parsedOutcome: PermissionOutcome =
          outcomeType === "selected"
            ? {
                outcome: "selected",
                option_id: expectString(outcome, "option_id", "permission_response.outcome"),
              }
            : { outcome: "cancelled" };
        return {
          command: "permission_response",
          session_id: expectString(raw, "session_id", "permission_response"),
          tool_call_id: expectString(raw, "tool_call_id", "permission_response"),
          outcome: parsedOutcome,
        };
      }
      case "question_response": {
        const outcome = asRecord(raw.outcome, "question_response.outcome");
        const outcomeType = expectString(outcome, "outcome", "question_response.outcome");
        if (outcomeType !== "answered" && outcomeType !== "cancelled") {
          throw new Error("question_response.outcome.outcome must be 'answered' or 'cancelled'");
        }
        const parsedOutcome: QuestionOutcome =
          outcomeType === "answered"
            ? {
                outcome: "answered",
                selected_option_ids: expectStringArray(
                  outcome,
                  "selected_option_ids",
                  "question_response.outcome",
                ),
                ...(outcome.annotation === undefined || outcome.annotation === null
                  ? {}
                  : { annotation: parseQuestionAnnotation(outcome.annotation) }),
              }
            : { outcome: "cancelled" };
        return {
          command: "question_response",
          session_id: expectString(raw, "session_id", "question_response"),
          tool_call_id: expectString(raw, "tool_call_id", "question_response"),
          outcome: parsedOutcome,
        };
      }
      case "elicitation_response":
        return {
          command: "elicitation_response",
          session_id: expectString(raw, "session_id", "elicitation_response"),
          elicitation_request_id: expectString(
            raw,
            "elicitation_request_id",
            "elicitation_response",
          ),
          action: expectElicitationAction(raw, "action", "elicitation_response"),
          ...(optionalJsonObject(raw, "content", "elicitation_response")
            ? { content: optionalJsonObject(raw, "content", "elicitation_response") }
            : {}),
        };
      case "mcp_authenticate":
        return {
          command: "mcp_authenticate",
          session_id: expectString(raw, "session_id", "mcp_authenticate"),
          server_name: expectString(raw, "server_name", "mcp_authenticate"),
        };
      case "mcp_clear_auth":
        return {
          command: "mcp_clear_auth",
          session_id: expectString(raw, "session_id", "mcp_clear_auth"),
          server_name: expectString(raw, "server_name", "mcp_clear_auth"),
        };
      case "mcp_oauth_callback_url":
        return {
          command: "mcp_oauth_callback_url",
          session_id: expectString(raw, "session_id", "mcp_oauth_callback_url"),
          server_name: expectString(raw, "server_name", "mcp_oauth_callback_url"),
          callback_url: expectString(raw, "callback_url", "mcp_oauth_callback_url"),
        };
      case "shutdown":
        return { command: "shutdown" };
      default:
        throw new Error(`unsupported command: ${commandName}`);
    }
  })();

  return { requestId, command };
}

function expectStringArray(
  record: Record<string, unknown>,
  key: string,
  context: string,
): string[] {
  const value = record[key];
  if (!Array.isArray(value)) {
    throw new Error(`${context}.${key} must be an array`);
  }
  return value.map((entry, index) => {
    if (typeof entry !== "string") {
      throw new Error(`${context}.${key}[${index}] must be a string`);
    }
    return entry;
  });
}

function expectStringMap(
  record: Record<string, unknown>,
  key: string,
  context: string,
): Record<string, string> {
  const value = record[key];
  const parsed = asRecord(value, `${context}.${key}`);
  return Object.fromEntries(
    Object.entries(parsed).map(([entryKey, entryValue]) => {
      if (typeof entryValue !== "string") {
        throw new Error(`${context}.${key}.${entryKey} must be a string`);
      }
      return [entryKey, entryValue];
    }),
  );
}

function parseQuestionAnnotation(value: unknown): { preview?: string; notes?: string } {
  const record = asRecord(value, "question_response.outcome.annotation");
  const preview = optionalString(record, "preview", "question_response.outcome.annotation");
  const notes = optionalString(record, "notes", "question_response.outcome.annotation");
  return {
    ...(preview !== undefined ? { preview } : {}),
    ...(notes !== undefined ? { notes } : {}),
  };
}

export function toPermissionMode(mode: string): PermissionMode | null {
  if (
    mode === "default" ||
    mode === "acceptEdits" ||
    mode === "bypassPermissions" ||
    mode === "plan" ||
    mode === "dontAsk"
  ) {
    return mode;
  }
  return null;
}

export function buildModeState(mode: PermissionMode): ModeState {
  return {
    current_mode_id: mode,
    current_mode_name: MODE_NAMES[mode],
    available_modes: MODE_OPTIONS,
  };
}

