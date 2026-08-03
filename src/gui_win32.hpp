/**
 * @file gui_win32.hpp
 * @brief ControlMux floating Control Center — supports 1 to 16 persons.
 */

#ifndef CONTROLMUX_GUI_WIN32_HPP
#define CONTROLMUX_GUI_WIN32_HPP

#include <windows.h>
#include <shellapi.h>
#include "device_manager.hpp"
#include "focus_router.hpp"
#include "config.hpp"

#define WM_TRAYICON           (WM_USER + 1)
#define ID_TRAY_TOGGLE_ENABLE  1001
#define ID_TRAY_MODE_SWITCHED  1002
#define ID_TRAY_MODE_DIRECT    1003
#define ID_TRAY_PAIR_WIZARD    1004
#define ID_TRAY_EXIT           1005



class GuiWin32 {
public:
    GuiWin32(DeviceManager& dev_mgr, FocusRouter& router, AppConfig& config);
    ~GuiWin32();

    /** @brief Initializes GUI and opens Control Center panel. */
    bool Initialize(HINSTANCE hInstance, HWND hwndMsg);

    /** @brief Opens / focuses the floating Control Center window. */
    void OpenControlPanel();

    /** @brief Opens panel (tray callback / compatibility alias). */
    void ShowContextMenu(HWND hwnd);

    /** @brief Opens panel (compatibility alias). */
    void OpenPairingWindow(HINSTANCE hInstance);

    /** @brief Updates tray tooltip text (no-op if tray unavailable). */
    void UpdateTrayTooltip();

    /** @brief Removes tray icon on shutdown. */
    void Shutdown();

    static GuiWin32* s_gui_instance;

    DeviceManager& m_dev_mgr;
    FocusRouter&   m_router;
    AppConfig&     m_config;

    int m_scroll_offset = 0;  ///< First visible person index in the list

private:
    static LRESULT CALLBACK PanelWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    HWND            m_hwndMsg    = NULL;
    HWND            m_hwndPanel  = NULL;
    HINSTANCE       m_hInstance  = NULL;
    NOTIFYICONDATAW m_nid        = {};
    bool            m_tray_added = false;
};

#endif // CONTROLMUX_GUI_WIN32_HPP
