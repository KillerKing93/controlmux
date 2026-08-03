/**
 * @file gui_win32.cpp
 * @brief Implementation of system tray icon, context menu events, and pairing UI.
 */

#include "gui_win32.hpp"
#include <iostream>

GuiWin32* GuiWin32::s_gui_instance = nullptr;

GuiWin32::GuiWin32(DeviceManager& dev_mgr, FocusRouter& router, AppConfig& config)
    : m_dev_mgr(dev_mgr), m_router(router), m_config(config) {
    s_gui_instance = this;
}

GuiWin32::~GuiWin32() {
    Shutdown();
}

/**
 * Registers system tray icon via Shell_NotifyIconW.
 */
bool GuiWin32::Initialize(HINSTANCE hInstance, HWND hwndMain) {
    m_hwndMain = hwndMain;

    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = hwndMain;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy_s(m_nid.szTip, L"ControlMux - Multi-Person Control");

    Shell_NotifyIconW(NIM_ADD, &m_nid);
    return true;
}

/**
 * Dynamically updates tray tooltip with active status and mode.
 */
void GuiWin32::UpdateTrayTooltip() {
    std::wstring status = L"ControlMux - ";
    status += m_config.enabled ? L"Active (" : L"Paused (";
    status += (m_config.mode == RoutingMode::SwitchedFocus ? L"Switched Focus)" : L"Direct Target)");
    wcscpy_s(m_nid.szTip, status.c_str());
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
}

/**
 * Displays popup menu on right-click on tray icon.
 */
void GuiWin32::ShowContextMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    
    AppendMenuW(hMenu, MF_STRING | (m_config.enabled ? MF_CHECKED : MF_UNCHECKED), ID_TRAY_TOGGLE_ENABLE, 
                m_config.enabled ? L"✔ ControlMux Enabled" : L"ControlMux Paused");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    
    AppendMenuW(hMenu, MF_STRING | (m_config.mode == RoutingMode::SwitchedFocus ? MF_CHECKED : MF_UNCHECKED), 
                ID_TRAY_MODE_SWITCHED, L"Mode: Switched Focus");
    AppendMenuW(hMenu, MF_STRING | (m_config.mode == RoutingMode::DirectTarget ? MF_CHECKED : MF_UNCHECKED), 
                ID_TRAY_MODE_DIRECT, L"Mode: Direct Window Target");
    
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_PAIR_WIZARD, L"Pair Devices Wizard...");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");

    SetForegroundWindow(hwnd);
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);

    switch (cmd) {
    case ID_TRAY_TOGGLE_ENABLE:
        m_config.enabled = !m_config.enabled;
        UpdateTrayTooltip();
        ConfigManager::Save(L"controlmux_config.ini", m_config);
        break;
    case ID_TRAY_MODE_SWITCHED:
        m_config.mode = RoutingMode::SwitchedFocus;
        UpdateTrayTooltip();
        ConfigManager::Save(L"controlmux_config.ini", m_config);
        break;
    case ID_TRAY_MODE_DIRECT:
        m_config.mode = RoutingMode::DirectTarget;
        UpdateTrayTooltip();
        ConfigManager::Save(L"controlmux_config.ini", m_config);
        break;
    case ID_TRAY_PAIR_WIZARD:
        OpenPairingWindow(GetModuleHandle(NULL));
        break;
    case ID_TRAY_EXIT:
        PostQuitMessage(0);
        break;
    }
}

/**
 * Displays device pairing information dialog.
 */
void GuiWin32::OpenPairingWindow(HINSTANCE hInstance) {
    std::wstring msg = L"ControlMux Device Profiles:\n\n";
    for (const auto& p : m_config.persons) {
        msg += p.name + L":\n";
        msg += L"  Mouse: " + (p.mouse_hwid.empty() ? L"(Unassigned - Auto)" : p.mouse_hwid) + L"\n";
        msg += L"  Keyboard: " + (p.keyboard_hwid.empty() ? L"(Unassigned - Auto)" : p.keyboard_hwid) + L"\n\n";
    }
    msg += L"To pair Person 1 Mouse: Click OK, then click Mouse 1.\nTo clear pairings, edit controlmux_config.ini.";

    MessageBoxW(m_hwndMain, msg.c_str(), L"ControlMux Device Profiles", MB_OK | MB_ICONINFORMATION);
}

void GuiWin32::Shutdown() {
    Shell_NotifyIconW(NIM_DELETE, &m_nid);
}
