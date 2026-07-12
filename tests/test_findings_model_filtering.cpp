// QuantumVerse Test: FindingsModel filtering, sorting, and detail view
// TDD validation that the FindingsModel supports severity/instrument
// filtering, timestamp/severity/confidence sorting, and a detail view for
// the currently selected finding.

#include <QCoreApplication>

#include <cassert>
#include <iostream>
#include <vector>

#include "discovery/FindingsModel.h"
#include "discovery/DiscoveryInstrument.h"
#include "spacetime/Event4D.h"

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

    assert(model.rowCount() == 4 && "Should have 4 findings");
    assert(model.count() == 4);

    // --- Severity filter ---
    model.setFilterSeverity("HIGH");
    assert(model.rowCount() == 1 && "Severity=HIGH should leave 1");
    assert(model.data(model.index(0, 0),
               quantumverse::FindingsModel::SeverityRole).toString().toStdString() == "HIGH");

    model.setFilterSeverity("CRITICAL");
    assert(model.rowCount() == 1 && "Severity=CRITICAL should leave 1");

    model.setFilterSeverity(""); // clear
    assert(model.rowCount() == 4 && "Empty severity filter restores all");

    // --- Instrument filter ---
    model.setFilterInstrument("LIGO");
    assert(model.rowCount() == 2 && "Instrument=LIGO should leave 2");
    assert(model.data(model.index(0, 0),
               quantumverse::FindingsModel::InstrumentNameRole).toString().toStdString() == "LIGO");

    model.setFilterInstrument(""); // clear
    assert(model.rowCount() == 4);

    // --- Combined filter ---
    model.setFilterSeverity("LOW");
    model.setFilterInstrument("IceCube");
    assert(model.rowCount() == 1 && "Combined LIGO? no, IceCube+LOW => 1");
    model.setFilterSeverity("");
    model.setFilterInstrument("");

    // --- Sort by timestamp (default descending => newest first) ---
    model.setSortRole("timestamp");
    model.setSortAscending(false);
    assert(model.data(model.index(0, 0),
               quantumverse::FindingsModel::TimestampRole).toDouble() == 20.0);
    assert(model.data(model.index(3, 0),
               quantumverse::FindingsModel::TimestampRole).toDouble() == 1.0);

    model.setSortAscending(true); // oldest first
    assert(model.data(model.index(0, 0),
               quantumverse::FindingsModel::TimestampRole).toDouble() == 1.0);
    assert(model.data(model.index(3, 0),
               quantumverse::FindingsModel::TimestampRole).toDouble() == 20.0);

    // --- Sort by severity (descending => CRITICAL first) ---
    model.setSortRole("severity");
    model.setSortAscending(false);
    assert(model.data(model.index(0, 0),
               quantumverse::FindingsModel::SeverityRole).toString().toStdString() == "CRITICAL");
    assert(model.data(model.index(3, 0),
               quantumverse::FindingsModel::SeverityRole).toString().toStdString() == "LOW");

    model.setSortAscending(true); // INFO/LOW first
    assert(model.data(model.index(0, 0),
               quantumverse::FindingsModel::SeverityRole).toString().toStdString() == "LOW");

    // --- Detail view ---
    model.setSortRole("timestamp");
    model.setSortAscending(false); // first is ts=20 (LIGO MEDIUM)
    model.select(0);
    QVariantMap detail = model.currentFinding();
    assert(!detail.isEmpty() && "Detail should not be empty after select");
    assert(detail["instrumentName"].toString().toStdString() == "LIGO");
    assert(std::abs(detail["timestamp"].toDouble() - 20.0) < 1e-9);

    model.select(-1); // deselect
    assert(model.currentFinding().isEmpty() && "Deselect clears detail");

    std::cout << "All FindingsModel filtering/sort/detail tests passed." << std::endl;
    return 0;
}
