// QuantumVerse Test: NeutronStarGlitchPhaseDetector
// TDD validation of glitch phase detection on a synthetic pulsar timing dataset.
// Builds a trajectory whose spin-rate proxy jumps suddenly (a glitch) and
// verifies the detector pinpoints the glitch time and magnitude.

#include <cmath>
#include <iostream>
#include <vector>

#include "discovery/NeutronStarGlitchPhaseDetector.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "test_assert.h"

int main() {
    std::cout << "=== NeutronStarGlitchPhaseDetector Test ===" << std::endl;

    // --- Synthetic glitch dataset -------------------------------------------
    // Spin rate is constant up to t = 100 s, then jumps (a glitch).
    const double glitchTime = 100.0;
    const double preRate = 1.0;
    const double postRate = 1.1;
    const double dt = 1.0;

    std::vector<quantumverse::Event4D> trajectory;
    for (double t = 0.0; t <= 200.0; t += dt) {
        double rate = (t < glitchTime) ? preRate : postRate;
        // The detector reads omega = sqrt(x^2 + y^2) / dt, so encoding the
        // spin rate directly in the radial coordinate makes omega == rate.
        trajectory.emplace_back(t, rate, 0.0, 0.0);
    }

    quantumverse::MetricTensor metric;
    quantumverse::Event4D location(glitchTime, 0.0, 0.0, 0.0);

    quantumverse::NeutronStarGlitchPhaseDetector detector;
    auto findings = detector.analyze(metric, location, trajectory);

    QV_CHECK(!findings.empty());

    const auto& f = findings.front();
    auto git = f.parameters.find("glitch_time");
    QV_CHECK(git != f.parameters.end());
    QV_CHECK_NEAR(git, >second - glitchTime, 1.0);

    auto dit = f.parameters.find("delta_rate");
    QV_CHECK(dit != f.parameters.end());
    QV_CHECK_NEAR(dit, >second - (postRate - preRate), 0.01);

    std::cout << "Detected glitch at t=" << git->second
              << " s, delta_rate=" << dit->second << std::endl;

    // --- Negative case: constant spin rate yields no glitch -----------------
    std::vector<quantumverse::Event4D> steady;
    for (double t = 0.0; t <= 200.0; t += dt) {
        steady.emplace_back(t, preRate, 0.0, 0.0);
    }
    auto none = detector.analyze(metric, location, steady);
    QV_CHECK(none.empty());

    // --- Edge case: too few points yields no finding -----------------------
    std::vector<quantumverse::Event4D> tiny = {
        quantumverse::Event4D(0.0, 1.0, 0.0, 0.0),
        quantumverse::Event4D(1.0, 1.1, 0.0, 0.0),
    };
    auto few = detector.analyze(metric, location, tiny);
    QV_CHECK(few.empty());

    std::cout << "All NeutronStarGlitchPhaseDetector tests passed." << std::endl;
    return 0;
}
