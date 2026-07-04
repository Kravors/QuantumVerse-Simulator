/**
 * @file ConfigManager.cpp
 * @brief Implementation of configuration management system
 */

#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

namespace quantumverse {
namespace utils {

// Static instance for singleton pattern
ConfigManager& ConfigManager::instance() {
    static ConfigManager inst;
    return inst;
}

ConfigManager::~ConfigManager() = default;  // unique_ptr handles cleanup automatically

bool ConfigManager::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        // Parse key=value format
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = line.substr(0, eqPos);
        std::string value = line.substr(eqPos + 1);

        // Try to parse as different types
        try {
            // Try integer
            if (m_config.count(key) > 0 && m_config[key].type == ConfigType::INT) {
                m_config[key].value.intVal = std::stoi(value);
            }
            // Try float
            else if (m_config.count(key) > 0 && m_config[key].type == ConfigType::FLOAT) {
                m_config[key].value.floatVal = std::stof(value);
            }
            // Try boolean
            else if (m_config.count(key) > 0 && m_config[key].type == ConfigType::BOOL) {
                m_config[key].value.boolVal = (value == "true" || value == "1");
            }
            // Try string
            else if (m_config.count(key) > 0 && m_config[key].type == ConfigType::STRING) {
                if (m_config[key].value.stringVal) {
                    *m_config[key].value.stringVal = value;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing config value for key '" << key << "': " << e.what() << std::endl;
        }
    }

    return true;
}

bool ConfigManager::saveToFile(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }

    for (const auto& pair : m_config) {
        file << pair.first << "=";
        switch (pair.second.type) {
            case ConfigType::INT:
                file << pair.second.value.intVal;
                break;
            case ConfigType::FLOAT:
                file << pair.second.value.floatVal;
                break;
            case ConfigType::BOOL:
                file << (pair.second.value.boolVal ? "true" : "false");
                break;
            case ConfigType::STRING:
                file << (pair.second.value.stringVal ? *pair.second.value.stringVal : "");
                break;
            default:
                break;
        }
        file << "\n";
    }

    return true;
}

int ConfigManager::getInt(const std::string& key, int defaultValue) const {
    auto it = m_config.find(key);
    if (it != m_config.end() && it->second.type == ConfigType::INT) {
        return it->second.value.intVal;
    }
    return defaultValue;
}

float ConfigManager::getFloat(const std::string& key, float defaultValue) const {
    auto it = m_config.find(key);
    if (it != m_config.end() && it->second.type == ConfigType::FLOAT) {
        return it->second.value.floatVal;
    }
    return defaultValue;
}

bool ConfigManager::getBool(const std::string& key, bool defaultValue) const {
    auto it = m_config.find(key);
    if (it != m_config.end() && it->second.type == ConfigType::BOOL) {
        return it->second.value.boolVal;
    }
    return defaultValue;
}

std::string ConfigManager::getString(const std::string& key, const std::string& defaultValue) const {
    auto it = m_config.find(key);
    if (it != m_config.end() && it->second.type == ConfigType::STRING) {
        return it->second.value.stringVal ? *it->second.value.stringVal : defaultValue;
    }
    return defaultValue;
}

void ConfigManager::setInt(const std::string& key, int value) {
    ConfigEntry entry;
    entry.type = ConfigType::INT;
    entry.value.intVal = value;
    m_config[key] = entry;
}

void ConfigManager::setFloat(const std::string& key, float value) {
    ConfigEntry entry;
    entry.type = ConfigType::FLOAT;
    entry.value.floatVal = value;
    m_config[key] = entry;
}

void ConfigManager::setBool(const std::string& key, bool value) {
    ConfigEntry entry;
    entry.type = ConfigType::BOOL;
    entry.value.boolVal = value;
    m_config[key] = entry;
}

void ConfigManager::setString(const std::string& key, const std::string& value) {
    ConfigEntry entry;
    entry.type = ConfigType::STRING;
    entry.value.stringVal = std::make_unique<std::string>(value);
    m_config[key] = entry;
}

void ConfigManager::registerCallback(const std::string& key, std::function<void(const ConfigValue&)> callback) {
    auto it = m_config.find(key);
    if (it != m_config.end()) {
        it->second.onChange = callback;
    }
}

std::vector<std::string> ConfigManager::getKeys() const {
    std::vector<std::string> keys;
    for (const auto& pair : m_config) {
        keys.push_back(pair.first);
    }
    return keys;
}

} // namespace utils
} // namespace quantumverse