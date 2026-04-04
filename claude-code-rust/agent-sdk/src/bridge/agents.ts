import type { AvailableAgent } from "../types.js";
import { emitSessionUpdate } from "./events.js";
import type { SessionState } from "./session_lifecycle.js";

function availableAgentsSignature(agents: AvailableAgent[]): string {
  return JSON.stringify(agents);
}

function normalizeAvailableAgentName(value: unknown): string {
  if (typeof value !== "string") {
    return "";
  }
  return value.trim();
}

export function mapAvailableAgents(value: unknown): AvailableAgent[] {
  if (!Array.isArray(value)) {
    return [];
  }

  const byName = new Map<string, AvailableAgent>();
  for (const entry of value) {
    if (!entry || typeof entry !== "object") {
      continue;
    }
    const record = entry as Record<string, unknown>;
    const name = normalizeAvailableAgentName(record.name);
    if (!name) {
      continue;
    }
    const description = typeof record.description === "string" ? record.description : "";
    const model = typeof record.model === "string" && record.model.trim().length > 0 ? record.model : undefined;
    const existing = byName.get(name);
    if (!existing) {
      byName.set(name, { name, description, model });
      continue;
    }
    if (existing.description.trim().length === 0 && description.trim().length > 0) {
      existing.description = description;
    }
    if (!existing.model && model) {
      existing.model = model;
    }
  }

  return [...byName.values()].sort((a, b) => a.name.localeCompare(b.name));
}

export function mapAvailableAgentsFromNames(value: unknown): AvailableAgent[] {
  if (!Array.isArray(value)) {
    return [];
  }
  const byName = new Map<string, AvailableAgent>();
  for (const entry of value) {
    const name = normalizeAvailableAgentName(entry);
    if (!name || byName.has(name)) {
      continue;
    }
    byName.set(name, { name, description: "" });
  }
  return [...byName.values()].sort((a, b) => a.name.localeCompare(b.name));
}

export function emitAvailableAgentsIfChanged(session: SessionState, agents: AvailableAgent[]): void {
  const signature = availableAgentsSignature(agents);
  if (session.lastAvailableAgentsSignature === signature) {
    return;
  }
  session.lastAvailableAgentsSignature = signature;
  emitSessionUpdate(session.sessionId, { type: "available_agents_update", agents });
}

export function refreshAvailableAgents(session: SessionState): void {
  if (typeof session.query.supportedAgents !== "function") {
    return;
  }
  void session.query
    .supportedAgents()
    .then((agents) => {
      emitAvailableAgentsIfChanged(session, mapAvailableAgents(agents));
    })
    .catch(() => {
      // Best-effort only.
    });
}
