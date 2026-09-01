/**
 * @file Scenario.h
 * @brief Scriptable scenario system for QuantumVerse
 *
 * Defines experiment scripts as JSON: initial conditions, camera paths,
 * timed events, and audio settings. Scenarios can be loaded and run
 * with one click from the QML interface.
 */

#ifndef QUANTUMVERSE_SCENARIO_H
#define QUANTUMVERSE_SCENARIO_H

#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <fstream>

#include <nlohmann/json.hpp>

namespace quantumverse {

using json = nlohmann::json;

// ============================================================================
// Scenario Event Types
// ============================================================================

enum class ScenarioEventType : int {
    INJECT_ASTEROID = 0,
    SET_CAMERA = 1,
    SET_CONSTANT = 2,
    ADD_BODY = 3,
    REMOVE_BODY = 4,
    SET_TIME_SCALE = 5,
    AUDIO_SETTINGS = 6,
    WAIT = 7,
    APPLY_FORCE = 8,
    SET_CENTRAL_BODY = 9
};

inline std::string eventTypeToString(ScenarioEventType type) {
    switch (type) {
        case ScenarioEventType::INJECT_ASTEROID: return "INJECT_ASTEROID";
        case ScenarioEventType::SET_CAMERA: return "SET_CAMERA";
        case ScenarioEventType::SET_CONSTANT: return "SET_CONSTANT";
        case ScenarioEventType::ADD_BODY: return "ADD_BODY";
        case ScenarioEventType::REMOVE_BODY: return "REMOVE_BODY";
        case ScenarioEventType::SET_TIME_SCALE: return "SET_TIME_SCALE";
        case ScenarioEventType::AUDIO_SETTINGS: return "AUDIO_SETTINGS";
        case ScenarioEventType::WAIT: return "WAIT";
        case ScenarioEventType::APPLY_FORCE: return "APPLY_FORCE";
        case ScenarioEventType::SET_CENTRAL_BODY: return "SET_CENTRAL_BODY";
        default: return "UNKNOWN";
    }
}

inline ScenarioEventType stringToEventType(const std::string& str) {
    if (str == "INJECT_ASTEROID") return ScenarioEventType::INJECT_ASTEROID;
    if (str == "SET_CAMERA") return ScenarioEventType::SET_CAMERA;
    if (str == "SET_CONSTANT") return ScenarioEventType::SET_CONSTANT;
    if (str == "ADD_BODY") return ScenarioEventType::ADD_BODY;
    if (str == "REMOVE_BODY") return ScenarioEventType::REMOVE_BODY;
    if (str == "SET_TIME_SCALE") return ScenarioEventType::SET_TIME_SCALE;
    if (str == "AUDIO_SETTINGS") return ScenarioEventType::AUDIO_SETTINGS;
    if (str == "WAIT") return ScenarioEventType::WAIT;
    if (str == "APPLY_FORCE") return ScenarioEventType::APPLY_FORCE;
    if (str == "SET_CENTRAL_BODY") return ScenarioEventType::SET_CENTRAL_BODY;
    return ScenarioEventType::WAIT;
}

// ============================================================================
// Scenario Data Structures
// ============================================================================

struct ScenarioBodyConfig {
    std::string name;
    double mass_kg = 0.0;
    double radius_m = 0.0;
    double position[3] = {0.0, 0.0, 0.0};
    double velocity[3] = {0.0, 0.0, 0.0};
    double color[3] = {0.5, 0.5, 0.5};
    bool is_star = false;
    bool is_central_body = false;
    double semi_major_axis_au = 0.0;
    double orbital_period_days = 0.0;
};

struct ScenarioCameraKeyframe {
    double time = 0.0;
    double position[3] = {0.0, 0.0, 500.0};
    double target[3] = {0.0, 0.0, 0.0};
    double fov = 60.0;
};

struct ScenarioEvent {
    double time = 0.0;
    ScenarioEventType type = ScenarioEventType::WAIT;
    std::string target_body;
    double params[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    std::string str_param;

    bool operator<(const ScenarioEvent& other) const {
        return time < other.time;
    }
};

struct ScenarioAudioConfig {
    bool enabled = false;
    double volume = 0.5;
    double base_frequency = 100.0;
    double frequency_scale = 1.0;
};

struct ScenarioCentralBody {
    std::string name = "Sun";
    double mass_kg = 1.989e30;
    double radius_m = 696340000.0;
    double color[3] = {1.0, 0.95, 0.6};
    bool is_star = true;
};

struct ScenarioPhysicsConstants {
    double G = 6.67430e-11;
    double c = 299792458.0;
    double h = 6.62607015e-34;
    bool override_constants = false;
};

struct Scenario {
    std::string name;
    std::string description;
    std::string version = "1.0";
    double duration = 60.0;
    double time_scale = 1.0;

    ScenarioCentralBody central_body;
    ScenarioPhysicsConstants physics;
    std::vector<ScenarioBodyConfig> bodies;
    std::vector<ScenarioEvent> events;
    std::vector<ScenarioCameraKeyframe> camera_path;
    ScenarioAudioConfig audio;

    void sortEvents() {
        std::sort(events.begin(), events.end());
    }

    bool saveToFile(const std::string& filepath) const;
    bool loadFromFile(const std::string& filepath);
    bool validate(std::string* error_msg = nullptr) const;
    std::vector<ScenarioEvent> getEventsInRange(double start_time, double end_time) const;
    std::vector<ScenarioEvent> getEventsOfType(ScenarioEventType type) const;
};

// ============================================================================
// JSON Serialization
// ============================================================================

inline void to_json(json& j, const ScenarioBodyConfig& b) {
    j = json{
        {"name", b.name},
        {"mass_kg", b.mass_kg},
        {"radius_m", b.radius_m},
        {"position", {b.position[0], b.position[1], b.position[2]}},
        {"velocity", {b.velocity[0], b.velocity[1], b.velocity[2]}},
        {"color", {b.color[0], b.color[1], b.color[2]}},
        {"is_star", b.is_star},
        {"is_central_body", b.is_central_body},
        {"semi_major_axis_au", b.semi_major_axis_au},
        {"orbital_period_days", b.orbital_period_days}
    };
}

inline void from_json(const json& j, ScenarioBodyConfig& b) {
    b.name = j.value("name", "Unknown");
    b.mass_kg = j.value("mass_kg", 0.0);
    b.radius_m = j.value("radius_m", 0.0);
    b.is_star = j.value("is_star", false);
    b.is_central_body = j.value("is_central_body", false);
    b.semi_major_axis_au = j.value("semi_major_axis_au", 0.0);
    b.orbital_period_days = j.value("orbital_period_days", 0.0);

    if (j.contains("position") && j["position"].is_array() && j["position"].size() == 3) {
        for (int i = 0; i < 3; i++) b.position[i] = j["position"][i].get<double>();
    }
    if (j.contains("velocity") && j["velocity"].is_array() && j["velocity"].size() == 3) {
        for (int i = 0; i < 3; i++) b.velocity[i] = j["velocity"][i].get<double>();
    }
    if (j.contains("color") && j["color"].is_array() && j["color"].size() == 3) {
        for (int i = 0; i < 3; i++) b.color[i] = j["color"][i].get<double>();
    }
}

inline void to_json(json& j, const ScenarioCameraKeyframe& k) {
    j = json{
        {"time", k.time},
        {"position", {k.position[0], k.position[1], k.position[2]}},
        {"target", {k.target[0], k.target[1], k.target[2]}},
        {"fov", k.fov}
    };
}

inline void from_json(const json& j, ScenarioCameraKeyframe& k) {
    k.time = j.value("time", 0.0);
    k.fov = j.value("fov", 60.0);
    if (j.contains("position") && j["position"].is_array() && j["position"].size() == 3) {
        for (int i = 0; i < 3; i++) k.position[i] = j["position"][i].get<double>();
    }
    if (j.contains("target") && j["target"].is_array() && j["target"].size() == 3) {
        for (int i = 0; i < 3; i++) k.target[i] = j["target"][i].get<double>();
    }
}

inline void to_json(json& j, const ScenarioEvent& e) {
    j = json{
        {"time", e.time},
        {"type", eventTypeToString(e.type)},
        {"target_body", e.target_body},
        {"params", {e.params[0], e.params[1], e.params[2], e.params[3],
                     e.params[4], e.params[5], e.params[6], e.params[7]}},
        {"str_param", e.str_param}
    };
}

inline void from_json(const json& j, ScenarioEvent& e) {
    e.time = j.value("time", 0.0);
    e.type = stringToEventType(j.value("type", "WAIT"));
    e.target_body = j.value("target_body", "");
    e.str_param = j.value("str_param", "");
    if (j.contains("params") && j["params"].is_array()) {
        for (size_t i = 0; i < 8 && i < j["params"].size(); i++) {
            e.params[i] = j["params"][i].get<double>();
        }
    }
}

inline void to_json(json& j, const ScenarioAudioConfig& a) {
    j = json{
        {"enabled", a.enabled},
        {"volume", a.volume},
        {"base_frequency", a.base_frequency},
        {"frequency_scale", a.frequency_scale}
    };
}

inline void from_json(const json& j, ScenarioAudioConfig& a) {
    a.enabled = j.value("enabled", false);
    a.volume = j.value("volume", 0.5);
    a.base_frequency = j.value("base_frequency", 100.0);
    a.frequency_scale = j.value("frequency_scale", 1.0);
}

inline void to_json(json& j, const ScenarioCentralBody& cb) {
    j = json{
        {"name", cb.name},
        {"mass_kg", cb.mass_kg},
        {"radius_m", cb.radius_m},
        {"color", {cb.color[0], cb.color[1], cb.color[2]}},
        {"is_star", cb.is_star}
    };
}

inline void from_json(const json& j, ScenarioCentralBody& cb) {
    cb.name = j.value("name", "Sun");
    cb.mass_kg = j.value("mass_kg", 1.989e30);
    cb.radius_m = j.value("radius_m", 696340000.0);
    cb.is_star = j.value("is_star", true);
    if (j.contains("color") && j["color"].is_array() && j["color"].size() == 3) {
        for (int i = 0; i < 3; i++) cb.color[i] = j["color"][i].get<double>();
    }
}

inline void to_json(json& j, const ScenarioPhysicsConstants& pc) {
    j = json{
        {"G", pc.G},
        {"c", pc.c},
        {"h", pc.h},
        {"override_constants", pc.override_constants}
    };
}

inline void from_json(const json& j, ScenarioPhysicsConstants& pc) {
    pc.G = j.value("G", 6.67430e-11);
    pc.c = j.value("c", 299792458.0);
    pc.h = j.value("h", 6.62607015e-34);
    pc.override_constants = j.value("override_constants", false);
}

inline void to_json(json& j, const Scenario& s) {
    j = json{
        {"name", s.name},
        {"description", s.description},
        {"version", s.version},
        {"duration", s.duration},
        {"time_scale", s.time_scale},
        {"central_body", s.central_body},
        {"physics", s.physics},
        {"bodies", s.bodies},
        {"events", s.events},
        {"camera_path", s.camera_path},
        {"audio", s.audio}
    };
}

inline void from_json(const json& j, Scenario& s) {
    s.name = j.value("name", "Untitled Scenario");
    s.description = j.value("description", "");
    s.version = j.value("version", "1.0");
    s.duration = j.value("duration", 60.0);
    s.time_scale = j.value("time_scale", 1.0);

    if (j.contains("central_body")) {
        s.central_body = j["central_body"].get<ScenarioCentralBody>();
    }
    if (j.contains("physics")) {
        s.physics = j["physics"].get<ScenarioPhysicsConstants>();
    }
    if (j.contains("bodies") && j["bodies"].is_array()) {
        s.bodies = j["bodies"].get<std::vector<ScenarioBodyConfig>>();
    }
    if (j.contains("events") && j["events"].is_array()) {
        s.events = j["events"].get<std::vector<ScenarioEvent>>();
    }
    if (j.contains("camera_path") && j["camera_path"].is_array()) {
        s.camera_path = j["camera_path"].get<std::vector<ScenarioCameraKeyframe>>();
    }
    if (j.contains("audio")) {
        s.audio = j["audio"].get<ScenarioAudioConfig>();
    }

    s.sortEvents();
}

// ============================================================================
// Camera Interpolation
// ============================================================================

struct ScenarioCameraState {
    double position[3];
    double target[3];
    double fov;
};

inline ScenarioCameraState interpolateCamera(const std::vector<ScenarioCameraKeyframe>& keyframes, double time) {
    ScenarioCameraState state{{0.0, 0.0, 500.0}, {0.0, 0.0, 0.0}, 60.0};

    if (keyframes.empty()) return state;
    if (keyframes.size() == 1) {
        for (int i = 0; i < 3; i++) {
            state.position[i] = keyframes[0].position[i];
            state.target[i] = keyframes[0].target[i];
        }
        state.fov = keyframes[0].fov;
        return state;
    }

    if (time <= keyframes.front().time) {
        for (int i = 0; i < 3; i++) {
            state.position[i] = keyframes.front().position[i];
            state.target[i] = keyframes.front().target[i];
        }
        state.fov = keyframes.front().fov;
        return state;
    }
    if (time >= keyframes.back().time) {
        for (int i = 0; i < 3; i++) {
            state.position[i] = keyframes.back().position[i];
            state.target[i] = keyframes.back().target[i];
        }
        state.fov = keyframes.back().fov;
        return state;
    }

    for (size_t i = 0; i < keyframes.size() - 1; i++) {
        if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
            double dt = keyframes[i + 1].time - keyframes[i].time;
            double t = (dt > 0.0) ? (time - keyframes[i].time) / dt : 0.0;
            t = std::clamp(t, 0.0, 1.0);

            double smooth = t * t * (3.0 - 2.0 * t);

            for (int j = 0; j < 3; j++) {
                state.position[j] = keyframes[i].position[j] +
                    (keyframes[i + 1].position[j] - keyframes[i].position[j]) * smooth;
                state.target[j] = keyframes[i].target[j] +
                    (keyframes[i + 1].target[j] - keyframes[i].target[j]) * smooth;
            }
            state.fov = keyframes[i].fov +
                (keyframes[i + 1].fov - keyframes[i].fov) * smooth;
            return state;
        }
    }

    return state;
}

} // namespace quantumverse

#endif // QUANTUMVERSE_SCENARIO_H
