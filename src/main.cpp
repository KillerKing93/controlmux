/**
 * @file main.cpp
 * @brief Application entry point for ControlMux (Multi-Person Input Control Utility).
 * 
 * Initializes settings, raw input engine, GDI+ overlay renderer, focus router,
 * system tray icon, and runs the main Win32 message pump loop.
 */

#include <windows.h>
#include <iostream>
#include "config.hpp"
#include "device_manager.hpp"
#include "overlay_renderer.hpp"
#include "focus_router.hpp"
#include "input_engine.hpp"
#include "gui_win32.hpp"

// Global application subsystem pointers
AppConfig g_config;
DeviceManager* g_dev_mgr = nullptr;
FocusRouter* g_router = nullptr;
OverlayRenderer* g_renderer = nullptr;
InputEngine* g_input_engine = nullptr;
GuiWin32* g_gui = nullptr;

/**
 * @brief Main window procedure for receiving WM_INPUT and WM_TRAYICON messages.
 */
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INPUT:
        if (g_input_engine) {
            g_input_engine->ProcessRawInput(wParam, lParam);
        }
        return 0;

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            if (g_gui) g_gui->ShowContextMenu(hwnd);
        } else if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK) {
            if (g_gui) g_gui->OpenPairingWindow(GetModuleHandle(NULL));
        }
        return 0;

    case WM_TIMER:
        if (wParam == TIMER_TRAY_RETRY && g_gui) {
            g_gui->OnTrayRetryTimer();
        }
        return 0;

    case WM_APP + 100:
        // Deferred startup: show Control Center dialog after message loop is running
        if (g_gui) g_gui->OpenPairingWindow(GetModuleHandle(NULL));
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

/**
 * @brief Win32 Application entry point (Unicode wWinMain).
 */
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {
    // 1. Load application configuration
    // If a config file exists, reset defaults so we don't double-append persons
    AppConfig fileConfig;
    fileConfig.persons.clear();
    if (ConfigManager::Load(L"controlmux_config.ini", fileConfig) && !fileConfig.persons.empty()) {
        g_config = fileConfig;
    }

    // 2. Instantiate core subsystems
    g_dev_mgr = new DeviceManager();
    g_router = new FocusRouter();
    g_renderer = new OverlayRenderer();

    g_dev_mgr->SyncWithConfig(g_config);

    // 3. Initialize GDI+ overlay renderer window
    if (!g_renderer->Initialize(hInstance)) {
        MessageBoxW(NULL, L"Failed to initialize GDI+ Overlay Window.", L"ControlMux Error", MB_ICONERROR);
        return 1;
    }

    // 4. Register hidden window for Raw Input and system tray message handling
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ControlMuxMainWndClass";

    RegisterClassExW(&wc);

    HWND hwndMain = CreateWindowExW(
        0, L"ControlMuxMainWndClass", L"ControlMux Controller",
        WS_POPUP, 0, 0, 0, 0, NULL, NULL, hInstance, NULL
    );

    if (!hwndMain) {
        MessageBoxW(NULL, L"Failed to create main control window.", L"ControlMux Error", MB_ICONERROR);
        return 1;
    }

    // 5. Initialize Raw Input engine & System Tray GUI
    g_input_engine = new InputEngine(*g_dev_mgr, *g_router, *g_renderer, g_config);
    if (!g_input_engine->Initialize(hwndMain)) {
        MessageBoxW(NULL, L"Failed to register Raw Input devices.", L"ControlMux Error", MB_ICONERROR);
        return 1;
    }

    g_gui = new GuiWin32(*g_dev_mgr, *g_router, g_config);
    g_gui->Initialize(hInstance, hwndMain);

    // Initial overlay render pass
    g_renderer->Render(g_dev_mgr->GetPersons(), g_router->GetActivePersonId());

    // Post a deferred message to show Control Center dialog AFTER message loop starts
    // This prevents blocking startup before the message pump is running
    PostMessage(hwndMain, WM_APP + 100, 0, 0);

    // 6. Execute Win32 event message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 7. Save settings and clean up memory on exit
    ConfigManager::Save(L"controlmux_config.ini", g_config);

    delete g_gui;
    delete g_input_engine;
    delete g_renderer;
    delete g_router;
    delete g_dev_mgr;

    return 0;
}
