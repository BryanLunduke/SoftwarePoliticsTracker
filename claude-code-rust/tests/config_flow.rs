use claude_code_rust::app::{ActiveView, App, handle_terminal_event};
use crossterm::event::{Event, KeyCode, KeyEvent, KeyModifiers};

#[test]
fn config_enter_closes_and_preserves_chat_draft() {
    let mut app = App::test_default();
    app.active_view = ActiveView::Config;
    app.input.set_text("seed");

    handle_terminal_event(&mut app, Event::Key(KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE)));

    assert_eq!(app.active_view, ActiveView::Chat);
    assert_eq!(app.input.text(), "seed");
    assert!(app.pending_submit.is_none());
}

#[test]
fn config_escape_closes_and_preserves_chat_draft() {
    let mut app = App::test_default();
    app.active_view = ActiveView::Config;
    app.input.set_text("seed");

    handle_terminal_event(&mut app, Event::Key(KeyEvent::new(KeyCode::Esc, KeyModifiers::NONE)));

    assert_eq!(app.active_view, ActiveView::Chat);
    assert_eq!(app.input.text(), "seed");
    assert!(app.pending_submit.is_none());
}

#[test]
fn config_blocks_chat_text_and_slash_activation() {
    let mut app = App::test_default();
    app.active_view = ActiveView::Config;
    app.input.set_text("seed");

    handle_terminal_event(
        &mut app,
        Event::Key(KeyEvent::new(KeyCode::Char('/'), KeyModifiers::NONE)),
    );
    handle_terminal_event(
        &mut app,
        Event::Key(KeyEvent::new(KeyCode::Char('x'), KeyModifiers::NONE)),
    );

    assert_eq!(app.input.text(), "seed");
    assert!(app.slash.is_none());
}

#[test]
fn config_ignores_paste_until_returning_to_chat() {
    let mut app = App::test_default();
    app.active_view = ActiveView::Config;

    handle_terminal_event(&mut app, Event::Paste("blocked".into()));

    assert!(app.pending_paste_text.is_empty());
    assert!(app.input.is_empty());

    handle_terminal_event(&mut app, Event::Key(KeyEvent::new(KeyCode::Enter, KeyModifiers::NONE)));
    handle_terminal_event(&mut app, Event::Paste("allowed".into()));

    assert_eq!(app.active_view, ActiveView::Chat);
    assert_eq!(app.pending_paste_text, "allowed");
}

#[test]
fn ctrl_q_still_quits_from_config() {
    let mut app = App::test_default();
    app.active_view = ActiveView::Config;

    handle_terminal_event(
        &mut app,
        Event::Key(KeyEvent::new(KeyCode::Char('q'), KeyModifiers::CONTROL)),
    );

    assert!(app.should_quit);
}
