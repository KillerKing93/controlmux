#ifndef CONTROLMUX_INPUT_ENGINE_HPP
#define CONTROLMUX_INPUT_ENGINE_HPP

#include <windows.h>
#include "device_manager.hpp"
#include "focus_router.hpp"
#include "overlay_renderer.hpp"
#include "config.hpp"

class InputEngine {
public:
    InputEngine(DeviceManager& dev_mgr, FocusRouter& router, OverlayRenderer& renderer, AppConfig& config);
    ~InputEngine();

    bool Initialize(HWND hwndMessage);
    LPARAM ProcessRawInput(WPARAM wParam, LPARAM lParam);

    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

    void Shutdown();

private:
    DeviceManager& m_dev_mgr;
    FocusRouter& m_router;
    OverlayRenderer& m_renderer;
    AppConfig& m_config;

    HWND m_hwnd = NULL;
    HHOOK m_keyboard_hook = NULL;

    static InputEngine* s_instance;
};

#endif // CONTROLMUX_INPUT_ENGINE_HPP
