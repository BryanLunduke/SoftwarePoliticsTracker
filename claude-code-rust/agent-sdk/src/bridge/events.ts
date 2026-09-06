import { listSessions, type ListSessionsOptions } from "@anthropic-ai/claude-agent-sdk";
import type {
  BridgeEvent,
  BridgeEventEnvelope,
  McpOperationError,
  SessionUpdate,
} from "../types.js";
import { buildModeState } from "./commands.js";
import { mapSdkSessions } from "./history.js";
import type { SessionState } from "./session_lifecycle.js";

const SESSION_LIST_LIMIT = 50;
let sessionListingDir: string | undefined;

export function buildSessionListOptions(
  dir: string | undefined,
  limit = SESSION_LIST_LIMIT,
): ListSessionsOptions {
  return dir ? { dir, includeWorktrees: true, limit } : { limit };
}

export function setSessionListingDir(dir: string | undefined): void {
  sessionListingDir = dir;
}

export function currentSessionListOptions(): ListSessionsOptions {
  return buildSessionListOptions(sessionListingDir);
}

export function writeEvent(event: BridgeEvent, requestId?: string): void {
  const envelope: BridgeEventEnvelope = {
    ...(requestId ? { request_id: requestId } : {}),
    ...event,
  };
  process.stdout.write(`${JSON.stringify(envelope)}\n`);
}

export function failConnection(message: string, requestId?: string): void {
  writeEvent({ event: "connection_failed", message }, requestId);
}

export function slashError(sessionId: string, message: string, requestId?: string): void {
  writeEvent({ event: "slash_error", session_id: sessionId, message }, requestId);
}

export function emitMcpOperationError(
  sessionId: string,
  error: McpOperationError,
  requestId?: string,
): void {
  writeEvent({ event: "mcp_operation_error", session_id: sessionId, error }, requestId);
}

export function emitSessionUpdate(sessionId: string, update: SessionUpdate): void {
  writeEvent({ event: "session_update", session_id: sessionId, update });
}

export function emitConnectEvent(session: SessionState): void {
  const historyUpdates = session.resumeUpdates;
  const connectEvent: BridgeEvent =
    session.connectEvent === "session_replaced"
      ? {
          event: "session_replaced",
          session_id: session.sessionId,
          cwd: session.cwd,
          model_name: session.model,
          available_models: session.availableModels,
          mode: session.mode ? buildModeState(session.mode) : null,
          ...(historyUpdates && historyUpdates.length > 0 ? { history_updates: historyUpdates } : {}),
        }
      : {
          event: "connected",
          session_id: session.sessionId,
          cwd: session.cwd,
          model_name: session.model,
          available_models: session.availableModels,
          mode: session.mode ? buildModeState(session.mode) : null,
          ...(historyUpdates && historyUpdates.length > 0 ? { history_updates: historyUpdates } : {}),
        };
  writeEvent(connectEvent, session.connectRequestId);
  session.connectRequestId = undefined;
  session.connected = true;
  session.authHintSent = false;
  session.resumeUpdates = undefined;

  const staleSessions = session.sessionsToCloseAfterConnect;
  session.sessionsToCloseAfterConnect = undefined;
  if (!staleSessions || staleSessions.length === 0) {
    refreshSessionsList();
    return;
  }
  void (async () => {
    // Lazy import to break circular dependency at module-evaluation time.
    const { sessions, closeSession } = await import("./session_lifecycle.js");
    for (const stale of staleSessions) {
      if (stale === session) {
        continue;
      }
      if (sessions.get(stale.sessionId) === stale) {
        sessions.delete(stale.sessionId);
      }
      await closeSession(stale);
    }
    refreshSessionsList();
  })();
}

export async function emitSessionsList(requestId?: string): Promise<void> {
  try {
    const sdkSessions = await listSessions(currentSessionListOptions());
    writeEvent({ event: "sessions_listed", sessions: mapSdkSessions(sdkSessions, SESSION_LIST_LIMIT) }, requestId);
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    console.error(`[sdk warn] listSessions failed: ${message}`);
    writeEvent({ event: "sessions_listed", sessions: [] }, requestId);
  }
}

export function refreshSessionsList(): void {
  void emitSessionsList().catch(() => {
    // Defensive no-op.
  });
}
