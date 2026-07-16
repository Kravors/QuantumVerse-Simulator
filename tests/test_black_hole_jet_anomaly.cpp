// QuantumVerse Test: BlackHoleJetAnomalyRecogniser
// TDD validation of black-hole jet anomaly detection. Generates a steady jet
// along x with a sudden deflection (kink) at a known time, and verifies the
// detector flags the anomaly and reports its time/type.

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "discovery/BlackHoleJetAnomalyRecogniser.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

int main() {
    std::cout << "=== BlackHoleJetAnomalyRecogniser Test ===" << std::endl;

    const double pi = 3.14159265358979323846;
    const double tAnomaly = 50.0;
    const double slope = std::tan(20.0 * pi / 180.0); // ~0.364 -> 20 deg bend

    // --- Synthetic jet with a deflection (kink) at tAnomaly ---------------
    std::vector<quantumverse::Event4D> trajectory;
    for (double t = 0.0; t <= 100.0; t += 1.0) {
        double y = (t >= tAnomaly) ? ((t - tAnomaly) * slope) : 0.0;
        trajectory.emplace_back(t, t, y, 0.0); // x=t advancing, y=deflection, z=0
    }

    quantumverse::MetricTensor metric;
    quantumverse::Event4D location(0.0, 0.0, 0.0, 0.0);

    quantumverse::BlackHoleJetAnomalyRecogniser recogniser;
    auto findings = recogniser.analyze(metric, location, trajectory);

    assert(!findings.empty() && "No jet anomaly detected");

    const auto& f = findings.front();
    bool mentions = f.description.find("jet anomaly") != std::string::npos ||
                    f.description.find("anomaly") != std::string::npos;
    assert(mentions && "Finding does not mention jet anomaly");
    (void)mentions;

    auto ait = f.parameters.find("anomaly_time");
    assert(ait != f.parameters.end() && "anomaly_time parameter missing");
    assert(std::abs(ait->second - tAnomaly) < 2.0 && "Anomaly time mismatch");

    std::cout << "Detected jet "
              << (f.parameters.at("anomaly_type") > 0.5 ? "precession" : "deflection")
              << " at t=" << ait->second << std::endl;

    // --- Negative case: straight steady jet -> no finding -----------------
    std::vector<quantumverse::Event4D> straight;
    for (double t = 0.0; t <= 100.0; t += 1.0) {
        straight.emplace_back(t, t, 0.0, 0.0);
    }
    auto none = recogniser.analyze(metric, location, straight);
    assert(none.empty() && "Straight jet should not trigger a finding");

    // --- Edge case: too few points ----------------------------------------
    std::vector<quantumverse::Event4D> tiny(5);
    auto few = recogniser.analyze(metric, location, tiny);
    assert(few.empty() && "Too-few-point trajectory should not produce a finding");

    std::cout << "All BlackHoleJetAnomalyRecogniser tests passed." << std::endl;
    return 0;
}
