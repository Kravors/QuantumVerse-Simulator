// QuantumVerse Test: FindingsModel
// TDD validation of the QML-facing FindingsModel (QAbstractListModel).
// Verifies that adding findings grows the row count and that role data is
// reported correctly, and that setFindings()/clear() replace/empty the list.

#include <QCoreApplication>

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "discovery/FindingsModel.h"
#include "discovery/DiscoveryInstrument.h"
#include "spacetime/Event4D.h"

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    std::cout << "=== FindingsModel Test ===" << std::endl;

    quantumverse::FindingsModel model;
    assert(model.rowCount() == 0 && "Empty model should have 0 rows");
    assert(model.count() == 0 && "Empty model should report count 0");

    quantumverse::InstrumentFinding f1;
    f1.id = "F1";
    f1.instrumentName = "ExoplanetaryTTVFifthForceHunter";
    f1.description = "Anomalous TTV residuals detected";
    f1.severity = quantumverse::AlertSeverity::HIGH;
    f1.confidence = 0.97;
    f1.timestamp = 12.5;
    f1.location = quantumverse::Event4D(12.5, 1.0, 2.0, 3.0);

    model.addFinding(f1);
    assert(model.rowCount() == 1 && "Row count should be 1 after add");
    assert(model.count() == 1 && "count() should be 1 after add");

    QModelIndex idx = model.index(0, 0);
    assert(model.data(idx, quantumverse::FindingsModel::InstrumentNameRole)
               .toString().toStdString() == "ExoplanetaryTTVFifthForceHunter");
    assert(model.data(idx, quantumverse::FindingsModel::DescriptionRole)
               .toString().toStdString() == "Anomalous TTV residuals detected");
    assert(model.data(idx, quantumverse::FindingsModel::SeverityRole)
               .toString().toStdString() == "HIGH");
    assert(std::abs(model.data(idx, quantumverse::FindingsModel::ConfidenceRole)
                        .toDouble() - 0.97) < 1e-9);
    assert(std::abs(model.data(idx, quantumverse::FindingsModel::TimestampRole)
                        .toDouble() - 12.5) < 1e-9);
    assert(std::abs(model.data(idx, quantumverse::FindingsModel::XRole)
                        .toDouble() - 1.0) < 1e-9);

    quantumverse::InstrumentFinding f2;
    f2.id = "F2";
    f2.instrumentName = "UltralightDMWaveInterferometer";
    f2.severity = quantumverse::AlertSeverity::CRITICAL;
    f2.confidence = 1.0;
    model.addFinding(f2);
    assert(model.rowCount() == 2 && "Row count should be 2 after second add");
    assert(model.data(model.index(1, 0), quantumverse::FindingsModel::SeverityRole)
               .toString().toStdString() == "CRITICAL");

    // setFindings() replaces the whole list
    std::vector<quantumverse::InstrumentFinding> list = { f1 };
    model.setFindings(list);
    assert(model.rowCount() == 1 && "setFindings should reset to 1 row");
    assert(model.data(model.index(0, 0), quantumverse::FindingsModel::InstrumentNameRole)
               .toString().toStdString() == "ExoplanetaryTTVFifthForceHunter");

    model.clear();
    assert(model.rowCount() == 0 && "clear() should empty the model");

    std::cout << "All FindingsModel tests passed." << std::endl;
    return 0;
}
