// Phase 33: Gradient Optimizer Test
// Validates optimizeWithGradient in TheoryDiscoveryAgent.

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

#include "discovery/TheoryDiscoveryAgent.h"

using namespace quantumverse;
using namespace quantumverse::discovery;

namespace {
constexpr double kEps = 1e-9;
}

void test_gradient_optimization_reduces_chi2() {
    TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::BRANS_DICKE);

    // Start from a non-GR parameter set
    std::vector<double> params = {500.0, 0.9};
    auto init_result = agent.evaluateTheory(params);
    double init_chi2 = init_result.observational_chi2;
    assert(std::isfinite(init_chi2) && init_chi2 > 0.0);

    std::cout << "  Initial chi2 = " << init_chi2 << "\n";

    auto opt_result = agent.optimizeWithGradient(20, 0.1, 1e-6);

    assert(std::isfinite(opt_result.observational_chi2));
    assert(opt_result.observational_chi2 > 0.0);

    std::cout << "  Final chi2   = " << opt_result.observational_chi2 << "\n";

    // Chi2 should not increase after optimization
    assert(opt_result.observational_chi2 <= init_chi2 + 1e-6 &&
           "Optimization should not increase chi2");
}

void test_gradient_optimization_from_best() {
    TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::FR_GRAVITY);

    // Run a few RL steps to get a starting point
    agent.discoverBestTheory(5);
    double best_before = agent.getBestResult().total_reward;

    auto opt_result = agent.optimizeWithGradient(10, 0.01, 1e-6);

    assert(std::isfinite(opt_result.total_reward));
    assert(opt_result.total_reward >= best_before - 1e-6 &&
           "Optimization should not degrade best reward");

    std::cout << "  Reward before = " << best_before
              << ", after = " << opt_result.total_reward << "\n";
}

void test_gradient_optimization_updates_pareto() {
    TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::BRANS_DICKE);
    agent.setMultiObjectiveEnabled(true);

    std::vector<double> params = {500.0, 0.9};
    agent.evaluateTheory(params);

    size_t initial_size = agent.getParetoFront().size();

    auto opt_result = agent.optimizeWithGradient(10, 0.1, 1e-6);
    (void)opt_result;

    size_t final_size = agent.getParetoFront().size();
    assert(final_size >= initial_size && "Pareto archive should not shrink");

    std::cout << "  Pareto size: " << initial_size << " -> " << final_size << "\n";
}

void test_gradient_optimization_clamps_params() {
    TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::BRANS_DICKE);

    // Start with params at boundary
    auto params = agent.denormalizeParams({1.0, 1.0});
    auto result = agent.optimizeWithGradient(5, 10.0, 1e-6);
    (void)result;

    // Should not crash with large learning rate
    assert(std::isfinite(result.total_reward));
    std::cout << "  Boundary param optimization survived.\n";
}

int main() {
    std::cout << "=== GradientOptimizerTest ===\n";

    test_gradient_optimization_reduces_chi2();
    test_gradient_optimization_from_best();
    test_gradient_optimization_updates_pareto();
    test_gradient_optimization_clamps_params();

    std::cout << "All GradientOptimizerTest checks passed." << std::endl;
    return 0;
}
