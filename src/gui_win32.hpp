#ifndef CONTROLMUX_GUI_WIN32_HPP
#define CONTROLMUX_GUI_WIN32_HPP

#include <windows.h>
#include <shellapi.h>
#include "device_manager.hpp"
#include "focus_router.hpp"
#include "config.hpp"

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_TOGGLE_ENABLE 1001
#define ID_TRAY_MODE_SWITCHED 1002
#define ID_TRAY_MODE_DIRECT   1003
#define ID_TRAY_PAIR_WIZARD   1004
#define ID_TRAY_EXIT          1005

class GuiWin32 {
public:
    GuiWin32(DeviceManager& dev_mgr, FocusRouter& router, AppConfig& config);
    ~GuiWin32();

    bool Initialize(HINSTANCE hInstance, HWND hwndMain);
    void ShowContextMenu(HWND hwnd);
    void OpenPairingWindow(HINSTANCE hInstance);
    void UpdateTrayTooltip();

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
