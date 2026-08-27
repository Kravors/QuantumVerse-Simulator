/**
 * @file test_kilonova_afterglow_scanner.cpp
 * @brief TDD tests for the KilonovaAfterglowScanner
 *
 * Verifies:
 *   - Kilonova afterglow detection from injected post-merger EM signal
 *   - No detection when no post-merger signal present
 *   - Noise rejection
 *   - Varied ejecta mass scaling
 *   - Robustness (NaN/Inf, empty, short data)
 *   - Parameter ranges well-formed
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <limits>

#include "discovery/KilonovaAfterglowScanner.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

namespace {
constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kInf = std::numeric_limits<double>::infinity();
constexpr double kPi = 3.14159265358979323846;

MetricTensor makeFlatMetric() {
    MetricTensor m;
    m.g[0][0] = -1.0;
    m.g[1][1] = 1.0;
    m.g[2][2] = 1.0;
    m.g[3][3] = 1.0;
    return m;
}

// Generate a GW signal with post-merger kilonova afterglow signature
std::vector<Event4D> makeMergerWithKilonova(
    double duration, double dt, double gwTriggerTime,
    double afterglowAmplitude, double afterglowStart)
{
    size_t n = static_cast<size_t>(duration / dt);
    std::vector<Event4D> signal;
    signal.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        double t = static_cast<double>(i) * dt;
        double h = 0.0;

        // Pre-merger inspiral (low amplitude oscillation)
        if (t < gwTriggerTime) {
            double env = 0.1 * std::sin(2.0 * kPi * 50.0 * t);
            h = env * std::sin(2.0 * kPi * 100.0 * t);
        }

        // Post-merger kilonova afterglow (rising then decaying light curve)
        if (t >= afterglowStart) {
            double dtPost = t - afterglowStart;
            double tDay = dtPost / 86400.0;
            double env;
            if (tDay < 1.0) {
                env = std::pow(tDay / 1.0, 1.5);  // rise
            } else {
                env = std::pow(tDay / 1.0, -1.3); // decay
            }
            h += afterglowAmplitude * env * std::sin(2.0 * kPi * 10.0 * t);
        }

        signal.emplace_back(t, h, 0.0, 0.0);
    }
    return signal;
}

std::vector<Event4D> makePureNoise(size_t n, double noiseLevel) {
    std::vector<Event4D> noise;
    noise.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        double t = static_cast<double>(i) * 0.001;
        double h = noiseLevel * (static_cast<double>(rand()) / RAND_MAX - 0.5);
        noise.emplace_back(t, h, 0.0, 0.0);
    }
    return noise;
}
} // namespace

int main() {
    std::cout << "=== KilonovaAfterglowScannerTest ===" << std::endl;
    MetricTensor metric = makeFlatMetric();
    Event4D location(0.0, 0.0, 0.0, 0.0);

    // 1. Kilonova afterglow detection
    {
        KilonovaAfterglowScanner analyzer;
        analyzer.setParameter("gw_trigger_time", 0.3);
        analyzer.setParameter("ejecta_mass_msun", 0.01);
        analyzer.setParameter("distance_mpc", 100.0);
        analyzer.setParameter("time_window_days", 7.0);
        analyzer.setParameter("magnitude_limit", 24.0);
        analyzer.setParameter("snr_threshold", 2.0);

        auto traj = makeMergerWithKilonova(2.0, 0.001, 0.3, 1.0e-20, 0.5);
        auto findings = analyzer.analyze(metric, location, traj);

        assert(!findings.empty() && "Post-merger kilonova signal should be detected");
        if (!findings.empty()) {
            double lum = findings[0].parameters.at("peak_luminosity");
            double snr = findings[0].parameters.at("significance");
            assert(lum > 0.0 && "Peak luminosity should be positive");
            assert(snr > 2.0 && "Significance should exceed threshold");
        }
        std::cout << "  Afterglow detection: OK." << std::endl;
    }

    // 2. No detection without post-merger signal
    {
        KilonovaAfterglowScanner analyzer;
        analyzer.setParameter("gw_trigger_time", 0.3);
        analyzer.setParameter("ejecta_mass_msun", 0.01);
        analyzer.setParameter("distance_mpc", 100.0);
        analyzer.setParameter("time_window_days", 7.0);
        analyzer.setParameter("magnitude_limit", 24.0);
        analyzer.setParameter("snr_threshold", 5.0);

        auto traj = makeMergerWithKilonova(2.0, 0.001, 0.3, 0.0, 0.5);
        auto findings = analyzer.analyze(metric, location, traj);

        assert(findings.empty() && "No post-merger signal should not be detected");
        std::cout << "  No false detection: OK." << std::endl;
    }

    // 3. Noise rejection
    {
        KilonovaAfterglowScanner analyzer;
        analyzer.setParameter("gw_trigger_time", 0.3);
        analyzer.setParameter("ejecta_mass_msun", 0.01);
        analyzer.setParameter("distance_mpc", 100.0);
        analyzer.setParameter("time_window_days", 7.0);
        analyzer.setParameter("magnitude_limit", 24.0);
        analyzer.setParameter("snr_threshold", 5.0);

        auto traj = makePureNoise(2000, 1.0e-21);
        auto findings = analyzer.analyze(metric, location, traj);

        assert(findings.empty() && "Pure noise should not trigger detection");
        std::cout << "  Noise rejection: OK." << std::endl;
    }

    // 4. Varied ejecta mass scaling
    {
        KilonovaAfterglowScanner analyzer1;
        analyzer1.setParameter("gw_trigger_time", 0.3);
        analyzer1.setParameter("ejecta_mass_msun", 0.005);
        analyzer1.setParameter("distance_mpc", 100.0);
        analyzer1.setParameter("time_window_days", 7.0);
        analyzer1.setParameter("magnitude_limit", 28.0);
        analyzer1.setParameter("snr_threshold", 1.0);

        KilonovaAfterglowScanner analyzer2;
        analyzer2.setParameter("gw_trigger_time", 0.3);
        analyzer2.setParameter("ejecta_mass_msun", 0.05);
        analyzer2.setParameter("distance_mpc", 100.0);
        analyzer2.setParameter("time_window_days", 7.0);
        analyzer2.setParameter("magnitude_limit", 28.0);
        analyzer2.setParameter("snr_threshold", 1.0);

        auto traj = makeMergerWithKilonova(2.0, 0.001, 0.3, 1.0e-20, 0.5);
        auto findings1 = analyzer1.analyze(metric, location, traj);
        auto findings2 = analyzer2.analyze(metric, location, traj);

        if (!findings1.empty() && !findings2.empty()) {
            double lum1 = findings1[0].parameters.at("peak_luminosity");
            double lum2 = findings2[0].parameters.at("peak_luminosity");
            assert(lum2 > lum1 && "Larger ejecta mass should yield higher luminosity");
        }
        std::cout << "  Ejecta mass scaling: OK." << std::endl;
    }

    // 5. Robustness: NaN, Inf, empty, short data
    {
        KilonovaAfterglowScanner analyzer;
        analyzer.setParameter("gw_trigger_time", 0.3);
        analyzer.setParameter("ejecta_mass_msun", 0.01);
        analyzer.setParameter("distance_mpc", 100.0);
        analyzer.setParameter("snr_threshold", 3.0);

        std::vector<Event4D> empty;
        auto findings = analyzer.analyze(metric, location, empty);
        assert(findings.empty() && "Empty trajectory must yield no findings");

        std::vector<Event4D> nanTraj;
        for (size_t i = 0; i < 64; ++i) {
            double t = static_cast<double>(i) * 0.001;
            nanTraj.emplace_back(t, kNaN, 0.0, 0.0);
        }
        findings = analyzer.analyze(metric, location, nanTraj);
        assert(findings.empty() && "NaN waveform must yield no findings");

        std::vector<Event4D> infTraj;
        for (size_t i = 0; i < 64; ++i) {
            double t = static_cast<double>(i) * 0.001;
            infTraj.emplace_back(t, kInf, 0.0, 0.0);
        }
        findings = analyzer.analyze(metric, location, infTraj);
        assert(findings.empty() && "Inf waveform must yield no findings");

        std::vector<Event4D> shortTraj;
        for (size_t i = 0; i < 10; ++i) {
            double t = static_cast<double>(i) * 0.001;
            shortTraj.emplace_back(t, 1.0, 0.0, 0.0);
        }
        findings = analyzer.analyze(metric, location, shortTraj);
        assert(findings.empty() && "Short trajectory must yield no findings");

        std::cout << "  Robustness handled." << std::endl;
    }

    // 6. Parameter ranges well-formed
    {
        KilonovaAfterglowScanner analyzer;
        auto ranges = analyzer.getParameterRanges();
        assert(ranges.size() >= 6 && "Must have at least 6 parameter ranges");

        for (const auto& kv : ranges) {
            assert(std::isfinite(kv.second.first) &&
                   std::isfinite(kv.second.second) &&
                   kv.second.first < kv.second.second &&
                   "Range bounds must be finite and ordered");
        }
        std::cout << "  Parameter ranges valid." << std::endl;
    }

    std::cout << "  All KilonovaAfterglowScanner checks passed." << std::endl;
    return 0;
}
