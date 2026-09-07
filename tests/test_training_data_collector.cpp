// QuantumVerse Test: TrainingDataCollector
// Validates sample collection, labeling, persistence, and loading

#include <cmath>
#include <iostream>
#include <cassert>
#include <fstream>
#include <chrono>
#include <filesystem>

#include "ml/FeatureVector.h"
#include "ml/AnomalyDetector.h"
#include "ml/TrainingDataCollector.h"

using namespace quantumverse;

static UI4D::PhysicsTelemetry makeTelemetry() {
    UI4D::PhysicsTelemetry tel;
    tel.totalKineticEnergy = 1.0e30;
    tel.totalPotentialEnergy = -2.0e30;
    tel.totalEnergy = -1.0e30;
    tel.angularMomentumZ = 1.0e40;
    tel.earthOrbitalSpeed = 29780.0;
    tel.earthDistance = 1.496e11;
    tel.gravitationalWaveStrain = 1.0e-21;
    tel.gravitationalWaveFreq = 0.5;
    tel.chirpMass = 1.0e30;
    tel.blackHoleMass = 1.989e30;
    tel.bodyCount = 2;
    return tel;
}

static std::vector<UI4D::SolarSystemBody> makeBodies() {
    std::vector<UI4D::SolarSystemBody> bodies;
    UI4D::SolarSystemBody sun;
    sun.name = "Sun";
    sun.mass = 1.989e30;
    sun.radius = 6.96e8;
    sun.isCentralBody = true;
    sun.isStar = true;
    sun.showOrbit = true;
    sun.velocity = Event4D(0.0, 0.0, 0.0, 0.0);
    bodies.push_back(sun);

    UI4D::SolarSystemBody earth;
    earth.name = "Earth";
    earth.mass = 5.972e24;
    earth.radius = 6.371e6;
    earth.isCentralBody = false;
    earth.isStar = false;
    earth.showOrbit = true;
    earth.velocity = Event4D(0.0, 0.0, 29780.0, 0.0);
    bodies.push_back(earth);
    return bodies;
}

static std::vector<InstrumentFinding> makeFindings(bool anomalous = false) {
    std::vector<InstrumentFinding> findings;
    InstrumentFinding f;
    f.confidence = anomalous ? 0.95 : 0.1;
    f.isAnomaly = anomalous;
    findings.push_back(f);
    return findings;
}

int main() {
    std::cout << "=== TrainingDataCollector Tests ===" << std::endl;

    const std::string testPath = "data/ml/test_training_data.json";

    // Test 1: Basic collection
    {
        TrainingDataCollector collector;
        CollectorConfig cfg;
        cfg.sampleInterval = 0.0;
        cfg.autoSave = false;
        collector.configure(cfg);
        collector.start();

        auto tel = makeTelemetry();
        auto bodies = makeBodies();
        collector.sample(tel, 1.989e30, bodies, makeFindings(false), 0.0, "test_scenario");
        collector.sample(tel, 1.989e30, bodies, makeFindings(false), 1.0, "test_scenario");

        assert(collector.count() == 2);
        auto samples = collector.getSamples();
        assert(samples[0].scenarioName == "test_scenario");
        assert(samples[1].scenarioName == "test_scenario");
        assert(!samples[0].isAnomalous);
        std::cout << "[PASS] Basic collection works" << std::endl;
    }

    // Test 2: Anomaly labeling from findings
    {
        TrainingDataCollector collector;
        CollectorConfig cfg;
        cfg.sampleInterval = 0.0;
        cfg.autoSave = false;
        cfg.labelFromScenarioEvents = true;
        collector.configure(cfg);
        collector.start();

        auto tel = makeTelemetry();
        auto bodies = makeBodies();
        collector.sample(tel, 1.989e30, bodies, makeFindings(true), 0.0, "scenario");

        assert(collector.count() == 1);
        assert(collector.getSamples()[0].isAnomalous);
        assert(collector.getSamples()[0].labelSource == "instrument");
        std::cout << "[PASS] Anomaly labeling from instrument works" << std::endl;
    }

    // Test 3: Manual anomaly tagging
    {
        TrainingDataCollector collector;
        CollectorConfig cfg;
        cfg.sampleInterval = 0.0;
        cfg.autoSave = false;
        cfg.labelFromScenarioEvents = false;
        collector.configure(cfg);
        collector.start();

        auto tel = makeTelemetry();
        auto bodies = makeBodies();

        collector.markNextAnomalous("injected BH");
        collector.sample(tel, 1.989e30, bodies, makeFindings(false), 0.0, "");

        assert(collector.count() == 1);
        assert(collector.getSamples()[0].isAnomalous);
        assert(collector.getSamples()[0].labelSource == "manual");
        assert(collector.getSamples()[0].description == "injected BH");
        std::cout << "[PASS] Manual anomaly tagging works" << std::endl;
    }

    // Test 4: Save and load round-trip
    {
        TrainingDataCollector collector;
        CollectorConfig cfg;
        cfg.sampleInterval = 0.0;
        cfg.autoSave = false;
        cfg.outputPath = testPath;
        collector.configure(cfg);
        collector.start();

        auto tel = makeTelemetry();
        auto bodies = makeBodies();
        collector.sample(tel, 1.989e30, bodies, makeFindings(false), 0.0, "scenario1");
        collector.sample(tel, 2.0 * 1.989e30, bodies, makeFindings(true), 1.0, "scenario2");

        collector.save();
        collector.clear();
        assert(collector.count() == 0);

        bool loaded = collector.load(testPath);
        assert(loaded);
        assert(collector.count() == 2);
        assert(collector.getSamples()[0].scenarioName == "scenario1");
        assert(!collector.getSamples()[0].isAnomalous);
        assert(collector.getSamples()[1].scenarioName == "scenario2");
        assert(collector.getSamples()[1].isAnomalous);
        std::cout << "[PASS] Save/load round-trip works" << std::endl;
    }

    // Test 5: Sampling respects interval
    {
        TrainingDataCollector collector;
        CollectorConfig cfg;
        cfg.sampleInterval = 10.0;
        cfg.autoSave = false;
        collector.configure(cfg);
        collector.start();

        auto tel = makeTelemetry();
        auto bodies = makeBodies();
        collector.sample(tel, 1.989e30, bodies, makeFindings(false), 0.0);
        collector.sample(tel, 1.989e30, bodies, makeFindings(false), 1.0);
        collector.sample(tel, 1.989e30, bodies, makeFindings(false), 11.0);

        assert(collector.count() == 2);
        std::cout << "[PASS] Sampling interval respected" << std::endl;
    }

    // Test 6: Feature extraction round-trip
    {
        TrainingDataCollector collector;
        CollectorConfig cfg;
        cfg.sampleInterval = 0.0;
        cfg.autoSave = false;
        collector.configure(cfg);
        collector.start();

        auto tel = makeTelemetry();
        auto bodies = makeBodies();
        collector.sample(tel, 1.989e30, bodies, makeFindings(false), 0.0);

        auto samples = collector.getSamples();
        auto v = samples[0].features.toVector();
        assert(v.size() == FeatureVector::Dim);

        auto restored = FeatureVector::fromVector(v);
        assert(restored.toVector() == v);
        std::cout << "[PASS] Feature vector round-trip preserved" << std::endl;
    }

    // Test 7: Max samples cap stops collection
    {
        TrainingDataCollector collector;
        CollectorConfig cfg;
        cfg.sampleInterval = 0.0;
        cfg.autoSave = false;
        cfg.maxSamples = 3;
        collector.configure(cfg);
        collector.start();

        auto tel = makeTelemetry();
        auto bodies = makeBodies();
        for (int i = 0; i < 5; ++i) {
            collector.sample(tel, 1.989e30, bodies, makeFindings(false), i * 1.0);
        }

        assert(collector.count() == 3);
        assert(!collector.isActive());
        std::cout << "[PASS] Max samples cap stops collection" << std::endl;
    }

    // Test 8: Performance benchmark
    {
        TrainingDataCollector collector;
        CollectorConfig cfg;
        cfg.sampleInterval = 0.0;
        cfg.autoSave = false;
        collector.configure(cfg);
        collector.start();

        auto tel = makeTelemetry();
        auto bodies = makeBodies();

        const int iters = 10000;
        auto t0 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iters; ++i) {
            collector.sample(tel, 1.989e30, bodies, makeFindings(false), i * 0.1);
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        std::cout << "[PASS] " << iters << " samples in " << ms << " ms ("
                  << (ms / iters * 1000.0) << " us/sample)" << std::endl;
    }

    // Cleanup
    {
        std::error_code ec;
        std::filesystem::remove(testPath, ec);
    }

    std::cout << "=== ALL TRAINING DATA COLLECTOR TESTS PASSED ===" << std::endl;
    return 0;
}
