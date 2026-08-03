/**
 * @file main.cpp — with startup diagnostic logging to cmux_startup.log
 */

#include <windows.h>
#include <fstream>
#include "config.hpp"
#include "device_manager.hpp"
#include "overlay_renderer.hpp"
#include "focus_router.hpp"
#include "input_engine.hpp"
#include "gui_win32.hpp"

AppConfig        g_config;
DeviceManager*   g_dev_mgr     = nullptr;
FocusRouter*     g_router      = nullptr;
OverlayRenderer* g_renderer    = nullptr;
InputEngine*     g_input_engine = nullptr;
GuiWin32*        g_gui         = nullptr;

static std::wofstream g_log;
#define LOG(x) do{if(g_log.is_open()){g_log<<x<<L"\n";g_log.flush();}}while(0)

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INPUT:
        if (g_input_engine) g_input_engine->ProcessRawInput(wParam, lParam);
        return 0;
    case WM_TRAYICON:
        if (g_gui) g_gui->OpenControlPanel();
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    // Enable Per-Monitor DPI Awareness V2 so coordinates across different monitor dimensions/DPI match physical pixels 1:1
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        typedef BOOL (WINAPI *PFN_SetProcessDpiAwarenessContext)(HANDLE);
        auto setDpiAware = (PFN_SetProcessDpiAwarenessContext)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (setDpiAware) {
            setDpiAware((HANDLE)-4); // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
        } else {
            SetProcessDPIAware();
        }
    }

    g_log.open(L"cmux_startup.log", std::ios::trunc);
    LOG(L"=== ControlMux startup ===");

    // 1. Config
    LOG(L"[1] Loading config...");
    AppConfig fileConfig;
    fileConfig.persons.clear();
    if (ConfigManager::Load(L"controlmux_config.ini", fileConfig) && !fileConfig.persons.empty())
        g_config = fileConfig;
    LOG(L"[1] Config loaded. persons=" << g_config.persons.size());

    // 2. Subsystems
    LOG(L"[2a] new DeviceManager...");
    g_dev_mgr  = new DeviceManager();
    LOG(L"[2b] new FocusRouter...");
    g_router   = new FocusRouter();
    LOG(L"[2c] new OverlayRenderer...");
    g_renderer = new OverlayRenderer();
    LOG(L"[2d] SyncWithConfig...");
    g_dev_mgr->SyncWithConfig(g_config);
    LOG(L"[2] Subsystems ready.");

    // 3. GDI+ overlay
    LOG(L"[3] Initializing GDI+ overlay...");
    if (!g_renderer->Initialize(hInstance)) {
        LOG(L"[3] FAILED");
        MessageBoxW(NULL, L"Failed to init overlay.", L"ControlMux", MB_ICONERROR | MB_TOPMOST);
        return 1;
    }
    LOG(L"[3] Overlay OK.");

    // 4. Main message window
    LOG(L"[4] Registering window class...");
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = MainWndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"ControlMuxMainWndClass";
    ATOM a = RegisterClassExW(&wc);
    LOG(L"[4] RegisterClassExW: atom=" << a << L" err=" << GetLastError());

    HWND hwndMain = CreateWindowExW(0, L"ControlMuxMainWndClass", L"ControlMux",
        WS_POPUP, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    LOG(L"[4] CreateWindowExW: hwnd=" << (ULONG_PTR)hwndMain << L" err=" << GetLastError());

    if (!hwndMain) {
        LOG(L"[4] FATAL: no main window");
        return 1;
    }

    // 5. Raw Input
    LOG(L"[5] Initializing Raw Input...");
    g_input_engine = new InputEngine(*g_dev_mgr, *g_router, *g_renderer, g_config);
    if (!g_input_engine->Initialize(hwndMain)) {
        LOG(L"[5] FAILED - RegisterRawInputDevices err=" << GetLastError());
        // Non-fatal for now — continue so GUI still shows
    } else {
        LOG(L"[5] Raw Input OK.");
    }

    // 6. GUI
    LOG(L"[6] Initializing GUI...");
    g_gui = new GuiWin32(*g_dev_mgr, *g_router, g_config);
    g_gui->Initialize(hInstance, hwndMain);
    LOG(L"[6] GUI init done.");

    // 7. Overlay render
    LOG(L"[7] First render...");
    g_renderer->Render(g_dev_mgr->GetPersons(), g_router->GetActivePersonId());
    LOG(L"[7] Render done.");

    // 8. Message loop
    LOG(L"[8] Entering message loop...");
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    LOG(L"[8] Message loop exited.");

    ConfigManager::Save(L"controlmux_config.ini", g_config);
    delete g_gui;
    delete g_input_engine;
    delete g_renderer;
    delete g_router;
    delete g_dev_mgr;
    LOG(L"=== Shutdown complete ===");
    return 0;
}
