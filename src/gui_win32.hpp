/**
 * @file gui_win32.hpp
 * @brief ControlMux floating control panel window + optional system tray icon.
 */

#ifndef CONTROLMUX_GUI_WIN32_HPP
#define CONTROLMUX_GUI_WIN32_HPP

#include <windows.h>
#include <shellapi.h>
#include "device_manager.hpp"
#include "focus_router.hpp"
#include "config.hpp"

#define WM_TRAYICON          (WM_USER + 1)
#define ID_TRAY_TOGGLE_ENABLE 1001
#define ID_TRAY_MODE_SWITCHED 1002
#define ID_TRAY_MODE_DIRECT   1003
#define ID_TRAY_PAIR_WIZARD   1004
#define ID_TRAY_EXIT          1005
#define TIMER_TRAY_RETRY      2001

class GuiWin32 {
public:
    GuiWin32(DeviceManager& dev_mgr, FocusRouter& router, AppConfig& config);
    ~GuiWin32();

    /** @brief Initialises tray (if Explorer is available) and shows control panel. */
    bool Initialize(HINSTANCE hInstance, HWND hwndMsg);

    /** @brief Opens / focuses the floating control panel window. */
    void OpenControlPanel();

    /** @brief Displays right-click tray context menu (or opens panel). */
    void ShowContextMenu(HWND hwnd);

    /** @brief Opens control panel (compatibility alias). */
    void OpenPairingWindow(HINSTANCE hInstance);

    /** @brief Updates tray tooltip text. */
    void UpdateTrayTooltip();

    /** @brief Removes tray icon on shutdown. */
    void Shutdown();

    static GuiWin32* s_gui_instance;

    // Exposed so MainWndProc can forward WM_TRAYICON
    DeviceManager& m_dev_mgr;
    FocusRouter&   m_router;
    AppConfig&     m_config;

private:
    static LRESULT CALLBACK PanelWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    HWND            m_hwndMsg    = NULL;
    HWND            m_hwndPanel  = NULL;
    HINSTANCE       m_hInstance  = NULL;
    NOTIFYICONDATAW m_nid        = {};
    bool            m_tray_added = false;
    bool            m_tray_available = false;
};

#endif // CONTROLMUX_GUI_WIN32_HPP
