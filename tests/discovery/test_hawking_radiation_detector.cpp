/**
 * @file test_hawking_radiation_detector.cpp
 * @brief TDD tests for the HawkingRadiationDetector discovery instrument
 *
 * Verifies the analytic Hawking relations in geometric units
 * (G = c = ħ = k_B = 1):
 *   - Temperature      T = 1 / (8πM)          (4D Schwarzschild, κ = 1/(4M))
 *   - Entropy          S = A/4 = 4π M²          (Bekenstein–Hawking)
 *   - 2D linear-dilaton asymptotic flux  ⟨T₋₋⟩ = 1/48   (Giddings 1994)
 * and the end-to-end analyze() path that emits a finding.
 *
 * Variables used only inside QV_CHECK() are marked [[maybe_unused]] so the
 * file stays warning-clean under the code-quality job's Release
 * -Wall -Wextra build (where NDEBUG compiles asserts out).
 */

#include <cmath>
#include <iostream>
#include <vector>

#include "discovery/HawkingRadiationDetector.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "test_assert.h"

using namespace quantumverse;

namespace {
constexpr double kPi = 3.14159265358979323846;
} // namespace

int main() {
    std::cout << "=== HawkingRadiationDetectorTest ===" << std::endl;
    MetricTensor metric;                 // placeholder; detector is driven by the "mass" parameter
    Event4D location(0.0, 0.0, 0.0, 0.0);

    // 1. Analytic temperature  T = 1/(8πM)
    for (double M : {0.5, 1.0, 2.0, 10.0}) {
        double T = HawkingRadiationDetector::hawkingTemperature(M);
        double expected = 1.0 / (8.0 * kPi * M);
        [[maybe_unused]] bool ok = std::isfinite(T) && std::abs(T - expected) < 1e-12;
        QV_CHECK(ok);
    }

    // 2. Entropy S = A/4 = 4π M²
    {
        double M = 2.0;
        double S = HawkingRadiationDetector::hawkingEntropy(M);
        double expected = 4.0 * kPi * M * M;
        [[maybe_unused]] bool ok = std::isfinite(S) && std::abs(S - expected) < 1e-9;
        QV_CHECK(ok);
    }

    // 3. 2D linear-dilaton asymptotic flux ⟨T₋₋⟩ = 1/48
    {
        double f = HawkingRadiationDetector::hawkingFlux2D();
        [[maybe_unused]] bool ok = std::isfinite(f) && std::abs(f - 1.0 / 48.0) < 1e-12;
        QV_CHECK(ok);
    }

    // 4. Luminosity is positive, finite, and scales as 1/M²
    {
        double L2 = HawkingRadiationDetector::hawkingLuminosity(2.0);
        double L4 = HawkingRadiationDetector::hawkingLuminosity(4.0);
        [[maybe_unused]] bool ok = L2 > 0.0 && L4 > 0.0 && std::isfinite(L2) && std::isfinite(L4);
        QV_CHECK(ok);
        // L ∝ 1/M²  ⇒  L2 * 2² ≈ L4 * 4²
        double ratio = (L2 * 4.0) / (L4 * 16.0);
        [[maybe_unused]] bool okScale = std::isfinite(ratio) && std::abs(ratio - 1.0) < 1e-9;
        QV_CHECK(okScale);
    }

    // 5. Temperature falls with mass (1/M)
    {
        [[maybe_unused]] bool ok = HawkingRadiationDetector::hawkingTemperature(4.0) <
                                   HawkingRadiationDetector::hawkingTemperature(2.0);
        QV_CHECK(ok);
    }

    // 6. End-to-end analyze() emits a finding for a configured BH mass
    {
        HawkingRadiationDetector hrd;
        hrd.setParameter("mass", 2.0);
        auto findings = hrd.analyze(metric, location);
        [[maybe_unused]] bool ok = !findings.empty();
        QV_CHECK(ok);
        if (!findings.empty()) {
            const auto& f = findings.front();
            double T = f.parameters.at("temperature");
            double expected = 1.0 / (8.0 * kPi * 2.0);
            [[maybe_unused]] bool okT = f.isAnomaly && std::abs(T - expected) < 1e-9;
            QV_CHECK(okT);
            [[maybe_unused]] bool okS = std::abs(f.parameters.at("entropy") - 4.0 * kPi * 4.0) < 1e-9;
            QV_CHECK(okS);
            [[maybe_unused]] bool okC = f.confidence > 0.9;
            QV_CHECK(okC);
        }
    }

    // 7. Non-positive mass yields no finding (robustness)
    {
        HawkingRadiationDetector hrd;
        hrd.setParameter("mass", 0.0);
        auto findings = hrd.analyze(metric, location);
        [[maybe_unused]] bool ok = findings.empty();
        QV_CHECK(ok);
    }

    // 8. Parameter ranges are well-formed and finite
    {
        HawkingRadiationDetector hrd;
        auto ranges = hrd.getParameterRanges();
        [[maybe_unused]] bool ok = !ranges.empty();
        QV_CHECK(ok);
        for ([[maybe_unused]] const auto& kv : ranges) {
            [[maybe_unused]] bool okkv = std::isfinite(kv.second.first) &&
                                        std::isfinite(kv.second.second);
            QV_CHECK(okkv);
        }
    }

    std::cout << "  All HawkingRadiationDetector checks passed." << std::endl;
    return 0;
}
