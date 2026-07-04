/**
 * @file Security.cpp
 * @brief Implementation of security utilities
 */

#include "Security.h"
#include <algorithm>
#include <cctype>

namespace quantumverse {
namespace utils {

// Allowed file extensions for loading/saving
const std::vector<std::string> Security::s_allowedExtensions = {
    ".spc", ".json", ".ini", ".txt", ".csv", ".dat"
};

std::filesystem::path Security::validatePath(const std::string& path) {
    // Convert to absolute path
    std::filesystem::path p = std::filesystem::absolute(path);
    
    // Check for path traversal attempts
    std::string pathStr = p.string();
    if (pathStr.find("..") != std::string::npos) {
        throw std::invalid_argument("Path traversal detected: " + path);
    }
    
    // Check for null bytes (potential security issue)
    if (pathStr.find('\0') != std::string::npos) {
        throw std::invalid_argument("Null byte in path detected: " + path);
    }
    
    // Check file extension
    if (!isAllowedExtension(path)) {
        throw std::invalid_argument("File extension not allowed: " + path);
    }
    
    return p;
}

std::string Security::sanitizeString(const std::string& input, size_t maxLength) {
    std::string result;
    result.reserve(input.length());
    
    // Copy only safe characters (alphanumeric, space, and common punctuation)
    for (char c : input) {
        if (std::isalnum(static_cast<unsigned char>(c)) || 
            c == ' ' || c == '-' || c == '_' || c == '.' || 
            c == '/' || c == '\\' || c == ':' || c == ',') {
            result += c;
        }
    }
    
    // Truncate if max length specified
    if (maxLength > 0 && result.length() > maxLength) {
        result = result.substr(0, maxLength);
    }
    
    return result;
}

bool Security::validateMass(double mass) {
    // Mass must be positive and within reasonable bounds
    // 0 < mass < 1e50 kg (approximately the mass of the observable universe)
    return mass > 0.0 && mass < 1e50;
}

bool Security::validateSpin(double spin) {
    // Spin must be in range [0, 1) for Kerr black holes
    return spin >= 0.0 && spin < 1.0;
}

bool Security::isAllowedExtension(const std::string& path) {
    std::string ext = std::filesystem::path(path).extension().string();
    // Convert to lowercase for comparison
    std::transform(ext.begin(), ext.end(), ext.begin(), 
                   [](unsigned char c){ return std::tolower(c); });
    
    for (const auto& allowed : s_allowedExtensions) {
        if (ext == allowed) {
            return true;
        }
    }
    return false;
}

const std::vector<std::string>& Security::getAllowedExtensions() {
    return s_allowedExtensions;
}

} // namespace utils
} // namespace quantumverse