/**
 * @file ScenarioManager.h
 * @brief Loads, manages, and provides access to scenario files
 *
 * Scenarios are JSON scripts stored in data/scenarios/ that define
 * complete experiments: initial conditions, camera paths, timed events.
 */

#ifndef QUANTUMVERSE_SCENARIO_MANAGER_H
#define QUANTUMVERSE_SCENARIO_MANAGER_H

#include "Scenario.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <filesystem>

namespace quantumverse {

class ScenarioManager {
public:
    static ScenarioManager& instance() {
        static ScenarioManager inst;
        return inst;
    }

    bool initialize(const std::string& scenario_dir = "data/scenarios/");

    std::vector<std::string> listScenarios() const;
    std::vector<std::string> listScenarioNames() const;

    bool hasScenario(const std::string& name) const;
    std::shared_ptr<Scenario> getScenario(const std::string& name);
    std::shared_ptr<Scenario> getScenarioByFilename(const std::string& filename);

    bool loadScenario(const std::string& filename);
    bool loadAllScenarios();

    bool saveScenario(const Scenario& scenario, const std::string& filename);

    void reload();
    void clear();

    const std::string& scenarioDir() const { return scenario_dir_; }
    size_t count() const { return scenarios_.size(); }

private:
    ScenarioManager() = default;

    bool doLoadAllScenarios();

    std::string scenario_dir_ = "data/scenarios/";
    std::unordered_map<std::string, std::shared_ptr<Scenario>> scenarios_;
    std::unordered_map<std::string, std::string> name_to_filename_;
    mutable std::mutex mutex_;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_SCENARIO_MANAGER_H
