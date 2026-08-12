#include "spacetime/Event4D.h"
#include "spacetime/MetricTensor.h"
#include "physics/GeodesicIntegrator.h"

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <array>
#include <vector>
#include <memory>
#include <limits>

using namespace quantumverse;

extern "C" {
    int LLVMFuzzerInitialize(int* argc, char*** argv) {
        (void)argc;
        (void)argv;
        return 0;
    }

    int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);
}

static bool hasBad(const Event4D& e) {
    return !std::isfinite(e.t) || !std::isfinite(e.x) ||
           !std::isfinite(e.y) || !std::isfinite(e.z);
}

static double clamp(double v, double lo, double hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/**
 * @brief Phase 21.x: Differential fuzz test for geodesic integration.
 *
 * Drives the adaptive Runge-Kutta geodesic integrator (GeodesicIntegrator::
 * integrate) and the fixed-step integrator (GeodesicIntegrator::integrateSimple)
 * over a fuzzed Schwarzschild geometry and initial conditions, then verifies:
 *   - No NaN/Inf appears in any integrated event.
 *   - The two integrators agree to within a gross-divergence bound, which would
 *     indicate a real integration or metric-evaluation bug.
 *
 * Build:
 *   cmake -DQUANTUMVERSE_USE_FUZZER=ON -DCMAKE_CXX_COMPILER=clang++ ..
 * Run:
 *   ./fuzz_geodesic_differential -max_total_time=300
 */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < sizeof(double) * 10) return 0;

    const double* vals = reinterpret_cast<const double*>(data);
    size_t numVals = size / sizeof(double);

    // Reject non-finite inputs at the source.
    for (size_t i = 0; i < numVals; ++i) {
        if (!std::isfinite(vals[i])) return 0;
    }

    // Black-hole mass: clamped so the horizon stays well inside the clamped
    // start radius (avoids degenerate metric blow-up near the singularity).
    double M = std::fabs(vals[0]) * 1e33 + 1e20;
    if (M > 1e33) M = 1e33;

    // Start event: clamp coordinates to a sane neighborhood (meters).
    Event4D start(clamp(vals[1], -1e7, 1e7),
                  clamp(vals[2], -1e7, 1e7),
                  clamp(vals[3], -1e7, 1e7),
                  clamp(vals[4], -1e7, 1e7));

    // Initial four-velocity: clamped to avoid extreme scales.
    std::array<double, 4> vel = {
        clamp(vals[5], -1e8, 1e8),
        clamp(vals[6], -1e8, 1e8),
        clamp(vals[7], -1e8, 1e8),
        clamp(vals[8], -1e8, 1e8)
    };

    // Integration span (proper time, seconds).
    double properTime = clamp(std::fabs(vals[9]) * 100.0 + 1.0, 0.1, 1000.0);

    auto metric = std::make_shared<SchwarzschildMetric>(M);

    GeodesicIntegrator integrator;
    integrator.setMetric(metric);

    // Differential: adaptive RK vs fixed-step RK over the same initial data.
    auto adaptive = integrator.integrate(start, vel, GeodesicType::TIMELIKE, properTime, true);
    const int steps = 200;
    double deltaTau = properTime / steps;
    auto simple = integrator.integrateSimple(start, vel, steps, deltaTau);

    // 1) No NaN/Inf anywhere in either trajectory.
    for (const auto& s : adaptive) {
        if (!s.valid || hasBad(s.event)) {
            fprintf(stderr, "GEODESIC FAILURE: bad adaptive step\n");
            __builtin_trap();
            return 1;
        }
    }
    for (const auto& e : simple) {
        if (hasBad(e)) {
            fprintf(stderr, "GEODESIC FAILURE: bad simple step\n");
            __builtin_trap();
            return 1;
        }
    }

    // 2) The two integrators must agree to within a gross-divergence bound.
    if (adaptive.size() >= 2 && simple.size() >= 2) {
        const Event4D& a = adaptive.back().event;
        const Event4D& b = simple.back();
        double dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z, dt = a.t - b.t;
        double diff = std::sqrt(dx * dx + dy * dy + dz * dz + dt * dt);

        // Expected displacement scale is ~ c * properTime.
        double expected = Event4D::C * properTime;
        if (expected < 1.0) expected = 1.0;

        if (diff > 100.0 * expected) {
            fprintf(stderr, "GEODESIC DIFFERENTIAL FAILURE: diff=%e expected=%e\n",
                    diff, expected);
            fprintf(stderr, "  start=(%f,%f,%f,%f) M=%e properTime=%f\n",
                    start.t, start.x, start.y, start.z, M, properTime);
            fprintf(stderr, "  adaptive=(%f,%f,%f,%f)\n", a.t, a.x, a.y, a.z);
            fprintf(stderr, "  simple  =(%f,%f,%f,%f)\n", b.t, b.x, b.y, b.z);
            __builtin_trap();
            return 1;
        }
    }

    return 0;
}
