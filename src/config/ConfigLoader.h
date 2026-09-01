/**
 * @file ConfigLoader.h
 * @brief JSON configuration loader for QuantumVerse simulator
 *
 * Loads solar system parameters, black hole properties, and rendering
 * settings from JSON config files at runtime.
 */

#ifndef QUANTUMVERSE_CONFIG_LOADER_H
#define QUANTUMVERSE_CONFIG_LOADER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <stdexcept>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

namespace quantumverse {

using json = nlohmann::json;

struct BlackHoleConfig {
    double mass_solar_masses = 10.0;
    double position[3] = {0.0, 0.0, 0.0};
    std::string type = "schwarzschild";
    double spin = 0.0;
    double charge = 0.0;
};

struct BodyConfig {
    std::string name;
    double mass_kg = 0.0;
    double radius_m = 0.0;
    double semi_major_axis_au = 0.0;
    double orbital_period_days = 0.0;
    std::string texture_type = "barren";
    int texture_layer = -1;
    double color[3] = {0.5, 0.5, 0.5};
    bool is_star = false;
    bool is_central_body = false;
};

struct GridConfig {
    int resolution = 30;
    float size = 100.0f;
    float deformation_scale = 0.01f;
    float max_displacement = 5.0f;
};

struct CameraConfig {
    double distance = 500.0;
    double azimuth = 45.0;
    double elevation = 30.0;
    double fov = 60.0;
};

struct PhysicsConstantsConfig {
    double G = 6.67430e-11;
    double c = 299792458.0;
    double h = 6.62607015e-34;
};

struct SimulatorConfig {
    BlackHoleConfig black_hole;
    BodyConfig central_body;
    std::vector<BodyConfig> bodies;
    GridConfig grid;
    CameraConfig camera;
    PhysicsConstantsConfig constants;
};

class ConfigLoader {
public:
    static ConfigLoader& instance() {
        static ConfigLoader inst;
        return inst;
    }

    bool loadFromFile(const std::string& path) {
        try {
            std::ifstream file(path);
            if (!file.is_open()) {
                std::cerr << "[ConfigLoader] Failed to open: " << path << std::endl;
                return false;
            }

            json j;
            file >> j;
            file.close();

            parseConfig(j);
            config_loaded_ = true;
            config_path_ = path;

            std::cout << "[ConfigLoader] Loaded config from: " << path << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "[ConfigLoader] Parse error: " << e.what() << std::endl;
            return false;
        }
    }

    bool loadDefaults() {
        json j;
        j["black_hole"]["mass_solar_masses"] = 10.0;
        j["black_hole"]["type"] = "schwarzschild";
        j["solar_system"]["central_body"]["name"] = "Sun";
        j["solar_system"]["central_body"]["mass_kg"] = 1.989e30;
        j["solar_system"]["central_body"]["radius_m"] = 696340000.0;
        j["rendering"]["grid"]["resolution"] = 30;
        j["rendering"]["grid"]["size"] = 100.0;
        j["rendering"]["camera"]["distance"] = 500.0;
        j["rendering"]["camera"]["azimuth"] = 45.0;
        j["rendering"]["camera"]["elevation"] = 30.0;

        parseConfig(j);
        config_loaded_ = true;
        return true;
    }

    const SimulatorConfig& config() const { return config_; }
    SimulatorConfig& config() { return config_; }
    bool isLoaded() const { return config_loaded_; }
    const std::string& path() const { return config_path_; }

    void reload() {
        if (!config_path_.empty()) {
            loadFromFile(config_path_);
        }
    }

private:
    ConfigLoader() = default;
    SimulatorConfig config_;
    bool config_loaded_ = false;
    std::string config_path_;

    void parseConfig(const json& j) {
        if (j.contains("black_hole")) {
            auto& bh = j["black_hole"];
            config_.black_hole.mass_solar_masses = bh.value("mass_solar_masses", 10.0);
            config_.black_hole.type = bh.value("type", "schwarzschild");
            config_.black_hole.spin = bh.value("spin", 0.0);
            config_.black_hole.charge = bh.value("charge", 0.0);
            if (bh.contains("position") && bh["position"].is_array() && bh["position"].size() == 3) {
                for (int i = 0; i < 3; i++)
                    config_.black_hole.position[i] = bh["position"][i].get<double>();
            }
        }

        if (j.contains("solar_system")) {
            auto& ss = j["solar_system"];
            if (ss.contains("central_body")) {
                parseBody(ss["central_body"], config_.central_body);
            }
            if (ss.contains("bodies") && ss["bodies"].is_array()) {
                config_.bodies.clear();
                for (auto& b : ss["bodies"]) {
                    BodyConfig body;
                    parseBody(b, body);
                    config_.bodies.push_back(body);
                }
            }
        }

        if (j.contains("rendering")) {
            auto& r = j["rendering"];
            if (r.contains("grid")) {
                auto& g = r["grid"];
                config_.grid.resolution = g.value("resolution", 30);
                config_.grid.size = g.value("size", 100.0f);
                config_.grid.deformation_scale = g.value("deformation_scale", 0.01f);
                config_.grid.max_displacement = g.value("max_displacement", 5.0f);
            }
            if (r.contains("camera")) {
                auto& c = r["camera"];
                config_.camera.distance = c.value("distance", 500.0);
                config_.camera.azimuth = c.value("azimuth", 45.0);
                config_.camera.elevation = c.value("elevation", 30.0);
                config_.camera.fov = c.value("fov", 60.0);
            }
        }

        if (j.contains("physics_constants")) {
            auto& pc = j["physics_constants"];
            config_.constants.G = pc.value("G", 6.67430e-11);
            config_.constants.c = pc.value("c", 299792458.0);
            config_.constants.h = pc.value("h", 6.62607015e-34);
        }
    }

    void parseBody(const json& j, BodyConfig& body) {
        body.name = j.value("name", "Unknown");
        body.mass_kg = j.value("mass_kg", 0.0);
        body.radius_m = j.value("radius_m", 0.0);
        body.semi_major_axis_au = j.value("semi_major_axis_au", 0.0);
        body.orbital_period_days = j.value("orbital_period_days", 0.0);
        body.texture_type = j.value("texture_type", "barren");
        body.texture_layer = j.value("texture_layer", -1);
        body.is_star = j.value("is_star", false);
        body.is_central_body = j.value("is_central_body", false);
        if (j.contains("color") && j["color"].is_array() && j["color"].size() == 3) {
            for (int i = 0; i < 3; i++)
                body.color[i] = j["color"][i].get<double>();
        }
    }
};

} // namespace quantumverse

#endif // QUANTUMVERSE_CONFIG_LOADER_H
