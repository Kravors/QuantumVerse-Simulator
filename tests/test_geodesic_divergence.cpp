// QuantumVerse Sandbox: Geodesic Divergence Benchmark
// Isolates GeodesicIntegrator to measure NaN/Inf divergence at high Kerr spins
// without UI or ONNX overhead. Directly addresses the root cause of ML panic
// scores (5.76e14) from diverged numerical integration.

#include <iostream>
#include <cmath>
#include <vector>
#include <array>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "physics/GeodesicIntegrator.h"
#include "spacetime/KerrMetric.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "test_assert.h"

using namespace quantumverse;

struct DivergenceResult {
    double spin;
    double probeR;
    int totalSteps;
    int nanSteps;
    int infSteps;
    double maxVelocity;
    double maxKretschmann;
    bool diverged;
};

// Count NaN/Inf in an array of 4 doubles
static int countNanInf(const std::array<double, 4>& arr, int& nanCount, int& infCount) {
    nanCount = 0;
    infCount = 0;
    for (double v : arr) {
        if (std::isnan(v)) nanCount++;
        if (std::isinf(v)) infCount++;
    }
    return nanCount + infCount;
}

static DivergenceResult runDivergenceTest(
    GeodesicIntegrator& integrator,
    double spin,
    double probeR
) {
    DivergenceResult result{spin, probeR, 0, 0, 0, 0.0, 0.0, false};

    // Mass = 10 M☉
    const double massKg = 10.0 * 1.989e30;
    KerrMetric kerr(massKg, spin);
    integrator.setMetric(std::make_shared<KerrMetric>(kerr));

    // Start just outside the horizon at the probe radius
    Event4D start(0.0, probeR, 0.0, 0.0);

    // Timelike circular-ish initial velocity (outward radial component small)
    std::array<double, 4> velocity = {1.0, 0.001, 0.0, 0.0};

    auto trajectory = integrator.integrate(start, velocity, GeodesicType::TIMELIKE, 1.0);

    for (const auto& step : trajectory) {
        result.totalSteps++;
        int nanCount, infCount;
        countNanInf(step.velocity, nanCount, infCount);
        result.nanSteps += nanCount;
        result.infSteps += infCount;

        double vNorm = 0.0;
        for (double v : step.velocity) vNorm += v * v;
        vNorm = std::sqrt(vNorm);
        if (vNorm > result.maxVelocity) result.maxVelocity = vNorm;

        auto cs = kerr.curvatureScalars(step.event);
        if (cs.valid && std::isfinite(cs.kretschmann)) {
            if (cs.kretschmann > result.maxKretschmann) result.maxKretschmann = cs.kretschmann;
        }
    }

    result.diverged = (result.nanSteps > 0 || result.infSteps > 0 || result.maxVelocity > 1e6);
    return result;
}

int main() {
    std::cout << "=== Geodesic Divergence Benchmark (Sandbox Session B) ===" << std::endl;
    std::cout << "Isolating GeodesicIntegrator NaN/Inf divergence at high Kerr spins" << std::endl;
    std::cout << "Mass = 10 M☉, target proper time = 0.1 s" << std::endl;
    std::cout << std::endl;

    const std::vector<double> spins = {0.0, 0.5, 0.9, 0.99};
    const std::vector<double> probeRs = {
        0.0,          // origin (clamped to 1.5·rs)
        2.0e4,        // 0.68·rs (inside clamp → clamped)
        4.4e4,        // 1.49·rs (at clamp boundary → clamped)
        6.0e4,        // 2.0·rs (just outside clamp zone)
        1.0e5,        // 3.38·rs (production probe)
        5.0e5,        // far field
        1e10,         // very far field
    };

    std::vector<DivergenceResult> results;

    for (double spin : spins) {
        GeodesicIntegrator integrator(1e-6);
        for (double probeR : probeRs) {
            auto r = runDivergenceTest(integrator, spin, probeR);
            results.push_back(r);

            const char* status = r.diverged ? "DIVERGED" : "stable";
            std::printf("spin=%.2f r=%.2e steps=%d nan=%d inf=%d maxV=%.3e maxK=%.3e [%s]\n",
                spin, probeR, r.totalSteps, r.nanSteps, r.infSteps,
                r.maxVelocity, r.maxKretschmann, status);
        }
        std::cout << std::endl;
    }

    // Regression: spin 0.0 (Schwarzschild) must never diverge
    bool allStable = true;
    for (const auto& r : results) {
        if (r.spin == 0.0 && r.diverged) {
            allStable = false;
            std::cerr << "[FAIL] Schwarzschild (spin=0.0) diverged at r=" << r.probeR << std::endl;
        }
    }

    // Summary: which spin levels show divergence
    std::cout << "=== Summary ===" << std::endl;
    for (double spin : spins) {
        bool anyDiverged = false;
        for (const auto& r : results) {
            if (r.spin == spin && r.diverged) anyDiverged = true;
        }
        std::cout << "spin=" << spin << ": " << (anyDiverged ? "DIVERGENCE DETECTED" : "all stable") << std::endl;
    }

    QV_CHECK(allStable);
    std::cout << "=== Benchmark complete ===" << std::endl;
    return 0;
}
