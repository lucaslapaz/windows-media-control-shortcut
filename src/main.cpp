#include <windows.h>

#include <commctrl.h>

#include <cstdlib>
#include <exception>
#include <string>
#include <string_view>

#include "application.hpp"
#include "win32_raii.hpp"

namespace {

/// Per-session, so that two users logged into the same machine each get their
/// own copy.
constexpr wchar_t kSingleInstanceMutex[] = L"Local\\MediaControl.SingleInstance";

/// Exception messages come from the C runtime in the system code page.
std::wstring widen(std::string_view text) {
    if (text.empty()) {
        return {};
    }

    const int length = ::MultiByteToWideChar(CP_ACP, 0, text.data(), static_cast<int>(text.size()),
                                             nullptr, 0);
    std::wstring wide(static_cast<size_t>(length), L'\0');
    ::MultiByteToWideChar(CP_ACP, 0, text.data(), static_cast<int>(text.size()), wide.data(),
                          length);
    return wide;
}

}  // namespace

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int) {
    const HANDLE mutex = ::CreateMutexW(nullptr, TRUE, kSingleInstanceMutex);
    const DWORD mutex_error = ::GetLastError();
    const mediactl::unique_handle instance_guard{mutex};

    if (mutex_error == ERROR_ALREADY_EXISTS) {
        // Another copy already owns the keyboard hook. Ask it to show its help
        // window - that is what someone launching the program twice is after -
        // and step aside.
        ::PostMessageW(HWND_BROADCAST, mediactl::show_help_request_message(), 0, 0);
        return EXIT_SUCCESS;
    }

    // Enables the version 6 common controls the manifest asks for, which
    // LoadIconMetric needs.
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_STANDARD_CLASSES};
    ::InitCommonControlsEx(&controls);

    try {
        mediactl::Application app(instance);
        return app.run();
    } catch (const std::exception& error) {
        const std::wstring message =
            L"Não foi possível iniciar o Media Control.\n\n" + widen(error.what());
        ::MessageBoxW(nullptr, message.c_str(), L"Media Control", MB_ICONERROR | MB_OK);
        return EXIT_FAILURE;
    }
}
