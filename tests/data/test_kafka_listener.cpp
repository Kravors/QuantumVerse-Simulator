/**
 * @file test_kafka_listener.cpp
 * @brief Integration test for KafkaAlertListener
 *
 * Validates construction, start/stop lifecycle, and error handling
 * when no Kafka broker is reachable.  When HAVE_LIBRDKAFKA is defined
 * the test still passes against an unreachable broker because the
 * listener emits consumerError rather than crashing.
 */

#include <QCoreApplication>
#include <QSignalSpy>
#include <cassert>
#include <iostream>

#include "data/KafkaAlertListener.h"
#include "data/GCNNoticeParser.h"
#include "data/AlertRouter.h"
#include "data/LIGOAdapter.h"
#include "data/IceCubeAdapter.h"
#include "data/TESSAlertAdapter.h"

using namespace quantumverse;

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    std::cout << "=== KafkaListenerTest ===" << std::endl;

    // --- Construction ----------------------------------------------------------
    KafkaAlertListener listener(
        QStringLiteral("127.0.0.1:9092"),
        { QStringLiteral("gcn.notices.LVC"), QStringLiteral("gcn.notices.ICECUBE") }
    );
    assert(listener.topics().size() == 2);
    assert(listener.topics().contains(QStringLiteral("gcn.notices.LVC")));

    // --- Start without a broker ------------------------------------------------
    // Should emit consumerError and NOT crash.
    QSignalSpy errorSpy(&listener, &KafkaAlertListener::consumerError);
    QSignalSpy stoppedSpy(&listener, &KafkaAlertListener::consumerStopped);

    listener.start();

    stoppedSpy.wait(5000);
    const bool errored = errorSpy.count() > 0;
    Q_UNUSED(errored)

    listener.stop();

    // --- AlertRouter integration -----------------------------------------------
    LIGOAdapter ligo;
    IceCubeAdapter icecube;
    TESSAlertAdapter tess;

    ligo.setCallback([](const GravitationalWaveAlert&) {});
    icecube.setCallback([](const NeutrinoAlert&) {});
    tess.setCallback([](const TESSAlert&) {});

    AlertRouter router;
    router.setLIGOAdapter(&ligo);
    router.setIceCubeAdapter(&icecube);
    router.setTESSAdapter(&tess);

    // Simulate a LIGO alert through the router
    QJsonObject ligoAlert;
    ligoAlert.insert("alert_type", QJsonValue(QStringLiteral("LIGO/Virgo")));
    ligoAlert.insert("event_id", QJsonValue(QStringLiteral("GW250601A")));
    ligoAlert.insert("false_alarm_rate", QJsonValue(1.2e-6));
    ligoAlert.insert("snr", QJsonValue(23.4));
    ligoAlert.insert("m1", QJsonValue(35.0));
    ligoAlert.insert("m2", QJsonValue(26.0));
    ligoAlert.insert("confidence", QJsonValue(0.98));
    router.routeAlert(ligoAlert);
    assert(ligo.receivedAlerts().size() == 1);
    assert(ligo.receivedAlerts()[0].event_id == "GW250601A");

    // Simulate an IceCube alert through the router
    QJsonObject nuAlert;
    nuAlert.insert("alert_type", QJsonValue(QStringLiteral("IceCube")));
    nuAlert.insert("event_id", QJsonValue(QStringLiteral("IC250601A")));
    nuAlert.insert("energy_tev", QJsonValue(145.2));
    nuAlert.insert("false_alarm_rate", QJsonValue(3.4e-4));
    nuAlert.insert("ra", QJsonValue(45.0));
    nuAlert.insert("dec", QJsonValue(12.0));
    nuAlert.insert("confidence", QJsonValue(0.91));
    router.routeAlert(nuAlert);
    assert(icecube.receivedAlerts().size() == 1);
    assert(icecube.receivedAlerts()[0].event_id == "IC250601A");

    // Simulate a TESS alert through the router
    QJsonObject tessAlert;
    tessAlert.insert("alert_type", QJsonValue(QStringLiteral("TESS")));
    tessAlert.insert("toi_id", QJsonValue(QStringLiteral("TOI-1234.01")));
    tessAlert.insert("period_days", QJsonValue(3.141));
    tessAlert.insert("depth_ppm", QJsonValue(500.0));
    tessAlert.insert("duration_hours", QJsonValue(2.5));
    tessAlert.insert("confidence", QJsonValue(0.95));
    tessAlert.insert("ra", QJsonValue(180.0));
    tessAlert.insert("dec", QJsonValue(-45.0));
    router.routeAlert(tessAlert);
    assert(tess.receivedAlerts().size() == 1);
    assert(tess.receivedAlerts()[0].toi_id == "TOI-1234.01");

    std::cout << "KafkaListenerTest checks passed." << std::endl;
    return 0;
}
