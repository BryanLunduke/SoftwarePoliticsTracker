import type { PlanEntry, ToolCall, ToolCallUpdateFields } from "../types.js";
import { emitSessionUpdate } from "./events.js";
import type { SessionState } from "./session_lifecycle.js";
import { buildToolResultFields, createToolCall } from "./tooling.js";

export function emitToolCall(session: SessionState, toolUseId: string, name: string, input: Record<string, unknown>): void {
  const toolCall = createToolCall(toolUseId, name, input);
  const status: ToolCall["status"] = "in_progress";
  toolCall.status = status;

  const existing = session.toolCalls.get(toolUseId);
  if (!existing) {
    session.toolCalls.set(toolUseId, toolCall);
    emitSessionUpdate(session.sessionId, { type: "tool_call", tool_call: toolCall });
    return;
  }

  const fields: ToolCallUpdateFields = {
    title: toolCall.title,
    kind: toolCall.kind,
    status,
    raw_input: toolCall.raw_input,
    locations: toolCall.locations,
    meta: toolCall.meta,
  };
  if (toolCall.content.length > 0) {
    fields.content = toolCall.content;
  }
  emitSessionUpdate(session.sessionId, {
    type: "tool_call_update",
    tool_call_update: { tool_call_id: toolUseId, fields },
  });

  existing.title = toolCall.title;
  existing.kind = toolCall.kind;
  existing.status = status;
  existing.raw_input = toolCall.raw_input;
  existing.locations = toolCall.locations;
  existing.meta = toolCall.meta;
  if (toolCall.content.length > 0) {
    existing.content = toolCall.content;
  }
}

export function ensureToolCallVisible(
  session: SessionState,
  toolUseId: string,
  toolName: string,
  input: Record<string, unknown>,
): ToolCall {
  const existing = session.toolCalls.get(toolUseId);
  if (existing) {
    return existing;
  }
  const toolCall = createToolCall(toolUseId, toolName, input);
  session.toolCalls.set(toolUseId, toolCall);
  emitSessionUpdate(session.sessionId, { type: "tool_call", tool_call: toolCall });
  return toolCall;
}

export function emitPlanIfTodoWrite(session: SessionState, name: string, input: Record<string, unknown>): void {
  if (name !== "TodoWrite" || !Array.isArray(input.todos)) {
    return;
  }
  const entries: PlanEntry[] = input.todos
    .map((todo) => {
      if (!todo || typeof todo !== "object") {
        return null;
      }
      const todoObj = todo as Record<string, unknown>;
      const content = typeof todoObj.content === "string" ? todoObj.content : "";
      const status = typeof todoObj.status === "string" ? todoObj.status : "pending";
      if (!content) {
        return null;
      }
      return { content, status, active_form: status };
    })
    .filter((entry): entry is PlanEntry => entry !== null);

  if (entries.length > 0) {
    emitSessionUpdate(session.sessionId, { type: "plan", entries });
  }
}

export function emitToolResultUpdate(
  session: SessionState,
  toolUseId: string,
  isError: boolean,
  rawContent: unknown,
  rawResult: unknown = rawContent,
): void {
  const base = session.toolCalls.get(toolUseId);
  const fields = buildToolResultFields(isError, rawContent, base, rawResult);
  const update = { tool_call_id: toolUseId, fields };
  emitSessionUpdate(session.sessionId, { type: "tool_call_update", tool_call_update: update });

  if (base) {
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
}

export function finalizeOpenToolCalls(session: SessionState, status: "completed" | "failed"): void {
  for (const [toolUseId, toolCall] of session.toolCalls) {
    if (toolCall.status !== "pending" && toolCall.status !== "in_progress") {
      continue;
    }
    const fields: ToolCallUpdateFields = { status };
    emitSessionUpdate(session.sessionId, {
      type: "tool_call_update",
      tool_call_update: { tool_call_id: toolUseId, fields },
    });
    toolCall.status = status;
  }
}

export function emitToolProgressUpdate(session: SessionState, toolUseId: string, toolName: string): void {
  const existing = session.toolCalls.get(toolUseId);
  if (!existing) {
    emitToolCall(session, toolUseId, toolName, {});
    return;
  }
  if (
    existing.status === "in_progress" ||
    existing.status === "completed" ||
    existing.status === "failed"
  ) {
    return;
  }

  const fields: ToolCallUpdateFields = { status: "in_progress" };
  emitSessionUpdate(session.sessionId, {
    type: "tool_call_update",
    tool_call_update: { tool_call_id: toolUseId, fields },
  });
  existing.status = "in_progress";
}

export function emitToolSummaryUpdate(session: SessionState, toolUseId: string, summary: string): void {
  const base = session.toolCalls.get(toolUseId);
  if (!base) {
    return;
  }
  const fields: ToolCallUpdateFields = {
    status: base.status === "failed" ? "failed" : "completed",
    raw_output: summary,
    content: [{ type: "content", content: { type: "text", text: summary } }],
  };
  emitSessionUpdate(session.sessionId, {
    type: "tool_call_update",
    tool_call_update: { tool_call_id: toolUseId, fields },
  });
  base.status = fields.status ?? base.status;
  base.raw_output = summary;
}

export function setToolCallStatus(
  session: SessionState,
  toolUseId: string,
  status: "pending" | "in_progress" | "completed" | "failed",
  message?: string,
): void {
  const base = session.toolCalls.get(toolUseId);
  if (!base) {
    return;
  }

  const fields: ToolCallUpdateFields = { status };
  if (message && message.length > 0) {
    fields.raw_output = message;
    fields.content = [{ type: "content", content: { type: "text", text: message } }];
  }
  emitSessionUpdate(session.sessionId, {
    type: "tool_call_update",
    tool_call_update: { tool_call_id: toolUseId, fields },
  });
  base.status = status;
  if (fields.raw_output) {
    base.raw_output = fields.raw_output;
  }
}

export function resolveTaskToolUseId(session: SessionState, msg: Record<string, unknown>): string {
  const direct = typeof msg.tool_use_id === "string" ? msg.tool_use_id : "";
  if (direct) {
    return direct;
  }
  const taskId = typeof msg.task_id === "string" ? msg.task_id : "";
  if (!taskId) {
    return "";
  }
  return session.taskToolUseIds.get(taskId) ?? "";
}

export function taskProgressText(msg: Record<string, unknown>): string {
  const summary = typeof msg.summary === "string" ? msg.summary.trim() : "";
  if (summary) {
    return summary;
  }
  const description = typeof msg.description === "string" ? msg.description : "";
  const lastTool = typeof msg.last_tool_name === "string" ? msg.last_tool_name : "";
  if (description && lastTool) {
    return `${description} (last tool: ${lastTool})`;
  }
  return description || lastTool;
}
