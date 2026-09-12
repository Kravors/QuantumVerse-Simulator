// QuantumVerse Test: BlackHoleJetAnomalyRecogniser
// TDD validation of black-hole jet anomaly detection. Generates a steady jet
// along x with a sudden deflection (kink) at a known time, and verifies the
// detector flags the anomaly and reports its time/type.

#include <cmath>
#include <iostream>
#include <vector>

#include "discovery/BlackHoleJetAnomalyRecogniser.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "test_assert.h"

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

    QV_CHECK(!findings.empty());

    const auto& f = findings.front();
    bool mentions = f.description.find("jet anomaly") != std::string::npos ||
                    f.description.find("anomaly") != std::string::npos;
    QV_CHECK(mentions);
    (void)mentions;

    auto ait = f.parameters.find("anomaly_time");
    QV_CHECK(ait != f.parameters.end());
    QV_CHECK_NEAR(ait, >second - tAnomaly, 2.0);

    std::cout << "Detected jet "
              << (f.parameters.at("anomaly_type") > 0.5 ? "precession" : "deflection")
              << " at t=" << ait->second << std::endl;

    // --- Negative case: straight steady jet -> no finding -----------------
    std::vector<quantumverse::Event4D> straight;
    for (double t = 0.0; t <= 100.0; t += 1.0) {
        straight.emplace_back(t, t, 0.0, 0.0);
    }
    auto none = recogniser.analyze(metric, location, straight);
    QV_CHECK(none.empty());

    // --- Edge case: too few points ----------------------------------------
    std::vector<quantumverse::Event4D> tiny(5);
    auto few = recogniser.analyze(metric, location, tiny);
    QV_CHECK(few.empty());

    std::cout << "All BlackHoleJetAnomalyRecogniser tests passed." << std::endl;
    return 0;
}
