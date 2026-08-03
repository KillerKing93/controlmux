#include "input_engine.hpp"
#include <iostream>

InputEngine* InputEngine::s_instance = nullptr;

InputEngine::InputEngine(DeviceManager& dev_mgr, FocusRouter& router, OverlayRenderer& renderer, AppConfig& config)
    : m_dev_mgr(dev_mgr), m_router(router), m_renderer(renderer), m_config(config) {
    s_instance = this;
}

InputEngine::~InputEngine() {
    Shutdown();
}

bool InputEngine::Initialize(HWND hwndMessage) {
    m_hwnd = hwndMessage;

    RAWINPUTDEVICE rid[2];

    // Mouse
    rid[0].usUsagePage = 0x01;
    rid[0].usUsage = 0x02;
    rid[0].dwFlags = RIDEV_INPUTSINK;
    rid[0].hwndTarget = hwndMessage;

    // Keyboard
    rid[1].usUsagePage = 0x01;
    rid[1].usUsage = 0x06;
    rid[1].dwFlags = RIDEV_INPUTSINK;
    rid[1].hwndTarget = hwndMessage;

    if (!RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE))) {
        return false;
    }

    // Install low-level keyboard hook
    m_keyboard_hook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);

    return true;
}

LPARAM InputEngine::ProcessRawInput(WPARAM wParam, LPARAM lParam) {
    UINT dwSize = 0;
    GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
    if (dwSize == 0) return 0;

    std::vector<BYTE> lpb(dwSize);
    if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb.data(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
        return 0;
    }

    RAWINPUT* raw = (RAWINPUT*)lpb.data();

    // Check if pairing wizard is waiting for input
    if (m_dev_mgr.IsPairing()) {
        if (m_dev_mgr.OnInputReceivedForPairing(raw->header.hDevice, raw->header.dwType, m_config)) {
            ConfigManager::Save(L"controlmux_config.ini", m_config);
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

            // Redraw virtual cursor overlay
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
        KBDLLHOOKSTRUCT* pKbd = (KBDLLHOOKSTRUCT*)lParam;
        // In switched focus mode, if a secondary user typed, low level hook allows filtering out extra OS keystroke echoes
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void InputEngine::Shutdown() {
    if (m_keyboard_hook) {
        UnhookWindowsHookEx(m_keyboard_hook);
        m_keyboard_hook = NULL;
    }
}
