/**
 * @file overlay_renderer.cpp
 * @brief Implementation of GDI+ transparent overlay window rendering for multi-cursor visualization.
 */

#include "overlay_renderer.hpp"
#include <iostream>

#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

OverlayRenderer::OverlayRenderer() {}

OverlayRenderer::~OverlayRenderer() {
    Shutdown();
}

LRESULT CALLBACK OverlayRenderer::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_ERASEBKGND:
        return 1; // Prevent background erasing flicker
    case WM_DESTROY:
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

/**
 * Creates the topmost click-through transparent layered window spanning all active monitors.
 */
bool OverlayRenderer::Initialize(HINSTANCE hInstance) {
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

    // Get total virtual screen area across all displays
    m_screen_x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    m_screen_y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    m_screen_w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    m_screen_h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszClassName = L"ControlMuxOverlayClass";

    RegisterClassExW(&wc);

    // Create layered, click-through, topmost window
    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
        L"ControlMuxOverlayClass",
        L"ControlMux Virtual Cursor Overlay",
        WS_POPUP,
        m_screen_x, m_screen_y, m_screen_w, m_screen_h,
        NULL, NULL, hInstance, NULL
    );

    if (!m_hwnd) return false;

    ShowWindow(m_hwnd, SW_SHOWNA);
    UpdateWindow(m_hwnd);

    return true;
}

/**
 * Double-buffers and updates the alpha channel of the layered window via UpdateLayeredWindow.
 */
void OverlayRenderer::Render(const std::vector<PersonState>& persons, int active_person_id) {
    if (!m_hwnd) return;

    // Dynamically refresh virtual screen bounds across all monitors
    m_screen_x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    m_screen_y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    m_screen_w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    m_screen_h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    // Re-assert HWND_TOPMOST so overlay window NEVER slips behind other windows
    SetWindowPos(m_hwnd, HWND_TOPMOST, m_screen_x, m_screen_y, m_screen_w, m_screen_h,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = m_screen_w;
    bmi.bmiHeader.biHeight = -m_screen_h; // Top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    VOID* pBits = NULL;
    HBITMAP hbmMem = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

    Graphics graphics(hdcMem);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    // Clear buffer with 100% transparent black
    graphics.Clear(Color(0, 0, 0, 0));

    // Render virtual cursors for each configured person profile
    for (const auto& person : persons) {
        DrawCursor(graphics, person, (person.id == active_person_id));
    }

    POINT ptSrc = { 0, 0 };
    SIZE sizeWnd = { m_screen_w, m_screen_h };
    POINT ptDst = { m_screen_x, m_screen_y };

    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    UpdateLayeredWindow(m_hwnd, hdcScreen, &ptDst, &sizeWnd, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hbmOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

/**
 * Draws a colored pointer arrow, drop shadow, outline, and name badge pill tag.
 */
void OverlayRenderer::DrawCursor(Graphics& g, const PersonState& person, bool is_active) {
    int x = person.cursor_x - m_screen_x;
    int y = person.cursor_y - m_screen_y;

    Color mainColor(person.color);
    Color outlineColor(255, 255, 255, 255);
    Color darkOutline(200, 0, 0, 0);

    // Draw mouse button click ripple animation
    if (person.mouse_down_left || person.mouse_down_right) {
        Pen ripplePen(mainColor, 2.5f);
        g.DrawEllipse(&ripplePen, x - 12, y - 12, 24, 24);
    }

    // Pointer arrow polygon vertices
    Point cursorPoints[] = {
        Point(x, y),
        Point(x + 5, y + 18),
        Point(x + 9, y + 14),
        Point(x + 15, y + 20),
        Point(x + 18, y + 17),
        Point(x + 12, y + 11),
        Point(x + 19, y + 11)
    };

    GraphicsPath path;
    path.AddPolygon(cursorPoints, 7);

    // 1. Dark drop shadow stroke
    Pen shadowPen(darkOutline, 4.0f);
    shadowPen.SetLineJoin(LineJoinRound);
    g.DrawPath(&shadowPen, &path);

    // 2. White outline stroke
    Pen outlinePen(outlineColor, 2.0f);
    outlinePen.SetLineJoin(LineJoinRound);
    g.DrawPath(&outlinePen, &path);

    // 3. Colored body fill
    SolidBrush fillBrush(mainColor);
    g.FillPath(&fillBrush, &path);

    // 4. Name Badge Pill Tag (e.g., "Person 1 ★")
    Font font(L"Segoe UI", 9, FontStyleBold, UnitPixel);
    std::wstring badgeText = person.name + (is_active ? L" ★" : L"");

    RectF textBounds;
    g.MeasureString(badgeText.c_str(), -1, &font, PointF(0, 0), &textBounds);

    int badgeWidth = (int)textBounds.Width + 10;
    int badgeHeight = (int)textBounds.Height + 4;
    int badgeX = x + 16;
    int badgeY = y + 16;

    SolidBrush badgeBg(Color(220, 20, 24, 30));
    Pen badgeBorder(mainColor, 1.5f);

    g.FillRectangle(&badgeBg, badgeX, badgeY, badgeWidth, badgeHeight);
    g.DrawRectangle(&badgeBorder, badgeX, badgeY, badgeWidth, badgeHeight);

    SolidBrush textBrush(Color(255, 255, 255, 255));
    g.DrawString(badgeText.c_str(), -1, &font, PointF((REAL)badgeX + 5, (REAL)badgeY + 2), &textBrush);
}

void OverlayRenderer::Shutdown() {
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }
    if (m_gdiplusToken) {
        GdiplusShutdown(m_gdiplusToken);
        m_gdiplusToken = 0;
    }
}
