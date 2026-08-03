#ifndef CONTROLMUX_CONFIG_HPP
#define CONTROLMUX_CONFIG_HPP

#include <windows.h>
#include <string>
#include <vector>

enum class RoutingMode {
    SwitchedFocus = 0,  // Active user gets OS focus; others isolated
    DirectTarget = 1    // Input injected directly to target window under each cursor
};

struct PersonConfig {
    int id;
    std::wstring name;
    DWORD color; // ARGB
    std::wstring mouse_hwid;
    std::wstring keyboard_hwid;
};

struct AppConfig {
    bool enabled = true;
    RoutingMode mode = RoutingMode::SwitchedFocus;
    int active_person_id = 1;
    std::vector<PersonConfig> persons;

    AppConfig() {
        // Default profiles
        PersonConfig p1;
        p1.id = 1;
        p1.name = L"Person 1";
        p1.color = 0xFFFF0033; // Bright Red
        p1.mouse_hwid = L"";
        p1.keyboard_hwid = L"";

        PersonConfig p2;
        p2.id = 2;
        p2.name = L"Person 2";
        p2.color = 0xFF0066FF; // Bright Blue
        p2.mouse_hwid = L"";
        p2.keyboard_hwid = L"";

        persons.push_back(p1);
        persons.push_back(p2);
    }
};

class ConfigManager {
public:
    static bool Load(const std::wstring& filename, AppConfig& config);
    static bool Save(const std::wstring& filename, const AppConfig& config);
};

#endif // CONTROLMUX_CONFIG_HPP
