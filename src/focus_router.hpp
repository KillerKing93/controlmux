/**
 * @file focus_router.hpp
 * @brief Input focus routing, target window resolution, and keyboard input isolation logic.
 */

#ifndef CONTROLMUX_FOCUS_ROUTER_HPP
#define CONTROLMUX_FOCUS_ROUTER_HPP

#include <windows.h>
#include "device_manager.hpp"
#include "config.hpp"

/**
 * @brief Handles input focus switching and keystroke routing between multiple persons.
 */
class FocusRouter {
public:
    FocusRouter();
    ~FocusRouter();

    /**
     * @brief Resolves the top-level or child window handle at screen coordinates (x, y).
     */
    HWND FindTargetWindowAt(int x, int y);

    /**
     * @brief Processes mouse movement deltas for a person, updating their virtual cursor position.
     */
    void OnPersonMouseMove(PersonState& person, int dx, int dy, AppConfig& config);

    /**
     * @brief Processes mouse button clicks, updating focus states and active person status.
     */
    void OnPersonMouseButton(PersonState& person, DWORD button_flags, AppConfig& config);

    /**
     * @brief Routes keyboard input for a person, applying focus isolation rules.
     * @return True if keypress was intercepted/handled, false if allowed to pass through normally.
     */
    bool OnPersonKeyboardInput(PersonState& person, USHORT vkey, USHORT flags, AppConfig& config);

    int GetActivePersonId() const { return m_active_person_id; }
    void SetActivePersonId(int id) { m_active_person_id = id; }

private:
    int m_active_person_id = 1;

    /**
     * @brief Shifts OS hardware mouse position and window focus to the active person.
     */
    void ActivatePersonFocus(PersonState& person);

    /**
     * @brief Directly posts WM_KEYDOWN, WM_KEYUP, and WM_CHAR messages to a target window handle.
     */
    void RouteKeyToWindow(HWND hwnd, USHORT vkey, USHORT flags);
};

#endif // CONTROLMUX_FOCUS_ROUTER_HPP
