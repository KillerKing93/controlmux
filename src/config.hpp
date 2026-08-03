/**
 * @file config.hpp
 * @brief Configuration data structures and persistence manager for ControlMux.
 * 
 * Defines the application settings, Person profile configs, input routing modes,
 * and light serialization methods to load/save settings from disk.
 */

#ifndef CONTROLMUX_CONFIG_HPP
#define CONTROLMUX_CONFIG_HPP

#include <windows.h>
#include <string>
#include <vector>

/**
 * @brief Operating modes for input focus routing.
 */
enum class RoutingMode {
    SwitchedFocus = 0,  ///< Active person gets OS mouse focus; keypresses from other persons are isolated.
    DirectTarget = 1    ///< Keypresses are injected directly to the target window under each person's virtual cursor.
};

/// Maximum number of simultaneous persons supported.
static constexpr int MAX_PERSONS = 16;

/**
 * @brief Profile configuration for an individual person.
 */
struct PersonConfig {
    int id;                      ///< Unique Person ID (e.g. 1, 2)
    std::wstring name;           ///< Display label (e.g. "Person 1")
    DWORD color;                 ///< ARGB color used for virtual cursor & badges (e.g. 0xFFFF0033 for Red)
    std::wstring mouse_hwid;     ///< Hardware device ID substring of assigned physical mouse
    std::wstring keyboard_hwid;  ///< Hardware device ID substring of assigned physical keyboard
};

/**
 * @brief Global application settings container.
 */
struct AppConfig {
    bool enabled = true;                             ///< True if ControlMux input multiplexing is active
    RoutingMode mode = RoutingMode::SwitchedFocus;  ///< Active routing mode
    int active_person_id = 1;                        ///< ID of the person currently holding active focus
    std::vector<PersonConfig> persons;              ///< List of configured person profiles

    /**
     * @brief Constructs default configuration with Person 1 (Red) and Person 2 (Blue).
     */
    AppConfig() {
        // Person 1 (Red)
        PersonConfig p1;
        p1.id = 1;
        p1.name = L"Person 1";
        p1.color = 0xFFFF0033; // Red
        p1.mouse_hwid = L"";
        p1.keyboard_hwid = L"";

        // Person 2 (Blue)
        PersonConfig p2;
        p2.id = 2;
        p2.name = L"Person 2";
        p2.color = 0xFF0066FF; // Blue
        p2.mouse_hwid = L"";
        p2.keyboard_hwid = L"";

        persons.push_back(p1);
        persons.push_back(p2);
    }
};

/**
 * @brief File manager for saving and loading configuration settings.
 */
class ConfigManager {
public:
    /**
     * @brief Loads application settings from a simple key=value config file.
     * @param filename Path to the configuration file.
     * @param config Target AppConfig struct to populate.
     * @return True if file was successfully read, false otherwise.
     */
    static bool Load(const std::wstring& filename, AppConfig& config);

    /**
     * @brief Saves application settings to a simple key=value config file.
     * @param filename Target file path.
     * @param config AppConfig instance to serialize.
     * @return True if save succeeded.
     */
    static bool Save(const std::wstring& filename, const AppConfig& config);
};

#endif // CONTROLMUX_CONFIG_HPP
