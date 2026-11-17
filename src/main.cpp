#define UNICODE
#define _UNICODE
#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>
#include <gdiplus.h>
#include <shlwapi.h>
#include <commctrl.h>
#include "resource.h"

using namespace Gdiplus;

namespace {
constexpr wchar_t WindowClass[] = L"NoSiestaWindow";
constexpr int ContentWidth = 430;
constexpr int ContentHeight = 270;
constexpr int AboutButtonId = 1001;
constexpr UINT ShowAboutMessage = WM_APP + 1;
HWND aboutButton = nullptr;
HWND aboutTooltip = nullptr;
constexpr wchar_t AboutText[] = L"NoSiesta v1.0\nFree & open-source \u00b7 MIT License\nMade with \u2615";

void ShowAboutTooltip(bool visible) {
    TOOLINFOW tool{TTTOOLINFOW_V2_SIZE};
    tool.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
    tool.hwnd = GetParent(aboutButton);
    tool.uId = reinterpret_cast<UINT_PTR>(aboutButton);
    RECT bounds{};
    GetWindowRect(aboutButton, &bounds);
    const int gap = MulDiv(6, GetDpiForWindow(aboutButton), 96);
    SendMessageW(aboutTooltip, TTM_TRACKPOSITION, 0,
        MAKELPARAM(bounds.right + gap, bounds.top));
    SendMessageW(aboutTooltip, TTM_TRACKACTIVATE, visible, reinterpret_cast<LPARAM>(&tool));
}

LRESULT CALLBACK AboutProcedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam,
    UINT_PTR, DWORD_PTR) {
    switch (message) {
    case WM_MOUSEMOVE: {
        TRACKMOUSEEVENT tracking{sizeof(tracking), TME_LEAVE, window, 0};
        TrackMouseEvent(&tracking);
        ShowAboutTooltip(true);
        break;
    }
    case WM_MOUSELEAVE:
        if (GetFocus() != window) ShowAboutTooltip(false);
        break;
    case WM_SETFOCUS:
        ShowAboutTooltip(true);
        break;
    case WM_KILLFOCUS:
        ShowAboutTooltip(false);
        break;
    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE) {
            ShowAboutTooltip(false);
            return 0;
        }
        break;
    case WM_NCDESTROY:
        RemoveWindowSubclass(window, AboutProcedure, 1);
        break;
    }
    return DefSubclassProc(window, message, wparam, lparam);
}

void DrawArtwork(Graphics& canvas) {
    HMODULE module = GetModuleHandleW(nullptr);
    HRSRC groupResource = FindResourceW(module, MAKEINTRESOURCEW(IDI_NOSIESTA), RT_GROUP_ICON);
    if (!groupResource) return;
    auto groupData = static_cast<BYTE*>(LockResource(LoadResource(module, groupResource)));
    if (!groupData) return;
    const int imageId = LookupIconIdFromDirectoryEx(groupData, TRUE, 256, 256, LR_DEFAULTCOLOR);
    if (!imageId) return;
    HRSRC imageResource = FindResourceW(module, MAKEINTRESOURCEW(imageId), RT_ICON);
    if (!imageResource) return;
    auto imageData = static_cast<const BYTE*>(LockResource(LoadResource(module, imageResource)));
    if (!imageData) return;
    IStream* stream = SHCreateMemStream(imageData, SizeofResource(module, imageResource));
    if (!stream) return;
    {
        // Decode the embedded PNG directly to preserve its full alpha channel.
        Bitmap artwork(stream);
        if (artwork.GetLastStatus() == Ok) {
            canvas.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            canvas.DrawImage(&artwork, Rect(183, 52, 64, 64));
        }
    }
    stream->Release();
}

void PaintWindow(HWND window, HDC dc) {
    RECT client{};
    GetClientRect(window, &client);
    const int width = client.right;
    const int height = client.bottom;
    if (width <= 0 || height <= 0) return;

    Bitmap buffer(width, height, PixelFormat32bppPARGB);
    Graphics canvas(&buffer);
    canvas.SetSmoothingMode(SmoothingModeAntiAlias);
    canvas.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    canvas.ScaleTransform(width / float(ContentWidth), height / float(ContentHeight));

    LinearGradientBrush background(Point(0, 0), Point(0, ContentHeight),
        Color(255, 214, 242, 255), Color(255, 113, 191, 222));
    canvas.FillRectangle(&background, 0, 0, ContentWidth, ContentHeight);

    // Translucent, overlapping waves give the surface its Aero glass sheen.
    GraphicsPath wave;
    wave.AddBezier(-20, 80, 110, 150, 310, -20, 450, 62);
    wave.AddLine(450, 62, 450, -10);
    wave.AddLine(450, -10, -20, -10);
    wave.CloseFigure();
    LinearGradientBrush sheen(Point(0, 0), Point(0, 125),
        Color(210, 255, 255, 255), Color(15, 255, 255, 255));
    canvas.FillPath(&sheen, &wave);

    GraphicsPath lowerWave;
    lowerWave.AddBezier(-20, 195, 135, 122, 295, 244, 450, 139);
    lowerWave.AddLine(450, 139, 450, ContentHeight);
    lowerWave.AddLine(450, ContentHeight, -20, ContentHeight);
    lowerWave.CloseFigure();
    SolidBrush lowerGlass(Color(38, 255, 255, 255));
    canvas.FillPath(&lowerGlass, &lowerWave);

    // Keep the reading surface opaque so background waves cannot cross the text.
    LinearGradientBrush panel(Point(0, 18), Point(0, ContentHeight - 18),
        Color(255, 243, 252, 255), Color(255, 208, 237, 247));
    GraphicsPath panelShape;
    constexpr int radius = 14;
    panelShape.AddArc(25, 18, radius, radius, 180, 90);
    panelShape.AddArc(391, 18, radius, radius, 270, 90);
    panelShape.AddArc(391, ContentHeight - 32, radius, radius, 0, 90);
    panelShape.AddArc(25, ContentHeight - 32, radius, radius, 90, 90);
    panelShape.CloseFigure();
    canvas.FillPath(&panel, &panelShape);
    Pen panelBorder(Color(230, 255, 255, 255), 1.0f);
    canvas.DrawPath(&panelBorder, &panelShape);

    DrawArtwork(canvas);

    FontFamily family(L"Segoe UI");
    Font headline(&family, 25, FontStyleRegular, UnitPixel);
    Font detail(&family, 13, FontStyleRegular, UnitPixel);
    StringFormat centered;
    centered.SetAlignment(StringAlignmentCenter);
    centered.SetLineAlignment(StringAlignmentCenter);
    SolidBrush shadow(Color(230, 255, 255, 255));
    SolidBrush ink(Color(255, 21, 73, 105));
    SolidBrush secondary(Color(255, 49, 93, 116));
    canvas.DrawString(L"Go touch grass.", -1, &headline,
        RectF(35, 129, 360, 39), &centered, &shadow);
    canvas.DrawString(L"Go touch grass.", -1, &headline,
        RectF(35, 128, 360, 39), &centered, &ink);
    canvas.DrawString(L"I'll keep your PC awake.", -1, &detail,
        RectF(35, 173, 360, 24), &centered, &secondary);
    canvas.DrawString(L"Close this window when you're back.", -1, &detail,
        RectF(35, 192, 360, 24), &centered, &secondary);

    Pen edge(Color(180, 255, 255, 255), 1.0f);
    canvas.DrawRectangle(&edge, 0, 0, ContentWidth - 1, ContentHeight - 1);
    Graphics screen(dc);
    screen.DrawImage(&buffer, 0, 0);
}

LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_CREATE: {
        aboutButton = CreateWindowExW(0, L"BUTTON", L"About NoSiesta",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW, 0, 0, 0, 0,
            window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(AboutButtonId)),
            GetModuleHandleW(nullptr), nullptr);
        aboutTooltip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
            WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT, window, nullptr, GetModuleHandleW(nullptr), nullptr);
        if (!aboutButton || !aboutTooltip) return -1;
        TOOLINFOW tool{TTTOOLINFOW_V2_SIZE};
        tool.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
        tool.hwnd = window;
        tool.uId = reinterpret_cast<UINT_PTR>(aboutButton);
        tool.lpszText = const_cast<wchar_t*>(AboutText);
        SendMessageW(aboutTooltip, TTM_SETMAXTIPWIDTH, 0, 320);
        if (!SendMessageW(aboutTooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&tool))) return -1;
        if (!SetWindowSubclass(aboutButton, AboutProcedure, 1, 0)) return -1;
        return 0;
    }
    case WM_SIZE:
        SetWindowPos(aboutButton, nullptr, MulDiv(369, LOWORD(lparam), ContentWidth),
            MulDiv(220, HIWORD(lparam), ContentHeight), MulDiv(24, LOWORD(lparam), ContentWidth),
            MulDiv(24, HIWORD(lparam), ContentHeight), SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    case WM_DRAWITEM: {
        const auto item = reinterpret_cast<DRAWITEMSTRUCT*>(lparam);
        if (item->CtlID != AboutButtonId) break;
        Graphics graphics(item->hDC);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
        const float size = float(item->rcItem.right - item->rcItem.left);
        graphics.ScaleTransform(size / 24, size / 24);
        LinearGradientBrush background(Point(0, -202), Point(0, 32),
            Color(255, 243, 252, 255), Color(255, 208, 237, 247));
        graphics.FillRectangle(&background, 0, 0, 24, 24);
        Pen outline(Color(255, 64, 118, 146), 1.0f);
        graphics.DrawEllipse(&outline, 4, 4, 16, 16);
        Font font(L"Segoe UI", 12, FontStyleBold, UnitPixel);
        SolidBrush ink(Color(255, 49, 93, 116));
        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);
        graphics.DrawString(L"i", -1, &font, RectF(4, 3, 16, 17), &format, &ink);
        if (item->itemState & ODS_FOCUS) {
            outline.SetDashStyle(DashStyleDot);
            graphics.DrawRectangle(&outline, 1, 1, 22, 22);
        }
        return TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wparam) == AboutButtonId && HIWORD(wparam) == BN_CLICKED) {
            // Open after the button finishes processing the mouse release.
            PostMessageW(window, ShowAboutMessage, 0, 0);
            return 0;
        }
        break;
    case ShowAboutMessage:
        ShowAboutTooltip(false);
        ShowAboutTooltip(true);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        PaintWindow(window, dc);
        EndPaint(window, &paint);
        return 0;
    }
    case WM_DPICHANGED: {
        const auto bounds = reinterpret_cast<const RECT*>(lparam);
        SetWindowPos(window, nullptr, bounds->left, bounds->top,
            bounds->right - bounds->left, bounds->bottom - bounds->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        InvalidateRect(window, nullptr, FALSE);
        return 0;
    }
    case WM_DESTROY:
        SetThreadExecutionState(ES_CONTINUOUS);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_WIN95_CLASSES};
    if (!InitCommonControlsEx(&controls)) return 1;
    GdiplusStartupInput input;
    ULONG_PTR graphicsToken = 0;
    if (GdiplusStartup(&graphicsToken, &input, nullptr) != Ok) {
        MessageBoxW(nullptr, L"NoSiesta could not initialize its window graphics.",
            L"NoSiesta", MB_OK | MB_ICONERROR);
        return 1;
    }

    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_NOSIESTA));
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = WindowClass;
    if (!RegisterClassW(&windowClass)) {
        GdiplusShutdown(graphicsToken);
        return 1;
    }

    constexpr DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    UINT dpi = GetDpiForSystem();
    RECT bounds{0, 0, MulDiv(ContentWidth, dpi, 96), MulDiv(ContentHeight, dpi, 96)};
    AdjustWindowRectExForDpi(&bounds, style, FALSE, 0, dpi);
    RECT workArea{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    int width = bounds.right - bounds.left;
    int height = bounds.bottom - bounds.top;
    HWND window = CreateWindowExW(0, WindowClass, L"NoSiesta", style,
        workArea.left + (workArea.right - workArea.left - width) / 2,
        workArea.top + (workArea.bottom - workArea.top - height) / 2,
        width, height, nullptr, nullptr, instance, nullptr);
    if (!window) {
        GdiplusShutdown(graphicsToken);
        return 1;
    }

    const UINT windowDpi = GetDpiForWindow(window);
    HICON smallIcon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_NOSIESTA),
        IMAGE_ICON, GetSystemMetricsForDpi(SM_CXSMICON, windowDpi),
        GetSystemMetricsForDpi(SM_CYSMICON, windowDpi), LR_SHARED));
    SendMessageW(window, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon));

    // Supported Windows versions color the native frame and retain standard controls.
    COLORREF caption = RGB(86, 159, 195);
    COLORREF captionText = RGB(255, 255, 255);
    DwmSetWindowAttribute(window, DWMWA_CAPTION_COLOR, &caption, sizeof(caption));
    DwmSetWindowAttribute(window, DWMWA_TEXT_COLOR, &captionText, sizeof(captionText));
    DWM_WINDOW_CORNER_PREFERENCE corners = DWMWCP_ROUND;
    DwmSetWindowAttribute(window, DWMWA_WINDOW_CORNER_PREFERENCE, &corners, sizeof(corners));

    if (!SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED)) {
        MessageBoxW(window, L"Windows could not activate awake mode. Please open NoSiesta again.",
            L"NoSiesta", MB_OK | MB_ICONERROR);
        DestroyWindow(window);
        GdiplusShutdown(graphicsToken);
        return 1;
    }
    ShowWindow(window, showCommand);
    UpdateWindow(window);
    MSG message{};
    int result;
    while ((result = GetMessageW(&message, nullptr, 0, 0)) > 0) {
        if (!IsDialogMessageW(window, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    if (IsWindow(window)) DestroyWindow(window);
    GdiplusShutdown(graphicsToken);
    return result == -1 ? 1 : static_cast<int>(message.wParam);
}
