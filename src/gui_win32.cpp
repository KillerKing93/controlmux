/**
 * @file gui_win32.cpp
 * @brief ControlMux floating Control Center panel — supports up to 16 persons.
 */

#include "gui_win32.hpp"
#include "overlay_renderer.hpp"
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>

GuiWin32* GuiWin32::s_gui_instance = nullptr;

class OverlayRenderer;
extern OverlayRenderer* g_renderer;

// ── Debug log ──────────────────────────────────────────────────────────────
static std::wofstream g_dbg;
#define DBG(x) do{if(g_dbg.is_open()){g_dbg<<x<<L"\n";g_dbg.flush();}}while(0)

// ── 16-person color palette (ARGB 0xFFRRGGBB) ─────────────────────────────
static const DWORD PERSON_COLORS[16] = {
    0xFFFF2244, // 1  Red
    0xFF1A7FFF, // 2  Blue
    0xFF00CC55, // 3  Green
    0xFFFF9900, // 4  Orange
    0xFFBB33FF, // 5  Purple
    0xFF00DDFF, // 6  Cyan
    0xFFFF55AA, // 7  Pink
    0xFFFFE800, // 8  Yellow
    0xFF00FFB0, // 9  Mint
    0xFFFF6644, // 10 Coral
    0xFF5588FF, // 11 Periwinkle
    0xFF88FF22, // 12 Lime
    0xFFFF3388, // 13 Rose
    0xFF9944FF, // 14 Violet
    0xFF22FFDD, // 15 Turquoise
    0xFFFF44FF, // 16 Magenta
};

// ── Palette: convert DWORD ARGB → COLORREF RGB ────────────────────────────
static inline COLORREF ArgbToColorref(DWORD argb) {
    return RGB((argb >> 16) & 0xFF, (argb >> 8) & 0xFF, argb & 0xFF);
}

// ── Layout constants ───────────────────────────────────────────────────────
static const int PW          = 460;  // panel width
static const int HDR_H       = 170;  // header height (title + status + controls)
static const int ROW_H       = 42;   // height of each person row
static const int LIST_ROWS   = 6;    // visible person rows at once
static const int LIST_H      = ROW_H * LIST_ROWS;
static const int BTN_AREA_H  = 90;   // add/remove/exit buttons area
static const int FOOTER_H    = 24;
static const int PH = HDR_H + LIST_H + BTN_AREA_H + FOOTER_H; // ≈ 540

// Button IDs
#define BTN_TOGGLE      100
#define BTN_MODE        101
#define BTN_EXIT        102
#define BTN_ADD         103
#define BTN_REMOVE      104
#define BTN_SCROLL_UP   105
#define BTN_SCROLL_DOWN 106

// ── Colours ────────────────────────────────────────────────────────────────
static const COLORREF BG       = RGB(12,  14,  22);
static const COLORREF ACCENT   = RGB(0,  200, 255);
static const COLORREF ACCENT2  = RGB(160, 60, 255);
static const COLORREF TXT      = RGB(210, 218, 235);
static const COLORREF TXT_DIM  = RGB(80,  90, 110);
static const COLORREF GREEN    = RGB(0,  220, 120);
static const COLORREF RED_C    = RGB(255, 60,  80);
static const COLORREF DIVIDER  = RGB(30,  35,  52);
static const COLORREF ROW_ALT  = RGB(18,  20,  32);
static const COLORREF ROW_HOV  = RGB(22,  26,  42);

// ── Font helper ───────────────────────────────────────────────────────────
static HFONT MakeFont(int sz, bool bold = false, bool italic = false) {
    return CreateFontW(sz, 0, 0, 0,
                       bold   ? FW_BOLD : FW_NORMAL,
                       italic ? TRUE : FALSE,
                       FALSE, FALSE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS,
                       L"Segoe UI");
}

// ── Draw a filled rounded rectangle (GDI approximation) ───────────────────
static void FillRoundRect(HDC hdc, int x, int y, int w, int h, int r, COLORREF c) {
    HBRUSH br = CreateSolidBrush(c);
    RECT rc = {x, y, x+w, y+h};
    // Use rounded corners via RoundRect
    HBRUSH old = (HBRUSH)SelectObject(hdc, br);
    HPEN pen = CreatePen(PS_NULL, 0, c);
    HPEN oldp = (HPEN)SelectObject(hdc, pen);
    RoundRect(hdc, x, y, x+w, y+h, r, r);
    SelectObject(hdc, old);
    SelectObject(hdc, oldp);
    DeleteObject(br);
    DeleteObject(pen);
}

// ══════════════════════════════════════════════════════════════════════════
// PanelWndProc
// ══════════════════════════════════════════════════════════════════════════
LRESULT CALLBACK GuiWin32::PanelWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    GuiWin32* self = reinterpret_cast<GuiWin32*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {

    // ── Create child controls ───────────────────────────────────────────
    case WM_CREATE: {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        self = reinterpret_cast<GuiWin32*>(cs->lpCreateParams);

        HFONT fBtn  = MakeFont(12, true);
        HFONT fSmall = MakeFont(11, false);
        HINSTANCE hi = cs->hInstance;

        // Row 1: Toggle + Mode
        auto Btn = [&](const wchar_t* txt, int x, int y, int w, int h, int id, HFONT f) {
            HWND b = CreateWindowExW(0, L"BUTTON", txt,
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                x, y, w, h, hwnd, (HMENU)(INT_PTR)id, hi, NULL);
            SendMessageW(b, WM_SETFONT, (WPARAM)f, TRUE);
            return b;
        };

        int y0 = HDR_H - 46;
        Btn(L"⏸ Toggle Enable",   14, y0,      210, 32, BTN_TOGGLE, fBtn);
        Btn(L"⇄ Switch Mode",     232, y0,      214, 32, BTN_MODE,   fBtn);

        // Scroll arrows for person list
        int listY = HDR_H;
        Btn(L"▲", PW - 30, listY,           26, LIST_H/2 - 1, BTN_SCROLL_UP,   fSmall);
        Btn(L"▼", PW - 30, listY + LIST_H/2, 26, LIST_H/2,     BTN_SCROLL_DOWN, fSmall);

        // Row below list: Add / Remove / Exit
        int bY = HDR_H + LIST_H + 10;
        Btn(L"＋ Add Person",      14,  bY,      210, 32, BTN_ADD,    fBtn);
        Btn(L"－ Remove Last",     232, bY,      214, 32, BTN_REMOVE, fBtn);
        Btn(L"✕  Exit ControlMux",14,  bY + 42, 432, 32, BTN_EXIT,  fBtn);

        return 0;
    }

    // ── Suppress background erase (double-buffering handles it) ────────
    case WM_ERASEBKGND:
        return 1;

    // ── Paint (Double-buffered to Memory DC for 60 FPS smooth dragging) ─
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdcWindow = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);

        // Double buffering memory DC
        HDC hdc = CreateCompatibleDC(hdcWindow);
        HBITMAP hbm = CreateCompatibleBitmap(hdcWindow, rc.right, rc.bottom);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdc, hbm);

        SetBkMode(hdc, TRANSPARENT);

        // Background
        HBRUSH brBg = CreateSolidBrush(BG);
        FillRect(hdc, &rc, brBg);
        DeleteObject(brBg);

        // ── Top accent bar (4px) ──────────────────────────────────────────
        RECT topBar = {0, 0, rc.right, 4};
        HBRUSH brAccent = CreateSolidBrush(ACCENT);
        FillRect(hdc, &topBar, brAccent);
        DeleteObject(brAccent);

        // Static cached fonts (created once, reused across frames)
        static HFONT fTitle = MakeFont(22, true);
        static HFONT fSub   = MakeFont(10, false, true);
        static HFONT fLbl   = MakeFont(12, true);
        static HFONT fHdr   = MakeFont(10, true);
        static HFONT fRow   = MakeFont(12, true);
        static HFONT fRowS  = MakeFont(10, true);
        static HFONT fFt    = MakeFont(9, false, true);

        // ── Title ────────────────────────────────────────────────────────
        HFONT fOld = (HFONT)SelectObject(hdc, fTitle);
        SetTextColor(hdc, ACCENT);
        RECT rtit = {16, 10, rc.right, 42};
        DrawTextW(hdc, L"ControlMux", -1, &rtit, DT_LEFT | DT_SINGLELINE);

        SelectObject(hdc, fSub);
        SetTextColor(hdc, TXT_DIM);
        RECT rsub = {16, 38, rc.right, 56};
        DrawTextW(hdc, L"Multi-Person Input Engine  v1.0.0  \u2014  Alif Nurhidayat",
                  -1, &rsub, DT_LEFT | DT_SINGLELINE);

        // ── Status + mode row ─────────────────────────────────────────────
        SelectObject(hdc, fLbl);

        if (self) {
            bool en = self->m_config.enabled;
            bool sf = (self->m_config.mode == RoutingMode::SwitchedFocus);
            int  np = (int)self->m_config.persons.size();

            // Status pill
            COLORREF pillC = en ? RGB(0,180,80) : RGB(160,40,60);
            FillRoundRect(hdc, 16, 62, 120, 22, 8, pillC);
            SetTextColor(hdc, RGB(255,255,255));
            RECT rsp = {16, 63, 136, 84};
            DrawTextW(hdc, en ? L"\u25CF  ACTIVE" : L"\u25A0  PAUSED", -1, &rsp, DT_CENTER | DT_SINGLELINE);

            // Mode pill
            FillRoundRect(hdc, 144, 62, 150, 22, 8, RGB(30,40,70));
            SetTextColor(hdc, ACCENT);
            RECT rmp = {144, 63, 294, 84};
            DrawTextW(hdc, sf ? L"\u21c4 Switched Focus" : L"\u2937 Direct Target",
                      -1, &rmp, DT_CENTER | DT_SINGLELINE);

            // Person count badge
            SetTextColor(hdc, TXT_DIM);
            RECT rpc = {300, 63, rc.right - 8, 84};
            std::wstring ps_str = std::to_wstring(np) + L" / 16";
            DrawTextW(hdc, ps_str.c_str(), -1, &rpc, DT_RIGHT | DT_SINGLELINE);

            // Divider
            HPEN div = CreatePen(PS_SOLID, 1, DIVIDER);
            HPEN divOld = (HPEN)SelectObject(hdc, div);
            MoveToEx(hdc, 14, 94, NULL); LineTo(hdc, rc.right - 14, 94);
            SelectObject(hdc, divOld); DeleteObject(div);

            // ── Pairing Banner or Person list header ─────────────────────
            SelectObject(hdc, fHdr);

            if (self->m_dev_mgr.IsPairing()) {
                // Active Pairing Mode Banner
                FillRoundRect(hdc, 14, 95, rc.right - 28, 20, 4, RGB(220, 120, 0));
                SetTextColor(hdc, RGB(255, 255, 255));
                RECT rpair = {14, 96, rc.right - 28, 115};
                DrawTextW(hdc, L"⚡ PAIRING ACTIVE: Click or move device now... (Click to cancel)",
                          -1, &rpair, DT_CENTER | DT_SINGLELINE);
            } else {
                SetTextColor(hdc, TXT_DIM);
                RECT rhdr = {14, 97, rc.right - 34, 115};
                DrawTextW(hdc, L"  #   PERSON NAME            MOUSE PAIR    KEYBOARD PAIR   RESET",
                          -1, &rhdr, DT_LEFT | DT_SINGLELINE);
            }

            // Divider 2
            MoveToEx(hdc, 14, 117, NULL); LineTo(hdc, rc.right - 34, 117);

            // ── Person rows ───────────────────────────────────────────────
            int listTop = HDR_H;  // == 170
            int visStart = self->m_scroll_offset;
            int visEnd   = visStart + LIST_ROWS;

            // Clip to list viewport
            HRGN clip = CreateRectRgn(0, listTop, rc.right - 28, listTop + LIST_H);
            SelectClipRgn(hdc, clip);

            for (int i = visStart; i < visEnd && i < np; ++i) {
                const auto& p = self->m_config.persons[i];
                int ry = listTop + (i - visStart) * ROW_H;

                // Alternating row bg
                HBRUSH rowBr = CreateSolidBrush((i % 2 == 0) ? BG : ROW_ALT);
                RECT rrow = {0, ry, rc.right - 28, ry + ROW_H};
                FillRect(hdc, &rrow, rowBr);
                DeleteObject(rowBr);

                // Color swatch
                COLORREF swatch = ArgbToColorref(p.color);
                FillRoundRect(hdc, 6, ry + 11, 14, 20, 4, swatch);

                // Person number
                SelectObject(hdc, fRowS);
                SetTextColor(hdc, TXT_DIM);
                RECT rnum = {24, ry + 12, 44, ry + 30};
                DrawTextW(hdc, std::to_wstring(p.id).c_str(), -1, &rnum, DT_RIGHT | DT_SINGLELINE);

                // Name
                SelectObject(hdc, fRow);
                SetTextColor(hdc, TXT);
                RECT rname = {46, ry + 10, 178, ry + 30};
                std::wstring nm = p.name;
                if (nm.size() > 12) nm = nm.substr(0, 11) + L"\u2026";
                DrawTextW(hdc, nm.c_str(), -1, &rname, DT_LEFT | DT_SINGLELINE);

                // 1. Mouse Button: x = 180, w = 75, h = 24
                bool hasM = !p.mouse_hwid.empty();
                COLORREF mc = hasM ? GREEN : RGB(35, 45, 70);
                FillRoundRect(hdc, 180, ry + 9, 75, 24, 6, mc);
                SelectObject(hdc, fRowS);
                SetTextColor(hdc, RGB(255, 255, 255));
                RECT rmouse = {180, ry + 13, 255, ry + 31};
                DrawTextW(hdc, hasM ? L"✔ Mouse" : L"🎯 Mouse", -1, &rmouse, DT_CENTER | DT_SINGLELINE);

                // 2. Keyboard Button: x = 260, w = 75, h = 24
                bool hasK = !p.keyboard_hwid.empty();
                COLORREF kc = hasK ? GREEN : RGB(35, 45, 70);
                FillRoundRect(hdc, 260, ry + 9, 75, 24, 6, kc);
                RECT rkb = {260, ry + 13, 335, ry + 31};
                DrawTextW(hdc, hasK ? L"✔ KB" : L"🎯 KB", -1, &rkb, DT_CENTER | DT_SINGLELINE);

                // 3. Reset Button: x = 340, w = 75, h = 24
                FillRoundRect(hdc, 340, ry + 9, 75, 24, 6, RGB(35, 38, 52));
                SetTextColor(hdc, (hasM || hasK) ? RED_C : TXT_DIM);
                RECT rreset = {340, ry + 13, 415, ry + 31};
                DrawTextW(hdc, L"🔄 Reset", -1, &rreset, DT_CENTER | DT_SINGLELINE);

                // Row bottom divider
                HPEN rowDiv = CreatePen(PS_SOLID, 1, DIVIDER);
                HPEN rdOld  = (HPEN)SelectObject(hdc, rowDiv);
                MoveToEx(hdc, 0, ry + ROW_H - 1, NULL);
                LineTo(hdc,   rc.right - 28, ry + ROW_H - 1);
                SelectObject(hdc, rdOld); DeleteObject(rowDiv);
            }

            // If fewer persons than visible rows, fill empty slots
            for (int i = np; i < visStart + LIST_ROWS; ++i) {
                int ry = listTop + (i - visStart) * ROW_H;
                HBRUSH eb = CreateSolidBrush((i % 2 == 0) ? BG : ROW_ALT);
                RECT er = {0, ry, rc.right - 28, ry + ROW_H};
                FillRect(hdc, &er, eb);
                DeleteObject(eb);
                if (np < MAX_PERSONS) {
                    SetTextColor(hdc, DIVIDER);
                    SelectObject(hdc, fRowS);
                    RECT eh = {0, ry + 13, rc.right - 28, ry + 29};
                    DrawTextW(hdc, L"\u2014  empty slot  \u2014", -1, &eh, DT_CENTER | DT_SINGLELINE);
                }
            }

            SelectClipRgn(hdc, NULL);
            DeleteObject(clip);
        }

        // ── Section label above button area ───────────────────────────────
        int btnY = HDR_H + LIST_H;
        HPEN div2 = CreatePen(PS_SOLID, 1, DIVIDER);
        HPEN d2Old = (HPEN)SelectObject(hdc, div2);
        MoveToEx(hdc, 0, btnY, NULL); LineTo(hdc, rc.right, btnY);
        SelectObject(hdc, d2Old); DeleteObject(div2);

        // ── Footer ────────────────────────────────────────────────────────
        SelectObject(hdc, fFt);
        SetTextColor(hdc, TXT_DIM);
        RECT rfooter = {0, rc.bottom - 20, rc.right, rc.bottom - 4};
        DrawTextW(hdc,
            L"ControlMux \u00a9 Alif Nurhidayat  \u2014  alifnurhidayatwork@gmail.com  "
            L"\u2014  Commercial: $29/seat or 7.5% royalty",
            -1, &rfooter, DT_CENTER | DT_SINGLELINE);

        SelectObject(hdc, fOld);

        // Atomic BitBlt memory buffer to screen
        BitBlt(hdcWindow, 0, 0, rc.right, rc.bottom, hdc, 0, 0, SRCCOPY);

        SelectObject(hdc, hbmOld);
        DeleteObject(hbm);
        DeleteDC(hdc);

        EndPaint(hwnd, &ps);
        return 0;
    }

    // ── Mouse wheel scrolls the person list ──────────────────────────────
    case WM_MOUSEWHEEL:
        if (self) {
            int delta = GET_WHEEL_DELTA_WPARAM(wp);
            int maxOff = (int)self->m_config.persons.size() - LIST_ROWS;
            if (delta < 0) self->m_scroll_offset = std::min(self->m_scroll_offset + 1, std::max(0, maxOff));
            else            self->m_scroll_offset = std::max(self->m_scroll_offset - 1, 0);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    // ── Pairing updated notification message ────────────────────────────
    case WM_PAIRING_UPDATED:
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    // ── Left click hit-testing for 1-click device pair & unassign buttons ─
    case WM_LBUTTONDOWN: {
        if (!self) break;
        int x = LOWORD(lp);
        int y = HIWORD(lp);

        // Click on active pairing banner cancels pairing
        if (self->m_dev_mgr.IsPairing() && y >= 94 && y <= 114) {
            self->m_dev_mgr.StopPairing();
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        int listTop  = HDR_H;
        int listBottom = listTop + LIST_H;
        if (y >= listTop && y < listBottom && x < PW - 30) {
            int clickedIdx = self->m_scroll_offset + (y - listTop) / ROW_H;
            if (clickedIdx >= 0 && clickedIdx < (int)self->m_config.persons.size()) {
                auto& p = self->m_config.persons[clickedIdx];

                // Mouse Button: x = 180..255
                if (x >= 180 && x <= 255) {
                    if (p.mouse_hwid.empty()) {
                        self->m_dev_mgr.StartPairing(p.id, true);
                    } else {
                        p.mouse_hwid.clear();
                        self->m_dev_mgr.SyncWithConfig(self->m_config);
                        ConfigManager::Save(L"controlmux_config.ini", self->m_config);
                    }
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                }
                // Keyboard Button: x = 260..335
                if (x >= 260 && x <= 335) {
                    if (p.keyboard_hwid.empty()) {
                        self->m_dev_mgr.StartPairing(p.id, false);
                    } else {
                        p.keyboard_hwid.clear();
                        self->m_dev_mgr.SyncWithConfig(self->m_config);
                        ConfigManager::Save(L"controlmux_config.ini", self->m_config);
                    }
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                }
                // Reset Button: x = 340..415
                if (x >= 340 && x <= 415) {
                    p.mouse_hwid.clear();
                    p.keyboard_hwid.clear();
                    self->m_dev_mgr.SyncWithConfig(self->m_config);
                    ConfigManager::Save(L"controlmux_config.ini", self->m_config);
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                }
            }
        }
        break;
    }

    // ── Button commands ───────────────────────────────────────────────────
    case WM_COMMAND:
        if (!self) break;
        switch (LOWORD(wp)) {

        case BTN_TOGGLE:
            self->m_config.enabled = !self->m_config.enabled;
            ConfigManager::Save(L"controlmux_config.ini", self->m_config);
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case BTN_MODE:
            self->m_config.mode = (self->m_config.mode == RoutingMode::SwitchedFocus)
                                  ? RoutingMode::DirectTarget
                                  : RoutingMode::SwitchedFocus;
            ConfigManager::Save(L"controlmux_config.ini", self->m_config);
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case BTN_ADD: {
            int n = (int)self->m_config.persons.size();
            if (n >= MAX_PERSONS) {
                MessageBoxW(hwnd, L"Maximum 16 persons reached.", L"ControlMux", MB_OK | MB_ICONINFORMATION);
                break;
            }
            PersonConfig p;
            p.id    = n + 1;
            p.name  = L"Person " + std::to_wstring(p.id);
            p.color = PERSON_COLORS[n % 16];
            self->m_config.persons.push_back(p);
            // Auto-scroll to bottom to show new entry
            int maxOff = (int)self->m_config.persons.size() - LIST_ROWS;
            self->m_scroll_offset = std::max(0, maxOff);
            self->m_dev_mgr.AddPerson(p);
            ConfigManager::Save(L"controlmux_config.ini", self->m_config);
            if (g_renderer) g_renderer->Render(self->m_dev_mgr.GetPersons(), self->m_config.active_person_id);
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
            break;
        }

        case BTN_REMOVE: {
            if (self->m_config.persons.size() <= 1) {
                MessageBoxW(hwnd, L"At least 1 person required.", L"ControlMux", MB_OK | MB_ICONINFORMATION);
                break;
            }
            self->m_config.persons.pop_back();
            int maxOff = (int)self->m_config.persons.size() - LIST_ROWS;
            self->m_scroll_offset = std::max(0, std::min(self->m_scroll_offset, maxOff));
            self->m_dev_mgr.RemoveLastPerson();
            ConfigManager::Save(L"controlmux_config.ini", self->m_config);
            if (g_renderer) g_renderer->Render(self->m_dev_mgr.GetPersons(), self->m_config.active_person_id);
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
            break;
        }

        case BTN_SCROLL_UP:
            self->m_scroll_offset = std::max(0, self->m_scroll_offset - 1);
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case BTN_SCROLL_DOWN: {
            int maxOff = (int)self->m_config.persons.size() - LIST_ROWS;
            self->m_scroll_offset = std::min(self->m_scroll_offset + 1, std::max(0, maxOff));
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

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

// ══════════════════════════════════════════════════════════════════════════
// Public interface
// ══════════════════════════════════════════════════════════════════════════
GuiWin32::GuiWin32(DeviceManager& dev_mgr, FocusRouter& router, AppConfig& config)
    : m_dev_mgr(dev_mgr), m_router(router), m_config(config) {
    s_gui_instance = this;
}

GuiWin32::~GuiWin32() { Shutdown(); }

bool GuiWin32::Initialize(HINSTANCE hInstance, HWND hwndMsg) {
    g_dbg.open(L"cmux_gui.log", std::ios::trunc);
    DBG(L"=== GuiWin32::Initialize ===");
    m_hwndMsg   = hwndMsg;
    m_hInstance = hInstance;
    DBG(L"Shell_TrayWnd: " << (FindWindowW(L"Shell_TrayWnd", NULL) ? L"FOUND" : L"NOT FOUND"));
    DBG(L"Calling OpenControlPanel...");
    OpenControlPanel();
    DBG(L"Done. m_hwndPanel=" << (ULONG_PTR)m_hwndPanel);
    return true;
}

void GuiWin32::OpenControlPanel() {
    if (m_hwndPanel && IsWindow(m_hwndPanel)) {
        SetForegroundWindow(m_hwndPanel);
        return;
    }

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = PanelWndProc;
    wc.hInstance     = m_hInstance;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"ControlMuxPanel";
    wc.hIcon         = LoadIcon(m_hInstance, MAKEINTRESOURCE(101));
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExW(&wc);

    POINT pt = {1, 1};
    HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi = {}; mi.cbSize = sizeof(mi);
    GetMonitorInfoW(hMon, &mi);
    int wx = mi.rcWork.left + (mi.rcWork.right  - mi.rcWork.left - PW) / 2;
    int wy = mi.rcWork.top  + (mi.rcWork.bottom - mi.rcWork.top  - PH) / 2;

    DBG(L"Monitor work=[" << mi.rcWork.left << L"," << mi.rcWork.top
        << L"," << mi.rcWork.right << L"," << mi.rcWork.bottom << L"]");
    DBG(L"Panel @ " << wx << L"," << wy << L" " << PW << L"x" << PH);

    m_hwndPanel = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_APPWINDOW,
        L"ControlMuxPanel", L"ControlMux \u2014 Control Center",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        wx, wy, PW, PH, NULL, NULL, m_hInstance, this);

    DBG(L"CreateWindowExW: " << (ULONG_PTR)m_hwndPanel << L" err=" << GetLastError());

    if (m_hwndPanel) {
        ShowWindow(m_hwndPanel, SW_SHOWNORMAL);
        UpdateWindow(m_hwndPanel);
        SetWindowPos(m_hwndPanel, HWND_TOPMOST, wx, wy, PW, PH,
                     SWP_SHOWWINDOW | SWP_NOACTIVATE);
        AllowSetForegroundWindow(GetCurrentProcessId());
        SwitchToThisWindow(m_hwndPanel, TRUE);
        SetForegroundWindow(m_hwndPanel);
        BringWindowToTop(m_hwndPanel);
        DBG(L"Panel shown OK.");
    }
}

void GuiWin32::ShowContextMenu(HWND) { OpenControlPanel(); }
void GuiWin32::OpenPairingWindow(HINSTANCE) { OpenControlPanel(); }

void GuiWin32::UpdateTrayTooltip() {
    if (!m_tray_added) return;
    std::wstring s = L"ControlMux \u2014 ";
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
