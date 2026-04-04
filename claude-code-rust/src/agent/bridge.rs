// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use crate::error::AppError;
use anyhow::Context as _;
use std::path::{Path, PathBuf};
use tokio::process::Command;

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct BridgeLauncher {
    pub runtime_path: PathBuf,
    pub script_path: PathBuf,
}

impl BridgeLauncher {
    #[must_use]
    pub fn describe(&self) -> String {
        format!("{} {}", self.runtime_path.to_string_lossy(), self.script_path.to_string_lossy())
    }

    #[must_use]
    pub fn command(&self) -> Command {
        let mut cmd = Command::new(&self.runtime_path);
        cmd.arg(&self.script_path);
        cmd.stdin(std::process::Stdio::piped());
        cmd.stdout(std::process::Stdio::piped());
        cmd.stderr(std::process::Stdio::piped());
        cmd
    }
}

pub fn resolve_bridge_launcher(explicit_script: Option<&Path>) -> anyhow::Result<BridgeLauncher> {
    let runtime = which::which("node")
        .map_err(|_| anyhow::Error::new(AppError::NodeNotFound))
        .context("failed to resolve `node` runtime")?;
    let script = resolve_bridge_script_path(explicit_script)?;
    Ok(BridgeLauncher { runtime_path: runtime, script_path: script })
}

fn resolve_bridge_script_path(explicit_script: Option<&Path>) -> anyhow::Result<PathBuf> {
    if let Some(path) = explicit_script {
        return validate_script_path(path);
    }

    if let Some(path) = std::env::var_os("CLAUDE_RS_AGENT_BRIDGE") {
        return validate_script_path(Path::new(&path));
    }

    let mut candidates = vec![
        PathBuf::from("agent-sdk/dist/bridge.js"),
        PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("agent-sdk/dist/bridge.js"),
    ];

    if let Ok(current_exe) = std::env::current_exe() {
        for ancestor in current_exe.ancestors().skip(1).take(8) {
            candidates.push(ancestor.join("agent-sdk/dist/bridge.js"));
        }
    }

    for candidate in candidates {
        if !candidate.as_os_str().is_empty() && candidate.exists() {
            return Ok(candidate);
        }
    }

    Err(anyhow::anyhow!(
        "bridge script not found. expected `agent-sdk/dist/bridge.js` or set CLAUDE_RS_AGENT_BRIDGE"
    ))
}

fn validate_script_path(path: &Path) -> anyhow::Result<PathBuf> {
    if !path.exists() {
        return Err(anyhow::anyhow!("bridge script does not exist: {}", path.display()));
    }
    if !path.is_file() {
        return Err(anyhow::anyhow!("bridge script is not a file: {}", path.display()));
    }
    Ok(path.to_path_buf())
}

#[cfg(test)]
mod tests {
    use super::resolve_bridge_launcher;

    #[test]
    fn explicit_missing_script_path_fails() {
        let result =
            resolve_bridge_launcher(Some(std::path::Path::new("agent-sdk/dist/missing.mjs")));
        assert!(result.is_err());
    }
}
