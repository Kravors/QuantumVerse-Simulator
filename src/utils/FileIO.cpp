/**
 * @file FileIO.cpp
 * @brief Implementation of secure file I/O operations
 */

#include "FileIO.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

namespace quantumverse {
namespace utils {

FileIO::FileIO() {
    m_dataDir = getUserDataDir();
}

std::filesystem::path FileIO::getUserDataDir() {
    // Get user data directory based on platform
    #ifdef _WIN32
        const char* appData = std::getenv("APPDATA");
        if (appData) {
            return std::filesystem::path(appData) / "QuantumVerse";
        }
        return std::filesystem::path("~/.quantumverse");
    #elif __APPLE__
        const char* home = std::getenv("HOME");
        if (home) {
            return std::filesystem::path(home) / "Library" / "Application Support" / "QuantumVerse";
        }
        return std::filesystem::path("~/.quantumverse");
    #else
        const char* home = std::getenv("HOME");
        if (home) {
            return std::filesystem::path(home) / ".quantumverse";
        }
        return std::filesystem::path("~/.quantumverse");
    #endif
}

std::filesystem::path FileIO::getUserDocumentsDir() {
    #ifdef _WIN32
        const char* docs = std::getenv("USERPROFILE");
        if (docs) {
            return std::filesystem::path(docs) / "Documents";
        }
    #elif __APPLE__
        const char* home = std::getenv("HOME");
        if (home) {
            return std::filesystem::path(home) / "Documents";
        }
    #else
        const char* home = std::getenv("HOME");
        if (home) {
            return std::filesystem::path(home) / "Documents";
        }
    #endif
    return std::filesystem::path(".");
}

bool FileIO::validatePath(const std::string& path) const {
    // Check for path traversal attempts
    if (path.find("..") != std::string::npos) {
        return false;
    }

    // Check for null bytes
    if (path.find('\0') != std::string::npos) {
        return false;
    }

    // Validate file extension
    std::string ext = std::filesystem::path(path).extension().string();
    if (ext != ".spc" && ext != ".json") {
        return false;
    }

    return true;
}

bool FileIO::validateConfig(const SpacetimeConfig& config) const {
    // Validate time scale
    if (config.timeScale < 0.0 || config.timeScale > 1000.0) {
        return false;
    }

    // Validate light cone angle
    if (config.lightConeAngle < 1.0f || config.lightConeAngle > 179.0f) {
        return false;
    }

    // Validate light cone resolution
    if (config.lightConeResolution < 4 || config.lightConeResolution > 256) {
        return false;
    }

    return true;
}

FileResult FileIO::loadSpacetimeConfig(const std::string& path, SpacetimeConfig& config) {
    if (!validatePath(path)) {
        return FileResult::SecurityViolation;
    }

    std::filesystem::path fsPath = std::filesystem::absolute(path);
    
    // Check if file exists
    if (!std::filesystem::exists(fsPath)) {
        return FileResult::FileNotFound;
    }

    std::ifstream file(fsPath);
    if (!file.is_open()) {
        return FileResult::AccessDenied;
    }

    std::string ext = fsPath.extension().string();
    
    if (ext == ".spc") {
        // Parse .spc format (custom format)
        std::string line;
        while (std::getline(file, line)) {
            size_t eqPos = line.find('=');
            if (eqPos == std::string::npos) continue;

            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);

            if (key == "time") {
                config.time = std::stod(value);
            } else if (key == "timeScale") {
                config.timeScale = std::stod(value);
            } else if (key == "activeTheory") {
                config.activeTheory = value;
            } else if (key == "showLightCones") {
                config.showLightCones = (value == "true" || value == "1");
            } else if (key == "lightConeAngle") {
                config.lightConeAngle = std::stof(value);
            } else if (key == "lightConeResolution") {
                config.lightConeResolution = std::stoi(value);
            }
        }
    } else if (ext == ".json") {
        // Simple JSON parsing (in production, use a proper JSON library)
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        
        // Basic JSON parsing for known keys
        // In production, use nlohmann/json or similar
        size_t pos;
        if ((pos = content.find("\"time\"")) != std::string::npos) {
            size_t colon = content.find(':', pos);
            size_t comma = content.find(',', colon);
            config.time = std::stod(content.substr(colon + 1, comma - colon - 1));
        }
        // ... more JSON parsing would be needed
    }

    if (!validateConfig(config)) {
        return FileResult::InvalidFormat;
    }

    return FileResult::Success;
}

FileResult FileIO::saveSpacetimeConfig(const std::string& path, const SpacetimeConfig& config) {
    if (!validatePath(path)) {
        return FileResult::SecurityViolation;
    }

    if (!validateConfig(config)) {
        return FileResult::InvalidFormat;
    }

    std::filesystem::path fsPath = std::filesystem::absolute(path);
    
    // Create parent directories if needed
    std::filesystem::create_directories(fsPath.parent_path());

    std::ofstream file(fsPath);
    if (!file.is_open()) {
        return FileResult::AccessDenied;
    }

    std::string ext = fsPath.extension().string();
    
    if (ext == ".spc") {
        // Write .spc format
        file << "# QuantumVerse Spacetime Configuration\n";
        file << "time=" << config.time << "\n";
        file << "timeScale=" << config.timeScale << "\n";
        file << "activeTheory=" << config.activeTheory << "\n";
        file << "showLightCones=" << (config.showLightCones ? "true" : "false") << "\n";
        file << "lightConeAngle=" << config.lightConeAngle << "\n";
        file << "lightConeResolution=" << config.lightConeResolution << "\n";
    } else if (ext == ".json") {
        // Write JSON format
        file << "{\n";
        file << "  \"time\": " << config.time << ",\n";
        file << "  \"timeScale\": " << config.timeScale << ",\n";
        file << "  \"activeTheory\": \"" << config.activeTheory << "\",\n";
        file << "  \"showLightCones\": " << (config.showLightCones ? "true" : "false") << ",\n";
        file << "  \"lightConeAngle\": " << config.lightConeAngle << ",\n";
        file << "  \"lightConeResolution\": " << config.lightConeResolution << "\n";
        file << "}\n";
    }

    return FileResult::Success;
}

} // namespace utils
} // namespace quantumverse