import type { TurnErrorKind } from "../types.js";
import { looksLikeAuthRequired } from "./auth.js";
import { writeEvent } from "./events.js";
import { emitSessionUpdate } from "./events.js";
import type { SessionState } from "./session_lifecycle.js";
import { parseFastModeState } from "./state_parsing.js";

export function emitAuthRequired(session: SessionState, detail?: string): void {
  if (session.authHintSent) {
    return;
  }
  session.authHintSent = true;
  writeEvent({
    event: "auth_required",
    method_name: "Claude Login",
    method_description:
      detail && detail.trim().length > 0
        ? detail
        : "Type /login to authenticate.",
  });
}

export function looksLikePlanLimitError(input: string): boolean {
  const normalized = input.toLowerCase();
  return (
    normalized.includes("rate limit") ||
    normalized.includes("rate-limit") ||
    normalized.includes("max turns") ||
    normalized.includes("max budget") ||
    normalized.includes("quota") ||
    normalized.includes("plan limit") ||
    normalized.includes("too many requests") ||
    normalized.includes("insufficient quota") ||
    normalized.includes("429")
  );
}

export function classifyTurnErrorKind(
  subtype: string,
  errors: string[],
  assistantError?: string,
): TurnErrorKind {
  const combined = errors.join("\n");

  if (
    subtype === "error_max_turns" ||
    subtype === "error_max_budget_usd" ||
    assistantError === "billing_error" ||
    assistantError === "rate_limit" ||
    (combined.length > 0 && looksLikePlanLimitError(combined))
  ) {
    return "plan_limit";
  }

  if (
    assistantError === "authentication_failed" ||
    errors.some((entry) => looksLikeAuthRequired(entry))
  ) {
    return "auth_required";
  }

  if (assistantError === "server_error") {
    return "internal";
  }

  return "other";
}

export function emitFastModeUpdateIfChanged(session: SessionState, value: unknown): void {
  const next = parseFastModeState(value);
  if (!next || next === session.fastModeState) {
    return;
  }
  session.fastModeState = next;
  emitSessionUpdate(session.sessionId, { type: "fast_mode_update", fast_mode_state: next });
}
