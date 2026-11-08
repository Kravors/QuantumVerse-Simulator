#include <QDebug>
#include <QTest>
#include "discovery/FindingsModel.h"
#include "ml/AnomalyDetector.h"
#include "ml/AnomalyMonitor.h"
#include "spacetime/MetricTensor.h"

class MinimalTest : public QObject {
    Q_OBJECT
   private slots:
    void test_basic() {
        qDebug() << "Test with all anomaly monitor headers running";
        QVERIFY(true);
    }
};

QTEST_MAIN(MinimalTest)
#include "test_minimal_headers.moc"
