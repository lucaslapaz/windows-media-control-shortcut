#include "tray_icon.hpp"

#include <iterator>

#include "win32_error.hpp"

namespace mediactl {

TrayIcon::TrayIcon(HWND owner, UINT callback_message, UINT id, HICON icon,
                   std::wstring_view tooltip) {
    data_.cbSize = sizeof(data_);
    data_.hWnd = owner;
    data_.uID = id;
    data_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    data_.uCallbackMessage = callback_message;
    data_.hIcon = icon;
    data_.uVersion = NOTIFYICON_VERSION_4;

    // szTip stays zero-filled past the copy, so the result is terminated.
    tooltip.copy(data_.szTip, std::size(data_.szTip) - 1);

    restore();
}

TrayIcon::~TrayIcon() {
    if (visible_) {
        ::Shell_NotifyIconW(NIM_DELETE, &data_);
    }
}

void TrayIcon::restore() {
    ensure(::Shell_NotifyIconW(NIM_ADD, &data_) != FALSE, "Shell_NotifyIconW(NIM_ADD)");
    visible_ = true;

    // Version 4 delivers the cursor position with every callback and lets the
    // shell place the tooltip itself.
    ::Shell_NotifyIconW(NIM_SETVERSION, &data_);
}

}  // namespace mediactl
