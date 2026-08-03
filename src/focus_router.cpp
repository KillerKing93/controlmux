/**
 * @file focus_router.cpp
 * @brief Implementation of target window resolution and input focus routing algorithms.
 */

#include "focus_router.hpp"
#include <iostream>

FocusRouter::FocusRouter() {}

FocusRouter::~FocusRouter() {}

/**
 * Uses WindowFromPoint and GetAncestor to locate the active window under a given screen coordinate.
 */
HWND FocusRouter::FindTargetWindowAt(int x, int y) {
    POINT pt = { x, y };
    HWND hwnd = WindowFromPoint(pt);
    if (hwnd) {
        // Find top-level ancestor window
        HWND parent = GetAncestor(hwnd, GA_ROOT);
        if (parent) return parent;
    }
    return hwnd;
}

/**
 * Updates virtual cursor location, clamping coordinates within virtual screen desktop bounds.
 */
void FocusRouter::OnPersonMouseMove(PersonState& person, int dx, int dy, AppConfig& config) {
    int screen_x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int screen_y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int screen_w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int screen_h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    if (person.id == m_active_person_id) {
        // Active focus person matches physical OS mouse cursor position to 100% exact pixel
        POINT pt;
        if (GetCursorPos(&pt)) {
            person.cursor_x = pt.x;
            person.cursor_y = pt.y;
        }
    } else {
        // Secondary persons accumulate relative movement deltas
        person.cursor_x += dx;
        person.cursor_y += dy;

        // Clamp coordinates to stay within visible virtual screen bounds
        if (person.cursor_x < screen_x) person.cursor_x = screen_x;
        if (person.cursor_x >= screen_x + screen_w) person.cursor_x = screen_x + screen_w - 1;
        if (person.cursor_y < screen_y) person.cursor_y = screen_y;
        if (person.cursor_y >= screen_y + screen_h) person.cursor_y = screen_y + screen_h - 1;
    }

    person.target_hwnd = FindTargetWindowAt(person.cursor_x, person.cursor_y);
}

/**
 * Updates button down states and activates focus when a person clicks.
 */
void FocusRouter::OnPersonMouseButton(PersonState& person, DWORD button_flags, AppConfig& config) {
    if (button_flags & RI_MOUSE_LEFT_BUTTON_DOWN) {
        person.mouse_down_left = true;
        m_active_person_id = person.id;
        config.active_person_id = person.id;
        ActivatePersonFocus(person);
    }
    if (button_flags & RI_MOUSE_LEFT_BUTTON_UP) {
        person.mouse_down_left = false;
    }
    if (button_flags & RI_MOUSE_RIGHT_BUTTON_DOWN) {
        person.mouse_down_right = true;
        m_active_person_id = person.id;
        config.active_person_id = person.id;
        ActivatePersonFocus(person);
    }
    if (button_flags & RI_MOUSE_RIGHT_BUTTON_UP) {
        person.mouse_down_right = false;
    }
}

/**
 * Switches OS hardware pointer and foreground focus context to the specified person.
 */
void FocusRouter::ActivatePersonFocus(PersonState& person) {
    SetCursorPos(person.cursor_x, person.cursor_y);

    HWND target = person.target_hwnd;
    if (target && IsWindow(target)) {
        SetForegroundWindow(target);
        BringWindowToTop(target);
    }
}

/**
 * Routes keyboard input according to selected mode (SwitchedFocus vs DirectTarget).
 */
bool FocusRouter::OnPersonKeyboardInput(PersonState& person, USHORT vkey, USHORT flags, AppConfig& config) {
    if (!config.enabled) return false;

    if (config.mode == RoutingMode::SwitchedFocus) {
        if (person.id == m_active_person_id) {
            // Active person's keypress passes through normally to active OS window
            return false; 
        } else {
            // Secondary person's keypress is isolated and sent directly to secondary person's target window
            RouteKeyToWindow(person.target_hwnd, vkey, flags);
            return true; // Filter out from main OS focus
        }
    } else if (config.mode == RoutingMode::DirectTarget) {
        // Direct target mode: Route keypress directly to targeted window handle
        RouteKeyToWindow(person.target_hwnd, vkey, flags);
        return true;
    }

    return false;
}

/**
 * Posts Win32 WM_KEYDOWN, WM_KEYUP, and WM_CHAR messages to target HWND.
 */
void FocusRouter::RouteKeyToWindow(HWND hwnd, USHORT vkey, USHORT flags) {
    if (!hwnd || !IsWindow(hwnd)) return;

    bool is_up = (flags & RI_KEY_BREAK);
    UINT msg = is_up ? WM_KEYUP : WM_KEYDOWN;

    LPARAM lParam = 1; // Repeat count
    UINT scan_code = MapVirtualKeyW(vkey, MAPVK_VK_TO_VSC);
    lParam |= (scan_code << 16);
    if (flags & RI_KEY_E0) lParam |= (1 << 24);
    if (is_up) {
        lParam |= (1 << 30);
        lParam |= (1 << 31);
    }

    PostMessageW(hwnd, msg, vkey, lParam);

    // Send character translation on keydown
    if (!is_up) {
        BYTE key_state[256];
        GetKeyboardState(key_state);
        WCHAR wchar_buf[4] = { 0 };
        if (ToUnicode(vkey, scan_code, key_state, wchar_buf, 4, 0) > 0) {
            PostMessageW(hwnd, WM_CHAR, wchar_buf[0], lParam);
        }
    }
}
