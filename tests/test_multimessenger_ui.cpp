// QuantumVerse Test: Live Multi-Messenger Alerts (UI wiring)
// TDD validation that LIGO/IceCube alerts flow into the FindingsModel.
// Each adapter exposes a callback; when an alert is pushed (simulateAlert)
// it is converted to an InstrumentFinding and appended to the model, so the
// QML Anomaly Feed updates automatically.

#include <QCoreApplication>

#include <iostream>

#include "data/LIGOAdapter.h"
#include "data/IceCubeAdapter.h"
#include "data/AlertToFinding.h"
#include "discovery/FindingsModel.h"
#include "test_assert.h"

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    std::cout << "=== Multi-Messenger UI Test ===" << std::endl;

    quantumverse::FindingsModel model;

    quantumverse::LIGOAdapter ligo;
    ligo.setCallback([&model](const quantumverse::GravitationalWaveAlert& alert) {
        model.addFinding(quantumverse::toInstrumentFinding(alert));
    });

    quantumverse::IceCubeAdapter icecube;
    icecube.setCallback([&model](const quantumverse::NeutrinoAlert& alert) {
        model.addFinding(quantumverse::toInstrumentFinding(alert));
    });

    // --- LIGO alert ----------------------------------------------------------
    quantumverse::GravitationalWaveAlert gw;
    gw.event_id = "GW250601A";
    gw.false_alarm_rate = 1e-6;
    gw.snr = 23.0;
    ligo.simulateAlert(gw);

    QV_CHECK(model.rowCount() == 1);
    QV_CHECK(model.data(model.index(0, 0),
               quantumverse::FindingsModel::InstrumentNameRole)
               .toString().toStdString() == "LIGO");
    QV_CHECK(model.data(model.index(0, 0),
               quantumverse::FindingsModel::SeverityRole)
               .toString().toStdString() == "CRITICAL");

    // --- IceCube alert -------------------------------------------------------
    quantumverse::NeutrinoAlert nu;
    nu.event_id = "IC250601A";
    nu.false_alarm_rate = 1e-4;
    nu.energy_tev = 145.0;
    icecube.simulateAlert(nu);

    QV_CHECK(model.rowCount() == 2);
    QV_CHECK(model.data(model.index(1, 0),
               quantumverse::FindingsModel::InstrumentNameRole)
               .toString().toStdString() == "IceCube");
    QV_CHECK(model.data(model.index(1, 0),
               quantumverse::FindingsModel::SeverityRole)
               .toString().toStdString() == "CRITICAL");

    std::cout << "All Multi-Messenger UI tests passed." << std::endl;
    return 0;
}
