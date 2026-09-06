/**
 * @file TrainingDataCollector.cpp
 * @brief Training data collector implementation
 */

#include "ml/TrainingDataCollector.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <cmath>
#include <algorithm>

namespace quantumverse {

TrainingDataCollector::TrainingDataCollector() = default;

TrainingDataCollector::~TrainingDataCollector() {
    stop();
    if (config_.autoSave && !samples_.empty()) {
        save();
    }
}

void TrainingDataCollector::configure(const CollectorConfig& cfg) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = cfg;
}

void TrainingDataCollector::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    active_ = true;
    lastSampleTime_ = 0.0;
}

void TrainingDataCollector::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    active_ = false;
}

void TrainingDataCollector::sample(
    const UI4D::PhysicsTelemetry& telemetry,
    double blackHoleMassKg,
    const std::vector<UI4D::SolarSystemBody>& bodies,
    const std::vector<InstrumentFinding>& findings,
    double simulationTime,
    const std::string& currentScenarioName
) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active_) return;

    if (samples_.empty()) {
        lastSampleTime_ = simulationTime;
    }

    if (simulationTime - lastSampleTime_ < config_.sampleInterval && !samples_.empty()) {
        return;
    }
    lastSampleTime_ = simulationTime;

    TrainingSample sample;
    sample.features = FeatureVector::extract(telemetry, blackHoleMassKg, bodies, findings);
    sample.timestamp = simulationTime;
    sample.scenarioName = currentScenarioName;
    sample.description = "";

    bool autoAnomaly = false;
    if (config_.labelFromScenarioEvents && !currentScenarioName.empty()) {
        for (const auto& f : findings) {
            if (f.isAnomaly && f.confidence > 0.8) {
                autoAnomaly = true;
                sample.labelSource = "instrument";
                sample.description = f.description;
                break;
            }
        }
    }

    if (manualAnomalyNext_) {
        sample.isAnomalous = true;
        sample.labelSource = "manual";
        sample.description = manualAnomalyDesc_;
        manualAnomalyNext_ = false;
        manualAnomalyDesc_.clear();
    } else if (autoAnomaly) {
        sample.isAnomalous = true;
    } else {
        sample.isAnomalous = false;
        sample.labelSource = currentScenarioName.empty() ? "unlabeled" : "scenario";
    }

    samples_.push_back(std::move(sample));

    if (samples_.size() >= static_cast<std::size_t>(config_.maxSamples)) {
        active_ = false;
    }

    flushIfNeeded();
}

void TrainingDataCollector::markNextAnomalous(const std::string& description) {
    std::lock_guard<std::mutex> lock(mutex_);
    manualAnomalyNext_ = true;
    manualAnomalyDesc_ = description;
}

std::vector<TrainingSample> TrainingDataCollector::getSamples() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return samples_;
}

std::size_t TrainingDataCollector::count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return samples_.size();
}

void TrainingDataCollector::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    samples_.clear();
}

bool TrainingDataCollector::save(const std::string& path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return writeJson(path.empty() ? config_.outputPath : path);
}

bool TrainingDataCollector::load(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    return readJson(path);
}

void TrainingDataCollector::flushIfNeeded() {
    if (!config_.autoSave) return;
    if (samples_.size() % static_cast<std::size_t>(config_.autoSaveInterval) == 0) {
        writeJson(config_.outputPath);
    }
}

bool TrainingDataCollector::writeJson(const std::string& path) const {
    try {
        std::filesystem::create_directories(std::filesystem::path(path).parent_path());
        nlohmann::json j;
        j["config"] = {
            {"sampleInterval", config_.sampleInterval},
            {"maxSamples", config_.maxSamples}
        };
        j["totalSamples"] = samples_.size();
        j["samples"] = nlohmann::json::array();

        for (const auto& s : samples_) {
            nlohmann::json sample;
            sample["timestamp"] = s.timestamp;
            sample["scenarioName"] = s.scenarioName;
            sample["isAnomalous"] = s.isAnomalous;
            sample["labelSource"] = s.labelSource;
            sample["description"] = s.description;
            sample["features"] = s.features.toVector();
            j["samples"].push_back(std::move(sample));
        }

        std::ofstream f(path);
        if (!f.is_open()) return false;
        f << j.dump(2);
        return true;
    } catch (...) {
        return false;
    }
}

bool TrainingDataCollector::readJson(const std::string& path) {
    try {
        std::ifstream f(path);
        if (!f.is_open()) return false;
        nlohmann::json j;
        f >> j;

        samples_.clear();
        if (!j.contains("samples") || !j["samples"].is_array()) return false;

        for (const auto& elem : j["samples"]) {
            TrainingSample s;
            s.timestamp = elem.value("timestamp", 0.0);
            s.scenarioName = elem.value("scenarioName", std::string{});
            s.isAnomalous = elem.value("isAnomalous", false);
            s.labelSource = elem.value("labelSource", std::string{});
            s.description = elem.value("description", std::string{});

            if (elem.contains("features") && elem["features"].is_array()) {
                s.features = FeatureVector::fromVector(elem["features"].get<std::vector<double>>());
            }

            samples_.push_back(std::move(s));
        }

        if (j.contains("config")) {
            config_.sampleInterval = j["config"].value("sampleInterval", config_.sampleInterval);
            config_.maxSamples = j["config"].value("maxSamples", config_.maxSamples);
        }

        return true;
    } catch (...) {
        return false;
    }
}

} // namespace quantumverse
