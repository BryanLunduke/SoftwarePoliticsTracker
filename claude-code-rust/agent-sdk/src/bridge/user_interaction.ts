import type { PermissionResult } from "@anthropic-ai/claude-agent-sdk";
import type {
  Json,
  PermissionOption,
  PermissionOutcome,
  PermissionRequest,
  QuestionAnnotation,
  QuestionOption,
  QuestionOutcome,
  QuestionPrompt,
  QuestionRequest,
  ToolCall,
  ToolCallUpdateFields,
} from "../types.js";
import { asRecordOrNull } from "./shared.js";
import { writeEvent, emitSessionUpdate } from "./events.js";
import { setToolCallStatus } from "./tool_calls.js";
import type { SessionState } from "./session_lifecycle.js";

export type AskUserQuestionOption = {
  label: string;
  description: string;
  preview?: string;
};

export type AskUserQuestionPrompt = {
  question: string;
  header: string;
  multiSelect: boolean;
  options: AskUserQuestionOption[];
};

export const ASK_USER_QUESTION_TOOL_NAME = "AskUserQuestion";
export const QUESTION_CHOICE_KIND = "question_choice";

export const EXIT_PLAN_MODE_TOOL_NAME = "ExitPlanMode";
export const PLAN_APPROVE_KIND = "plan_approve";
export const PLAN_REJECT_KIND = "plan_reject";

export async function requestExitPlanModeApproval(
  session: SessionState,
  toolUseId: string,
  inputData: Record<string, unknown>,
  baseToolCall: ToolCall,
): Promise<PermissionResult> {
  const options: PermissionOption[] = [
    {
      option_id: "approve",
      name: "Approve",
      description: "Approve the plan and continue",
      kind: PLAN_APPROVE_KIND,
    },
    {
      option_id: "reject",
      name: "Reject",
      description: "Reject the plan",
      kind: PLAN_REJECT_KIND,
    },
  ];

  const request: PermissionRequest = {
    tool_call: baseToolCall,
    options,
  };

  const outcome = await new Promise<PermissionOutcome>((resolve) => {
    session.pendingPermissions.set(toolUseId, {
      onOutcome: resolve,
      toolName: EXIT_PLAN_MODE_TOOL_NAME,
      inputData,
    });
    writeEvent({ event: "permission_request", session_id: session.sessionId, request });
  });

  if (outcome.outcome !== "selected" || outcome.option_id === "reject") {
    setToolCallStatus(session, toolUseId, "failed", "Plan rejected");
    return { behavior: "deny", message: "Plan rejected", toolUseID: toolUseId };
  }

  return { behavior: "allow", updatedInput: inputData, toolUseID: toolUseId };
}

export function parseAskUserQuestionPrompts(inputData: Record<string, unknown>): AskUserQuestionPrompt[] {
  const rawQuestions = Array.isArray(inputData.questions) ? inputData.questions : [];
  const prompts: AskUserQuestionPrompt[] = [];

  for (const rawQuestion of rawQuestions) {
    const questionRecord = asRecordOrNull(rawQuestion);
    if (!questionRecord) {
      continue;
    }
    const question = typeof questionRecord.question === "string" ? questionRecord.question.trim() : "";
    if (!question) {
      continue;
    }
    const headerRaw = typeof questionRecord.header === "string" ? questionRecord.header.trim() : "";
    const header = headerRaw || `Q${prompts.length + 1}`;
    const multiSelect = Boolean(questionRecord.multiSelect);
    const rawOptions = Array.isArray(questionRecord.options) ? questionRecord.options : [];
    const options: AskUserQuestionOption[] = [];
    for (const rawOption of rawOptions) {
      const optionRecord = asRecordOrNull(rawOption);
      if (!optionRecord) {
        continue;
      }
      const label = typeof optionRecord.label === "string" ? optionRecord.label.trim() : "";
      const description =
        typeof optionRecord.description === "string" ? optionRecord.description.trim() : "";
      const preview = typeof optionRecord.preview === "string" ? optionRecord.preview.trim() : "";
      if (!label) {
        continue;
      }
      options.push({
        label,
        description,
        ...(preview.length > 0 ? { preview } : {}),
      });
    }
    if (options.length < 2) {
      continue;
    }
    prompts.push({ question, header, multiSelect, options });
  }

  return prompts;
}

function askUserQuestionOptions(prompt: AskUserQuestionPrompt): QuestionOption[] {
  return prompt.options.map((option, index) => ({
    option_id: `question_${index}`,
    label: option.label,
    description: option.description,
    ...(option.preview ? { preview: option.preview } : {}),
  }));
}

function askUserQuestionPromptRawInput(
  prompt: AskUserQuestionPrompt,
  index: number,
  total: number,
): Json {
  return {
    prompt: {
      question: prompt.question,
      header: prompt.header,
      multi_select: prompt.multiSelect,
      options: prompt.options.map((option, optionIndex) => ({
        option_id: `question_${optionIndex}`,
        label: option.label,
        description: option.description,
        ...(option.preview ? { preview: option.preview } : {}),
      })),
    },
    question_index: index,
    total_questions: total,
  };
}

function askUserQuestionPromptToolCall(
  base: ToolCall,
  prompt: AskUserQuestionPrompt,
  index: number,
  total: number,
): ToolCall {
  return {
    ...base,
    title: prompt.question,
    raw_input: askUserQuestionPromptRawInput(prompt, index, total),
  };
}

function buildQuestionRequest(
  promptToolCall: ToolCall,
  prompt: AskUserQuestionPrompt,
  index: number,
  total: number,
): QuestionRequest {
  return {
    tool_call: promptToolCall,
    prompt: {
      question: prompt.question,
      header: prompt.header,
      multi_select: prompt.multiSelect,
      options: askUserQuestionOptions(prompt),
    },
    question_index: index,
    total_questions: total,
  };
}

function askUserQuestionTranscript(
  answers: Array<{ header: string; question: string; answer: string }>,
): string {
  return answers.map((entry) => `${entry.header}: ${entry.answer}\n  ${entry.question}`).join("\n");
}

function deriveAnnotation(
  selectedOptions: QuestionOption[],
  annotation?: QuestionAnnotation,
): QuestionAnnotation | undefined {
  const preview = annotation?.preview?.trim().length
    ? annotation.preview
    : selectedOptions
        .map((option) => option.preview?.trim() ?? "")
        .filter((previewText) => previewText.length > 0)
        .join("\n\n");
  const notes = annotation?.notes?.trim().length ? annotation.notes.trim() : undefined;
  if (!preview && !notes) {
    return undefined;
  }
  return {
    ...(preview ? { preview } : {}),
    ...(notes ? { notes } : {}),
  };
}

export async function requestAskUserQuestionAnswers(
  session: SessionState,
  toolUseId: string,
  inputData: Record<string, unknown>,
  baseToolCall: ToolCall,
): Promise<PermissionResult> {
  const prompts = parseAskUserQuestionPrompts(inputData);
  if (prompts.length === 0) {
    return { behavior: "allow", updatedInput: inputData, toolUseID: toolUseId };
  }

  const answers: Record<string, string> = {};
  const annotations: Record<string, QuestionAnnotation> = {};
  const transcript: Array<{ header: string; question: string; answer: string }> = [];

  for (const [index, prompt] of prompts.entries()) {
    const promptToolCall = askUserQuestionPromptToolCall(baseToolCall, prompt, index, prompts.length);
    const fields: ToolCallUpdateFields = {
      title: promptToolCall.title,
      status: "in_progress",
      raw_input: promptToolCall.raw_input,
    };
    emitSessionUpdate(session.sessionId, {
      type: "tool_call_update",
      tool_call_update: { tool_call_id: toolUseId, fields },
    });
    const tracked = session.toolCalls.get(toolUseId);
    if (tracked) {
      tracked.title = promptToolCall.title;
      tracked.status = "in_progress";
      tracked.raw_input = promptToolCall.raw_input;
    }

    const request = buildQuestionRequest(promptToolCall, prompt, index, prompts.length);
    const outcome = await new Promise<QuestionOutcome>((resolve) => {
      session.pendingQuestions.set(toolUseId, {
        onOutcome: resolve,
        toolName: ASK_USER_QUESTION_TOOL_NAME,
        inputData,
      });
      writeEvent({ event: "question_request", session_id: session.sessionId, request });
    });

    if (outcome.outcome !== "answered") {
      setToolCallStatus(session, toolUseId, "failed", "Question cancelled");
      return { behavior: "deny", message: "Question cancelled", toolUseID: toolUseId };
    }

    const selectedOptions = request.prompt.options.filter((option) =>
      outcome.selected_option_ids.includes(option.option_id),
    );
    if (
      selectedOptions.length === 0 ||
      (!prompt.multiSelect && selectedOptions.length !== 1)
    ) {
      setToolCallStatus(session, toolUseId, "failed", "Question answer was invalid");
      return { behavior: "deny", message: "Question answer was invalid", toolUseID: toolUseId };
    }

    const answer = selectedOptions.map((option) => option.label).join(", ");
    answers[prompt.question] = answer;
    const annotation = deriveAnnotation(selectedOptions, outcome.annotation);
    if (annotation) {
      annotations[prompt.question] = annotation;
    }
    transcript.push({ header: prompt.header, question: prompt.question, answer });

    const summary = askUserQuestionTranscript(transcript);
    const progressFields: ToolCallUpdateFields = {
      status: index + 1 >= prompts.length ? "completed" : "in_progress",
      raw_output: summary,
      content: [{ type: "content", content: { type: "text", text: summary } }],
    };
    emitSessionUpdate(session.sessionId, {
      type: "tool_call_update",
      tool_call_update: { tool_call_id: toolUseId, fields: progressFields },
    });
    if (tracked) {
      tracked.status = progressFields.status ?? tracked.status;
      tracked.raw_output = summary;
      tracked.content = progressFields.content ?? tracked.content;
    }
  }

  return {
    behavior: "allow",
    updatedInput: {
      ...inputData,
      answers,
      ...(Object.keys(annotations).length > 0 ? { annotations } : {}),
    },
    toolUseID: toolUseId,
  };
}
