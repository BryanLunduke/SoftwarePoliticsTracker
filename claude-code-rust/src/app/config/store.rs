// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use serde_json::{Map, Value};
use std::fs::OpenOptions;
use std::io::Write;
use std::path::{Path, PathBuf};
use std::time::{SystemTime, UNIX_EPOCH};

use super::{
    DefaultPermissionMode, OutputStyle, PreferredNotifChannel, SettingId, SettingKind, SettingSpec,
    setting_spec,
};
use crate::agent::model::EffortLevel;

const SETTINGS_FILENAME: &str = "settings.json";
const LOCAL_SETTINGS_FILENAME: &str = "settings.local.json";
const PREFERENCES_FILENAME: &str = ".claude.json";
const CLAUDE_DIR: &str = ".claude";

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum PersistedSettingValue {
    Missing,
    Bool(bool),
    String(String),
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SettingsPaths {
    pub settings: PathBuf,
    pub local_settings: PathBuf,
    pub preferences: PathBuf,
}

pub struct LoadedSettingsDocuments {
    pub paths: SettingsPaths,
    pub settings_document: Value,
    pub local_settings_document: Value,
    pub preferences_document: Value,
    pub notice: Option<String>,
}

pub fn load(
    home_override: Option<&Path>,
    project_root_override: Option<&Path>,
) -> Result<LoadedSettingsDocuments, String> {
    let paths = resolve_paths(home_override, project_root_override)?;
    let (settings_document, settings_notice) = load_document(&paths.settings)?;
    let (local_settings_document, local_settings_notice) = load_document(&paths.local_settings)?;
    let (preferences_document, preferences_notice) = load_document(&paths.preferences)?;

    let notices = [settings_notice, local_settings_notice, preferences_notice]
        .into_iter()
        .flatten()
        .collect::<Vec<_>>();
    let notice = (!notices.is_empty()).then(|| notices.join(" "));

    Ok(LoadedSettingsDocuments {
        paths,
        settings_document,
        local_settings_document,
        preferences_document,
        notice,
    })
}

pub fn save(path: &Path, document: &Value) -> Result<(), String> {
    let parent = path.parent().ok_or_else(|| "Settings path has no parent directory".to_owned())?;
    std::fs::create_dir_all(parent)
        .map_err(|err| format!("Failed to create settings directory: {err}"))?;

    let normalized = normalized_root(document);
    let temp_path = unique_temp_path(parent, path.file_name().and_then(std::ffi::OsStr::to_str));
    let mut temp = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(&temp_path)
        .map_err(|err| format!("Failed to create settings temp file: {err}"))?;
    serde_json::to_writer_pretty(&mut temp, &normalized)
        .map_err(|err| format!("Failed to serialize settings: {err}"))?;
    temp.write_all(b"\n").map_err(|err| format!("Failed to finalize settings file: {err}"))?;
    temp.flush().map_err(|err| format!("Failed to flush settings file: {err}"))?;
    temp.sync_all().map_err(|err| format!("Failed to sync settings file: {err}"))?;
    drop(temp);
    std::fs::rename(&temp_path, path)
        .map_err(|err| format!("Failed to move settings file into place: {err}"))?;
    Ok(())
}

pub fn read_persisted_setting(
    document: &Value,
    spec: &SettingSpec,
) -> Result<PersistedSettingValue, ()> {
    let Some(value) = read_json_path(document, spec.json_path) else {
        return Ok(PersistedSettingValue::Missing);
    };

    match spec.kind {
        SettingKind::Bool => match value {
            Value::Bool(flag) => Ok(PersistedSettingValue::Bool(*flag)),
            _ => Err(()),
        },
        SettingKind::Enum | SettingKind::DynamicEnum | SettingKind::Text => match value {
            Value::String(text) => Ok(PersistedSettingValue::String(text.clone())),
            _ => Err(()),
        },
    }
}

pub fn write_persisted_setting(
    document: &mut Value,
    spec: &SettingSpec,
    value: PersistedSettingValue,
) {
    match value {
        PersistedSettingValue::Missing => remove_json_path(document, spec.json_path),
        PersistedSettingValue::Bool(flag) => {
            set_json_path(document, spec.json_path, Value::Bool(flag));
        }
        PersistedSettingValue::String(text) => {
            set_json_path(document, spec.json_path, Value::String(text));
        }
    }
}

pub fn fast_mode(document: &Value) -> Result<bool, ()> {
    match read_persisted_setting(document, setting_spec(SettingId::FastMode))? {
        PersistedSettingValue::Missing => Ok(false),
        PersistedSettingValue::Bool(value) => Ok(value),
        PersistedSettingValue::String(_) => Err(()),
    }
}

pub fn set_fast_mode(document: &mut Value, enabled: bool) {
    write_persisted_setting(
        document,
        setting_spec(SettingId::FastMode),
        PersistedSettingValue::Bool(enabled),
    );
}

pub fn always_thinking_enabled(document: &Value) -> Result<bool, ()> {
    match read_persisted_setting(document, setting_spec(SettingId::AlwaysThinking))? {
        PersistedSettingValue::Missing => Ok(false),
        PersistedSettingValue::Bool(value) => Ok(value),
        PersistedSettingValue::String(_) => Err(()),
    }
}

pub fn set_always_thinking_enabled(document: &mut Value, enabled: bool) {
    write_persisted_setting(
        document,
        setting_spec(SettingId::AlwaysThinking),
        PersistedSettingValue::Bool(enabled),
    );
}

pub fn thinking_effort_level(document: &Value) -> Result<EffortLevel, ()> {
    match read_persisted_setting(document, setting_spec(SettingId::ThinkingEffort))? {
        PersistedSettingValue::Missing => Ok(EffortLevel::Medium),
        PersistedSettingValue::Bool(_) => Err(()),
        PersistedSettingValue::String(value) => EffortLevel::from_stored(&value).ok_or(()),
    }
}

pub fn set_thinking_effort_level(document: &mut Value, level: EffortLevel) {
    write_persisted_setting(
        document,
        setting_spec(SettingId::ThinkingEffort),
        PersistedSettingValue::String(level.as_stored().to_owned()),
    );
}

pub fn spinner_tips_enabled(document: &Value) -> Result<bool, ()> {
    match read_persisted_setting(document, setting_spec(SettingId::ShowTips))? {
        PersistedSettingValue::Missing => Ok(true),
        PersistedSettingValue::Bool(value) => Ok(value),
        PersistedSettingValue::String(_) => Err(()),
    }
}

pub fn set_spinner_tips_enabled(document: &mut Value, enabled: bool) {
    write_persisted_setting(
        document,
        setting_spec(SettingId::ShowTips),
        PersistedSettingValue::Bool(enabled),
    );
}

pub fn terminal_progress_bar_enabled(document: &Value) -> Result<bool, ()> {
    match read_persisted_setting(document, setting_spec(SettingId::TerminalProgressBar))? {
        PersistedSettingValue::Missing => Ok(true),
        PersistedSettingValue::Bool(value) => Ok(value),
        PersistedSettingValue::String(_) => Err(()),
    }
}

pub fn set_terminal_progress_bar_enabled(document: &mut Value, enabled: bool) {
    write_persisted_setting(
        document,
        setting_spec(SettingId::TerminalProgressBar),
        PersistedSettingValue::Bool(enabled),
    );
}

pub fn prefers_reduced_motion(document: &Value) -> Result<bool, ()> {
    match read_persisted_setting(document, setting_spec(SettingId::ReduceMotion))? {
        PersistedSettingValue::Missing => Ok(false),
        PersistedSettingValue::Bool(value) => Ok(value),
        PersistedSettingValue::String(_) => Err(()),
    }
}

pub fn set_prefers_reduced_motion(document: &mut Value, enabled: bool) {
    write_persisted_setting(
        document,
        setting_spec(SettingId::ReduceMotion),
        PersistedSettingValue::Bool(enabled),
    );
}

pub fn output_style(document: &Value) -> Result<OutputStyle, ()> {
    match read_persisted_setting(document, setting_spec(SettingId::OutputStyle))? {
        PersistedSettingValue::Missing => Ok(OutputStyle::Default),
        PersistedSettingValue::Bool(_) => Err(()),
        PersistedSettingValue::String(value) => OutputStyle::from_stored(&value).ok_or(()),
    }
}

pub fn set_output_style(document: &mut Value, style: OutputStyle) {
    write_persisted_setting(
        document,
        setting_spec(SettingId::OutputStyle),
        PersistedSettingValue::String(style.as_stored().to_owned()),
    );
}

pub fn model(document: &Value) -> Result<Option<String>, ()> {
    match read_persisted_setting(document, setting_spec(SettingId::Model))? {
        PersistedSettingValue::Missing => Ok(None),
        PersistedSettingValue::Bool(_) => Err(()),
        PersistedSettingValue::String(value) => Ok(Some(value)),
    }
}

pub fn set_model(document: &mut Value, model: Option<&str>) {
    let value = model.map_or(PersistedSettingValue::Missing, |model| {
        PersistedSettingValue::String(model.to_owned())
    });
    write_persisted_setting(document, setting_spec(SettingId::Model), value);
}

#[cfg(test)]
pub fn default_permission_mode(document: &Value) -> Result<DefaultPermissionMode, ()> {
    match read_persisted_setting(document, setting_spec(SettingId::DefaultPermissionMode))? {
        PersistedSettingValue::Missing => Ok(DefaultPermissionMode::Default),
        PersistedSettingValue::Bool(_) => Err(()),
        PersistedSettingValue::String(value) => {
            DefaultPermissionMode::from_stored(&value).ok_or(())
        }
    }
}

pub fn set_default_permission_mode(document: &mut Value, mode: DefaultPermissionMode) {
    write_persisted_setting(
        document,
        setting_spec(SettingId::DefaultPermissionMode),
        PersistedSettingValue::String(mode.as_stored().to_owned()),
    );
}

pub fn language(document: &Value) -> Result<Option<String>, ()> {
    match read_persisted_setting(document, setting_spec(SettingId::Language))? {
        PersistedSettingValue::Missing => Ok(None),
        PersistedSettingValue::Bool(_) => Err(()),
        PersistedSettingValue::String(value) => Ok(Some(value)),
    }
}

pub fn set_language(document: &mut Value, value: Option<&str>) {
    let persisted = value
        .map(str::trim)
        .filter(|text| !text.is_empty())
        .map_or(PersistedSettingValue::Missing, |text| {
            PersistedSettingValue::String(text.to_owned())
        });
    write_persisted_setting(document, setting_spec(SettingId::Language), persisted);
}

pub fn respect_gitignore(document: &Value) -> Result<bool, ()> {
    match read_persisted_setting(document, setting_spec(SettingId::RespectGitignore))? {
        PersistedSettingValue::Missing => Ok(true),
        PersistedSettingValue::Bool(value) => Ok(value),
        PersistedSettingValue::String(_) => Err(()),
    }
}

pub fn set_respect_gitignore(document: &mut Value, enabled: bool) {
    write_persisted_setting(
        document,
        setting_spec(SettingId::RespectGitignore),
        PersistedSettingValue::Bool(enabled),
    );
}

pub fn preferred_notification_channel(document: &Value) -> Result<PreferredNotifChannel, ()> {
    match read_persisted_setting(document, setting_spec(SettingId::Notifications))? {
        PersistedSettingValue::Missing => Ok(PreferredNotifChannel::default()),
        PersistedSettingValue::Bool(_) => Err(()),
        PersistedSettingValue::String(value) => {
            PreferredNotifChannel::from_stored(&value).ok_or(())
        }
    }
}

pub fn set_preferred_notification_channel(document: &mut Value, channel: PreferredNotifChannel) {
    write_persisted_setting(
        document,
        setting_spec(SettingId::Notifications),
        PersistedSettingValue::String(channel.as_stored().to_owned()),
    );
}

fn resolve_paths(
    home_override: Option<&Path>,
    project_root_override: Option<&Path>,
) -> Result<SettingsPaths, String> {
    let home = if let Some(path) = home_override {
        path.to_path_buf()
    } else {
        dirs::home_dir().ok_or_else(|| "Failed to resolve home directory".to_owned())?
    };
    let project_root = if let Some(path) = project_root_override {
        path.to_path_buf()
    } else {
        std::env::current_dir()
            .map_err(|err| format!("Failed to resolve current directory: {err}"))?
    };

    Ok(SettingsPaths {
        settings: home.join(CLAUDE_DIR).join(SETTINGS_FILENAME),
        local_settings: project_root.join(CLAUDE_DIR).join(LOCAL_SETTINGS_FILENAME),
        preferences: home.join(PREFERENCES_FILENAME),
    })
}

fn load_document(path: &Path) -> Result<(Value, Option<String>), String> {
    match std::fs::read_to_string(path) {
        Ok(raw) => parse_document(path, &raw),
        Err(err) if err.kind() == std::io::ErrorKind::NotFound => {
            Ok((Value::Object(Map::new()), None))
        }
        Err(err) => Err(format!("Failed to read settings file: {err}")),
    }
}

fn parse_document(path: &Path, raw: &str) -> Result<(Value, Option<String>), String> {
    if let Ok(Value::Object(object)) = serde_json::from_str::<Value>(raw) {
        Ok((Value::Object(object), None))
    } else {
        let backup = backup_malformed_file(path)?;
        Ok((
            Value::Object(Map::new()),
            Some(format!("Malformed settings file backed up to {}", backup.display())),
        ))
    }
}

fn backup_malformed_file(path: &Path) -> Result<PathBuf, String> {
    let stamp =
        SystemTime::now().duration_since(UNIX_EPOCH).map_or(0, |duration| duration.as_secs());
    let backup = path.with_extension(format!("json.bak.{stamp}"));
    std::fs::copy(path, &backup)
        .map_err(|err| format!("Failed to back up malformed settings file: {err}"))?;
    Ok(backup)
}

fn unique_temp_path(parent: &Path, filename_hint: Option<&str>) -> PathBuf {
    let stamp =
        SystemTime::now().duration_since(UNIX_EPOCH).map_or(0, |duration| duration.as_nanos());
    let filename = filename_hint.unwrap_or(SETTINGS_FILENAME);
    parent.join(format!(".{filename}.{stamp}.tmp"))
}

fn read_json_path<'a>(document: &'a Value, path: &[&str]) -> Option<&'a Value> {
    let mut current = document;
    for key in path {
        current = current.as_object()?.get(*key)?;
    }
    Some(current)
}

fn set_json_path(document: &mut Value, path: &[&str], value: Value) {
    let Some((last_key, parents)) = path.split_last() else {
        return;
    };

    let mut current = ensure_object_mut(document);
    for key in parents {
        let child = current.entry((*key).to_owned()).or_insert_with(|| Value::Object(Map::new()));
        if !child.is_object() {
            *child = Value::Object(Map::new());
        }
        current = match child {
            Value::Object(object) => object,
            _ => unreachable!("child must be an object after normalization"),
        };
    }

    current.insert((*last_key).to_owned(), value);
}

fn remove_json_path(document: &mut Value, path: &[&str]) {
    if let Value::Object(object) = document {
        remove_from_object_path(object, path);
    }
}

fn remove_from_object_path(object: &mut Map<String, Value>, path: &[&str]) -> bool {
    let Some((head, tail)) = path.split_first() else {
        return object.is_empty();
    };

    if tail.is_empty() {
        object.remove(*head);
        return object.is_empty();
    }

    let should_remove_child = if let Some(child) = object.get_mut(*head) {
        match child {
            Value::Object(child_object) => remove_from_object_path(child_object, tail),
            _ => true,
        }
    } else {
        false
    };

    if should_remove_child {
        object.remove(*head);
    }

    object.is_empty()
}

fn normalized_root(document: &Value) -> Value {
    match document {
        Value::Object(object) => Value::Object(object.clone()),
        _ => Value::Object(Map::new()),
    }
}

fn ensure_object_mut(document: &mut Value) -> &mut Map<String, Value> {
    if !document.is_object() {
        *document = Value::Object(Map::new());
    }

    match document {
        Value::Object(object) => object,
        _ => unreachable!("document must be an object after normalization"),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::app::config::setting_spec;

    #[test]
    fn load_missing_files_returns_empty_objects() {
        let dir = tempfile::tempdir().expect("tempdir");

        let loaded = load(Some(dir.path()), Some(dir.path())).expect("load");

        assert_eq!(loaded.settings_document, Value::Object(Map::new()));
        assert_eq!(loaded.local_settings_document, Value::Object(Map::new()));
        assert_eq!(loaded.preferences_document, Value::Object(Map::new()));
        assert!(loaded.notice.is_none());
        assert_eq!(loaded.paths.settings, dir.path().join(".claude").join("settings.json"));
        assert_eq!(
            loaded.paths.local_settings,
            dir.path().join(".claude").join("settings.local.json")
        );
        assert_eq!(loaded.paths.preferences, dir.path().join(".claude.json"));
    }

    #[test]
    fn load_malformed_preferences_file_creates_backup_and_preserves_settings() {
        let dir = tempfile::tempdir().expect("tempdir");
        let settings_path = dir.path().join(".claude").join("settings.json");
        let preferences_path = dir.path().join(".claude.json");
        std::fs::create_dir_all(settings_path.parent().expect("settings parent"))
            .expect("create settings dir");
        std::fs::write(&settings_path, r#"{"fastMode":true}"#).expect("write settings");
        std::fs::write(&preferences_path, "{ not-json").expect("write malformed");

        let loaded = load(Some(dir.path()), Some(dir.path())).expect("load");

        assert_eq!(fast_mode(&loaded.settings_document), Ok(true));
        assert_eq!(loaded.preferences_document, Value::Object(Map::new()));
        let notice = loaded.notice.expect("backup notice");
        assert!(notice.contains("Malformed settings file backed up"));
        let backups = std::fs::read_dir(dir.path())
            .expect("read dir")
            .filter_map(Result::ok)
            .map(|entry| entry.path())
            .filter(|candidate| candidate != &preferences_path)
            .filter(|candidate| candidate.file_name().is_some_and(|name| name != ".claude"))
            .collect::<Vec<_>>();
        assert_eq!(backups.len(), 1);
    }

    #[test]
    fn save_preserves_unknown_keys_and_updates_fast_mode() {
        let dir = tempfile::tempdir().expect("tempdir");
        let path = dir.path().join("settings.json");
        let mut document = serde_json::json!({
            "fastMode": false,
            "unknown": {
                "keep": true
            }
        });
        set_fast_mode(&mut document, true);

        save(&path, &document).expect("save");
        let raw = std::fs::read_to_string(path).expect("read");
        assert!(raw.contains("\"fastMode\": true"));
        assert!(raw.contains("\"keep\": true"));
    }

    #[test]
    fn default_permission_mode_defaults_to_default() {
        let document = Value::Object(Map::new());

        assert_eq!(default_permission_mode(&document), Ok(DefaultPermissionMode::Default));
    }

    #[test]
    fn respect_gitignore_defaults_to_true() {
        let document = Value::Object(Map::new());

        assert_eq!(respect_gitignore(&document), Ok(true));
    }

    #[test]
    fn terminal_progress_bar_defaults_to_true() {
        let document = Value::Object(Map::new());

        assert_eq!(terminal_progress_bar_enabled(&document), Ok(true));
    }

    #[test]
    fn output_style_defaults_to_default() {
        let document = Value::Object(Map::new());

        assert_eq!(output_style(&document), Ok(OutputStyle::Default));
    }

    #[test]
    fn model_defaults_to_none() {
        let document = Value::Object(Map::new());

        assert_eq!(model(&document), Ok(None));
    }

    #[test]
    fn language_defaults_to_none() {
        let document = Value::Object(Map::new());

        assert_eq!(language(&document), Ok(None));
    }

    #[test]
    fn preferred_notification_channel_defaults_to_iterm2() {
        let document = Value::Object(Map::new());

        assert_eq!(preferred_notification_channel(&document), Ok(PreferredNotifChannel::Iterm2));
    }

    #[test]
    fn preferred_notification_channel_rejects_invalid_stored_value() {
        let document = serde_json::json!({
            "preferredNotifChannel": "not-a-channel"
        });

        assert_eq!(preferred_notification_channel(&document), Err(()));
    }

    #[test]
    fn output_style_rejects_invalid_stored_value() {
        let document = serde_json::json!({
            "outputStyle": "Verbose"
        });

        assert_eq!(output_style(&document), Err(()));
    }

    #[test]
    fn respect_gitignore_rejects_invalid_stored_value() {
        let document = serde_json::json!({
            "respectGitignore": "yes"
        });

        assert_eq!(respect_gitignore(&document), Err(()));
    }

    #[test]
    fn model_rejects_invalid_stored_value() {
        let document = serde_json::json!({
            "model": true
        });

        assert_eq!(model(&document), Err(()));
    }

    #[test]
    fn language_rejects_invalid_stored_value() {
        let document = serde_json::json!({
            "language": true
        });

        assert_eq!(language(&document), Err(()));
    }

    #[test]
    fn default_permission_mode_rejects_invalid_stored_value() {
        let document = serde_json::json!({
            "permissions": {
                "defaultMode": "not-a-mode"
            }
        });

        assert_eq!(default_permission_mode(&document), Err(()));
    }

    #[test]
    fn save_preserves_unknown_keys_and_updates_default_permission_mode() {
        let dir = tempfile::tempdir().expect("tempdir");
        let path = dir.path().join("settings.json");
        let mut document = serde_json::json!({
            "permissions": {
                "defaultMode": "default",
                "keep": true
            },
            "unknown": {
                "keep": true
            }
        });
        set_default_permission_mode(&mut document, DefaultPermissionMode::Plan);

        save(&path, &document).expect("save");
        let raw = std::fs::read_to_string(path).expect("read");

        assert!(raw.contains("\"defaultMode\": \"plan\""));
        assert!(raw.contains("\"keep\": true"));
    }

    #[test]
    fn save_preserves_unknown_keys_and_updates_model() {
        let dir = tempfile::tempdir().expect("tempdir");
        let path = dir.path().join("settings.json");
        let mut document = serde_json::json!({
            "model": "old-model",
            "unknown": {
                "keep": true
            }
        });
        set_model(&mut document, Some("sonnet"));

        save(&path, &document).expect("save");
        let raw = std::fs::read_to_string(path).expect("read");

        assert!(raw.contains("\"model\": \"sonnet\""));
        assert!(raw.contains("\"keep\": true"));
    }

    #[test]
    fn save_preserves_unknown_keys_and_updates_language() {
        let dir = tempfile::tempdir().expect("tempdir");
        let path = dir.path().join("settings.json");
        let mut document = serde_json::json!({
            "language": "English",
            "unknown": {
                "keep": true
            }
        });
        set_language(&mut document, Some("German"));

        save(&path, &document).expect("save");
        let raw = std::fs::read_to_string(path).expect("read");

        assert!(raw.contains("\"language\": \"German\""));
        assert!(raw.contains("\"keep\": true"));
    }

    #[test]
    fn set_language_trims_and_removes_whitespace_only_values() {
        let mut document = Value::Object(Map::new());
        set_language(&mut document, Some("  German  "));
        assert_eq!(language(&document), Ok(Some("German".to_owned())));

        set_language(&mut document, Some("   "));
        assert_eq!(language(&document), Ok(None));
    }

    #[test]
    fn save_preserves_unknown_keys_and_updates_notifications() {
        let dir = tempfile::tempdir().expect("tempdir");
        let path = dir.path().join(".claude.json");
        let mut document = serde_json::json!({
            "preferredNotifChannel": "iterm2",
            "theme": "dark"
        });
        set_preferred_notification_channel(&mut document, PreferredNotifChannel::TerminalBell);

        save(&path, &document).expect("save");
        let raw = std::fs::read_to_string(path).expect("read");

        assert!(raw.contains("\"preferredNotifChannel\": \"terminal_bell\""));
        assert!(raw.contains("\"theme\": \"dark\""));
    }

    #[test]
    fn save_preserves_unknown_keys_and_updates_output_style() {
        let dir = tempfile::tempdir().expect("tempdir");
        let path = dir.path().join(".claude").join("settings.local.json");
        std::fs::create_dir_all(path.parent().expect("settings parent")).expect("create dir");
        let mut document = serde_json::json!({
            "outputStyle": "Default",
            "spinnerTipsEnabled": true
        });
        set_output_style(&mut document, OutputStyle::Learning);

        save(&path, &document).expect("save");
        let raw = std::fs::read_to_string(path).expect("read");

        assert!(raw.contains("\"outputStyle\": \"Learning\""));
        assert!(raw.contains("\"spinnerTipsEnabled\": true"));
    }

    #[test]
    fn save_preserves_unknown_keys_and_updates_respect_gitignore() {
        let dir = tempfile::tempdir().expect("tempdir");
        let path = dir.path().join(".claude.json");
        let mut document = serde_json::json!({
            "respectGitignore": true,
            "preferredNotifChannel": "iterm2"
        });
        set_respect_gitignore(&mut document, false);

        save(&path, &document).expect("save");
        let raw = std::fs::read_to_string(path).expect("read");

        assert!(raw.contains("\"respectGitignore\": false"));
        assert!(raw.contains("\"preferredNotifChannel\": \"iterm2\""));
    }

    #[test]
    fn save_preserves_unknown_keys_and_updates_terminal_progress_bar() {
        let dir = tempfile::tempdir().expect("tempdir");
        let path = dir.path().join(".claude.json");
        let mut document = serde_json::json!({
            "terminalProgressBarEnabled": true,
            "preferredNotifChannel": "iterm2"
        });
        set_terminal_progress_bar_enabled(&mut document, false);

        save(&path, &document).expect("save");
        let raw = std::fs::read_to_string(path).expect("read");

        assert!(raw.contains("\"terminalProgressBarEnabled\": false"));
        assert!(raw.contains("\"preferredNotifChannel\": \"iterm2\""));
    }

    #[test]
    fn write_persisted_setting_removes_nested_path_and_prunes_empty_parent() {
        let mut document = serde_json::json!({
            "permissions": {
                "defaultMode": "plan"
            },
            "keep": true
        });

        write_persisted_setting(
            &mut document,
            setting_spec(SettingId::DefaultPermissionMode),
            PersistedSettingValue::Missing,
        );

        assert_eq!(
            document,
            serde_json::json!({
                "keep": true
            })
        );
    }

    #[test]
    fn read_persisted_setting_uses_json_path_metadata() {
        let document = serde_json::json!({
            "permissions": {
                "defaultMode": "plan"
            }
        });

        let value =
            read_persisted_setting(&document, setting_spec(SettingId::DefaultPermissionMode));

        assert_eq!(value, Ok(PersistedSettingValue::String("plan".to_owned())));
    }
}
