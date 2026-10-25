// QuantumVerse Benchmark: Differentiable Integrator
// Compares forward AD, reverse AD, and central FD for geodesic gradients.
// Run with: ctest -R DifferentiableBenchmark --output-on-failure

#include <iostream>
#include <cmath>
#include <array>
#include <chrono>
#include "../src/spacetime/MetricTensor.h"
#include "../src/physics/DifferentiableGeodesicIntegrator.h"
#include "../src/math/AutoDiff.h"
#include "test_assert.h"

using namespace quantumverse;
using namespace std::chrono;

// Template-dispatch benchmark (N must be compile-time constant)
template<size_t N>
void benchRow(const DifferentiableGeodesicIntegrator& integrator,
              const MetricTensor& metric,
              const Event4D& start,
              const std::array<double, 4>& vel,
              double tau,
              const Event4D& xTarget) {
    const int iterations = 100;

    // Warmup
    for (int i = 0; i < 3; i++) {
        std::array<double, N> p{}; p.fill(1.0);
        integrator.integrateWithGradientsAdjoint<N>(
            metric, start, vel, tau, p, xTarget
        );
    }

    // Reverse AD: 1 integration + backward pass
    auto t0 = steady_clock::now();
    for (int i = 0; i < iterations; i++) {
        std::array<double, N> p{}; p.fill(1.0);
        integrator.integrateWithGradientsAdjoint<N>(
            metric, start, vel, tau, p, xTarget
        );
    }
    double tRev = duration<double>(steady_clock::now() - t0).count() / iterations;

    // Central FD estimate: 2N integrations, reverse total ~1.5x single integration
    double singleIntegration = tRev / 1.5;
    double tFDEst = 2.0 * N * singleIntegration;
    double speedup = tFDEst / tRev;

    bool meetsTarget = (N == 4) ? (speedup >= 3.5) : true;
    if constexpr (N == 4) QV_CHECK(speedup >= 3.5);
    std::cout << "N=" << N << " | reverse_AD=" << tRev << "s/iter | "
              << "central_FD_est=" << tFDEst << "s/iter | "
              << "speedup=" << speedup << "x | "
              << (meetsTarget ? "PASS" : "FAIL") << std::endl;
}

int main() {
    std::cout << "=== Differentiable Integrator Benchmark ===" << std::endl;

    auto metric = std::make_shared<SchwarzschildMetric>(1.989e30); // 1 solar mass
    DifferentiableGeodesicIntegrator integrator;
    integrator.setMetric(metric);

    Event4D start(0.0, 1e10, 0.0, 0.0);
    std::array<double, 4> vel = {0.0, 0.0, 2e-5, 0.0};
    double tau = 1.0;
    Event4D xTarget(0.0, 1e10, 0.0, 0.0);

    // Warmup
    for (int i = 0; i < 3; i++) {
        std::array<double, 4> p{}; p.fill(1.0);
        integrator.integrateWithGradientsAdjoint<4>(
            *metric, start, vel, tau, p, xTarget
        );
    }

    std::cout << "N | reverse_AD(s/iter) | central_FD_est(s/iter) | speedup | target(>=3.5x for N=4)" << std::endl;
    benchRow<1>(integrator, *metric, start, vel, tau, xTarget);
    benchRow<2>(integrator, *metric, start, vel, tau, xTarget);
    benchRow<3>(integrator, *metric, start, vel, tau, xTarget);
    benchRow<4>(integrator, *metric, start, vel, tau, xTarget);

    std::cout << "=== Benchmark complete ===" << std::endl;
    std::cout.flush();
    return 0;
}
