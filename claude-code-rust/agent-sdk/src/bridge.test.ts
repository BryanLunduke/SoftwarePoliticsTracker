import test from "node:test";
import assert from "node:assert/strict";
import {
  AsyncQueue,
  CACHE_SPLIT_POLICY,
  buildRateLimitUpdate,
  buildQueryOptions,
  canGenerateSessionTitle,
  generatePersistedSessionTitle,
  buildSessionMutationOptions,
  buildSessionListOptions,
  buildToolResultFields,
  createToolCall,
  handleTaskSystemMessage,
  mapAvailableAgents,
  mapAvailableModels,
  mapSessionMessagesToUpdates,
  mapSdkSessions,
  agentSdkVersionCompatibilityError,
  looksLikeAuthRequired,
  normalizeToolResultText,
  parseFastModeState,
  parseRateLimitStatus,
  normalizeToolKind,
  parseCommandEnvelope,
  permissionOptionsFromSuggestions,
  permissionResultFromOutcome,
  previewKilobyteLabel,
  staleMcpAuthCandidates,
  resolveInstalledAgentSdkVersion,
  unwrapToolUseResult,
} from "./bridge.js";
import type { SessionState } from "./bridge.js";
import { emitToolProgressUpdate } from "./bridge/tool_calls.js";
import { requestAskUserQuestionAnswers } from "./bridge/user_interaction.js";

function makeSessionState(): SessionState {
  const input = new AsyncQueue<import("@anthropic-ai/claude-agent-sdk").SDKUserMessage>();
  return {
    sessionId: "session-1",
    cwd: "C:/work",
    model: "haiku",
    availableModels: [],
    mode: null,
    fastModeState: "off",
    query: {} as import("@anthropic-ai/claude-agent-sdk").Query,
    input,
    connected: true,
    connectEvent: "connected",
    toolCalls: new Map(),
    taskToolUseIds: new Map(),
    pendingPermissions: new Map(),
    pendingQuestions: new Map(),
    pendingElicitations: new Map(),
    mcpStatusRevalidatedAt: new Map(),
    authHintSent: false,
  };
}

function captureBridgeEvents(run: () => void): Array<Record<string, unknown>> {
  const writes: string[] = [];
  const originalWrite = process.stdout.write;
  (process.stdout.write as unknown as (...args: unknown[]) => boolean) = (
    chunk: unknown,
  ): boolean => {
    if (typeof chunk === "string") {
      writes.push(chunk);
    } else if (Buffer.isBuffer(chunk)) {
      writes.push(chunk.toString("utf8"));
    } else {
      writes.push(String(chunk));
    }
    return true;
  };

  try {
    run();
  } finally {
    process.stdout.write = originalWrite;
  }

  return writes
    .map((line) => line.trim())
    .filter((line) => line.startsWith("{"))
    .flatMap((line) => {
      try {
        return [JSON.parse(line) as Record<string, unknown>];
      } catch {
        return [];
      }
    });
}

async function captureBridgeEventsAsync(
  run: () => Promise<void>,
): Promise<Array<Record<string, unknown>>> {
  const writes: string[] = [];
  const originalWrite = process.stdout.write;
  (process.stdout.write as unknown as (...args: unknown[]) => boolean) = (
    chunk: unknown,
  ): boolean => {
    if (typeof chunk === "string") {
      writes.push(chunk);
    } else if (Buffer.isBuffer(chunk)) {
      writes.push(chunk.toString("utf8"));
    } else {
      writes.push(String(chunk));
    }
    return true;
  };

  try {
    await run();
  } finally {
    process.stdout.write = originalWrite;
  }

  return writes
    .map((line) => line.trim())
    .filter((line) => line.startsWith("{"))
    .flatMap((line) => {
      try {
        return [JSON.parse(line) as Record<string, unknown>];
      } catch {
        return [];
      }
    });
}

test("parseCommandEnvelope validates initialize command", () => {
  const parsed = parseCommandEnvelope(
    JSON.stringify({
      request_id: "req-1",
      command: "initialize",
      cwd: "C:/work",
    }),
  );
  assert.equal(parsed.requestId, "req-1");
  assert.equal(parsed.command.command, "initialize");
  if (parsed.command.command !== "initialize") {
    throw new Error("unexpected command variant");
  }
  assert.equal(parsed.command.cwd, "C:/work");
});

test("parseCommandEnvelope validates resume_session command without cwd", () => {
  const parsed = parseCommandEnvelope(
    JSON.stringify({
      request_id: "req-2",
      command: "resume_session",
      session_id: "session-123",
      launch_settings: {
        language: "German",
        settings: {
          alwaysThinkingEnabled: true,
          model: "haiku",
          permissions: { defaultMode: "plan" },
          fastMode: false,
          effortLevel: "high",
          outputStyle: "Default",
          spinnerTipsEnabled: true,
          terminalProgressBarEnabled: true,
        },
        agent_progress_summaries: true,
      },
    }),
  );
  assert.equal(parsed.requestId, "req-2");
  assert.equal(parsed.command.command, "resume_session");
  if (parsed.command.command !== "resume_session") {
    throw new Error("unexpected command variant");
  }
  assert.equal(parsed.command.session_id, "session-123");
  assert.equal(parsed.command.launch_settings.language, "German");
  assert.deepEqual(parsed.command.launch_settings.settings, {
    alwaysThinkingEnabled: true,
    model: "haiku",
    permissions: { defaultMode: "plan" },
    fastMode: false,
    effortLevel: "high",
    outputStyle: "Default",
    spinnerTipsEnabled: true,
    terminalProgressBarEnabled: true,
  });
  assert.equal(parsed.command.launch_settings.agent_progress_summaries, true);
});

test("parseCommandEnvelope validates rename_session command", () => {
  const parsed = parseCommandEnvelope(
    JSON.stringify({
      request_id: "req-rename",
      command: "rename_session",
      session_id: "session-123",
      title: "Renamed session",
    }),
  );

  assert.equal(parsed.requestId, "req-rename");
  assert.equal(parsed.command.command, "rename_session");
  if (parsed.command.command !== "rename_session") {
    throw new Error("unexpected command variant");
  }
  assert.equal(parsed.command.session_id, "session-123");
  assert.equal(parsed.command.title, "Renamed session");
});

test("parseCommandEnvelope validates generate_session_title command", () => {
  const parsed = parseCommandEnvelope(
    JSON.stringify({
      request_id: "req-generate",
      command: "generate_session_title",
      session_id: "session-123",
      description: "Current custom title",
    }),
  );

  assert.equal(parsed.requestId, "req-generate");
  assert.equal(parsed.command.command, "generate_session_title");
  if (parsed.command.command !== "generate_session_title") {
    throw new Error("unexpected command variant");
  }
  assert.equal(parsed.command.session_id, "session-123");
  assert.equal(parsed.command.description, "Current custom title");
});

test("parseCommandEnvelope validates mcp_toggle command", () => {
  const parsed = parseCommandEnvelope(
    JSON.stringify({
      request_id: "req-mcp-toggle",
      command: "mcp_toggle",
      session_id: "session-123",
      server_name: "notion",
      enabled: false,
    }),
  );

  assert.equal(parsed.requestId, "req-mcp-toggle");
  assert.equal(parsed.command.command, "mcp_toggle");
  if (parsed.command.command !== "mcp_toggle") {
    throw new Error("unexpected command variant");
  }
  assert.equal(parsed.command.session_id, "session-123");
  assert.equal(parsed.command.server_name, "notion");
  assert.equal(parsed.command.enabled, false);
});

test("parseCommandEnvelope validates mcp_set_servers command", () => {
  const parsed = parseCommandEnvelope(
    JSON.stringify({
      request_id: "req-mcp-set",
      command: "mcp_set_servers",
      session_id: "session-123",
      servers: {
        notion: {
          type: "http",
          url: "https://mcp.notion.com/mcp",
          headers: {
            "X-Test": "1",
          },
        },
      },
    }),
  );

  assert.equal(parsed.requestId, "req-mcp-set");
  assert.equal(parsed.command.command, "mcp_set_servers");
  if (parsed.command.command !== "mcp_set_servers") {
    throw new Error("unexpected command variant");
  }
  assert.equal(parsed.command.session_id, "session-123");
  assert.deepEqual(parsed.command.servers, {
    notion: {
      type: "http",
      url: "https://mcp.notion.com/mcp",
      headers: {
        "X-Test": "1",
      },
    },
  });
});

test("staleMcpAuthCandidates selects previously connected servers that regressed to needs-auth", () => {
  const candidates = staleMcpAuthCandidates(
    [
      {
        name: "supabase",
        status: "needs-auth",
        server_info: undefined,
        error: undefined,
        config: undefined,
        scope: undefined,
        tools: [],
      },
      {
        name: "notion",
        status: "needs-auth",
        server_info: undefined,
        error: undefined,
        config: undefined,
        scope: undefined,
        tools: [],
      },
    ],
    new Set(["supabase"]),
    new Map(),
    10_000,
    1_000,
  );

  assert.deepEqual(candidates, ["supabase"]);
});

test("staleMcpAuthCandidates respects the revalidation cooldown", () => {
  const candidates = staleMcpAuthCandidates(
    [
      {
        name: "supabase",
        status: "needs-auth",
        server_info: undefined,
        error: undefined,
        config: undefined,
        scope: undefined,
        tools: [],
      },
    ],
    new Set(["supabase"]),
    new Map([["supabase", 9_500]]),
    10_000,
    1_000,
  );

  assert.deepEqual(candidates, []);
});

test("buildSessionMutationOptions scopes rename requests to the session cwd", () => {
  assert.deepEqual(buildSessionMutationOptions("C:/worktree"), { dir: "C:/worktree" });
  assert.equal(buildSessionMutationOptions(undefined), undefined);
});

test("canGenerateSessionTitle detects supported query objects", () => {
  const query = {
    async generateSessionTitle(): Promise<string> {
      return "Generated";
    },
  } as unknown as import("@anthropic-ai/claude-agent-sdk").Query;

  assert.equal(canGenerateSessionTitle(query), true);
  assert.equal(canGenerateSessionTitle({} as import("@anthropic-ai/claude-agent-sdk").Query), false);
});

test("generatePersistedSessionTitle calls sdk query with persist true", async () => {
  const calls: Array<{ description: string; persist?: boolean }> = [];
  const query = {
    async generateSessionTitle(
      description: string,
      options?: { persist?: boolean },
    ): Promise<string> {
      calls.push({ description, persist: options?.persist });
      return "Generated title";
    },
  } as unknown as import("@anthropic-ai/claude-agent-sdk").Query;

  const title = await generatePersistedSessionTitle(query, "Current summary");

  assert.equal(title, "Generated title");
  assert.deepEqual(calls, [{ description: "Current summary", persist: true }]);
});

test("buildQueryOptions maps launch settings into sdk query options", () => {
  const input = new AsyncQueue<import("@anthropic-ai/claude-agent-sdk").SDKUserMessage>();
  const options = buildQueryOptions({
    cwd: "C:/work",
    launchSettings: {
      language: "German",
      settings: {
        alwaysThinkingEnabled: true,
        model: "haiku",
        permissions: { defaultMode: "plan" },
        fastMode: false,
        effortLevel: "medium",
        outputStyle: "Default",
        spinnerTipsEnabled: true,
        terminalProgressBarEnabled: true,
      },
      agent_progress_summaries: true,
    },
    provisionalSessionId: "session-1",
    input,
    canUseTool: async () => ({ behavior: "deny", message: "not used" }),
    enableSdkDebug: false,
    enableSpawnDebug: false,
    sessionIdForLogs: () => "session-1",
  });

  assert.deepEqual(options.settings, {
    alwaysThinkingEnabled: true,
    model: "haiku",
    permissions: { defaultMode: "plan" },
    fastMode: false,
    effortLevel: "medium",
    outputStyle: "Default",
    spinnerTipsEnabled: true,
    terminalProgressBarEnabled: true,
  });
  assert.deepEqual(options.systemPrompt, {
    type: "preset",
    preset: "claude_code",
    append:
      "Always respond to the user in German unless the user explicitly asks for a different language. " +
      "Keep code, shell commands, file paths, API names, tool names, and raw error text unchanged unless the user explicitly asks for translation.",
  });
  assert.equal(options.model, "haiku");
  assert.equal(options.permissionMode, "plan");
  assert.equal("allowDangerouslySkipPermissions" in options, false);
  assert.equal("thinking" in options, false);
  assert.equal("effort" in options, false);
  assert.equal(options.agentProgressSummaries, true);
  assert.equal(options.sessionId, "session-1");
  assert.deepEqual(options.settingSources, ["user", "project", "local"]);
  assert.deepEqual(options.toolConfig, {
    askUserQuestion: { previewFormat: "markdown" },
  });
});

test("buildQueryOptions forwards settings and maps startup model and permission mode", () => {
  const input = new AsyncQueue<import("@anthropic-ai/claude-agent-sdk").SDKUserMessage>();
  const options = buildQueryOptions({
    cwd: "C:/work",
    launchSettings: {
      settings: {
        alwaysThinkingEnabled: false,
        permissions: { defaultMode: "default" },
        fastMode: true,
        effortLevel: "high",
        outputStyle: "Learning",
        spinnerTipsEnabled: false,
        terminalProgressBarEnabled: false,
      },
    },
    provisionalSessionId: "session-3",
    input,
    canUseTool: async () => ({ behavior: "deny", message: "not used" }),
    enableSdkDebug: false,
    enableSpawnDebug: false,
    sessionIdForLogs: () => "session-3",
  });

  assert.deepEqual(options.settings, {
    alwaysThinkingEnabled: false,
    permissions: { defaultMode: "default" },
    fastMode: true,
    effortLevel: "high",
    outputStyle: "Learning",
    spinnerTipsEnabled: false,
    terminalProgressBarEnabled: false,
  });
  assert.equal("model" in options, false);
  assert.equal(options.permissionMode, "default");
  assert.equal("allowDangerouslySkipPermissions" in options, false);
  assert.equal("thinking" in options, false);
  assert.equal("effort" in options, false);
});

test("buildQueryOptions trims startup model before passing sdk option", () => {
  const input = new AsyncQueue<import("@anthropic-ai/claude-agent-sdk").SDKUserMessage>();
  const options = buildQueryOptions({
    cwd: "C:/work",
    launchSettings: {
      settings: {
        model: "  claude-opus-4-6  ",
        permissions: { defaultMode: "plan" },
      },
    },
    provisionalSessionId: "session-model",
    input,
    canUseTool: async () => ({ behavior: "deny", message: "not used" }),
    enableSdkDebug: false,
    enableSpawnDebug: false,
    sessionIdForLogs: () => "session-model",
  });

  assert.equal(options.model, "claude-opus-4-6");
  assert.equal(options.permissionMode, "plan");
});

test("buildQueryOptions enables dangerous skip flag for bypass permissions startup mode", () => {
  const input = new AsyncQueue<import("@anthropic-ai/claude-agent-sdk").SDKUserMessage>();
  const options = buildQueryOptions({
    cwd: "C:/work",
    launchSettings: {
      settings: {
        permissions: { defaultMode: "bypassPermissions" },
      },
    },
    provisionalSessionId: "session-4",
    input,
    canUseTool: async () => ({ behavior: "deny", message: "not used" }),
    enableSdkDebug: false,
    enableSpawnDebug: false,
    sessionIdForLogs: () => "session-4",
  });

  assert.equal(options.permissionMode, "bypassPermissions");
  assert.equal(options.allowDangerouslySkipPermissions, true);
});

test("buildQueryOptions omits startup overrides for default logout path", () => {
  const input = new AsyncQueue<import("@anthropic-ai/claude-agent-sdk").SDKUserMessage>();
  const options = buildQueryOptions({
    cwd: "C:/work",
    launchSettings: {},
    provisionalSessionId: "session-2",
    input,
    canUseTool: async () => ({ behavior: "deny", message: "not used" }),
    enableSdkDebug: false,
    enableSpawnDebug: false,
    sessionIdForLogs: () => "session-2",
  });

  assert.equal("model" in options, false);
  assert.equal("permissionMode" in options, false);
  assert.equal("allowDangerouslySkipPermissions" in options, false);
  assert.equal("systemPrompt" in options, false);
  assert.equal("agentProgressSummaries" in options, false);
});

test("handleTaskSystemMessage prefers task_progress summary over fallback text", () => {
  const session = makeSessionState();

  const events = captureBridgeEvents(() => {
    handleTaskSystemMessage(session, "task_started", {
      task_id: "task-1",
      tool_use_id: "tool-1",
      description: "Initial task description",
    });
    handleTaskSystemMessage(session, "task_progress", {
      task_id: "task-1",
      summary: "Analyzing authentication flow",
      description: "Should not be shown",
      last_tool_name: "Read",
    });
  });

  const lastEvent = events.at(-1);
  assert.ok(lastEvent);
  assert.equal(lastEvent.event, "session_update");
  assert.deepEqual(lastEvent.update, {
    type: "tool_call_update",
    tool_call_update: {
      tool_call_id: "tool-1",
      fields: {
        status: "in_progress",
        raw_output: "Analyzing authentication flow",
        content: [
          {
            type: "content",
            content: { type: "text", text: "Analyzing authentication flow" },
          },
        ],
      },
    },
  });
});

test("handleTaskSystemMessage falls back to description and last tool when progress summary is absent", () => {
  const session = makeSessionState();

  const events = captureBridgeEvents(() => {
    handleTaskSystemMessage(session, "task_started", {
      task_id: "task-1",
      tool_use_id: "tool-1",
      description: "Initial task description",
    });
    handleTaskSystemMessage(session, "task_progress", {
      task_id: "task-1",
      description: "Inspecting auth code",
      last_tool_name: "Read",
    });
  });

  const lastEvent = events.at(-1);
  assert.ok(lastEvent);
  assert.equal(lastEvent.event, "session_update");
  assert.deepEqual(lastEvent.update, {
    type: "tool_call_update",
    tool_call_update: {
      tool_call_id: "tool-1",
      fields: {
        status: "in_progress",
        raw_output: "Inspecting auth code (last tool: Read)",
        content: [
          {
            type: "content",
            content: { type: "text", text: "Inspecting auth code (last tool: Read)" },
          },
        ],
      },
    },
  });
});

test("handleTaskSystemMessage final summary replaces prior task content and finalizes status", () => {
  const session = makeSessionState();

  const events = captureBridgeEvents(() => {
    handleTaskSystemMessage(session, "task_started", {
      task_id: "task-1",
      tool_use_id: "tool-1",
      description: "Initial task description",
    });
    handleTaskSystemMessage(session, "task_progress", {
      task_id: "task-1",
      summary: "Analyzing authentication flow",
      description: "Should not be shown",
    });
    handleTaskSystemMessage(session, "task_notification", {
      task_id: "task-1",
      status: "completed",
      summary: "Found the auth bug and prepared the fix",
    });
  });

  const lastEvent = events.at(-1);
  assert.ok(lastEvent);
  assert.equal(lastEvent.event, "session_update");
  assert.deepEqual(lastEvent.update, {
    type: "tool_call_update",
    tool_call_update: {
      tool_call_id: "tool-1",
      fields: {
        status: "completed",
        raw_output: "Found the auth bug and prepared the fix",
        content: [
          {
            type: "content",
            content: { type: "text", text: "Found the auth bug and prepared the fix" },
          },
        ],
      },
    },
  });
  assert.equal(session.taskToolUseIds.has("task-1"), false);
});

test("emitToolProgressUpdate does not reopen completed tools", () => {
  const session = makeSessionState();
  session.toolCalls.set("tool-1", {
    tool_call_id: "tool-1",
    title: "Bash",
    kind: "execute",
    status: "completed",
    content: [],
    locations: [],
    meta: { claudeCode: { toolName: "Bash" } },
  });

  const events = captureBridgeEvents(() => {
    emitToolProgressUpdate(session, "tool-1", "Bash");
  });

  assert.equal(events.length, 0);
  assert.equal(session.toolCalls.get("tool-1")?.status, "completed");
});

test("buildQueryOptions trims language before appending system prompt", () => {
  const input = new AsyncQueue<import("@anthropic-ai/claude-agent-sdk").SDKUserMessage>();
  const options = buildQueryOptions({
    cwd: "C:/work",
    launchSettings: {
      language: "  German  ",
    },
    provisionalSessionId: "session-4",
    input,
    canUseTool: async () => ({ behavior: "deny", message: "not used" }),
    enableSdkDebug: false,
    enableSpawnDebug: false,
    sessionIdForLogs: () => "session-4",
  });

  assert.deepEqual(options.systemPrompt, {
    type: "preset",
    preset: "claude_code",
    append:
      "Always respond to the user in German unless the user explicitly asks for a different language. " +
      "Keep code, shell commands, file paths, API names, tool names, and raw error text unchanged unless the user explicitly asks for translation.",
  });
});

test("parseCommandEnvelope rejects missing required fields", () => {
  assert.throws(
    () => parseCommandEnvelope(JSON.stringify({ command: "set_model", session_id: "s1" })),
    /set_model\.model must be a string/,
  );
});

test("parseCommandEnvelope validates question_response command", () => {
  const parsed = parseCommandEnvelope(
    JSON.stringify({
      request_id: "req-question",
      command: "question_response",
      session_id: "session-1",
      tool_call_id: "tool-1",
      outcome: {
        outcome: "answered",
        selected_option_ids: ["question_0", "question_2"],
        annotation: {
          preview: "Rendered preview",
          notes: "User note",
        },
      },
    }),
  );

  assert.equal(parsed.requestId, "req-question");
  assert.equal(parsed.command.command, "question_response");
  if (parsed.command.command !== "question_response") {
    throw new Error("unexpected command variant");
  }
  assert.deepEqual(parsed.command.outcome, {
    outcome: "answered",
    selected_option_ids: ["question_0", "question_2"],
    annotation: {
      preview: "Rendered preview",
      notes: "User note",
    },
  });
});

test("requestAskUserQuestionAnswers preserves previews and annotations in updated input", async () => {
  const session = makeSessionState();
  const baseToolCall = {
    tool_call_id: "tool-question",
    title: "AskUserQuestion",
    kind: "other",
    status: "in_progress",
    content: [] as Array<import("./types.js").ToolCallContent>,
    locations: [] as Array<import("./types.js").ToolLocation>,
    meta: { claudeCode: { toolName: "AskUserQuestion" } },
  };

  const events = await captureBridgeEventsAsync(async () => {
    const resultPromise = requestAskUserQuestionAnswers(
      session,
      "tool-question",
      {
        questions: [
          {
            question: "Pick deployment target",
            header: "Target",
            multiSelect: true,
            options: [
              {
                label: "Staging",
                description: "Low-risk validation",
                preview: "Deploy to staging first.",
              },
              {
                label: "Production",
                description: "Customer-facing rollout",
                preview: "Deploy to production after approval.",
              },
            ],
          },
        ],
      },
      baseToolCall,
    );

    await new Promise((resolve) => setImmediate(resolve));
    const pending = session.pendingQuestions.get("tool-question");
    assert.ok(pending, "expected pending question");
    pending.onOutcome({
      outcome: "answered",
      selected_option_ids: ["question_0", "question_1"],
      annotation: {
        notes: "Roll out in both environments",
      },
    });

    const result = await resultPromise;
    assert.equal(result.behavior, "allow");
    if (result.behavior !== "allow") {
      throw new Error("expected allow result");
    }
    assert.deepEqual(result.updatedInput, {
      questions: [
        {
          question: "Pick deployment target",
          header: "Target",
          multiSelect: true,
          options: [
            {
              label: "Staging",
              description: "Low-risk validation",
              preview: "Deploy to staging first.",
            },
            {
              label: "Production",
              description: "Customer-facing rollout",
              preview: "Deploy to production after approval.",
            },
          ],
        },
      ],
      answers: {
        "Pick deployment target": "Staging, Production",
      },
      annotations: {
        "Pick deployment target": {
          preview: "Deploy to staging first.\n\nDeploy to production after approval.",
          notes: "Roll out in both environments",
        },
      },
    });
  });

  const questionEvent = events.find((event) => event.event === "question_request");
  assert.ok(questionEvent, "expected question request event");
  assert.deepEqual(questionEvent.request, {
    tool_call: {
      tool_call_id: "tool-question",
      title: "Pick deployment target",
      kind: "other",
      status: "in_progress",
      content: [],
      locations: [],
      meta: { claudeCode: { toolName: "AskUserQuestion" } },
      raw_input: {
        prompt: {
          question: "Pick deployment target",
          header: "Target",
          multi_select: true,
          options: [
            {
              option_id: "question_0",
              label: "Staging",
              description: "Low-risk validation",
              preview: "Deploy to staging first.",
            },
            {
              option_id: "question_1",
              label: "Production",
              description: "Customer-facing rollout",
              preview: "Deploy to production after approval.",
            },
          ],
        },
        question_index: 0,
        total_questions: 1,
      },
    },
    prompt: {
      question: "Pick deployment target",
      header: "Target",
      multi_select: true,
      options: [
        {
          option_id: "question_0",
          label: "Staging",
          description: "Low-risk validation",
          preview: "Deploy to staging first.",
        },
        {
          option_id: "question_1",
          label: "Production",
          description: "Customer-facing rollout",
          preview: "Deploy to production after approval.",
        },
      ],
    },
    question_index: 0,
    total_questions: 1,
  });
});

test("normalizeToolKind maps known tool names", () => {
  assert.equal(normalizeToolKind("Bash"), "execute");
  assert.equal(normalizeToolKind("Delete"), "delete");
  assert.equal(normalizeToolKind("Move"), "move");
  assert.equal(normalizeToolKind("Task"), "think");
  assert.equal(normalizeToolKind("Agent"), "think");
  assert.equal(normalizeToolKind("ExitPlanMode"), "switch_mode");
  assert.equal(normalizeToolKind("TodoWrite"), "other");
});

test("parseFastModeState accepts known values and rejects unknown values", () => {
  assert.equal(parseFastModeState("off"), "off");
  assert.equal(parseFastModeState("cooldown"), "cooldown");
  assert.equal(parseFastModeState("on"), "on");
  assert.equal(parseFastModeState("CD"), null);
  assert.equal(parseFastModeState(undefined), null);
});

test("parseRateLimitStatus accepts known values and rejects unknown values", () => {
  assert.equal(parseRateLimitStatus("allowed"), "allowed");
  assert.equal(parseRateLimitStatus("allowed_warning"), "allowed_warning");
  assert.equal(parseRateLimitStatus("rejected"), "rejected");
  assert.equal(parseRateLimitStatus("warn"), null);
  assert.equal(parseRateLimitStatus(undefined), null);
});

test("buildRateLimitUpdate maps SDK fields to wire shape", () => {
  const update = buildRateLimitUpdate({
    status: "allowed_warning",
    resetsAt: 1_741_280_000,
    utilization: 0.92,
    rateLimitType: "five_hour",
    overageStatus: "rejected",
    overageResetsAt: 1_741_280_600,
    overageDisabledReason: "out_of_credits",
    isUsingOverage: false,
    surpassedThreshold: 0.9,
  });

  assert.deepEqual(update, {
    type: "rate_limit_update",
    status: "allowed_warning",
    resets_at: 1_741_280_000,
    utilization: 0.92,
    rate_limit_type: "five_hour",
    overage_status: "rejected",
    overage_resets_at: 1_741_280_600,
    overage_disabled_reason: "out_of_credits",
    is_using_overage: false,
    surpassed_threshold: 0.9,
  });
});

test("buildRateLimitUpdate rejects invalid payloads", () => {
  assert.equal(buildRateLimitUpdate(null), null);
  assert.equal(buildRateLimitUpdate({}), null);
  assert.equal(buildRateLimitUpdate({ status: "warning" }), null);
  assert.deepEqual(
    buildRateLimitUpdate({
      status: "rejected",
      overageStatus: "bad_status",
    }),
    { type: "rate_limit_update", status: "rejected" },
  );
});

test("mapAvailableAgents normalizes and deduplicates agents", () => {
  const agents = mapAvailableAgents([
    { name: "reviewer", description: "", model: "" },
    { name: "reviewer", description: "Reviews code", model: "haiku" },
    { name: "explore", description: "Explore codebase", model: "sonnet" },
    { name: "  ", description: "ignored" },
    {},
  ]);

  assert.deepEqual(agents, [
    { name: "explore", description: "Explore codebase", model: "sonnet" },
    { name: "reviewer", description: "Reviews code", model: "haiku" },
  ]);
});

test("mapAvailableAgents rejects non-array payload", () => {
  assert.deepEqual(mapAvailableAgents(null), []);
  assert.deepEqual(mapAvailableAgents({}), []);
});

test("createToolCall builds edit diff content", () => {
  const toolCall = createToolCall("tc-1", "Edit", {
    file_path: "src/main.rs",
    old_string: "old",
    new_string: "new",
  });
  assert.equal(toolCall.kind, "edit");
  assert.equal(toolCall.content.length, 1);
  assert.deepEqual(toolCall.content[0], {
    type: "diff",
    old_path: "src/main.rs",
    new_path: "src/main.rs",
    old: "old",
    new: "new",
  });
  assert.deepEqual(toolCall.meta, { claudeCode: { toolName: "Edit" } });
});

test("createToolCall builds write preview diff content", () => {
  const toolCall = createToolCall("tc-w", "Write", {
    file_path: "src/new-file.ts",
    content: "export const x = 1;\n",
  });
  assert.equal(toolCall.kind, "edit");
  assert.deepEqual(toolCall.content, [
    {
      type: "diff",
      old_path: "src/new-file.ts",
      new_path: "src/new-file.ts",
      old: "",
      new: "export const x = 1;\n",
    },
  ]);
});

test("createToolCall includes glob and webfetch context in title", () => {
  const glob = createToolCall("tc-g", "Glob", { pattern: "**/*.md", path: "notes" });
  assert.equal(glob.title, "Glob **/*.md in notes");

  const fetch = createToolCall("tc-f", "WebFetch", { url: "https://example.com" });
  assert.equal(fetch.title, "WebFetch https://example.com");
});

test("buildToolResultFields extracts plain-text output", () => {
  const fields = buildToolResultFields(false, [{ text: "line 1" }, { text: "line 2" }]);
  assert.equal(fields.status, "completed");
  assert.equal(fields.raw_output, "line 1\nline 2");
  assert.deepEqual(fields.content, [
    { type: "content", content: { type: "text", text: "line 1\nline 2" } },
  ]);
});

test("normalizeToolResultText collapses persisted-output payload to first meaningful line", () => {
  const normalized = normalizeToolResultText(`
<persisted-output>
  │ Output too large (132.5KB). Full output saved to: C:\\tmp\\tool-results\\bbf63b9.txt
  │
  │ Preview (first 2KB):
  │
  │ {"huge":"payload"}
  │ ...
  │ </persisted-output>
`);
  assert.equal(normalized, "Output too large (132.5KB). Full output saved to: C:\\tmp\\tool-results\\bbf63b9.txt");
});

test("normalizeToolResultText does not sanitize non-error output", () => {
  const text =
    "The user doesn't want to proceed with this tool use. The tool use was rejected (eg. if it was a file edit, the new_string was NOT written to the file). STOP what you are doing and wait for the user to tell you how to proceed.";
  assert.equal(normalizeToolResultText(text), text);
});

test("normalizeToolResultText sanitizes exact SDK rejection payloads for errors", () => {
  const cancelledText =
    "The user doesn't want to proceed with this tool use. The tool use was rejected (eg. if it was a file edit, the new_string was NOT written to the file). STOP what you are doing and wait for the user to tell you how to proceed.";
  assert.equal(normalizeToolResultText(cancelledText, true), "Cancelled by user.");

  const deniedText =
    "Permission for this tool use was denied. The tool use was rejected (eg. if it was a file edit, the new_string was NOT written to the file). Try a different approach or report the limitation to complete your task.";
  assert.equal(normalizeToolResultText(deniedText, true), "Permission denied.");
});

test("normalizeToolResultText sanitizes SDK rejection prefixes with user follow-up", () => {
  const cancelledWithUserMessage =
    "The user doesn't want to proceed with this tool use. The tool use was rejected (eg. if it was a file edit, the new_string was NOT written to the file). To tell you how to proceed, the user said:\nPlease skip this";
  assert.equal(normalizeToolResultText(cancelledWithUserMessage, true), "Cancelled by user.");

  const deniedWithUserMessage =
    "Permission for this tool use was denied. The tool use was rejected (eg. if it was a file edit, the new_string was NOT written to the file). The user said:\nNot now";
  assert.equal(normalizeToolResultText(deniedWithUserMessage, true), "Permission denied.");
});

test("normalizeToolResultText does not sanitize substring matches in error output", () => {
  const bashOutput = "grep output: doesn't want to proceed with this tool use";
  assert.equal(normalizeToolResultText(bashOutput, true), bashOutput);
});

test("cache split policy defaults stay aligned with UI thresholds", () => {
  assert.equal(CACHE_SPLIT_POLICY.softLimitBytes, 1536);
  assert.equal(CACHE_SPLIT_POLICY.hardLimitBytes, 4096);
  assert.equal(CACHE_SPLIT_POLICY.previewLimitBytes, 2048);
  assert.equal(previewKilobyteLabel(CACHE_SPLIT_POLICY), "2KB");
});

test("buildToolResultFields uses normalized persisted-output text", () => {
  const fields = buildToolResultFields(
    false,
    `<persisted-output>
      │ Output too large (14KB). Full output saved to: C:\\tmp\\tool-results\\x.txt
      │
      │ Preview (first 2KB):
      │ {"k":"v"}
      │ </persisted-output>`,
  );
  assert.equal(fields.raw_output, "Output too large (14KB). Full output saved to: C:\\tmp\\tool-results\\x.txt");
  assert.deepEqual(fields.content, [
    {
      type: "content",
      content: {
        type: "text",
        text: "Output too large (14KB). Full output saved to: C:\\tmp\\tool-results\\x.txt",
      },
    },
  ]);
});

test("buildToolResultFields sanitizes SDK rejection text only for failed results", () => {
  const sdkRejectionText =
    "The user doesn't want to proceed with this tool use. The tool use was rejected (eg. if it was a file edit, the new_string was NOT written to the file). STOP what you are doing and wait for the user to tell you how to proceed.";

  const successFields = buildToolResultFields(false, sdkRejectionText);
  assert.equal(successFields.raw_output, sdkRejectionText);

  const errorFields = buildToolResultFields(true, sdkRejectionText);
  assert.equal(errorFields.raw_output, "Cancelled by user.");
});

test("buildToolResultFields maps structured Write output to diff content", () => {
  const base = createToolCall("tc-w", "Write", {
    file_path: "src/main.ts",
    content: "new",
  });
  const fields = buildToolResultFields(
    false,
    {
      type: "update",
      filePath: "src/main.ts",
      content: "new",
      originalFile: "old",
      structuredPatch: [],
      gitDiff: {
        repository: "acme/project",
      },
    },
    base,
  );
  assert.equal(fields.status, "completed");
  assert.deepEqual(fields.content, [
    {
      type: "diff",
      old_path: "src/main.ts",
      new_path: "src/main.ts",
      old: "old",
      new: "new",
      repository: "acme/project",
    },
  ]);
});

test("buildToolResultFields preserves Edit diff content from input and structured repository", () => {
  const base = createToolCall("tc-e", "Edit", {
    file_path: "src/main.ts",
    old_string: "old",
    new_string: "new",
  });
  const fields = buildToolResultFields(
    false,
    [{ text: "Updated successfully" }],
    base,
    {
      result: {
        filePath: "src/main.ts",
        gitDiff: {
          repository: "acme/project",
        },
      },
    },
  );
  assert.equal(fields.status, "completed");
  assert.deepEqual(fields.content, [
    {
      type: "diff",
      old_path: "src/main.ts",
      new_path: "src/main.ts",
      old: "old",
      new: "new",
      repository: "acme/project",
    },
  ]);
});

test("buildToolResultFields prefers structured Bash stdout over token-saver output", () => {
  const base = createToolCall("tc-bash", "Bash", { command: "npm test" });
  const fields = buildToolResultFields(
    false,
    {
      stdout: "real stdout",
      stderr: "",
      interrupted: false,
      tokenSaverOutput: "compressed output for model",
    },
    base,
    {
      result: {
        stdout: "real stdout",
        stderr: "",
        interrupted: false,
        tokenSaverOutput: "compressed output for model",
      },
    },
  );

  assert.equal(fields.raw_output, "real stdout");
  assert.deepEqual(fields.output_metadata, {
    bash: {
      token_saver_active: true,
    },
  });
});

test("buildToolResultFields adds Bash auto-backgrounded metadata and message", () => {
  const base = createToolCall("tc-bash-bg", "Bash", { command: "npm run watch" });
  const fields = buildToolResultFields(
    false,
    {
      stdout: "",
      stderr: "",
      interrupted: false,
      backgroundTaskId: "task-42",
      assistantAutoBackgrounded: true,
    },
    base,
    {
      result: {
        stdout: "",
        stderr: "",
        interrupted: false,
        backgroundTaskId: "task-42",
        assistantAutoBackgrounded: true,
      },
    },
  );

  assert.equal(
    fields.raw_output,
    "Command was auto-backgrounded by assistant mode with ID: task-42.",
  );
  assert.deepEqual(fields.output_metadata, {
    bash: {
      assistant_auto_backgrounded: true,
    },
  });
});

test("buildToolResultFields maps structured ReadMcpResource output to typed resource content", () => {
  const base = createToolCall("tc-mcp", "ReadMcpResource", {
    server: "docs",
    uri: "file://manual.pdf",
  });
  const fields = buildToolResultFields(
    false,
    {
      contents: [
        {
          uri: "file://manual.pdf",
          mimeType: "application/pdf",
          text: "[Resource from docs at file://manual.pdf] Saved to C:\\tmp\\manual.pdf",
          blobSavedTo: "C:\\tmp\\manual.pdf",
        },
      ],
    },
    base,
    {
      result: {
        contents: [
          {
            uri: "file://manual.pdf",
            mimeType: "application/pdf",
            text: "[Resource from docs at file://manual.pdf] Saved to C:\\tmp\\manual.pdf",
            blobSavedTo: "C:\\tmp\\manual.pdf",
          },
        ],
      },
    },
  );

  assert.equal(fields.status, "completed");
  assert.deepEqual(fields.content, [
    {
      type: "mcp_resource",
      uri: "file://manual.pdf",
      mime_type: "application/pdf",
      text: "[Resource from docs at file://manual.pdf] Saved to C:\\tmp\\manual.pdf",
      blob_saved_to: "C:\\tmp\\manual.pdf",
    },
  ]);
});

test("buildToolResultFields restores ReadMcpResource blob paths from transcript JSON text", () => {
  const base = createToolCall("tc-mcp-history", "ReadMcpResource", {
    server: "docs",
    uri: "file://manual.pdf",
  });
  const transcriptJson = JSON.stringify({
    contents: [
      {
        uri: "file://manual.pdf",
        mimeType: "application/pdf",
        text: "[Resource from docs at file://manual.pdf] Saved to C:\\tmp\\manual.pdf",
        blobSavedTo: "C:\\tmp\\manual.pdf",
      },
    ],
  });
  const fields = buildToolResultFields(false, transcriptJson, base, {
    type: "tool_result",
    tool_use_id: "tc-mcp-history",
    content: transcriptJson,
  });

  assert.deepEqual(fields.content, [
    {
      type: "mcp_resource",
      uri: "file://manual.pdf",
      mime_type: "application/pdf",
      text: "[Resource from docs at file://manual.pdf] Saved to C:\\tmp\\manual.pdf",
      blob_saved_to: "C:\\tmp\\manual.pdf",
    },
  ]);
});

test("unwrapToolUseResult extracts error/content payload", () => {
  const parsed = unwrapToolUseResult({
    is_error: true,
    content: [{ text: "failure output" }],
  });
  assert.equal(parsed.isError, true);
  assert.deepEqual(parsed.content, [{ text: "failure output" }]);
});

test("permissionResultFromOutcome maps selected and cancelled outcomes", () => {
  const allow = permissionResultFromOutcome(
    { outcome: "selected", option_id: "allow_always" },
    "tool-1",
    { command: "echo test" },
    [],
  );
  assert.equal(allow.behavior, "allow");
  if (allow.behavior === "allow") {
    assert.deepEqual(allow.updatedInput, { command: "echo test" });
  }

  const deny = permissionResultFromOutcome(
    { outcome: "selected", option_id: "reject_once" },
    "tool-1",
    { command: "echo test" },
  );
  assert.equal(deny.behavior, "deny");
  assert.match(String(deny.message), /Permission denied/);

  const cancelled = permissionResultFromOutcome(
    { outcome: "cancelled" },
    "tool-1",
    { command: "echo test" },
  );
  assert.equal(cancelled.behavior, "deny");
  assert.match(String(cancelled.message), /cancelled/i);
});

test("permissionOptionsFromSuggestions uses session label when only session scope is suggested", () => {
  const options = permissionOptionsFromSuggestions([
    {
      type: "setMode",
      mode: "acceptEdits",
      destination: "session",
    },
  ]);
  assert.deepEqual(options, [
    { option_id: "allow_once", name: "Allow once", kind: "allow_once" },
    { option_id: "allow_session", name: "Allow for session", kind: "allow_session" },
    { option_id: "reject_once", name: "Deny", kind: "reject_once" },
  ]);
});

test("permissionOptionsFromSuggestions uses persistent label when settings scope is suggested", () => {
  const options = permissionOptionsFromSuggestions([
    {
      type: "addRules",
      behavior: "allow",
      destination: "localSettings",
      rules: [{ toolName: "Bash", ruleContent: "npm install" }],
    },
  ]);
  assert.deepEqual(options, [
    { option_id: "allow_once", name: "Allow once", kind: "allow_once" },
    { option_id: "allow_always", name: "Always allow", kind: "allow_always" },
    { option_id: "reject_once", name: "Deny", kind: "reject_once" },
  ]);
});

test("permissionResultFromOutcome keeps Bash allow_always suggestions unchanged", () => {
  const allow = permissionResultFromOutcome(
    { outcome: "selected", option_id: "allow_always" },
    "tool-1",
    { command: "npm install" },
    [
      {
        type: "addRules",
        behavior: "allow",
        destination: "localSettings",
        rules: [
          { toolName: "Bash", ruleContent: "npm install" },
          { toolName: "WebFetch", ruleContent: "https://example.com" },
          { toolName: "Bash", ruleContent: "dir /B" },
        ],
      },
    ],
    "Bash",
  );

  assert.equal(allow.behavior, "allow");
  if (allow.behavior !== "allow") {
    throw new Error("expected allow permission result");
  }
  assert.deepEqual(allow.updatedPermissions, [
    {
      type: "addRules",
      behavior: "allow",
      destination: "localSettings",
      rules: [
        { toolName: "Bash", ruleContent: "npm install" },
        { toolName: "WebFetch", ruleContent: "https://example.com" },
        { toolName: "Bash", ruleContent: "dir /B" },
      ],
    },
  ]);
});

test("permissionResultFromOutcome keeps Write allow_session suggestions unchanged", () => {
  const suggestions = [
    {
      type: "addRules" as const,
      behavior: "allow" as const,
      destination: "session" as const,
      rules: [{ toolName: "Write", ruleContent: "C:\\work\\foo.txt" }],
    },
  ];
  const allow = permissionResultFromOutcome(
    { outcome: "selected", option_id: "allow_session" },
    "tool-2",
    { file_path: "C:\\work\\foo.txt" },
    suggestions,
    "Write",
  );

  assert.equal(allow.behavior, "allow");
  if (allow.behavior !== "allow") {
    throw new Error("expected allow permission result");
  }
  assert.deepEqual(allow.updatedPermissions, suggestions);
});

test("permissionResultFromOutcome falls back to session tool rule for allow_session when suggestions are missing", () => {
  const allow = permissionResultFromOutcome(
    { outcome: "selected", option_id: "allow_session" },
    "tool-3",
    { file_path: "C:\\work\\bar.txt" },
    undefined,
    "Write",
  );

  assert.equal(allow.behavior, "allow");
  if (allow.behavior !== "allow") {
    throw new Error("expected allow permission result");
  }
  assert.deepEqual(allow.updatedPermissions, [
    {
      type: "addRules",
      behavior: "allow",
      destination: "session",
      rules: [{ toolName: "Write" }],
    },
  ]);
});

test("permissionResultFromOutcome falls back to localSettings rule for allow_always when only session suggestions exist", () => {
  const allow = permissionResultFromOutcome(
    { outcome: "selected", option_id: "allow_always" },
    "tool-4",
    { file_path: "C:\\work\\baz.txt" },
    [
      {
        type: "addRules",
        behavior: "allow",
        destination: "session",
        rules: [{ toolName: "Write", ruleContent: "C:\\work\\baz.txt" }],
      },
    ],
    "Write",
  );

  assert.equal(allow.behavior, "allow");
  if (allow.behavior !== "allow") {
    throw new Error("expected allow permission result");
  }
  assert.deepEqual(allow.updatedPermissions, [
    {
      type: "addRules",
      rules: [{ toolName: "Write" }],
      behavior: "allow",
      destination: "localSettings",
    },
  ]);
});

test("looksLikeAuthRequired detects login hints", () => {
  assert.equal(looksLikeAuthRequired("Please run /login to continue"), true);
  assert.equal(looksLikeAuthRequired("normal tool output"), false);
});

test("agent sdk version compatibility check matches pinned version", () => {
  assert.equal(resolveInstalledAgentSdkVersion(), "0.2.74");
  assert.equal(agentSdkVersionCompatibilityError(), undefined);
});

test("mapSessionMessagesToUpdates maps message content blocks", () => {
  const updates = mapSessionMessagesToUpdates([
    {
      type: "user",
      uuid: "u1",
      session_id: "s1",
      parent_tool_use_id: null,
      message: {
        role: "user",
        content: [{ type: "text", text: "Top-level user prompt" }],
      },
    },
    {
      type: "assistant",
      uuid: "a1",
      session_id: "s1",
      parent_tool_use_id: null,
      message: {
        id: "msg-1",
        role: "assistant",
        content: [
          { type: "tool_use", id: "tool-1", name: "Bash", input: { command: "echo hello" } },
          { type: "text", text: "Nested assistant final" },
        ],
        usage: {
          input_tokens: 11,
          output_tokens: 7,
          cache_read_input_tokens: 5,
          cache_creation_input_tokens: 3,
        },
      },
    },
    {
      type: "user",
      uuid: "u2",
      session_id: "s1",
      parent_tool_use_id: null,
      message: {
        role: "user",
        content: [
          {
            type: "tool_result",
            tool_use_id: "tool-1",
            content: "ok",
            is_error: false,
          },
        ],
      },
    },
  ]);

  const variantCounts = new Map<string, number>();
  for (const update of updates) {
    variantCounts.set(update.type, (variantCounts.get(update.type) ?? 0) + 1);
  }

  assert.equal(variantCounts.get("user_message_chunk"), 1);
  assert.equal(variantCounts.get("agent_message_chunk"), 1);
  assert.equal(variantCounts.get("tool_call"), 1);
  assert.equal(variantCounts.get("tool_call_update"), 1);
});

test("mapSessionMessagesToUpdates ignores unsupported records", () => {
  const updates = mapSessionMessagesToUpdates([
    {
      type: "user",
      uuid: "u1",
      session_id: "s1",
      parent_tool_use_id: null,
      message: {
        role: "assistant",
        content: [{ type: "thinking", thinking: "h" }],
      },
    },
  ]);
  assert.equal(updates.length, 0);
});

test("mapSdkSessions normalizes and sorts sessions", () => {
  const mapped = mapSdkSessions([
    {
      sessionId: "older",
      summary: " Older summary ",
      lastModified: 100,
      fileSize: 10,
      cwd: "C:/work",
    },
    {
      sessionId: "latest",
      summary: "",
      lastModified: 200,
      fileSize: 20,
      customTitle: "Custom title",
      gitBranch: "main",
      firstPrompt: "hello",
    },
  ]);

  assert.deepEqual(mapped, [
    {
      session_id: "latest",
      summary: "Custom title",
      last_modified_ms: 200,
      file_size_bytes: 20,
      git_branch: "main",
      custom_title: "Custom title",
      first_prompt: "hello",
    },
    {
      session_id: "older",
      summary: "Older summary",
      last_modified_ms: 100,
      file_size_bytes: 10,
      cwd: "C:/work",
    },
  ]);
});

test("buildSessionListOptions scopes repo-local listings to worktrees", () => {
  assert.deepEqual(buildSessionListOptions("C:/repo"), {
    dir: "C:/repo",
    includeWorktrees: true,
    limit: 50,
  });
  assert.deepEqual(buildSessionListOptions(undefined), {
    limit: 50,
  });
});

test("buildToolResultFields extracts ExitPlanMode ultraplan metadata from structured results", () => {
  const base = createToolCall("tc-plan", "ExitPlanMode", {});
  const fields = buildToolResultFields(
    false,
    [{ text: "Plan ready for approval" }],
    base,
    {
      result: {
        plan: "Plan contents",
        isUltraplan: true,
      },
    },
  );

  assert.deepEqual(fields.output_metadata, {
    exit_plan_mode: {
      is_ultraplan: true,
    },
  });
});

test("buildToolResultFields extracts TodoWrite verification metadata from structured results", () => {
  const base = createToolCall("tc-todo", "TodoWrite", {
    todos: [{ content: "Verify changes", status: "pending", activeForm: "Verifying changes" }],
  });
  const fields = buildToolResultFields(
    false,
    [{ text: "Todos have been modified successfully." }],
    base,
    {
      data: {
        oldTodos: [],
        newTodos: [],
        verificationNudgeNeeded: true,
      },
    },
  );

  assert.deepEqual(fields.output_metadata, {
    todo_write: {
      verification_nudge_needed: true,
    },
  });
});

test("mapAvailableModels preserves optional fast and auto mode metadata", () => {
  const mapped = mapAvailableModels([
    {
      value: "sonnet",
      displayName: "Claude Sonnet",
      description: "Balanced model",
      supportsEffort: true,
      supportedEffortLevels: ["low", "medium", "high", "max"],
      supportsAdaptiveThinking: true,
      supportsFastMode: true,
      supportsAutoMode: false,
    },
    {
      value: "haiku",
      displayName: "Claude Haiku",
      description: "Fast model",
      supportsEffort: false,
    },
  ]);

  assert.deepEqual(mapped, [
    {
      id: "sonnet",
      display_name: "Claude Sonnet",
      description: "Balanced model",
      supports_effort: true,
      supported_effort_levels: ["low", "medium", "high"],
      supports_adaptive_thinking: true,
      supports_fast_mode: true,
      supports_auto_mode: false,
    },
    {
      id: "haiku",
      display_name: "Claude Haiku",
      description: "Fast model",
      supports_effort: false,
      supported_effort_levels: [],
    },
  ]);
});
