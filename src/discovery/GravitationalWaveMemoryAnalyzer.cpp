/**
 * @file GravitationalWaveMemoryAnalyzer.cpp
 * @brief Implementation of the Christodoulou GW memory analyzer
 */

#include "GravitationalWaveMemoryAnalyzer.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

namespace {
    constexpr double G_const = 6.67430e-11;
    constexpr double c_light = 299792458.0;
    constexpr double c2 = c_light * c_light;
    constexpr double c4 = c2 * c2;
    constexpr double MpcToMeters = 3.08567758e22;
}

GravitationalWaveMemoryAnalyzer::GravitationalWaveMemoryAnalyzer()
{
    setParameter("energy_radiated", 1.0);
    setParameter("distance_mpc", 100.0);
    setParameter("baseline_duration", 0.2);
    setParameter("threshold_sigma", 2.0);
}

std::map<std::string, std::pair<double, double>>
GravitationalWaveMemoryAnalyzer::getParameterRanges() const
{
    return {
        {"energy_radiated", {0.1, 100.0}},
        {"distance_mpc", {1.0, 10000.0}},
        {"baseline_duration", {0.05, 1.0}},
        {"threshold_sigma", {1.0, 5.0}}
    };
}

double GravitationalWaveMemoryAnalyzer::christodoulouMemory(
    double energyRadiated, double distanceMpc)
{
    if (energyRadiated <= 0.0 || distanceMpc <= 0.0) return 0.0;
    double distanceM = distanceMpc * MpcToMeters;
    double factor = 4.0 * G_const / (c4 * distanceM);
    double isotropicFactor = 1.0 / (2.0 * M_PI);
    return factor * energyRadiated * isotropicFactor;
}

double GravitationalWaveMemoryAnalyzer::estimateNoiseSigma(
    const std::vector<Event4D>& trajectory,
    double baselineMean,
    size_t baselineSamples)
{
    if (baselineSamples < 2) return 0.0;
    double sumSq = 0.0;
    for (size_t i = 0; i < baselineSamples; ++i) {
        double d = trajectory[i].x - baselineMean;
        sumSq += d * d;
    }
    return std::sqrt(sumSq / static_cast<double>(baselineSamples));
}

GravitationalWaveMemoryAnalyzer::MemoryResult
GravitationalWaveMemoryAnalyzer::computeMemoryOffset(
    const std::vector<Event4D>& trajectory,
    double baselineDuration,
    double energyRadiated,
    double distanceMpc)
{
    MemoryResult result;
    size_t n = trajectory.size();
    if (n < kMinSamples) return result;

    double dt = (n > 1) ? (trajectory[1].t - trajectory[0].t) : 0.0;
    if (dt <= 0.0) return result;

    size_t baselineSamples = static_cast<size_t>(baselineDuration / dt);
    if (baselineSamples < 4) baselineSamples = 4;
    if (baselineSamples >= n / 2) baselineSamples = n / 4;

    double baselineSum = 0.0;
    for (size_t i = 0; i < baselineSamples; ++i) {
        if (std::isnan(trajectory[i].x) || std::isinf(trajectory[i].x)) return result;
        baselineSum += trajectory[i].x;
    }
    result.baselineMean = baselineSum / static_cast<double>(baselineSamples);

    double noiseSigma = estimateNoiseSigma(trajectory, result.baselineMean, baselineSamples);
    result.sigma = noiseSigma;

    size_t postStart = baselineSamples + (n - baselineSamples) / 3;
    if (postStart >= n) postStart = (n + baselineSamples) / 2;
    size_t postCount = n - postStart;
    if (postCount < 2) return result;

    double postSum = 0.0;
    for (size_t i = postStart; i < n; ++i) {
        if (std::isnan(trajectory[i].x) || std::isinf(trajectory[i].x)) return result;
        postSum += trajectory[i].x;
    }
    result.postMean = postSum / static_cast<double>(postCount);

    result.observedOffset = result.postMean - result.baselineMean;
    result.expectedOffset = christodoulouMemory(energyRadiated, distanceMpc);

    constexpr double kMinMemoryOffset = 1e-21;
    if (std::abs(result.observedOffset) > kMinMemoryOffset) {
        if (noiseSigma > 0.0) {
            result.snr = std::abs(result.observedOffset) / noiseSigma;
        } else {
            result.snr = 1.0 / 1e-30;
            result.sigma = std::abs(result.observedOffset) * 1e-3 + 1e-30;
        }
    }

    return result;
}

std::vector<InstrumentFinding>
GravitationalWaveMemoryAnalyzer::analyze(
    const MetricTensor& metric,
    const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    (void)metric;
    std::vector<InstrumentFinding> findings;

    if (trajectory.size() < kMinSamples) return findings;

    double dt = (trajectory.size() > 1) ?
        (trajectory[1].t - trajectory[0].t) : 0.0;
    if (dt <= 0.0) return findings;

    for (const auto& pt : trajectory) {
        if (std::isnan(pt.x) || std::isinf(pt.x)) return findings;
    }

    double energy = getParameter("energy_radiated");
    double distance = getParameter("distance_mpc");
    double baselineDuration = getParameter("baseline_duration");
    double thresholdSigma = getParameter("threshold_sigma");

    MemoryResult mem = computeMemoryOffset(
        trajectory, baselineDuration, energy, distance);

    if (mem.sigma <= 0.0 || mem.snr <= 0.0) return findings;

    if (mem.snr > thresholdSigma) {
        InstrumentFinding finding;
        finding.id = "GWMA_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(std::min(1.0, mem.snr / 5.0));
        finding.confidence = std::min(1.0, mem.snr / 5.0);
        finding.isAnomaly = true;
        finding.description = "GW memory detected: Δh_obs=" +
            std::to_string(mem.observedOffset) + ", SNR=" +
            std::to_string(mem.snr) + "σ, expected Δh=" +
            std::to_string(mem.expectedOffset) + ".";
        finding.location = location;
        finding.timestamp = trajectory.back().t;
        finding.parameters["observed_offset"] = mem.observedOffset;
        finding.parameters["expected_offset"] = mem.expectedOffset;
        finding.parameters["snr"] = mem.snr;
        finding.parameters["noise_sigma"] = mem.sigma;
        finding.parameters["baseline_mean"] = mem.baselineMean;
        finding.parameters["post_mean"] = mem.postMean;
        finding.parameters["energy_radiated"] = energy;
        finding.parameters["distance_mpc"] = distance;
        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

} // namespace quantumverse
