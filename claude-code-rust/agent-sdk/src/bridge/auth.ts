export function looksLikeAuthRequired(input: string): boolean {
  const normalized = input.toLowerCase();
  return (
    normalized.includes("/login") ||
    normalized.includes("auth required") ||
    normalized.includes("authentication failed") ||
    normalized.includes("please log in")
  );
}

