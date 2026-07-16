/**
 * @file BlackHoleJetAnomalyRecogniser.cpp
 * @brief Implementation of the non-Kerr jet anomaly recogniser
 */

#include "BlackHoleJetAnomalyRecogniser.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

BlackHoleJetAnomalyRecogniser::BlackHoleJetAnomalyRecogniser()
{
    setParameter("deflection_threshold_deg", 5.0);
    setParameter("min_points", 8.0);
}

std::vector<InstrumentFinding> BlackHoleJetAnomalyRecogniser::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    (void)metric;
    std::vector<InstrumentFinding> findings;
    double minPoints = getParameter("min_points");
    if (trajectory.size() < static_cast<size_t>(minPoints)) return findings;

    double deflThreshold = getParameter("deflection_threshold_deg");

    // Build direction vectors between consecutive samples.
    std::vector<std::pair<double, Event4D>> dirs;
    for (size_t i = 1; i < trajectory.size(); ++i) {
        double dt = trajectory[i].t - trajectory[i - 1].t;
        if (dt <= 0) continue;
        dirs.emplace_back(trajectory[i].t, trajectory[i] - trajectory[i - 1]);
    }
    if (dirs.size() < 3) return findings;

    // Baseline (steady jet) direction from the first segment.
    const Event4D& base = dirs[0].second;
    double baseMag = std::sqrt(base.x * base.x + base.y * base.y + base.z * base.z);
    if (baseMag < 1e-12) return findings;

    // Deviation of each direction from the baseline: angle + signed cross component.
    double anomalyTime = 0.0;
    double anomalyAngle = 0.0;
    int signChanges = 0;
    int lastSign = 0;
    for (size_t i = 1; i < dirs.size(); ++i) {
        const Event4D& v = dirs[i].second;
        double mag = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        if (mag < 1e-12) continue;
        double dot = base.x * v.x + base.y * v.y + base.z * v.z;
        double cosAng = std::max(-1.0, std::min(1.0, dot / (baseMag * mag)));
        double angDeg = std::acos(cosAng) * 180.0 / M_PI;

        // Signed perpendicular component (base x v).y, for precession detection.
        double crossY = base.z * v.x - base.x * v.z;
        int sign = (crossY > 1e-15) ? 1 : ((crossY < -1e-15) ? -1 : 0);
        if (sign != 0 && lastSign != 0 && sign != lastSign) signChanges++;
        if (sign != 0) lastSign = sign;

        if (angDeg > deflThreshold && anomalyTime == 0.0) {
            anomalyTime = dirs[i].first;
            anomalyAngle = angDeg;
        }
    }

    if (anomalyTime > 0.0) {
        std::string type = (signChanges >= 2) ? "precession" : "deflection";
        double confidence = std::min(1.0, 0.6 + 0.4 * (anomalyAngle / (deflThreshold * 4.0)));

        InstrumentFinding finding;
        finding.id = "BHJA_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(confidence);
        finding.confidence = confidence;
        finding.description = "Black hole jet anomaly detected at t = " +
            std::to_string(anomalyTime) + " (type: " + type +
            "), deviation " + std::to_string(anomalyAngle) +
            " deg from the steady jet axis.";
        finding.location = location;
        finding.timestamp = anomalyTime;
        finding.parameters["anomaly_time"] = anomalyTime;
        finding.parameters["anomaly_angle_deg"] = anomalyAngle;
        finding.parameters["anomaly_type"] = (type == "precession") ? 1.0 : 0.0;
        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

} // namespace quantumverse
