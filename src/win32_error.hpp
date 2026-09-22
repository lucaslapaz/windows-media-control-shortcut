// Turns a failed Win32 call into an exception, so that failures during start-up
// travel up to wWinMain instead of being silently ignored.
#pragma once

#include <windows.h>

#include <string>
#include <system_error>

namespace mediactl {

[[noreturn]] inline void throw_last_error(const char* api) {
    const DWORD code = ::GetLastError();
    throw std::system_error(static_cast<int>(code), std::system_category(), api);
}

/// Throws unless `succeeded`, naming the API that failed.
inline void ensure(bool succeeded, const char* api) {
    if (!succeeded) {
        throw_last_error(api);
    }
}

}  // namespace mediactl
