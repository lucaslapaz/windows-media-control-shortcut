#pragma once

#include <windows.h>

#include "shortcuts.hpp"

namespace mediactl {

/// Stamped on every keystroke this program injects, so the keyboard listener
/// can recognise its own output and let it pass. Keying on the generic
/// "injected" flag instead would also reject keystrokes from remote desktop
/// clients, on-screen keyboards and macro software, which have every reason to
/// work.
inline constexpr ULONG_PTR kInjectionTag = 0x4D43'0001;  // 'MC'

/// Injects the multimedia key that `command` stands for, exactly as a keyboard
/// with dedicated media keys would report it. Windows takes it from there: it
/// routes playback keys to the app that currently owns the media session and
/// shows its own volume overlay for the volume keys.
void send_media_command(MediaCommand command) noexcept;

}  // namespace mediactl
