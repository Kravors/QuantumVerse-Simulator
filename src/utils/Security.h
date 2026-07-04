/**
 * @file Security.h
 * @brief Security utilities for QuantumVerse Simulator
 *
 * Provides input validation, path sanitization, and security utilities
 * for file I/O and user input handling.
 */

#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <filesystem>

namespace quantumverse {
namespace utils {

/**
 * @brief Security validation utilities
 */
class Security {
public:
    /**
     * @brief Validate a file path for security
     * @param path The path to validate
     * @return The validated absolute path
     * @throws std::invalid_argument if path is invalid or unsafe
     */
    static std::filesystem::path validatePath(const std::string& path);

    /**
     * @brief Sanitize a string input
     * @param input The input string to sanitize
     * @param maxLength Maximum allowed length (0 for no limit)
     * @return The sanitized string
     */
    static std::string sanitizeString(const std::string& input, size_t maxLength = 0);

    /**
     * @brief Validate a physics parameter (mass)
     * @param mass The mass value in kg
     * @return true if valid
     */
    static bool validateMass(double mass);

    /**
     * @brief Validate a spin parameter
     * @param spin The spin value (0-1)
     * @return true if valid
     */
    static bool validateSpin(double spin);

    /**
     * @brief Check if a file extension is allowed
     * @param path The file path
     * @return true if extension is in allowed list
     */
    static bool isAllowedExtension(const std::string& path);

    /**
     * @brief Get allowed file extensions
     */
    static const std::vector<std::string>& getAllowedExtensions();

private:
    static const std::vector<std::string> s_allowedExtensions;
};

} // namespace utils
} // namespace quantumverse