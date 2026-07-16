#include "physics/GeodesicIntegrator.h"
#include "quantumgravity/CDTEngine.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include <cmath>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace quantumverse;

static double baselineGeodesicMs = 5000.0;
static double baselineCDTMs = 3000.0;

int main() {
    int failures = 0;

    // Geodesic throughput benchmark: integrate 1000 lightlike geodesics
    // in Schwarzschild, measure total time.
    {
        std::printf("Benchmark: geodesic throughput (1000 lightlike geodesics)...\n");
        auto metric = std::make_shared<SchwarzschildMetric>(1.989e30);
        GeodesicIntegrator integrator;
        integrator.setMetric(metric);

        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < 1000; ++i) {
            Event4D startEv(0.0, 1e10 + i * 1e3, 0.0, 0.0);
            std::array<double, 4> vel = {0.0, 0.0, 0.0, 0.1};
            auto traj = integrator.integrate(startEv, vel, GeodesicType::LIGHTLIKE, 10.0, true);
            (void)traj;
        }
        auto elapsed = std::chrono::steady_clock::now() - start;
        double ms = std::chrono::duration<double, std::milli>(elapsed).count();
        std::printf("  Measured: %.2f ms (threshold: %.2f ms)\n", ms, baselineGeodesicMs * 1.3);

        if (ms > baselineGeodesicMs * 1.3) {
            std::fprintf(stderr,
                "[FAIL] Geodesic throughput exceeded threshold: %.2f ms > %.2f ms\n",
                ms, baselineGeodesicMs * 1.3);
            ++failures;
        } else {
            std::printf("[PASS] Geodesic throughput within threshold\n");
        }
    }

    // CDT Monte Carlo sweep benchmark: execute 100 MC steps, measure time.
    {
        std::printf("Benchmark: CDT Monte Carlo sweep (100 steps)...\n");
        quantumverse::quantumgravity::CDTEngine engine(200, 50.0, 50.0);
        engine.thermalize(50);

        auto start = std::chrono::steady_clock::now();
        engine.runMonteCarlo(100);
        auto elapsed = std::chrono::steady_clock::now() - start;
        double ms = std::chrono::duration<double, std::milli>(elapsed).count();
        std::printf("  Measured: %.2f ms (threshold: %.2f ms)\n", ms, baselineCDTMs * 1.3);

        if (ms > baselineCDTMs * 1.3) {
            std::fprintf(stderr,
                "[FAIL] CDT MC sweep exceeded threshold: %.2f ms > %.2f ms\n",
                ms, baselineCDTMs * 1.3);
            ++failures;
        } else {
            std::printf("[PASS] CDT MC sweep within threshold\n");
        }
    }

    if (failures == 0) {
        std::printf("=== PERFORMANCE REGRESSION GATE PASSED ===\n");
        return 0;
    }
    std::fprintf(stderr, "%d performance gate check(s) failed.\n", failures);
    return 1;
}
