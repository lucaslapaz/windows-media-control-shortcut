#pragma once

#include <windows.h>

#include <array>
#include <string_view>

namespace mediactl {

/// One of the media actions this program can trigger.
enum class MediaCommand {
    PlayPause,
    PreviousTrack,
    NextTrack,
    VolumeDown,
    VolumeUp,
    ToggleMute,
};

struct Shortcut {
    int trigger_key;                 ///< Virtual-key code pressed together with the right Ctrl.
    MediaCommand command;            ///< Action performed when the chord is pressed.
    std::wstring_view chord;         ///< Chord as shown to the user.
    std::wstring_view description;   ///< What the chord does, as shown to the user.
};

/// The authoritative shortcut table. The keyboard listener and the help window
/// both read it, so what the program does and what it documents cannot drift
/// apart.
inline constexpr std::array kShortcuts = std::to_array<Shortcut>({
    {VK_F5, MediaCommand::PlayPause, L"Ctrl direito + F5", L"Reproduzir ou pausar"},
    {VK_F6, MediaCommand::PreviousTrack, L"Ctrl direito + F6", L"Faixa anterior"},
    {VK_F7, MediaCommand::NextTrack, L"Ctrl direito + F7", L"Próxima faixa"},
    {VK_F8, MediaCommand::VolumeDown, L"Ctrl direito + F8", L"Diminuir o volume"},
    {VK_F9, MediaCommand::VolumeUp, L"Ctrl direito + F9", L"Aumentar o volume"},
    {VK_F10, MediaCommand::ToggleMute, L"Ctrl direito + F10", L"Ativar ou desativar o mudo"},
});

}  // namespace mediactl
