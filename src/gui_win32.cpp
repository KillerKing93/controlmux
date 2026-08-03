/**
 * @file gui_win32.cpp
 * @brief Implementation of system tray icon, context menu events, and pairing UI.
 */

#include "gui_win32.hpp"
#include <thread>
#include <chrono>

GuiWin32* GuiWin32::s_gui_instance = nullptr;

GuiWin32::GuiWin32(DeviceManager& dev_mgr, FocusRouter& router, AppConfig& config)
    : m_dev_mgr(dev_mgr), m_router(router), m_config(config) {
    s_gui_instance = this;
}

GuiWin32::~GuiWin32() {
    Shutdown();
}

/**
 * Registers system tray icon. Schedules async retry via WM_APP+101
 * if Explorer tray host is not yet ready (returns E_FAIL / error 0x80004005).
 * This never blocks the calling thread.
 */
bool GuiWin32::Initialize(HINSTANCE hInstance, HWND hwndMain) {
    m_hwndMain  = hwndMain;
    m_hInstance = hInstance;

    // Load icon: try embedded resource, then file, then system fallback
    m_hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101));
    if (!m_hIcon) {
        m_hIcon = (HICON)LoadImageW(NULL, L"assets/app_icon.ico",
                                    IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
    }
    if (!m_hIcon) {
        m_hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }

    BuildNid();
    TryAddTrayIcon();   // First attempt; schedules async retry if Shell not ready
    return true;
}

void GuiWin32::BuildNid() {
    ZeroMemory(&m_nid, sizeof(m_nid));
    m_nid.cbSize           = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd             = m_hwndMain;
    m_nid.uID              = 1;
    m_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon            = m_hIcon;
    wcscpy_s(m_nid.szTip, L"ControlMux \u2014 Multi-Person Input Engine");
}

void GuiWin32::TryAddTrayIcon() {
    if (Shell_NotifyIconW(NIM_ADD, &m_nid)) {
        m_tray_added = true;
        return;
    }
    // Explorer shell not ready yet \u2014 schedule another attempt in 500 ms via
    // a WM_APP+101 message pumped through the main window message loop.
    if (m_hwndMain) {
        SetTimer(m_hwndMain, TIMER_TRAY_RETRY, 500, NULL);
    }
}

/**
 * Called from MainWndProc when WM_TIMER fires with TIMER_TRAY_RETRY ID.
 */
void GuiWin32::OnTrayRetryTimer() {
    KillTimer(m_hwndMain, TIMER_TRAY_RETRY);
    if (m_tray_added) return;  // already succeeded on a previous attempt
    TryAddTrayIcon();
}

/**
 * Dynamically updates tray tooltip with active status and mode.
 */
void GuiWin32::UpdateTrayTooltip() {
    if (!m_tray_added) return;
    std::wstring status = L"ControlMux \u2014 ";
    status += m_config.enabled ? L"Active (" : L"Paused (";
    status += (m_config.mode == RoutingMode::SwitchedFocus
               ? L"Switched Focus)" : L"Direct Target)");
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

    AppendMenuW(hMenu, MF_STRING | (m_config.enabled ? MF_CHECKED : MF_UNCHECKED),
                ID_TRAY_TOGGLE_ENABLE,
                m_config.enabled ? L"\u2714 ControlMux Enabled" : L"ControlMux Paused");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

    AppendMenuW(hMenu,
                MF_STRING | (m_config.mode == RoutingMode::SwitchedFocus ? MF_CHECKED : MF_UNCHECKED),
                ID_TRAY_MODE_SWITCHED, L"Mode: Switched Focus");
    AppendMenuW(hMenu,
                MF_STRING | (m_config.mode == RoutingMode::DirectTarget ? MF_CHECKED : MF_UNCHECKED),
                ID_TRAY_MODE_DIRECT, L"Mode: Direct Window Target");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_PAIR_WIZARD, L"Control Center...");
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
        Shutdown();
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
    msg += L"Status: ";
    msg += (m_config.enabled ? L"ACTIVE \u2714" : L"PAUSED \u23f8");
    msg += L"\nRouting Mode: ";
    msg += (m_config.mode == RoutingMode::SwitchedFocus
            ? L"Switched Focus" : L"Direct Target");
    msg += L"\n\nConfigured Profiles:\n";
    for (const auto& p : m_config.persons) {
        msg += L"  \u2022 " + p.name + L":\n";
        msg += L"      Mouse:    " + (p.mouse_hwid.empty()
               ? L"(Auto-assigned)" : p.mouse_hwid) + L"\n";
        msg += L"      Keyboard: " + (p.keyboard_hwid.empty()
               ? L"(Auto-assigned)" : p.keyboard_hwid) + L"\n";
    }
    msg += L"\nControlMux is active in your system tray (bottom-right corner).\n";
    msg += L"Right-click the tray icon to switch modes, toggle, or exit.";

    MessageBoxW(NULL, msg.c_str(), L"ControlMux Control Center",
                MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
}

void GuiWin32::Shutdown() {
    if (m_tray_added && m_nid.hWnd) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_tray_added = false;
        m_nid.hWnd   = NULL;
    }
}
