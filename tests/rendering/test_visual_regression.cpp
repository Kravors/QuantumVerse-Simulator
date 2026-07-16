// Visual regression test for QuantumVerse headless rendering
#include <QTest>
#include <QProcess>
#include <QImage>
#include <QFile>
#include <QDir>
#include <QtMath>

class VisualRegressionTest : public QObject
{
    Q_OBJECT

private slots:
    void test_default_rendering_is_stable()
    {
        runScreenshotTest("default");
    }

private:
    void runScreenshotTest(const QString &name)
    {
        QString exePath = QCoreApplication::applicationDirPath() + "/quantumverse_qml";
#ifdef Q_OS_WIN
        exePath += ".exe";
#endif

        QString tempOutput = QDir::tempPath() + QString("/qv_test_%1.png").arg(name);
        QString referencePath = QDir::currentPath() + "/tests/reference/" + name + "_baseline.png";

        QFile::remove(tempOutput);

        QProcess process;
        QStringList args;
        args << "--frames" << "1" << "--screenshot" << tempOutput;

        process.start(exePath, args);
        bool finished = process.waitForFinished(30000);
        QVERIFY2(finished, qPrintable("Process timed out or failed to start: " + process.errorString()));
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.exitCode(), 0);

        QVERIFY(QFile::exists(tempOutput));
        QVERIFY(QFile::exists(referencePath));

        QImage testImg(tempOutput);
        QImage refImg(referencePath);

        QVERIFY(!testImg.isNull());
        QVERIFY(!refImg.isNull());
        QCOMPARE(testImg.size(), refImg.size());

        int differentPixels = 0;
        const int totalPixels = testImg.width() * testImg.height();
        const int threshold = 5;

        for (int y = 0; y < testImg.height(); ++y) {
            for (int x = 0; x < testImg.width(); ++x) {
                QColor tc = testImg.pixelColor(x, y);
                QColor rc = refImg.pixelColor(x, y);
                if (qAbs(tc.red() - rc.red()) > threshold ||
                    qAbs(tc.green() - rc.green()) > threshold ||
                    qAbs(tc.blue() - rc.blue()) > threshold) {
                    differentPixels++;
                }
            }
        }

        double pctDiff = (double)differentPixels / totalPixels * 100.0;
        qDebug() << "Percentage of pixels different:" << pctDiff;

        QVERIFY2(pctDiff <= 1.0, qPrintable(
            QString("Visual regression: %1% pixels differ (>%2 threshold)")
                .arg(pctDiff, 0, 'f', 3)
                .arg(threshold)));

        QFile::remove(tempOutput);
    }
};

QTEST_MAIN(VisualRegressionTest)
#include "test_visual_regression.moc"
