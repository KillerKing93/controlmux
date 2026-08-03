/**
 * @file input_engine.cpp
 * @brief Implementation of Raw Input device registration and input message processing.
 */

#include "input_engine.hpp"
#include "gui_win32.hpp"
#include <iostream>

InputEngine* InputEngine::s_instance = nullptr;

InputEngine::InputEngine(DeviceManager& dev_mgr, FocusRouter& router, OverlayRenderer& renderer, AppConfig& config)
    : m_dev_mgr(dev_mgr), m_router(router), m_renderer(renderer), m_config(config) {
    s_instance = this;
}

InputEngine::~InputEngine() {
    Shutdown();
}

/**
 * Registers mouse and keyboard devices with Win32 RegisterRawInputDevices using RIDEV_INPUTSINK.
 */
bool InputEngine::Initialize(HWND hwndMessage) {
    m_hwnd = hwndMessage;

    RAWINPUTDEVICE rid[2];

    // Generic Desktop Mouse (Usage Page 0x01, Usage 0x02)
    rid[0].usUsagePage = 0x01;
    rid[0].usUsage = 0x02;
    rid[0].dwFlags = RIDEV_INPUTSINK;
    rid[0].hwndTarget = hwndMessage;

    // Generic Desktop Keyboard (Usage Page 0x01, Usage 0x06)
    rid[1].usUsagePage = 0x01;
    rid[1].usUsage = 0x06;
    rid[1].dwFlags = RIDEV_INPUTSINK;
    rid[1].hwndTarget = hwndMessage;

    if (!RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE))) {
        return false;
    }

    // Install WH_KEYBOARD_LL hook for keystroke filtering
    m_keyboard_hook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);

    return true;
}

/**
 * Handles WM_INPUT messages, extracting RAWINPUT structs and forwarding to FocusRouter & OverlayRenderer.
 */
LPARAM InputEngine::ProcessRawInput(WPARAM wParam, LPARAM lParam) {
    UINT dwSize = 0;
    GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
    if (dwSize == 0) return 0;

    std::vector<BYTE> lpb(dwSize);
    if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb.data(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
        return 0;
    }

    RAWINPUT* raw = (RAWINPUT*)lpb.data();

    // Check if device pairing wizard is listening
    if (m_dev_mgr.IsPairing()) {
        if (m_dev_mgr.OnInputReceivedForPairing(raw->header.hDevice, raw->header.dwType, m_config)) {
            ConfigManager::Save(L"controlmux_config.ini", m_config);
            if (GuiWin32::s_gui_instance && GuiWin32::s_gui_instance->GetPanelHwnd()) {
                PostMessage(GuiWin32::s_gui_instance->GetPanelHwnd(), WM_PAIRING_UPDATED, 0, 0);
            }
            return 0;
        }
    }

    if (!m_config.enabled) return 0;

    if (raw->header.dwType == RIM_TYPEMOUSE) {
        HANDLE hMouse = raw->header.hDevice;
        PersonState* person = m_dev_mgr.GetPersonByMouseHandle(hMouse);
        if (person) {
            int dx = raw->data.mouse.lLastX;
            int dy = raw->data.mouse.lLastY;
            DWORD button_flags = raw->data.mouse.usButtonFlags;

            m_router.OnPersonMouseMove(*person, dx, dy, m_config);
            if (button_flags != 0) {
                m_router.OnPersonMouseButton(*person, button_flags, m_config);
            }

            // Trigger overlay render update
            m_renderer.Render(m_dev_mgr.GetPersons(), m_router.GetActivePersonId());
        }
    } else if (raw->header.dwType == RIM_TYPEKEYBOARD) {
        HANDLE hKeyboard = raw->header.hDevice;
        PersonState* person = m_dev_mgr.GetPersonByKeyboardHandle(hKeyboard);
        if (person) {
            USHORT vkey = raw->data.keyboard.VKey;
            USHORT flags = raw->data.keyboard.Flags;

            m_router.OnPersonKeyboardInput(*person, vkey, flags, m_config);
        }
    }

    return 0;
}

LRESULT CALLBACK InputEngine::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && s_instance && s_instance->m_config.enabled) {
        // Low level keyboard filtering procedure
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void InputEngine::Shutdown() {
    if (m_keyboard_hook) {
        UnhookWindowsHookEx(m_keyboard_hook);
        m_keyboard_hook = NULL;
    }
}
