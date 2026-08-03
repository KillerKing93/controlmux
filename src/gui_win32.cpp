/**
 * @file gui_win32.cpp
 * @brief Implementation of ControlMux GUI: floating control panel window
 *        (system tray is skipped if Shell_TrayWnd is not available).
 */

#include "gui_win32.hpp"
#include <string>
#include <sstream>

GuiWin32* GuiWin32::s_gui_instance = nullptr;

// ── colours ────────────────────────────────────────────────────────────────
static const COLORREF CLR_BG       = RGB(15,  17,  26);   // near-black navy
static const COLORREF CLR_ACCENT   = RGB(0,  200, 255);   // cyan
static const COLORREF CLR_TEXT     = RGB(220, 225, 240);  // light grey
static const COLORREF CLR_GREEN    = RGB(0,  220, 120);
static const COLORREF CLR_RED      = RGB(255,  60,  80);
static const COLORREF CLR_BTN      = RGB(30,  34,  50);
static const COLORREF CLR_BTN_HOV  = RGB(0,  170, 210);
static const int      PANEL_W      = 380;
static const int      PANEL_H      = 280;

// ── button IDs ─────────────────────────────────────────────────────────────
#define BTN_TOGGLE   100
#define BTN_MODE     101
#define BTN_EXIT     102

// ── helpers ────────────────────────────────────────────────────────────────
static HFONT MakeFont(int sz, bool bold = false) {
    return CreateFontW(sz, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
                       FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS,
                       L"Segoe UI");
}

// ── window procedure ───────────────────────────────────────────────────────
LRESULT CALLBACK GuiWin32::PanelWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    GuiWin32* self = reinterpret_cast<GuiWin32*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        self = reinterpret_cast<GuiWin32*>(cs->lpCreateParams);

        // Buttons
        HFONT fBtn = MakeFont(13, true);
        HWND hToggle = CreateWindowExW(0, L"BUTTON", L"Toggle Enable/Pause",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 160, 160, 34, hwnd, (HMENU)BTN_TOGGLE, cs->hInstance, NULL);
        SendMessageW(hToggle, WM_SETFONT, (WPARAM)fBtn, TRUE);

        HWND hMode = CreateWindowExW(0, L"BUTTON", L"Switch Mode",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            195, 160, 160, 34, hwnd, (HMENU)BTN_MODE, cs->hInstance, NULL);
        SendMessageW(hMode, WM_SETFONT, (WPARAM)fBtn, TRUE);

        HWND hExit = CreateWindowExW(0, L"BUTTON", L"Exit ControlMux",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 210, 335, 34, hwnd, (HMENU)BTN_EXIT, cs->hInstance, NULL);
        SendMessageW(hExit, WM_SETFONT, (WPARAM)fBtn, TRUE);

        return 0;
    }

    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wp;
        RECT rc; GetClientRect(hwnd, &rc);
        HBRUSH br = CreateSolidBrush(CLR_BG);
        FillRect(hdc, &rc, br);
        DeleteObject(br);
        return 1;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        SetBkMode(hdc, TRANSPARENT);

        RECT rc; GetClientRect(hwnd, &rc);

        // Background
        HBRUSH brBg = CreateSolidBrush(CLR_BG);
        FillRect(hdc, &rc, brBg);
        DeleteObject(brBg);

        // Accent top bar
        RECT topBar = {0, 0, rc.right, 4};
        HBRUSH brAccent = CreateSolidBrush(CLR_ACCENT);
        FillRect(hdc, &topBar, brAccent);
        DeleteObject(brAccent);

        // Title
        HFONT fTitle = MakeFont(20, true);
        HFONT fOld   = (HFONT)SelectObject(hdc, fTitle);
        SetTextColor(hdc, CLR_ACCENT);
        RECT rcTitle = {20, 14, rc.right - 20, 50};
        DrawTextW(hdc, L"ControlMux", -1, &rcTitle, DT_LEFT | DT_SINGLELINE);

        // Sub-title
        HFONT fSub = MakeFont(11);
        SelectObject(hdc, fSub);
        SetTextColor(hdc, CLR_TEXT);
        RECT rcSub = {20, 42, rc.right - 20, 66};
        DrawTextW(hdc, L"Multi-Person Input Engine  v1.0.0", -1, &rcSub, DT_LEFT | DT_SINGLELINE);

        // Separator
        HPEN pen = CreatePen(PS_SOLID, 1, CLR_BTN);
        HPEN penOld = (HPEN)SelectObject(hdc, pen);
        MoveToEx(hdc, 20, 68, NULL);
        LineTo(hdc, rc.right - 20, 68);
        SelectObject(hdc, penOld);
        DeleteObject(pen);

        // Status row
        if (self) {
            HFONT fLabel = MakeFont(12, true);
            SelectObject(hdc, fLabel);

            // Status
            bool en = self->m_config.enabled;
            SetTextColor(hdc, en ? CLR_GREEN : CLR_RED);
            RECT rcSt = {20, 80, 200, 106};
            DrawTextW(hdc, en ? L"\u25CF  ACTIVE" : L"\u25CF  PAUSED", -1, &rcSt, DT_LEFT | DT_SINGLELINE);

            // Mode
            SetTextColor(hdc, CLR_TEXT);
            RECT rcMd = {20, 108, rc.right - 20, 132};
            bool sf = (self->m_config.mode == RoutingMode::SwitchedFocus);
            std::wstring modeStr = std::wstring(L"Mode:  ") + (sf ? L"Switched Focus" : L"Direct Target");
            DrawTextW(hdc, modeStr.c_str(), -1, &rcMd, DT_LEFT | DT_SINGLELINE);

            // Person count
            RECT rcPc = {20, 130, rc.right - 20, 154};
            std::wstring pcStr = L"Profiles:  " + std::to_wstring(self->m_config.persons.size()) + L" person(s) configured";
            DrawTextW(hdc, pcStr.c_str(), -1, &rcPc, DT_LEFT | DT_SINGLELINE);

            DeleteObject(fLabel);
        }

        SelectObject(hdc, fOld);
        DeleteObject(fTitle);
        DeleteObject(fSub);

        // Footer
        HFONT fFt = MakeFont(10);
        SelectObject(hdc, fFt);
        SetTextColor(hdc, RGB(80, 90, 110));
        RECT rcFt = {0, rc.bottom - 22, rc.right, rc.bottom - 4};
        DrawTextW(hdc, L"Alif Nurhidayat \u00a9 2024  |  alifnurhidayatwork@gmail.com",
                  -1, &rcFt, DT_CENTER | DT_SINGLELINE);
        DeleteObject(fFt);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_COMMAND:
        if (!self) break;
        switch (LOWORD(wp)) {
        case BTN_TOGGLE:
            self->m_config.enabled = !self->m_config.enabled;
            ConfigManager::Save(L"controlmux_config.ini", self->m_config);
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case BTN_MODE:
            self->m_config.mode = (self->m_config.mode == RoutingMode::SwitchedFocus)
                                  ? RoutingMode::DirectTarget
                                  : RoutingMode::SwitchedFocus;
            ConfigManager::Save(L"controlmux_config.ini", self->m_config);
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case BTN_EXIT:
            DestroyWindow(hwnd);
            break;
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
    return 0;
}

// ── public interface ───────────────────────────────────────────────────────
GuiWin32::GuiWin32(DeviceManager& dev_mgr, FocusRouter& router, AppConfig& config)
    : m_dev_mgr(dev_mgr), m_router(router), m_config(config) {
    s_gui_instance = this;
}

GuiWin32::~GuiWin32() {
    Shutdown();
}

bool GuiWin32::Initialize(HINSTANCE hInstance, HWND hwndMsg) {
    m_hwndMsg   = hwndMsg;
    m_hInstance = hInstance;

    // ── Try system tray first (only if Explorer taskbar exists) ──
    HWND hTray = FindWindowW(L"Shell_TrayWnd", NULL);
    if (hTray) {
        m_tray_available = true;
        HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101));
        if (!hIcon) hIcon = (HICON)LoadImageW(NULL, L"assets/app_icon.ico",
                                               IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
        if (!hIcon) hIcon = LoadIcon(NULL, IDI_APPLICATION);

        ZeroMemory(&m_nid, sizeof(m_nid));
        m_nid.cbSize           = sizeof(NOTIFYICONDATAW);
        m_nid.hWnd             = hwndMsg;
        m_nid.uID              = 1;
        m_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        m_nid.uCallbackMessage = WM_TRAYICON;
        m_nid.hIcon            = hIcon;
        wcscpy_s(m_nid.szTip, L"ControlMux — Multi-Person Input Engine");

        if (Shell_NotifyIconW(NIM_ADD, &m_nid)) {
            m_tray_added = true;
        }
    }

    // ── Always open control panel window ──
    OpenControlPanel();
    return true;
}

void GuiWin32::OpenControlPanel() {
    if (m_hwndPanel && IsWindow(m_hwndPanel)) {
        SetForegroundWindow(m_hwndPanel);
        return;
    }

    // Register panel class if not done yet
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = PanelWndProc;
    wc.hInstance     = m_hInstance;
    wc.hbrBackground = CreateSolidBrush(CLR_BG);
    wc.lpszClassName = L"ControlMuxPanel";
    wc.hIcon         = LoadIcon(m_hInstance, MAKEINTRESOURCE(101));
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExW(&wc); // OK if already registered

    // Centre on the primary monitor's work area (not SM_CXSCREEN which can be wrong on multi-monitor)
    POINT ptPrimary = {1, 1};  // a point guaranteed on the primary monitor
    HMONITOR hMon = MonitorFromPoint(ptPrimary, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    GetMonitorInfoW(hMon, &mi);
    int wx = mi.rcWork.left + (mi.rcWork.right  - mi.rcWork.left - PANEL_W) / 2;
    int wy = mi.rcWork.top  + (mi.rcWork.bottom - mi.rcWork.top  - PANEL_H) / 2;

    m_hwndPanel = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_APPWINDOW,
        L"ControlMuxPanel",
        L"ControlMux — Control Center",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        wx, wy, PANEL_W, PANEL_H,
        NULL, NULL, m_hInstance, this);  // pass 'this' as lpParam

    if (m_hwndPanel) {
        ShowWindow(m_hwndPanel, SW_SHOW);
        UpdateWindow(m_hwndPanel);
        SetForegroundWindow(m_hwndPanel);
    }
}

void GuiWin32::ShowContextMenu(HWND /*hwnd*/) {
    OpenControlPanel();  // Just open panel on tray click too
}

void GuiWin32::OpenPairingWindow(HINSTANCE /*hInstance*/) {
    OpenControlPanel();
}

void GuiWin32::UpdateTrayTooltip() {
    if (!m_tray_added) return;
    std::wstring s = L"ControlMux — ";
    s += m_config.enabled ? L"Active" : L"Paused";
    wcscpy_s(m_nid.szTip, s.c_str());
    m_nid.uFlags = NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
}

void GuiWin32::Shutdown() {
    if (m_tray_added) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_tray_added = false;
    }
}
