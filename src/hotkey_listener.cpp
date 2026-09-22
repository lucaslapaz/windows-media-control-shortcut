#include "hotkey_listener.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "media_command.hpp"
#include "win32_error.hpp"

namespace mediactl {
namespace {

/// The one listener the hook callback reports to.
HotkeyListener* g_listener = nullptr;

bool is_right_control_down() noexcept {
    // Inside a low-level hook the thread message queue has not seen the key yet,
    // so the asynchronous state is the one that reflects reality.
    return (::GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0;
}

const Shortcut* find_shortcut(DWORD virtual_key) noexcept {
    const auto match = std::ranges::find(kShortcuts, static_cast<int>(virtual_key),
                                         &Shortcut::trigger_key);
    return match == kShortcuts.end() ? nullptr : &*match;
}

}  // namespace

HotkeyListener::HotkeyListener(CommandHandler on_command)
    : on_command_(std::move(on_command)) {
    if (g_listener != nullptr) {
        throw std::logic_error("a HotkeyListener is already installed");
    }

    g_listener = this;
    hook_ = ::SetWindowsHookExW(WH_KEYBOARD_LL, &hook_callback, ::GetModuleHandleW(nullptr), 0);
    if (hook_ == nullptr) {
        g_listener = nullptr;
        throw_last_error("SetWindowsHookExW(WH_KEYBOARD_LL)");
    }
}

HotkeyListener::~HotkeyListener() {
    if (hook_ != nullptr) {
        ::UnhookWindowsHookEx(hook_);
    }
    g_listener = nullptr;
}

LRESULT CALLBACK HotkeyListener::hook_callback(int code, WPARAM wparam, LPARAM lparam) noexcept {
    if (code == HC_ACTION && g_listener != nullptr) {
        const auto& key = *reinterpret_cast<const KBDLLHOOKSTRUCT*>(lparam);
        if (g_listener->consume(wparam, key)) {
            return 1;
        }
    }
    return ::CallNextHookEx(nullptr, code, wparam, lparam);
}

bool HotkeyListener::consume(WPARAM message, const KBDLLHOOKSTRUCT& key) noexcept {
    // Our own output passes straight through, so the hook can never feed itself.
    if (key.dwExtraInfo == kInjectionTag || key.vkCode >= swallowed_.size()) {
        return false;
    }

    const bool pressed = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
    const bool released = message == WM_KEYUP || message == WM_SYSKEYUP;
    if (!pressed && !released) {
        return false;
    }

    const Shortcut* shortcut = find_shortcut(key.vkCode);
    if (shortcut == nullptr) {
        return false;
    }

    bool& swallowed = swallowed_[key.vkCode];
    if (released) {
        return std::exchange(swallowed, false);
    }
    if (!is_right_control_down()) {
        return false;
    }

    swallowed = true;
    if (on_command_) {
        on_command_(shortcut->command);
    }
    return true;
}

}  // namespace mediactl
