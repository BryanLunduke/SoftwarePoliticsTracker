# Contributing to claude_rust

Thank you for considering contributing to claude_rust! This document provides
guidelines and information for contributors.

## Code of Conduct

This project follows the [Contributor Covenant Code of Conduct](CODE_OF_CONDUCT.md).
By participating, you agree to uphold this code.

## How to Contribute

### Reporting Bugs

- Use the [Bug Report](../../issues/new?template=bug_report.yml) issue template
- Include reproduction steps, expected vs actual behavior, and environment details
- Run with `RUST_LOG=debug` and include relevant log output

### Suggesting Features

- Use the [Feature Request](../../issues/new?template=feature_request.yml) template
- Check existing issues and discussions first
- Describe the problem being solved, not just the desired solution

### Submitting Code

1. Fork the repository
2. Create a feature branch from `main`: `git checkout -b feat/my-feature`
3. Make your changes following the coding standards below
4. Add or update tests as appropriate
5. Ensure all checks pass:
   ```bash
   cargo fmt --all -- --check
   cargo clippy --all-targets --all-features -- -D warnings
   cargo test --all-features
   ```
6. Commit using [Conventional Commits](https://www.conventionalcommits.org/):
   ```
   feat: add keyboard shortcut for tool collapse
   fix: prevent panic on empty terminal output
   ```
7. Push to your fork and open a Pull Request against `main`
8. Fill out the PR template completely

## Development Setup

```bash
# Prerequisites
# - Rust 1.88.0+ (install via https://rustup.rs)
# - Node.js 18+ (for the ACP adapter)
# - npx (included with Node.js)

# Clone and build
git clone https://github.com/srothgan/claude-code-rust.git
cd claude_rust
cargo build

# Run
cargo run

# Run with debug logging
RUST_LOG=debug cargo run

# Run tests
cargo test

# Check formatting
cargo fmt --all -- --check

# Run lints
cargo clippy --all-targets --all-features -- -D warnings
```

## Coding Standards

- **Formatting**: Use `rustfmt` (configured via `rustfmt.toml`)
- **Linting**: `cargo clippy` must pass with zero warnings (configured via `clippy.toml` and `Cargo.toml` `[lints.clippy]`)
- **Naming**: Follow [Rust API Guidelines](https://rust-lang.github.io/api-guidelines/naming.html)
- **Error handling**: Use `thiserror` for library errors, `anyhow` in main/app
- **Comments**: Only where the logic isn't self-evident
- **License headers**: Every new `.rs` file should include `// SPDX-License-Identifier: Apache-2.0`

## Architecture

See [detailed-plan.md](notes/detailed-plan.md) for the full architecture and implementation plan.

Key architectural decisions:
- ACP futures are `!Send` - all ACP code runs in `tokio::task::LocalSet`
- UI and ACP communicate via `tokio::sync::mpsc` channels
- The TUI uses Ratatui with Crossterm backend (cross-platform)

## License

By contributing, you agree that your contributions will be licensed under the
Apache-2.0 license, the same license as the project.
