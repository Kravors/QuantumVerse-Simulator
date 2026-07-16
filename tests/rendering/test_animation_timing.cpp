// Animation timing test for QuantumVerse headless rendering
// Verifies that frame times are consistent (95% under threshold).

#include <QTest>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QVector>
#include <algorithm>
#include <cmath>

class AnimationTimingTest : public QObject
{
    Q_OBJECT

private slots:
    void test_frame_time_consistency()
    {
        QString exePath = QCoreApplication::applicationDirPath() + "/quantumverse_qml";
#ifdef Q_OS_WIN
        exePath += ".exe";
#endif

        QString dumpFile = QDir::tempPath() + "/qv_frame_times.txt";
        QFile::remove(dumpFile);

        QProcess process;
        QStringList args;
        args << "--frames" << "200"
             << "--dump-frame-times" << dumpFile;

        process.start(exePath, args);
        bool finished = process.waitForFinished(60000);
        QVERIFY2(finished, qPrintable("Process timed out or failed to start: " + process.errorString()));
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.exitCode(), 0);

        QVERIFY(QFile::exists(dumpFile));

        QVector<double> deltas;
        {
            QFile f(dumpFile);
            QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
            QTextStream in(&f);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (!line.isEmpty()) {
                    bool ok = false;
                    double val = line.toDouble(&ok);
                    if (ok) {
                        deltas.push_back(val);
                    }
                }
            }
        }

        QVERIFY(!deltas.isEmpty());
        QVERIFY(deltas.size() >= 100);

        const double thresholdMs = 33.0;
        int violations = 0;
        for (double d : deltas) {
            if (d > thresholdMs) {
                violations++;
            }
        }

        double pctOver = 100.0 * violations / static_cast<double>(deltas.size());
        qDebug() << "Frame time stats: count=" << deltas.size()
                 << "violations=" << violations
                 << "pct_over=" << pctOver;

        QVERIFY2(pctOver <= 5.0,
                 qPrintable(QString("Frame time consistency failed: %1% of frames exceeded %2 ms")
                                .arg(pctOver, 0, 'f', 2)
                                .arg(thresholdMs)));

        QFile::remove(dumpFile);
    }
};

QTEST_MAIN(AnimationTimingTest)
#include "test_animation_timing.moc"
