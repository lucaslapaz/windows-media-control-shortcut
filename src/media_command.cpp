#include "media_command.hpp"

#include <windows.h>

#include <iterator>

namespace mediactl {
namespace {

WORD virtual_key_for(MediaCommand command) noexcept {
    switch (command) {
        case MediaCommand::PlayPause:
            return VK_MEDIA_PLAY_PAUSE;
        case MediaCommand::PreviousTrack:
            return VK_MEDIA_PREV_TRACK;
        case MediaCommand::NextTrack:
            return VK_MEDIA_NEXT_TRACK;
        case MediaCommand::VolumeDown:
            return VK_VOLUME_DOWN;
        case MediaCommand::VolumeUp:
            return VK_VOLUME_UP;
        case MediaCommand::ToggleMute:
            return VK_VOLUME_MUTE;
    }
    return 0;
}

}  // namespace

void send_media_command(MediaCommand command) noexcept {
    const WORD key = virtual_key_for(command);
    if (key == 0) {
        return;
    }

    INPUT events[2] = {};
    for (INPUT& event : events) {
        event.type = INPUT_KEYBOARD;
        event.ki.wVk = key;
        event.ki.dwExtraInfo = kInjectionTag;
    }
    events[1].ki.dwFlags = KEYEVENTF_KEYUP;

    ::SendInput(static_cast<UINT>(std::size(events)), events, sizeof(INPUT));
}

}  // namespace mediactl
