#include <QTest>
#include <QDebug>

class MinimalTest : public QObject
{
    Q_OBJECT
private slots:
    void test_basic()
    {
        qDebug() << "MinimalTest running";
        QVERIFY(true);
    }
};

QTEST_MAIN(MinimalTest)
#include "test_minimal_qt.moc"
