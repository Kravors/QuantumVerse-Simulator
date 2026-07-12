// QuantumVerse Test: NeutronStarGlitchPhaseDetector
// TDD validation of glitch phase detection on a synthetic pulsar timing dataset.
// Builds a trajectory whose spin-rate proxy jumps suddenly (a glitch) and
// verifies the detector pinpoints the glitch time and magnitude.

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "discovery/NeutronStarGlitchPhaseDetector.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

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

    assert(!findings.empty() && "No glitch finding produced");

    const auto& f = findings.front();
    auto git = f.parameters.find("glitch_time");
    assert(git != f.parameters.end() && "glitch_time parameter missing");
    assert(std::abs(git->second - glitchTime) < 1.0 && "Glitch time mismatch");

    auto dit = f.parameters.find("delta_rate");
    assert(dit != f.parameters.end() && "delta_rate parameter missing");
    assert(std::abs(dit->second - (postRate - preRate)) < 0.01 && "delta_rate mismatch");

    std::cout << "Detected glitch at t=" << git->second
              << " s, delta_rate=" << dit->second << std::endl;

    // --- Negative case: constant spin rate yields no glitch -----------------
    std::vector<quantumverse::Event4D> steady;
    for (double t = 0.0; t <= 200.0; t += dt) {
        steady.emplace_back(t, preRate, 0.0, 0.0);
    }
    auto none = detector.analyze(metric, location, steady);
    assert(none.empty() && "Steady spin rate should not produce a glitch");

    // --- Edge case: too few points yields no finding -----------------------
    std::vector<quantumverse::Event4D> tiny = {
        quantumverse::Event4D(0.0, 1.0, 0.0, 0.0),
        quantumverse::Event4D(1.0, 1.1, 0.0, 0.0),
    };
    auto few = detector.analyze(metric, location, tiny);
    assert(few.empty() && "Too-few-point trajectory should not produce a finding");

    std::cout << "All NeutronStarGlitchPhaseDetector tests passed." << std::endl;
    return 0;
}
