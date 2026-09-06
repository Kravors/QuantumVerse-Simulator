#include <QTest>
#include <QDebug>
#include "ml/AnomalyMonitor.h"
#include "ml/AnomalyDetector.h"
#include "discovery/FindingsModel.h"
#include "spacetime/MetricTensor.h"

class MinimalTest : public QObject
{
    Q_OBJECT
private slots:
    void test_construct()
    {
        qDebug() << "Constructing AnomalyMonitor...";
        quantumverse::AnomalyMonitor monitor;
        qDebug() << "AnomalyMonitor constructed OK";
        QVERIFY(true);
    }
};

QTEST_MAIN(MinimalTest)
#include "test_minimal_construct.moc"
