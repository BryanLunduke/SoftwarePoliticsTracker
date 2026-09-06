use super::{
    DEFAULT_MODEL_ID, DEFAULT_PERMISSION_OPTIONS, DefaultPermissionMode, LANGUAGE_MAX_CHARS,
    LANGUAGE_MIN_CHARS, OutputStyle, PreferredNotifChannel, ResolvedChoice, ResolvedSetting,
    ResolvedSettingValue, RuntimeCatalogKind, SettingId, SettingOptions, SettingSpec,
    SettingValidation, store,
};
use crate::agent::model::AvailableModel;
use serde_json::Value;

pub(super) fn resolve_setting_document(
    document: &Value,
    setting_id: SettingId,
    available_models: &[AvailableModel],
) -> ResolvedSetting {
    let spec = super::setting_spec(setting_id);
    match setting_id {
        SettingId::AlwaysThinking | SettingId::FastMode | SettingId::ReduceMotion => {
            resolve_bool_setting(document, spec, false)
        }
        SettingId::DefaultPermissionMode => {
            resolve_string_setting(document, spec, DefaultPermissionMode::Default.as_stored())
        }
        SettingId::Language => resolve_language_setting(document, spec),
        SettingId::ShowTips | SettingId::RespectGitignore | SettingId::TerminalProgressBar => {
            resolve_bool_setting(document, spec, true)
        }
        SettingId::Model => resolve_model_setting(document, spec, available_models),
        SettingId::OutputStyle => {
            resolve_string_setting(document, spec, OutputStyle::Default.as_stored())
        }
        SettingId::ThinkingEffort => resolve_string_setting(document, spec, "medium"),
        SettingId::Theme => resolve_string_setting(document, spec, "dark"),
        SettingId::Notifications => {
            resolve_string_setting(document, spec, PreferredNotifChannel::default().as_stored())
        }
        SettingId::EditorMode => resolve_string_setting(document, spec, "default"),
    }
}

fn resolve_bool_setting(document: &Value, spec: &SettingSpec, fallback: bool) -> ResolvedSetting {
    match store::read_persisted_setting(document, spec) {
        Ok(store::PersistedSettingValue::Bool(value)) => ResolvedSetting {
            value: ResolvedSettingValue::Bool(value),
            validation: SettingValidation::Valid,
        },
        Ok(store::PersistedSettingValue::Missing) => ResolvedSetting {
            value: ResolvedSettingValue::Bool(fallback),
            validation: SettingValidation::Valid,
        },
        Ok(store::PersistedSettingValue::String(_)) | Err(()) => ResolvedSetting {
            value: ResolvedSettingValue::Bool(fallback),
            validation: SettingValidation::InvalidValue,
        },
    }
}

fn resolve_string_setting(
    document: &Value,
    spec: &SettingSpec,
    fallback: &'static str,
) -> ResolvedSetting {
    match store::read_persisted_setting(document, spec) {
        Ok(store::PersistedSettingValue::String(value)) if option_exists(spec, &value) => {
            ResolvedSetting {
                value: ResolvedSettingValue::Choice(ResolvedChoice::Stored(value)),
                validation: SettingValidation::Valid,
            }
        }
        Ok(store::PersistedSettingValue::Missing) => ResolvedSetting {
            value: ResolvedSettingValue::Choice(ResolvedChoice::Stored(fallback.to_owned())),
            validation: SettingValidation::Valid,
        },
        Ok(store::PersistedSettingValue::String(_) | store::PersistedSettingValue::Bool(_))
        | Err(()) => ResolvedSetting {
            value: ResolvedSettingValue::Choice(ResolvedChoice::Stored(fallback.to_owned())),
            validation: SettingValidation::InvalidValue,
        },
    }
}

fn resolve_language_setting(document: &Value, spec: &SettingSpec) -> ResolvedSetting {
    match store::read_persisted_setting(document, spec) {
        Ok(store::PersistedSettingValue::String(value)) => normalized_language_value(&value)
            .filter(|normalized| language_input_validation_message(normalized).is_none())
            .map_or(
                ResolvedSetting {
                    value: ResolvedSettingValue::Text(String::new()),
                    validation: SettingValidation::InvalidValue,
                },
                |normalized| ResolvedSetting {
                    value: ResolvedSettingValue::Text(normalized),
                    validation: SettingValidation::Valid,
                },
            ),
        Ok(store::PersistedSettingValue::Missing) => ResolvedSetting {
            value: ResolvedSettingValue::Text(String::new()),
            validation: SettingValidation::Valid,
        },
        Ok(store::PersistedSettingValue::Bool(_)) | Err(()) => ResolvedSetting {
            value: ResolvedSettingValue::Text(String::new()),
            validation: SettingValidation::InvalidValue,
        },
    }
}

fn resolve_model_setting(
    document: &Value,
    spec: &SettingSpec,
    available_models: &[AvailableModel],
) -> ResolvedSetting {
    match store::read_persisted_setting(document, spec) {
        Ok(store::PersistedSettingValue::Missing) => ResolvedSetting {
            value: ResolvedSettingValue::Choice(ResolvedChoice::Automatic),
            validation: SettingValidation::Valid,
        },
        Ok(store::PersistedSettingValue::String(value))
            if available_models.is_empty()
                || value == DEFAULT_MODEL_ID
                || available_models.iter().any(|model| model.id == value) =>
        {
            ResolvedSetting {
                value: ResolvedSettingValue::Choice(ResolvedChoice::Stored(value)),
                validation: SettingValidation::Valid,
            }
        }
        Ok(store::PersistedSettingValue::String(_)) => ResolvedSetting {
            value: ResolvedSettingValue::Choice(ResolvedChoice::Automatic),
            validation: SettingValidation::UnavailableOption,
        },
        Ok(store::PersistedSettingValue::Bool(_)) | Err(()) => ResolvedSetting {
            value: ResolvedSettingValue::Choice(ResolvedChoice::Automatic),
            validation: SettingValidation::InvalidValue,
        },
    }
}

fn option_exists(spec: &SettingSpec, value: &str) -> bool {
    match spec.options {
        SettingOptions::Static(options) => options.iter().any(|option| option.stored == value),
        SettingOptions::RuntimeCatalog(RuntimeCatalogKind::PermissionModes) => {
            DEFAULT_PERMISSION_OPTIONS.iter().any(|option| option.stored == value)
        }
        SettingOptions::RuntimeCatalog(RuntimeCatalogKind::Models) => value == DEFAULT_MODEL_ID,
        SettingOptions::None => false,
    }
}

#[must_use]
pub(crate) fn language_input_validation_message(value: &str) -> Option<&'static str> {
    let value = normalized_language_value(value)?;
    let length = value.chars().count();
    if length < LANGUAGE_MIN_CHARS {
        Some("Language must be at least 2 characters.")
    } else if length > LANGUAGE_MAX_CHARS {
        Some("Language must be at most 30 characters.")
    } else {
        None
    }
}

pub(super) fn normalized_language_value(value: &str) -> Option<String> {
    let trimmed = value.trim();
    (!trimmed.is_empty()).then(|| trimmed.to_owned())
}
