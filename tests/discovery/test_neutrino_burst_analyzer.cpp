/**
 * @file test_neutrino_burst_analyzer.cpp
 * @brief TDD tests for the NeutrinoBurstAnalyzer
 *
 * Verifies:
 *   - Neutrino-GW coincidence detection from injected post-merger signal
 *   - No detection when no post-merger signal present
 *   - Noise rejection
 *   - Varied disk mass scaling
 *   - Robustness (NaN/Inf, empty, short data)
 *   - Parameter ranges well-formed
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <limits>

#include "discovery/NeutrinoBurstAnalyzer.h"
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

// Generate a GW signal with post-merger neutrino-heated ejecta signature
std::vector<Event4D> makeMergerWithNeutrinoSignal(
    double duration, double dt, double gwTriggerTime,
    double postMergerAmplitude, double postMergerStart)
{
    size_t n = static_cast<size_t>(duration / dt);
    std::vector<Event4D> signal;
    signal.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        double t = static_cast<double>(i) * dt;
        double h = 0.0;

        // Pre-merger inspiral (low amplitude oscillation)
        if (t < gwTriggerTime) {
            double env = 1.0e-21 * std::sin(2.0 * kPi * 50.0 * t);
            h = env * std::sin(2.0 * kPi * 100.0 * t);
        }

        // Post-merger neutrino-heated ejecta (decaying oscillation)
        if (t >= postMergerStart) {
            double dtPost = t - postMergerStart;
            double env = std::exp(-dtPost * 2.0);
            h += postMergerAmplitude * env * std::sin(2.0 * kPi * 200.0 * t);
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
    std::cout << "=== NeutrinoBurstAnalyzerTest ===" << std::endl;
    MetricTensor metric = makeFlatMetric();
    Event4D location(0.0, 0.0, 0.0, 0.0);

    // 1. Neutrino-GW coincidence detection
    {
        NeutrinoBurstAnalyzer analyzer;
        analyzer.setParameter("gw_trigger_time", 0.3);
        analyzer.setParameter("disk_mass_msun", 0.01);
        analyzer.setParameter("distance_mpc", 100.0);
        analyzer.setParameter("time_window", 5.0);
        analyzer.setParameter("energy_threshold", 1.0e-45);
        analyzer.setParameter("snr_threshold", 2.0);

        auto traj = makeMergerWithNeutrinoSignal(1.0, 0.0005, 0.3, 5.0e-21, 0.35);
        auto findings = analyzer.analyze(metric, location, traj);

        assert(!findings.empty() && "Post-merger neutrino signal should be detected");
        if (!findings.empty()) {
            double flux = findings[0].parameters.at("neutrino_flux");
            double snr = findings[0].parameters.at("significance");
            assert(flux > 0.0 && "Neutrino flux should be positive");
            assert(snr > 2.0 && "Significance should exceed threshold");
        }
        std::cout << "  Coincidence detection: OK." << std::endl;
    }

    // 2. No detection without post-merger signal
    {
        NeutrinoBurstAnalyzer analyzer;
        analyzer.setParameter("gw_trigger_time", 0.3);
        analyzer.setParameter("disk_mass_msun", 0.01);
        analyzer.setParameter("distance_mpc", 100.0);
        analyzer.setParameter("time_window", 5.0);
        analyzer.setParameter("energy_threshold", 1.0e-45);
        analyzer.setParameter("snr_threshold", 5.0);

        // Only inspiral, no post-merger signal
        auto traj = makeMergerWithNeutrinoSignal(1.0, 0.0005, 0.3, 0.0, 0.35);
        auto findings = analyzer.analyze(metric, location, traj);

        assert(findings.empty() && "No post-merger signal should not be detected");
        std::cout << "  No false detection: OK." << std::endl;
    }

    // 3. Noise rejection
    {
        NeutrinoBurstAnalyzer analyzer;
        analyzer.setParameter("gw_trigger_time", 0.3);
        analyzer.setParameter("disk_mass_msun", 0.01);
        analyzer.setParameter("distance_mpc", 100.0);
        analyzer.setParameter("time_window", 5.0);
        analyzer.setParameter("energy_threshold", 1.0e-45);
        analyzer.setParameter("snr_threshold", 5.0);

        auto traj = makePureNoise(2000, 1.0e-21);
        auto findings = analyzer.analyze(metric, location, traj);

        assert(findings.empty() && "Pure noise should not trigger detection");
        std::cout << "  Noise rejection: OK." << std::endl;
    }

    // 4. Varied disk mass scaling
    {
        NeutrinoBurstAnalyzer analyzer1;
        analyzer1.setParameter("gw_trigger_time", 0.3);
        analyzer1.setParameter("disk_mass_msun", 0.005);
        analyzer1.setParameter("distance_mpc", 100.0);
        analyzer1.setParameter("time_window", 5.0);
        analyzer1.setParameter("energy_threshold", 1.0e-45);
        analyzer1.setParameter("snr_threshold", 1.0);

        NeutrinoBurstAnalyzer analyzer2;
        analyzer2.setParameter("gw_trigger_time", 0.3);
        analyzer2.setParameter("disk_mass_msun", 0.05);
        analyzer2.setParameter("distance_mpc", 100.0);
        analyzer2.setParameter("time_window", 5.0);
        analyzer2.setParameter("energy_threshold", 1.0e-45);
        analyzer2.setParameter("snr_threshold", 1.0);

        auto traj = makeMergerWithNeutrinoSignal(1.0, 0.0005, 0.3, 5.0e-21, 0.35);
        auto findings1 = analyzer1.analyze(metric, location, traj);
        auto findings2 = analyzer2.analyze(metric, location, traj);

        if (!findings1.empty() && !findings2.empty()) {
            double flux1 = findings1[0].parameters.at("neutrino_flux");
            double flux2 = findings2[0].parameters.at("neutrino_flux");
            assert(flux2 > flux1 && "Larger disk mass should yield higher flux");
        }
        std::cout << "  Disk mass scaling: OK." << std::endl;
    }

    // 5. Robustness: NaN, Inf, empty, short data
    {
        NeutrinoBurstAnalyzer analyzer;
        analyzer.setParameter("gw_trigger_time", 0.3);
        analyzer.setParameter("disk_mass_msun", 0.01);
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
        NeutrinoBurstAnalyzer analyzer;
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

    std::cout << "  All NeutrinoBurstAnalyzer checks passed." << std::endl;
    return 0;
}
