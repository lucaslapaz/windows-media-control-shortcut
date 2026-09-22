#pragma once

#include <windows.h>

#include <array>
#include <functional>

#include "shortcuts.hpp"

namespace mediactl {

/// Watches every keystroke for the chords in `kShortcuts` and reports the
/// matching command.
///
/// A low-level keyboard hook is used instead of `RegisterHotKey` because only a
/// hook can tell the right Ctrl apart from the left one. That distinction is
/// the point of the program: claiming the left Ctrl chords would break Ctrl+F5
/// in browsers, Ctrl+F6 in editors, and so on.
///
/// Only one listener may exist at a time. Windows passes no user data to a hook
/// callback, so the live instance is reached through a file-scope pointer.
class HotkeyListener {
public:
    /// Invoked from the hook callback, which runs on the thread that installed
    /// the hook and blocks input delivery while it runs. It must return
    /// immediately; posting a message to the application window is the
    /// intended implementation.
    using CommandHandler = std::function<void(MediaCommand)>;

    explicit HotkeyListener(CommandHandler on_command);
    ~HotkeyListener();

    HotkeyListener(const HotkeyListener&) = delete;
    HotkeyListener& operator=(const HotkeyListener&) = delete;
    HotkeyListener(HotkeyListener&&) = delete;
    HotkeyListener& operator=(HotkeyListener&&) = delete;

private:
    static LRESULT CALLBACK hook_callback(int code, WPARAM wparam, LPARAM lparam) noexcept;

    /// Returns true when the keystroke belongs to a shortcut and must not reach
    /// the focused window.
    bool consume(WPARAM message, const KBDLLHOOKSTRUCT& key) noexcept;

    CommandHandler on_command_;
    HHOOK hook_{nullptr};

    /// Keys whose press was swallowed, indexed by virtual-key code. Their
    /// release has to be swallowed as well, even if the right Ctrl was let go
    /// in between, so that no window ever sees an unpaired key-up.
    std::array<bool, 256> swallowed_{};
};

}  // namespace mediactl
