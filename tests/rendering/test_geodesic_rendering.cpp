#include <QTest>
#include <QObject>

class GeodesicRenderingTest : public QObject
{
    Q_OBJECT

private slots:
    void test_trivial()
    {
        QVERIFY(true);
    }
};

QTEST_MAIN(GeodesicRenderingTest)
#include "test_geodesic_rendering.moc"
