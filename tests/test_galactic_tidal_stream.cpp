// QuantumVerse Test: GalacticTidalStreamCartographer
// TDD validation of tidal-stream anomaly detection. Generates a smooth stream
// path with a sudden kink (deflection) at a known time and verifies the
// detector flags the anomaly and reports its location/type.

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "discovery/GalacticTidalStreamCartographer.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

int main() {
    std::cout << "=== GalacticTidalStreamCartographer Test ===" << std::endl;

    const double pi = 3.14159265358979323846;
    const double tAnomaly = 50.0;
    const double slope = std::tan(20.0 * pi / 180.0); // ~0.364 -> 20 deg kink

    // --- Synthetic stream: smooth path with a kink at tAnomaly ----------
    std::vector<quantumverse::Event4D> trajectory;
    for (double t = 0.0; t <= 100.0; t += 1.0) {
        double y = (t >= tAnomaly) ? ((t - tAnomaly) * slope) : 0.0;
        trajectory.emplace_back(t, t, y, 0.0); // x=t advancing, y=deflection
    }

    quantumverse::MetricTensor metric;
    quantumverse::Event4D location(0.0, 0.0, 0.0, 0.0);

    quantumverse::GalacticTidalStreamCartographer cartographer;
    auto findings = cartographer.analyze(metric, location, trajectory);

    assert(!findings.empty() && "No stream anomaly detected");

    const auto& f = findings.front();
    bool mentions = f.description.find("stream") != std::string::npos ||
                    f.description.find("anomaly") != std::string::npos;
    assert(mentions && "Finding does not mention stream anomaly");

    auto ait = f.parameters.find("anomaly_time");
    assert(ait != f.parameters.end() && "anomaly_time parameter missing");
    assert(std::abs(ait->second - tAnomaly) < 2.0 && "Anomaly time mismatch");

    std::cout << "Detected stream "
              << (f.parameters.at("anomaly_type") > 0.5 ? "gap" : "kink")
              << " at t=" << ait->second << std::endl;

    // --- Negative case: perfectly smooth straight stream -> no finding ----
    std::vector<quantumverse::Event4D> smooth;
    for (double t = 0.0; t <= 100.0; t += 1.0) {
        smooth.emplace_back(t, t, 0.0, 0.0);
    }
    auto none = cartographer.analyze(metric, location, smooth);
    assert(none.empty() && "Smooth straight stream should not trigger a finding");

    // --- Edge case: too few points ----------------------------------------
    std::vector<quantumverse::Event4D> tiny(5);
    auto few = cartographer.analyze(metric, location, tiny);
    assert(few.empty() && "Too-few-point trajectory should not produce a finding");

    std::cout << "All GalacticTidalStreamCartographer tests passed." << std::endl;
    return 0;
}
