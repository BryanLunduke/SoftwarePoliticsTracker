import type { FastModeState, RateLimitStatus, SessionUpdate } from "../types.js";
import { asRecordOrNull } from "./shared.js";

export function numberField(record: Record<string, unknown>, ...keys: string[]): number | undefined {
  for (const key of keys) {
    const value = record[key];
    if (typeof value === "number" && Number.isFinite(value)) {
      return value;
    }
  }
  return undefined;
}

export function parseFastModeState(value: unknown): FastModeState | null {
  if (value === "off" || value === "cooldown" || value === "on") {
    return value;
  }
  return null;
}

export function parseRateLimitStatus(value: unknown): RateLimitStatus | null {
  if (value === "allowed" || value === "allowed_warning" || value === "rejected") {
    return value;
  }
  return null;
}

export function buildRateLimitUpdate(
  rateLimitInfo: unknown,
): Extract<SessionUpdate, { type: "rate_limit_update" }> | null {
  const info = asRecordOrNull(rateLimitInfo);
  if (!info) {
    return null;
  }

  const status = parseRateLimitStatus(info.status);
  if (!status) {
    return null;
  }

  const update: Extract<SessionUpdate, { type: "rate_limit_update" }> = {
    type: "rate_limit_update",
    status,
  };

  const resetsAt = numberField(info, "resetsAt");
  if (resetsAt !== undefined) {
    update.resets_at = resetsAt;
  }

  const utilization = numberField(info, "utilization");
  if (utilization !== undefined) {
    update.utilization = utilization;
  }

  if (typeof info.rateLimitType === "string" && info.rateLimitType.length > 0) {
    update.rate_limit_type = info.rateLimitType;
  }

  const overageStatus = parseRateLimitStatus(info.overageStatus);
  if (overageStatus) {
    update.overage_status = overageStatus;
  }

  const overageResetsAt = numberField(info, "overageResetsAt");
  if (overageResetsAt !== undefined) {
    update.overage_resets_at = overageResetsAt;
  }

  if (typeof info.overageDisabledReason === "string" && info.overageDisabledReason.length > 0) {
    update.overage_disabled_reason = info.overageDisabledReason;
  }

  if (typeof info.isUsingOverage === "boolean") {
    update.is_using_overage = info.isUsingOverage;
  }

  const surpassedThreshold = numberField(info, "surpassedThreshold");
  if (surpassedThreshold !== undefined) {
    update.surpassed_threshold = surpassedThreshold;
  }

  return update;
}
