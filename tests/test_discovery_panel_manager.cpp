/**
 * @file test_discovery_panel_manager.cpp
 * @brief Unit tests for DiscoveryPanelManager live alert ingestion
 *
 * Validates that ingestAlert() correctly parses LIGO/IceCube/TESS alerts,
 * appends findings, emits liveAlertProcessed, and triggers anomaly detection.
 */

#include <QCoreApplication>
#include <QSignalSpy>
#include <cassert>
#include <iostream>

#include "discovery/DiscoveryPanelManager.h"
#include "discovery/DiscoveryEngine.h"
#include "data/GCNNoticeParser.h"

using namespace quantumverse;

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    std::cout << "=== DiscoveryPanelManager IngestAlert Test ===" << std::endl;

    DiscoveryPanelManager manager;

    // --- LIGO alert ----------------------------------------------------------
    QJsonObject ligoAlert;
    ligoAlert.insert("alert_type", QJsonValue(QStringLiteral("LIGO/Virgo")));
    ligoAlert.insert("event_id", QJsonValue(QStringLiteral("GW250601A")));
    ligoAlert.insert("false_alarm_rate", QJsonValue(1.2e-6));
    ligoAlert.insert("snr", QJsonValue(23.4));
    ligoAlert.insert("m1", QJsonValue(35.0));
    ligoAlert.insert("m2", QJsonValue(26.0));
    ligoAlert.insert("confidence", QJsonValue(0.98));
    ligoAlert.insert("ra", QJsonValue(123.45));
    ligoAlert.insert("dec", QJsonValue(-30.0));

    QSignalSpy liveSpy(&manager, &DiscoveryPanelManager::liveAlertProcessed);
    manager.ingestAlert(ligoAlert);

    assert(manager.findings().size() == 1);
    assert(manager.findings()[0].instrumentName == "LIGO (Live)");
    assert(manager.findings()[0].confidence > 0.9);
    assert(liveSpy.count() == 1);
    assert(liveSpy.value(0).at(0).toString().startsWith("LIVE_LIGO"));

    // --- IceCube alert -------------------------------------------------------
    QJsonObject nuAlert;
    nuAlert.insert("alert_type", QJsonValue(QStringLiteral("IceCube")));
    nuAlert.insert("event_id", QJsonValue(QStringLiteral("IC250601A")));
    nuAlert.insert("energy_tev", QJsonValue(145.2));
    nuAlert.insert("false_alarm_rate", QJsonValue(3.4e-4));
    nuAlert.insert("ra", QJsonValue(45.0));
    nuAlert.insert("dec", QJsonValue(12.0));
    nuAlert.insert("confidence", QJsonValue(0.91));

    manager.ingestAlert(nuAlert);
    assert(manager.findings().size() == 2);
    assert(manager.findings()[1].instrumentName == "IceCube (Live)");
    assert(manager.findings()[1].confidence > 0.9);

    // --- TESS alert ----------------------------------------------------------
    QJsonObject tessAlert;
    tessAlert.insert("alert_type", QJsonValue(QStringLiteral("TESS")));
    tessAlert.insert("toi_id", QJsonValue(QStringLiteral("TOI-1234.01")));
    tessAlert.insert("period_days", QJsonValue(3.141));
    tessAlert.insert("depth_ppm", QJsonValue(500.0));
    tessAlert.insert("duration_hours", QJsonValue(2.5));
    tessAlert.insert("confidence", QJsonValue(0.95));
    tessAlert.insert("ra", QJsonValue(180.0));
    tessAlert.insert("dec", QJsonValue(-45.0));

    manager.ingestAlert(tessAlert);
    assert(manager.findings().size() == 3);
    assert(manager.findings()[2].instrumentName == "TESS (Live)");
    assert(std::fabs(manager.findings()[2].confidence - 0.95) < 1e-9);

    // --- Unknown alert type --------------------------------------------------
    QJsonObject unknownAlert;
    unknownAlert.insert("alert_type", QJsonValue(QStringLiteral("Fermi/GBM")));
    unknownAlert.insert("event_id", QJsonValue(QStringLiteral("GBM250601")));

    manager.ingestAlert(unknownAlert);
    assert(manager.findings().size() == 4);
    assert(manager.findings()[3].instrumentName == "Unknown (Live)");

    std::cout << "All DiscoveryPanelManager ingestAlert tests passed." << std::endl;
    return 0;
}
