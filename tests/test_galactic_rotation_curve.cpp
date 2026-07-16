// QuantumVerse Test: GalacticRotationCurveScanner
// TDD validation of dark-matter halo detection from a galactic rotation curve.
// Inner region follows a Keplerian fall-off (v = A / sqrt(r)); the outer
// region flattens to a constant velocity, simulating dark-matter dominance.
// Convention: radius is carried in (x, y), circular velocity in z.

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "discovery/GalacticRotationCurveScanner.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

int main() {
    std::cout << "=== GalacticRotationCurveScanner Test ===" << std::endl;

    // --- Synthetic rotation curve ------------------------------------------
    const double rFlat = 20.0;       // radius where flattening begins
    const double aKepler = 10.0;     // inner Keplerian scale: v = aKepler / sqrt(r)
    const double vFlat = 8.0;        // outer flat velocity

    std::vector<quantumverse::Event4D> trajectory;
    for (double r = 1.0; r <= 100.0; r += 1.0) {
        double v = (r < rFlat) ? (aKepler / std::sqrt(r)) : vFlat;
        trajectory.emplace_back(r, r, 0.0, v); // x=r, y=0, z=v
    }

    quantumverse::MetricTensor metric;
    quantumverse::Event4D location(0.0, 0.0, 0.0, 0.0);

    quantumverse::GalacticRotationCurveScanner scanner;
    auto findings = scanner.analyze(metric, location, trajectory);

    assert(!findings.empty() && "No dark matter signal detected");

    const auto& f = findings.front();
    bool mentionsDM = f.description.find("dark matter") != std::string::npos;
    assert(mentionsDM && "Finding does not mention dark matter");
    (void)mentionsDM;

    auto rit = f.parameters.find("flatten_radius");
    assert(rit != f.parameters.end() && "flatten_radius parameter missing");
    assert(std::abs(rit->second - rFlat) < 2.0 && "Flattening radius mismatch");

    std::cout << "Detected dark matter halo: flatten_radius=" << rit->second
              << " (expected ~" << rFlat << ")" << std::endl;

    // --- Negative case: pure Keplerian curve (no flattening) ---------------
    std::vector<quantumverse::Event4D> kepler;
    for (double r = 1.0; r <= 100.0; r += 1.0) {
        double v = aKepler / std::sqrt(r);
        kepler.emplace_back(r, r, 0.0, v);
    }
    auto none = scanner.analyze(metric, location, kepler);
    assert(none.empty() && "Pure Keplerian curve should not trigger a finding");

    // --- Edge case: too few points -----------------------------------------
    std::vector<quantumverse::Event4D> tiny = {
        quantumverse::Event4D(1.0, 1.0, 0.0, 5.0),
        quantumverse::Event4D(5.0, 5.0, 0.0, 2.0),
    };
    auto few = scanner.analyze(metric, location, tiny);
    assert(few.empty() && "Too-few-point trajectory should not produce a finding");

    std::cout << "All GalacticRotationCurveScanner tests passed." << std::endl;
    return 0;
}
