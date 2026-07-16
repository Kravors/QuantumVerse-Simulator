#include "discovery/DiscoveryInstrument.h"
#include "discovery/ExoplanetaryTTVFifthForceHunter.h"
#include "discovery/GalacticRotationCurveScanner.h"
#include "discovery/FineStructureConstantDriftObservatory.h"
#include "discovery/BosonStarCollisionPredictor.h"
#include "discovery/NeutronStarGlitchPhaseDetector.h"
#include "discovery/UltralightDMWaveInterferometer.h"
#include "discovery/BlackHoleJetAnomalyRecogniser.h"
#include "discovery/PrimordialLithiumCrisisSolver.h"
#include "discovery/GalacticTidalStreamCartographer.h"
#include "discovery/RecombinationConstantVariationImager.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include <cmath>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>

using namespace quantumverse;

static bool hasNaNOrInf(const std::vector<InstrumentFinding>& findings) {
    for (const auto& f : findings) {
        if (std::isnan(f.confidence) || std::isinf(f.confidence)) return true;
        if (std::isnan(f.timestamp) || std::isinf(f.timestamp)) return true;
        for (const auto& kv : f.parameters) {
            if (std::isnan(kv.second) || std::isinf(kv.second)) return true;
        }
    }
    return false;
}

static bool metricHasNaNOrInf(const std::array<std::array<double, 4>, 4>& g) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            double v = g[i][j];
            if (std::isnan(v) || std::isinf(v)) return true;
        }
    }
    return false;
}

static double randDouble(double lo, double hi) {
    return lo + (hi - lo) * (std::rand() / (RAND_MAX + 1.0));
}

static Event4D randomEvent() {
    double t = randDouble(-1e6, 1e6);
    double x = randDouble(1e3, 1e15);
    double y = randDouble(-1e14, 1e14);
    double z = randDouble(-1e14, 1e14);
    return Event4D(t, x, y, z);
}

static std::vector<Event4D> randomTrajectory(int n) {
    std::vector<Event4D> traj;
    traj.reserve(n);
    for (int i = 0; i < n; ++i) {
        traj.push_back(Event4D(
            randDouble(-1e6, 1e6),
            randDouble(1e3, 1e15),
            randDouble(-1e14, 1e14),
            randDouble(-1e14, 1e14)
        ));
    }
    return traj;
}

int main() {
    std::srand(789u);
    const int kCyclesPerInstrument = 100;
    const auto kTimeout = std::chrono::seconds(1);
    int failures = 0;

    std::vector<std::unique_ptr<DiscoveryInstrument>> instruments;
    instruments.push_back(std::make_unique<ExoplanetaryTTVFifthForceHunter>());
    instruments.push_back(std::make_unique<GalacticRotationCurveScanner>());
    instruments.push_back(std::make_unique<FineStructureConstantDriftObservatory>());
    instruments.push_back(std::make_unique<BosonStarCollisionPredictor>());
    instruments.push_back(std::make_unique<NeutronStarGlitchPhaseDetector>());
    instruments.push_back(std::make_unique<UltralightDMWaveInterferometer>());
    instruments.push_back(std::make_unique<BlackHoleJetAnomalyRecogniser>());
    instruments.push_back(std::make_unique<PrimordialLithiumCrisisSolver>());
    instruments.push_back(std::make_unique<GalacticTidalStreamCartographer>());
    instruments.push_back(std::make_unique<RecombinationConstantVariationImager>());

    std::printf("=== Fuzz Instruments: %zu instruments, %d cycles each ===\n",
                instruments.size(), kCyclesPerInstrument);

    for (size_t idx = 0; idx < instruments.size(); ++idx) {
        const char* name = instruments[idx]->getName().c_str();
        int instrumentFailures = 0;

        for (int cycle = 0; cycle < kCyclesPerInstrument; ++cycle) {
            auto metric = std::make_shared<SchwarzschildMetric>(randDouble(1e30, 1e35));
            Event4D location = randomEvent();
            std::vector<Event4D> traj = randomTrajectory(20);

            auto start = std::chrono::steady_clock::now();
            std::vector<InstrumentFinding> findings;
            bool timedOut = false;

            try {
                findings = instruments[idx]->analyze(*metric, location, traj);
            } catch (const std::exception& e) {
                std::fprintf(stderr, "[FAIL] %s cycle %d threw: %s\n", name, cycle, e.what());
                ++instrumentFailures;
                continue;
            } catch (...) {
                std::fprintf(stderr, "[FAIL] %s cycle %d threw unknown exception\n", name, cycle);
                ++instrumentFailures;
                continue;
            }

            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed > kTimeout) {
                std::fprintf(stderr, "[FAIL] %s cycle %d timed out\n", name, cycle);
                ++instrumentFailures;
                timedOut = true;
            }

            if (!timedOut && hasNaNOrInf(findings)) {
                std::fprintf(stderr, "[FAIL] %s cycle %d produced NaN/Inf finding\n", name, cycle);
                ++instrumentFailures;
            }
        }

        if (instrumentFailures == 0) {
            std::printf("[PASS] %s\n", name);
        } else {
            std::printf("[FAIL] %s: %d/%d failures\n", name, instrumentFailures, kCyclesPerInstrument);
            ++failures;
        }
    }

    // Fuzz MetricTensor::evaluate() with NaN/Inf coordinates
    std::printf("=== Fuzz MetricTensor::evaluate() with bad coordinates ===\n");
    SchwarzschildMetric sch(1.989e30);
    double badCoords[][4] = {
        {NAN, 0.0, 0.0, 0.0},
        {0.0, NAN, 0.0, 0.0},
        {0.0, 0.0, INFINITY, 0.0},
        {0.0, 0.0, 0.0, -INFINITY},
        {NAN, NAN, NAN, NAN},
        {INFINITY, INFINITY, INFINITY, INFINITY}
    };
    int metricFailures = 0;
    for (auto& bc : badCoords) {
        Event4D ev(bc[0], bc[1], bc[2], bc[3]);
        auto result = sch.evaluate(ev);
        if (metricHasNaNOrInf(result)) {
            std::fprintf(stderr,
                "[FAIL] MetricTensor::evaluate() returned NaN/Inf for input (%f,%f,%f,%f)\n",
                bc[0], bc[1], bc[2], bc[3]);
            ++metricFailures;
        }
    }
    if (metricFailures == 0) {
        std::printf("[PASS] MetricTensor::evaluate() handles bad coordinates\n");
    } else {
        std::printf("[FAIL] MetricTensor::evaluate(): %d bad-coordinate cases leaked NaN/Inf\n",
                     metricFailures);
        ++failures;
    }

    if (failures == 0) {
        std::printf("=== ALL FUZZ TESTS PASSED ===\n");
        return 0;
    }
    std::fprintf(stderr, "%d instrument(s) had fuzz failures.\n", failures);
    return 1;
}
