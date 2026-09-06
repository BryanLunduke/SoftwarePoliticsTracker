# Claude Code Rust

A native Rust terminal interface for Claude Code. Drop-in replacement for Anthropic's stock Node.js/React Ink TUI, built for performance and a better user experience.

[![npm version](https://img.shields.io/npm/v/claude-code-rust)](https://www.npmjs.com/package/claude-code-rust)
[![npm downloads](https://img.shields.io/npm/dm/claude-code-rust)](https://www.npmjs.com/package/claude-code-rust)
[![CI](https://github.com/srothgan/claude-code-rust/actions/workflows/ci.yml/badge.svg)](https://github.com/srothgan/claude-code-rust/actions/workflows/ci.yml)
[![License: Apache-2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://www.apache.org/licenses/LICENSE-2.0)
[![Node.js](https://img.shields.io/badge/Node.js-%3E%3D18-green.svg)](https://nodejs.org/)

## About

Claude Code Rust replaces the stock Claude Code terminal interface with a native Rust binary built on [Ratatui](https://ratatui.rs/). It connects to the same Claude API through a local Agent SDK bridge. Core Claude Code functionality - tool calls, file editing, terminal commands, and permissions - works unchanged.

## Requisites

- Node.js 18+ (for the Agent SDK bridge)
- Existing Claude Code authentication (`~/.claude/config.json`)

## Install

### npm (global, recommended)

```bash
npm install -g claude-code-rust
```

The published package installs a `claude-rs` command and fetches the matching
prebuilt release binary for your platform during install.

If `claude-rs` resolves to an older global shim, ensure your npm global bin
directory comes first on `PATH` or remove the stale shim before retrying.

## Usage

```bash
claude-rs
```

## Why

The stock Claude Code TUI runs on Node.js with React Ink. This causes real problems:

- **Memory**: 200-400MB baseline vs ~20-50MB for a native binary
- **Startup**: 2-5 seconds vs under 100ms
- **Scrollback**: Broken virtual scrolling that loses history
- **Input latency**: Event queue delays on keystroke handling
- **Copy/paste**: Custom implementation instead of native terminal support

Claude Code Rust fixes all of these by compiling to a single native binary with direct terminal control via Crossterm.

## Architecture

Three-layer design:

**Presentation** (Rust/Ratatui) - Single binary with an async event loop (Tokio) handling keyboard input and bridge client events concurrently. Virtual-scrolled chat history with syntax-highlighted code blocks.

**Agent SDK Bridge** (stdio JSON envelopes) - Spawns `agent-sdk/dist/bridge.js` as a child process and communicates via line-delimited JSON envelopes over stdin/stdout. Bidirectional streaming for user messages, tool updates, and permission requests.

**Agent Runtime** (Anthropic Agent SDK) - The TypeScript bridge drives `@anthropic-ai/claude-agent-sdk`, which manages authentication, session/query lifecycle, and tool execution.

## Status

This project is pre-1.0 and under active development. See [CONTRIBUTING.md](CONTRIBUTING.md) for how to get involved.

## License

This project is licensed under the [Apache License 2.0](LICENSE).
Apache-2.0 was chosen to keep usage and redistribution straightforward for individual users, downstream packagers, and commercial adopters.

## Disclaimer

This project is an unofficial terminal UI for Claude Code and is not affiliated with, endorsed by, or supported by Anthropic.
Use it at your own risk.
For official Claude documentation, see [https://claude.ai/docs](https://claude.ai/docs).
