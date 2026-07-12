/**
 * @file GalacticTidalStreamCartographer.cpp
 * @brief Implementation of the tidal stream dark matter cartographer
 */

#include "GalacticTidalStreamCartographer.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

GalacticTidalStreamCartographer::GalacticTidalStreamCartographer()
{
    setParameter("kink_threshold_deg", 5.0);
    setParameter("gap_threshold", 5.0);
    setParameter("min_points", 8.0);
}

std::vector<InstrumentFinding> GalacticTidalStreamCartographer::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    std::vector<InstrumentFinding> findings;
    double minPoints = getParameter("min_points");
    if (trajectory.size() < static_cast<size_t>(minPoints)) return findings;

    double kinkThreshold = getParameter("kink_threshold_deg");
    double gapThreshold = getParameter("gap_threshold");

    // Build segment direction vectors and lengths from the stream path.
    struct Seg { double t; Event4D v; double len; };
    std::vector<Seg> segs;
    for (size_t i = 1; i < trajectory.size(); ++i) {
        double dt = trajectory[i].t - trajectory[i - 1].t;
        if (dt <= 0) continue;
        Event4D v = trajectory[i] - trajectory[i - 1];
        double len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        segs.push_back({trajectory[i].t, v, len});
    }
    if (segs.size() < 3) return findings;

    // Baseline (first segment) direction and median segment length.
    const Event4D& base = segs[0].v;
    double baseMag = std::sqrt(base.x * base.x + base.y * base.y + base.z * base.z);
    if (baseMag < 1e-12) return findings;

    std::vector<double> lengths;
    lengths.reserve(segs.size());
    for (const auto& s : segs) lengths.push_back(s.len);
    std::vector<double> sorted = lengths;
    std::sort(sorted.begin(), sorted.end());
    double medianLen = sorted[sorted.size() / 2];

    double anomalyTime = 0.0;
    double anomalyAngle = 0.0;
    std::string type;

    // Scan for a gap (unusually large spacing => missing stars) first.
    for (size_t i = 1; i < segs.size(); ++i) {
        if (anomalyTime == 0.0 && segs[i].len > gapThreshold * (medianLen + 1e-12)) {
            anomalyTime = segs[i].t;
            anomalyAngle = 0.0;
            type = "gap";
            break;
        }
    }

    // Scan for a kink (sudden change in stream direction).
    for (size_t i = 1; i < segs.size(); ++i) {
        const Event4D& v = segs[i].v;
        double mag = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        if (mag < 1e-12) continue;
        double dot = base.x * v.x + base.y * v.y + base.z * v.z;
        double cosAng = std::max(-1.0, std::min(1.0, dot / (baseMag * mag)));
        double angDeg = std::acos(cosAng) * 180.0 / M_PI;
        if (angDeg > kinkThreshold && anomalyTime == 0.0) {
            anomalyTime = segs[i].t;
            anomalyAngle = angDeg;
            type = "kink";
        }
    }

    if (anomalyTime > 0.0) {
        double confidence = (type == "gap")
            ? 0.9
            : std::min(1.0, 0.6 + 0.4 * (anomalyAngle / (kinkThreshold * 4.0)));

        InstrumentFinding finding;
        finding.id = "GTSC_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(confidence);
        finding.confidence = confidence;
        finding.description = "Tidal stream anomaly detected at t = " +
            std::to_string(anomalyTime) + " (type: " + type +
            "), suggesting a dark matter subhalo perturbation.";
        finding.location = location;
        finding.timestamp = anomalyTime;
        finding.parameters["anomaly_time"] = anomalyTime;
        finding.parameters["anomaly_angle_deg"] = anomalyAngle;
        finding.parameters["anomaly_type"] = (type == "gap") ? 1.0 : 0.0;
        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

} // namespace quantumverse
