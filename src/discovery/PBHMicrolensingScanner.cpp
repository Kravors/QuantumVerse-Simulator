/**
 * @file PBHMicrolensingScanner.cpp
 * @brief Implementation of the primordial black-hole microlensing scanner
 */

#include "PBHMicrolensingScanner.h"
#include <cmath>
#include <algorithm>
#include <utility>
#include <vector>

namespace quantumverse {

double PBHMicrolensingScanner::paczynskiMagnification(double u)
{
    if (u <= 0.0) return 1.0;
    double u2 = u * u;
    return (u2 + 2.0) / (u * std::sqrt(u2 + 4.0));
}

PBHMicrolensingScanner::PBHMicrolensingScanner()
{
    setParameter("min_magnification", 1.1);
    setParameter("fit_ratio_threshold", 0.6);
    // Geometry / kinematics priors used to convert a fitted tE into a mass.
    setParameter("lens_distance_kpc", 4.0);     // assumed PBH distance
    setParameter("source_distance_kpc", 8.0);   // assumed background-star distance
    setParameter("lens_velocity_kms", 220.0);  // assumed transverse velocity
    setParameter("seconds_per_time_unit", 86400.0); // light-curve time unit (days)
}

std::vector<InstrumentFinding> PBHMicrolensingScanner::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    (void)metric;
    std::vector<InstrumentFinding> findings;
    if (trajectory.size() < kMinTrajectorySize) return findings;

    double minMag = getParameter("min_magnification");
    double fitRatioThreshold = getParameter("fit_ratio_threshold");

    // Validate samples, skipping NaN/Inf in the time or flux channels.
    std::vector<std::pair<double, double>> samples;
    samples.reserve(trajectory.size());
    for (const auto& pt : trajectory) {
        if (std::isnan(pt.t) || std::isnan(pt.x) || std::isinf(pt.t) || std::isinf(pt.x)) {
            continue;
        }
        samples.emplace_back(pt.t, pt.x);
    }
    if (samples.size() < kMinTrajectorySize) return findings;
    std::sort(samples.begin(), samples.end());

    // Robust baseline flux (median).
    std::vector<double> sx;
    sx.reserve(samples.size());
    for (const auto& s : samples) sx.push_back(s.second);
    std::sort(sx.begin(), sx.end());
    double F0 = sx[sx.size() / 2];
    if (F0 <= 0.0) return findings;

    double tMin = samples.front().first;
    double tMax = samples.back().first;
    double T = tMax - tMin;
    if (T <= 0.0) return findings;

    // Baseline (constant-flux) residual.
    double baseSSE = 0.0;
    for (const auto& s : samples) {
        double d = s.second - F0;
        baseSSE += d * d;
    }
    double baseRMS = std::sqrt(baseSSE / samples.size());
    if (baseRMS <= 0.0) return findings; // perfectly flat: no event

    // Grid search for the best Paczynski microlensing fit. The magnification
    // is always >= 1, so a darkening (dip) or asymmetric flare cannot be fit,
    // keeping the detector specific to brightening lensing events.
    double bestSSE = baseSSE;
    double bestU0 = 1.0;
    double bestTE = T;
    double bestT0 = (tMin + tMax) * 0.5;
    const int nU0 = 12, nTE = 14, nT0 = 20;
    for (int iu = 0; iu < nU0; ++iu) {
        double u0 = 0.05 + (1.5 - 0.05) * iu / (nU0 - 1);
        for (int it = 0; it < nTE; ++it) {
            double tE = 0.1 * T + (T - 0.1 * T) * it / (nTE - 1);
            if (tE <= 0.0) continue;
            for (int ic = 0; ic < nT0; ++ic) {
                double t0 = tMin + T * ic / (nT0 - 1);
                double sse = 0.0;
                for (const auto& s : samples) {
                    double u = std::sqrt(u0 * u0 + ((s.first - t0) / tE) * ((s.first - t0) / tE));
                    double fPred = F0 * paczynskiMagnification(u);
                    double d = s.second - fPred;
                    sse += d * d;
                }
                if (sse < bestSSE) {
                    bestSSE = sse;
                    bestU0 = u0;
                    bestTE = tE;
                    bestT0 = t0;
                }
            }
        }
    }

    double resRMS = std::sqrt(bestSSE / samples.size());
    double ratio = resRMS / baseRMS;
    double A0 = paczynskiMagnification(bestU0);

    if (ratio > fitRatioThreshold) return findings; // fit does not explain the variance
    if (A0 < minMag) return findings;                // magnification too weak

    double confidence = std::min(1.0, std::max(0.0, 1.0 - ratio));

    double secPerUnit = getParameter("seconds_per_time_unit");
    double lensDist = getParameter("lens_distance_kpc");
    double srcDist = getParameter("source_distance_kpc");
    double velKms = getParameter("lens_velocity_kms");

    PBHEstimate est = estimatePBH(bestTE * secPerUnit, bestU0,
        lensDist, srcDist, velKms, confidence);

    InstrumentFinding f;
    f.id = "PBHML_" + std::to_string(getTotalFindings());
    f.instrumentName = getName();
    f.severity = confidenceToSeverity(confidence);
    f.confidence = confidence;
    f.isAnomaly = true;
    f.description = "Primordial black-hole microlensing candidate: symmetric "
        "Paczynski magnification bump with peak A=" + std::to_string(A0) +
        " at t=" + std::to_string(bestT0) + " (Einstein-radius crossing time tE=" +
        std::to_string(bestTE) + ", impact parameter u0=" + std::to_string(bestU0) +
        "). A dark, compact lens is consistent with a primordial black hole. "
        "Assuming D_l=" + std::to_string(lensDist) + " kpc, D_s=" +
        std::to_string(srcDist) + " kpc, v_t=" + std::to_string(velKms) +
        " km/s, the lens mass is M ~ " + std::to_string(est.mass_solar) +
        " M_sun (R_E=" + std::to_string(est.einstein_radius_au) + " AU).";
    f.location = location;
    f.timestamp = bestT0;
    f.parameters["event_time"] = bestT0;
    f.parameters["einstein_time"] = bestTE;
    f.parameters["impact_parameter_u0"] = bestU0;
    f.parameters["peak_magnification"] = A0;
    f.parameters["fit_ratio"] = ratio;
    f.parameters["baseline_rms"] = baseRMS;
    f.parameters["residual_rms"] = resRMS;
    f.parameters["baseline_flux"] = F0;
    f.parameters["pbh_mass_solar"] = est.mass_solar;
    f.parameters["pbh_velocity_kms"] = est.velocity_kms;
    f.parameters["einstein_radius_au"] = est.einstein_radius_au;
    f.parameters["lens_distance_kpc"] = est.lens_distance_kpc;
    f.parameters["source_distance_kpc"] = est.source_distance_kpc;
    addFinding(f);
    findings.push_back(f);
    return findings;
}

std::map<std::string, std::pair<double, double>> PBHMicrolensingScanner::getParameterRanges() const
{
    return {
        {"min_magnification", {1.05, 3.0}},
        {"fit_ratio_threshold", {0.3, 0.8}},
        {"lens_distance_kpc", {0.1, 50.0}},
        {"source_distance_kpc", {1.0, 100.0}},
        {"lens_velocity_kms", {10.0, 1000.0}},
        {"seconds_per_time_unit", {1.0, 3.15576e7}}
    };
}

PBHMicrolensingScanner::PBHEstimate PBHMicrolensingScanner::estimatePBH(
    double tE, double u0, double lensDistanceKpc,
    double sourceDistanceKpc, double lensVelocityKms, double confidence)
{
    (void)u0;
    PBHEstimate est;
    est.confidence = confidence;
    est.lens_distance_kpc = lensDistanceKpc;
    est.source_distance_kpc = sourceDistanceKpc;
    est.velocity_kms = lensVelocityKms;

    double Dls = sourceDistanceKpc - lensDistanceKpc;
    if (tE <= 0.0 || lensDistanceKpc <= 0.0 || sourceDistanceKpc <= 0.0 ||
        lensVelocityKms <= 0.0 || Dls <= 0.0) {
        return est; // degenerate/ill-posed geometry: no estimate
    }

    const double G = 6.67430e-11;      // m^3 kg^-1 s^-2
    const double c = 299792458.0;      // m/s
    const double Msun = 1.98847e30;    // kg
    const double kpc = 3.085677581e19; // m
    const double au = 1.495978707e11;  // m

    const double v = lensVelocityKms * 1000.0;  // m/s
    const double RE = v * tE;                   // R_E = v_t * t_E  (m)
    const double Dl_m = lensDistanceKpc * kpc;
    const double Dls_m = Dls * kpc;
    const double Ds_m = sourceDistanceKpc * kpc;

    // M = R_E^2 c^2 D_s / (4 G D_l D_ls)
    const double M = (RE * RE) * (c * c) * Ds_m / (4.0 * G * Dl_m * Dls_m);

    est.mass_solar = M / Msun;
    est.einstein_radius_au = RE / au;
    return est;
}

} // namespace quantumverse
