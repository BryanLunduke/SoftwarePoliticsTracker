import { randomUUID } from "node:crypto";
import { spawn as spawnChild } from "node:child_process";
import fs from "node:fs";
import {
  getSessionMessages,
  listSessions,
  query,
  type CanUseTool,
  type ModelInfo,
  type PermissionMode,
  type PermissionResult,
  type PermissionUpdate,
  type Query,
  type SDKUserMessage,
  type SettingSource,
} from "@anthropic-ai/claude-agent-sdk";
import type {
  AvailableCommand,
  AvailableModel,
  BridgeCommand,
  ElicitationAction,
  ElicitationRequest,
  FastModeState,
  Json,
  PermissionOutcome,
  PermissionRequest,
  QuestionOutcome,
  SessionLaunchSettings,
  SessionUpdate,
  ToolCall,
} from "../types.js";
import { AsyncQueue, logPermissionDebug } from "./shared.js";
import {
  formatPermissionUpdates,
  permissionOptionsFromSuggestions,
  permissionResultFromOutcome,
} from "./permissions.js";
import { mapSessionMessagesToUpdates } from "./history.js";
import {
  writeEvent,
  failConnection,
  slashError,
  emitSessionUpdate,
  emitConnectEvent,
  emitSessionsList,
  refreshSessionsList,
} from "./events.js";
import {
  ensureToolCallVisible,
  setToolCallStatus,
} from "./tool_calls.js";
import {
  requestExitPlanModeApproval,
  requestAskUserQuestionAnswers,
  EXIT_PLAN_MODE_TOOL_NAME,
  ASK_USER_QUESTION_TOOL_NAME,
} from "./user_interaction.js";
import { mapAvailableAgents, emitAvailableAgentsIfChanged, refreshAvailableAgents } from "./agents.js";
import { emitAuthRequired, emitFastModeUpdateIfChanged } from "./error_classification.js";

export type ConnectEventKind = "connected" | "session_replaced";

export type PendingPermission = {
  resolve?: (result: PermissionResult) => void;
  onOutcome?: (outcome: PermissionOutcome) => void;
  toolName: string;
  inputData: Record<string, unknown>;
  suggestions?: PermissionUpdate[];
};

export type PendingQuestion = {
  onOutcome: (outcome: QuestionOutcome) => void;
  toolName: string;
  inputData: Record<string, unknown>;
};

export type PendingElicitation = {
  resolve: (result: {
    action: ElicitationAction;
    content?: Record<string, unknown>;
  }) => void;
  serverName: string;
  elicitationId?: string;
};

export type SessionState = {
  sessionId: string;
  cwd: string;
  model: string;
  availableModels: AvailableModel[];
  mode: PermissionMode | null;
  fastModeState: FastModeState;
  query: Query;
  input: AsyncQueue<SDKUserMessage>;
  connected: boolean;
  connectEvent: ConnectEventKind;
  connectRequestId?: string;
  toolCalls: Map<string, ToolCall>;
  taskToolUseIds: Map<string, string>;
  pendingPermissions: Map<string, PendingPermission>;
  pendingQuestions: Map<string, PendingQuestion>;
  pendingElicitations: Map<string, PendingElicitation>;
  mcpStatusRevalidatedAt: Map<string, number>;
  authHintSent: boolean;
  lastAvailableAgentsSignature?: string;
  lastAssistantError?: string;
  sessionsToCloseAfterConnect?: SessionState[];
  resumeUpdates?: SessionUpdate[];
};

export const sessions = new Map<string, SessionState>();
const DEFAULT_SETTING_SOURCES: SettingSource[] = ["user", "project", "local"];
const DEFAULT_MODEL_NAME = "default";
const DEFAULT_PERMISSION_MODE: PermissionMode = "default";

function settingsObjectFromLaunchSettings(
  launchSettings: SessionLaunchSettings,
): Record<string, unknown> | undefined {
  return launchSettings.settings;
}

export function sessionById(sessionId: string): SessionState | null {
  return sessions.get(sessionId) ?? null;
}

export function updateSessionId(session: SessionState, newSessionId: string): void {
  if (session.sessionId === newSessionId) {
    return;
  }
  sessions.delete(session.sessionId);
  session.sessionId = newSessionId;
  sessions.set(newSessionId, session);
}

export async function closeSession(session: SessionState): Promise<void> {
  session.input.close();
  session.query.close();
  for (const pending of session.pendingPermissions.values()) {
    pending.resolve?.({ behavior: "deny", message: "Session closed" });
    pending.onOutcome?.({ outcome: "cancelled" });
  }
  session.pendingPermissions.clear();
  for (const pending of session.pendingQuestions.values()) {
    pending.onOutcome({ outcome: "cancelled" });
  }
  session.pendingQuestions.clear();
  for (const pending of session.pendingElicitations.values()) {
    pending.resolve({ action: "cancel" });
  }
  session.pendingElicitations.clear();
}

export async function closeAllSessions(): Promise<void> {
  const active = Array.from(sessions.values());
  sessions.clear();
  await Promise.all(active.map((session) => closeSession(session)));
}

export async function createSession(params: {
  cwd: string;
  resume?: string;
  launchSettings: SessionLaunchSettings;
  connectEvent: ConnectEventKind;
  requestId?: string;
  sessionsToCloseAfterConnect?: SessionState[];
  resumeUpdates?: SessionUpdate[];
}): Promise<void> {
  const input = new AsyncQueue<SDKUserMessage>();
  const provisionalSessionId = params.resume ?? randomUUID();
  const initialModel = initialSessionModel(params.launchSettings);
  const initialMode = initialSessionMode(params.launchSettings);

  let session!: SessionState;
  const canUseTool: CanUseTool = async (toolName, inputData, options) => {
    const toolUseId = options.toolUseID;
    if (toolName === EXIT_PLAN_MODE_TOOL_NAME) {
      const existing = ensureToolCallVisible(session, toolUseId, toolName, inputData);
      return await requestExitPlanModeApproval(session, toolUseId, inputData, existing);
    }
    logPermissionDebug(
      `request tool_use_id=${toolUseId} tool=${toolName} blocked_path=${options.blockedPath ?? "<none>"} ` +
        `decision_reason=${options.decisionReason ?? "<none>"} suggestions=${formatPermissionUpdates(options.suggestions)}`,
    );
    const existing = ensureToolCallVisible(session, toolUseId, toolName, inputData);

    if (toolName === ASK_USER_QUESTION_TOOL_NAME) {
      return await requestAskUserQuestionAnswers(
        session,
        toolUseId,
        inputData,
        existing,
      );
    }

    const request: PermissionRequest = {
      tool_call: existing,
      options: permissionOptionsFromSuggestions(options.suggestions),
    };
    writeEvent({ event: "permission_request", session_id: session.sessionId, request });

    return await new Promise<PermissionResult>((resolve) => {
      session.pendingPermissions.set(toolUseId, {
        resolve,
        toolName,
        inputData: inputData,
        suggestions: options.suggestions,
      });
    });
  };

  const claudeCodeExecutable = process.env.CLAUDE_CODE_EXECUTABLE;
  const sdkDebugFile = process.env.CLAUDE_RS_SDK_DEBUG_FILE;
  const enableSdkDebug = process.env.CLAUDE_RS_SDK_DEBUG === "1" || Boolean(sdkDebugFile);
  const enableSpawnDebug = process.env.CLAUDE_RS_SDK_SPAWN_DEBUG === "1";
  if (claudeCodeExecutable && !fs.existsSync(claudeCodeExecutable)) {
    throw new Error(`CLAUDE_CODE_EXECUTABLE does not exist: ${claudeCodeExecutable}`);
  }

  let queryHandle: Query;
  try {
    queryHandle = query({
      prompt: input,
      options: buildQueryOptions({
        cwd: params.cwd,
        resume: params.resume,
        launchSettings: params.launchSettings,
        provisionalSessionId,
        input,
        canUseTool,
        claudeCodeExecutable,
        sdkDebugFile,
        enableSdkDebug,
        enableSpawnDebug,
        sessionIdForLogs: () => session.sessionId,
      }),
    });
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    throw new Error(
      `query() failed: node_executable=${process.execPath}; cwd=${params.cwd}; ` +
        `resume=${params.resume ?? "<none>"}; ` +
        `CLAUDE_CODE_EXECUTABLE=${claudeCodeExecutable ?? "<unset>"}; error=${message}`,
    );
  }

  session = {
    sessionId: provisionalSessionId,
    cwd: params.cwd,
    model: initialModel,
    availableModels: [],
    mode: initialMode,
    fastModeState: "off",
    query: queryHandle,
    input,
    connected: false,
    connectEvent: params.connectEvent,
    connectRequestId: params.requestId,
    toolCalls: new Map<string, ToolCall>(),
    taskToolUseIds: new Map<string, string>(),
    pendingPermissions: new Map<string, PendingPermission>(),
    pendingQuestions: new Map<string, PendingQuestion>(),
    pendingElicitations: new Map<string, PendingElicitation>(),
    mcpStatusRevalidatedAt: new Map<string, number>(),
    authHintSent: false,
    ...(params.resumeUpdates && params.resumeUpdates.length > 0
      ? { resumeUpdates: params.resumeUpdates }
      : {}),
    ...(params.sessionsToCloseAfterConnect
      ? { sessionsToCloseAfterConnect: params.sessionsToCloseAfterConnect }
      : {}),
  };
  sessions.set(provisionalSessionId, session);

  // In stream-input mode the SDK may defer init until input arrives.
  // Trigger initialization explicitly so the Rust UI can receive `connected`
  // before the first user prompt.
  void session.query
    .initializationResult()
    .then((result) => {
      session.availableModels = mapAvailableModels(result.models);
      if (!session.connected) {
        emitConnectEvent(session);
      }
      // Proactively detect missing auth from account info so the UI can
      // show the login hint immediately, without waiting for the first prompt.
      const acct = result.account;
      const hasCredentials =
        (typeof acct.email === "string" && acct.email.trim().length > 0) ||
        (typeof acct.apiKeySource === "string" && acct.apiKeySource.trim().length > 0);
      if (!hasCredentials) {
        emitAuthRequired(session);
      }
      emitFastModeUpdateIfChanged(session, result.fast_mode_state);

      const commands = Array.isArray(result.commands)
        ? result.commands.map((command) => ({
            name: command.name,
            description: command.description ?? "",
            input_hint: command.argumentHint ?? undefined,
          }))
        : [];
      if (commands.length > 0) {
        emitSessionUpdate(session.sessionId, { type: "available_commands_update", commands });
      }
      emitAvailableAgentsIfChanged(session, mapAvailableAgents(result.agents));
      refreshAvailableAgents(session);
    })
    .catch((error) => {
      if (session.connected) {
        return;
      }
      const message = error instanceof Error ? error.message : String(error);
      failConnection(`agent initialization failed: ${message}`, session.connectRequestId);
      session.connectRequestId = undefined;
    });

  void (async () => {
    try {
      for await (const message of session.query) {
        // Lazy import to break circular dependency at module-evaluation time.
        const { handleSdkMessage } = await import("./message_handlers.js");
        handleSdkMessage(session, message);
      }
      if (!session.connected) {
        failConnection("agent stream ended before session initialization", params.requestId);
      }
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      failConnection(`agent stream failed: ${message}`, params.requestId);
    }
  })();
}

type QueryOptionsBuilderParams = {
  cwd: string;
  resume?: string;
  launchSettings: SessionLaunchSettings;
  provisionalSessionId: string;
  input: AsyncQueue<SDKUserMessage>;
  canUseTool: CanUseTool;
  claudeCodeExecutable?: string;
  sdkDebugFile?: string;
  enableSdkDebug: boolean;
  enableSpawnDebug: boolean;
  sessionIdForLogs: () => string;
};

function permissionModeFromSettingsValue(rawMode: unknown): PermissionMode | undefined {
  if (typeof rawMode !== "string") {
    return undefined;
  }
  switch (rawMode) {
    case "default":
    case "acceptEdits":
    case "bypassPermissions":
    case "plan":
    case "dontAsk":
      return rawMode;
    default:
      throw new Error(`unsupported launch_settings.settings.permissions.defaultMode: ${rawMode}`);
  }
}

function initialSessionModel(launchSettings: SessionLaunchSettings): string {
  const settings = settingsObjectFromLaunchSettings(launchSettings);
  const model = typeof settings?.model === "string" ? settings.model.trim() : "";
  return model || DEFAULT_MODEL_NAME;
}

function startupModelOption(
  launchSettings: SessionLaunchSettings,
): {
  model?: string;
} {
  const settings = settingsObjectFromLaunchSettings(launchSettings);
  const model = typeof settings?.model === "string" ? settings.model.trim() : "";
  return model ? { model } : {};
}

function initialSessionMode(launchSettings: SessionLaunchSettings): PermissionMode {
  const settings = settingsObjectFromLaunchSettings(launchSettings);
  const permissions =
    settings?.permissions && typeof settings.permissions === "object" && !Array.isArray(settings.permissions)
      ? (settings.permissions as Record<string, unknown>)
      : undefined;
  return permissionModeFromSettingsValue(permissions?.defaultMode) ?? DEFAULT_PERMISSION_MODE;
}

function startupPermissionModeOptions(
  launchSettings: SessionLaunchSettings,
): {
  permissionMode?: PermissionMode;
  allowDangerouslySkipPermissions?: boolean;
} {
  const settings = settingsObjectFromLaunchSettings(launchSettings);
  const permissions =
    settings?.permissions && typeof settings.permissions === "object" && !Array.isArray(settings.permissions)
      ? (settings.permissions as Record<string, unknown>)
      : undefined;
  const permissionMode = permissionModeFromSettingsValue(permissions?.defaultMode);
  if (!permissionMode) {
    return {};
  }
  return permissionMode === "bypassPermissions"
    ? {
        permissionMode,
        allowDangerouslySkipPermissions: true,
      }
    : { permissionMode };
}

function systemPromptFromLaunchSettings(
  launchSettings: SessionLaunchSettings,
):
  | {
      type: "preset";
      preset: "claude_code";
      append: string;
    }
  | undefined {
  const language = launchSettings.language?.trim();
  if (!language) {
    return undefined;
  }

  return {
    type: "preset",
    preset: "claude_code",
    append:
      `Always respond to the user in ${language} unless the user explicitly asks for a different language. ` +
      `Keep code, shell commands, file paths, API names, tool names, and raw error text unchanged unless the user explicitly asks for translation.`,
  };
}

export function buildQueryOptions(params: QueryOptionsBuilderParams) {
  const systemPrompt = systemPromptFromLaunchSettings(params.launchSettings);
  const modelOption = startupModelOption(params.launchSettings);
  const permissionModeOptions = startupPermissionModeOptions(params.launchSettings);
  return {
    cwd: params.cwd,
    includePartialMessages: true,
    executable: "node" as const,
    ...(params.resume ? {} : { sessionId: params.provisionalSessionId }),
    ...(params.launchSettings.settings ? { settings: params.launchSettings.settings } : {}),
    ...modelOption,
    ...permissionModeOptions,
    toolConfig: { askUserQuestion: { previewFormat: "markdown" as const } },
    ...(systemPrompt ? { systemPrompt } : {}),
    ...(params.launchSettings.agent_progress_summaries !== undefined
      ? { agentProgressSummaries: params.launchSettings.agent_progress_summaries }
      : {}),
    ...(params.claudeCodeExecutable
      ? { pathToClaudeCodeExecutable: params.claudeCodeExecutable }
      : {}),
    ...(params.enableSdkDebug ? { debug: true } : {}),
    ...(params.sdkDebugFile ? { debugFile: params.sdkDebugFile } : {}),
    stderr: (line: string) => {
      if (line.trim().length > 0) {
        console.error(`[sdk stderr] ${line}`);
      }
    },
    ...(params.enableSpawnDebug
      ? {
          spawnClaudeCodeProcess: (options: {
            command: string;
            args: string[];
            cwd?: string;
            env: Record<string, string | undefined>;
            signal: AbortSignal;
          }) => {
            console.error(
              `[sdk spawn] command=${options.command} args=${JSON.stringify(options.args)} cwd=${options.cwd ?? "<none>"}`,
            );
            const child = spawnChild(options.command, options.args, {
              cwd: options.cwd,
              env: options.env,
              signal: options.signal,
              stdio: ["pipe", "pipe", "pipe"],
              windowsHide: true,
            });
            child.on("error", (error) => {
              console.error(
                `[sdk spawn error] code=${(error as NodeJS.ErrnoException).code ?? "<none>"} message=${error.message}`,
              );
            });
            return child;
          },
        }
      : {}),
    // Match claude-agent-acp defaults to avoid emitting an empty
    // --setting-sources argument.
    settingSources: DEFAULT_SETTING_SOURCES,
    resume: params.resume,
    canUseTool: params.canUseTool,
    onElicitation: async (request: {
      mode?: string;
      serverName?: string;
      message?: string;
      url?: string;
      elicitationId?: string;
      requestedSchema?: Record<string, unknown>;
    }) => {
      const requestId = randomUUID();
      const mode =
        request.mode === "form" || request.mode === "url"
          ? request.mode
          : typeof request.url === "string" && request.url.trim().length > 0
            ? "url"
            : "form";
      const normalized: ElicitationRequest = {
        request_id: requestId,
        server_name:
          typeof request.serverName === "string" && request.serverName.trim().length > 0
            ? request.serverName
            : "unknown",
        message:
          typeof request.message === "string" && request.message.trim().length > 0
            ? request.message
            : "<no message>",
        mode,
        ...(typeof request.url === "string" && request.url.trim().length > 0
          ? { url: request.url }
          : {}),
        ...(typeof request.elicitationId === "string" && request.elicitationId.trim().length > 0
          ? { elicitation_id: request.elicitationId }
          : {}),
        ...(request.requestedSchema
          ? { requested_schema: request.requestedSchema as Record<string, Json> }
          : {}),
      };
      writeEvent({
        event: "elicitation_request",
        session_id: params.sessionIdForLogs(),
        request: normalized,
      });
      return await new Promise<{
        action: ElicitationAction;
        content?: Record<string, unknown>;
      }>((resolve) => {
        const currentSession = sessions.get(params.sessionIdForLogs());
        if (!currentSession) {
          resolve({ action: "cancel" });
          return;
        }
        currentSession.pendingElicitations.set(requestId, {
          resolve,
          serverName: normalized.server_name,
          elicitationId: normalized.elicitation_id,
        });
      });
    },
  };
}

export function mapAvailableModels(models: ModelInfo[] | undefined): AvailableModel[] {
  if (!Array.isArray(models)) {
    return [];
  }

  return models
    .filter((entry): entry is ModelInfo & { value: string; displayName: string } => {
      return (
        typeof entry?.value === "string" &&
        entry.value.trim().length > 0 &&
        typeof entry.displayName === "string" &&
        entry.displayName.trim().length > 0
      );
    })
    .map((entry) => ({
      id: entry.value,
      display_name: entry.displayName,
      supports_effort: entry.supportsEffort === true,
      supported_effort_levels: Array.isArray(entry.supportedEffortLevels)
        ? entry.supportedEffortLevels.filter(
            (level): level is "low" | "medium" | "high" =>
              level === "low" || level === "medium" || level === "high",
          )
        : [],
      ...(typeof entry.supportsAdaptiveThinking === "boolean"
        ? { supports_adaptive_thinking: entry.supportsAdaptiveThinking }
        : {}),
      ...(typeof entry.supportsFastMode === "boolean"
        ? { supports_fast_mode: entry.supportsFastMode }
        : {}),
      ...(typeof entry.supportsAutoMode === "boolean"
        ? { supports_auto_mode: entry.supportsAutoMode }
        : {}),
      ...(typeof entry.description === "string" && entry.description.trim().length > 0
        ? { description: entry.description }
        : {}),
    }));
}

export function handlePermissionResponse(command: Extract<BridgeCommand, { command: "permission_response" }>): void {
  const session = sessionById(command.session_id);
  if (!session) {
    logPermissionDebug(
      `response dropped: unknown session session_id=${command.session_id} tool_call_id=${command.tool_call_id}`,
    );
    return;
  }
  const resolver = session.pendingPermissions.get(command.tool_call_id);
  if (!resolver) {
    logPermissionDebug(
      `response dropped: no pending resolver session_id=${command.session_id} tool_call_id=${command.tool_call_id}`,
    );
    return;
  }
  session.pendingPermissions.delete(command.tool_call_id);

  const outcome = command.outcome as PermissionOutcome;
  if (resolver.onOutcome) {
    resolver.onOutcome(outcome);
    return;
  }
  if (!resolver.resolve) {
    logPermissionDebug(
      `response dropped: resolver missing callback session_id=${command.session_id} tool_call_id=${command.tool_call_id}`,
    );
    return;
  }
  const selectedOption = outcome.outcome === "selected" ? outcome.option_id : "cancelled";
  logPermissionDebug(
    `response session_id=${command.session_id} tool_call_id=${command.tool_call_id} tool=${resolver.toolName} ` +
      `selected=${selectedOption} suggestions=${formatPermissionUpdates(resolver.suggestions)}`,
  );
  if (
    outcome.outcome === "selected" &&
    (outcome.option_id === "allow_once" ||
      outcome.option_id === "allow_session" ||
      outcome.option_id === "allow_always")
  ) {
    setToolCallStatus(session, command.tool_call_id, "in_progress");
  } else if (outcome.outcome === "selected") {
    setToolCallStatus(session, command.tool_call_id, "failed", "Permission denied");
  } else {
    setToolCallStatus(session, command.tool_call_id, "failed", "Permission cancelled");
  }

  const permissionResult = permissionResultFromOutcome(
    outcome,
    command.tool_call_id,
    resolver.inputData,
    resolver.suggestions,
    resolver.toolName,
  );
  if (permissionResult.behavior === "allow") {
    logPermissionDebug(
      `result tool_call_id=${command.tool_call_id} behavior=allow updated_permissions=` +
        `${formatPermissionUpdates(permissionResult.updatedPermissions)}`,
    );
  } else {
    logPermissionDebug(
      `result tool_call_id=${command.tool_call_id} behavior=deny message=${permissionResult.message}`,
    );
  }
  resolver.resolve(permissionResult);
}

export function handleQuestionResponse(command: Extract<BridgeCommand, { command: "question_response" }>): void {
  const session = sessionById(command.session_id);
  if (!session) {
    logPermissionDebug(
      `question response dropped: unknown session session_id=${command.session_id} tool_call_id=${command.tool_call_id}`,
    );
    return;
  }
  const resolver = session.pendingQuestions.get(command.tool_call_id);
  if (!resolver) {
    logPermissionDebug(
      `question response dropped: no pending resolver session_id=${command.session_id} tool_call_id=${command.tool_call_id}`,
    );
    return;
  }
  session.pendingQuestions.delete(command.tool_call_id);
  resolver.onOutcome(command.outcome);
}

export function handleElicitationResponse(
  command: Extract<BridgeCommand, { command: "elicitation_response" }>,
): void {
  const session = sessionById(command.session_id);
  if (!session) {
    console.error(
      `[sdk warn] elicitation response dropped: unknown session ` +
        `session_id=${command.session_id} request_id=${command.elicitation_request_id}`,
    );
    return;
  }
  const pending = session.pendingElicitations.get(command.elicitation_request_id);
  if (!pending) {
    console.error(
      `[sdk warn] elicitation response dropped: no pending request ` +
        `session_id=${command.session_id} request_id=${command.elicitation_request_id}`,
    );
    return;
  }
  session.pendingElicitations.delete(command.elicitation_request_id);
  pending.resolve({
    action: command.action,
    ...(command.content ? { content: command.content } : {}),
  });
}
