/**
 * @file device_manager.hpp
 * @brief Device discovery, hardware ID extraction, and runtime Person state tracking.
 */

#ifndef CONTROLMUX_DEVICE_MANAGER_HPP
#define CONTROLMUX_DEVICE_MANAGER_HPP

#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include "config.hpp"

/**
 * @brief Information about a physical input device enumerated via Raw Input.
 */
struct PhysicalDevice {
    HANDLE handle;             ///< Win32 device handle (hDevice)
    DWORD type;                ///< RIM_TYPEMOUSE or RIM_TYPEKEYBOARD
    std::wstring name;         ///< Friendly display name
    std::wstring hardware_id;  ///< Extracted HID hardware instance ID string
};

/**
 * @brief Live runtime state for an active Person profile.
 */
struct PersonState {
    int id;
    std::wstring name;
    DWORD color;
    std::wstring mouse_hwid;
    std::wstring keyboard_hwid;
    HANDLE mouse_handle = NULL;
    HANDLE keyboard_handle = NULL;

    int cursor_x = 0;              ///< Virtual cursor X coordinate
    int cursor_y = 0;              ///< Virtual cursor Y coordinate
    bool mouse_down_left = false;  ///< Left button state
    bool mouse_down_right = false; ///< Right button state
    bool mouse_down_middle = false;///< Middle button state
    HWND target_hwnd = NULL;       ///< Currently targeted window handle under virtual cursor
};

/**
 * @brief Manages enumeration, identification, and profile assignment of physical HID input devices.
 */
class DeviceManager {
public:
    DeviceManager();
    ~DeviceManager();

    /**
     * @brief Queries Win32 Raw Input API to enumerate all connected physical mice and keyboards.
     */
    void RefreshDevices();

    std::vector<PhysicalDevice> GetConnectedMice() const;
    std::vector<PhysicalDevice> GetConnectedKeyboards() const;

    /**
     * @brief Resolves a runtime Win32 device handle (HANDLE) to its unique HID hardware instance ID.
     */
    std::wstring GetHardwareId(HANDLE hDevice);

    /**
     * @brief Returns a friendly human-readable label for a device handle.
     */
    std::wstring GetFriendlyDeviceName(HANDLE hDevice, DWORD type);

    /**
     * @brief Synchronizes runtime PersonState handles with AppConfig persistent settings.
     */
    void SyncWithConfig(AppConfig& config);
    
    PersonState* GetPersonByMouseHandle(HANDLE hDevice);
    PersonState* GetPersonByKeyboardHandle(HANDLE hDevice);
    PersonState* GetPersonById(int id);
    std::vector<PersonState>& GetPersons() { return m_persons; }

    // Interactive Device Pairing Wizard
    void StartPairing(int person_id, bool is_mouse);
    void StopPairing();
    bool IsPairing() const { return m_pairing_active; }
    
    /**
     * @brief Called when raw input arrives while the pairing wizard is active to bind the device.
     */
    bool OnInputReceivedForPairing(HANDLE hDevice, DWORD type, AppConfig& config);

private:
    std::vector<PhysicalDevice> m_devices;
    std::vector<PersonState> m_persons;
    std::map<HANDLE, std::wstring> m_handle_to_hwid_cache;
    mutable std::mutex m_mutex;

    bool m_pairing_active = false;
    int m_pairing_person_id = 0;
    bool m_pairing_is_mouse = false;
};

#endif // CONTROLMUX_DEVICE_MANAGER_HPP
