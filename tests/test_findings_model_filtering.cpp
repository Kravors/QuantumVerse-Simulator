// QuantumVerse Test: FindingsModel filtering, sorting, and detail view
// TDD validation that the FindingsModel supports severity/instrument
// filtering, timestamp/severity/confidence sorting, and a detail view for
// the currently selected finding.

#include <QCoreApplication>

#include <iostream>
#include <vector>

#include "discovery/FindingsModel.h"
#include "discovery/DiscoveryInstrument.h"
#include "spacetime/Event4D.h"
#include "test_assert.h"

namespace quantumverse {

static InstrumentFinding make(const std::string& inst, AlertSeverity sev,
                              double ts, double conf)
{
    InstrumentFinding f;
    f.id = inst + "_" + std::to_string(static_cast<int>(ts));
    f.instrumentName = inst;
    f.severity = sev;
    f.confidence = conf;
    f.timestamp = ts;
    f.description = "finding from " + inst;
    return f;
}

} // namespace quantumverse

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    std::cout << "=== FindingsModel Filtering/Sort/Detail Test ===" << std::endl;

    quantumverse::FindingsModel model;
    model.addFinding(quantumverse::make("LIGO", quantumverse::AlertSeverity::CRITICAL, 10.0, 0.99));
    model.addFinding(quantumverse::make("IceCube", quantumverse::AlertSeverity::HIGH, 5.0, 0.96));
    model.addFinding(quantumverse::make("LIGO", quantumverse::AlertSeverity::MEDIUM, 20.0, 0.85));
    model.addFinding(quantumverse::make("IceCube", quantumverse::AlertSeverity::LOW, 1.0, 0.55));

    QV_CHECK(model.rowCount() == 4);
    QV_CHECK(model.count() == 4);

    // --- Severity filter ---
    model.setFilterSeverity("HIGH");
    QV_CHECK(model.rowCount() == 1);
    QV_CHECK(model.data(model.index(0, 0),
               quantumverse::FindingsModel::SeverityRole).toString().toStdString() == "HIGH");

    model.setFilterSeverity("CRITICAL");
    QV_CHECK(model.rowCount() == 1);

    model.setFilterSeverity(""); // clear
    QV_CHECK(model.rowCount() == 4);

    // --- Instrument filter ---
    model.setFilterInstrument("LIGO");
    QV_CHECK(model.rowCount() == 2);
    QV_CHECK(model.data(model.index(0, 0),
               quantumverse::FindingsModel::InstrumentNameRole).toString().toStdString() == "LIGO");

    model.setFilterInstrument(""); // clear
    QV_CHECK(model.rowCount() == 4);

    // --- Combined filter ---
    model.setFilterSeverity("LOW");
    model.setFilterInstrument("IceCube");
    QV_CHECK(model.rowCount() == 1);
    model.setFilterSeverity("");
    model.setFilterInstrument("");

    // --- Sort by timestamp (default descending => newest first) ---
    model.setSortRole("timestamp");
    model.setSortAscending(false);
    QV_CHECK(model.data(model.index(0, 0),
               quantumverse::FindingsModel::TimestampRole).toDouble() == 20.0);
    QV_CHECK(model.data(model.index(3, 0),
               quantumverse::FindingsModel::TimestampRole).toDouble() == 1.0);

    model.setSortAscending(true); // oldest first
    QV_CHECK(model.data(model.index(0, 0),
               quantumverse::FindingsModel::TimestampRole).toDouble() == 1.0);
    QV_CHECK(model.data(model.index(3, 0),
               quantumverse::FindingsModel::TimestampRole).toDouble() == 20.0);

    // --- Sort by severity (descending => CRITICAL first) ---
    model.setSortRole("severity");
    model.setSortAscending(false);
    QV_CHECK(model.data(model.index(0, 0),
               quantumverse::FindingsModel::SeverityRole).toString().toStdString() == "CRITICAL");
    QV_CHECK(model.data(model.index(3, 0),
               quantumverse::FindingsModel::SeverityRole).toString().toStdString() == "LOW");

    model.setSortAscending(true); // INFO/LOW first
    QV_CHECK(model.data(model.index(0, 0),
               quantumverse::FindingsModel::SeverityRole).toString().toStdString() == "LOW");

    // --- Detail view ---
    model.setSortRole("timestamp");
    model.setSortAscending(false); // first is ts=20 (LIGO MEDIUM)
    model.select(0);
    QVariantMap detail = model.currentFinding();
    QV_CHECK(!detail.isEmpty());
    QV_CHECK(detail["instrumentName"].toString().toStdString() == "LIGO");
    QV_CHECK_NEAR(detail["timestamp"].toDouble() - 20.0, 0.0, 1e-9);

    model.select(-1); // deselect
    QV_CHECK(model.currentFinding().isEmpty());

    std::cout << "All FindingsModel filtering/sort/detail tests passed." << std::endl;
    return 0;
}
