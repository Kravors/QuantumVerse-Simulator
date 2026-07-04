/**
 * @file FileIO.h
 * @brief Secure file I/O operations for QuantumVerse
 *
 * Provides file loading and saving with security validation,
 * supporting .spc (spacetime configuration) and .json formats.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <array>
#include <filesystem>

namespace quantumverse {
namespace utils {

/**
 * @brief Result of a file operation
 */
enum class FileResult {
    Success,
    FileNotFound,
    AccessDenied,
    InvalidFormat,
    SecurityViolation,
    UnknownError
};

/**
 * @brief Spacetime configuration data structure
 */
struct SpacetimeConfig {
    double time = 0.0;
    double timeScale = 1.0;
    std::array<double, 4> cameraPosition = {0.0, 0.0, 0.0, 0.0};
    std::array<double, 4> cameraOrientation = {1.0, 0.0, 0.0, 0.0};
    std::string activeTheory = "GR";
    bool showLightCones = true;
    float lightConeAngle = 45.0f;
    int lightConeResolution = 32;
};

/**
 * @brief Secure file I/O manager
 */
class FileIO {
public:
    FileIO();
    ~FileIO() = default;

    /**
     * @brief Load a spacetime configuration file
     * @param path Path to the .spc or .json file
     * @param[out] config Loaded configuration
     * @return FileResult indicating success or error
     */
    FileResult loadSpacetimeConfig(const std::string& path, SpacetimeConfig& config);

    /**
     * @brief Save a spacetime configuration file
     * @param path Path to save the .spc or .json file
     * @param config Configuration to save
     * @return FileResult indicating success or error
     */
    FileResult saveSpacetimeConfig(const std::string& path, const SpacetimeConfig& config);

    /**
     * @brief Get the user's data directory
     */
    static std::filesystem::path getUserDataDir();

    /**
     * @brief Get the user's documents directory
     */
    static std::filesystem::path getUserDocumentsDir();

private:
    // Security validation
    bool validatePath(const std::string& path) const;
    bool validateConfig(const SpacetimeConfig& config) const;

    // Path to user data directory
    std::filesystem::path m_dataDir;
};

} // namespace utils
} // namespace quantumverse