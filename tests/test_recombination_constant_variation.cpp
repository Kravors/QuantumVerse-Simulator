// QuantumVerse Test: RecombinationConstantVariationImager
// TDD validation of fine-structure-constant variation detection at recombination.
// Generates a synthetic CMB power spectrum (ℓ vs C_l) with a known scaling
// C_l -> C_l*(1 + Δα/α) and verifies the detector recovers Δα/α.
// Convention: ℓ is carried in t, C_l is carried in x.

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "discovery/RecombinationConstantVariationImager.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

int main() {
    std::cout << "=== RecombinationConstantVariationImager Test ===" << std::endl;

    const double deltaAlpha = 1e-4;

    // --- Synthetic CMB power spectrum with a known alpha variation ----------
    std::vector<quantumverse::Event4D> trajectory;
    for (int l = 2; l <= 2000; l += 10) {
        double clStandard = 1000.0 / (l * (l + 1));
        double clModified = clStandard * (1.0 + deltaAlpha);
        trajectory.emplace_back(l, clModified, 0.0, 0.0); // t=ℓ, x=C_l
    }

    quantumverse::MetricTensor metric;
    quantumverse::Event4D location(0.0, 0.0, 0.0, 0.0);

    quantumverse::RecombinationConstantVariationImager imager;
    auto findings = imager.analyze(metric, location, trajectory);

    assert(!findings.empty() && "No variation detected");

    bool foundVariation = false;
    for (const auto& f : findings) {
        if (f.description.find("alpha") != std::string::npos) {
            foundVariation = true;
            auto it = f.parameters.find("delta_alpha_over_alpha");
            assert(it != f.parameters.end() && "delta_alpha_over_alpha parameter missing");
            assert(std::abs(it->second - deltaAlpha) < 1e-9 && "Recovered Δα/α mismatch");
            std::cout << "Recovered Δα/α = " << it->second
                      << " (injected " << deltaAlpha << ")" << std::endl;
            break;
        }
    }
    assert(foundVariation && "Finding does not mention alpha variation");

    // --- Negative case: standard (unmodified) spectrum -> no finding --------
    std::vector<quantumverse::Event4D> standard;
    for (int l = 2; l <= 2000; l += 10) {
        double cl = 1000.0 / (l * (l + 1));
        standard.emplace_back(l, cl, 0.0, 0.0);
    }
    auto stdFindings = imager.analyze(metric, location, standard);
    assert(stdFindings.empty() && "False positive on standard spectrum");
    (void)stdFindings;

    // --- Edge case: too few points -> no finding ----------------------------
    std::vector<quantumverse::Event4D> shortTraj(3);
    auto shortFindings = imager.analyze(metric, location, shortTraj);
    assert(shortFindings.empty() && "Short trajectory should yield no findings");
    (void)shortFindings;

    std::cout << "All RecombinationConstantVariationImager tests passed." << std::endl;
    return 0;
}
