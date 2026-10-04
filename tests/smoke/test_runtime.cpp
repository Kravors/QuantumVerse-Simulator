/**
 * @file test_runtime.cpp
 * @brief Runtime smoke test for QuantumVerse headless launch
 *
 * Launches quantumverse_qml --headless --frames 300, captures stdout,
 * and asserts on the [REALITY] diagnostic dumps.
 */

#include <QProcess>
#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QFileInfo>
#include <QImage>
#include <QtTest/QtTest>

class RuntimeSmokeTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Locate the quantumverse_qml executable relative to the build tree
        const QStringList candidates = {
            QStringLiteral("./quantumverse_qml.exe"),
            QStringLiteral("./quantumverse_qml"),
            QStringLiteral("../build/Release/quantumverse_qml.exe"),
            QStringLiteral("../build/Debug/quantumverse_qml.exe"),
        };
        for (const auto& c : candidates) {
            if (QFileInfo::exists(c)) {
                m_binary = c;
                break;
            }
        }
        QVERIFY2(!m_binary.isEmpty(), "quantumverse_qml executable not found");
    }

    void testHeadlessRealityDump() {
        QProcess proc;
        proc.setProgram(m_binary);
        proc.setArguments({
            QStringLiteral("--headless"),
            QStringLiteral("--frames"),
            QStringLiteral("300"),
            QStringLiteral("--metric"),
            QStringLiteral("schwarzschild"),
        });
        proc.setProcessChannelMode(QProcess::MergedChannels);
        proc.start();
        const bool started = proc.waitForFinished(300000); // 5 minutes max
        if (!started) {
            qWarning() << "Process timed out or failed to start:" << proc.errorString();
        }
        QVERIFY2(started, "quantumverse_qml did not finish within timeout");
        QCOMPARE(proc.exitCode(), 0);

        const QByteArray rawOutput = proc.readAll();
        const QString output = QString::fromLocal8Bit(rawOutput.constData(), rawOutput.size());
        m_output = output;

        // Tokenize into lines
        const QStringList lines = output.split('\n', Qt::SkipEmptyParts);

        // Collect [REALITY] lines
        QStringList realityLines;
        QStringList errorLines;
        QString dataPathsLine;

        for (const auto& line : lines) {
            if (line.contains(QLatin1String("[ERROR]"))) {
                errorLines.append(line);
            }
            if (line.contains(QLatin1String("[DataPaths]"))) {
                dataPathsLine = line;
            }
            if (line.contains(QLatin1String("[REALITY]"))) {
                realityLines.append(line);
            }
        }

        // 1. >= 3 [REALITY] dumps
        qWarning() << "Reality dump count:" << realityLines.size();
        QVERIFY2(realityLines.size() >= 3,
                  qPrintable(QStringLiteral("Expected >= 3 [REALITY] dumps, got %1")
                                 .arg(realityLines.size())));

        // 2. bodies=9 in every dump
        for (const auto& line : realityLines) {
            QVERIFY2(line.contains(QLatin1String("bodies= 9")),
                     qPrintable(QStringLiteral("bodies != 9 in: %1").arg(line)));
        }

        // 3. simTime strictly increasing across dumps
        QList<double> simTimes;
        static const QRegularExpression simTimeRe("simTime=\\s*([0-9.]+)");
        for (const auto& line : realityLines) {
            const auto match = simTimeRe.match(line);
            QVERIFY2(match.hasMatch(),
                     qPrintable(QStringLiteral("No simTime in: %1").arg(line)));
            bool ok = false;
            const double t = match.captured(1).toDouble(&ok);
            QVERIFY2(ok, qPrintable(QStringLiteral("Failed to parse simTime in: %1").arg(line)));
            simTimes.append(t);
        }
        for (int i = 1; i < simTimes.size(); ++i) {
            QVERIFY2(simTimes[i] > simTimes[i - 1],
                     qPrintable(QStringLiteral("simTime not increasing: %1 -> %2")
                                    .arg(simTimes[i - 1]).arg(simTimes[i])));
        }

        // 4. gridZRange nonzero
        static const QRegularExpression gridRe("gridZRange=\\s*\"([^\"]+)\"");
        for (const auto& line : realityLines) {
            const auto match = gridRe.match(line);
            QVERIFY2(match.hasMatch(),
                     qPrintable(QStringLiteral("No gridZRange in: %1").arg(line)));
            const QString range = match.captured(1);
            QVERIFY2(range != QLatin1String("0..0"),
                     qPrintable(QStringLiteral("gridZRange is zero in: %1").arg(line)));
        }

        // 5. Zero [ERROR] lines
        qWarning() << "Error line count:" << errorLines.size();
        QVERIFY2(errorLines.isEmpty(),
                  qPrintable(QStringLiteral("Found %1 [ERROR] lines").arg(errorLines.size())));

        // 6. [DataPaths] resolves to source tree, not build/
        qWarning() << "DataPaths line:" << dataPathsLine;
        QVERIFY2(!dataPathsLine.isEmpty(),
                  "[DataPaths] diagnostic line not found in output");
        QVERIFY2(!dataPathsLine.contains(QLatin1String("build")),
                  qPrintable(QStringLiteral("DataPaths resolved to build/ tree: %1")
                                   .arg(dataPathsLine)));
        QVERIFY2(dataPathsLine.contains(QLatin1String("dataRoot=")),
                  "[DataPaths] output missing dataRoot= field");
    }

    void testHeadlessCompletes() {
        // Sanity: the process should produce output and not hang
        QProcess proc;
        proc.setProgram(m_binary);
        proc.setArguments({
            QStringLiteral("--headless"),
            QStringLiteral("--frames"),
            QStringLiteral("3"),
            QStringLiteral("--metric"),
            QStringLiteral("schwarzschild"),
        });
        proc.setProcessChannelMode(QProcess::MergedChannels);
        proc.start();
        const bool started = proc.waitForFinished(120000);
        QVERIFY2(started, "quantumverse_qml --frames 3 did not finish within 120s");
        QCOMPARE(proc.exitCode(), 0);
    }

    void testGeodesicPixelAssertion() {
        const QString screenshotPath = QStringLiteral("smoke_render.png");
        QProcess proc;
        proc.setProgram(m_binary);
        proc.setArguments({
            QStringLiteral("--headless"),
            QStringLiteral("--frames"),
            QStringLiteral("300"),
            QStringLiteral("--enable-geodesics"),
            QStringLiteral("--screenshot"),
            screenshotPath,
        });
        proc.setProcessChannelMode(QProcess::MergedChannels);
        proc.start();
        const bool started = proc.waitForFinished(300000);
        QVERIFY2(started, "quantumverse_qml screenshot did not finish within timeout");
        QCOMPARE(proc.exitCode(), 0);

        const QImage img(screenshotPath);
        QVERIFY2(!img.isNull(), "Screenshot image is null — headless render may not have produced output");
        QVERIFY2(img.width() > 0 && img.height() > 0, "Screenshot has zero size");

        int geodesicPixels = 0;
        for (int y = 0; y < img.height(); ++y) {
            const QRgb* scanline = reinterpret_cast<const QRgb*>(img.scanLine(y));
            for (int x = 0; x < img.width(); ++x) {
                const QRgb p = scanline[x];
                const int r = qRed(p);
                const int g = qGreen(p);
                const int b = qBlue(p);
                if (g > 200 && r < 50 && b < 50) {
                    ++geodesicPixels;
                }
            }
        }
        qWarning() << "Geodesic pixel count:" << geodesicPixels;
        QVERIFY2(geodesicPixels > 300,
                  qPrintable(QStringLiteral("Expected > 300 geodesic pixels, got %1")
                                  .arg(geodesicPixels)));
    }

private:
    QString m_binary;
    QString m_output;
};

QTEST_MAIN(RuntimeSmokeTest)
#include "test_runtime.moc"
