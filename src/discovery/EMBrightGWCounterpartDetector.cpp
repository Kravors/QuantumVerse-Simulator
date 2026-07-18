/**
 * @file EMBrightGWCounterpartDetector.cpp
 * @brief Implementation of the EM-bright GW counterpart detector
 */

#include "EMBrightGWCounterpartDetector.h"
#include <cmath>
#include <algorithm>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

EMBrightGWCounterpartDetector::EMBrightGWCounterpartDetector()
{
    setParameter("angular_threshold_deg", 5.0);
    setParameter("time_window_sec", 600.0);
    setParameter("min_confidence", 0.7);
    setParameter("gw_weight", 1.0);
    setParameter("em_weight", 1.0);
}

std::vector<InstrumentFinding> EMBrightGWCounterpartDetector::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    (void)metric;
    std::vector<InstrumentFinding> findings;

    double angThreshold = getParameter("angular_threshold_deg");
    double timeWindow = getParameter("time_window_sec");
    double minConf = getParameter("min_confidence");

    size_t n = trajectory.size();
    if (n < 2) return findings;

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            const Event4D& a = trajectory[i];
            const Event4D& b = trajectory[j];

            double angSep = angularSeparation(a, b);
            double dt = timeDelaySec(a, b);

            if (angSep > angThreshold) continue;
            if (std::abs(dt) > timeWindow) continue;

            double angScore = 1.0 - std::min(angSep / angThreshold, 1.0);
            double timeScore = 1.0 - std::min(std::abs(dt) / timeWindow, 1.0);
            double confidence = std::max(0.0, std::min(1.0,
                0.5 * angScore + 0.5 * timeScore));

            if (confidence < minConf) continue;

            InstrumentFinding finding;
            finding.id = "EM_COUNTERPART_" + std::to_string(getTotalFindings());
            finding.instrumentName = getName();
            finding.severity = confidenceToSeverity(confidence);
            finding.confidence = confidence;
            finding.location = location;
            finding.timestamp = std::max(a.t, b.t);

            finding.description = "GW-EM counterpart candidate: angular separation = "
                + std::to_string(angSep) + " deg, time delay = "
                + std::to_string(dt) + " s, confidence = "
                + std::to_string(confidence);
            finding.parameters["angular_sep_deg"] = angSep;
            finding.parameters["time_delay_sec"] = dt;
            finding.parameters["confidence"] = confidence;
            finding.parameters["ang_score"] = angScore;
            finding.parameters["time_score"] = timeScore;

            addFinding(finding);
            findings.push_back(finding);
        }
    }

    return findings;
}

double EMBrightGWCounterpartDetector::angularSeparation(const Event4D& a, const Event4D& b)
{
    double dra = a.x - b.x;
    double ddec = a.y - b.y;
    double cosDec = std::cos(a.y * M_PI / 180.0);
    double sep2 = (dra * cosDec) * (dra * cosDec) + ddec * ddec;
    return std::sqrt(sep2);
}

double EMBrightGWCounterpartDetector::timeDelaySec(const Event4D& gw, const Event4D& em)
{
    return em.t - gw.t;
}

} // namespace quantumverse
