/**
 * @file ConfigManager.h
 * @brief Configuration management system for QuantumVerse
 *
 * Provides centralized configuration management with JSON support,
 * runtime modification, and automatic persistence.
 */

#pragma once

#include <string>
#include <map>
#include <vector>
#include <cstdint>
#include <functional>
#include <memory>

namespace quantumverse {
namespace utils {

/**
 * @brief Configuration value types
 */
enum class ConfigType {
    INT,
    FLOAT,
    BOOL,
    STRING,
    VECTOR2,
    VECTOR3,
    VECTOR4
};

/**
 * @brief Configuration value storage - uses unique_ptr for string to ensure RAII
 */
struct ConfigValue {
    int intVal{0};
    float floatVal{0.0f};
    bool boolVal{false};
    std::unique_ptr<std::string> stringVal;
    float vec2[2]{0.0f, 0.0f};
    float vec3[3]{0.0f, 0.0f, 0.0f};
    float vec4[4]{0.0f, 0.0f, 0.0f, 0.0f};
    
    // Default constructor
    ConfigValue() = default;
    
    // Copy constructor
    ConfigValue(const ConfigValue& other) {
        intVal = other.intVal;
        floatVal = other.floatVal;
        boolVal = other.boolVal;
        if (other.stringVal) {
            stringVal = std::make_unique<std::string>(*other.stringVal);
        }
        for (int i = 0; i < 2; ++i) vec2[i] = other.vec2[i];
        for (int i = 0; i < 3; ++i) vec3[i] = other.vec3[i];
        for (int i = 0; i < 4; ++i) vec4[i] = other.vec4[i];
    }
    
    // Copy assignment
    ConfigValue& operator=(const ConfigValue& other) {
        if (this != &other) {
            intVal = other.intVal;
            floatVal = other.floatVal;
            boolVal = other.boolVal;
            if (other.stringVal) {
                stringVal = std::make_unique<std::string>(*other.stringVal);
            } else {
                stringVal.reset();
            }
            for (int i = 0; i < 2; ++i) vec2[i] = other.vec2[i];
            for (int i = 0; i < 3; ++i) vec3[i] = other.vec3[i];
            for (int i = 0; i < 4; ++i) vec4[i] = other.vec4[i];
        }
        return *this;
    }
    
    // Move constructor
    ConfigValue(ConfigValue&&) noexcept = default;
    
    // Move assignment
    ConfigValue& operator=(ConfigValue&&) noexcept = default;
};

/**
 * @brief Configuration entry
 */
struct ConfigEntry {
    ConfigType type{ConfigType::INT};
    ConfigValue value;
    std::string description;
    std::function<void(const ConfigValue&)> onChange;
};

/**
 * @brief Central configuration manager
 */
class ConfigManager {
public:
    static ConfigManager& instance();

    /**
     * @brief Load configuration from file
     */
    bool loadFromFile(const std::string& path);

    /**
     * @brief Save configuration to file
     */
    bool saveToFile(const std::string& path);

    /**
     * @brief Get integer value
     */
    int getInt(const std::string& key, int defaultValue = 0) const;

    /**
     * @brief Get float value
     */
    float getFloat(const std::string& key, float defaultValue = 0.0f) const;

    /**
     * @brief Get boolean value
     */
    bool getBool(const std::string& key, bool defaultValue = false) const;

    /**
     * @brief Get string value
     */
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;

    /**
     * @brief Set integer value
     */
    void setInt(const std::string& key, int value);

    /**
     * @brief Set float value
     */
    void setFloat(const std::string& key, float value);

    /**
     * @brief Set boolean value
     */
    void setBool(const std::string& key, bool value);

    /**
     * @brief Set string value
     */
    void setString(const std::string& key, const std::string& value);

    /**
     * @brief Register a configuration change callback
     */
    void registerCallback(const std::string& key, std::function<void(const ConfigValue&)> callback);

    /**
     * @brief Get all configuration keys
     */
    std::vector<std::string> getKeys() const;

private:
    ConfigManager() = default;
    ~ConfigManager();

    std::map<std::string, ConfigEntry> m_config;
};

// Convenience macros
#define CONFIG_INT(key, default) ConfigManager::instance().getInt(key, default)
#define CONFIG_FLOAT(key, default) ConfigManager::instance().getFloat(key, default)
#define CONFIG_BOOL(key, default) ConfigManager::instance().getBool(key, default)
#define CONFIG_STRING(key, default) ConfigManager::instance().getString(key, default)

} // namespace utils
} // namespace quantumverse