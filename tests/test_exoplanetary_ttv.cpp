// QuantumVerse Test: ExoplanetaryTTVFifthForceHunter
// TDD validation of the fifth-force TTV hunter. Builds a synthetic transit
// sequence whose observed times deviate from a linear Keplerian ephemeris by an
// oscillatory perturbation (a Yukawa-type fifth force), and verifies the hunter
// recovers a positive fifth-force strength. Also covers the clean (null) case,
// the too-few-points edge case, and the grid-analysis path.

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "discovery/ExoplanetaryTTVFifthForceHunter.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main() {
    std::cout << "=== ExoplanetaryTTVFifthForceHunter Test ===" << std::endl;

    const double P = 10.0; // orbital period
    const double a = 0.1;   // semi-major axis (also used as host-planet separation)
    const int N = 50;

    // --- Injected fifth-force TTV signal ------------------------------------
    {
        const double amp = 1e-2; // timing perturbation amplitude
        std::vector<quantumverse::Event4D> transits;
        for (int n = 0; n < N; ++n) {
            double t = n * P + amp * std::sin(2.0 * M_PI * static_cast<double>(n) / 12.0);
            transits.emplace_back(t, a, 0.0, 0.0);
        }

        quantumverse::MetricTensor metric;
        quantumverse::Event4D loc(0.0, 0.0, 0.0, 0.0);
        quantumverse::ExoplanetaryTTVFifthForceHunter hunter;
        hunter.setParameter("orbital_period", P);
        hunter.setParameter("semi_major_axis", a);

        auto findings = hunter.analyze(metric, loc, transits);
        assert(!findings.empty() && "Fifth-force TTV should be detected");

        const auto& f = findings.front();
        auto sit = f.parameters.find("fifth_force_strength");
        assert(sit != f.parameters.end() && "fifth_force_strength missing");
        assert(sit->second > 0.0 && "fifth force strength should be positive");
        std::cout << "Detected fifth force: alpha=" << sit->second
                  << " confidence=" << f.confidence << std::endl;
    }

    // --- Clean Keplerian signal (no fifth force) ----------------------------
    {
        std::vector<quantumverse::Event4D> transits;
        for (int n = 0; n < N; ++n) {
            double t = n * P + 1e-6 * std::sin(2.0 * M_PI * static_cast<double>(n));
            transits.emplace_back(t, a, 0.0, 0.0);
        }

        quantumverse::MetricTensor metric;
        quantumverse::Event4D loc(0.0, 0.0, 0.0, 0.0);
        quantumverse::ExoplanetaryTTVFifthForceHunter hunter;
        hunter.setParameter("orbital_period", P);
        hunter.setParameter("semi_major_axis", a);
        hunter.setParameter("timing_noise", 1e-4);

        auto findings = hunter.analyze(metric, loc, transits);
        assert(findings.empty() && "Clean Keplerian signal should not trigger");
    }

    // --- Too few transits ----------------------------------------------------
    {
        std::vector<quantumverse::Event4D> tiny = {
            quantumverse::Event4D(0.0, a, 0.0, 0.0),
            quantumverse::Event4D(P, a, 0.0, 0.0),
        };

        quantumverse::MetricTensor metric;
        quantumverse::Event4D loc(0.0, 0.0, 0.0, 0.0);
        quantumverse::ExoplanetaryTTVFifthForceHunter hunter;
        hunter.setParameter("orbital_period", P);
        hunter.setParameter("semi_major_axis", a);

        auto findings = hunter.analyze(metric, loc, tiny);
        assert(findings.empty() && "Too-few transits should not produce finding");
    }

    // --- Grid analysis path --------------------------------------------------
    {
        std::vector<quantumverse::PlanetaryGridResult> grid;
        quantumverse::PlanetaryGridResult r1;
        r1.anomalyDetected = true;
        r1.fifthForceStrength = 2e-3;
        quantumverse::PlanetaryGridResult r2;
        r2.anomalyDetected = true;
        r2.fifthForceStrength = 4e-3;
        grid.push_back(r1);
        grid.push_back(r2);

        quantumverse::ExoplanetaryTTVFifthForceHunter hunter;
        auto findings = hunter.analyzeGrid(grid);
        assert(!findings.empty() && "Grid with anomalies should yield finding");

        const auto& f = findings.front();
        auto sit = f.parameters.find("fifth_force_strength");
        assert(sit != f.parameters.end() && "grid fifth_force_strength missing");
        assert(std::abs(sit->second - 3e-3) < 1e-12 && "grid mean strength wrong");
        std::cout << "Grid fifth-force mean: " << sit->second << std::endl;
    }

    std::cout << "All ExoplanetaryTTVFifthForceHunter tests passed." << std::endl;
    return 0;
}
