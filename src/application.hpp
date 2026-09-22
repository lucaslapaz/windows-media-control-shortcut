#pragma once

#include <windows.h>

#include <optional>

#include "help_window.hpp"
#include "hotkey_listener.hpp"
#include "tray_icon.hpp"
#include "win32_raii.hpp"

namespace mediactl {

/// The message a second copy of the program broadcasts to ask the copy that is
/// already running to bring up its help window.
UINT show_help_request_message() noexcept;

/// Ties everything together: an invisible top-level window that owns the tray
/// icon, receives the commands the keyboard listener detects and runs the
/// message loop.
///
/// The window has no visible surface, but it is a real top-level window rather
/// than a message-only one because the notification area needs an owner it can
/// bring to the foreground when a menu is open.
class Application {
public:
    explicit Application(HINSTANCE instance);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    /// Pumps messages until the user chooses "Sair". Returns the exit code.
    int run();

private:
    static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam,
                                        LPARAM lparam) noexcept;
    LRESULT handle_message(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

    void create_window();
    void open_tray_menu(POINT anchor);

    HINSTANCE instance_;
    /// Two sizes of the same icon: the shell picks the small one for the
    /// notification area and the large one for Alt+Tab and the task manager.
    unique_icon large_icon_;
    unique_icon small_icon_;
    ATOM window_class_{0};
    HWND window_{nullptr};

    /// Broadcast by the shell when Explorer restarts and the notification area
    /// has to be repopulated.
    UINT taskbar_created_message_{0};

    // Constructed in this order because each one needs the window to exist.
    HelpWindow help_;
    std::optional<TrayIcon> tray_;
    std::optional<HotkeyListener> hotkeys_;
};

}  // namespace mediactl
