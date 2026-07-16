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

extern "C" {
    int LLVMFuzzerInitialize(int* argc, char*** argv) { return 0; }

    int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
        if (size < sizeof(double) * 8) return 0;

        const double* vals = reinterpret_cast<const double*>(data);

        double t = vals[0];
        double x = vals[1];
        double y = vals[2];
        double z = vals[3];
        double mass = std::abs(vals[4]) + 1e-6;
        double theta = std::abs(vals[5]) * M_PI;
        double phi = std::abs(vals[6]) * 2.0 * M_PI;

        if (!std::isfinite(t) || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
            return 0;
        }

        Event4D location(t, x, y, z);
        auto metric = std::make_shared<SchwarzschildMetric>(mass * 1.989e30);

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

        for (const auto& inst : instruments) {
            std::vector<Event4D> trajectory;
            for (int i = 0; i < 10; i++) {
                trajectory.push_back(Event4D(
                    location.t + static_cast<double>(i),
                    location.x + static_cast<double>(i) * 1e3,
                    location.y + static_cast<double>(i) * 1e3,
                    location.z
                ));
            }

            auto findings = inst->analyze(*metric, location, trajectory);
            if (hasNaNOrInf(findings)) {
                fprintf(stderr, "NaN/Inf finding from %s at (%f,%f,%f,%f)\n",
                        inst->getName().c_str(), t, x, y, z);
                __builtin_trap();
                return 1;
            }
        }

        return 0;
    }
}
