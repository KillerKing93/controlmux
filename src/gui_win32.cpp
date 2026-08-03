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
 * Retries up to 10 times with 500ms delay to handle cases where
 * the Windows Shell/Explorer tray host is not yet ready (E_FAIL / error 2147500037).
 */
bool GuiWin32::Initialize(HINSTANCE hInstance, HWND hwndMain) {
    m_hwndMain = hwndMain;

    // Load icon: try embedded resource first, then file, then system default
    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101));
    if (!hIcon) {
        hIcon = (HICON)LoadImageW(NULL, L"assets/app_icon.ico", IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
    }
    if (!hIcon) {
        hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }

    ZeroMemory(&m_nid, sizeof(m_nid));
    m_nid.cbSize           = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd             = hwndMain;
    m_nid.uID              = 1;
    m_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon            = hIcon;
    wcscpy_s(m_nid.szTip, L"ControlMux - Multi-Person Input Engine");

    // Retry loop: Explorer shell tray host may not be ready immediately on startup
    const int MAX_RETRIES = 20;
    const DWORD RETRY_DELAY_MS = 250;
    bool added = false;
    for (int i = 0; i < MAX_RETRIES; ++i) {
        if (Shell_NotifyIconW(NIM_ADD, &m_nid)) {
            added = true;
            break;
        }
        Sleep(RETRY_DELAY_MS);
    }

    if (!added) {
        // Non-fatal: app still works without tray icon
        return false;
    }

    // Send balloon notification separately AFTER successful NIM_ADD
    NOTIFYICONDATAW nidBalloon = m_nid;
    nidBalloon.uFlags     = NIF_INFO;
    nidBalloon.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;
    wcscpy_s(nidBalloon.szInfoTitle, L"ControlMux Active");
    wcscpy_s(nidBalloon.szInfo, L"Multi-person input engine running. Right-click tray icon to configure.");
    Shell_NotifyIconW(NIM_MODIFY, &nidBalloon);

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
    m_nid.uFlags = NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
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
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_PAIR_WIZARD, L"Pair Devices / Control Center...");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit ControlMux");

    SetForegroundWindow(hwnd);
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
                             pt.x, pt.y, 0, hwnd, NULL);
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
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        PostQuitMessage(0);
        break;
    }
}

/**
 * Displays Control Center status dialog.
 */
void GuiWin32::OpenPairingWindow(HINSTANCE /*hInstance*/) {
    std::wstring msg = L"ControlMux Multi-Person Input Engine v1.0.0\n";
    msg += L"Created by Alif Nurhidayat (alifnurhidayatwork@gmail.com)\n\n";
    msg += L"Status: " + std::wstring(m_config.enabled ? L"ACTIVE ✔" : L"PAUSED ⏸") + L"\n";
    msg += L"Routing Mode: " + std::wstring(m_config.mode == RoutingMode::SwitchedFocus
                                             ? L"Switched Focus" : L"Direct Target") + L"\n\n";
    msg += L"Configured Profiles:\n";
    for (const auto& p : m_config.persons) {
        msg += L"  • " + p.name + L":\n";
        msg += L"      Mouse:    " + (p.mouse_hwid.empty()    ? L"(Auto-assigned)" : p.mouse_hwid)    + L"\n";
        msg += L"      Keyboard: " + (p.keyboard_hwid.empty() ? L"(Auto-assigned)" : p.keyboard_hwid) + L"\n";
    }
    msg += L"\nControlMux is running in your System Tray (bottom-right corner).\n";
    msg += L"Right-click the tray icon to switch modes, toggle, or exit.";

    MessageBoxW(NULL, msg.c_str(), L"ControlMux Control Center",
                MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
}

void GuiWin32::Shutdown() {
    if (m_nid.hWnd) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_nid.hWnd = NULL;
    }
}
