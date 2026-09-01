/**
 * @file FastRadioBurstAnalyzer.cpp
 * @brief Implementation of the Fast Radio Burst analyzer
 */

#include "FastRadioBurstAnalyzer.h"
#include <cmath>
#include <algorithm>
#include <numeric>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

FastRadioBurstAnalyzer::FastRadioBurstAnalyzer()
{
    setParameter("snr_threshold", 5.0);
    setParameter("time_window_seconds", 60.0);
    setParameter("dm_min", 100.0);
    setParameter("dm_max", 3000.0);
    setParameter("gw_trigger_time", 0.5);
}

std::map<std::string, std::pair<double, double>>
FastRadioBurstAnalyzer::getParameterRanges() const
{
    return {
        {"snr_threshold", {3.0, 10.0}},
        {"time_window_seconds", {1.0, 3600.0}},
        {"dm_min", {50.0, 500.0}},
        {"dm_max", {1000.0, 5000.0}},
        {"gw_trigger_time", {0.0, 10.0}}
    };
}

double FastRadioBurstAnalyzer::computeSNR(double peakFlux, double noiseSigma)
{
    if (noiseSigma <= 0.0) return 0.0;
    return peakFlux / noiseSigma;
}

double FastRadioBurstAnalyzer::estimateDM(double pulseWidthSec, double freqSweepMHz)
{
    if (freqSweepMHz <= 0.0 || pulseWidthSec <= 0.0) return 0.0;
    return pulseWidthSec * 100000.0;
}

double FastRadioBurstAnalyzer::estimateNoiseSigma(const std::vector<Event4D>& trajectory,
                                                 double& medianFlux)
{
    if (trajectory.size() < 4) return 0.0;

    std::vector<double> fluxes;
    fluxes.reserve(trajectory.size());
    for (const auto& evt : trajectory) {
        if (std::isfinite(evt.x)) {
            fluxes.push_back(evt.x);
        }
    }
    if (fluxes.size() < 4) return 0.0;

    // Median
    std::nth_element(fluxes.begin(), fluxes.begin() + fluxes.size() / 2, fluxes.end());
    medianFlux = fluxes[fluxes.size() / 2];

    // MAD (Median Absolute Deviation) as robust noise estimator
    std::vector<double> absDevs;
    absDevs.reserve(fluxes.size());
    for (double f : fluxes) {
        absDevs.push_back(std::abs(f - medianFlux));
    }
    std::nth_element(absDevs.begin(), absDevs.begin() + absDevs.size() / 2, absDevs.end());
    double mad = absDevs[absDevs.size() / 2];

    // Convert MAD to Gaussian sigma: sigma ≈ 1.4826 * MAD
    return 1.4826 * mad;
}

FastRadioBurstAnalyzer::BurstResult
FastRadioBurstAnalyzer::detectBurst(
    const std::vector<Event4D>& trajectory,
    double snrThreshold,
    const std::vector<double>& gwTriggerTimes,
    double timeWindowSec)
{
    BurstResult result;
    size_t n = trajectory.size();
    if (n < kMinSamples) return result;

    double dt = (n > 1) ? (trajectory[1].t - trajectory[0].t) : 0.0;
    if (dt <= 0.0) return result;

    double medianFlux = 0.0;
    double noiseSigma = estimateNoiseSigma(trajectory, medianFlux);
    if (noiseSigma <= 0.0) return result;

    // Find the peak flux
    double peakFlux = 0.0;
    size_t peakIdx = 0;
    for (size_t i = 0; i < n; ++i) {
        if (std::isnan(trajectory[i].x) || std::isinf(trajectory[i].x)) continue;
        if (trajectory[i].x > peakFlux) {
            peakFlux = trajectory[i].x;
            peakIdx = i;
        }
    }

    result.peakFlux = peakFlux;
    result.peakTime = trajectory[peakIdx].t;
    result.snr = computeSNR(peakFlux - medianFlux, noiseSigma);

    if (result.snr < snrThreshold) return result;

    // Estimate pulse duration (FWHM)
    double halfMax = medianFlux + (peakFlux - medianFlux) * 0.5;
    size_t leftIdx = peakIdx;
    size_t rightIdx = peakIdx;
    while (leftIdx > 0 && trajectory[leftIdx].x > halfMax) --leftIdx;
    while (rightIdx < n - 1 && trajectory[rightIdx].x > halfMax) ++rightIdx;
    result.duration = (rightIdx - leftIdx) * dt;

    // DM proxy from pulse duration (simplified dispersion sweep)
    double freqSweepMHz = 400.0; // Typical CHIME bandwidth
    result.dmProxy = estimateDM(result.duration, freqSweepMHz);

    // Check for GW coincidence: burst must fit within the time window
    for (double gwTime : gwTriggerTimes) {
        double offset = std::abs(result.peakTime - gwTime);
        if (offset + result.duration / 2.0 <= timeWindowSec) {
            result.hasCoincidence = true;
            result.gwTimeOffset = result.peakTime - gwTime;
            break;
        }
    }

    return result;
}

std::vector<InstrumentFinding>
FastRadioBurstAnalyzer::analyze(
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

    double snrThresh = getParameter("snr_threshold");
    double timeWindow = getParameter("time_window_seconds");
    double dmMin = getParameter("dm_min");
    double dmMax = getParameter("dm_max");
    double gwTrigger = getParameter("gw_trigger_time");

    std::vector<double> gwTriggers = {gwTrigger};

    BurstResult burst = detectBurst(trajectory, snrThresh, gwTriggers, timeWindow);

    if (burst.snr <= snrThresh) return findings;
    if (burst.dmProxy < dmMin || burst.dmProxy > dmMax) return findings;

    InstrumentFinding finding;
    finding.id = "FRB_" + std::to_string(getTotalFindings());
    finding.instrumentName = getName();
    finding.severity = confidenceToSeverity(std::min(1.0, burst.snr / 10.0));
    finding.confidence = std::min(1.0, burst.snr / 10.0);
    finding.isAnomaly = burst.hasCoincidence;

    if (burst.hasCoincidence) {
        finding.description = "FRB-GW coincidence: SNR=" +
            std::to_string(burst.snr) + ", DM=" +
            std::to_string(burst.dmProxy) + " pc/cm^3, dt=" +
            std::to_string(burst.gwTimeOffset) + "s, duration=" +
            std::to_string(burst.duration) + "s.";
    } else {
        finding.description = "FRB detected: SNR=" +
            std::to_string(burst.snr) + ", DM=" +
            std::to_string(burst.dmProxy) + " pc/cm^3, duration=" +
            std::to_string(burst.duration) + "s.";
    }

    finding.location = location;
    finding.timestamp = trajectory.back().t;
    finding.parameters["peak_flux"] = burst.peakFlux;
    finding.parameters["snr"] = burst.snr;
    finding.parameters["dm_proxy"] = burst.dmProxy;
    finding.parameters["duration"] = burst.duration;
    finding.parameters["peak_time"] = burst.peakTime;
    finding.parameters["has_coincidence"] = burst.hasCoincidence ? 1.0 : 0.0;
    finding.parameters["gw_time_offset"] = burst.gwTimeOffset;
    addFinding(finding);
    findings.push_back(finding);

    return findings;
}

} // namespace quantumverse
