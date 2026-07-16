// Phase 6.1: Instrument Output Auditing
// Validates that all 10 discovery instruments produce finite, physically
// bounded outputs and do not crash on repeated scans.

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>

#include "discovery/ExoplanetaryTTVFifthForceHunter.h"
#include "discovery/GalacticRotationCurveScanner.h"
#include "discovery/FineStructureConstantDriftObservatory.h"
#include "discovery/BosonStarCollisionPredictor.h"
#include "discovery/NeutronStarGlitchPhaseDetector.h"
#include "discovery/UltralightDMWaveInterferometer.h"
#include "discovery/BlackHoleJetAnomalyRecogniser.h"
#include "discovery/PrimordialLithiumCrisisSolver.h"
#include "discovery/GalacticTidalStreamCartographer.h"
#include "discovery/RecombinationConstantVariationImager.h"
#include "discovery/DiscoveryPanelManager.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

namespace {
constexpr double kEps = 1e-9;

bool isFiniteDouble(double v) {
    return std::isfinite(v);
}

void assertFinite(const std::string& label, double v) {
    assert(isFiniteDouble(v) && ("Non-finite value for " + label).c_str());
    (void)label;
    (void)v;
}

Event4D makeTrajectoryPoint(double, double, double, double) {
    return Event4D(0.0, 0.0, 0.0, 0.0);
}

std::vector<Event4D> makeGalacticTrajectory() {
    std::vector<Event4D> traj;
    traj.reserve(100);
    for (double r = 1.0; r <= 100.0; r += 1.0) {
        double v = (r < 20.0) ? (10.0 / std::sqrt(r)) : 8.0;
        traj.emplace_back(r, r, 0.0, v);
    }
    return traj;
}

std::vector<Event4D> makeTTVTrajectory() {
    std::vector<Event4D> traj;
    traj.reserve(50);
    for (double t = 0.0; t < 50.0; t += 1.0) {
        traj.emplace_back(t, std::sin(t * 0.5), std::cos(t * 0.3), 0.0);
    }
    return traj;
}

std::vector<Event4D> makeGenericTrajectory() {
    std::vector<Event4D> traj;
    traj.reserve(20);
    for (int i = 0; i < 20; ++i) {
        traj.emplace_back(i * 0.5, i * 0.1, i * 0.2, i * 0.05);
    }
    return traj;
}
}

int main() {
    std::cout << "=== InstrumentAuditTest ===" << std::endl;

    MetricTensor metric;
    Event4D location(0.0, 0.0, 0.0, 0.0);
    auto galacticTraj = makeGalacticTrajectory();
    auto ttTraj = makeTTVTrajectory();
    auto genericTraj = makeGenericTrajectory();

    // --- ExoplanetaryTTVFifthForceHunter --------------------------------------
    {
        ExoplanetaryTTVFifthForceHunter hunter;
        hunter.setParameter("orbital_period", 10.0);
        hunter.setParameter("semi_major_axis", 0.05);
        hunter.setParameter("fifth_force_strength", 0.01);
        auto findings = hunter.analyze(metric, location, ttTraj);
        for (const auto& f : findings) {
            assertFinite("TTV confidence", f.confidence);
            assert(f.confidence >= 0.0 && f.confidence <= 1.0);
            assert(f.severity >= AlertSeverity::INFO && f.severity <= AlertSeverity::CRITICAL);
        }
        std::cout << "  ExoplanetaryTTVFifthForceHunter: " << findings.size() << " findings" << std::endl;
    }

    // --- GalacticRotationCurveScanner -----------------------------------------
    {
        GalacticRotationCurveScanner scanner;
        auto findings = scanner.analyze(metric, location, galacticTraj);
        for (const auto& f : findings) {
            assertFinite("Galactic confidence", f.confidence);
            assert(f.confidence >= 0.0 && f.confidence <= 1.0);
            auto it = f.parameters.find("flatten_radius");
            if (it != f.parameters.end()) {
                assertFinite("flatten_radius", it->second);
                assert(it->second > 0.0);
            }
        }
        std::cout << "  GalacticRotationCurveScanner: " << findings.size() << " findings" << std::endl;
    }

    // --- FineStructureConstantDriftObservatory ---------------------------------
    {
        FineStructureConstantDriftObservatory obs;
        obs.setParameter("drift_rate", 1.0e-6);
        auto findings = obs.analyze(metric, location, genericTraj);
        for (const auto& f : findings) {
            assertFinite("FineStructure confidence", f.confidence);
            assert(f.confidence >= 0.0 && f.confidence <= 1.0);
        }
        std::cout << "  FineStructureConstantDriftObservatory: " << findings.size() << " findings" << std::endl;
    }

    // --- BosonStarCollisionPredictor ------------------------------------------
    {
        BosonStarCollisionPredictor predictor;
        predictor.setParameter("merger_time", 100.0);
        predictor.setParameter("peak_strain", 1.0e-21);
        predictor.setParameter("ringdown_freq_hz", 100.0);
        predictor.setParameter("boson_mass_ev", 1.0e-22);
        auto findings = predictor.analyze(metric, location, genericTraj);
        for (const auto& f : findings) {
            assertFinite("BosonStar confidence", f.confidence);
            assert(f.confidence >= 0.0 && f.confidence <= 1.0);
        }
        std::cout << "  BosonStarCollisionPredictor: " << findings.size() << " findings" << std::endl;
    }

    // --- NeutronStarGlitchPhaseDetector ----------------------------------------
    {
        NeutronStarGlitchPhaseDetector detector;
        auto findings = detector.analyze(metric, location, genericTraj);
        for (const auto& f : findings) {
            assertFinite("NeutronStar confidence", f.confidence);
            assert(f.confidence >= 0.0 && f.confidence <= 1.0);
        }
        std::cout << "  NeutronStarGlitchPhaseDetector: " << findings.size() << " findings" << std::endl;
    }

    // --- UltralightDMWaveInterferometer ----------------------------------------
    {
        UltralightDMWaveInterferometer interferometer;
        interferometer.setParameter("oscillation_freq_rad_s", 1.0e-15);
        interferometer.setParameter("signal_amplitude", 1.0e-22);
        interferometer.setParameter("snr", 5.0);
        auto findings = interferometer.analyze(metric, location, genericTraj);
        for (const auto& f : findings) {
            assertFinite("UltralightDM confidence", f.confidence);
            assert(f.confidence >= 0.0 && f.confidence <= 1.0);
        }
        std::cout << "  UltralightDMWaveInterferometer: " << findings.size() << " findings" << std::endl;
    }

    // --- BlackHoleJetAnomalyRecogniser -----------------------------------------
    {
        BlackHoleJetAnomalyRecogniser recogniser;
        recogniser.setParameter("anomaly_time", 0.0);
        recogniser.setParameter("anomaly_type", 0.0);
        auto findings = recogniser.analyze(metric, location, genericTraj);
        for (const auto& f : findings) {
            assertFinite("BlackHoleJet confidence", f.confidence);
            assert(f.confidence >= 0.0 && f.confidence <= 1.0);
        }
        std::cout << "  BlackHoleJetAnomalyRecogniser: " << findings.size() << " findings" << std::endl;
    }

    // --- PrimordialLithiumCrisisSolver -----------------------------------------
    {
        PrimordialLithiumCrisisSolver solver;
        auto findings = solver.analyze(metric, location, genericTraj);
        for (const auto& f : findings) {
            assertFinite("Lithium confidence", f.confidence);
            assert(f.confidence >= 0.0 && f.confidence <= 1.0);
        }
        std::cout << "  PrimordialLithiumCrisisSolver: " << findings.size() << " findings" << std::endl;
    }

    // --- GalacticTidalStreamCartographer ---------------------------------------
    {
        GalacticTidalStreamCartographer cartographer;
        auto findings = cartographer.analyze(metric, location, genericTraj);
        for (const auto& f : findings) {
            assertFinite("TidalStream confidence", f.confidence);
            assert(f.confidence >= 0.0 && f.confidence <= 1.0);
        }
        std::cout << "  GalacticTidalStreamCartographer: " << findings.size() << " findings" << std::endl;
    }

    // --- RecombinationConstantVariationImager ----------------------------------
    {
        RecombinationConstantVariationImager imager;
        imager.setParameter("delta_alpha_over_alpha", 1.0e-5);
        auto findings = imager.analyze(metric, location, genericTraj);
        for (const auto& f : findings) {
            assertFinite("Recombination confidence", f.confidence);
            assert(f.confidence >= 0.0 && f.confidence <= 1.0);
        }
        std::cout << "  RecombinationConstantVariationImager: " << findings.size() << " findings" << std::endl;
    }

    // --- DiscoveryPanelManager batch scan --------------------------------------
    {
        DiscoveryPanelManager manager;
        manager.registerInstrument(std::make_unique<GalacticRotationCurveScanner>());
        manager.registerInstrument(std::make_unique<FineStructureConstantDriftObservatory>());
        manager.runScan(metric, location);
        std::cout << "  DiscoveryPanelManager batch scan completed without crash." << std::endl;
    }

    // --- Repeated-scan determinism spot check ----------------------------------
    {
        GalacticRotationCurveScanner scanner;
        auto first = scanner.analyze(metric, location, galacticTraj);
        auto second = scanner.analyze(metric, location, galacticTraj);
        assert(first.size() == second.size() && "Repeated scan results differ in size");
        for (size_t i = 0; i < first.size(); ++i) {
            assert(first[i].confidence == second[i].confidence);
            assert(first[i].description == second[i].description);
        }
        std::cout << "  Repeated-scan determinism check passed." << std::endl;
    }

    std::cout << "All InstrumentAuditTest checks passed." << std::endl;
    return 0;
}
