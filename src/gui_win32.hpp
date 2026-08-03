/**
 * @file gui_win32.hpp
 * @brief Native Windows System Tray icon management and device pairing dialog interface.
 */

#ifndef CONTROLMUX_GUI_WIN32_HPP
#define CONTROLMUX_GUI_WIN32_HPP

#include <windows.h>
#include <shellapi.h>
#include "device_manager.hpp"
#include "focus_router.hpp"
#include "config.hpp"

// System Tray Notification Constants
#define WM_TRAYICON          (WM_USER + 1)
#define ID_TRAY_TOGGLE_ENABLE 1001
#define ID_TRAY_MODE_SWITCHED 1002
#define ID_TRAY_MODE_DIRECT   1003
#define ID_TRAY_PAIR_WIZARD   1004
#define ID_TRAY_EXIT          1005
#define TIMER_TRAY_RETRY      2001

/**
 * @brief Manages the Win32 system tray notification icon, context menus, and configuration popups.
 */
class GuiWin32 {
public:
    GuiWin32(DeviceManager& dev_mgr, FocusRouter& router, AppConfig& config);
    ~GuiWin32();

    /** @brief Adds the notification icon to the Windows taskbar system tray (non-blocking). */
    bool Initialize(HINSTANCE hInstance, HWND hwndMain);

    /** @brief Called when TIMER_TRAY_RETRY fires — retries Shell_NotifyIconW. */
    void OnTrayRetryTimer();

    /** @brief Displays the right-click system tray context menu. */
    void ShowContextMenu(HWND hwnd);

    /** @brief Opens the Control Center status dialog. */
    void OpenPairingWindow(HINSTANCE hInstance);

    /** @brief Updates the system tray tooltip with current active mode status. */
    void UpdateTrayTooltip();

    /** @brief Removes system tray icon on shutdown. */
    void Shutdown();

    static GuiWin32* s_gui_instance;

private:
    void BuildNid();
    void TryAddTrayIcon();

    DeviceManager& m_dev_mgr;
    FocusRouter&   m_router;
    AppConfig&     m_config;

    HWND            m_hwndMain  = NULL;
    HINSTANCE       m_hInstance = NULL;
    HICON           m_hIcon     = NULL;
    NOTIFYICONDATAW m_nid       = { 0 };
    bool            m_tray_added = false;
};

#endif // CONTROLMUX_GUI_WIN32_HPP
