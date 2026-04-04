import type { BridgeCommand, McpServerConfig, McpServerStatus } from "../types.js";
import { emitMcpOperationError, slashError, writeEvent } from "./events.js";
import type { SessionState } from "./session_lifecycle.js";

type QueryWithMcpAuth = import("@anthropic-ai/claude-agent-sdk").Query & {
  mcpAuthenticate?: (serverName: string) => Promise<unknown>;
  mcpClearAuth?: (serverName: string) => Promise<unknown>;
  mcpSubmitOAuthCallbackUrl?: (serverName: string, callbackUrl: string) => Promise<unknown>;
};

type McpAuthMethodName =
  | "mcpAuthenticate"
  | "mcpClearAuth"
  | "mcpSubmitOAuthCallbackUrl";

export const MCP_STALE_STATUS_REVALIDATION_COOLDOWN_MS = 30_000;
const knownConnectedMcpServers = new Set<string>();

function queryWithMcpAuth(session: SessionState): QueryWithMcpAuth {
  return session.query as QueryWithMcpAuth;
}

async function callMcpAuthMethod(
  session: SessionState,
  methodName: McpAuthMethodName,
  args: string[],
): Promise<unknown> {
  const query = queryWithMcpAuth(session);
  switch (methodName) {
    case "mcpAuthenticate":
      if (typeof query.mcpAuthenticate !== "function") {
        throw new Error("installed SDK does not support mcpAuthenticate");
      }
      return await query.mcpAuthenticate(args[0] ?? "");
    case "mcpClearAuth":
      if (typeof query.mcpClearAuth !== "function") {
        throw new Error("installed SDK does not support mcpClearAuth");
      }
      return await query.mcpClearAuth(args[0] ?? "");
    case "mcpSubmitOAuthCallbackUrl":
      if (typeof query.mcpSubmitOAuthCallbackUrl !== "function") {
        throw new Error("installed SDK does not support mcpSubmitOAuthCallbackUrl");
      }
      return await query.mcpSubmitOAuthCallbackUrl(args[0] ?? "", args[1] ?? "");
  }
}

function extractMcpAuthRedirect(
  serverName: string,
  value: unknown,
): import("../types.js").McpAuthRedirect | null {
  if (!value || typeof value !== "object") {
    return null;
  }
  const authUrl = Reflect.get(value, "authUrl");
  if (typeof authUrl !== "string" || authUrl.trim().length === 0) {
    return null;
  }
  const requiresUserAction = Reflect.get(value, "requiresUserAction");
  return {
    server_name: serverName,
    auth_url: authUrl,
    requires_user_action: requiresUserAction === true,
  };
}

function emitMcpCommandError(
  sessionId: string,
  operation: string,
  message: string,
  requestId?: string,
  serverName?: string,
): void {
  emitMcpOperationError(
    sessionId,
    {
      ...(serverName ? { server_name: serverName } : {}),
      operation,
      message,
    },
    requestId,
  );
}

export async function emitMcpSnapshotEvent(
  session: SessionState,
  requestId?: string,
): Promise<McpServerStatus[]> {
  const servers = await session.query.mcpServerStatus();
  let mapped = servers.map(mapMcpServerStatus);
  mapped = await reconcileSuspiciousMcpStatuses(session, mapped);
  rememberKnownConnectedMcpServers(mapped);
  writeEvent(
    {
      event: "mcp_snapshot",
      session_id: session.sessionId,
      servers: mapped,
    },
    requestId,
  );
  return mapped;
}

export function staleMcpAuthCandidates(
  servers: readonly McpServerStatus[],
  knownConnectedServerNames: ReadonlySet<string>,
  lastRevalidatedAt: ReadonlyMap<string, number>,
  now = Date.now(),
  cooldownMs = MCP_STALE_STATUS_REVALIDATION_COOLDOWN_MS,
): string[] {
  return servers
    .filter((server) => {
      if (server.status !== "needs-auth") {
        return false;
      }
      if (!knownConnectedServerNames.has(server.name)) {
        return false;
      }
      const lastAttempt = lastRevalidatedAt.get(server.name) ?? 0;
      return now - lastAttempt >= cooldownMs;
    })
    .map((server) => server.name);
}

function rememberKnownConnectedMcpServers(servers: readonly McpServerStatus[]): void {
  for (const server of servers) {
    if (server.status === "connected") {
      knownConnectedMcpServers.add(server.name);
    }
  }
}

function forgetKnownConnectedMcpServer(serverName: string): void {
  knownConnectedMcpServers.delete(serverName);
}

async function reconcileSuspiciousMcpStatuses(
  session: SessionState,
  servers: McpServerStatus[],
): Promise<McpServerStatus[]> {
  const candidates = staleMcpAuthCandidates(
    servers,
    knownConnectedMcpServers,
    session.mcpStatusRevalidatedAt,
  );
  if (candidates.length === 0) {
    return servers;
  }

  const now = Date.now();
  for (const serverName of candidates) {
    session.mcpStatusRevalidatedAt.set(serverName, now);
    console.error(
      `[sdk mcp reconcile] session=${session.sessionId} server=${serverName} ` +
        `status=needs-auth reason=previously-connected action=reconnect`,
    );
    try {
      await session.query.reconnectMcpServer(serverName);
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      console.error(
        `[sdk mcp reconcile] session=${session.sessionId} server=${serverName} ` +
          `action=reconnect failed=${message}`,
      );
    }
  }

  return (await session.query.mcpServerStatus()).map(mapMcpServerStatus);
}

function shouldKeepMonitoringMcpAuth(server: McpServerStatus | undefined): boolean {
  return server?.status === "needs-auth" || server?.status === "pending";
}

function scheduleMcpAuthSnapshotMonitor(
  session: SessionState,
  serverName: string,
  attempt = 0,
): void {
  const maxAttempts = 180;
  const delayMs = 1000;
  setTimeout(() => {
    void monitorMcpAuthSnapshot(session, serverName, attempt + 1, maxAttempts, delayMs);
  }, delayMs);
}

async function monitorMcpAuthSnapshot(
  session: SessionState,
  serverName: string,
  attempt: number,
  maxAttempts: number,
  delayMs: number,
): Promise<void> {
  try {
    const servers = await emitMcpSnapshotEvent(session);
    const server = servers.find((candidate) => candidate.name === serverName);
    if (attempt < maxAttempts && shouldKeepMonitoringMcpAuth(server)) {
      setTimeout(() => {
        void monitorMcpAuthSnapshot(session, serverName, attempt + 1, maxAttempts, delayMs);
      }, delayMs);
    }
  } catch {
    if (attempt < maxAttempts) {
      setTimeout(() => {
        void monitorMcpAuthSnapshot(session, serverName, attempt + 1, maxAttempts, delayMs);
      }, delayMs);
    }
  }
}

export async function handleMcpStatusCommand(
  session: SessionState,
  requestId?: string,
): Promise<void> {
  try {
    await emitMcpSnapshotEvent(session, requestId);
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    writeEvent(
      {
        event: "mcp_snapshot",
        session_id: session.sessionId,
        servers: [],
        error: message,
      },
      requestId,
    );
  }
}

export async function handleMcpReconnectCommand(
  session: SessionState,
  command: Extract<BridgeCommand, { command: "mcp_reconnect" }>,
  requestId?: string,
): Promise<void> {
  try {
    await session.query.reconnectMcpServer(command.server_name);
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    emitMcpCommandError(
      command.session_id,
      "reconnect",
      message,
      requestId,
      command.server_name,
    );
  }
}

export async function handleMcpToggleCommand(
  session: SessionState,
  command: Extract<BridgeCommand, { command: "mcp_toggle" }>,
  requestId?: string,
): Promise<void> {
  try {
    await session.query.toggleMcpServer(command.server_name, command.enabled);
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    emitMcpCommandError(command.session_id, "toggle", message, requestId, command.server_name);
  }
}

export async function handleMcpSetServersCommand(
  session: SessionState,
  command: Extract<BridgeCommand, { command: "mcp_set_servers" }>,
  requestId?: string,
): Promise<void> {
  try {
    await session.query.setMcpServers(command.servers as Record<string, McpServerConfig>);
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    slashError(command.session_id, `failed to set MCP servers: ${message}`, requestId);
  }
}

export async function handleMcpAuthenticateCommand(
  session: SessionState,
  command: Extract<BridgeCommand, { command: "mcp_authenticate" }>,
  requestId?: string,
): Promise<void> {
  try {
    const result = await callMcpAuthMethod(session, "mcpAuthenticate", [command.server_name]);
    const redirect = extractMcpAuthRedirect(command.server_name, result);
    if (redirect) {
      writeEvent({
        event: "mcp_auth_redirect",
        session_id: command.session_id,
        redirect,
      });
    }
    scheduleMcpAuthSnapshotMonitor(session, command.server_name);
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    emitMcpCommandError(
      command.session_id,
      "authenticate",
      message,
      requestId,
      command.server_name,
    );
  }
}

export async function handleMcpClearAuthCommand(
  session: SessionState,
  command: Extract<BridgeCommand, { command: "mcp_clear_auth" }>,
  requestId?: string,
): Promise<void> {
  try {
    await callMcpAuthMethod(session, "mcpClearAuth", [command.server_name]);
    forgetKnownConnectedMcpServer(command.server_name);
    session.mcpStatusRevalidatedAt.delete(command.server_name);
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    emitMcpCommandError(
      command.session_id,
      "clear-auth",
      message,
      requestId,
      command.server_name,
    );
  }
}

export async function handleMcpOauthCallbackUrlCommand(
  session: SessionState,
  command: Extract<BridgeCommand, { command: "mcp_oauth_callback_url" }>,
  requestId?: string,
): Promise<void> {
  try {
    await callMcpAuthMethod(session, "mcpSubmitOAuthCallbackUrl", [
      command.server_name,
      command.callback_url,
    ]);
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    emitMcpCommandError(
      command.session_id,
      "submit-callback-url",
      message,
      requestId,
      command.server_name,
    );
  }
}

function mapMcpServerStatus(
  status: Awaited<ReturnType<import("@anthropic-ai/claude-agent-sdk").Query["mcpServerStatus"]>>[number],
): McpServerStatus {
  return {
    name: status.name,
    status: status.status,
    ...(status.serverInfo
      ? {
          server_info: {
            name: status.serverInfo.name,
            version: status.serverInfo.version,
          },
        }
      : {}),
    ...(status.error ? { error: status.error } : {}),
    ...(status.config ? { config: mapMcpServerStatusConfig(status.config) } : {}),
    ...(status.scope ? { scope: status.scope } : {}),
    tools: Array.isArray(status.tools)
      ? status.tools.map((tool) => ({
          name: tool.name,
          ...(tool.description ? { description: tool.description } : {}),
          ...(tool.annotations
            ? {
                annotations: {
                  ...(typeof tool.annotations.readOnly === "boolean"
                    ? { read_only: tool.annotations.readOnly }
                    : {}),
                  ...(typeof tool.annotations.destructive === "boolean"
                    ? { destructive: tool.annotations.destructive }
                    : {}),
                  ...(typeof tool.annotations.openWorld === "boolean"
                    ? { open_world: tool.annotations.openWorld }
                    : {}),
                },
              }
            : {}),
        }))
      : [],
  };
}

function mapMcpServerStatusConfig(
  config: NonNullable<
    Awaited<ReturnType<import("@anthropic-ai/claude-agent-sdk").Query["mcpServerStatus"]>>[number]["config"]
  >,
): import("../types.js").McpServerStatusConfig {
  switch (config.type) {
    case "stdio":
      return {
        type: "stdio",
        command: config.command,
        ...(Array.isArray(config.args) && config.args.length > 0 ? { args: config.args } : {}),
        ...(config.env ? { env: config.env } : {}),
      };
    case "sse":
      return {
        type: "sse",
        url: config.url,
        ...(config.headers ? { headers: config.headers } : {}),
      };
    case "http":
      return {
        type: "http",
        url: config.url,
        ...(config.headers ? { headers: config.headers } : {}),
      };
    case "sdk":
      return {
        type: "sdk",
        name: config.name,
      };
    case "claudeai-proxy":
      return {
        type: "claudeai-proxy",
        url: config.url,
        id: config.id,
      };
    default:
      throw new Error(`unsupported MCP status config: ${JSON.stringify(config)}`);
  }
}
