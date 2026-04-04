// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

use super::config::PreferredNotifChannel;
use std::borrow::Cow;

/// Events that can trigger a user notification.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum NotifyEvent {
    /// A tool call requires explicit user approval.
    PermissionRequired,
    /// `AskUserQuestion` is waiting for structured input.
    QuestionRequired,
    /// The agent finished its turn.
    TurnComplete,
}

/// Central notification manager.
///
/// Tracks whether the terminal window is focused (via crossterm
/// `FocusGained`/`FocusLost` events backed by DECSET 1004) and dispatches
/// notifications only when the window is **not** focused.
///
/// Two notification layers fire in parallel:
/// 1. **Terminal bell** (`BEL \x07`) -- causes a taskbar flash / dock bounce
///    on virtually every terminal emulator.
/// 2. **Desktop notification** via `notify-rust` -- OS-native toast popup
///    (Windows Toast, macOS Notification Center, Linux freedesktop D-Bus).
///    Spawned on a background thread so it never blocks the TUI event loop.
///    Silently ignored when the notification backend is unavailable (e.g. SSH).
#[derive(Debug)]
pub struct NotificationManager {
    terminal_focused: bool,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
struct NotificationPlan {
    ring_bell: bool,
    send_desktop: bool,
    osc9_text: Option<&'static str>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
struct TerminalCapabilities {
    osc9_notifications: bool,
}

impl NotificationManager {
    #[must_use]
    pub const fn new() -> Self {
        // Default to `true` (focused) so that terminals which do not support
        // DECSET 1004 never fire spurious notifications.
        Self { terminal_focused: true }
    }

    /// Call when the terminal emits a `FocusGained` event.
    pub fn on_focus_gained(&mut self) {
        self.terminal_focused = true;
    }

    /// Call when the terminal emits a `FocusLost` event.
    pub fn on_focus_lost(&mut self) {
        self.terminal_focused = false;
    }

    /// Whether the terminal window currently has OS focus.
    #[must_use]
    pub const fn is_focused(&self) -> bool {
        self.terminal_focused
    }

    /// Send a notification if the terminal is not focused.
    ///
    /// This is the single entry-point that all event handlers should call.
    /// It is intentionally cheap when focused (just a bool check).
    pub fn notify(&self, channel: PreferredNotifChannel, event: NotifyEvent) {
        if self.terminal_focused {
            return;
        }
        let plan = notification_plan(channel, detect_terminal_capabilities(), event);
        if let Some(text) = plan.osc9_text {
            send_osc9_notification(text);
        }
        if plan.ring_bell {
            ring_bell();
        }
        if plan.send_desktop {
            send_desktop_notification(event);
        }
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

/// Write the ASCII BEL character to stdout, causing a taskbar flash / dock
/// bounce in most terminal emulators.
fn ring_bell() {
    use std::io::Write;
    let _ = std::io::stdout().write_all(b"\x07");
    let _ = std::io::stdout().flush();
}

/// Spawn a background thread that sends an OS-native desktop notification.
///
/// Runs on `std::thread::spawn` rather than tokio because `notify-rust`'s
/// `show()` may block on a D-Bus round-trip (Linux) or COM call (Windows).
/// Errors are silently discarded -- the bell is the reliable fallback.
fn send_desktop_notification(event: NotifyEvent) {
    let (summary, body) = match event {
        NotifyEvent::PermissionRequired => {
            ("Claude Code", "Permission required -- waiting for your approval")
        }
        NotifyEvent::QuestionRequired => {
            ("Claude Code", "Question required -- waiting for your input")
        }
        NotifyEvent::TurnComplete => ("Claude Code", "Turn complete"),
    };
    std::thread::spawn(move || {
        let _ = notify_rust::Notification::new().summary(summary).body(body).show();
    });
}

fn send_osc9_notification(message: &str) {
    use std::io::Write;

    let sequence = osc9_escape_sequence(message);
    let _ = std::io::stdout().write_all(sequence.as_bytes());
    let _ = std::io::stdout().flush();
}

fn notification_plan(
    channel: PreferredNotifChannel,
    capabilities: TerminalCapabilities,
    event: NotifyEvent,
) -> NotificationPlan {
    let osc9_text = capabilities.osc9_notifications.then(|| notification_text(event));
    match channel {
        PreferredNotifChannel::NotificationsDisabled => {
            NotificationPlan { ring_bell: false, send_desktop: false, osc9_text: None }
        }
        PreferredNotifChannel::TerminalBell => {
            NotificationPlan { ring_bell: true, send_desktop: false, osc9_text: None }
        }
        // "Auto / iTerm2" replaced the original always-bell-plus-desktop behavior.
        // Preserve that reliable fallback whenever OSC 9 is unavailable.
        PreferredNotifChannel::Iterm2 => NotificationPlan {
            ring_bell: osc9_text.is_none(),
            send_desktop: osc9_text.is_none(),
            osc9_text,
        },
        PreferredNotifChannel::Ghostty => {
            NotificationPlan { ring_bell: false, send_desktop: osc9_text.is_none(), osc9_text }
        }
        PreferredNotifChannel::Iterm2WithBell => {
            NotificationPlan { ring_bell: true, send_desktop: osc9_text.is_none(), osc9_text }
        }
    }
}

fn detect_terminal_capabilities() -> TerminalCapabilities {
    terminal_capabilities_from_env(
        std::env::vars_os()
            .filter_map(|(key, value)| Some((key.into_string().ok()?, value.into_string().ok()?))),
    )
}

fn terminal_capabilities_from_env<I>(vars: I) -> TerminalCapabilities
where
    I: IntoIterator<Item = (String, String)>,
{
    let mut term_program = None::<String>;
    let mut iterm_session = false;

    for (key, value) in vars {
        match key.as_str() {
            "TERM_PROGRAM" => term_program = Some(value),
            "ITERM_SESSION_ID" if !value.is_empty() => iterm_session = true,
            _ => {}
        }
    }

    let osc9_notifications =
        matches!(term_program.as_deref(), Some("iTerm.app" | "ghostty")) || iterm_session;
    TerminalCapabilities { osc9_notifications }
}

const fn notification_text(event: NotifyEvent) -> &'static str {
    match event {
        NotifyEvent::PermissionRequired => "Claude Code: Permission required",
        NotifyEvent::QuestionRequired => "Claude Code: Question required",
        NotifyEvent::TurnComplete => "Claude Code: Turn complete",
    }
}

fn osc9_escape_sequence(message: &str) -> Cow<'_, str> {
    let sanitized = sanitize_osc9_message(message);
    let mut sequence = String::with_capacity(sanitized.len() + 8);
    sequence.push('\u{1b}');
    sequence.push_str("]9;");
    sequence.push_str(&sanitized);
    sequence.push('\u{1b}');
    sequence.push('\\');
    Cow::Owned(sequence)
}

fn sanitize_osc9_message(message: &str) -> String {
    let mut sanitized = String::with_capacity(message.len());
    for ch in message.chars() {
        match ch {
            '\u{07}' | '\u{1b}' | '\u{9c}' => {}
            '\r' | '\n' => sanitized.push(' '),
            _ => sanitized.push(ch),
        }
    }
    sanitized
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn defaults_to_focused() {
        let mgr = NotificationManager::new();
        assert!(mgr.is_focused(), "should default to focused to suppress spurious notifications");
    }

    #[test]
    fn focus_lost_sets_unfocused() {
        let mut mgr = NotificationManager::new();
        mgr.on_focus_lost();
        assert!(!mgr.is_focused());
    }

    #[test]
    fn focus_gained_restores_focused() {
        let mut mgr = NotificationManager::new();
        mgr.on_focus_lost();
        mgr.on_focus_gained();
        assert!(mgr.is_focused());
    }

    #[test]
    fn disabled_notifications_plan_is_silent() {
        assert_eq!(
            notification_plan(
                PreferredNotifChannel::NotificationsDisabled,
                TerminalCapabilities { osc9_notifications: true },
                NotifyEvent::TurnComplete,
            ),
            NotificationPlan { ring_bell: false, send_desktop: false, osc9_text: None }
        );
    }

    #[test]
    fn terminal_bell_plan_skips_desktop_notification() {
        assert_eq!(
            notification_plan(
                PreferredNotifChannel::TerminalBell,
                TerminalCapabilities { osc9_notifications: true },
                NotifyEvent::TurnComplete,
            ),
            NotificationPlan { ring_bell: true, send_desktop: false, osc9_text: None }
        );
    }

    #[test]
    fn iterm2_uses_osc9_when_supported() {
        assert_eq!(
            notification_plan(
                PreferredNotifChannel::Iterm2,
                TerminalCapabilities { osc9_notifications: true },
                NotifyEvent::TurnComplete,
            ),
            NotificationPlan {
                ring_bell: false,
                send_desktop: false,
                osc9_text: Some("Claude Code: Turn complete"),
            }
        );
    }

    #[test]
    fn iterm2_auto_preserves_bell_and_desktop_fallback_when_osc9_is_unavailable() {
        assert_eq!(
            notification_plan(
                PreferredNotifChannel::Iterm2,
                TerminalCapabilities { osc9_notifications: false },
                NotifyEvent::TurnComplete,
            ),
            NotificationPlan { ring_bell: true, send_desktop: true, osc9_text: None }
        );
    }

    #[test]
    fn iterm2_with_bell_uses_osc9_and_bell_when_supported() {
        assert_eq!(
            notification_plan(
                PreferredNotifChannel::Iterm2WithBell,
                TerminalCapabilities { osc9_notifications: true },
                NotifyEvent::PermissionRequired,
            ),
            NotificationPlan {
                ring_bell: true,
                send_desktop: false,
                osc9_text: Some("Claude Code: Permission required"),
            }
        );
    }

    #[test]
    fn iterm2_with_bell_falls_back_to_desktop_and_bell() {
        assert_eq!(
            notification_plan(
                PreferredNotifChannel::Iterm2WithBell,
                TerminalCapabilities { osc9_notifications: false },
                NotifyEvent::PermissionRequired,
            ),
            NotificationPlan { ring_bell: true, send_desktop: true, osc9_text: None }
        );
    }

    #[test]
    fn ghostty_uses_osc9_when_supported() {
        assert_eq!(
            notification_plan(
                PreferredNotifChannel::Ghostty,
                TerminalCapabilities { osc9_notifications: true },
                NotifyEvent::TurnComplete,
            ),
            NotificationPlan {
                ring_bell: false,
                send_desktop: false,
                osc9_text: Some("Claude Code: Turn complete"),
            }
        );
    }

    #[test]
    fn detects_iterm2_via_term_program() {
        let capabilities =
            terminal_capabilities_from_env([("TERM_PROGRAM".to_owned(), "iTerm.app".to_owned())]);

        assert!(capabilities.osc9_notifications);
    }

    #[test]
    fn detects_iterm2_via_session_id() {
        let capabilities =
            terminal_capabilities_from_env([("ITERM_SESSION_ID".to_owned(), "w0t1p0".to_owned())]);

        assert!(capabilities.osc9_notifications);
    }

    #[test]
    fn detects_ghostty_via_term_program() {
        let capabilities =
            terminal_capabilities_from_env([("TERM_PROGRAM".to_owned(), "ghostty".to_owned())]);

        assert!(capabilities.osc9_notifications);
    }

    #[test]
    fn unsupported_term_does_not_advertise_osc9() {
        let capabilities =
            terminal_capabilities_from_env([("TERM_PROGRAM".to_owned(), "wezterm".to_owned())]);

        assert!(!capabilities.osc9_notifications);
    }

    #[test]
    fn osc9_sequence_uses_st_terminator_and_sanitizes_message() {
        assert_eq!(
            osc9_escape_sequence("hello\n\u{1b}world\u{07}").as_ref(),
            "\u{1b}]9;hello world\u{1b}\\"
        );
    }
}
