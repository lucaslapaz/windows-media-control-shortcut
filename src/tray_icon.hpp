#pragma once

#include <windows.h>
#include <shellapi.h>

#include <string>

namespace mediactl {

/// Owns the notification-area icon: it appears when the object is constructed
/// and is removed when the object dies, including when an exception unwinds
/// past it.
class TrayIcon {
public:
    /// `callback_message` should be a value in the WM_APP range; the owner
    /// window receives it for every mouse and keyboard interaction with the
    /// icon.
    TrayIcon(HWND owner, UINT callback_message, UINT id, HICON icon, std::wstring_view tooltip);
    ~TrayIcon();

    TrayIcon(const TrayIcon&) = delete;
    TrayIcon& operator=(const TrayIcon&) = delete;
    TrayIcon(TrayIcon&&) = delete;
    TrayIcon& operator=(TrayIcon&&) = delete;

    /// Puts the icon back after Explorer restarted and cleared the
    /// notification area.
    void restore();

private:
    NOTIFYICONDATAW data_{};
    bool visible_{false};
};

}  // namespace mediactl
