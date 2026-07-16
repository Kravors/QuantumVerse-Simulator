/**
 * @file PrimordialLithiumCrisisSolver.cpp
 * @brief Implementation of the primordial lithium crisis solver
 */

#include "PrimordialLithiumCrisisSolver.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

PrimordialLithiumCrisisSolver::PrimordialLithiumCrisisSolver()
{
    setParameter("expected_abundance", 5.0e-10);
    setParameter("deviation_threshold", 0.04);
}

std::vector<InstrumentFinding> PrimordialLithiumCrisisSolver::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    (void)metric;
    std::vector<InstrumentFinding> findings;
    if (trajectory.size() < 5) return findings;

    double expected = getParameter("expected_abundance");
    double devThreshold = getParameter("deviation_threshold");
    if (expected == 0.0) return findings;

    // Extract the measured Li-7 abundance series. Convention: x = Li/H, t = time.
    double sum = 0.0;
    for (const auto& ev : trajectory) sum += ev.x;
    double measured = sum / static_cast<double>(trajectory.size());

    double relDev = (measured - expected) / expected;
    if (std::abs(relDev) <= devThreshold) return findings;

    // Suggest a resolution based on the sign of the deviation.
    std::string resolution = (measured < expected)
        ? "lower baryon-to-photon ratio (eta) or varying fine-structure constant"
        : "higher baryon-to-photon ratio or extra relativistic degrees of freedom";

    double confidence = std::min(1.0, std::abs(relDev) / (2.0 * devThreshold));

    InstrumentFinding finding;
    finding.id = "PLCS_" + std::to_string(getTotalFindings());
    finding.instrumentName = getName();
    finding.severity = confidenceToSeverity(confidence);
    finding.confidence = confidence;
    finding.description = "Primordial lithium-7 crisis detected: measured Li-7/H = " +
        std::to_string(measured) + " vs BBN prediction " + std::to_string(expected) +
        " (rel deviation " + std::to_string(relDev * 100.0) + "%). Possible resolution: " +
        resolution + ".";
    finding.location = location;
    finding.timestamp = location.t;
    finding.parameters["measured_abundance"] = measured;
    finding.parameters["expected_abundance"] = expected;
    finding.parameters["relative_deviation"] = relDev;
    addFinding(finding);
    findings.push_back(finding);

    return findings;
}

} // namespace quantumverse
