/**
 * @file TrainingDataCollector.h
 * @brief Collects labeled feature vectors from live simulation runs
 *
 * Samples FeatureVector::extract() periodically and persists them to JSON
 * for offline autoencoder training. Supports scenario-based automatic labeling
 * and manual anomaly tagging.
 */

#ifndef QUANTUMVERSE_TRAINING_DATA_COLLECTOR_H
#define QUANTUMVERSE_TRAINING_DATA_COLLECTOR_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <fstream>
#include "ml/FeatureVector.h"
#include "discovery/DiscoveryInstrument.h"

namespace quantumverse {

/**
 * @brief A single labeled training sample
 */
struct TrainingSample {
    FeatureVector features;
    double timestamp = 0.0;          // Simulation time (seconds)
    std::string scenarioName;        // Active scenario, if any
    bool isAnomalous = false;        // Ground-truth label
    std::string labelSource;         // "scenario", "manual", "instrument"
    std::string description;         // Optional context
};

/**
 * @brief Collector configuration
 */
struct CollectorConfig {
    double sampleInterval = 1.0;     // Seconds between samples
    int maxSamples = 10000;          // In-memory cap before flush
    std::string outputPath = "data/ml/training_data.json";
    bool autoSave = true;            // Flush to disk on every N samples
    int autoSaveInterval = 500;      // Flush every N samples
    bool labelFromScenarioEvents = true;
    double anomalyEventWindow = 5.0; // Label surrounding time as anomalous
};

/**
 * @brief Training data collector for ML anomaly detection
 *
 * Usage:
 *   TrainingDataCollector collector;
 *   collector.configure(config);
 *   collector.start();
 *
 *   // In simulation loop:
 *   collector.sample(ui4d, blackHoleMass, findings, simulationTime);
 *
 *   // On shutdown:
 *   collector.stop();
 *   collector.save();
 */
class TrainingDataCollector {
public:
    TrainingDataCollector();
    ~TrainingDataCollector();

    // Non-copyable
    TrainingDataCollector(const TrainingDataCollector&) = delete;
    TrainingDataCollector& operator=(const TrainingDataCollector&) = delete;

    /**
     * @brief Configure collector parameters
     */
    void configure(const CollectorConfig& cfg);

    /**
     * @brief Start collecting samples
     */
    void start();

    /**
     * @brief Stop collecting samples
     */
    void stop();

    /**
     * @brief Check if collector is active
     */
    bool isActive() const noexcept { return active_; }

    /**
     * @brief Sample current simulation state
     *
     * @param telemetry Physics telemetry from UI4D
     * @param blackHoleMassKg Current black hole mass in kg
     * @param bodies Solar system bodies
     * @param findings Discovery instrument findings
     * @param simulationTime Current simulation time in seconds
     * @param currentScenarioName Name of active scenario, or empty
     */
    void sample(
        const UI4D::PhysicsTelemetry& telemetry,
        double blackHoleMassKg,
        const std::vector<UI4D::SolarSystemBody>& bodies,
        const std::vector<InstrumentFinding>& findings,
        double simulationTime,
        const std::string& currentScenarioName = {}
    );

    /**
     * @brief Manually tag the next sample as anomalous
     */
    void markNextAnomalous(const std::string& description = {});

    /**
     * @brief Get collected samples (thread-safe copy)
     */
    std::vector<TrainingSample> getSamples() const;

    /**
     * @brief Get sample count
     */
    std::size_t count() const;

    /**
     * @brief Save all samples to JSON file
     */
    bool save(const std::string& path = {}) const;

    /**
     * @brief Load samples from JSON file
     */
    bool load(const std::string& path);

    /**
     * @brief Clear all collected samples
     */
    void clear();

    /**
     * @brief Get current configuration
     */
    const CollectorConfig& config() const noexcept { return config_; }

private:
    CollectorConfig config_;
    std::vector<TrainingSample> samples_;
    mutable std::mutex mutex_;
    bool active_ = false;
    bool manualAnomalyNext_ = false;
    std::string manualAnomalyDesc_;
    double lastSampleTime_ = 0.0;

    void flushIfNeeded();
    bool writeJson(const std::string& path) const;
    bool readJson(const std::string& path);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_TRAINING_DATA_COLLECTOR_H
