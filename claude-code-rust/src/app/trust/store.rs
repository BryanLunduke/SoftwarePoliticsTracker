use serde_json::{Map, Value};
use std::path::{Path, PathBuf};

const TRUST_FIELD: &str = "hasTrustDialogAccepted";
const PROJECTS_FIELD: &str = "projects";

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TrustLookup {
    pub project_key: String,
    pub trusted: bool,
}

pub fn read_status(document: &Value, project_root: &Path) -> TrustLookup {
    let project_key = normalize_project_key(project_root);
    let projects = document.get(PROJECTS_FIELD).and_then(Value::as_object);
    let trusted = projects.is_some_and(|projects| {
        projects.iter().any(|(key, value)| {
            project_keys_match(key, &project_key) && trust_value(value).unwrap_or(false)
        })
    });

    TrustLookup { project_key, trusted }
}

pub fn set_trusted(document: &mut Value, project_root: &Path) -> String {
    let project_key = normalize_project_key(project_root);
    let root = ensure_object_mut(document);
    let projects =
        root.entry(PROJECTS_FIELD.to_owned()).or_insert_with(|| Value::Object(Map::new()));
    if !projects.is_object() {
        *projects = Value::Object(Map::new());
    }

    let Value::Object(projects) = projects else {
        unreachable!("projects must be an object after normalization");
    };

    let matching_keys = projects
        .keys()
        .filter(|key| project_keys_match(key, &project_key))
        .cloned()
        .collect::<Vec<_>>();

    if matching_keys.is_empty() {
        let entry =
            projects.entry(project_key.clone()).or_insert_with(|| Value::Object(Map::new()));
        if !entry.is_object() {
            *entry = Value::Object(Map::new());
        }
        match entry {
            Value::Object(project) => {
                project.insert(TRUST_FIELD.to_owned(), Value::Bool(true));
            }
            _ => unreachable!("project entry must be an object after normalization"),
        }
        return project_key;
    }

    for key in matching_keys {
        let entry = projects.entry(key).or_insert_with(|| Value::Object(Map::new()));
        if !entry.is_object() {
            *entry = Value::Object(Map::new());
        }
        match entry {
            Value::Object(project) => {
                project.insert(TRUST_FIELD.to_owned(), Value::Bool(true));
            }
            _ => unreachable!("project entry must be an object after normalization"),
        }
    }

    project_key
}

pub fn normalize_project_key(project_root: &Path) -> String {
    let absolute = absolutize(project_root);
    normalize_project_key_string(&absolute.to_string_lossy())
}

fn project_keys_match(stored_key: &str, project_key: &str) -> bool {
    let normalized_stored = normalize_project_key_string(stored_key);
    if same_os_path_key(&normalized_stored, project_key) {
        return true;
    }

    canonical_project_key(Path::new(stored_key))
        .is_some_and(|canonical| same_os_path_key(&canonical, project_key))
}

fn absolutize(project_root: &Path) -> PathBuf {
    if let Ok(canonical) = project_root.canonicalize() {
        return canonical;
    }
    if project_root.is_absolute() {
        return project_root.to_path_buf();
    }
    std::env::current_dir()
        .map_or_else(|_| project_root.to_path_buf(), |cwd| cwd.join(project_root))
}

fn normalize_project_key_string(raw: &str) -> String {
    let mut normalized = raw.trim().replace('\\', "/");
    if let Some(stripped) = normalized.strip_prefix("//?/UNC/") {
        normalized = format!("//{stripped}");
    } else if let Some(stripped) = normalized.strip_prefix("//?/") {
        let stripped = stripped.to_owned();
        normalized = stripped;
    }

    let (prefix, root_segments, rest) = split_root_prefix(&normalized);
    let mut normalized_segments = Vec::new();
    for segment in rest.split('/') {
        if segment.is_empty() || segment == "." {
            continue;
        }
        if segment == ".." {
            if !normalized_segments.is_empty() {
                normalized_segments.pop();
            }
            continue;
        }
        normalized_segments.push(segment);
    }

    let mut result = String::new();
    result.push_str(&prefix);
    for segment in root_segments {
        if !result.ends_with('/') {
            result.push('/');
        }
        result.push_str(segment);
    }
    for segment in normalized_segments {
        if !result.ends_with('/') && !result.is_empty() {
            result.push('/');
        }
        result.push_str(segment);
    }

    if result.is_empty() { normalized } else { trim_trailing_separators(result) }
}

fn split_root_prefix(normalized: &str) -> (String, Vec<&str>, &str) {
    if let Some(rest) = normalized.strip_prefix("//") {
        let mut parts = rest.split('/').filter(|segment| !segment.is_empty());
        let server = parts.next();
        let share = parts.next();
        if let (Some(server), Some(share)) = (server, share) {
            let consumed = format!("//{server}/{share}");
            let tail = normalized.strip_prefix(&consumed).unwrap_or_default();
            let rest = tail.trim_start_matches('/');
            return ("//".to_owned(), vec![server, share], rest);
        }
        return ("//".to_owned(), Vec::new(), rest);
    }

    if normalized.len() >= 2 && normalized.as_bytes()[1] == b':' {
        let mut chars = normalized.chars();
        let first = chars.next().unwrap_or_default().to_ascii_uppercase();
        let drive = if normalized[2..].starts_with('/') {
            format!("{first}:/")
        } else {
            format!("{first}:")
        };
        let rest = normalized[2..].trim_start_matches('/');
        return (drive, Vec::new(), rest);
    }

    if let Some(rest) = normalized.strip_prefix('/') {
        return ("/".to_owned(), Vec::new(), rest);
    }

    (String::new(), Vec::new(), normalized)
}

fn trim_trailing_separators(mut normalized: String) -> String {
    let minimum_len = if normalized == "//" {
        2
    } else if normalized.len() >= 3
        && normalized.as_bytes()[1] == b':'
        && normalized.as_bytes()[2] == b'/'
    {
        3
    } else {
        usize::from(normalized.starts_with('/'))
    };
    while normalized.len() > minimum_len && normalized.ends_with('/') {
        normalized.pop();
    }
    normalized
}

fn same_os_path_key(left: &str, right: &str) -> bool {
    if cfg!(windows) { left.eq_ignore_ascii_case(right) } else { left == right }
}

fn canonical_project_key(project_root: &Path) -> Option<String> {
    let canonical = project_root.canonicalize().ok()?;
    Some(normalize_project_key_string(&canonical.to_string_lossy()))
}

fn trust_value(value: &Value) -> Option<bool> {
    value.as_object()?.get(TRUST_FIELD)?.as_bool()
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
    use serde_json::json;

    #[cfg(windows)]
    #[test]
    fn read_status_accepts_equivalent_backslash_entry() {
        let document = json!({
            "projects": {
                r"C:\Users\Simon Peter Rothgang\Desktop\claude_rust": {
                    "hasTrustDialogAccepted": true
                }
            }
        });

        let lookup = read_status(
            &document,
            Path::new(r"c:\Users\Simon Peter Rothgang\Desktop\claude_rust\"),
        );

        assert!(lookup.trusted);
        assert_eq!(lookup.project_key, "C:/Users/Simon Peter Rothgang/Desktop/claude_rust");
    }

    #[cfg(not(windows))]
    #[test]
    fn read_status_accepts_equivalent_normalized_entry() {
        let document = json!({
            "projects": {
                "/home/user/work/project/": {
                    "hasTrustDialogAccepted": true
                }
            }
        });

        let lookup = read_status(&document, Path::new("/home/user/work/project"));

        assert!(lookup.trusted);
        assert_eq!(lookup.project_key, "/home/user/work/project");
    }

    #[cfg(windows)]
    #[test]
    fn read_status_treats_any_equivalent_true_entry_as_trusted() {
        let document = json!({
            "projects": {
                "C:/Users/Simon Peter Rothgang/Desktop/claude_rust": {
                    "hasTrustDialogAccepted": false
                },
                r"C:\Users\Simon Peter Rothgang\Desktop\claude_rust": {
                    "hasTrustDialogAccepted": true
                }
            }
        });

        let lookup =
            read_status(&document, Path::new(r"C:\Users\Simon Peter Rothgang\Desktop\claude_rust"));

        assert!(lookup.trusted);
    }

    #[cfg(not(windows))]
    #[test]
    fn read_status_treats_any_equivalent_true_entry_as_trusted() {
        let document = json!({
            "projects": {
                "/home/user/work/project": {
                    "hasTrustDialogAccepted": false
                },
                "/home/user/work/project/": {
                    "hasTrustDialogAccepted": true
                }
            }
        });

        let lookup = read_status(&document, Path::new("/home/user/work/project"));

        assert!(lookup.trusted);
    }

    #[test]
    fn set_trusted_preserves_unknown_project_fields() {
        let project_path =
            if cfg!(windows) { "C:/work/project" } else { "/home/user/work/project" };

        let mut document = serde_json::json!({
            "projects": {},
            "theme": "dark"
        });
        document["projects"][project_path] = json!({
            "allowedTools": [],
            "hasTrustDialogAccepted": false
        });

        let project_key = set_trusted(&mut document, Path::new(project_path));

        let mut expected = json!({
            "projects": {},
            "theme": "dark"
        });
        expected["projects"][project_path] = json!({
            "allowedTools": [],
            "hasTrustDialogAccepted": true
        });
        assert_eq!(document, expected);
        assert_eq!(project_key, project_path);
    }

    #[test]
    fn normalize_project_key_uppercases_drive_and_trims_trailing_separator() {
        let normalized = normalize_project_key_string(r"c:\work\project\");

        assert_eq!(normalized, "C:/work/project");
    }

    #[test]
    fn normalize_project_key_collapses_dot_segments_for_windows_paths() {
        let normalized = normalize_project_key_string(r"c:\work\demo\..\project\.\");

        assert_eq!(normalized, "C:/work/project");
    }

    #[test]
    fn normalize_project_key_preserves_unc_root_structure() {
        let normalized = normalize_project_key_string(r"\\server\share\team\..\project\");

        assert_eq!(normalized, "//server/share/project");
    }

    #[test]
    fn normalize_project_key_handles_posix_paths() {
        let normalized = normalize_project_key_string("/Users/simon/work/../project/");

        assert_eq!(normalized, "/Users/simon/project");
    }

    #[cfg(windows)]
    #[test]
    fn read_status_accepts_case_differences_for_existing_windows_paths() {
        let dir = tempfile::tempdir().expect("tempdir");
        let project_root = dir.path().join("TrustCaseProject");
        std::fs::create_dir_all(&project_root).expect("create project");

        let stored_key =
            normalize_project_key(&project_root).replace('/', "\\").to_ascii_lowercase();
        let document = json!({
            "projects": {
                stored_key: {
                    "hasTrustDialogAccepted": true
                }
            }
        });

        let lookup = read_status(&document, &project_root);

        assert!(lookup.trusted);
    }

    #[cfg(unix)]
    #[test]
    fn read_status_accepts_symlink_alias_for_existing_unix_paths() {
        use std::os::unix::fs::symlink;

        let dir = tempfile::tempdir().expect("tempdir");
        let project_root = dir.path().join("project");
        let alias_root = dir.path().join("project-link");
        std::fs::create_dir_all(&project_root).expect("create project");
        symlink(&project_root, &alias_root).expect("create symlink");

        let document = json!({
            "projects": {
                alias_root.to_string_lossy().to_string(): {
                    "hasTrustDialogAccepted": true
                }
            }
        });

        let lookup = read_status(&document, &project_root);

        assert!(lookup.trusted);
    }

    #[cfg(windows)]
    #[test]
    fn set_trusted_marks_equivalent_windows_aliases_without_rewriting_metadata() {
        let dir = tempfile::tempdir().expect("tempdir");
        let project_root = dir.path().join("TrustCaseProject");
        std::fs::create_dir_all(&project_root).expect("create project");

        let canonical_key = normalize_project_key(&project_root);
        let alias_key = canonical_key.replace('/', "\\");
        let mut document = json!({
            "projects": {
                alias_key: {
                    "allowedTools": ["git"],
                    "hasTrustDialogAccepted": false
                }
            }
        });

        let project_key = set_trusted(&mut document, &project_root);

        assert_eq!(project_key, canonical_key);
        assert_eq!(
            document,
            json!({
                "projects": {
                    canonical_key.replace('/', "\\"): {
                        "allowedTools": ["git"],
                        "hasTrustDialogAccepted": true
                    }
                }
            })
        );
    }

    #[cfg(unix)]
    #[test]
    fn set_trusted_marks_equivalent_unix_aliases_without_rewriting_metadata() {
        use std::os::unix::fs::symlink;

        let dir = tempfile::tempdir().expect("tempdir");
        let project_root = dir.path().join("project");
        let alias_root = dir.path().join("project-link");
        std::fs::create_dir_all(&project_root).expect("create project");
        symlink(&project_root, &alias_root).expect("create symlink");

        let alias_key = alias_root.to_string_lossy().to_string();
        let mut document = json!({
            "projects": {
                alias_key: {
                    "allowedTools": ["git"],
                    "hasTrustDialogAccepted": false
                }
            }
        });

        let project_key = set_trusted(&mut document, &project_root);

        assert_eq!(project_key, normalize_project_key(&project_root));
        assert_eq!(
            document,
            json!({
                "projects": {
                    alias_root.to_string_lossy().to_string(): {
                        "allowedTools": ["git"],
                        "hasTrustDialogAccepted": true
                    }
                }
            })
        );
    }
}
