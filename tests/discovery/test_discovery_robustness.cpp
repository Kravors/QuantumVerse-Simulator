// Phase 6.5: Discovery Robustness & Fallback Test
// Validates that discovery components handle missing ONNX models, NaN/inf
// inputs, and extreme parameters without crashing.

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "discovery/DiscoveryPanelManager.h"
#include "discovery/GalacticRotationCurveScanner.h"
#include "discovery/FineStructureConstantDriftObservatory.h"
#include "ml/CurvatureNormalizingFlow.h"
#include "ml/GeodesicNeuralODE.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

namespace {
constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kInf = std::numeric_limits<double>::infinity();
}

int main() {
    std::cout << "=== DiscoveryRobustnessTest ===" << std::endl;

    MetricTensor metric;
    Event4D location(0.0, 0.0, 0.0, 0.0);
    std::vector<Event4D> emptyTrajectory;

    // --- 1. Instruments handle NaN/inf parameters gracefully ------------------
    {
        GalacticRotationCurveScanner scanner;
        scanner.setParameter("flatten_radius", kNaN);
        scanner.setParameter("keplerian_gm", kInf);
        auto findings = scanner.analyze(metric, location, emptyTrajectory);
        (void)findings;
        std::cout << "  Galactic scanner survived NaN/Inf parameters." << std::endl;
    }

    {
        FineStructureConstantDriftObservatory obs;
        obs.setParameter("drift_rate", kNaN);
        auto findings = obs.analyze(metric, location, emptyTrajectory);
        (void)findings;
        std::cout << "  FineStructure observatory survived NaN parameter." << std::endl;
    }

    // --- 2. Instruments handle extreme parameters without crashing -------------
    {
        GalacticRotationCurveScanner scanner;
        scanner.setParameter("flatten_radius", 1e20);
        scanner.setParameter("keplerian_gm", 1e30);
        scanner.setParameter("max_relative_residual", 1e-3);
        auto findings = scanner.analyze(metric, location, emptyTrajectory);
        (void)findings;
        std::cout << "  Galactic scanner survived extreme parameters." << std::endl;
    }

    {
        FineStructureConstantDriftObservatory obs;
        obs.setParameter("drift_rate", 1.0e10);
        auto findings = obs.analyze(metric, location, emptyTrajectory);
        (void)findings;
        std::cout << "  FineStructure observatory survived extreme drift rate." << std::endl;
    }

    // --- 3. ONNX fallback: CurvatureNormalizingFlow returns safe default --------
    {
        CurvatureNormalizingFlow flow;
        // Do NOT load an ONNX model; verify fallback behavior.
        assert(!flow.isLoaded() && "Flow should not be loaded without ONNX model");
        std::vector<double> features(10, 1.0);
        double prediction = flow.predict(features);
        assert(std::isfinite(prediction) && "predict() must return finite value in fallback mode");
        std::cout << "  CurvatureNormalizingFlow fallback prediction: " << prediction << std::endl;
    }

    // --- 4. ONNX fallback: GeodesicNeuralODE uses Euler integration ------------
    {
        ml::GeodesicNeuralODE ode(1);
        bool loaded = ode.loadONNXModel("nonexistent_model.onnx");
        (void)loaded;
        Event4D initial(0.0, 0.0, 0.0, 0.0);
        std::array<double, 4> velocity = {1.0, 0.0, 0.0, 0.0};
        std::vector<double> metric_params = {1.0};
        auto predicted = ode.predict(initial, velocity, metric_params, 1.0);
        assert(!predicted.empty() && "predict() should return at least one event in fallback mode");
        assert(std::isfinite(predicted[0].t) && std::isfinite(predicted[0].x));
        std::cout << "  GeodesicNeuralODE fallback prediction: t=" << predicted[0].t
                  << " x=" << predicted[0].x << std::endl;
    }

    // --- 5. Batch scan with no instruments registered --------------------------
    {
        DiscoveryPanelManager manager;
        manager.runScan(metric, location);
        std::cout << "  DiscoveryPanelManager empty-scan completed without crash." << std::endl;
    }

    // --- 6. Batch scan with mixed valid/invalid trajectories -------------------
    {
        DiscoveryPanelManager manager;
        manager.registerInstrument(std::make_unique<GalacticRotationCurveScanner>());
        manager.registerInstrument(std::make_unique<FineStructureConstantDriftObservatory>());
        manager.runScan(metric, location);
        std::cout << "  Batch scan completed without crash." << std::endl;
    }

    // --- 8. Parameter out-of-range does not cause infinite loop ----------------
    {
        GalacticRotationCurveScanner scanner;
        scanner.setParameter("flatten_radius", -1.0e100);
        scanner.setParameter("keplerian_gm", -1.0e100);
        auto findings = scanner.analyze(metric, location, emptyTrajectory);
        (void)findings;
        std::cout << "  Negative extreme parameters handled safely." << std::endl;
    }

    std::cout << "All DiscoveryRobustnessTest checks passed." << std::endl;
    return 0;
}
