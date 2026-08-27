/**
 * @file NeutrinoBurstAnalyzer.cpp
 * @brief Implementation of the multi-messenger neutrino-GW coincidence detector
 */

#include "NeutrinoBurstAnalyzer.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

namespace {
    // Physical constants in CGS
    constexpr double MPC_TO_CM = 3.08567758e24;   // cm/Mpc

    // Neutrino emission model parameters
    constexpr double L_NU_REF = 1.0e52;   // erg/s reference luminosity
    constexpr double M_DISK_REF = 0.01;   // solar masses reference
    constexpr double T_FALLBACK_IDX = 5.0 / 3.0; // fallback decay index
}

NeutrinoBurstAnalyzer::NeutrinoBurstAnalyzer()
{
    setParameter("gw_trigger_time", 0.5);
    setParameter("disk_mass_msun", 0.01);
    setParameter("distance_mpc", 100.0);
    setParameter("time_window", 10.0);
    setParameter("energy_threshold", 1.0e-8);
    setParameter("snr_threshold", 3.0);
}

std::map<std::string, std::pair<double, double>>
NeutrinoBurstAnalyzer::getParameterRanges() const
{
    return {
        {"gw_trigger_time", {0.0, 10.0}},
        {"disk_mass_msun", {0.001, 0.1}},
        {"distance_mpc", {1.0, 10000.0}},
        {"time_window", {0.1, 100.0}},
        {"energy_threshold", {1.0e-10, 1.0e-6}},
        {"snr_threshold", {1.0, 5.0}}
    };
}

double NeutrinoBurstAnalyzer::neutrinoLuminosity(double diskMassMsun, double timeSec)
{
    if (diskMassMsun <= 0.0 || timeSec <= 0.0) return 0.0;
    double massRatio = diskMassMsun / M_DISK_REF;
    double timeDecay = std::pow(timeSec, -T_FALLBACK_IDX);
    return L_NU_REF * massRatio * timeDecay;
}

double NeutrinoBurstAnalyzer::neutrinoFlux(double luminosity, double distanceMpc)
{
    if (luminosity <= 0.0 || distanceMpc <= 0.0) return 0.0;
    double distanceCm = distanceMpc * MPC_TO_CM;
    double area = 4.0 * M_PI * distanceCm * distanceCm;
    return luminosity / area;
}

NeutrinoBurstAnalyzer::CoincidenceResult
NeutrinoBurstAnalyzer::computeCoincidence(
    const std::vector<Event4D>& trajectory,
    double gwTriggerTime,
    double diskMassMsun,
    double distanceMpc,
    double timeWindowSec,
    double energyThreshold)
{
    CoincidenceResult result;
    size_t n = trajectory.size();
    if (n < kMinSamples) return result;

    double dt = (n > 1) ? (trajectory[1].t - trajectory[0].t) : 0.0;
    if (dt <= 0.0) return result;

    result.diskMass = diskMassMsun;

    // Find the post-merger phase (after GW trigger)
    size_t postStart = 0;
    for (size_t i = 0; i < n; ++i) {
        if (trajectory[i].t >= gwTriggerTime) {
            postStart = i;
            break;
        }
    }
    if (postStart >= n) return result;

    // Compute neutrino luminosity at the trigger time
    double t0 = 0.01; // 10ms after trigger (avoid t=0 singularity)
    result.neutrinoLuminosity = neutrinoLuminosity(diskMassMsun, t0);
    result.neutrinoFlux = neutrinoFlux(result.neutrinoLuminosity, distanceMpc);

    // Find peak in post-merger strain (neutrino-heated ejecta signature)
    double peakVal = 0.0;
    size_t peakIdx = postStart;
    for (size_t i = postStart; i < n; ++i) {
        if (std::isnan(trajectory[i].x) || std::isinf(trajectory[i].x)) continue;
        double absVal = std::abs(trajectory[i].x);
        if (absVal > peakVal) {
            peakVal = absVal;
            peakIdx = i;
        }
    }
    result.peakTime = trajectory[peakIdx].t;
    result.timeOffset = result.peakTime - gwTriggerTime;

    // Compute noise level from pre-trigger baseline
    double baselineMean = 0.0;
    size_t baselineCount = 0;
    for (size_t i = 0; i < postStart && i < n; ++i) {
        baselineMean += trajectory[i].x;
        baselineCount++;
    }
    if (baselineCount > 0) baselineMean /= static_cast<double>(baselineCount);

    double noiseSigma = 0.0;
    for (size_t i = 0; i < postStart && i < n; ++i) {
        double d = trajectory[i].x - baselineMean;
        noiseSigma += d * d;
    }
    if (baselineCount > 1) {
        noiseSigma = std::sqrt(noiseSigma / static_cast<double>(baselineCount - 1));
    }

    // Significance of the post-merger peak
    if (noiseSigma > 0.0) {
        result.significance = (peakVal > std::abs(baselineMean)) ?
            (peakVal - std::abs(baselineMean)) / noiseSigma : 0.0;
    }

    return result;
}

std::vector<InstrumentFinding>
NeutrinoBurstAnalyzer::analyze(
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

    double gwTrigger = getParameter("gw_trigger_time");
    double diskMass = getParameter("disk_mass_msun");
    double distance = getParameter("distance_mpc");
    double timeWindow = getParameter("time_window");
    double energyThresh = getParameter("energy_threshold");
    double snrThresh = getParameter("snr_threshold");

    CoincidenceResult coin = computeCoincidence(
        trajectory, gwTrigger, diskMass, distance, timeWindow, energyThresh);

    if (coin.neutrinoFlux <= 0.0) return findings;

    // Check for multi-messenger coincidence:
    // 1. Post-merger signal within time window
    // 2. Significance above SNR threshold
    // 3. Neutrino flux above energy threshold
    bool inTimeWindow = (coin.timeOffset >= 0.0 && coin.timeOffset <= timeWindow);
    bool aboveThreshold = (coin.significance > snrThresh);
    bool fluxAboveThreshold = (coin.neutrinoFlux > energyThresh);

    if (inTimeWindow && aboveThreshold && fluxAboveThreshold) {
        InstrumentFinding finding;
        finding.id = "NBURST_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(std::min(1.0, coin.significance / 5.0));
        finding.confidence = std::min(1.0, coin.significance / 5.0);
        finding.isAnomaly = true;
        finding.description = "Neutrino-GW coincidence: L_nu=" +
            std::to_string(coin.neutrinoLuminosity) + " erg/s, F_nu=" +
            std::to_string(coin.neutrinoFlux) + " erg/s/cm^2, dt=" +
            std::to_string(coin.timeOffset) + "s, SNR=" +
            std::to_string(coin.significance) + ".";
        finding.location = location;
        finding.timestamp = trajectory.back().t;
        finding.parameters["neutrino_luminosity"] = coin.neutrinoLuminosity;
        finding.parameters["neutrino_flux"] = coin.neutrinoFlux;
        finding.parameters["time_offset"] = coin.timeOffset;
        finding.parameters["significance"] = coin.significance;
        finding.parameters["peak_time"] = coin.peakTime;
        finding.parameters["disk_mass_msun"] = coin.diskMass;
        finding.parameters["distance_mpc"] = distance;
        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

} // namespace quantumverse
