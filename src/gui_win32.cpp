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
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101));
    if (!m_nid.hIcon) {
        m_nid.hIcon = (HICON)LoadImageW(NULL, L"assets/app_icon.ico", IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
    }
    if (!m_nid.hIcon) {
        m_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }
    wcscpy_s(m_nid.szTip, L"ControlMux - Multi-Person Input Engine");
    wcscpy_s(m_nid.szInfoTitle, L"ControlMux Active 🖱️⌨️");
    wcscpy_s(m_nid.szInfo, L"ControlMux is running in your System Tray (bottom-right area). Right-click icon to configure device pairings.");
    m_nid.dwInfoFlags = NIIF_INFO;

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
    std::wstring msg = L"ControlMux Multi-Person Input Engine v1.0.0\n";
    msg += L"Created by Alif Nurhidayat (alifnurhidayatwork@gmail.com)\n\n";
    msg += L"Status: " + std::wstring(m_config.enabled ? L"ACTIVE ✔" : L"PAUSED ⏸") + L"\n";
    msg += L"Routing Mode: " + std::wstring(m_config.mode == RoutingMode::SwitchedFocus ? L"Switched Focus" : L"Direct Target") + L"\n\n";
    msg += L"Configured Profiles:\n";
    for (const auto& p : m_config.persons) {
        msg += L"  • " + p.name + L":\n";
        msg += L"      Mouse: " + (p.mouse_hwid.empty() ? L"(Auto / Unassigned)" : p.mouse_hwid) + L"\n";
        msg += L"      Keyboard: " + (p.keyboard_hwid.empty() ? L"(Auto / Unassigned)" : p.keyboard_hwid) + L"\n";
    }
    msg += L"\nControlMux is running in your System Tray (bottom-right area).\nRight-click tray icon to switch modes or toggle active status.";

    MessageBoxW(NULL, msg.c_str(), L"ControlMux Control Center", MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
}

void GuiWin32::Shutdown() {
    Shell_NotifyIconW(NIM_DELETE, &m_nid);
}
