#include <benchmark/benchmark.h>
#include "math/Vector4D.h"
#include "math/Matrix4x4.h"
#include "physics/GeodesicIntegrator.h"
#include "spacetime/MetricTensor.h"
#include "quantumgravity/CDTEngine.h"
#include <array>

using namespace quantumverse;

static void BM_SchwarzschildMetric(benchmark::State& state) {
    for (auto _ : state) {
        auto m = MetricTensor::schwarzschild(1.0, 10.0, M_PI/2, 0.0);
        benchmark::DoNotOptimize(m);
    }
}
BENCHMARK(BM_SchwarzschildMetric);

static void BM_GeorodesicIntegration(benchmark::State& state) {
    Event4D start(0, 10, 0, 0);
    std::array<double,4> vel = {0, 0, 0, 0.1};
    auto metric = MetricTensor::schwarzschild(1.0, 10.0, M_PI/2, 0.0);
    GeodesicIntegrator integrator;
    integrator.setMetric(metric);
    for (auto _ : state) {
        auto steps = integrator.integrate(start, vel, GeodesicType::TIMELIKE, 1.0, true);
        benchmark::DoNotOptimize(steps);
    }
}
BENCHMARK(BM_GeorodesicIntegration);

static void BM_Vector4DLorentzianDot(benchmark::State& state) {
    Vector4d a(1.0, 2.0, 3.0, 4.0);
    Vector4d b(2.0, 3.0, 4.0, 5.0);
    for (auto _ : state) {
        double result = a.lorentzianDot(b);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Vector4DLorentzianDot);

static void BM_Matrix4x4Determinant(benchmark::State& state) {
    Matrix4x4<double> m;
    m(0, 0) = 1; m(0, 1) = 0.1; m(0, 2) = 0.2; m(0, 3) = 0.3;
    m(1, 0) = 0.1; m(1, 1) = 1; m(1, 2) = 0.2; m(1, 3) = 0.3;
    m(2, 0) = 0.2; m(2, 1) = 0.1; m(2, 2) = 1; m(2, 3) = 0.3;
    m(3, 0) = 0.3; m(3, 1) = 0.2; m(3, 2) = 0.1; m(3, 3) = 1;
    for (auto _ : state) {
        double det = m.determinant();
        benchmark::DoNotOptimize(det);
    }
}
BENCHMARK(BM_Matrix4x4Determinant);

static void BM_GeodesicThroughput(benchmark::State& state) {
    Event4D start(0, 10, 0, 0);
    std::array<double,4> vel = {0, 0, 0, 0.1};
    auto metric = MetricTensor::schwarzschild(1.0, 10.0, M_PI/2, 0.0);
    GeodesicIntegrator integrator;
    integrator.setMetric(metric);
    int count = 0;
    for (auto _ : state) {
        auto steps = integrator.integrate(start, vel, GeodesicType::TIMELIKE, 1.0, true);
        benchmark::DoNotOptimize(steps);
        ++count;
    }
    state.SetItemsProcessed(count);
}
BENCHMARK(BM_GeodesicThroughput);

static void BM_CDTSweepTime(benchmark::State& state) {
    quantumgravity::CDTEngine engine(1000, 100.0, 100.0);
    engine.thermalize(500);
    for (auto _ : state) {
        engine.runMonteCarlo(100);
        benchmark::DoNotOptimize(engine.getSpectralDimension());
    }
}
BENCHMARK(BM_CDTSweepTime);

BENCHMARK_MAIN();