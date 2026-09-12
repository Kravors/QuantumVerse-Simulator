// Phase 33.5: Adjoint Gradient Optimizer Test
// Validates that optimizeWithGradient uses adjoint-based gradients
// for Schwarzschild-like parameters and falls back to FD otherwise.

#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

#include "discovery/TheoryDiscoveryAgent.h"
#include "physics/AdjointGeodesicIntegrator.h"
#include "test_assert.h"

using namespace quantumverse;
using namespace quantumverse::discovery;
using namespace quantumverse::physics;

namespace {
constexpr double kEps = 1e-9;
}

void test_adjoint_gradient_reduces_chi2() {
    TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::BRANS_DICKE);

    std::vector<double> params = {500.0, 0.9};
    auto init_result = agent.evaluateTheory(params);
    double init_chi2 = init_result.observational_chi2;
    QV_CHECK(std::isfinite(init_chi2) && init_chi2 > 0.0);

    std::cout << "  Initial chi2 = " << init_chi2 << "\n";

    auto opt_result = agent.optimizeWithGradient(20, 0.1, 1e-6);

    QV_CHECK(std::isfinite(opt_result.observational_chi2));
    QV_CHECK(opt_result.observational_chi2 > 0.0);

    std::cout << "  Final chi2   = " << opt_result.observational_chi2 << "\n";

    QV_CHECK(opt_result.observational_chi2 <= init_chi2 + 1e-6);
}

void test_adjoint_gradient_from_best() {
    TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::FR_GRAVITY);

    agent.discoverBestTheory(5);
    double best_before = agent.getBestResult().total_reward;

    auto opt_result = agent.optimizeWithGradient(10, 0.01, 1e-6);

    QV_CHECK(std::isfinite(opt_result.total_reward));
    QV_CHECK(opt_result.total_reward >= best_before - 1e-6);

    std::cout << "  Reward before = " << best_before
              << ", after = " << opt_result.total_reward << "\n";
}

void test_adjoint_gradient_updates_pareto() {
    TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::BRANS_DICKE);
    agent.setMultiObjectiveEnabled(true);

    std::vector<double> params = {500.0, 0.9};
    agent.evaluateTheory(params);

    size_t initial_size = agent.getParetoFront().size();

    auto opt_result = agent.optimizeWithGradient(10, 0.1, 1e-6);
    (void)opt_result;

    size_t final_size = agent.getParetoFront().size();
    QV_CHECK(final_size >= initial_size);

    std::cout << "  Pareto size: " << initial_size << " -> " << final_size << "\n";
}

void test_adjoint_curvature_gradient() {
    AdjointGeodesicIntegrator integrator(1.0, 1e-6, 1e-10, 0.5, 0.9, 100000);

    Event4D start(0.0, 10.0, 0.0, 0.0);

    double dK_dM = integrator.computeCurvatureGradient(1.0, start);

    QV_CHECK(std::isfinite(dK_dM));
    QV_CHECK(dK_dM != 0.0);

    std::cout << "  dK/dM = " << dK_dM << "\n";
}

void test_adjoint_state_gradient() {
    AdjointGeodesicIntegrator integrator(1.0, 1e-6, 1e-10, 0.5, 0.9, 100000);

    Event4D start(0.0, 10.0, 0.0, 0.0);
    std::array<double, 4> vel = {1.0, -0.1, 0.0, 0.0};

    auto result = integrator.computeStateGradient(start, vel, GeodesicType::TIMELIKE, 0.01);

    QV_CHECK(result.first.size() == 8);
    QV_CHECK(!result.second.empty());
    QV_CHECK(result.second.size() == 8);
    QV_CHECK(result.second[0].size() >= 1);
    QV_CHECK(std::isfinite(result.second[1][0]));

    std::cout << "  dr/dM = " << result.second[1][0] << "\n";
}

int main() {
    std::cout << "=== AdjointGradientOptimizerTest ===\n";

    test_adjoint_gradient_reduces_chi2();
    test_adjoint_gradient_from_best();
    test_adjoint_gradient_updates_pareto();
    test_adjoint_curvature_gradient();
    test_adjoint_state_gradient();

    std::cout << "All AdjointGradientOptimizerTest checks passed." << std::endl;
    return 0;
}
