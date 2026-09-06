// AnomalyMonitor integration test
#include <QTest>
#include <QSignalSpy>
#include <QJsonObject>
#include <QCoreApplication>
#include <memory>

#include "ml/AnomalyMonitor.h"
#include "ml/AnomalyDetector.h"
#include "discovery/FindingsModel.h"
#include "spacetime/MetricTensor.h"

class AnomalyMonitorTest : public QObject
{
    Q_OBJECT

private slots:
    void test_default_properties()
    {
        quantumverse::AnomalyMonitor monitor;
        QVERIFY(!monitor.enabled());
        QCOMPARE(monitor.thresholdMultiplier(), 1.0);
        QCOMPARE(monitor.sampleIntervalMs(), 1000);
        QCOMPARE(monitor.sampleCount(), 0);
        QCOMPARE(monitor.lastScore(), 0.0);
    }

    void test_setters()
    {
        quantumverse::AnomalyMonitor monitor;
        monitor.setEnabled(true);
        QVERIFY(monitor.enabled());

        monitor.setThresholdMultiplier(2.5);
        QCOMPARE(monitor.thresholdMultiplier(), 2.5);

        monitor.setSampleIntervalMs(500);
        QCOMPARE(monitor.sampleIntervalMs(), 500);

        monitor.setEnabled(false);
        QVERIFY(!monitor.enabled());
    }

    void test_start_stop()
    {
        quantumverse::AnomalyMonitor monitor;
        monitor.setSampleIntervalMs(50);
        monitor.start();
        QVERIFY(monitor.enabled());
        QTest::qWait(150);
        monitor.stop();
        QVERIFY(!monitor.enabled());
    }

    void test_anomaly_signal_with_model()
    {
        quantumverse::AnomalyMonitor monitor;
        monitor.setSampleIntervalMs(100);
        monitor.setThresholdMultiplier(0.1);

        auto detector = std::make_shared<quantumverse::AnomalyDetector>();
        QString modelPath = QCoreApplication::applicationDirPath() + "/../data/ml/anomaly_model.json";
        bool loaded = detector->load(modelPath.toStdString());
        if (!loaded) {
            qWarning() << "Skipping anomaly signal test: model not loaded";
            return;
        }
        monitor.setAnomalyDetector(detector);

        quantumverse::UI4D::PhysicsTelemetry telemetry{};
        telemetry.totalKineticEnergy = 1.0e30;
        telemetry.totalPotentialEnergy = -2.0e30;
        telemetry.totalEnergy = -1.0e30;
        telemetry.angularMomentumZ = 1.0e40;
        telemetry.earthOrbitalSpeed = 30000.0;
        telemetry.earthDistance = 1.496e11;
        telemetry.gravitationalWaveStrain = 1.0e-21;
        telemetry.gravitationalWaveFreq = 150.0;
        telemetry.blackHoleMass = 1.0e30;
        telemetry.bodyCount = 9;

        std::vector<quantumverse::UI4D::SolarSystemBody> bodies;
        bodies.push_back({"Earth", 5.972e24, 6.371e6,
            quantumverse::Event4D(0, 1.496e11, 0, 0),
            quantumverse::Event4D(0, 30000, 0, 0)});

        monitor.setTelemetryProvider([telemetry, bodies]() mutable {
            return std::make_pair(telemetry, bodies);
        });

        auto findingsModel = new quantumverse::FindingsModel(this);
        monitor.setFindingsProvider([findingsModel]() {
            return std::vector<quantumverse::InstrumentFinding>{};
        });

        QSignalSpy scoreSpy(&monitor, &quantumverse::AnomalyMonitor::scoreUpdated);
        QSignalSpy anomalySpy(&monitor, &quantumverse::AnomalyMonitor::anomalyDetected);

        monitor.start();
        QTest::qWait(300);
        monitor.stop();

        QVERIFY(monitor.sampleCount() > 0);
        QVERIFY(scoreSpy.count() >= monitor.sampleCount());
        if (monitor.lastScore() > detector->threshold() * monitor.thresholdMultiplier()) {
            QVERIFY(anomalySpy.count() > 0);
            QList<QVariant> args = anomalySpy.takeFirst();
            QJsonValue val = QJsonValue::fromVariant(args.first());
            QJsonObject anomaly = val.toObject();
            QVERIFY(anomaly.contains("timestamp"));
            QVERIFY(anomaly.contains("score"));
            QVERIFY(anomaly.contains("threshold"));
        }
    }
};

QTEST_MAIN(AnomalyMonitorTest)
#include "test_anomaly_monitor.moc"
