#ifndef CONTROLMUX_FOCUS_ROUTER_HPP
#define CONTROLMUX_FOCUS_ROUTER_HPP

#include <windows.h>
#include "device_manager.hpp"
#include "config.hpp"

class FocusRouter {
public:
    FocusRouter();
    ~FocusRouter();

    HWND FindTargetWindowAt(int x, int y);

    void OnPersonMouseMove(PersonState& person, int dx, int dy, AppConfig& config);
    void OnPersonMouseButton(PersonState& person, DWORD button_flags, AppConfig& config);
    bool OnPersonKeyboardInput(PersonState& person, USHORT vkey, USHORT flags, AppConfig& config);

    int GetActivePersonId() const { return m_active_person_id; }
    void SetActivePersonId(int id) { m_active_person_id = id; }

private:
    int m_active_person_id = 1;
    void ActivatePersonFocus(PersonState& person);
    void RouteKeyToWindow(HWND hwnd, USHORT vkey, USHORT flags);
};

#endif // CONTROLMUX_FOCUS_ROUTER_HPP
