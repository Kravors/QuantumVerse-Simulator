/**
 * @file GWRingdownScanner.cpp
 * @brief Implementation of the GW ringdown quasi-normal mode scanner
 */

#include "GWRingdownScanner.h"
#include <cmath>
#include <algorithm>

namespace quantumverse {

GWRingdownScanner::GWRingdownScanner()
{
    setParameter("sigma_threshold", kSigmaThreshold);
    setParameter("spin", 0.0);
    setParameter("mass_solar", 1.0);
}

std::vector<InstrumentFinding> GWRingdownScanner::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    (void)metric;
    std::vector<InstrumentFinding> findings;

    if (trajectory.size() < kMinTrajectorySize) {
        return findings;
    }

    double spin = getParameter("spin");
    double massSolar = getParameter("mass_solar");

    double predictedReal = predictedFrequency(massSolar, spin);
    double predictedImag = schwarzschildQNMImag();

    double maxDeviation = 0.0;
    double maxDeviationImag = 0.0;
    size_t validPoints = 0;

    for (const auto& pt : trajectory) {
        double obsReal = pt.x;
        double obsImag = pt.y;
        double errReal = pt.z;
        double errImag = pt.t;

        if (std::isnan(obsReal) || std::isnan(obsImag) || std::isnan(errReal) || std::isnan(errImag) ||
            std::isinf(obsReal) || std::isinf(obsImag) || std::isinf(errReal) || std::isinf(errImag)) {
            continue;
        }

        if (errReal <= 0.0 && errImag <= 0.0) continue;

        double devReal = 0.0;
        double devImag = 0.0;

        if (errReal > 0.0) {
            devReal = std::abs(obsReal - predictedReal) / errReal;
        }
        if (errImag > 0.0) {
            devImag = std::abs(obsImag - predictedImag) / errImag;
        }

        double totalDev = std::sqrt(devReal * devReal + devImag * devImag);
        if (totalDev > maxDeviation) {
            maxDeviation = totalDev;
            maxDeviationImag = devImag;
        }
        validPoints++;
    }

    if (validPoints < kMinTrajectorySize) {
        return findings;
    }

    double threshold = getParameter("sigma_threshold");

    if (maxDeviation > threshold) {
        InstrumentFinding finding;
        finding.id = "GWRING_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        double confidence = std::min(1.0, (maxDeviation - threshold) / (2.0 * threshold));
        finding.severity = confidenceToSeverity(confidence);
        finding.confidence = confidence;
        finding.isAnomaly = true;
        finding.description = "GW ringdown anomaly: QNM frequency deviation of " +
                             std::to_string(maxDeviation) + " sigma detected. " +
                             "This may indicate a non-Kerr compact object or modified gravity.";
        finding.location = location;
        finding.timestamp = trajectory.back().t;
        finding.parameters["max_deviation_sigma"] = maxDeviation;
        finding.parameters["imag_deviation_sigma"] = maxDeviationImag;
        finding.parameters["threshold_sigma"] = threshold;
        finding.parameters["predicted_real"] = predictedReal;
        finding.parameters["predicted_imag"] = predictedImag;
        finding.parameters["spin"] = spin;
        finding.parameters["mass_solar"] = massSolar;
        finding.parameters["valid_points"] = static_cast<double>(validPoints);
        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

std::map<std::string, std::pair<double, double>> GWRingdownScanner::getParameterRanges() const
{
    return {
        {"sigma_threshold", {2.0, 5.0}},
        {"spin", {-1.0, 1.0}},
        {"mass_solar", {0.1, 100.0}}
    };
}

double GWRingdownScanner::schwarzschildQNMReal()
{
    return 0.37367;
}

double GWRingdownScanner::schwarzschildQNMImag()
{
    return -0.0890;
}

double GWRingdownScanner::predictedFrequency(double massSolar, double spin)
{
    double m = massSolar;
    double a = spin;
    double m3 = m * m * m;

    double omegaR = 1.0 / (3.0 * std::sqrt(3.0)) * (1.0 - 0.63 * a * a) / m3;
    return omegaR;
}

} // namespace quantumverse
