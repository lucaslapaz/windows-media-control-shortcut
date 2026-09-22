#pragma once

#include <windows.h>

#include "win32_raii.hpp"

namespace mediactl {

/// The "Atalhos e ajuda" window: a short explanation followed by the shortcut
/// table from `kShortcuts`.
///
/// The window is created the first time it is shown and merely hidden when
/// closed, so reopening it is instant and it keeps whatever position the user
/// dragged it to. Its contents are drawn directly rather than built from child
/// controls, which keeps the layout under one roof and makes per-monitor DPI
/// changes a matter of recomputing a few numbers.
class HelpWindow {
public:
    HelpWindow(HINSTANCE instance, HICON large_icon, HICON small_icon);
    ~HelpWindow();

    HelpWindow(const HelpWindow&) = delete;
    HelpWindow& operator=(const HelpWindow&) = delete;
    HelpWindow(HelpWindow&&) = delete;
    HelpWindow& operator=(HelpWindow&&) = delete;

    /// Brings the window up, restoring and focusing it when it is already open.
    void show();

private:
    struct Layout;
    struct Palette;

    static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam,
                                        LPARAM lparam) noexcept;
    LRESULT handle_message(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

    void create();
    void adopt_system_theme();
    void rebuild_fonts();
    void resize_to_content();
    void centre_on_active_monitor();

    Layout compute_layout(HDC device) const;
    Palette palette() const noexcept;
    void paint(HDC device, const RECT& client) const;

    int scale(int value) const noexcept {
        return ::MulDiv(value, static_cast<int>(dpi_), USER_DEFAULT_SCREEN_DPI);
    }

    HINSTANCE instance_;
    HICON large_icon_;
    HICON small_icon_;
    ATOM window_class_{0};
    HWND window_{nullptr};
    UINT dpi_{USER_DEFAULT_SCREEN_DPI};
    bool dark_mode_{false};
    unique_font title_font_;
    unique_font body_font_;
    unique_font chord_font_;
};

}  // namespace mediactl
