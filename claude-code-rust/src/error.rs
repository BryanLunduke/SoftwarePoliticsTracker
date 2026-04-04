// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

#[derive(Debug, Clone, PartialEq, Eq, thiserror::Error)]
pub enum AppError {
    #[error("Node.js runtime not found")]
    NodeNotFound,
    #[error("Agent bridge process failed")]
    AdapterCrashed,
    #[error("Agent bridge connection failed")]
    ConnectionFailed,
    #[error("Session not found")]
    SessionNotFound,
    #[error("Authentication required")]
    AuthRequired,
}

impl AppError {
    pub const NODE_NOT_FOUND_EXIT_CODE: i32 = 20;
    pub const ADAPTER_CRASHED_EXIT_CODE: i32 = 21;
    pub const CONNECTION_FAILED_EXIT_CODE: i32 = 22;
    pub const SESSION_NOT_FOUND_EXIT_CODE: i32 = 23;
    pub const AUTH_REQUIRED_EXIT_CODE: i32 = 24;

    #[must_use]
    pub fn exit_code(&self) -> i32 {
        match self {
            Self::NodeNotFound => Self::NODE_NOT_FOUND_EXIT_CODE,
            Self::AdapterCrashed => Self::ADAPTER_CRASHED_EXIT_CODE,
            Self::ConnectionFailed => Self::CONNECTION_FAILED_EXIT_CODE,
            Self::SessionNotFound => Self::SESSION_NOT_FOUND_EXIT_CODE,
            Self::AuthRequired => Self::AUTH_REQUIRED_EXIT_CODE,
        }
    }

    #[must_use]
    pub fn user_message(&self) -> &'static str {
        match self {
            Self::NodeNotFound => {
                "Node.js runtime not found. Install Node.js and ensure `node` is on PATH."
            }
            Self::AdapterCrashed => "Agent bridge process crashed or failed to start.",
            Self::ConnectionFailed => {
                "Failed to establish or maintain the Agent SDK bridge connection."
            }
            Self::SessionNotFound => "The requested session was not found.",
            Self::AuthRequired => {
                "Authentication required. Type /login to authenticate, or run `claude auth login` in a terminal."
            }
        }
    }
}
