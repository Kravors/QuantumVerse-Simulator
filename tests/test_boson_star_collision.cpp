// QuantumVerse Test: BosonStarCollisionPredictor
// TDD validation of boson-star merger detection. Generates a synthetic GW
// strain: a pre-merger chirp (growing frequency/amplitude) followed by a
// damped ringdown. Convention: t = time, z = GW strain.

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "discovery/BosonStarCollisionPredictor.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

int main() {
    std::cout << "=== BosonStarCollisionPredictor Test ===" << std::endl;

    const double twoPi = 2.0 * 3.14159265358979323846;
    const double tMerger = 50.0;

    // --- Synthetic boson-star merger signal --------------------------------
    std::vector<quantumverse::Event4D> trajectory;
    for (double t = 0.0; t < 100.0; t += 0.1) {
        double strain = 0.0;
        if (t < tMerger) {
            double f = 0.1 + (0.5 - 0.1) * (t / tMerger);
            double A = 1e-6 * (t / tMerger);
            strain = A * std::sin(twoPi * f * t);
        } else {
            double tau = 10.0;
            double A = 1e-6 * std::exp(-(t - tMerger) / tau);
            strain = A * std::sin(twoPi * 0.5 * (t - tMerger));
        }
        trajectory.emplace_back(t, 0.0, 0.0, strain); // t=time, z=strain
    }

    quantumverse::MetricTensor metric;
    quantumverse::Event4D location(0.0, 0.0, 0.0, 0.0);

    quantumverse::BosonStarCollisionPredictor predictor;
    auto findings = predictor.analyze(metric, location, trajectory);

    assert(!findings.empty() && "No merger detected");

    const auto& f = findings.front();
    bool mentions = f.description.find("boson star") != std::string::npos ||
                    f.description.find("merger") != std::string::npos;
    assert(mentions && "Finding does not mention boson star merger");

    auto mit = f.parameters.find("merger_time");
    assert(mit != f.parameters.end() && "merger_time parameter missing");
    assert(std::abs(mit->second - tMerger) < 1.0 && "Merger time mismatch");

    std::cout << "Detected boson star merger at t=" << mit->second << std::endl;

    // --- Negative case: zero signal -> no finding --------------------------
    std::vector<quantumverse::Event4D> zeroTraj;
    for (double t = 0.0; t < 100.0; t += 0.1) {
        zeroTraj.emplace_back(t, 0.0, 0.0, 0.0);
    }
    auto zero = predictor.analyze(metric, location, zeroTraj);
    assert(zero.empty() && "False positive on zero signal");

    // --- Edge case: too few points -----------------------------------------
    std::vector<quantumverse::Event4D> tiny(5);
    auto few = predictor.analyze(metric, location, tiny);
    assert(few.empty() && "Short trajectory should yield no findings");

    std::cout << "All BosonStarCollisionPredictor tests passed." << std::endl;
    return 0;
}
