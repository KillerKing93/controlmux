/**
 * @file config.cpp
 * @brief Implementation of configuration loading and saving routines.
 */

#include "config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

/**
 * Parses the custom key-value configuration file into an AppConfig instance.
 */
bool ConfigManager::Load(const std::wstring& filename, AppConfig& config) {
    std::wifstream infile(filename.c_str());
    if (!infile.is_open()) {
        return false; // Uses default pre-initialized config if file doesn't exist
    }

    std::wstring line;
    PersonConfig* current_person = nullptr;

    while (std::getline(infile, line)) {
        // Parse global settings
        if (line.find(L"enabled=") == 0) {
            config.enabled = (line.substr(8) == L"1");
        } else if (line.find(L"mode=") == 0) {
            config.mode = (RoutingMode)_wtoi(line.substr(5).c_str());
        } else if (line.find(L"[person]") == 0) {
            // New Person block detected
            PersonConfig p;
            p.id = (int)config.persons.size() + 1;
            p.name = L"Person " + std::to_wstring(p.id);
            p.color = (p.id == 1) ? 0xFFFF0033 : 0xFF0066FF;
            config.persons.push_back(p);
            current_person = &config.persons.back();
        } else if (current_person) {
            // Parse person-specific attributes
            if (line.find(L"id=") == 0) {
                current_person->id = _wtoi(line.substr(3).c_str());
            } else if (line.find(L"name=") == 0) {
                current_person->name = line.substr(5);
            } else if (line.find(L"color=") == 0) {
                current_person->color = wcstoul(line.substr(6).c_str(), NULL, 16);
            } else if (line.find(L"mouse_hwid=") == 0) {
                current_person->mouse_hwid = line.substr(11);
            } else if (line.find(L"keyboard_hwid=") == 0) {
                current_person->keyboard_hwid = line.substr(14);
            }
        }
    }
    return true;
}

/**
 * Writes the AppConfig state out to a clean key-value formatted text file.
 */
bool ConfigManager::Save(const std::wstring& filename, const AppConfig& config) {
    std::wofstream outfile(filename.c_str());
    if (!outfile.is_open()) return false;

    outfile << L"enabled=" << (config.enabled ? L"1" : L"0") << L"\n";
    outfile << L"mode=" << (int)config.mode << L"\n";

    for (const auto& p : config.persons) {
        outfile << L"[person]\n";
        outfile << L"id=" << p.id << L"\n";
        outfile << L"name=" << p.name << L"\n";
        outfile << L"color=" << std::hex << p.color << std::dec << L"\n";
        outfile << L"mouse_hwid=" << p.mouse_hwid << L"\n";
        outfile << L"keyboard_hwid=" << p.keyboard_hwid << L"\n";
    }

    return true;
}
