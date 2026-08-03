/**
 * @file overlay_renderer.hpp
 * @brief Transparent double-buffered GDI+ overlay renderer for rendering virtual cursors.
 */

#ifndef CONTROLMUX_OVERLAY_RENDERER_HPP
#define CONTROLMUX_OVERLAY_RENDERER_HPP

#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include "device_manager.hpp"

/**
 * @brief Manages a topmost, transparent layered Win32 overlay window and renders virtual pointers.
 */
class OverlayRenderer {
public:
    OverlayRenderer();
    ~OverlayRenderer();

    /**
     * @brief Initializes GDI+ runtime and creates the click-through layered overlay window.
     * @param hInstance Module instance handle.
     * @return True if overlay window creation succeeded.
     */
    bool Initialize(HINSTANCE hInstance);

    /**
     * @brief Redraws all person virtual cursors and updates the layered window buffer.
     * @param persons List of PersonState objects to render.
     * @param active_person_id ID of currently active person.
     */
    void Render(const std::vector<PersonState>& persons, int active_person_id);

    /**
     * @brief Destroys overlay window and shuts down GDI+.
     */
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

    /**
     * @brief Draws a custom colored cursor arrow and label badge for a person profile.
     */
    void DrawCursor(Gdiplus::Graphics& g, const PersonState& person, bool is_active);
};

#endif // CONTROLMUX_OVERLAY_RENDERER_HPP
