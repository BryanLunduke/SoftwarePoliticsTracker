import type {
  PermissionResult,
  PermissionRuleValue,
  PermissionUpdate,
} from "@anthropic-ai/claude-agent-sdk";
import type { PermissionOutcome, PermissionOption } from "../types.js";

type PermissionSuggestionsByScope = {
  session: PermissionUpdate[];
  persistent: PermissionUpdate[];
};

const SESSION_PERMISSION_DESTINATIONS = new Set(["session", "cliArg"]);
const PERSISTENT_PERMISSION_DESTINATIONS = new Set(["userSettings", "projectSettings", "localSettings"]);

function formatPermissionRule(rule: PermissionRuleValue): string {
  return rule.ruleContent === undefined ? rule.toolName : `${rule.toolName}(${rule.ruleContent})`;
}

export function formatPermissionUpdates(updates: PermissionUpdate[] | undefined): string {
  if (!updates || updates.length === 0) {
    return "<none>";
  }
  return updates
    .map((update) => {
      if (update.type === "addRules" || update.type === "replaceRules" || update.type === "removeRules") {
        const rules = update.rules.map((rule) => formatPermissionRule(rule)).join(", ");
        return `${update.type}:${update.behavior}:${update.destination}=[${rules}]`;
      }
      if (update.type === "setMode") {
        return `${update.type}:${update.mode}:${update.destination}`;
      }
      return `${update.type}:${update.destination}=[${update.directories.join(", ")}]`;
    })
    .join(" | ");
}

function splitPermissionSuggestionsByScope(
  suggestions: PermissionUpdate[] | undefined,
): PermissionSuggestionsByScope {
  if (!suggestions || suggestions.length === 0) {
    return { session: [], persistent: [] };
  }

  const session: PermissionUpdate[] = [];
  const persistent: PermissionUpdate[] = [];
  for (const suggestion of suggestions) {
    if (SESSION_PERMISSION_DESTINATIONS.has(suggestion.destination)) {
      session.push(suggestion);
      continue;
    }
    if (PERSISTENT_PERMISSION_DESTINATIONS.has(suggestion.destination)) {
      persistent.push(suggestion);
      continue;
    }
    session.push(suggestion);
  }
  return { session, persistent };
}

export function permissionOptionsFromSuggestions(
  suggestions: PermissionUpdate[] | undefined,
): PermissionOption[] {
  const scoped = splitPermissionSuggestionsByScope(suggestions);
  const hasSessionScoped = scoped.session.length > 0;
  const hasPersistentScoped = scoped.persistent.length > 0;
  const sessionOnly = hasSessionScoped && !hasPersistentScoped;

  const options: PermissionOption[] = [{ option_id: "allow_once", name: "Allow once", kind: "allow_once" }];
  options.push({
    option_id: sessionOnly ? "allow_session" : "allow_always",
    name: sessionOnly ? "Allow for session" : "Always allow",
    kind: sessionOnly ? "allow_session" : "allow_always",
  });
  options.push({ option_id: "reject_once", name: "Deny", kind: "reject_once" });
  return options;
}

export function permissionResultFromOutcome(
  outcome: PermissionOutcome,
  toolCallId: string,
  inputData: Record<string, unknown>,
  suggestions?: PermissionUpdate[],
  toolName?: string,
): PermissionResult {
  const scopedSuggestions = splitPermissionSuggestionsByScope(suggestions);

  if (outcome.outcome === "selected") {
    if (outcome.option_id === "allow_once") {
      return { behavior: "allow", updatedInput: inputData, toolUseID: toolCallId };
    }
    if (outcome.option_id === "allow_session") {
      const sessionSuggestions = scopedSuggestions.session;
      const fallbackSuggestions: PermissionUpdate[] | undefined =
        sessionSuggestions.length > 0
          ? sessionSuggestions
          : toolName
            ? [
                {
                  type: "addRules",
                  rules: [{ toolName }],
                  behavior: "allow",
                  destination: "session",
                },
              ]
            : undefined;
      return {
        behavior: "allow",
        updatedInput: inputData,
        ...(fallbackSuggestions && fallbackSuggestions.length > 0
          ? { updatedPermissions: fallbackSuggestions }
          : {}),
        toolUseID: toolCallId,
      };
    }
    if (outcome.option_id === "allow_always") {
      const persistentSuggestions = scopedSuggestions.persistent;
      const fallbackSuggestions: PermissionUpdate[] | undefined =
        persistentSuggestions.length > 0
          ? persistentSuggestions
          : toolName
            ? [
                {
                  type: "addRules",
                  rules: [{ toolName }],
                  behavior: "allow",
                  destination: "localSettings",
                },
              ]
            : undefined;
      return {
        behavior: "allow",
        updatedInput: inputData,
        ...(fallbackSuggestions && fallbackSuggestions.length > 0
          ? { updatedPermissions: fallbackSuggestions }
          : {}),
        toolUseID: toolCallId,
      };
    }
    return { behavior: "deny", message: "Permission denied", toolUseID: toolCallId };
  }
  return { behavior: "deny", message: "Permission cancelled", toolUseID: toolCallId };
}

