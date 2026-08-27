/**
 * @file test_fast_radio_burst_analyzer.cpp
 * @brief TDD tests for the FastRadioBurstAnalyzer
 *
 * Verifies:
 *   - Burst detection from injected FRB signal
 *   - SNR cutoff for weak bursts
 *   - GW coincidence detection
 *   - No false positive without GW trigger
 *   - Noise rejection
 *   - Robustness (NaN/Inf, empty, short data)
 *   - Parameter ranges well-formed
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <limits>

#include "discovery/FastRadioBurstAnalyzer.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

namespace {
constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kInf = std::numeric_limits<double>::infinity();

MetricTensor makeFlatMetric() {
    MetricTensor m;
    m.g[0][0] = -1.0;
    m.g[1][1] = 1.0;
    m.g[2][2] = 1.0;
    m.g[3][3] = 1.0;
    return m;
}

// Generate a radio time series with an injected FRB burst
std::vector<Event4D> makeRadioWithBurst(
    double duration, double dt, double burstTime,
    double burstAmplitude, double burstWidth)
{
    size_t n = static_cast<size_t>(duration / dt);
    std::vector<Event4D> signal;
    signal.reserve(n);

    // Background noise level
    double noiseLevel = 0.1;

    for (size_t i = 0; i < n; ++i) {
        double t = static_cast<double>(i) * dt;
        double flux = noiseLevel * (static_cast<double>(rand()) / RAND_MAX - 0.5);

        // Gaussian burst profile
        double dtBurst = t - burstTime;
        double burst = burstAmplitude * std::exp(-0.5 * std::pow(dtBurst / burstWidth, 2));
        flux += burst;

        signal.emplace_back(t, flux, 0.0, 0.0);
    }
    return signal;
}

std::vector<Event4D> makePureNoise(size_t n, double noiseLevel) {
    std::vector<Event4D> noise;
    noise.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        double t = static_cast<double>(i) * 0.001;
        double flux = noiseLevel * (static_cast<double>(rand()) / RAND_MAX - 0.5);
        noise.emplace_back(t, flux, 0.0, 0.0);
    }
    return noise;
}
} // namespace

int main() {
    std::cout << "=== FastRadioBurstAnalyzerTest ===" << std::endl;
    MetricTensor metric = makeFlatMetric();
    Event4D location(0.0, 0.0, 0.0, 0.0);

    // 1. Burst detection
    {
        FastRadioBurstAnalyzer analyzer;
        analyzer.setParameter("snr_threshold", 5.0);
        analyzer.setParameter("time_window_seconds", 60.0);
        analyzer.setParameter("dm_min", 50.0);
        analyzer.setParameter("dm_max", 5000.0);
        analyzer.setParameter("gw_trigger_time", 0.5);

        auto traj = makeRadioWithBurst(1.0, 0.0005, 0.5, 5.0, 0.005);
        auto findings = analyzer.analyze(metric, location, traj);

        assert(!findings.empty() && "Injected burst should be detected");
        if (!findings.empty()) {
            double snr = findings[0].parameters.at("snr");
            assert(snr > 5.0 && "SNR should exceed threshold");
        }
        std::cout << "  Burst detection: OK." << std::endl;
    }

    // 2. SNR cutoff
    {
        FastRadioBurstAnalyzer analyzer;
        analyzer.setParameter("snr_threshold", 8.0);
        analyzer.setParameter("time_window_seconds", 60.0);
        analyzer.setParameter("gw_trigger_time", 0.5);

        auto traj = makeRadioWithBurst(1.0, 0.0005, 0.5, 1.0, 0.005);
        auto findings = analyzer.analyze(metric, location, traj);

        assert(findings.empty() && "Weak burst should not be detected");
        std::cout << "  SNR cutoff: OK." << std::endl;
    }

    // 3. GW coincidence
    {
        FastRadioBurstAnalyzer analyzer;
        analyzer.setParameter("snr_threshold", 5.0);
        analyzer.setParameter("time_window_seconds", 60.0);
        analyzer.setParameter("gw_trigger_time", 0.5);

        auto traj = makeRadioWithBurst(1.0, 0.0005, 0.5, 5.0, 0.005);
        auto findings = analyzer.analyze(metric, location, traj);

        assert(!findings.empty() && "Burst should be detected");
        if (!findings.empty()) {
            double hasCoincidence = findings[0].parameters.at("has_coincidence");
            assert(hasCoincidence > 0.5 && "GW coincidence should be flagged");
        }
        std::cout << "  GW coincidence: OK." << std::endl;
    }

    // 4. No false positive without GW trigger
    {
        FastRadioBurstAnalyzer analyzer;
        analyzer.setParameter("snr_threshold", 5.0);
        analyzer.setParameter("time_window_seconds", 0.001);
        analyzer.setParameter("gw_trigger_time", 0.5);

        auto traj = makeRadioWithBurst(1.0, 0.0005, 0.5, 5.0, 0.005);
        auto findings = analyzer.analyze(metric, location, traj);

        if (!findings.empty()) {
            double hasCoincidence = findings[0].parameters.at("has_coincidence");
            assert(hasCoincidence < 0.5 && "No coincidence with tiny time window");
        }
        std::cout << "  No false positive: OK." << std::endl;
    }

    // 5. Noise rejection
    {
        FastRadioBurstAnalyzer analyzer;
        analyzer.setParameter("snr_threshold", 5.0);
        analyzer.setParameter("time_window_seconds", 60.0);
        analyzer.setParameter("gw_trigger_time", 0.5);

        auto traj = makePureNoise(2000, 0.1);
        auto findings = analyzer.analyze(metric, location, traj);

        assert(findings.empty() && "Pure noise should not trigger detection");
        std::cout << "  Noise rejection: OK." << std::endl;
    }

    // 6. Robustness
    {
        FastRadioBurstAnalyzer analyzer;
        analyzer.setParameter("snr_threshold", 5.0);
        analyzer.setParameter("time_window_seconds", 60.0);
        analyzer.setParameter("gw_trigger_time", 0.5);

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

    // 7. Parameter ranges
    {
        FastRadioBurstAnalyzer analyzer;
        auto ranges = analyzer.getParameterRanges();
        assert(ranges.size() >= 5 && "Must have at least 5 parameter ranges");

        for (const auto& kv : ranges) {
            assert(std::isfinite(kv.second.first) &&
                   std::isfinite(kv.second.second) &&
                   kv.second.first < kv.second.second &&
                   "Range bounds must be finite and ordered");
        }
        std::cout << "  Parameter ranges valid." << std::endl;
    }

    std::cout << "  All FastRadioBurstAnalyzer checks passed." << std::endl;
    return 0;
}
