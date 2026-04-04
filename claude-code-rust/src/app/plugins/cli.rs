use super::{
    InstalledPluginEntry, MarketplaceEntry, MarketplaceSourceEntry, PluginCapability,
    PluginsInventorySnapshot,
};
use serde::Deserialize;
use serde_json::Value;
use std::path::{Path, PathBuf};
use std::process::Command;

#[derive(Debug, Deserialize)]
struct InstalledPluginJson {
    id: String,
    version: Option<String>,
    scope: String,
    enabled: bool,
    #[serde(rename = "installedAt")]
    installed_at: Option<String>,
    #[serde(rename = "lastUpdated")]
    last_updated: Option<String>,
    #[serde(rename = "projectPath")]
    project_path: Option<String>,
    #[serde(rename = "mcpServers")]
    mcp_servers: Option<Value>,
}

#[derive(Debug, Deserialize)]
struct MarketplaceListJson {
    available: Vec<AvailablePluginJson>,
}

#[derive(Debug, Deserialize)]
struct AvailablePluginJson {
    #[serde(rename = "pluginId")]
    plugin_id: String,
    name: String,
    description: Option<String>,
    #[serde(rename = "marketplaceName")]
    marketplace_name: Option<String>,
    version: Option<String>,
    #[serde(rename = "installCount")]
    install_count: Option<u64>,
    source: Option<Value>,
}

#[derive(Debug, Deserialize)]
struct MarketplaceSourceJson {
    name: String,
    source: Option<String>,
    repo: Option<String>,
}

pub(super) async fn refresh_inventory(
    cwd_raw: String,
    cached_claude_path: Option<PathBuf>,
) -> Result<(PluginsInventorySnapshot, PathBuf), String> {
    tokio::task::spawn_blocking(move || {
        let claude_path = resolve_claude_path(cached_claude_path)?;
        let snapshot = refresh_inventory_blocking(&claude_path, &cwd_raw)?;
        Ok((snapshot, claude_path))
    })
    .await
    .map_err(|error| format!("Plugin inventory task failed: {error}"))?
}

pub(super) async fn run_cli_command_and_refresh(
    cwd_raw: String,
    cached_claude_path: Option<PathBuf>,
    args: Vec<String>,
) -> Result<(PluginsInventorySnapshot, PathBuf), String> {
    tokio::task::spawn_blocking(move || {
        let claude_path = resolve_claude_path(cached_claude_path)?;
        run_command(&claude_path, &cwd_raw, &args)?;
        let snapshot = refresh_inventory_blocking(&claude_path, &cwd_raw)?;
        Ok((snapshot, claude_path))
    })
    .await
    .map_err(|error| format!("Plugin CLI action task failed: {error}"))?
}

fn resolve_claude_path(cached_claude_path: Option<PathBuf>) -> Result<PathBuf, String> {
    if let Some(path) = cached_claude_path
        && path.is_file()
    {
        return Ok(path);
    }
    which::which("claude").map_err(|_| "claude CLI not found in PATH".to_owned())
}

fn refresh_inventory_blocking(
    claude_path: &Path,
    cwd_raw: &str,
) -> Result<PluginsInventorySnapshot, String> {
    let installed = parse_json_command::<Vec<InstalledPluginJson>>(
        claude_path,
        cwd_raw,
        &["plugin", "list", "--json"],
    )?;
    let available = parse_json_command::<MarketplaceListJson>(
        claude_path,
        cwd_raw,
        &["plugin", "list", "--available", "--json"],
    )?;
    let marketplaces = parse_json_command::<Vec<MarketplaceSourceJson>>(
        claude_path,
        cwd_raw,
        &["plugin", "marketplace", "list", "--json"],
    )?;

    let mut installed_entries = installed
        .into_iter()
        .map(|entry| InstalledPluginEntry {
            id: entry.id,
            version: entry.version,
            scope: entry.scope,
            enabled: entry.enabled,
            installed_at: entry.installed_at,
            last_updated: entry.last_updated,
            project_path: entry.project_path,
            capability: if entry.mcp_servers.is_some() {
                PluginCapability::Mcp
            } else {
                PluginCapability::Skill
            },
        })
        .collect::<Vec<_>>();
    installed_entries.sort_by_cached_key(|entry| entry.id.to_ascii_lowercase());

    let mut marketplace_entries = available
        .available
        .into_iter()
        .map(|entry| MarketplaceEntry {
            plugin_id: entry.plugin_id,
            name: entry.name,
            description: entry.description,
            marketplace_name: entry.marketplace_name,
            version: entry.version,
            install_count: entry.install_count,
            source: entry.source,
        })
        .collect::<Vec<_>>();
    marketplace_entries.sort_by_cached_key(|entry| {
        (
            entry.marketplace_name.as_deref().unwrap_or_default().to_ascii_lowercase(),
            entry.name.to_ascii_lowercase(),
        )
    });

    let mut marketplace_sources = marketplaces
        .into_iter()
        .map(|entry| MarketplaceSourceEntry {
            name: entry.name,
            source: entry.source,
            repo: entry.repo,
        })
        .collect::<Vec<_>>();
    marketplace_sources.sort_by_cached_key(|entry| entry.name.to_ascii_lowercase());

    Ok(PluginsInventorySnapshot {
        installed: installed_entries,
        marketplace: marketplace_entries,
        marketplaces: marketplace_sources,
    })
}

fn parse_json_command<T>(claude_path: &Path, cwd_raw: &str, args: &[&str]) -> Result<T, String>
where
    T: for<'de> Deserialize<'de>,
{
    let output = Command::new(claude_path)
        .args(args)
        .current_dir(cwd_raw)
        .output()
        .map_err(|error| format!("Failed to run `claude {}`: {error}", args.join(" ")))?;

    if !output.status.success() {
        let stderr = String::from_utf8_lossy(&output.stderr).trim().to_owned();
        let exit_code =
            output.status.code().map_or_else(|| "unknown".to_owned(), |code| code.to_string());
        let detail = if stderr.is_empty() {
            format!("exit code {exit_code}")
        } else {
            format!("exit code {exit_code}: {stderr}")
        };
        return Err(format!("`claude {}` failed: {detail}", args.join(" ")));
    }

    serde_json::from_slice(&output.stdout)
        .map_err(|error| format!("Failed to parse JSON from `claude {}`: {error}", args.join(" ")))
}

fn run_command(claude_path: &Path, cwd_raw: &str, args: &[String]) -> Result<(), String> {
    let output = Command::new(claude_path)
        .args(args)
        .current_dir(cwd_raw)
        .output()
        .map_err(|error| format!("Failed to run `claude {}`: {error}", args.join(" ")))?;

    if output.status.success() {
        return Ok(());
    }

    let stderr = String::from_utf8_lossy(&output.stderr).trim().to_owned();
    let exit_code =
        output.status.code().map_or_else(|| "unknown".to_owned(), |code| code.to_string());
    let detail = if stderr.is_empty() {
        format!("exit code {exit_code}")
    } else {
        format!("exit code {exit_code}: {stderr}")
    };
    Err(format!("`claude {}` failed: {detail}", args.join(" ")))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_installed_plugin_entries() {
        let json = r#"
[
  {
    "id": "frontend-design@claude-plugins-official",
    "version": "55b58ec6e564",
    "scope": "local",
    "enabled": false,
    "installedAt": "2026-02-05T15:37:39.555Z",
    "lastUpdated": "2026-03-02T18:10:00.820Z",
    "projectPath": "C:\\work"
  }
]
"#;

        let parsed = serde_json::from_str::<Vec<InstalledPluginJson>>(json).expect("parse json");
        assert_eq!(parsed.len(), 1);
        assert_eq!(parsed[0].id, "frontend-design@claude-plugins-official");
        assert_eq!(parsed[0].scope, "local");
        assert!(!parsed[0].enabled);
        assert_eq!(parsed[0].project_path.as_deref(), Some("C:\\work"));
    }

    #[test]
    fn detects_mcp_plugins_from_installed_payload() {
        let json = r#"
[
  {
    "id": "supabase@claude-plugins-official",
    "scope": "local",
    "enabled": true,
    "mcpServers": {
      "supabase": {
        "type": "http",
        "url": "https://mcp.supabase.com/mcp"
      }
    }
  }
]
"#;

        let parsed = serde_json::from_str::<Vec<InstalledPluginJson>>(json).expect("parse json");
        let entry = InstalledPluginEntry {
            id: parsed[0].id.clone(),
            version: parsed[0].version.clone(),
            scope: parsed[0].scope.clone(),
            enabled: parsed[0].enabled,
            installed_at: parsed[0].installed_at.clone(),
            last_updated: parsed[0].last_updated.clone(),
            project_path: parsed[0].project_path.clone(),
            capability: if parsed[0].mcp_servers.is_some() {
                PluginCapability::Mcp
            } else {
                PluginCapability::Skill
            },
        };

        assert_eq!(entry.capability, PluginCapability::Mcp);
    }

    #[test]
    fn parses_marketplace_entries_and_sources() {
        let available_json = r#"
{
  "installed": [],
  "available": [
    {
      "pluginId": "frontend-design@claude-plugins-official",
      "name": "frontend-design",
      "description": "Create distinctive interfaces",
      "marketplaceName": "claude-plugins-official",
      "version": "1.0.0",
      "source": "./plugins/frontend-design",
      "installCount": 42
    }
  ]
}
"#;
        let source_json = r#"
[
  {
    "name": "claude-plugins-official",
    "source": "github",
    "repo": "anthropics/claude-plugins-official"
  }
]
"#;

        let parsed_available =
            serde_json::from_str::<MarketplaceListJson>(available_json).expect("parse available");
        let parsed_sources =
            serde_json::from_str::<Vec<MarketplaceSourceJson>>(source_json).expect("parse sources");

        assert_eq!(parsed_available.available.len(), 1);
        assert_eq!(
            parsed_available.available[0].marketplace_name.as_deref(),
            Some("claude-plugins-official")
        );
        assert_eq!(parsed_available.available[0].install_count, Some(42));
        assert_eq!(parsed_sources[0].repo.as_deref(), Some("anthropics/claude-plugins-official"));
    }
}
