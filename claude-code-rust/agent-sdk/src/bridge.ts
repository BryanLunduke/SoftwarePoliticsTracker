import { createRequire } from "node:module";
import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import readline from "node:readline";
import { pathToFileURL } from "node:url";
import {
  getSessionMessages,
  listSessions,
  renameSession,
} from "@anthropic-ai/claude-agent-sdk";
import type { BridgeCommand } from "./types.js";
import { parseCommandEnvelope, toPermissionMode, buildModeState } from "./bridge/commands.js";
import {
  writeEvent,
  failConnection,
  slashError,
  emitSessionUpdate,
  emitSessionsList,
  currentSessionListOptions,
  setSessionListingDir,
} from "./bridge/events.js";
import { textFromPrompt } from "./bridge/message_handlers.js";
import {
  sessions,
  sessionById,
  createSession,
  closeSession,
  closeAllSessions,
  handleElicitationResponse,
  handlePermissionResponse,
  handleQuestionResponse,
} from "./bridge/session_lifecycle.js";
import { mapSessionMessagesToUpdates } from "./bridge/history.js";
import {
  MCP_STALE_STATUS_REVALIDATION_COOLDOWN_MS,
  handleMcpAuthenticateCommand,
  handleMcpClearAuthCommand,
  handleMcpOauthCallbackUrlCommand,
  handleMcpReconnectCommand,
  handleMcpSetServersCommand,
  handleMcpStatusCommand,
  handleMcpToggleCommand,
  staleMcpAuthCandidates,
} from "./bridge/mcp.js";

// Re-exports: all symbols that tests and external consumers import from bridge.js.
export { AsyncQueue, logPermissionDebug } from "./bridge/shared.js";
export { asRecordOrNull } from "./bridge/shared.js";
export { CACHE_SPLIT_POLICY, previewKilobyteLabel } from "./bridge/cache_policy.js";
export {
  buildToolResultFields,
  createToolCall,
  normalizeToolKind,
  normalizeToolResultText,
  unwrapToolUseResult,
} from "./bridge/tooling.js";
export { looksLikeAuthRequired } from "./bridge/auth.js";
export { parseCommandEnvelope } from "./bridge/commands.js";
export { buildSessionListOptions } from "./bridge/events.js";
export {
  permissionOptionsFromSuggestions,
  permissionResultFromOutcome,
} from "./bridge/permissions.js";
export {
  mapSessionMessagesToUpdates,
  mapSdkSessions,
} from "./bridge/history.js";
export { handleTaskSystemMessage } from "./bridge/message_handlers.js";
export { mapAvailableAgents } from "./bridge/agents.js";
export { buildQueryOptions, mapAvailableModels } from "./bridge/session_lifecycle.js";
export {
  parseFastModeState,
  parseRateLimitStatus,
  buildRateLimitUpdate,
} from "./bridge/state_parsing.js";
export { MCP_STALE_STATUS_REVALIDATION_COOLDOWN_MS, staleMcpAuthCandidates };
export type {
  SessionState,
  ConnectEventKind,
  PendingPermission,
  PendingQuestion,
} from "./bridge/session_lifecycle.js";

export function buildSessionMutationOptions(
  cwd?: string,
): import("@anthropic-ai/claude-agent-sdk").SessionMutationOptions | undefined {
  return cwd ? { dir: cwd } : undefined;
}

type SessionTitleGeneratingQuery = import("@anthropic-ai/claude-agent-sdk").Query & {
  generateSessionTitle: (
    description: string,
    options?: { persist?: boolean },
  ) => Promise<string | null | undefined>;
};

export function canGenerateSessionTitle(
  query: import("@anthropic-ai/claude-agent-sdk").Query,
): query is SessionTitleGeneratingQuery {
  return typeof (query as { generateSessionTitle?: unknown }).generateSessionTitle === "function";
}

export async function generatePersistedSessionTitle(
  query: import("@anthropic-ai/claude-agent-sdk").Query,
  description: string,
): Promise<string> {
  if (!canGenerateSessionTitle(query)) {
    throw new Error("SDK query does not support generateSessionTitle");
  }
  const title = await query.generateSessionTitle(description, { persist: true });
  if (typeof title !== "string" || title.trim().length === 0) {
    throw new Error("SDK did not return a generated session title");
  }
  return title;
}

const EXPECTED_AGENT_SDK_VERSION = "0.2.74";
const require = createRequire(import.meta.url);

export function resolveInstalledAgentSdkVersion(): string | undefined {
  try {
    const entryPath = require.resolve("@anthropic-ai/claude-agent-sdk");
    const packageJsonPath = join(dirname(entryPath), "package.json");
    const pkg = JSON.parse(readFileSync(packageJsonPath, "utf8")) as { version?: unknown };
    return typeof pkg.version === "string" ? pkg.version : undefined;
  } catch {
    return undefined;
  }
}

export function agentSdkVersionCompatibilityError(): string | undefined {
  const installed = resolveInstalledAgentSdkVersion();
  if (!installed) {
    return (
      `Agent SDK version check failed: unable to resolve installed ` +
      `@anthropic-ai/claude-agent-sdk package.json (expected ${EXPECTED_AGENT_SDK_VERSION}).`
    );
  }
  if (installed === EXPECTED_AGENT_SDK_VERSION) {
    return undefined;
  }
  return (
    `Unsupported @anthropic-ai/claude-agent-sdk version: expected ${EXPECTED_AGENT_SDK_VERSION}, ` +
    `found ${installed}.`
  );
}

async function handleCommand(command: BridgeCommand, requestId?: string): Promise<void> {
  const sdkVersionError = agentSdkVersionCompatibilityError();
  if (sdkVersionError && command.command !== "initialize" && command.command !== "shutdown") {
    failConnection(sdkVersionError, requestId);
    return;
  }

  switch (command.command) {
    case "initialize":
      if (sdkVersionError) {
        failConnection(sdkVersionError, requestId);
        return;
      }
      setSessionListingDir(command.cwd);
      writeEvent(
        {
          event: "initialized",
          result: {
            agent_name: "claude-rs-agent-bridge",
            agent_version: "0.1.0",
            auth_methods: [
              {
                id: "claude-login",
                name: "Log in with Claude",
                description: "Run `claude /login` in a terminal",
              },
            ],
            capabilities: {
              prompt_image: false,
              prompt_embedded_context: true,
              supports_session_listing: true,
              supports_resume_session: true,
            },
          },
        },
        requestId,
      );
      await emitSessionsList(requestId);
      return;

    case "create_session":
      setSessionListingDir(command.cwd);
      await createSession({
        cwd: command.cwd,
        resume: command.resume,
        launchSettings: command.launch_settings,
        connectEvent: "connected",
        requestId,
      });
      return;

    case "resume_session": {
      try {
        const sdkSessions = await listSessions(currentSessionListOptions());
        const matched = sdkSessions.find((entry) => entry.sessionId === command.session_id);
        if (!matched) {
          slashError(command.session_id, `unknown session: ${command.session_id}`, requestId);
          return;
        }
        setSessionListingDir(matched.cwd ?? process.cwd());
        const historyMessages = await getSessionMessages(
          command.session_id,
          matched.cwd ? { dir: matched.cwd } : undefined,
        );
        const resumeUpdates = mapSessionMessagesToUpdates(historyMessages);
        const staleSessions = Array.from(sessions.values());
        const hadActiveSession = staleSessions.length > 0;
        await createSession({
          cwd: matched.cwd ?? process.cwd(),
          resume: command.session_id,
          launchSettings: command.launch_settings,
          ...(resumeUpdates.length > 0 ? { resumeUpdates } : {}),
          connectEvent: hadActiveSession ? "session_replaced" : "connected",
          requestId,
          ...(hadActiveSession ? { sessionsToCloseAfterConnect: staleSessions } : {}),
        });
      } catch (error) {
        const message = error instanceof Error ? error.message : String(error);
        slashError(command.session_id, `failed to resume session: ${message}`, requestId);
      }
      return;
    }

    case "new_session":
      await closeAllSessions();
      setSessionListingDir(command.cwd);
      await createSession({
        cwd: command.cwd,
        launchSettings: command.launch_settings,
        connectEvent: "session_replaced",
        requestId,
      });
      return;

    case "prompt": {
      const session = sessionById(command.session_id);
      if (!session) {
        slashError(command.session_id, `unknown session: ${command.session_id}`, requestId);
        return;
      }
      const text = textFromPrompt(command);
      if (!text.trim()) {
        return;
      }
      session.input.enqueue({
        type: "user",
        session_id: session.sessionId,
        parent_tool_use_id: null,
        message: {
          role: "user",
          content: [{ type: "text", text }],
        },
      } as import("@anthropic-ai/claude-agent-sdk").SDKUserMessage);
      return;
    }

    case "cancel_turn": {
      const session = sessionById(command.session_id);
      if (!session) {
        slashError(command.session_id, `unknown session: ${command.session_id}`, requestId);
        return;
      }
      await session.query.interrupt();
      return;
    }

    case "set_model": {
      const session = sessionById(command.session_id);
      if (!session) {
        slashError(command.session_id, `unknown session: ${command.session_id}`, requestId);
        return;
      }
      await session.query.setModel(command.model);
      session.model = command.model;
      emitSessionUpdate(session.sessionId, {
        type: "config_option_update",
        option_id: "model",
        value: command.model,
      });
      return;
    }

    case "set_mode": {
      const session = sessionById(command.session_id);
      if (!session) {
        slashError(command.session_id, `unknown session: ${command.session_id}`, requestId);
        return;
      }
      const mode = toPermissionMode(command.mode);
      if (!mode) {
        slashError(command.session_id, `unsupported mode: ${command.mode}`, requestId);
        return;
      }
      await session.query.setPermissionMode(mode);
      session.mode = mode;
      emitSessionUpdate(session.sessionId, {
        type: "current_mode_update",
        current_mode_id: mode,
      });
      return;
    }

    case "generate_session_title": {
      const session = sessionById(command.session_id);
      if (!session) {
        slashError(command.session_id, `unknown session: ${command.session_id}`, requestId);
        return;
      }
      try {
        await generatePersistedSessionTitle(session.query, command.description);
        setSessionListingDir(session.cwd);
        await emitSessionsList(requestId);
      } catch (error) {
        const message = error instanceof Error ? error.message : String(error);
        slashError(command.session_id, `failed to generate session title: ${message}`, requestId);
      }
      return;
    }

    case "rename_session": {
      const session = sessionById(command.session_id);
      if (!session) {
        slashError(command.session_id, `unknown session: ${command.session_id}`, requestId);
        return;
      }
      try {
        await renameSession(
          command.session_id,
          command.title,
          buildSessionMutationOptions(session.cwd),
        );
        setSessionListingDir(session.cwd);
        await emitSessionsList(requestId);
      } catch (error) {
        const message = error instanceof Error ? error.message : String(error);
        slashError(command.session_id, `failed to rename session: ${message}`, requestId);
      }
      return;
    }

    case "get_status_snapshot": {
      const session = sessionById(command.session_id);
      if (!session) {
        slashError(command.session_id, `unknown session: ${command.session_id}`, requestId);
        return;
      }
      const account = await session.query.accountInfo();
      writeEvent(
        {
          event: "status_snapshot",
          session_id: session.sessionId,
          account: {
            email: account.email,
            organization: account.organization,
            subscription_type: account.subscriptionType,
            token_source: account.tokenSource,
            api_key_source: account.apiKeySource,
          },
        },
        requestId,
      );
      return;
    }

    case "mcp_status": {
      const session = sessionById(command.session_id);
      if (!session) {
        slashError(command.session_id, `unknown session: ${command.session_id}`, requestId);
        return;
      }
      await handleMcpStatusCommand(session, requestId);
      return;
    }

    case "mcp_reconnect": {
      const session = sessionById(command.session_id);
      if (!session) {
        slashError(command.session_id, `unknown session: ${command.session_id}`, requestId);
        return;
      }
      await handleMcpReconnectCommand(session, command, requestId);
      return;
    }

    case "mcp_toggle": {
      const session = sessionById(command.session_id);
      if (!session) {
        slashError(command.session_id, `unknown session: ${command.session_id}`, requestId);
        return;
      }
      await handleMcpToggleCommand(session, command, requestId);
      return;
    }

    case "mcp_set_servers": {
      const session = sessionById(command.session_id);
      if (!session) {
        slashError(command.session_id, `unknown session: ${command.session_id}`, requestId);
        return;
      }
      await handleMcpSetServersCommand(session, command, requestId);
      return;
    }

    case "mcp_authenticate": {
      const session = sessionById(command.session_id);
      if (!session) {
        slashError(command.session_id, `unknown session: ${command.session_id}`, requestId);
        return;
      }
      await handleMcpAuthenticateCommand(session, command, requestId);
      return;
    }

    case "mcp_clear_auth": {
      const session = sessionById(command.session_id);
      if (!session) {
        slashError(command.session_id, `unknown session: ${command.session_id}`, requestId);
        return;
      }
      await handleMcpClearAuthCommand(session, command, requestId);
      return;
    }

    case "mcp_oauth_callback_url": {
      const session = sessionById(command.session_id);
      if (!session) {
        slashError(command.session_id, `unknown session: ${command.session_id}`, requestId);
        return;
      }
      await handleMcpOauthCallbackUrlCommand(session, command, requestId);
      return;
    }

    case "permission_response":
      handlePermissionResponse(command);
      return;

    case "question_response":
      handleQuestionResponse(command);
      return;

    case "elicitation_response":
      handleElicitationResponse(command);
      return;

    case "shutdown":
      await closeAllSessions();
      process.exit(0);

    default:
      failConnection(`unhandled command: ${(command as { command?: string }).command ?? "unknown"}`, requestId);
  }
}

function main(): void {
  const rl = readline.createInterface({
    input: process.stdin,
    crlfDelay: Number.POSITIVE_INFINITY,
  });

  rl.on("line", (line) => {
    if (line.trim().length === 0) {
      return;
    }
    void (async () => {
      let parsed: { requestId?: string; command: BridgeCommand };
      try {
        parsed = parseCommandEnvelope(line);
      } catch (error) {
        const message = error instanceof Error ? error.message : String(error);
        failConnection(`invalid command envelope: ${message}`);
        return;
      }

      try {
        await handleCommand(parsed.command, parsed.requestId);
      } catch (error) {
        const message = error instanceof Error ? error.message : String(error);
        failConnection(
          `bridge command failed (${parsed.command.command}): ${message}`,
          parsed.requestId,
        );
      }
    })();
  });

  rl.on("close", () => {
    void closeAllSessions().finally(() => process.exit(0));
  });
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main();
}
