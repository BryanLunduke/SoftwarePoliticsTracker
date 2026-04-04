import type { Json, ToolCall, ToolCallUpdateFields } from "../types.js";
import { asRecordOrNull } from "./shared.js";
import { CACHE_SPLIT_POLICY, previewKilobyteLabel } from "./cache_policy.js";

export const TOOL_RESULT_TYPES = new Set([
  "tool_result",
  "tool_search_tool_result",
  "web_fetch_tool_result",
  "web_search_tool_result",
  "code_execution_tool_result",
  "bash_code_execution_tool_result",
  "text_editor_code_execution_tool_result",
  "mcp_tool_result",
]);

export function isToolUseBlockType(blockType: string): boolean {
  return blockType === "tool_use" || blockType === "server_tool_use" || blockType === "mcp_tool_use";
}

export function normalizeToolKind(name: string): string {
  switch (name) {
    case "Bash":
      return "execute";
    case "Read":
    case "ReadMcpResource":
      return "read";
    case "Write":
    case "Edit":
      return "edit";
    case "Delete":
      return "delete";
    case "Move":
      return "move";
    case "Glob":
    case "Grep":
      return "search";
    case "WebFetch":
      return "fetch";
    case "TodoWrite":
      return "other";
    case "Task":
    case "Agent":
      return "think";
    case "ExitPlanMode":
      return "switch_mode";
    default:
      return "think";
  }
}

export function toolTitle(name: string, input: Record<string, unknown>): string {
  if (name === "Bash") {
    const command = typeof input.command === "string" ? input.command : "";
    return command || "Terminal";
  }
  if (name === "Glob") {
    const pattern = typeof input.pattern === "string" ? input.pattern : "";
    const path = typeof input.path === "string" ? input.path : "";
    if (pattern && path) {
      return `Glob ${pattern} in ${path}`;
    }
    if (pattern) {
      return `Glob ${pattern}`;
    }
    if (path) {
      return `Glob ${path}`;
    }
  }
  if (name === "WebFetch") {
    const url = typeof input.url === "string" ? input.url : "";
    if (url) {
      return `WebFetch ${url}`;
    }
  }
  if (name === "WebSearch") {
    const query = typeof input.query === "string" ? input.query : "";
    if (query) {
      return `WebSearch ${query}`;
    }
  }
  if ((name === "Read" || name === "Write" || name === "Edit") && typeof input.file_path === "string") {
    return `${name} ${input.file_path}`;
  }
  if (name === "ReadMcpResource") {
    const uri = typeof input.uri === "string" ? input.uri : "";
    const server = typeof input.server === "string" ? input.server : "";
    if (server && uri) {
      return `ReadMcpResource ${server} ${uri}`;
    }
    if (uri) {
      return `ReadMcpResource ${uri}`;
    }
  }
  return name;
}

function editDiffContent(name: string, input: Record<string, unknown>): ToolCall["content"] {
  const filePath = typeof input.file_path === "string" ? input.file_path : "";
  if (!filePath) {
    return [];
  }

  if (name === "Edit") {
    const oldText = typeof input.old_string === "string" ? input.old_string : "";
    const newText = typeof input.new_string === "string" ? input.new_string : "";
    if (!oldText && !newText) {
      return [];
    }
    return [{ type: "diff", old_path: filePath, new_path: filePath, old: oldText, new: newText }];
  }

  if (name === "Write") {
    const newText = typeof input.content === "string" ? input.content : "";
    if (!newText) {
      return [];
    }
    return [{ type: "diff", old_path: filePath, new_path: filePath, old: "", new: newText }];
  }

  return [];
}

export function createToolCall(toolUseId: string, name: string, input: Record<string, unknown>): ToolCall {
  return {
    tool_call_id: toolUseId,
    title: toolTitle(name, input),
    kind: normalizeToolKind(name),
    status: "pending",
    content: editDiffContent(name, input),
    raw_input: input as unknown as Json,
    locations: typeof input.file_path === "string" ? [{ path: input.file_path }] : [],
    meta: {
      claudeCode: {
        toolName: name,
      },
    },
  };
}

function resultRecordCandidates(rawResult: unknown, rawContent: unknown): Record<string, unknown>[] {
  const candidates: Record<string, unknown>[] = [];

  const pushRecord = (value: unknown): void => {
    const record = asRecordOrNull(value);
    if (record) {
      candidates.push(record);
    }
  };

  const pushNestedRecords = (value: unknown): void => {
    const record = asRecordOrNull(value);
    if (!record) {
      return;
    }
    pushRecord(record.result);
    pushRecord(record.data);
    pushRecord(record.content);
  };

  pushRecord(rawResult);
  pushNestedRecords(rawResult);
  pushRecord(rawContent);
  pushNestedRecords(rawContent);

  return candidates;
}

function parseJsonCandidate(value: unknown): unknown {
  const text = typeof value === "string" ? value : extractText(value);
  const trimmed = text.trim();
  if (!(trimmed.startsWith("{") || trimmed.startsWith("["))) {
    return undefined;
  }
  try {
    return JSON.parse(trimmed);
  } catch {
    return undefined;
  }
}

function pushStructuredRecordCandidates(
  candidates: Record<string, unknown>[],
  value: unknown,
): void {
  const record = asRecordOrNull(value);
  if (!record) {
    return;
  }
  candidates.push(record);

  const nestedResult = asRecordOrNull(record.result);
  if (nestedResult) {
    candidates.push(nestedResult);
  }
  const nestedData = asRecordOrNull(record.data);
  if (nestedData) {
    candidates.push(nestedData);
  }
  const nestedContent = asRecordOrNull(record.content);
  if (nestedContent) {
    candidates.push(nestedContent);
  }
}

function mcpResourceContentFromResult(rawResult: unknown, rawContent: unknown): ToolCall["content"] {
  const candidates: Record<string, unknown>[] = [];
  for (const candidate of [rawResult, rawContent, parseJsonCandidate(rawResult), parseJsonCandidate(rawContent)]) {
    pushStructuredRecordCandidates(candidates, candidate);
  }

  for (const candidate of candidates) {
    const contents = Array.isArray(candidate.contents) ? candidate.contents : null;
    if (!contents || contents.length === 0) {
      continue;
    }

    const mapped: ToolCall["content"] = [];
    for (const entry of contents) {
      const record = asRecordOrNull(entry);
      if (!record) {
        continue;
      }
      const uri = typeof record.uri === "string" ? record.uri : "";
      if (!uri) {
        continue;
      }
      const text =
        typeof record.text === "string" && record.text.length > 0 ? record.text : undefined;
      const mimeType =
        typeof record.mimeType === "string" && record.mimeType.trim().length > 0
          ? record.mimeType.trim()
          : undefined;
      const blobSavedTo =
        typeof record.blobSavedTo === "string" && record.blobSavedTo.trim().length > 0
          ? record.blobSavedTo.trim()
          : undefined;
      if (!text && !blobSavedTo) {
        continue;
      }
      mapped.push({
        type: "mcp_resource",
        uri,
        ...(mimeType ? { mime_type: mimeType } : {}),
        ...(text ? { text } : {}),
        ...(blobSavedTo ? { blob_saved_to: blobSavedTo } : {}),
      });
    }

    if (mapped.length > 0) {
      return mapped;
    }
  }

  return [];
}

function extractToolOutputMetadata(
  toolName: string,
  rawResult: unknown,
  rawContent: unknown,
): import("../types.js").ToolOutputMetadata | undefined {
  const candidates = resultRecordCandidates(rawResult, rawContent);

  if (toolName === "Bash") {
    for (const candidate of candidates) {
      const hasAssistantAutoBackgrounded = typeof candidate.assistantAutoBackgrounded === "boolean";
      const hasTokenSaverOutput =
        typeof candidate.tokenSaverOutput === "string" && candidate.tokenSaverOutput.length > 0;
      if (hasAssistantAutoBackgrounded || hasTokenSaverOutput) {
        const bashMetadata: import("../types.js").BashOutputMetadata = {};
        if (hasAssistantAutoBackgrounded) {
          bashMetadata.assistant_auto_backgrounded = candidate.assistantAutoBackgrounded as boolean;
        }
        if (hasTokenSaverOutput) {
          bashMetadata.token_saver_active = true;
        }
        return {
          bash: bashMetadata,
        };
      }
    }
    return undefined;
  }

  if (toolName === "ExitPlanMode") {
    for (const candidate of candidates) {
      if (typeof candidate.isUltraplan === "boolean") {
        return { exit_plan_mode: { is_ultraplan: candidate.isUltraplan } };
      }
    }
    return undefined;
  }

  if (toolName === "TodoWrite") {
    for (const candidate of candidates) {
      if (typeof candidate.verificationNudgeNeeded === "boolean") {
        return {
          todo_write: { verification_nudge_needed: candidate.verificationNudgeNeeded },
        };
      }
    }
  }

  return undefined;
}

export function extractText(value: unknown): string {
  if (typeof value === "string") {
    return value;
  }
  if (Array.isArray(value)) {
    return value
      .map((entry) => {
        if (typeof entry === "string") {
          return entry;
        }
        if (entry && typeof entry === "object" && "text" in entry && typeof entry.text === "string") {
          return entry.text;
        }
        return "";
      })
      .filter((part) => part.length > 0)
      .join("\n");
  }
  if (value && typeof value === "object" && "text" in value && typeof value.text === "string") {
    return value.text;
  }
  return "";
}

const PERSISTED_OUTPUT_OPEN_TAG = "<persisted-output>";
const PERSISTED_OUTPUT_CLOSE_TAG = "</persisted-output>";
const EXPECTED_PREVIEW_LINE = `preview (first ${previewKilobyteLabel(CACHE_SPLIT_POLICY).toLowerCase()}):`;

function extractPersistedOutputInnerText(text: string): string | null {
  const lower = text.toLowerCase();
  const openIdx = lower.indexOf(PERSISTED_OUTPUT_OPEN_TAG);
  if (openIdx < 0) {
    return null;
  }
  const bodyStart = openIdx + PERSISTED_OUTPUT_OPEN_TAG.length;
  const closeIdx = lower.indexOf(PERSISTED_OUTPUT_CLOSE_TAG, bodyStart);
  if (closeIdx < 0) {
    return null;
  }
  return text.slice(bodyStart, closeIdx);
}

function persistedOutputFirstLine(text: string): string | null {
  const inner = extractPersistedOutputInnerText(text);
  if (inner === null) {
    return null;
  }

  for (const line of inner.split(/\r?\n/)) {
    const cleaned = line.replace(/^[\s|│┃║]+/u, "").trim();
    if (cleaned.length > 0) {
      if (cleaned.toLowerCase() === EXPECTED_PREVIEW_LINE) {
        continue;
      }
      return cleaned;
    }
  }
  return null;
}

/**
 * Replace verbose SDK-internal tool rejection messages with short user-facing text.
 * The SDK sends these as tool result content meant for Claude, not for the user.
 */
const USER_REJECTED_TOOL_USE_EXACT =
  "The user doesn't want to proceed with this tool use. The tool use was rejected (eg. if it was a file edit, the new_string was NOT written to the file). STOP what you are doing and wait for the user to tell you how to proceed.";
const USER_REJECTED_TOOL_USE_PREFIX =
  "The user doesn't want to proceed with this tool use. The tool use was rejected (eg. if it was a file edit, the new_string was NOT written to the file). To tell you how to proceed, the user said:";
const PERMISSION_DENIED_TOOL_USE_EXACT =
  "Permission for this tool use was denied. The tool use was rejected (eg. if it was a file edit, the new_string was NOT written to the file). Try a different approach or report the limitation to complete your task.";
const PERMISSION_DENIED_TOOL_USE_PREFIX =
  "Permission for this tool use was denied. The tool use was rejected (eg. if it was a file edit, the new_string was NOT written to the file). The user said:";

function sanitizeSdkRejectionText(text: string): string {
  const normalized = text.trim();
  if (
    normalized === USER_REJECTED_TOOL_USE_EXACT ||
    normalized.startsWith(USER_REJECTED_TOOL_USE_PREFIX)
  ) {
    return "Cancelled by user.";
  }
  if (
    normalized === PERMISSION_DENIED_TOOL_USE_EXACT ||
    normalized.startsWith(PERMISSION_DENIED_TOOL_USE_PREFIX)
  ) {
    return "Permission denied.";
  }
  return text;
}

export function normalizeToolResultText(value: unknown, isError = false): string {
  const text = extractText(value);
  if (!text) {
    return "";
  }
  const persistedLine = persistedOutputFirstLine(text);
  const normalized = persistedLine || text;
  if (!isError) {
    return normalized;
  }
  return sanitizeSdkRejectionText(normalized);
}

function resolveToolName(toolCall: ToolCall | undefined): string {
  const meta = asRecordOrNull(toolCall?.meta);
  const claudeCode = asRecordOrNull(meta?.claudeCode);
  const toolName = claudeCode?.toolName;
  return typeof toolName === "string" ? toolName : "";
}

function writeDiffFromInput(rawInput: Json | undefined): ToolCall["content"] {
  const input = asRecordOrNull(rawInput);
  if (!input) {
    return [];
  }
  const filePath = typeof input.file_path === "string" ? input.file_path : "";
  const content = typeof input.content === "string" ? input.content : "";
  if (!filePath || !content) {
    return [];
  }
  return [{ type: "diff", old_path: filePath, new_path: filePath, old: "", new: content }];
}

function editDiffFromInput(rawInput: Json | undefined): ToolCall["content"] {
  const input = asRecordOrNull(rawInput);
  if (!input) {
    return [];
  }
  const filePath = typeof input.file_path === "string" ? input.file_path : "";
  const oldText =
    typeof input.old_string === "string"
      ? input.old_string
      : typeof input.oldString === "string"
        ? input.oldString
        : "";
  const newText =
    typeof input.new_string === "string"
      ? input.new_string
      : typeof input.newString === "string"
        ? input.newString
        : "";
  if (!filePath || (!oldText && !newText)) {
    return [];
  }
  return [{ type: "diff", old_path: filePath, new_path: filePath, old: oldText, new: newText }];
}

function writeDiffFromResult(rawContent: unknown): ToolCall["content"] {
  const candidates = Array.isArray(rawContent) ? rawContent : [rawContent];
  for (const candidate of candidates) {
    const record = asRecordOrNull(candidate);
    if (!record) {
      continue;
    }
    const filePath =
      typeof record.filePath === "string"
        ? record.filePath
        : typeof record.file_path === "string"
          ? record.file_path
          : "";
    const content = typeof record.content === "string" ? record.content : "";
    const originalRaw =
      "originalFile" in record ? record.originalFile : "original_file" in record ? record.original_file : undefined;
    const gitDiff = asRecordOrNull(record.gitDiff);
    const repository =
      typeof gitDiff?.repository === "string" && gitDiff.repository.trim().length > 0
        ? gitDiff.repository.trim()
        : undefined;
    if (!filePath || !content || originalRaw === undefined) {
      continue;
    }
    const original = typeof originalRaw === "string" ? originalRaw : originalRaw === null ? "" : "";
    return [
      {
        type: "diff",
        old_path: filePath,
        new_path: filePath,
        old: original,
        new: content,
        ...(repository ? { repository } : {}),
      },
    ];
  }
  return [];
}

function editDiffFromResult(rawResult: unknown, rawInput: Json | undefined): ToolCall["content"] {
  const input = asRecordOrNull(rawInput);
  const filePath = typeof input?.file_path === "string" ? input.file_path : "";
  const oldText =
    typeof input?.old_string === "string"
      ? input.old_string
      : typeof input?.oldString === "string"
        ? input.oldString
        : "";
  const newText =
    typeof input?.new_string === "string"
      ? input.new_string
      : typeof input?.newString === "string"
        ? input.newString
        : "";
  if (!filePath || (!oldText && !newText)) {
    return [];
  }

  for (const candidate of resultRecordCandidates(rawResult, undefined)) {
    const candidatePath =
      typeof candidate.filePath === "string"
        ? candidate.filePath
        : typeof candidate.file_path === "string"
          ? candidate.file_path
          : "";
    const gitDiff = asRecordOrNull(candidate.gitDiff);
    if (!candidatePath && !gitDiff) {
      continue;
    }
    if (candidatePath && candidatePath !== filePath) {
      continue;
    }
    const repository =
      typeof gitDiff?.repository === "string" && gitDiff.repository.trim().length > 0
        ? gitDiff.repository.trim()
        : undefined;
    return [
      {
        type: "diff",
        old_path: filePath,
        new_path: filePath,
        old: oldText,
        new: newText,
        ...(repository ? { repository } : {}),
      },
    ];
  }

  return editDiffFromInput(rawInput);
}

function findBashResultRecord(
  rawResult: unknown,
  rawContent: unknown,
): Record<string, unknown> | undefined {
  return resultRecordCandidates(rawResult, rawContent).find(
    (candidate) =>
      "stdout" in candidate ||
      "stderr" in candidate ||
      "backgroundTaskId" in candidate ||
      "backgroundedByUser" in candidate ||
      "assistantAutoBackgrounded" in candidate ||
      "tokenSaverOutput" in candidate,
  );
}

function bashBackgroundMessage(record: Record<string, unknown>): string {
  const backgroundTaskId =
    typeof record.backgroundTaskId === "string" ? record.backgroundTaskId : "";
  if (!backgroundTaskId) {
    return "";
  }
  if (record.assistantAutoBackgrounded === true) {
    return `Command was auto-backgrounded by assistant mode with ID: ${backgroundTaskId}.`;
  }
  if (record.backgroundedByUser === true) {
    return `Command was backgrounded by user with ID: ${backgroundTaskId}.`;
  }
  return `Command is running in background with ID: ${backgroundTaskId}.`;
}

function buildBashDisplayOutput(record: Record<string, unknown>): string {
  const segments: string[] = [];
  const stdout = typeof record.stdout === "string" ? record.stdout : "";
  const stderr = typeof record.stderr === "string" ? record.stderr : "";
  if (stdout) {
    segments.push(stdout);
  }
  if (stderr) {
    segments.push(stderr);
  }
  if (record.interrupted === true) {
    segments.push("Command was aborted before completion.");
  }
  const backgroundMessage = bashBackgroundMessage(record);
  if (backgroundMessage) {
    segments.push(backgroundMessage);
  }
  return segments.join("\n");
}

export function buildToolResultFields(
  isError: boolean,
  rawContent: unknown,
  base?: ToolCall,
  rawResult?: unknown,
): ToolCallUpdateFields {
  const toolName = resolveToolName(base);
  const bashResultRecord = toolName === "Bash" ? findBashResultRecord(rawResult, rawContent) : undefined;
  const normalizedRawOutput = normalizeToolResultText(rawContent, isError);
  const rawOutput = bashResultRecord
    ? buildBashDisplayOutput(bashResultRecord)
    : normalizedRawOutput || JSON.stringify(rawContent);
  const fields: ToolCallUpdateFields = {
    status: isError ? "failed" : "completed",
  };
  if (rawOutput) {
    fields.raw_output = rawOutput;
  }
  const outputMetadata = extractToolOutputMetadata(toolName, rawResult, rawContent);
  if (outputMetadata) {
    fields.output_metadata = outputMetadata;
  }

  if (!isError && toolName === "Write") {
    const structuredDiff = writeDiffFromResult(rawContent);
    if (structuredDiff.length > 0) {
      fields.content = structuredDiff;
      return fields;
    }
    const inputDiff = writeDiffFromInput(base?.raw_input);
    if (inputDiff.length > 0) {
      fields.content = inputDiff;
      return fields;
    }
  }

  if (!isError && toolName === "Edit") {
    const structuredDiff = editDiffFromResult(rawResult, base?.raw_input);
    if (structuredDiff.length > 0) {
      fields.content = structuredDiff;
      return fields;
    }
    if (base?.content.some((entry) => entry.type === "diff")) {
      return fields;
    }
  }

  if (!isError && toolName === "ReadMcpResource") {
    const structuredResourceContent = mcpResourceContentFromResult(rawResult, rawContent);
    if (structuredResourceContent.length > 0) {
      fields.content = structuredResourceContent;
      return fields;
    }
  }

  if (rawOutput) {
    fields.content = [{ type: "content", content: { type: "text", text: rawOutput } }];
  }
  return fields;
}

export function unwrapToolUseResult(rawResult: unknown): { isError: boolean; content: unknown } {
  if (!rawResult || typeof rawResult !== "object") {
    return { isError: false, content: rawResult };
  }
  const record = rawResult as Record<string, unknown>;
  const isError =
    (typeof record.is_error === "boolean" && record.is_error) ||
    (typeof record.error === "boolean" && record.error);
  if ("content" in record) {
    return { isError: Boolean(isError), content: record.content };
  }
  if ("result" in record) {
    return { isError: Boolean(isError), content: record.result };
  }
  if ("text" in record) {
    return { isError: Boolean(isError), content: record.text };
  }
  return { isError: Boolean(isError), content: rawResult };
}

