/**
 * @file Scenario.cpp
 * @brief Scenario JSON parsing and camera interpolation implementation
 */

#include "Scenario.h"

namespace quantumverse {

// ============================================================================
// Scenario I/O
// ============================================================================

bool Scenario::saveToFile(const std::string& filepath) const {
    try {
        json j = *this;
        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[Scenario] Failed to open for writing: " << filepath << std::endl;
            return false;
        }
        file << j.dump(2);
        file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[Scenario] Save error: " << e.what() << std::endl;
        return false;
    }
}

bool Scenario::loadFromFile(const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[Scenario] Failed to open: " << filepath << std::endl;
            return false;
        }
        json j;
        file >> j;
        file.close();

        *this = j.get<Scenario>();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[Scenario] Load error: " << e.what() << std::endl;
        return false;
    }
}

// ============================================================================
// Scenario Validation
// ============================================================================

bool Scenario::validate(std::string* error_msg) const {
    if (name.empty()) {
        if (error_msg) *error_msg = "Scenario name is empty";
        return false;
    }
    if (duration <= 0.0) {
        if (error_msg) *error_msg = "Duration must be positive";
        return false;
    }
    if (time_scale <= 0.0) {
        if (error_msg) *error_msg = "Time scale must be positive";
        return false;
    }
    if (central_body.mass_kg <= 0.0) {
        if (error_msg) *error_msg = "Central body mass must be positive";
        return false;
    }

    for (size_t i = 0; i < events.size(); i++) {
        if (events[i].time < 0.0 || events[i].time > duration) {
            if (error_msg) *error_msg = "Event " + std::to_string(i) + " time out of range";
            return false;
        }
    }

    for (size_t i = 0; i < camera_path.size(); i++) {
        if (camera_path[i].time < 0.0 || camera_path[i].time > duration) {
            if (error_msg) *error_msg = "Camera keyframe " + std::to_string(i) + " time out of range";
            return false;
        }
    }

    if (error_msg) *error_msg = "";
    return true;
}

// ============================================================================
// Event Query
// ============================================================================

std::vector<ScenarioEvent> Scenario::getEventsInRange(double start_time, double end_time) const {
    std::vector<ScenarioEvent> result;
    for (const auto& event : events) {
        if (event.time >= start_time && event.time <= end_time) {
            result.push_back(event);
        }
    }
    return result;
}

std::vector<ScenarioEvent> Scenario::getEventsOfType(ScenarioEventType type) const {
    std::vector<ScenarioEvent> result;
    for (const auto& event : events) {
        if (event.type == type) {
            result.push_back(event);
        }
    }
    return result;
}

} // namespace quantumverse
