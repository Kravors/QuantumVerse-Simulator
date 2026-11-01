/**
 * @file test_experiment_smoke.cpp
 * @brief Collection-level smoke test for the experiment framework.
 *
 * Exercises every registered experiment through the same code path the
 * QML ExperimentsPanel uses: instantiate with default parameters, run(),
 * verify non-empty output (curve or scalars).
 *
 * Runs headless without QApplication: Experiment::run() is synchronous,
 * so no event loop is required.
 *
 * This catches collection-level bugs that per-experiment unit tests miss:
 * - Experiment whose run() returns success=true but empty curve+scalars
 * - Default parameter value outside its own declared [min, max] range
 * - Experiment that crashes on default parameters
 */

#include "test_assert.h"
#include "experiment/Experiment.h"
#include "experiment/cosmology/LambdaCDMReferenceProvider.h"

#include "experiment/experiments/MassSweepExperiment.h"
#include "experiment/experiments/SpinSweepExperiment.h"
#include "experiment/experiments/LensingDeflectionExperiment.h"
#include "experiment/experiments/LensingTimeDelayExperiment.h"
#include "experiment/experiments/FRBGeneratorExperiment.h"
#include "experiment/experiments/PTASpectrumExperiment.h"
#include "experiment/experiments/GWRingdownExperiment.h"
#include "experiment/experiments/PhotonSphereExperiment.h"
#include "experiment/experiments/ChargeSweepExperiment.h"
#include "experiment/experiments/MercuryPrecessionExperiment.h"
#include "experiment/experiments/FrameDraggingExperiment.h"
#include "experiment/experiments/RedshiftProfileExperiment.h"
#include "experiment/experiments/AnomalyScanExperiment.h"
#include "experiment/experiments/TheoryComparisonExperiment.h"
#include "experiment/experiments/QuantumGeometryExperiment.h"
#include "experiment/experiments/AgeOfUniverseExperiment.h"
#include "experiment/experiments/DistanceLadderExperiment.h"
#include "experiment/experiments/CMBTemperatureExperiment.h"
#include "experiment/experiments/BAOSoundHorizonExperiment.h"
#include "experiment/experiments/HubbleEvolutionExperiment.h"
#include "experiment/experiments/GrowthFactorExperiment.h"
#include "experiment/experiments/CosmologyValidationSuite.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

using namespace quantumverse;

static QVariantMap buildDefaultParams(const QVector<ExperimentParameter>& params) {
    QVariantMap m;
    for (const auto& p : params) {
        m[p.name] = p.defaultValue;
    }
    return m;
}

static bool valueInRange(const QVariant& defaultValue, const QVariant& minV,
                         const QVariant& maxV) {
    if (!defaultValue.canConvert<double>()) return true; // non-numeric: skip
    double v = defaultValue.toDouble();
    double mn = minV.canConvert<double>() ? minV.toDouble() : -1e300;
    double mx = maxV.canConvert<double>() ? maxV.toDouble() : 1e300;
    return v >= mn && v <= mx;
}

int main() {
    int passed = 0, failed = 0;

    auto check = [&](const char* id, bool cond, const char* msg) {
        if (cond) {
            ++passed;
            std::cout << "  PASS " << id << ": " << msg << std::endl;
        } else {
            ++failed;
            std::cout << "  FAIL " << id << ": " << msg << std::endl;
        }
    };

    // --- Register all 22 experiments (same list as main_qml.cpp) ---
    std::vector<std::unique_ptr<Experiment>> exps;
    exps.push_back(std::make_unique<quantumverse::MassSweepExperiment>());
    exps.push_back(std::make_unique<quantumverse::SpinSweepExperiment>());
    exps.push_back(std::make_unique<quantumverse::LensingDeflectionExperiment>());
    exps.push_back(std::make_unique<quantumverse::LensingTimeDelayExperiment>());
    exps.push_back(std::make_unique<quantumverse::FRBGeneratorExperiment>());
    exps.push_back(std::make_unique<quantumverse::PTASpectrumExperiment>());
    exps.push_back(std::make_unique<quantumverse::GWRingdownExperiment>());
    exps.push_back(std::make_unique<quantumverse::PhotonSphereExperiment>());
    exps.push_back(std::make_unique<quantumverse::ChargeSweepExperiment>());
    exps.push_back(std::make_unique<quantumverse::MercuryPrecessionExperiment>());
    exps.push_back(std::make_unique<quantumverse::FrameDraggingExperiment>());
    exps.push_back(std::make_unique<quantumverse::RedshiftProfileExperiment>());
    exps.push_back(std::make_unique<quantumverse::AnomalyScanExperiment>());
    exps.push_back(std::make_unique<quantumverse::TheoryComparisonExperiment>());
    exps.push_back(std::make_unique<quantumverse::QuantumGeometryExperiment>());
    exps.push_back(std::make_unique<quantumverse::AgeOfUniverseExperiment>(
        std::make_shared<quantumverse::LambdaCDMReferenceProvider>()));
    exps.push_back(std::make_unique<quantumverse::DistanceLadderExperiment>(
        std::make_shared<quantumverse::LambdaCDMReferenceProvider>()));
    exps.push_back(std::make_unique<quantumverse::CMBTemperatureExperiment>(
        std::make_shared<quantumverse::LambdaCDMReferenceProvider>()));
    exps.push_back(std::make_unique<quantumverse::BAOSoundHorizonExperiment>(
        std::make_shared<quantumverse::LambdaCDMReferenceProvider>()));
    exps.push_back(std::make_unique<quantumverse::HubbleEvolutionExperiment>(
        std::make_shared<quantumverse::LambdaCDMReferenceProvider>()));
    exps.push_back(std::make_unique<quantumverse::GrowthFactorExperiment>(
        std::make_shared<quantumverse::LambdaCDMReferenceProvider>()));
    exps.push_back(std::unique_ptr<Experiment>(new quantumverse::CosmologyValidationSuite()));

    QV_CHECK(exps.size() == 22);

    for (auto& e : exps) {
        const QString id = e->id();
        const QString name = e->name();
        const QString cat = e->category();

        std::cout << "Testing: " << id.toStdString() << " (" << name.toStdString()
                  << ") [" << cat.toStdString() << "]" << std::endl;

        // Build default parameter map from declared parameters
        QVariantMap params = buildDefaultParams(e->parameters());

        // Check every default value is within its declared range
        bool defaultsInRange = true;
        for (const auto& p : e->parameters()) {
            if (!valueInRange(p.defaultValue, p.min, p.max)) {
                defaultsInRange = false;
                std::cout << "    DEFAULT OUT OF RANGE: " << p.name.toStdString()
                          << " = " << p.defaultValue.toString().toStdString()
                          << " (min=" << p.min.toString().toStdString()
                          << ", max=" << p.max.toString().toStdString() << ")"
                          << std::endl;
            }
        }
        check(id.toStdString().c_str(), defaultsInRange,
              "all default parameters within declared ranges");

        // Run the experiment with default parameters
        ExperimentResult r;
        bool ranOK = true;
        try {
            r = e->run(params, [](double){}, [](){ return false; });
        } catch (const std::exception& ex) {
            ranOK = false;
            std::cout << "    EXCEPTION: " << ex.what() << std::endl;
        } catch (...) {
            ranOK = false;
            std::cout << "    UNKNOWN EXCEPTION" << std::endl;
        }

        check(id.toStdString().c_str(), ranOK, "run() completes without exception");
        if (!ranOK) { ++failed; continue; }

        bool hasOutput = !r.curve.isEmpty() || !r.scalars.isEmpty();
        check(id.toStdString().c_str(), hasOutput,
              "produces non-empty curve or scalars");

        // success=true means "all internal checks passed" — for cosmology
        // validation experiments this can legitimately be false (comparing
        // against observational data). Report it as info, not fail.
        if (r.success)
            std::cout << "    [all validation checks passed]" << std::endl;
        else
            std::cout << "    [some validation checks failed - expected for validation experiments]" << std::endl;

        if (ranOK && hasOutput) ++passed;
        else ++failed;
    }

    // --- Summary ---
    std::cout << "\n=== Experiment Smoke Test Summary ===" << std::endl;
    std::cout << "Experiments tested: " << exps.size() << std::endl;
    std::cout << "Checks passed:      " << passed << std::endl;
    std::cout << "Checks failed:      " << failed << std::endl;

    return failed == 0 ? 0 : 1;
}
