export interface CacheSplitPolicy {
  softLimitBytes: number;
  hardLimitBytes: number;
  previewLimitBytes: number;
}

export const CACHE_SPLIT_POLICY: Readonly<CacheSplitPolicy> = Object.freeze({
  softLimitBytes: 1536,
  hardLimitBytes: 4096,
  previewLimitBytes: 2048,
});

export function previewKilobyteLabel(policy: Readonly<CacheSplitPolicy> = CACHE_SPLIT_POLICY): string {
  const kb = policy.previewLimitBytes / 1024;
  return Number.isInteger(kb) ? `${kb}KB` : `${kb.toFixed(1)}KB`;
}
