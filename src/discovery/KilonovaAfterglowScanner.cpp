/**
 * @file KilonovaAfterglowScanner.cpp
 * @brief Implementation of the kilonova afterglow scanner
 */

#include "KilonovaAfterglowScanner.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

namespace {
    // Physical constants
    constexpr double MPC_TO_CM = 3.08567758e24;   // cm/Mpc
    constexpr double DAY_TO_SEC = 86400.0;        // s/day

    // Kilonova model parameters (Metzger 2017, Kasen 2017)
    constexpr double L_PEAK_REF = 1.0e41;   // erg/s reference peak luminosity
    constexpr double M_EJ_REF = 0.01;      // solar masses reference ejecta mass
    constexpr double T_PEAK = 1.0;         // days, time of peak
    constexpr double ALPHA_RISE = 1.5;     // rise power-law index
    constexpr double ALPHA_DECAY = -1.3;   // decay power-law index (r-process)
    constexpr double SOLAR_L = 3.828e33;   // solar luminosity (erg/s)
    constexpr double ABS_MAG_SUN = 4.74;   // solar absolute bolometric magnitude
}

KilonovaAfterglowScanner::KilonovaAfterglowScanner()
{
    setParameter("gw_trigger_time", 0.5);
    setParameter("ejecta_mass_msun", 0.01);
    setParameter("distance_mpc", 100.0);
    setParameter("time_window_days", 7.0);
    setParameter("magnitude_limit", 24.0);
    setParameter("snr_threshold", 3.0);
}

std::map<std::string, std::pair<double, double>>
KilonovaAfterglowScanner::getParameterRanges() const
{
    return {
        {"gw_trigger_time", {0.0, 10.0}},
        {"ejecta_mass_msun", {0.001, 0.1}},
        {"distance_mpc", {1.0, 10000.0}},
        {"time_window_days", {0.1, 30.0}},
        {"magnitude_limit", {18.0, 28.0}},
        {"snr_threshold", {1.0, 5.0}}
    };
}

double KilonovaAfterglowScanner::kilonovaLuminosity(double ejectaMassMsun, double timeDay)
{
    if (ejectaMassMsun <= 0.0 || timeDay <= 0.0) return 0.0;

    double massRatio = ejectaMassMsun / M_EJ_REF;
    double L_peak = L_PEAK_REF * massRatio;

    double luminosity;
    if (timeDay < T_PEAK) {
        double ratio = timeDay / T_PEAK;
        luminosity = L_peak * std::pow(ratio, ALPHA_RISE);
    } else {
        double ratio = timeDay / T_PEAK;
        luminosity = L_peak * std::pow(ratio, ALPHA_DECAY);
    }

    return luminosity;
}

double KilonovaAfterglowScanner::apparentMagnitude(double luminosityErgS, double distanceMpc)
{
    if (luminosityErgS <= 0.0 || distanceMpc <= 0.0) return 99.0;
    // Absolute magnitude from luminosity
    double absMag = ABS_MAG_SUN - 2.5 * std::log10(luminosityErgS / SOLAR_L);
    // Distance modulus: m - M = 5 log10(d/10pc)
    double distancePc = distanceMpc * 1.0e6;
    double distModulus = 5.0 * std::log10(distancePc / 10.0);
    return absMag + distModulus;
}

KilonovaAfterglowScanner::AfterglowResult
KilonovaAfterglowScanner::computeAfterglow(
    const std::vector<Event4D>& trajectory,
    double gwTriggerTime,
    double ejectaMassMsun,
    double distanceMpc,
    double timeWindowDay,
    double magLimit)
{
    AfterglowResult result;
    size_t n = trajectory.size();
    if (n < kMinSamples) return result;

    double dt = (n > 1) ? (trajectory[1].t - trajectory[0].t) : 0.0;
    if (dt <= 0.0) return result;

    result.ejectaMass = ejectaMassMsun;

    // Find the post-merger phase (after GW trigger)
    size_t postStart = 0;
    for (size_t i = 0; i < n; ++i) {
        if (trajectory[i].t >= gwTriggerTime) {
            postStart = i;
            break;
        }
    }
    if (postStart >= n) return result;

    // Find peak in post-merger "light curve" (EM channel)
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

    // Compute kilonova model at peak time
    double timeDay = result.timeOffset / DAY_TO_SEC;
    if (timeDay > 0.0 && timeDay <= timeWindowDay) {
        result.peakLuminosity = kilonovaLuminosity(ejectaMassMsun, timeDay);
        result.magnitude = apparentMagnitude(result.peakLuminosity, distanceMpc);
    }

    return result;
}

std::vector<InstrumentFinding>
KilonovaAfterglowScanner::analyze(
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
    double ejectaMass = getParameter("ejecta_mass_msun");
    double distance = getParameter("distance_mpc");
    double timeWindow = getParameter("magnitude_limit");
    double magLimit = getParameter("magnitude_limit");
    double snrThresh = getParameter("snr_threshold");

    AfterglowResult afterglow = computeAfterglow(
        trajectory, gwTrigger, ejectaMass, distance, timeWindow, magLimit);

    if (afterglow.peakLuminosity <= 0.0) return findings;

    // Check for kilonova detection:
    // 1. Post-merger signal within time window
    // 2. Significance above SNR threshold
    // 3. Magnitude brighter than detection limit
    double timeDay = afterglow.timeOffset / DAY_TO_SEC;
    bool inTimeWindow = (timeDay >= 0.0 && timeDay <= timeWindow);
    bool aboveThreshold = (afterglow.significance > snrThresh);
    bool brightEnough = (afterglow.magnitude < magLimit);

    if (inTimeWindow && aboveThreshold && brightEnough) {
        InstrumentFinding finding;
        finding.id = "KILONOVA_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(std::min(1.0, afterglow.significance / 5.0));
        finding.confidence = std::min(1.0, afterglow.significance / 5.0);
        finding.isAnomaly = true;
        finding.description = "Kilonova afterglow detected: L_peak=" +
            std::to_string(afterglow.peakLuminosity) + " erg/s, m=" +
            std::to_string(afterglow.magnitude) + " mag, dt=" +
            std::to_string(timeDay) + " days, SNR=" +
            std::to_string(afterglow.significance) + ".";
        finding.location = location;
        finding.timestamp = trajectory.back().t;
        finding.parameters["peak_luminosity"] = afterglow.peakLuminosity;
        finding.parameters["peak_magnitude"] = afterglow.magnitude;
        finding.parameters["time_offset_days"] = timeDay;
        finding.parameters["significance"] = afterglow.significance;
        finding.parameters["ejecta_mass_msun"] = afterglow.ejectaMass;
        finding.parameters["distance_mpc"] = distance;
        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

} // namespace quantumverse
