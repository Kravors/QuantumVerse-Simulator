/**
 * @file test_ui_wiring.cpp
 * @brief UI wiring tests (Layer 4: behavioral stub-catcher + Layer 2: multi-probe)
 *
 * Validates that runParameterSweep actually produces varying physics rows,
 * exportCurrentFindings writes a non-empty CSV, and PhysicsValidator covers
 * N probes including the origin clamp (probe E).
 *
 * Builds with QUANTUMVERSE_USE_QML (DiscoveryPanelManager is QObject-based).
 */

#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

#include "discovery/DiscoveryPanelManager.h"
#include "physics/PhysicsValidator.h"
#include "spacetime/Event4D.h"
#include "spacetime/KerrMetric.h"
#include "spacetime/MetricTensor.h"
#include "test_assert.h"

using namespace quantumverse;

namespace {
Event4D makeProbe(double r, double theta) {
    return Event4D(0.0, r * std::sin(theta), 0.0, r * std::cos(theta));
}
}  // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    std::cout << "=== UI Wiring Tests (Layer 4 + Layer 2) ===" << std::endl;
    std::cout.flush();

    // -----------------------------------------------------------------------
    // Test 1: SweepProducesVaryingRows
    // runParameterSweep must reconstruct the metric per step so K varies.
    // -----------------------------------------------------------------------
    {
        DiscoveryPanelManager mgr;
        mgr.runParameterSweep(0.1, 10.0, 50, 0.0, 0.0, 1e5);
        auto results = mgr.sweepResults();
        QV_CHECK(results.size() == 50);

        double minK = std::numeric_limits<double>::max();
        double maxK = 0.0;
        for (const auto& item : results) {
            QVariantMap row = item.toMap();
            double k = row["kretschmann"].toDouble();
            minK = std::min(minK, k);
            maxK = std::max(maxK, k);
        }
        QV_CHECK(minK > 0.0);
        QV_CHECK(maxK / minK > 1.001);  // rows must actually vary

        std::cout << "[PASS] SweepProducesVaryingRows — minK=" << minK
                  << " maxK=" << maxK << std::endl;
        std::cout.flush();
    }

    // -----------------------------------------------------------------------
    // Test 2: ExportWritesNonEmptyCsv
    // exportCurrentFindings(path) must write a CSV with header + data rows.
    // -----------------------------------------------------------------------
    {
        DiscoveryPanelManager mgr;
        mgr.runParameterSweep(0.1, 10.0, 10, 0.0, 0.0, 1e5);

        QTemporaryDir tmpDir;
        QV_CHECK(tmpDir.isValid());
        QString path = tmpDir.path() + "/sweep_test.csv";
        mgr.exportCurrentFindings(path);

        QV_CHECK(QFile::exists(path));
        QFile file(path);
        QV_CHECK(file.open(QIODevice::ReadOnly));
        QTextStream in(&file);
        QString content = in.readAll();
        file.close();
        QV_CHECK(!content.isEmpty());
        QV_CHECK(content.contains("step,"));
        QV_CHECK(content.contains("mass_msun,"));
        QV_CHECK(content.contains("kretschmann,"));

        std::cout << "[PASS] ExportWritesNonEmptyCsv — " << content.size()
                  << " bytes" << std::endl;
        std::cout.flush();
    }

    // -----------------------------------------------------------------------
    // Test 3: AllProbesValidate (A-D)
    // Same 4 checks at 4 probes (horizon → far field) all pass.
    // -----------------------------------------------------------------------
    {
        double mMsun = 10.0;
        double mKg = mMsun * 1.989e30;
        KerrMetric metric(mKg, 0.0);  // Schwarzschild limit
        double rs = metric.schwarzschildRadius();

        std::vector<Event4D> probes = {
            makeProbe(1.1 * rs, M_PI / 2.0),   // A: just outside horizon
            makeProbe(1.5 * rs, M_PI / 4.0),   // B: outside ergosphere
            makeProbe(1e5,      M_PI / 4.0),   // C: production probe
            makeProbe(1e10,     M_PI / 4.0),   // D: far field
        };

        PhysicsValidator validator(std::make_shared<MetricTensor>());
        auto result = validator.validateAtProbes(metric, probes);
        QV_CHECK(result.passed);
        QV_CHECK(result.totalChecks == 16);  // 4 probes × 4 checks
        QV_CHECK(result.totalFailures == 0);

        std::cout << "[PASS] AllProbesValidate — " << result.totalChecks
                  << " checks, " << result.totalFailures << " failures" << std::endl;
        std::cout.flush();
    }

    // -----------------------------------------------------------------------
    // Test 4: OriginProbeClampedNotInvalid (E)
    // r=0 must be clamped (valid=true, finite K) — locks in the Phase 5 fix.
    // -----------------------------------------------------------------------
    {
        double mMsun = 10.0;
        double mKg = mMsun * 1.989e30;
        KerrMetric metric(mKg, 0.0);

        Event4D origin(0.0, 0.0, 0.0, 0.0);  // r = 0
        auto cs = metric.curvatureScalars(origin);
        QV_CHECK(cs.valid);  // Origin probe must be clamped to valid
        QV_CHECK(std::isfinite(cs.kretschmann));
        QV_CHECK(cs.kretschmann >= 0.0);

        std::cout << "[PASS] OriginProbeClampedNotInvalid — K=" << cs.kretschmann
                  << " valid=" << cs.valid << std::endl;
        std::cout.flush();
    }

    std::cout << "All UI wiring tests passed." << std::endl;
    return 0;
}
