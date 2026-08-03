#ifndef CONTROLMUX_OVERLAY_RENDERER_HPP
#define CONTROLMUX_OVERLAY_RENDERER_HPP

#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include "device_manager.hpp"

class OverlayRenderer {
public:
    OverlayRenderer();
    ~OverlayRenderer();

    bool Initialize(HINSTANCE hInstance);
    void Render(const std::vector<PersonState>& persons, int active_person_id);
    void Shutdown();

    HWND GetHWND() const { return m_hwnd; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND m_hwnd = NULL;
    int m_screen_x = 0;
    int m_screen_y = 0;
    int m_screen_w = 0;
    int m_screen_h = 0;
    ULONG_PTR m_gdiplusToken = 0;

    void DrawCursor(Gdiplus::Graphics& g, const PersonState& person, bool is_active);
};

#endif // CONTROLMUX_OVERLAY_RENDERER_HPP
