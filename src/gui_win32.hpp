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
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_TOGGLE_ENABLE 1001
#define ID_TRAY_MODE_SWITCHED 1002
#define ID_TRAY_MODE_DIRECT   1003
#define ID_TRAY_PAIR_WIZARD   1004
#define ID_TRAY_EXIT          1005

/**
 * @brief Manages the Win32 system tray notification icon, context menus, and configuration popups.
 */
class GuiWin32 {
public:
    GuiWin32(DeviceManager& dev_mgr, FocusRouter& router, AppConfig& config);
    ~GuiWin32();

    /**
     * @brief Adds the notification icon to the Windows taskbar system tray.
     */
    bool Initialize(HINSTANCE hInstance, HWND hwndMain);

    /**
     * @brief Displays the right-click system tray context menu.
     */
    void ShowContextMenu(HWND hwnd);

    /**
     * @brief Opens the device pairing overview & wizard.
     */
    void OpenPairingWindow(HINSTANCE hInstance);

    /**
     * @brief Updates the system tray tooltip with current active mode status.
     */
    void UpdateTrayTooltip();

    /**
     * @brief Removes system tray icon on shutdown.
     */
    void Shutdown();

private:
    DeviceManager& m_dev_mgr;
    FocusRouter& m_router;
    AppConfig& m_config;

    HWND m_hwndMain = NULL;
    NOTIFYICONDATAW m_nid = { 0 };

    static INT_PTR CALLBACK PairingDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static GuiWin32* s_gui_instance;
};

#endif // CONTROLMUX_GUI_WIN32_HPP
