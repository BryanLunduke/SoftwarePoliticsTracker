import type { SDKSessionInfo, SessionMessage } from "@anthropic-ai/claude-agent-sdk";
import type { SessionListEntry, SessionUpdate, ToolCall } from "../types.js";
import { asRecordOrNull } from "./shared.js";
import { TOOL_RESULT_TYPES, buildToolResultFields, createToolCall, isToolUseBlockType } from "./tooling.js";

function nonEmptyTrimmed(value: unknown): string | undefined {
  if (typeof value !== "string") {
    return undefined;
  }
  const trimmed = value.trim();
  return trimmed.length > 0 ? trimmed : undefined;
}

function messageCandidates(raw: unknown): Record<string, unknown>[] {
  const candidates: Record<string, unknown>[] = [];
  const topLevel = asRecordOrNull(raw);
  if (topLevel) {
    candidates.push(topLevel);
    const nested = asRecordOrNull(topLevel.message);
    if (nested) {
      candidates.push(nested);
    }
  }
  return candidates;
}

function pushResumeTextChunk(updates: SessionUpdate[], role: "user" | "assistant", text: string): void {
  if (!text.trim()) {
    return;
  }
  if (role === "assistant") {
    updates.push({ type: "agent_message_chunk", content: { type: "text", text } });
    return;
  }
  updates.push({ type: "user_message_chunk", content: { type: "text", text } });
}

function pushResumeToolUse(
  updates: SessionUpdate[],
  toolCalls: Map<string, ToolCall>,
  block: Record<string, unknown>,
): void {
  const toolUseId = typeof block.id === "string" ? block.id : "";
  if (!toolUseId) {
    return;
  }
  const name = typeof block.name === "string" ? block.name : "Tool";
  const input = asRecordOrNull(block.input) ?? {};

  const toolCall = createToolCall(toolUseId, name, input);
  toolCall.status = "in_progress";
  toolCalls.set(toolUseId, toolCall);
  updates.push({ type: "tool_call", tool_call: toolCall });
}

function pushResumeToolResult(
  updates: SessionUpdate[],
  toolCalls: Map<string, ToolCall>,
  block: Record<string, unknown>,
): void {
  const toolUseId = typeof block.tool_use_id === "string" ? block.tool_use_id : "";
  if (!toolUseId) {
    return;
  }
  const isError = Boolean(block.is_error);
  const base = toolCalls.get(toolUseId);
  const fields = buildToolResultFields(isError, block.content, base, block);
  updates.push({ type: "tool_call_update", tool_call_update: { tool_call_id: toolUseId, fields } });

  if (!base) {
    return;
  }
  base.status = fields.status ?? base.status;
  if (fields.raw_output) {
    base.raw_output = fields.raw_output;
  }
  if (fields.content) {
    base.content = fields.content;
  }
  if (fields.output_metadata) {
    base.output_metadata = fields.output_metadata;
  }
}

function summaryFromSession(info: SDKSessionInfo): string {
  return (
    nonEmptyTrimmed(info.summary) ??
    nonEmptyTrimmed(info.customTitle) ??
    nonEmptyTrimmed(info.firstPrompt) ??
    info.sessionId
  );
}

export function mapSdkSessionInfo(info: SDKSessionInfo): SessionListEntry {
  return {
    session_id: info.sessionId,
    summary: summaryFromSession(info),
    last_modified_ms: info.lastModified,
    file_size_bytes: info.fileSize,
    ...(nonEmptyTrimmed(info.cwd) ? { cwd: info.cwd?.trim() } : {}),
    ...(nonEmptyTrimmed(info.gitBranch) ? { git_branch: info.gitBranch?.trim() } : {}),
    ...(nonEmptyTrimmed(info.customTitle) ? { custom_title: info.customTitle?.trim() } : {}),
    ...(nonEmptyTrimmed(info.firstPrompt) ? { first_prompt: info.firstPrompt?.trim() } : {}),
  };
}

export function mapSdkSessions(infos: SDKSessionInfo[], limit = 50): SessionListEntry[] {
  const sorted = [...infos].sort((a, b) => b.lastModified - a.lastModified);
  const entries: SessionListEntry[] = [];
  const seen = new Set<string>();
  for (const info of sorted) {
    if (!info.sessionId || seen.has(info.sessionId)) {
      continue;
    }
    seen.add(info.sessionId);
    entries.push(mapSdkSessionInfo(info));
    if (entries.length >= limit) {
      break;
    }
  }
  return entries;
}

export function mapSessionMessagesToUpdates(messages: SessionMessage[]): SessionUpdate[] {
  const updates: SessionUpdate[] = [];
  const toolCalls = new Map<string, ToolCall>();

  for (const entry of messages) {
    const fallbackRole = entry.type === "assistant" ? "assistant" : "user";
    for (const message of messageCandidates(entry.message)) {
      const roleCandidate = message.role;
      const role = roleCandidate === "assistant" || roleCandidate === "user" ? roleCandidate : fallbackRole;

      const content = Array.isArray(message.content) ? message.content : [];
      for (const item of content) {
        const block = asRecordOrNull(item);
        if (!block) {
          continue;
        }
        const blockType = typeof block.type === "string" ? block.type : "";
        if (blockType === "thinking") {
          continue;
        }
        if (blockType === "text" && typeof block.text === "string") {
          pushResumeTextChunk(updates, role, block.text);
          continue;
        }
        if (isToolUseBlockType(blockType) && role === "assistant") {
          pushResumeToolUse(updates, toolCalls, block);
          continue;
        }
        if (TOOL_RESULT_TYPES.has(blockType)) {
          pushResumeToolResult(updates, toolCalls, block);
          continue;
        }
        if (blockType === "image") {
          pushResumeTextChunk(updates, role, "[image]");
        }
      }
    }
  }

  return updates;
}
