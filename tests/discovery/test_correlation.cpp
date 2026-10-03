// Phase 2: Sim-frame correlation test
// Validates MultiMessengerCorrelator::correlateFindings() pairwise
// coincidence detection in simulation coordinates (meters + seconds).

#include <QCoreApplication>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>

#include "data/MultiMessengerCorrelator.h"
#include "spacetime/Event4D.h"
#include "discovery/DiscoveryInstrument.h"
#include "test_assert.h"

using namespace quantumverse;

namespace {
constexpr double kEps = 1e-9;

InstrumentFinding makeFinding(const std::string& id,
                              const std::string& instrument,
                              double t, double x, double y, double z,
                              double confidence = 0.9)
{
    InstrumentFinding f;
    f.id = id;
    f.instrumentName = instrument;
    f.confidence = confidence;
    f.timestamp = t;
    f.location = Event4D(t, x, y, z);
    return f;
}
} // anonymous namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    std::cout << "=== SimFrameCorrelationTest ===" << std::endl;

    MultiMessengerCorrelator correlator(nullptr);

    // T1: same location, same time → correlated
    {
        std::vector<InstrumentFinding> findings = {
            makeFinding("f1", "GWScanner", 0.0, 1.0, 2.0, 3.0),
            makeFinding("f2", "NeutrinoBurst", 0.0, 1.0, 2.0, 3.0),
        };

        auto events = correlator.correlateFindings(findings, 1.0e6, 1.0);

        QV_CHECK(events.size() == 1);
        QV_CHECK_NEAR(events[0].spatialDistance, 0.0, kEps);
        QV_CHECK_NEAR(events[0].timeSeparation, 0.0, kEps);
        QV_CHECK(events[0].combinedConfidence > 0.0);
        QV_CHECK(events[0].combinedConfidence <= 1.0);
    }

    // T2: beyond spatial threshold → not correlated
    {
        std::vector<InstrumentFinding> findings = {
            makeFinding("f1", "GWScanner", 0.0, 0.0, 0.0, 0.0),
            makeFinding("f2", "NeutrinoBurst", 0.0, 1.0e9, 0.0, 0.0),
        };

        auto events = correlator.correlateFindings(findings, 1.0e6, 1.0);

        QV_CHECK(events.empty());
    }

    // T3: beyond time window → not correlated
    {
        std::vector<InstrumentFinding> findings = {
            makeFinding("f1", "GWScanner", 0.0, 1.0, 2.0, 3.0),
            makeFinding("f2", "NeutrinoBurst", 1.0e6, 1.0, 2.0, 3.0),
        };

        auto events = correlator.correlateFindings(findings, 1.0e6, 1.0);

        QV_CHECK(events.empty());
    }

    // T4: three findings equally spaced → three correlations (all pairs within threshold)
    {
        std::vector<InstrumentFinding> findings = {
            makeFinding("f1", "GWScanner", 0.0, 0.0, 0.0, 0.0),
            makeFinding("f2", "NeutrinoBurst", 0.0, 100.0, 0.0, 0.0),
            makeFinding("f3", "FermiGBM",    0.0, 200.0, 0.0, 0.0),
        };

        auto events = correlator.correlateFindings(findings, 1.0e6, 1.0);

        QV_CHECK(events.size() == 3);

        int f1f2 = 0, f1f3 = 0, f2f3 = 0;
        for (const auto& ev : events) {
            QString ids = ev.alertIds.join(",");
            if (ids.contains("f1") && ids.contains("f2")) ++f1f2;
            if (ids.contains("f1") && ids.contains("f3")) ++f1f3;
            if (ids.contains("f2") && ids.contains("f3")) ++f2f3;
        }
        QV_CHECK(f1f2 == 1);
        QV_CHECK(f1f3 == 1);
        QV_CHECK(f2f3 == 1);
    }

    // T5: single finding → no self-correlation
    {
        std::vector<InstrumentFinding> findings = {
            makeFinding("f1", "GWScanner", 0.0, 1.0, 2.0, 3.0),
        };

        auto events = correlator.correlateFindings(findings, 1.0e6, 1.0);

        QV_CHECK(events.empty());
    }

    // T6: empty findings → no crash
    {
        std::vector<InstrumentFinding> findings;
        auto events = correlator.correlateFindings(findings, 1.0e6, 1.0);
        QV_CHECK(events.empty());
    }

    std::cout << "All correlation checks passed." << std::endl;
    return 0;
}
