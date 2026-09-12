// QuantumVerse Test: FindingsModel
// TDD validation of the QML-facing FindingsModel (QAbstractListModel).
// Verifies that adding findings grows the row count and that role data is
// reported correctly, and that setFindings()/clear() replace/empty the list.

#include <QCoreApplication>

#include <cmath>
#include <iostream>
#include <vector>

#include "discovery/FindingsModel.h"
#include "discovery/DiscoveryInstrument.h"
#include "spacetime/Event4D.h"
#include "test_assert.h"

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    std::cout << "=== FindingsModel Test ===" << std::endl;

    quantumverse::FindingsModel model;
    QV_CHECK(model.rowCount() == 0);
    QV_CHECK(model.count() == 0);

    quantumverse::InstrumentFinding f1;
    f1.id = "F1";
    f1.instrumentName = "ExoplanetaryTTVFifthForceHunter";
    f1.description = "Anomalous TTV residuals detected";
    f1.severity = quantumverse::AlertSeverity::HIGH;
    f1.confidence = 0.97;
    f1.timestamp = 12.5;
    f1.location = quantumverse::Event4D(12.5, 1.0, 2.0, 3.0);

    model.addFinding(f1);
    QV_CHECK(model.rowCount() == 1);
    QV_CHECK(model.count() == 1);

    QModelIndex idx = model.index(0, 0);
    QV_CHECK(model.data(idx, quantumverse::FindingsModel::InstrumentNameRole)
               .toString().toStdString() == "ExoplanetaryTTVFifthForceHunter");
    QV_CHECK(model.data(idx, quantumverse::FindingsModel::DescriptionRole)
               .toString().toStdString() == "Anomalous TTV residuals detected");
    QV_CHECK(model.data(idx, quantumverse::FindingsModel::SeverityRole)
               .toString().toStdString() == "HIGH");
    QV_CHECK_NEAR(model.data(idx, quantumverse::FindingsModel::ConfidenceRole)
                        .toDouble() - 0.97, 0.0, 1e-9);
    QV_CHECK_NEAR(model.data(idx, quantumverse::FindingsModel::TimestampRole)
                        .toDouble() - 12.5, 0.0, 1e-9);
    QV_CHECK_NEAR(model.data(idx, quantumverse::FindingsModel::XRole)
                        .toDouble() - 1.0, 0.0, 1e-9);

    quantumverse::InstrumentFinding f2;
    f2.id = "F2";
    f2.instrumentName = "UltralightDMWaveInterferometer";
    f2.severity = quantumverse::AlertSeverity::CRITICAL;
    f2.confidence = 1.0;
    model.addFinding(f2);
    QV_CHECK(model.rowCount() == 2);
    QV_CHECK(model.data(model.index(1, 0), quantumverse::FindingsModel::SeverityRole)
               .toString().toStdString() == "CRITICAL");

    // setFindings() replaces the whole list
    std::vector<quantumverse::InstrumentFinding> list = { f1 };
    model.setFindings(list);
    QV_CHECK(model.rowCount() == 1);
    QV_CHECK(model.data(model.index(0, 0), quantumverse::FindingsModel::InstrumentNameRole)
               .toString().toStdString() == "ExoplanetaryTTVFifthForceHunter");

    model.clear();
    QV_CHECK(model.rowCount() == 0);

    std::cout << "All FindingsModel tests passed." << std::endl;
    return 0;
}
