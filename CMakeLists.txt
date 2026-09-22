cmake_minimum_required(VERSION 3.21)

project(MediaControl
    VERSION 2.0.0
    DESCRIPTION "Atalhos de teclado para controlar a midia no Windows"
    LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "Media Control is a Win32 application and only builds for Windows.")
endif()

enable_language(RC)

option(MEDIACONTROL_STATIC_RUNTIME
    "Link the C/C++ runtime statically so the executable runs on its own" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Exported for clangd, Visual Studio Code and any other tool that reads it.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(media-control WIN32
    src/main.cpp
    src/application.cpp
    src/help_window.cpp
    src/hotkey_listener.cpp
    src/media_command.cpp
    src/tray_icon.cpp
    src/app.rc)

set_target_properties(media-control PROPERTIES OUTPUT_NAME "MediaControl")

# assets/ is on the include path so that app.rc can name app.ico directly.
target_include_directories(media-control PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets")

target_compile_definitions(media-control PRIVATE
    UNICODE
    _UNICODE
    STRICT
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00)

target_link_libraries(media-control PRIVATE
    user32     # windows, messages, the keyboard hook
    shell32    # the notification area
    comctl32   # LoadIconMetric and visual styles
    gdi32      # drawing the help window
    dwmapi     # dark title bar
    advapi32)  # reading the system light/dark preference

if(MSVC)
    target_compile_options(media-control PRIVATE /W4 /permissive- /utf-8 /EHsc)

    # app.rc already carries the manifest; keep the linker from adding a second.
    target_link_options(media-control PRIVATE /MANIFEST:NO)

    if(MEDIACONTROL_STATIC_RUNTIME)
        set_property(TARGET media-control PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
else()
    target_compile_options(media-control PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow
        -finput-charset=UTF-8
        -fexec-charset=UTF-8
        -fwide-exec-charset=UTF-16LE)

    # Selects the wide start-up code, so the entry point is wWinMain as it is
    # under MSVC. Without it the MinGW runtime looks for a narrow WinMain.
    target_compile_options(media-control PRIVATE -municode)
    target_link_options(media-control PRIVATE -municode)

    if(MEDIACONTROL_STATIC_RUNTIME)
        target_link_options(media-control PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
endif()
