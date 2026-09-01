/**
 * @file ScenarioManager.cpp
 * @brief ScenarioManager implementation
 */

#include "ScenarioManager.h"
#include <filesystem>
#include <algorithm>
#include <iostream>

namespace quantumverse {

namespace fs = std::filesystem;

// ============================================================================
// Initialization
// ============================================================================

bool ScenarioManager::initialize(const std::string& scenario_dir) {
    std::lock_guard<std::mutex> lock(mutex_);
    scenario_dir_ = scenario_dir;

    if (!fs::exists(scenario_dir_)) {
        std::cerr << "[ScenarioManager] Directory not found: " << scenario_dir_ << std::endl;
        std::cerr << "[ScenarioManager] Attempting to create: " << scenario_dir_ << std::endl;
        try {
            fs::create_directories(scenario_dir_);
        } catch (const std::exception& e) {
            std::cerr << "[ScenarioManager] Failed to create directory: " << e.what() << std::endl;
            return false;
        }
    }

    scenarios_.clear();
    name_to_filename_.clear();
    return doLoadAllScenarios();
}

// ============================================================================
// Listing
// ============================================================================

std::vector<std::string> ScenarioManager::listScenarios() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    for (const auto& pair : name_to_filename_) {
        names.push_back(pair.first);
    }
    std::sort(names.begin(), names.end());
    return names;
}

std::vector<std::string> ScenarioManager::listScenarioNames() const {
    return listScenarios();
}

// ============================================================================
// Access
// ============================================================================

bool ScenarioManager::hasScenario(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return name_to_filename_.find(name) != name_to_filename_.end();
}

std::shared_ptr<Scenario> ScenarioManager::getScenario(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = name_to_filename_.find(name);
    if (it == name_to_filename_.end()) return nullptr;

    auto sit = scenarios_.find(it->second);
    if (sit != scenarios_.end()) return sit->second;

    return nullptr;
}

std::shared_ptr<Scenario> ScenarioManager::getScenarioByFilename(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = scenarios_.find(filename);
    if (it != scenarios_.end()) return it->second;

    std::string full_path = scenario_dir_ + "/" + filename;
    auto scenario = std::make_shared<Scenario>();
    if (!scenario->loadFromFile(full_path)) {
        return nullptr;
    }

    scenarios_[filename] = scenario;
    name_to_filename_[scenario->name] = filename;
    return scenario;
}

// ============================================================================
// Loading
// ============================================================================

bool ScenarioManager::loadScenario(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string full_path = scenario_dir_ + "/" + filename;

    auto scenario = std::make_shared<Scenario>();
    if (!scenario->loadFromFile(full_path)) {
        std::cerr << "[ScenarioManager] Failed to load: " << filename << std::endl;
        return false;
    }

    std::string error;
    if (!scenario->validate(&error)) {
        std::cerr << "[ScenarioManager] Validation failed for " << filename
                  << ": " << error << std::endl;
        return false;
    }

    scenarios_[filename] = scenario;
    name_to_filename_[scenario->name] = filename;

    std::cout << "[ScenarioManager] Loaded: " << scenario->name
              << " from " << filename << std::endl;
    return true;
}

bool ScenarioManager::loadAllScenarios() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!fs::exists(scenario_dir_)) {
        std::cerr << "[ScenarioManager] Directory not found: " << scenario_dir_ << std::endl;
        return false;
    }

    scenarios_.clear();
    name_to_filename_.clear();
    return doLoadAllScenarios();
}

bool ScenarioManager::doLoadAllScenarios() {
    if (!fs::exists(scenario_dir_)) {
        return false;
    }

    int loaded = 0;

    for (const auto& entry : fs::directory_iterator(scenario_dir_)) {
        if (!entry.is_regular_file()) continue;
        auto ext = entry.path().extension().string();
        if (ext != ".json") continue;

        std::string filename = entry.path().filename().string();
        std::string full_path = scenario_dir_ + "/" + filename;

        auto scenario = std::make_shared<Scenario>();
        if (!scenario->loadFromFile(full_path)) {
            std::cerr << "[ScenarioManager] Skipping: " << filename << std::endl;
            continue;
        }

        std::string error;
        if (!scenario->validate(&error)) {
            std::cerr << "[ScenarioManager] Validation failed for " << filename
                      << ": " << error << std::endl;
            continue;
        }

        scenarios_[filename] = scenario;
        name_to_filename_[scenario->name] = filename;
        loaded++;
    }

    std::cout << "[ScenarioManager] Loaded " << loaded << " scenarios from "
              << scenario_dir_ << std::endl;
    return loaded > 0;
}

// ============================================================================
// Saving
// ============================================================================

bool ScenarioManager::saveScenario(const Scenario& scenario, const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string full_path = scenario_dir_ + "/" + filename;

    if (!scenario.saveToFile(full_path)) {
        return false;
    }

    auto copy = std::make_shared<Scenario>(scenario);
    scenarios_[filename] = copy;
    name_to_filename_[scenario.name] = filename;
    return true;
}

// ============================================================================
// Reload / Clear
// ============================================================================

void ScenarioManager::reload() {
    std::cout << "[ScenarioManager] Reloading all scenarios..." << std::endl;
    loadAllScenarios();
}

void ScenarioManager::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    scenarios_.clear();
    name_to_filename_.clear();
}

} // namespace quantumverse
