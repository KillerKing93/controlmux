/**
 * @file input_engine.hpp
 * @brief Raw Input registration, message loop processing, and low-level hook handlers.
 */

#ifndef CONTROLMUX_INPUT_ENGINE_HPP
#define CONTROLMUX_INPUT_ENGINE_HPP

#include <windows.h>
#include "device_manager.hpp"
#include "focus_router.hpp"
#include "overlay_renderer.hpp"
#include "config.hpp"

/**
 * @brief Manages Win32 Raw Input device registration (WM_INPUT) and input processing.
 */
class InputEngine {
public:
    InputEngine(DeviceManager& dev_mgr, FocusRouter& router, OverlayRenderer& renderer, AppConfig& config);
    ~InputEngine();

    /**
     * @brief Registers Raw Input devices for mice and keyboards with RIDEV_INPUTSINK flag.
     * @param hwndMessage Message window handle to receive WM_INPUT messages.
     * @return True if registration succeeded.
     */
    bool Initialize(HWND hwndMessage);

    /**
     * @brief Processes incoming WM_INPUT raw input messages from the Win32 message loop.
     */
    LPARAM ProcessRawInput(WPARAM wParam, LPARAM lParam);

    /**
     * @brief Low-level keyboard hook callback procedure.
     */
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

    /**
     * @brief Unregisters raw input and unhooks low-level hooks.
     */
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
