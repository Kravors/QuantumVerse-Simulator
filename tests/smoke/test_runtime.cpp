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

        // 4b. probeValid=1 in every dump (probe must be sampling)
        for (const auto& line : realityLines) {
            QVERIFY2(line.contains(QLatin1String("probeValid= 1")),
                      qPrintable(QStringLiteral("probeValid != 1 in: %1").arg(line)));
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
                if (g > 200 && r < 50 && b > 30 && b < 80) {
                    ++geodesicPixels;
                }
            }
        }
        qWarning() << "Geodesic pixel count:" << geodesicPixels;
        QVERIFY2(geodesicPixels > 300,
                  qPrintable(QStringLiteral("Expected > 300 geodesic pixels, got %1")
                                  .arg(geodesicPixels)));

        const int cx = img.width() / 2;
        const int cy = img.height() / 2;
        int sunWarmPixels = 0;
        for (int y = qMax(0, cy - 50); y < qMin(img.height(), cy + 50); ++y) {
            const QRgb* scanline = reinterpret_cast<const QRgb*>(img.scanLine(y));
            for (int x = qMax(0, cx - 50); x < qMin(img.width(), cx + 50); ++x) {
                const QRgb p = scanline[x];
                const int r = qRed(p);
                const int b = qBlue(p);
                if (r > 180 && r > b + 30) {
                    ++sunWarmPixels;
                }
            }
        }
        qWarning() << "Sun warm pixel count:" << sunWarmPixels;
        QVERIFY2(sunWarmPixels > 50,
                  qPrintable(QStringLiteral("Expected > 50 warm Sun pixels, got %1")
                                  .arg(sunWarmPixels)));
    }

    void testAnnulusMagentaPixelAssertion() {
        // Regression surface for the black hole annulus debug path (debugAnnulus
        // in celestialFragSrc). Two properties are worth asserting, and they are
        // different instruments:
        //
        //  (a) PRESENCE: magenta pixels appear at all. The annulus is drawn with
        //      glDrawElementsInstancedBaseInstance and a flat magenta fragment,
        //      so a count of 0 means the mesh, the instance offset or the
        //      uniform wiring is broken.
        //  (b) SCALE: the count stays in the tens. Measured 52 px at az=0 and
        //      az=180. Moving the disk 25 world units (config black_hole.position
        //      = [25,0,0]) changed it to 51 px - one pixel, inside the 20 px
        //      same-azimuth noise floor. So this count is deliberately NOT a
        //      geometry check; it is a scale guard. A count in the hundreds means
        //      something changed the annulus's world scale, which is what the
        //      upper bound catches.
        //
        // The camera flags are part of what is under test: they must be parsed
        // and must survive the QML startup-overwrite gates (see main.qml
        // cliCameraSet), otherwise this capture is taken from whatever pose QML
        // chose and the count is not reproducible.
        const QString screenshotPath = QStringLiteral("smoke_annulus.png");
        QProcess proc;
        proc.setProgram(m_binary);
        proc.setArguments({
            QStringLiteral("--frames"),
            QStringLiteral("1"),
            QStringLiteral("--cam-az"),
            QStringLiteral("0"),
            QStringLiteral("--cam-el"),
            QStringLiteral("30"),
            QStringLiteral("--cam-dist"),
            QStringLiteral("150"),
            QStringLiteral("--screenshot"),
            screenshotPath,
        });
        proc.setProcessChannelMode(QProcess::MergedChannels);
        proc.start();
        const bool started = proc.waitForFinished(120000);
        QVERIFY2(started, "quantumverse_qml annulus screenshot did not finish within timeout");
        QCOMPARE(proc.exitCode(), 0);

        const QImage img(screenshotPath);
        QVERIFY2(!img.isNull(), "Screenshot image is null — headless render may not have produced output");
        QVERIFY2(img.width() > 0 && img.height() > 0, "Screenshot has zero size");

        int magentaPixels = 0;
        for (int y = 0; y < img.height(); ++y) {
            const QRgb* scanline = reinterpret_cast<const QRgb*>(img.scanLine(y));
            for (int x = 0; x < img.width(); ++x) {
                const QRgb p = scanline[x];
                if (qRed(p) > 200 && qGreen(p) < 60 && qBlue(p) > 200) {
                    ++magentaPixels;
                }
            }
        }
        qWarning() << "Annulus magenta pixel count:" << magentaPixels;
        QVERIFY2(magentaPixels >= 20,
                 qPrintable(QStringLiteral("Expected >= 20 annulus magenta pixels, got %1")
                                .arg(magentaPixels)));
        // Upper bound is the scale guard: measured 52, noise floor 20, so 120
        // is ~6x the measured value and ~2x the largest plausible noise drift.
        QVERIFY2(magentaPixels <= 120,
                 qPrintable(QStringLiteral("Annulus magenta pixel count %1 exceeds the scale "
                                           "guard of 120 - the annulus is no longer at the "
                                           "design scale (0.72-1.80 world units)")
                                .arg(magentaPixels)));
    }

    void testToolbarOverflowAssertion() {
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
        const bool started = proc.waitForFinished(300000);
        QVERIFY2(started, "quantumverse_qml toolbar check did not finish within timeout");
        QCOMPARE(proc.exitCode(), 0);

        const QByteArray rawOutput = proc.readAll();
        const QString output = QString::fromLocal8Bit(rawOutput.constData(), rawOutput.size());
        const QStringList lines = output.split('\n', Qt::SkipEmptyParts);

        QStringList realityLines;
        for (const auto& line : lines) {
            if (line.contains(QLatin1String("[REALITY]"))) {
                realityLines.append(line);
            }
        }

        qWarning() << "Reality dump count for toolbar check:" << realityLines.size();
        QVERIFY2(!realityLines.isEmpty(), "No [REALITY] dumps found for toolbar check");

        bool overflowClear = false;
        for (const auto& line : realityLines) {
            if (line.contains(QLatin1String("toolbar_overflow= 0"))) {
                overflowClear = true;
                break;
            }
        }
        qWarning() << "Toolbar overflow clear:" << overflowClear;
        QVERIFY2(overflowClear,
                  qPrintable(QStringLiteral("toolbar_overflow remained 1 in all [REALITY] dumps; layout still overflows")));
    }

private:
    QString m_binary;
    QString m_output;
};

QTEST_MAIN(RuntimeSmokeTest)
#include "test_runtime.moc"
