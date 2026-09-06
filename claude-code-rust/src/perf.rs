// Copyright 2025 Simon Peter Rothgang
// SPDX-License-Identifier: Apache-2.0

//! Lightweight per-frame performance logger for rendering instrumentation.
//!
//! Gated behind `--features perf`. When the feature is disabled, all types
//! become zero-size and all methods are no-ops that the compiler eliminates.
//!
//! # Usage
//!
//! ```bash
//! cargo run --features perf -- --perf-log performance.log
//! # Writes JSON lines:
//! # {"run":"...","frame":1234,"ts_ms":1739599900793,"fn":"chat::render_msgs","ms":2.345,"n":42}
//! ```

#[cfg(feature = "perf")]
mod enabled {
    use std::cell::RefCell;
    use std::fs::{File, OpenOptions};
    use std::io::{BufWriter, Write};
    use std::path::Path;
    use std::time::{Instant, SystemTime, UNIX_EPOCH};

    // Thread-local file handle so Timer::drop can log without borrowing PerfLogger.
    thread_local! {
        pub(crate) static LOG_FILE: RefCell<Option<BufWriter<File>>> = const { RefCell::new(None) };
        static FRAME_COUNTER: RefCell<u64> = const { RefCell::new(0) };
        static RUN_ID: RefCell<String> = const { RefCell::new(String::new()) };
    }

    pub struct PerfLogger {
        _private: (),
    }

    fn unix_ms() -> u128 {
        SystemTime::now().duration_since(UNIX_EPOCH).map_or(0, |duration| duration.as_millis())
    }

    pub(crate) fn write_entry(name: &'static str, ms: f64, extra: Option<(&'static str, usize)>) {
        let frame = FRAME_COUNTER.with(|c| *c.borrow());
        let ts_ms = unix_ms();
        LOG_FILE.with(|f| {
            if let Some(ref mut file) = *f.borrow_mut() {
                RUN_ID.with(|run| {
                    let run_id = run.borrow();
                    if let Some((k, v)) = extra {
                        let _ = writeln!(
                            file,
                            r#"{{"run":"{run_id}","frame":{frame},"ts_ms":{ts_ms},"fn":"{name}","ms":{ms:.3},"{k}":{v}}}"#,
                        );
                    } else {
                        let _ = writeln!(
                            file,
                            r#"{{"run":"{run_id}","frame":{frame},"ts_ms":{ts_ms},"fn":"{name}","ms":{ms:.3}}}"#,
                        );
                    }
                });
            }
        });
    }

    #[allow(clippy::unused_self)]
    impl PerfLogger {
        /// Open (or create) the log file. Returns `None` on I/O error.
        pub fn open(path: &Path, append: bool) -> Option<Self> {
            let mut options = OpenOptions::new();
            options.create(true).write(true);
            if append {
                options.append(true);
            } else {
                options.truncate(true);
            }
            let file = options.open(path).ok()?;
            let mut writer = BufWriter::new(file);
            let run_id = uuid::Uuid::new_v4().to_string();
            let ts_ms = unix_ms();
            let _ = writeln!(
                writer,
                r#"{{"event":"run_start","run":"{run_id}","ts_ms":{ts_ms},"pid":{},"version":"{}","append":{append}}}"#,
                std::process::id(),
                env!("CARGO_PKG_VERSION")
            );
            let _ = writer.flush();
            LOG_FILE.with(|f| *f.borrow_mut() = Some(writer));
            RUN_ID.with(|r| *r.borrow_mut() = run_id);
            FRAME_COUNTER.with(|c| *c.borrow_mut() = 0);
            Some(Self { _private: () })
        }

        /// Increment the frame counter. Call once at the start of each render frame.
        pub fn next_frame(&mut self) {
            let frame = FRAME_COUNTER.with(|c| {
                let mut value = c.borrow_mut();
                *value += 1;
                *value
            });
            if frame.is_multiple_of(240) {
                LOG_FILE.with(|f| {
                    if let Some(ref mut file) = *f.borrow_mut() {
                        let _ = file.flush();
                    }
                });
            }
        }

        /// Start a named timer. Logs duration on drop.
        #[must_use]
        pub fn start(&self, name: &'static str) -> Timer {
            Timer { name, start: Instant::now(), extra: None }
        }

        /// Start a named timer with an extra numeric field (e.g. message count).
        #[must_use]
        pub fn start_with(
            &self,
            name: &'static str,
            extra_name: &'static str,
            extra_val: usize,
        ) -> Timer {
            Timer { name, start: Instant::now(), extra: Some((extra_name, extra_val)) }
        }

        /// Log an instant marker for the current frame (`ms = 0`).
        pub fn mark(&self, name: &'static str) {
            write_entry(name, 0.0, None);
        }

        /// Log an instant marker with an extra numeric field (`ms = 0`).
        pub fn mark_with(&self, name: &'static str, extra_name: &'static str, extra_val: usize) {
            write_entry(name, 0.0, Some((extra_name, extra_val)));
        }
    }

    pub struct Timer {
        pub(crate) name: &'static str,
        pub(crate) start: Instant,
        pub(crate) extra: Option<(&'static str, usize)>,
    }

    #[allow(clippy::unused_self)]
    impl Timer {
        /// Manually stop and log. Useful when you need to end timing before scope exit.
        pub fn stop(self) {
            // Drop impl handles logging
        }
    }

    impl Drop for Timer {
        fn drop(&mut self) {
            let ms = self.start.elapsed().as_secs_f64() * 1000.0;
            write_entry(self.name, ms, self.extra);
        }
    }
}

#[cfg(not(feature = "perf"))]
mod disabled {
    use std::path::Path;

    pub struct PerfLogger;
    pub struct Timer;

    #[allow(clippy::unused_self)]
    impl PerfLogger {
        #[inline]
        pub fn open(_path: &Path, _append: bool) -> Option<Self> {
            None
        }
        #[inline]
        pub fn next_frame(&mut self) {}
        #[inline]
        #[must_use]
        pub fn start(&self, _name: &'static str) -> Timer {
            Timer
        }
        #[inline]
        #[must_use]
        pub fn start_with(
            &self,
            _name: &'static str,
            _extra_name: &'static str,
            _extra_val: usize,
        ) -> Timer {
            Timer
        }
        #[inline]
        pub fn mark(&self, _name: &'static str) {}
        #[inline]
        pub fn mark_with(&self, _name: &'static str, _extra_name: &'static str, _extra_val: usize) {
        }
    }

    #[allow(clippy::unused_self)]
    impl Timer {
        #[inline]
        pub fn stop(self) {}
    }
}

/// Start a timer without needing a `PerfLogger` reference.
/// Uses the thread-local log file directly. Returns `None` (and is a no-op)
/// when the `perf` feature is disabled or no logger has been opened.
#[cfg(feature = "perf")]
#[must_use]
#[inline]
pub fn start(name: &'static str) -> Option<Timer> {
    // Only create a timer if the log file is actually open
    enabled::LOG_FILE.with(|f| {
        if f.borrow().is_some() {
            Some(Timer { name, start: std::time::Instant::now(), extra: None })
        } else {
            None
        }
    })
}

#[cfg(feature = "perf")]
#[must_use]
#[inline]
pub fn start_with(name: &'static str, extra_name: &'static str, extra_val: usize) -> Option<Timer> {
    enabled::LOG_FILE.with(|f| {
        if f.borrow().is_some() {
            Some(Timer {
                name,
                start: std::time::Instant::now(),
                extra: Some((extra_name, extra_val)),
            })
        } else {
            None
        }
    })
}

#[cfg(not(feature = "perf"))]
#[must_use]
#[inline]
pub fn start(_name: &'static str) -> Option<Timer> {
    None
}

#[cfg(not(feature = "perf"))]
#[must_use]
#[inline]
pub fn start_with(
    _name: &'static str,
    _extra_name: &'static str,
    _extra_val: usize,
) -> Option<Timer> {
    None
}

/// Write an instant marker for the current frame (`ms = 0`).
#[cfg(feature = "perf")]
#[inline]
pub fn mark(name: &'static str) {
    enabled::write_entry(name, 0.0, None);
}

#[cfg(not(feature = "perf"))]
#[inline]
pub fn mark(_name: &'static str) {}

/// Write an instant marker with one numeric field (`ms = 0`).
#[cfg(feature = "perf")]
#[inline]
pub fn mark_with(name: &'static str, extra_name: &'static str, extra_val: usize) {
    enabled::write_entry(name, 0.0, Some((extra_name, extra_val)));
}

#[cfg(not(feature = "perf"))]
#[inline]
pub fn mark_with(_name: &'static str, _extra_name: &'static str, _extra_val: usize) {}

#[cfg(feature = "perf")]
pub use enabled::{PerfLogger, Timer};

#[cfg(not(feature = "perf"))]
pub use disabled::{PerfLogger, Timer};
