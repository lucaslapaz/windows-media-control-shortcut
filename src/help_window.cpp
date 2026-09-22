#include "help_window.hpp"

#include <dwmapi.h>

#include <string_view>

#include "shortcuts.hpp"
#include "win32_error.hpp"

// Supported since Windows 10 1809, but missing from some SDK headers.
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace mediactl {
namespace {

constexpr wchar_t kClassName[] = L"MediaControl.HelpWindow";
constexpr wchar_t kWindowTitle[] = L"Media Control - atalhos e ajuda";

constexpr std::wstring_view kHeading = L"Media Control";

constexpr std::wstring_view kIntroduction =
    L"O Media Control fica em segundo plano e transforma o Ctrl direito nas teclas "
    L"de mídia que o seu teclado não tem. Basta deixar o programa aberto: os "
    L"atalhos funcionam em qualquer aplicativo, mesmo com esta janela fechada.";

constexpr std::wstring_view kTableHeading = L"Atalhos disponíveis";

constexpr std::wstring_view kFooter =
    L"O Ctrl esquerdo continua livre, então atalhos como Ctrl+F5 nos navegadores e "
    L"nos editores seguem funcionando normalmente.\n\n"
    L"Para encerrar o programa, clique no ícone ao lado do relógio e escolha Sair.";

// Layout measurements in pixels at 96 DPI; everything else is scaled from these.
constexpr int kPadding = 24;
constexpr int kContentWidth = 440;
constexpr int kRowHeight = 32;
constexpr int kChordColumnWidth = 190;
constexpr int kRowIndent = 12;
constexpr int kGapAfterHeading = 10;
constexpr int kGapAfterIntroduction = 22;
constexpr int kGapAfterTableHeading = 10;
constexpr int kGapAfterTable = 20;

bool system_prefers_dark_mode() noexcept {
    DWORD light_theme = 1;
    DWORD size = sizeof(light_theme);
    const LSTATUS status = ::RegGetValueW(
        HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &light_theme, &size);
    return status == ERROR_SUCCESS && light_theme == 0;
}

int measure(HDC device, HFONT font, std::wstring_view text, int width) noexcept {
    const HGDIOBJ previous = ::SelectObject(device, font);
    RECT box{0, 0, width, 0};
    ::DrawTextW(device, text.data(), static_cast<int>(text.size()), &box,
                DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
    ::SelectObject(device, previous);
    return box.bottom - box.top;
}

void draw(HDC device, HFONT font, COLORREF colour, std::wstring_view text, RECT area,
          UINT format) noexcept {
    const HGDIOBJ previous = ::SelectObject(device, font);
    ::SetTextColor(device, colour);
    ::DrawTextW(device, text.data(), static_cast<int>(text.size()), &area, format | DT_NOPREFIX);
    ::SelectObject(device, previous);
}

void fill(HDC device, const RECT& area, COLORREF colour) noexcept {
    const unique_brush brush{::CreateSolidBrush(colour)};
    if (brush) {
        ::FillRect(device, &area, brush.get());
    }
}

}  // namespace

struct HelpWindow::Palette {
    COLORREF background;
    COLORREF heading;
    COLORREF text;
    COLORREF muted;
    COLORREF accent;
    COLORREF row;
};

struct HelpWindow::Layout {
    int window_width{};
    int total_height{};
    int row_height{};
    int chord_column{};
    int row_indent{};
    RECT heading{};
    RECT introduction{};
    RECT table_heading{};
    RECT table{};
    RECT footer{};
};

HelpWindow::HelpWindow(HINSTANCE instance, HICON large_icon, HICON small_icon)
    : instance_(instance), large_icon_(large_icon), small_icon_(small_icon) {
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = &HelpWindow::window_proc;
    window_class.hInstance = instance_;
    window_class.hIcon = large_icon_;
    window_class.hIconSm = small_icon_;
    window_class.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    window_class.lpszClassName = kClassName;

    window_class_ = ::RegisterClassExW(&window_class);
    ensure(window_class_ != 0, "RegisterClassExW(HelpWindow)");
}

HelpWindow::~HelpWindow() {
    if (window_ != nullptr) {
        ::DestroyWindow(window_);
    }
    if (window_class_ != 0) {
        ::UnregisterClassW(kClassName, instance_);
    }
}

void HelpWindow::show() {
    if (window_ == nullptr) {
        create();
    }

    ::ShowWindow(window_, ::IsIconic(window_) ? SW_RESTORE : SW_SHOW);
    ::SetForegroundWindow(window_);
}

void HelpWindow::create() {
    // No sizing border and no maximise box: the content has one natural size.
    constexpr DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;

    // window_ is filled in by WM_NCCREATE, before this call returns.
    const HWND window = ::CreateWindowExW(0, kClassName, kWindowTitle, style, CW_USEDEFAULT,
                                          CW_USEDEFAULT, 100, 100, nullptr, nullptr, instance_,
                                          this);
    ensure(window != nullptr, "CreateWindowExW(HelpWindow)");

    dpi_ = ::GetDpiForWindow(window_);
    rebuild_fonts();
    adopt_system_theme();
    resize_to_content();
    centre_on_active_monitor();
}

void HelpWindow::adopt_system_theme() {
    dark_mode_ = system_prefers_dark_mode();

    const BOOL dark = dark_mode_ ? TRUE : FALSE;
    // Best effort: an older Windows build simply keeps its light title bar.
    ::DwmSetWindowAttribute(window_, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
}

void HelpWindow::rebuild_fonts() {
    NONCLIENTMETRICSW metrics{};
    metrics.cbSize = sizeof(metrics);

    LOGFONTW body{};
    if (::SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0, dpi_)) {
        body = metrics.lfMessageFont;
    } else {
        body.lfHeight = -scale(12);
        ::lstrcpynW(body.lfFaceName, L"Segoe UI", LF_FACESIZE);
    }
    body_font_.reset(::CreateFontIndirectW(&body));

    LOGFONTW chord = body;
    chord.lfWeight = FW_SEMIBOLD;
    chord_font_.reset(::CreateFontIndirectW(&chord));

    LOGFONTW heading = body;
    heading.lfHeight = body.lfHeight * 9 / 5;  // lfHeight is negative, so this enlarges it.
    heading.lfWeight = FW_SEMIBOLD;
    title_font_.reset(::CreateFontIndirectW(&heading));
}

HelpWindow::Layout HelpWindow::compute_layout(HDC device) const {
    Layout layout;
    const int padding = scale(kPadding);
    const int content_width = scale(kContentWidth);

    layout.window_width = content_width + 2 * padding;
    layout.row_height = scale(kRowHeight);
    layout.chord_column = scale(kChordColumnWidth);
    layout.row_indent = scale(kRowIndent);

    const int left = padding;
    const int right = left + content_width;
    int top = padding;

    const auto stack = [&](HFONT font, std::wstring_view text, RECT& area, int gap) {
        const int height = measure(device, font, text, content_width);
        area = RECT{left, top, right, top + height};
        top += height + gap;
    };

    stack(title_font_.get(), kHeading, layout.heading, scale(kGapAfterHeading));
    stack(body_font_.get(), kIntroduction, layout.introduction, scale(kGapAfterIntroduction));
    stack(chord_font_.get(), kTableHeading, layout.table_heading, scale(kGapAfterTableHeading));

    const int table_height = layout.row_height * static_cast<int>(kShortcuts.size());
    layout.table = RECT{left, top, right, top + table_height};
    top += table_height + scale(kGapAfterTable);

    stack(body_font_.get(), kFooter, layout.footer, padding);

    layout.total_height = top;
    return layout;
}

HelpWindow::Palette HelpWindow::palette() const noexcept {
    if (dark_mode_) {
        return Palette{RGB(32, 32, 32),    RGB(255, 255, 255), RGB(226, 226, 226),
                       RGB(158, 158, 158), RGB(105, 182, 255), RGB(45, 45, 45)};
    }
    return Palette{RGB(255, 255, 255), RGB(17, 17, 17),  RGB(32, 32, 32),
                   RGB(98, 98, 98),    RGB(0, 95, 184),  RGB(243, 243, 243)};
}

void HelpWindow::paint(HDC device, const RECT& client) const {
    const Palette colours = palette();
    const Layout layout = compute_layout(device);

    fill(device, client, colours.background);
    ::SetBkMode(device, TRANSPARENT);

    draw(device, title_font_.get(), colours.heading, kHeading, layout.heading, DT_WORDBREAK);
    draw(device, body_font_.get(), colours.muted, kIntroduction, layout.introduction, DT_WORDBREAK);
    draw(device, chord_font_.get(), colours.text, kTableHeading, layout.table_heading,
         DT_WORDBREAK);

    int top = layout.table.top;
    bool shaded = true;
    for (const Shortcut& shortcut : kShortcuts) {
        const RECT row{layout.table.left, top, layout.table.right, top + layout.row_height};
        if (shaded) {
            fill(device, row, colours.row);
        }
        shaded = !shaded;

        const RECT chord{row.left + layout.row_indent, row.top, row.left + layout.chord_column,
                         row.bottom};
        draw(device, chord_font_.get(), colours.accent, shortcut.chord, chord,
             DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

        const RECT action{row.left + layout.chord_column, row.top, row.right - layout.row_indent,
                          row.bottom};
        draw(device, body_font_.get(), colours.text, shortcut.description, action,
             DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

        top = row.bottom;
    }

    draw(device, body_font_.get(), colours.muted, kFooter, layout.footer, DT_WORDBREAK);
}

void HelpWindow::resize_to_content() {
    const HDC screen = ::GetDC(nullptr);
    const Layout layout = compute_layout(screen);
    ::ReleaseDC(nullptr, screen);

    RECT frame{0, 0, layout.window_width, layout.total_height};
    ::AdjustWindowRectExForDpi(&frame, static_cast<DWORD>(::GetWindowLongPtrW(window_, GWL_STYLE)),
                               FALSE, 0, dpi_);

    ::SetWindowPos(window_, nullptr, 0, 0, frame.right - frame.left, frame.bottom - frame.top,
                   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void HelpWindow::centre_on_active_monitor() {
    POINT cursor{};
    ::GetCursorPos(&cursor);

    MONITORINFO monitor{};
    monitor.cbSize = sizeof(monitor);
    if (!::GetMonitorInfoW(::MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY), &monitor)) {
        return;
    }

    RECT frame{};
    ::GetWindowRect(window_, &frame);
    const LONG width = frame.right - frame.left;
    const LONG height = frame.bottom - frame.top;

    const LONG left = monitor.rcWork.left + (monitor.rcWork.right - monitor.rcWork.left - width) / 2;
    const LONG top = monitor.rcWork.top + (monitor.rcWork.bottom - monitor.rcWork.top - height) / 2;

    ::SetWindowPos(window_, nullptr, left, top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

LRESULT CALLBACK HelpWindow::window_proc(HWND window, UINT message, WPARAM wparam,
                                         LPARAM lparam) noexcept {
    if (message == WM_NCCREATE) {
        auto* created = static_cast<HelpWindow*>(
            reinterpret_cast<const CREATESTRUCTW*>(lparam)->lpCreateParams);
        created->window_ = window;
        ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(created));
    }

    auto* self = reinterpret_cast<HelpWindow*>(::GetWindowLongPtrW(window, GWLP_USERDATA));
    if (self == nullptr) {
        return ::DefWindowProcW(window, message, wparam, lparam);
    }
    return self->handle_message(window, message, wparam, lparam);
}

LRESULT HelpWindow::handle_message(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
        case WM_ERASEBKGND:
            // WM_PAINT covers every pixel from an off-screen buffer already.
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT paint_info{};
            const HDC device = ::BeginPaint(window_, &paint_info);

            RECT client{};
            ::GetClientRect(window_, &client);

            // Composed off-screen so that repaints never flicker.
            const HDC buffer_device = ::CreateCompatibleDC(device);
            const unique_bitmap buffer{
                ::CreateCompatibleBitmap(device, client.right, client.bottom)};

            if (buffer_device != nullptr && buffer) {
                const HGDIOBJ previous = ::SelectObject(buffer_device, buffer.get());
                paint(buffer_device, client);
                ::BitBlt(device, 0, 0, client.right, client.bottom, buffer_device, 0, 0, SRCCOPY);
                ::SelectObject(buffer_device, previous);
            }

            ::DeleteDC(buffer_device);
            ::EndPaint(window_, &paint_info);
            return 0;
        }

        case WM_DPICHANGED: {
            dpi_ = HIWORD(wparam);
            rebuild_fonts();

            const RECT& suggested = *reinterpret_cast<const RECT*>(lparam);
            ::SetWindowPos(window_, nullptr, suggested.left, suggested.top, 0, 0,
                           SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            resize_to_content();
            ::InvalidateRect(window_, nullptr, TRUE);
            return 0;
        }

        case WM_SETTINGCHANGE:
            if (lparam != 0 &&
                ::lstrcmpiW(reinterpret_cast<const wchar_t*>(lparam), L"ImmersiveColorSet") == 0) {
                adopt_system_theme();
                ::InvalidateRect(window_, nullptr, TRUE);
            }
            break;

        case WM_KEYDOWN:
            if (wparam == VK_ESCAPE) {
                ::ShowWindow(window_, SW_HIDE);
                return 0;
            }
            break;

        case WM_CLOSE:
            // Kept alive so it reopens instantly, where the user left it.
            ::ShowWindow(window_, SW_HIDE);
            return 0;

        case WM_DESTROY:
            window_ = nullptr;
            return 0;

        default:
            break;
    }

    return ::DefWindowProcW(window, message, wparam, lparam);
}

}  // namespace mediactl
