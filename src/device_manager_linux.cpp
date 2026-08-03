/**
 * @file device_manager_linux.cpp
 * @brief Linux native device discovery and /dev/input/ event node mapping.
 * 
 * Supports Arch Linux, Debian, CachyOS, SteamOS, Fedora, Ubuntu, and openSUSE.
 * Enumerates input devices using libevdev & /dev/input/by-id/ hardware symlinks.
 */

#ifndef _WIN32

#include "device_manager.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <libevdev/libevdev.h>
#include <iostream>
#include <algorithm>

DeviceManager::DeviceManager() {
    RefreshDevices();
}

DeviceManager::~DeviceManager() {}

/**
 * Scans /dev/input/ directory for event* nodes and enumerates HID input capabilities.
 */
void DeviceManager::RefreshDevices() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_devices.clear();

    const char* input_dir = "/dev/input";
    DIR* dir = opendir(input_dir);
    if (!dir) return;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "event", 5) == 0) {
            std::string path = std::string(input_dir) + "/" + entry->d_name;
            int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
            if (fd < 0) continue;

            struct libevdev* dev = NULL;
            if (libevdev_new_from_fd(fd, &dev) < 0) {
                close(fd);
                continue;
            }

            PhysicalDevice pdev;
            pdev.handle = (HANDLE)(intptr_t)fd;
            
            // Check device capabilities (Mouse vs Keyboard)
            bool is_mouse = libevdev_has_event_code(dev, EV_REL, REL_X) && libevdev_has_event_code(dev, EV_REL, REL_Y);
            bool is_keyboard = libevdev_has_event_code(dev, EV_KEY, KEY_A) && libevdev_has_event_code(dev, EV_KEY, KEY_Z);

            if (is_mouse) pdev.type = RIM_TYPEMOUSE;
            else if (is_keyboard) pdev.type = RIM_TYPEKEYBOARD;
            else {
                libevdev_free(dev);
                close(fd);
                continue;
            }

            const char* name = libevdev_get_name(dev);
            std::string name_str = name ? name : "Unknown Input Device";
            pdev.name = std::wstring(name_str.begin(), name_str.end());

            // Build hardware ID from Vendor ID, Product ID, and Device Name
            int bustype = libevdev_get_id_bustype(dev);
            int vendor = libevdev_get_id_vendor(dev);
            int product = libevdev_get_id_product(dev);

            char hwid_buf[256];
            snprintf(hwid_buf, sizeof(hwid_buf), "USB_VID_%04X&PID_%04X_%s", vendor, product, name_str.c_str());
            std::string hwid_str(hwid_buf);
            pdev.hardware_id = std::wstring(hwid_str.begin(), hwid_str.end());

            m_devices.push_back(pdev);
            libevdev_free(dev);
        }
    }
    closedir(dir);
}

std::wstring DeviceManager::GetHardwareId(HANDLE hDevice) {
    for (const auto& dev : m_devices) {
        if (dev.handle == hDevice) return dev.hardware_id;
    }
    return L"";
}

std::wstring DeviceManager::GetFriendlyDeviceName(HANDLE hDevice, DWORD type) {
    for (const auto& dev : m_devices) {
        if (dev.handle == hDevice) return dev.name;
    }
    return L"Linux Input Device";
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

void DeviceManager::SyncWithConfig(AppConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    RefreshDevices();

    m_persons.clear();

    for (const auto& pc : config.persons) {
        PersonState ps;
        ps.id = pc.id;
        ps.name = pc.name;
        ps.color = pc.color;
        ps.mouse_hwid = pc.mouse_hwid;
        ps.keyboard_hwid = pc.keyboard_hwid;
        ps.cursor_x = 100 * pc.id;
        ps.cursor_y = 300;

        for (const auto& dev : m_devices) {
            if (dev.type == RIM_TYPEMOUSE && !pc.mouse_hwid.empty()) {
                if (dev.hardware_id.find(pc.mouse_hwid) != std::wstring::npos) {
                    ps.mouse_handle = dev.handle;
                }
            }
            if (dev.type == RIM_TYPEKEYBOARD && !pc.keyboard_hwid.empty()) {
                if (dev.hardware_id.find(pc.keyboard_hwid) != std::wstring::npos) {
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
    if (!m_persons.empty()) return &m_persons[0];
    return nullptr;
}

PersonState* DeviceManager::GetPersonByKeyboardHandle(HANDLE hDevice) {
    for (auto& person : m_persons) {
        if (person.keyboard_handle == hDevice) return &person;
    }
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

bool DeviceManager::OnInputReceivedForPairing(HANDLE hDevice, DWORD type, AppConfig& config) {
    if (!m_pairing_active) return false;
    std::wstring hwid = GetHardwareId(hDevice);
    for (auto& pc : config.persons) {
        if (pc.id == m_pairing_person_id) {
            if (m_pairing_is_mouse && type == RIM_TYPEMOUSE) {
                pc.mouse_hwid = hwid;
                StopPairing();
                SyncWithConfig(config);
                return true;
            } else if (!m_pairing_is_mouse && type == RIM_TYPEKEYBOARD) {
                pc.keyboard_hwid = hwid;
                StopPairing();
                SyncWithConfig(config);
                return true;
            }
        }
    }
    return false;
}

#endif // !_WIN32
