#!/usr/bin/env node
"use strict";

const { spawn } = require("node:child_process");
const fs = require("node:fs");
const path = require("node:path");

const TARGETS = {
  "darwin:arm64": { target: "aarch64-apple-darwin", exe: "claude-rs" },
  "darwin:x64": { target: "x86_64-apple-darwin", exe: "claude-rs" },
  "linux:x64": { target: "x86_64-unknown-linux-gnu", exe: "claude-rs" },
  "win32:x64": { target: "x86_64-pc-windows-msvc", exe: "claude-rs.exe" }
};

function resolveInstall() {
  const key = `${process.platform}:${process.arch}`;
  const info = TARGETS[key];
  if (!info) {
    return { error: `Unsupported platform/arch for claude-rs: ${key}` };
  }

  const binaryPath = path.join(__dirname, "..", "vendor", info.target, info.exe);
  if (!fs.existsSync(binaryPath)) {
    return {
      error:
        `Missing binary at ${binaryPath}\n` +
        "Reinstall with `npm install -g claude-code-rust` to fetch release artifacts."
    };
  }

  return { binaryPath };
}

const resolved = resolveInstall();
if (resolved.error) {
  console.error(resolved.error);
  process.exit(1);
}

const child = spawn(resolved.binaryPath, process.argv.slice(2), {
  stdio: "inherit",
  windowsHide: true
});

child.on("error", (error) => {
  console.error(`Failed to launch claude-rs: ${error.message}`);
  process.exit(1);
});

child.on("exit", (code, signal) => {
  if (signal) {
    process.kill(process.pid, signal);
    return;
  }
  process.exit(code ?? 1);
});
