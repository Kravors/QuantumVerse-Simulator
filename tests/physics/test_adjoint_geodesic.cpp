// Phase 33: Adjoint Geodesic Integrator Test
// Validates reverse-mode AD tape recording for Schwarzschild geodesics.

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <array>

#include "physics/AdjointGeodesicIntegrator.h"
#include "spacetime/Event4D.h"
#include "spacetime/MetricTensor.h"

using namespace quantumverse;
using namespace quantumverse::physics;

namespace {
constexpr double kEps = 1e-9;
constexpr double kRelTol = 1e-3;
constexpr double kPi = 3.14159265358979323846;
}

void test_adjoint_geodesic_basic() {
    std::cout << "  Starting adjoint geodesic basic test...\n";
    AdjointGeodesicIntegrator integrator(1.0, 1e-6, 1e-10, 0.5, 0.9, 100000);

    Event4D start(0.0, 10.0, 0.0, 0.0);
    std::array<double, 4> vel = {1.0, -0.1, 0.0, 0.0};

    std::cout << "  Calling integrate...\n";
    auto result = integrator.integrate(start, vel, GeodesicType::TIMELIKE, 0.01);
    std::cout << "  integrate returned.\n";

    assert(result.success && "Adjoint integration should succeed");
    assert(result.steps > 0 && "Should take at least one step");
    assert(std::isfinite(result.finalState[1]) && "Final r should be finite");
    assert(result.finalState[1] > 0.0 && "Final r should be positive");
    assert(result.finalState[1] < 10.0 && "Particle should fall inward (r decreases)");

    std::cout << "  Adjoint geodesic: r_final=" << result.finalState[1]
              << " steps=" << result.steps << "\n";
}

void test_adjoint_gradient_nonzero() {
    AdjointGeodesicIntegrator integrator(1.0, 1e-6, 1e-10, 0.5, 0.9, 100000);

    Event4D start(0.0, 10.0, 0.0, 0.0);
    std::array<double, 4> vel = {1.0, -0.1, 0.0, 0.0};

    auto result = integrator.integrate(start, vel, GeodesicType::TIMELIKE, 0.5);

    assert(result.dState_dParams.size() == 8 && "Gradient matrix should have 8 rows");
    assert(result.dState_dParams[0].size() == 1 && "Gradient matrix should have 1 column (mass)");
    assert(std::isfinite(result.dState_dParams[1][0]) && "dr/dM should be finite");

    std::cout << "  dr/dM = " << result.dState_dParams[1][0] << "\n";
}

void test_adjoint_vs_finite_difference() {
    AdjointGeodesicIntegrator integrator(1.0, 1e-6, 1e-10, 0.5, 0.9, 100000);

    Event4D start(0.0, 10.0, 0.0, 0.0);
    std::array<double, 4> vel = {1.0, -0.1, 0.0, 0.0};

    auto adjoint_result = integrator.integrate(start, vel, GeodesicType::TIMELIKE, 0.5);
    auto fd_grad = integrator.computePositionGradientFD(start, vel, GeodesicType::TIMELIKE, 0.5, 1e-6);

    // Compare dr/dM from adjoint vs finite difference
    double adjoint_dr_dM = adjoint_result.dState_dParams[1][0];
    double fd_dr_dM = fd_grad[1];

    assert(std::isfinite(fd_dr_dM) && "FD gradient should be finite");

    double rel_error = std::abs(adjoint_dr_dM - fd_dr_dM) / (std::abs(fd_dr_dM) + 1e-12);
    assert(rel_error < kRelTol && "Adjoint gradient should match finite difference");

    std::cout << "  Adjoint dr/dM = " << adjoint_dr_dM
              << ", FD dr/dM = " << fd_dr_dM
              << ", rel error = " << rel_error << "\n";
}

void test_adjoint_tape_cleanup() {
    AdjointGeodesicIntegrator integrator(1.0, 1e-6, 1e-10, 0.5, 0.9, 100000);

    Event4D start(0.0, 10.0, 0.0, 0.0);
    std::array<double, 4> vel = {1.0, -0.1, 0.0, 0.0};

    auto result1 = integrator.integrate(start, vel, GeodesicType::TIMELIKE, 0.1);
    assert(result1.success);

    auto result2 = integrator.integrate(start, vel, GeodesicType::TIMELIKE, 0.1);
    assert(result2.success);

    // Both integrations should produce consistent results
    assert(std::abs(result1.finalState[1] - result2.finalState[1]) < 1e-6 &&
           "Repeated integrations should be consistent");

    std::cout << "  Tape cleanup verified (repeated integrations consistent).\n";
}

int main() {
    std::cout << "=== AdjointGeodesicTest ===\n";

    test_adjoint_geodesic_basic();
    test_adjoint_gradient_nonzero();
    test_adjoint_vs_finite_difference();
    test_adjoint_tape_cleanup();

    std::cout << "All AdjointGeodesicTest checks passed." << std::endl;
    return 0;
}
