#include <windows.h>
#include <iostream>
#include "config.hpp"
#include "device_manager.hpp"
#include "overlay_renderer.hpp"
#include "focus_router.hpp"
#include "input_engine.hpp"
#include "gui_win32.hpp"

// Global instances
AppConfig g_config;
DeviceManager* g_dev_mgr = nullptr;
FocusRouter* g_router = nullptr;
OverlayRenderer* g_renderer = nullptr;
InputEngine* g_input_engine = nullptr;
GuiWin32* g_gui = nullptr;

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INPUT:
        if (g_input_engine) {
            g_input_engine->ProcessRawInput(wParam, lParam);
        }
        return 0;

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) {
            if (g_gui) g_gui->ShowContextMenu(hwnd);
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {
    // 1. Load config
    ConfigManager::Load(L"controlmux_config.ini", g_config);

    // 2. Initialize subsystems
    g_dev_mgr = new DeviceManager();
    g_router = new FocusRouter();
    g_renderer = new OverlayRenderer();

    g_dev_mgr->SyncWithConfig(g_config);

    if (!g_renderer->Initialize(hInstance)) {
        MessageBoxW(NULL, L"Failed to initialize GDI+ Overlay Window.", L"ControlMux Error", MB_ICONERROR);
        return 1;
    }

    // 3. Create hidden main window for Raw Input & Tray messages
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ControlMuxMainWndClass";

    RegisterClassExW(&wc);

    HWND hwndMain = CreateWindowExW(
        0, L"ControlMuxMainWndClass", L"ControlMux Controller",
        0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL
    );

    if (!hwndMain) {
        MessageBoxW(NULL, L"Failed to create main control window.", L"ControlMux Error", MB_ICONERROR);
        return 1;
    }

    // 4. Initialize Input Engine & System Tray GUI
    g_input_engine = new InputEngine(*g_dev_mgr, *g_router, *g_renderer, g_config);
    if (!g_input_engine->Initialize(hwndMain)) {
        MessageBoxW(NULL, L"Failed to register Raw Input devices.", L"ControlMux Error", MB_ICONERROR);
        return 1;
    }

    g_gui = new GuiWin32(*g_dev_mgr, *g_router, g_config);
    g_gui->Initialize(hInstance, hwndMain);

    // Initial overlay render
    g_renderer->Render(g_dev_mgr->GetPersons(), g_router->GetActivePersonId());

    // 5. Main Win32 Message Loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 6. Cleanup & Save config
    ConfigManager::Save(L"controlmux_config.ini", g_config);

    delete g_gui;
    delete g_input_engine;
    delete g_renderer;
    delete g_router;
    delete g_dev_mgr;

    return 0;
}
