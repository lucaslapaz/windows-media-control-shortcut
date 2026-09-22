// Small ownership wrappers for the handful of Win32 handle types this program
// creates. They exist so that no cleanup call is ever written by hand.
#pragma once

#include <windows.h>

#include <memory>
#include <type_traits>

namespace mediactl {

namespace detail {

struct HandleDeleter {
    void operator()(HANDLE handle) const noexcept { ::CloseHandle(handle); }
};

struct GdiObjectDeleter {
    void operator()(HGDIOBJ object) const noexcept { ::DeleteObject(object); }
};

struct IconDeleter {
    void operator()(HICON icon) const noexcept { ::DestroyIcon(icon); }
};

struct MenuDeleter {
    void operator()(HMENU menu) const noexcept { ::DestroyMenu(menu); }
};

template <typename Handle, typename Deleter>
using unique_win32 = std::unique_ptr<std::remove_pointer_t<Handle>, Deleter>;

}  // namespace detail

using unique_handle = detail::unique_win32<HANDLE, detail::HandleDeleter>;
using unique_font = detail::unique_win32<HFONT, detail::GdiObjectDeleter>;
using unique_brush = detail::unique_win32<HBRUSH, detail::GdiObjectDeleter>;
using unique_bitmap = detail::unique_win32<HBITMAP, detail::GdiObjectDeleter>;
using unique_icon = detail::unique_win32<HICON, detail::IconDeleter>;
using unique_menu = detail::unique_win32<HMENU, detail::MenuDeleter>;

}  // namespace mediactl
