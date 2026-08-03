/**
 * @file device_manager.cpp
 * @brief Implementation of device discovery, hardware ID extraction, and runtime Person state tracking.
 */

#include "device_manager.hpp"
#include <algorithm>
#include <iostream>

DeviceManager::DeviceManager() {
    RefreshDevices();
}

DeviceManager::~DeviceManager() {}

/**
 * Enumerates all connected HID mouse and keyboard devices using GetRawInputDeviceList.
 */
void DeviceManager::RefreshDevices() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_devices.clear();

    UINT num_devices = 0;
    if (GetRawInputDeviceList(NULL, &num_devices, sizeof(RAWINPUTDEVICELIST)) != 0 || num_devices == 0) {
        return;
    }

    std::vector<RAWINPUTDEVICELIST> raw_list(num_devices);
    if (GetRawInputDeviceList(raw_list.data(), &num_devices, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1) {
        return;
    }

    for (UINT i = 0; i < num_devices; ++i) {
        if (raw_list[i].dwType == RIM_TYPEMOUSE || raw_list[i].dwType == RIM_TYPEKEYBOARD) {
            PhysicalDevice dev;
            dev.handle = raw_list[i].hDevice;
            dev.type = raw_list[i].dwType;
            dev.hardware_id = GetHardwareId(dev.handle);
            dev.name = GetFriendlyDeviceName(dev.handle, dev.type);

            m_handle_to_hwid_cache[dev.handle] = dev.hardware_id;
            m_devices.push_back(dev);
        }
    }
}

/**
 * Queries GetRawInputDeviceInfoW to retrieve the unique HID device path and extracts the hardware ID.
 */
std::wstring DeviceManager::GetHardwareId(HANDLE hDevice) {
    if (hDevice == NULL) return L"";

    UINT name_size = 0;
    GetRawInputDeviceInfoW(hDevice, RIDI_DEVICENAME, NULL, &name_size);
    if (name_size == 0) return L"";

    std::vector<wchar_t> name_buf(name_size);
    if (GetRawInputDeviceInfoW(hDevice, RIDI_DEVICENAME, name_buf.data(), &name_size) == (UINT)-1) {
        return L"";
    }

    std::wstring raw_name(name_buf.data());
    
    // Extract hardware instance ID string (e.g. HID#VID_046D&PID_C52B...)
    size_t pos = raw_name.find(L"HID#");
    if (pos == std::wstring::npos) pos = raw_name.find(L"ACPI#");
    if (pos == std::wstring::npos) pos = raw_name.find(L"USB#");

    if (pos != std::wstring::npos) {
        std::wstring hwid = raw_name.substr(pos);
        // Trim trailing GUID substring #{...}
        size_t hash_pos = hwid.find(L"#{");
        if (hash_pos != std::wstring::npos) {
            hwid = hwid.substr(0, hash_pos);
        }
        return hwid;
    }

    return raw_name;
}

std::wstring DeviceManager::GetFriendlyDeviceName(HANDLE hDevice, DWORD type) {
    std::wstring hwid = GetHardwareId(hDevice);
    if (type == RIM_TYPEMOUSE) {
        return L"Mouse (" + (hwid.empty() ? L"Standard HID" : hwid.substr(0, 24)) + L")";
    } else {
        return L"Keyboard (" + (hwid.empty() ? L"Standard HID" : hwid.substr(0, 24)) + L")";
    }
}

std::vector<PhysicalDevice> DeviceManager::GetConnectedMice() const {
    std::vector<PhysicalDevice> mice;
    for (const auto& dev : m_devices) {
        if (dev.type == RIM_TYPEMOUSE) mice.push_back(dev);
    }
    return mice;
}

std::vector<PhysicalDevice> DeviceManager::GetConnectedKeyboards() const {
    std::vector<PhysicalDevice> keyboards;
    for (const auto& dev : m_devices) {
        if (dev.type == RIM_TYPEKEYBOARD) keyboards.push_back(dev);
    }
    return keyboards;
}

/**
 * Matches configured Person profile hardware IDs against live connected physical device handles.
 */
void DeviceManager::SyncWithConfig(AppConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    RefreshDevices();

    m_persons.clear();
    int virtual_screen_w = GetSystemMetrics(SM_CXSCREEN);
    int virtual_screen_h = GetSystemMetrics(SM_CYSCREEN);

    for (const auto& pc : config.persons) {
        PersonState ps;
        ps.id = pc.id;
        ps.name = pc.name;
        ps.color = pc.color;
        ps.mouse_hwid = pc.mouse_hwid;
        ps.keyboard_hwid = pc.keyboard_hwid;

        // Position initial virtual cursors evenly across screen
        ps.cursor_x = (virtual_screen_w / (config.persons.size() + 1)) * pc.id;
        ps.cursor_y = virtual_screen_h / 2;

        // Match hardware device handles by ID
        for (const auto& dev : m_devices) {
            if (dev.type == RIM_TYPEMOUSE && !pc.mouse_hwid.empty()) {
                if (dev.hardware_id.find(pc.mouse_hwid) != std::wstring::npos ||
                    pc.mouse_hwid.find(dev.hardware_id) != std::wstring::npos) {
                    ps.mouse_handle = dev.handle;
                }
            }
            if (dev.type == RIM_TYPEKEYBOARD && !pc.keyboard_hwid.empty()) {
                if (dev.hardware_id.find(pc.keyboard_hwid) != std::wstring::npos ||
                    pc.keyboard_hwid.find(dev.hardware_id) != std::wstring::npos) {
                    ps.keyboard_handle = dev.handle;
                }
            }
        }

        m_persons.push_back(ps);
    }
}

PersonState* DeviceManager::GetPersonByMouseHandle(HANDLE hDevice) {
    for (auto& person : m_persons) {
        if (person.mouse_handle == hDevice) return &person;
    }
    // Fallback: Default to Person 1 if device is unassigned
    if (!m_persons.empty()) return &m_persons[0];
    return nullptr;
}

PersonState* DeviceManager::GetPersonByKeyboardHandle(HANDLE hDevice) {
    for (auto& person : m_persons) {
        if (person.keyboard_handle == hDevice) return &person;
    }
    // Fallback
    if (!m_persons.empty()) return &m_persons[0];
    return nullptr;
}

PersonState* DeviceManager::GetPersonById(int id) {
    for (auto& person : m_persons) {
        if (person.id == id) return &person;
    }
    return nullptr;
}

void DeviceManager::StartPairing(int person_id, bool is_mouse) {
    m_pairing_active = true;
    m_pairing_person_id = person_id;
    m_pairing_is_mouse = is_mouse;
}

void DeviceManager::StopPairing() {
    m_pairing_active = false;
}

/**
 * Handles incoming raw input during interactive device pairing wizard to capture and bind device hardware IDs.
 */
bool DeviceManager::OnInputReceivedForPairing(HANDLE hDevice, DWORD type, AppConfig& config) {
    if (!m_pairing_active) return false;

    if (m_pairing_is_mouse && type == RIM_TYPEMOUSE) {
        std::wstring hwid = GetHardwareId(hDevice);
        for (auto& pc : config.persons) {
            if (pc.id == m_pairing_person_id) {
                pc.mouse_hwid = hwid;
                StopPairing();
                SyncWithConfig(config);
                return true;
            }
        }
    } else if (!m_pairing_is_mouse && type == RIM_TYPEKEYBOARD) {
        std::wstring hwid = GetHardwareId(hDevice);
        for (auto& pc : config.persons) {
            if (pc.id == m_pairing_person_id) {
                pc.keyboard_hwid = hwid;
                StopPairing();
                SyncWithConfig(config);
                return true;
            }
        }
    }

    return false;
}
