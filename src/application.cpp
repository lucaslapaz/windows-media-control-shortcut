#include "application.hpp"

#include <commctrl.h>
#include <windowsx.h>

#include <system_error>

#include "media_command.hpp"
#include "resource.h"
#include "win32_error.hpp"

namespace mediactl {
namespace {

constexpr wchar_t kClassName[] = L"MediaControl.Application";
constexpr wchar_t kTooltip[] = L"Media Control - atalhos de mídia ativos";

constexpr UINT kTrayIconId = 1;
constexpr UINT WM_TRAY_NOTIFICATION = WM_APP + 1;
constexpr UINT WM_MEDIA_COMMAND = WM_APP + 2;

constexpr WORD kMenuShowHelp = 1;
constexpr WORD kMenuExit = 2;

/// Picks the frame of the multi-resolution icon that matches `metric` at the
/// current DPI, instead of stretching whichever frame happens to come first.
unique_icon load_app_icon(HINSTANCE instance, int metric) {
    HICON icon = nullptr;
    const HRESULT result =
        ::LoadIconMetric(instance, MAKEINTRESOURCEW(IDI_APP_ICON), metric, &icon);
    if (FAILED(result)) {
        throw std::system_error(result, std::system_category(), "LoadIconMetric");
    }
    return unique_icon{icon};
}

}  // namespace

UINT show_help_request_message() noexcept {
    static const UINT message = ::RegisterWindowMessageW(L"MediaControl.ShowHelp");
    return message;
}

Application::Application(HINSTANCE instance)
    : instance_(instance),
      large_icon_(load_app_icon(instance, LIM_LARGE)),
      small_icon_(load_app_icon(instance, LIM_SMALL)),
      taskbar_created_message_(::RegisterWindowMessageW(L"TaskbarCreated")),
      help_(instance, large_icon_.get(), small_icon_.get()) {
    create_window();
    tray_.emplace(window_, WM_TRAY_NOTIFICATION, kTrayIconId, small_icon_.get(), kTooltip);
    hotkeys_.emplace([this](MediaCommand command) {
        // Runs inside the keyboard hook, where blocking would stall every
        // keystroke on the machine, so the work is handed to the message loop.
        ::PostMessageW(window_, WM_MEDIA_COMMAND, static_cast<WPARAM>(command), 0);
    });
}

Application::~Application() {
    // The listener and the tray icon go first, so neither outlives the window
    // they post to.
    hotkeys_.reset();
    tray_.reset();

    if (window_ != nullptr) {
        ::DestroyWindow(window_);
    }
    if (window_class_ != 0) {
        ::UnregisterClassW(kClassName, instance_);
    }
}

void Application::create_window() {
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = &Application::window_proc;
    window_class.hInstance = instance_;
    window_class.lpszClassName = kClassName;

    window_class_ = ::RegisterClassExW(&window_class);
    ensure(window_class_ != 0, "RegisterClassExW(Application)");

    window_ = ::CreateWindowExW(0, kClassName, L"Media Control", WS_POPUP, 0, 0, 0, 0, nullptr,
                                nullptr, instance_, this);
    ensure(window_ != nullptr, "CreateWindowExW(Application)");
}

int Application::run() {
    MSG message{};
    BOOL result = FALSE;

    while ((result = ::GetMessageW(&message, nullptr, 0, 0)) != 0) {
        if (result == -1) {
            throw_last_error("GetMessageW");
        }
        ::TranslateMessage(&message);
        ::DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}

void Application::open_tray_menu(POINT anchor) {
    const unique_menu menu{::CreatePopupMenu()};
    if (!menu) {
        return;
    }

    ::AppendMenuW(menu.get(), MF_STRING, kMenuShowHelp, L"&Atalhos e ajuda");
    ::AppendMenuW(menu.get(), MF_SEPARATOR, 0, nullptr);
    ::AppendMenuW(menu.get(), MF_STRING, kMenuExit, L"&Sair");
    ::SetMenuDefaultItem(menu.get(), static_cast<UINT>(kMenuShowHelp), FALSE);

    // A tray menu only closes on an outside click while its owner is the
    // foreground window; the trailing WM_NULL is the documented companion that
    // lets the menu dismiss correctly afterwards.
    ::SetForegroundWindow(window_);
    ::TrackPopupMenuEx(menu.get(), TPM_RIGHTBUTTON, anchor.x, anchor.y, window_, nullptr);
    ::PostMessageW(window_, WM_NULL, 0, 0);
}

LRESULT CALLBACK Application::window_proc(HWND window, UINT message, WPARAM wparam,
                                          LPARAM lparam) noexcept {
    if (message == WM_NCCREATE) {
        auto* created = static_cast<Application*>(
            reinterpret_cast<const CREATESTRUCTW*>(lparam)->lpCreateParams);
        ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(created));
    }

    auto* self = reinterpret_cast<Application*>(::GetWindowLongPtrW(window, GWLP_USERDATA));
    if (self == nullptr) {
        return ::DefWindowProcW(window, message, wparam, lparam);
    }
    return self->handle_message(window, message, wparam, lparam);
}

LRESULT Application::handle_message(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == taskbar_created_message_ && tray_) {
        tray_->restore();
        return 0;
    }
    if (message == show_help_request_message()) {
        help_.show();
        return 0;
    }

    switch (message) {
        case WM_TRAY_NOTIFICATION:
            // With NOTIFYICON_VERSION_4 the event is in the low word of lParam
            // and the screen position of the click is packed into wParam.
            switch (LOWORD(lparam)) {
                case NIN_SELECT:
                case NIN_KEYSELECT:
                case WM_CONTEXTMENU:
                    open_tray_menu(POINT{GET_X_LPARAM(wparam), GET_Y_LPARAM(wparam)});
                    return 0;
                default:
                    return 0;
            }

        case WM_MEDIA_COMMAND:
            send_media_command(static_cast<MediaCommand>(wparam));
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wparam)) {
                case kMenuShowHelp:
                    help_.show();
                    return 0;
                case kMenuExit:
                    // Posted rather than destroyed here: WM_COMMAND arrives
                    // while TrackPopupMenuEx is still on the stack, which is no
                    // place to tear the window down.
                    ::PostMessageW(window, WM_CLOSE, 0, 0);
                    return 0;
                default:
                    break;
            }
            break;

        case WM_CLOSE:
            ::DestroyWindow(window);
            return 0;

        case WM_DESTROY:
            window_ = nullptr;
            ::PostQuitMessage(EXIT_SUCCESS);
            return 0;

        default:
            break;
    }

    return ::DefWindowProcW(window, message, wparam, lparam);
}

}  // namespace mediactl
