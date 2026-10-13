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
#include <QVector3D>
#include <cmath>
#include <QtTest/QtTest>

namespace {

// Run one headless annulus capture at the Doppler verification pose
// (el=30, dist=150, config black_hole.position (0,0,0), single frame) and
// load the resulting screenshot.
QImage captureAnnulus(const QString& binary, const QString& mode, int azimuth,
                      const QString& path, QString& error, double beamingGain = 1.0)
{
    QProcess proc;
    proc.setProgram(binary);
    QStringList args = {
        QStringLiteral("--frames"), QStringLiteral("1"),
        QStringLiteral("--cam-az"), QString::number(azimuth),
        QStringLiteral("--cam-el"), QStringLiteral("30"),
        QStringLiteral("--cam-dist"), QStringLiteral("150"),
        QStringLiteral("--annulus-debug"), mode,
        QStringLiteral("--beaming-gain"), QString::number(beamingGain, 'f', 2),
        QStringLiteral("--screenshot"), path,
    };
    proc.setArguments(args);
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start();
    if (!proc.waitForFinished(120000)) {
        error = QStringLiteral("annulus capture (mode=%1 az=%2) timed out: %3")
                    .arg(mode).arg(azimuth).arg(proc.errorString());
        return QImage();
    }
    if (proc.exitCode() != 0) {
        error = QStringLiteral("annulus capture (mode=%1 az=%2) exited with code %3")
                    .arg(mode).arg(azimuth).arg(proc.exitCode());
        return QImage();
    }
    const QImage img(path);
    if (img.isNull()) {
        error = QStringLiteral("annulus capture (mode=%1 az=%2) produced a null image")
                    .arg(mode).arg(azimuth);
    }
    return img;
}

// POSITION encoding: RGB = (rel + 2) / 4, with rel relative to bhCenterWorld.
QVector3D decodeAnnulusPosition(QRgb p)
{
    return QVector3D(qRed(p) / 255.0f * 4.0f - 2.0f,
                     qGreen(p) / 255.0f * 4.0f - 2.0f,
                     qBlue(p) / 255.0f * 4.0f - 2.0f);
}

// Locate the annulus fragment nearest `target` (disk-relative world units)
// inside a small window around the image center. The window is load-bearing:
// scene pixels outside the annulus (lensing ring, volumetric disk glow) can
// decode into the annulus radial band and would otherwise win the nearest
// search. Measured at 724x432: every band-passing pixel inside the central
// 60x60 box is a genuine annulus fragment, and the annulus itself bboxes to
// (356,213)-(367,218) there.
bool findNearestAnnulusPixel(const QImage& positionCapture, const QVector3D& target,
                             QPoint& outPixel, QVector3D& outWorld)
{
    const int cx = positionCapture.width() / 2;
    const int cy = positionCapture.height() / 2;
    const int half = 30;
    bool found = false;
    float bestDist = 0.0f;
    for (int y = qMax(0, cy - half); y < qMin(positionCapture.height(), cy + half); ++y) {
        const QRgb* scanline = reinterpret_cast<const QRgb*>(positionCapture.scanLine(y));
        for (int x = qMax(0, cx - half); x < qMin(positionCapture.width(), cx + half); ++x) {
            const QVector3D rel = decodeAnnulusPosition(scanline[x]);
            const float radius = std::sqrt(rel.x() * rel.x() + rel.z() * rel.z());
            if (std::abs(rel.y()) > 0.15f || radius < 0.60f || radius > 1.95f) {
                continue;
            }
            const float d = (rel - target).length();
            if (!found || d < bestDist) {
                found = true;
                bestDist = d;
                outPixel = QPoint(x, y);
                outWorld = rel;
            }
        }
    }
    return found;
}

QRgb pixelAt(const QImage& img, const QPoint& p)
{
    return reinterpret_cast<const QRgb*>(img.scanLine(p.y()))[p.x()];
}

} // namespace

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
        // The shipped default is NORMAL (the shaded disk with Doppler
        // beaming), so this debug path is requested explicitly. Relying on a
        // default here would silently measure the shipped disk instead of the
        // magenta overlay, and the count would stay 0 through any future
        // default change.
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
            QStringLiteral("--annulus-debug"),
            QStringLiteral("magenta"),
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

    void testAnnulusDopplerSign() {
        // Regression surface for the DOPPLER annulus debug mode
        // (debugAnnulusDoppler in celestialFragSrc). The mode encodes the sign
        // of los = dot(v_dir, view_dir): red-dominant fragments sit on the
        // disk's approaching limb, blue-dominant on the receding limb.
        //
        // The decision point is the flip of the SAME world point when the
        // camera crosses to the opposite side. A fixed screen pixel cannot
        // show it: the mirror between az=90 and az=270 moves the world point
        // to the opposite screen side but keeps its colour, so the test tracks
        // world (0, 0, +1.294) through the POSITION encoding in both
        // azimuths, then samples the DOPPLER captures at the located pixels -
        // four spawns of the same el=30 dist=150 pose pair.
        //
        // Measured 2026-10-13 at 724x432, config (0,0,0): the fragment
        // nearest (0,0,+1.294) lands at (357,215) in the az=90 position
        // capture and (366,215) in the az=270 capture, both decoding
        // z = +1.294 (r = 1.326). The DOPPLER captures read (215,0,0) and
        // (0,0,215) there, i.e. |los| ~ 0.84 against the geometric prediction
        // 0.844 - the sign check the beaming-ratio measurement depends on.
        QString error;
        const QImage pos90 = captureAnnulus(m_binary, QStringLiteral("position"), 90,
                                            QStringLiteral("smoke_annulus_pos90.png"), error);
        QVERIFY2(!pos90.isNull(), qPrintable(error));
        const QImage pos270 = captureAnnulus(m_binary, QStringLiteral("position"), 270,
                                             QStringLiteral("smoke_annulus_pos270.png"), error);
        QVERIFY2(!pos270.isNull(), qPrintable(error));
        const QImage dop90 = captureAnnulus(m_binary, QStringLiteral("doppler"), 90,
                                            QStringLiteral("smoke_annulus_dop90.png"), error);
        QVERIFY2(!dop90.isNull(), qPrintable(error));
        const QImage dop270 = captureAnnulus(m_binary, QStringLiteral("doppler"), 270,
                                             QStringLiteral("smoke_annulus_dop270.png"), error);
        QVERIFY2(!dop270.isNull(), qPrintable(error));

        const QVector3D target(0.0f, 0.0f, 1.294f);
        QPoint px90, px270;
        QVector3D w90, w270;
        QVERIFY2(findNearestAnnulusPixel(pos90, target, px90, w90),
                 "No annulus fragment found near (0,0,+1.294) in the az=90 position capture");
        QVERIFY2(findNearestAnnulusPixel(pos270, target, px270, w270),
                 "No annulus fragment found near (0,0,+1.294) in the az=270 position capture");

        // Both picks must be fragments of the same (+Z) side of the disk,
        // otherwise the flip below would compare two different world points.
        QVERIFY2(w90.z() > 0.5f, qPrintable(QStringLiteral(
                     "az=90 pick decoded to z=%1 - expected the +Z side").arg(w90.z())));
        QVERIFY2(w270.z() > 0.5f, qPrintable(QStringLiteral(
                     "az=270 pick decoded to z=%1 - expected the +Z side").arg(w270.z())));
        QVERIFY2((w90 - w270).length() < 0.7f, qPrintable(QStringLiteral(
                     "az=90 and az=270 picks are different world points: "
                     "(%1,%2,%3) vs (%4,%5,%6)")
                     .arg(w90.x()).arg(w90.y()).arg(w90.z())
                     .arg(w270.x()).arg(w270.y()).arg(w270.z())));

        const QRgb s90 = pixelAt(dop90, px90);
        const QRgb s270 = pixelAt(dop270, px270);
        qWarning() << "Doppler az=90 px" << px90 << "RGB" << qRed(s90) << qGreen(s90) << qBlue(s90)
                   << "| az=270 px" << px270 << "RGB" << qRed(s270) << qGreen(s270) << qBlue(s270);

        // Decision point: the same world point reads approaching (red) with
        // the eye on +X and receding (blue) with the eye on -X.
        QVERIFY2(qRed(s90) > qBlue(s90) + 100, qPrintable(QStringLiteral(
                     "az=90 sample %1 is not red-dominant: RGB(%2,%3,%4) - "
                     "los sign did not point at the +X camera")
                     .arg(QStringLiteral("(%1,%2)").arg(px90.x()).arg(px90.y()))
                     .arg(qRed(s90)).arg(qGreen(s90)).arg(qBlue(s90))));
        QVERIFY2(qBlue(s270) > qRed(s270) + 100, qPrintable(QStringLiteral(
                     "az=270 sample %1 is not blue-dominant: RGB(%2,%3,%4) - "
                     "the los sign did not flip with the camera")
                     .arg(QStringLiteral("(%1,%2)").arg(px270.x()).arg(px270.y()))
                     .arg(qRed(s270)).arg(qGreen(s270)).arg(qBlue(s270))));
        // The encoding writes exactly one of (los,0,0) / (0,0,-los) with
        // alpha 1.0; green staying 0 and alpha staying 255 keep the decode
        // unambiguous against skybox blending.
        QVERIFY2(qGreen(s90) < 60 && qGreen(s270) < 60,
                 "Doppler samples carry green - the encoding is not (r,0,b,1)");
        QVERIFY2(qAlpha(s90) == 255 && qAlpha(s270) == 255,
                 "Doppler samples are semi-transparent - they blended with the skybox");

        // Presence guard: both limbs exist in each capture. Counts use the
        // shader's own strict signature and stay loose on purpose - warm
        // scene glow nearby also passes it, so the thresholds sit far below
        // the measured values (red 75/49, blue 12/12).
        const int cx = dop90.width() / 2;
        const int cy = dop90.height() / 2;
        for (int pass = 0; pass < 2; ++pass) {
            const QImage& img = (pass == 0) ? dop90 : dop270;
            const int az = (pass == 0) ? 90 : 270;
            int red = 0, blue = 0;
            for (int y = qMax(0, cy - 30); y < qMin(img.height(), cy + 30); ++y) {
                const QRgb* scanline = reinterpret_cast<const QRgb*>(img.scanLine(y));
                for (int x = qMax(0, cx - 30); x < qMin(img.width(), cx + 30); ++x) {
                    const QRgb p = scanline[x];
                    if (qRed(p) > 150 && qGreen(p) < 60 && qBlue(p) < 60) {
                        ++red;
                    } else if (qBlue(p) > 150 && qRed(p) < 60 && qGreen(p) < 60) {
                        ++blue;
                    }
                }
            }
            qWarning() << "Doppler limb pixel counts az=" << az << "red:" << red
                       << "blue:" << blue;
            QVERIFY2(red >= 20, qPrintable(QStringLiteral(
                         "az=%1 capture has %2 red-dominant pixels, expected >= 20 - "
                         "the approaching limb is missing").arg(az).arg(red)));
            QVERIFY2(blue >= 5, qPrintable(QStringLiteral(
                         "az=%1 capture has %2 blue-dominant pixels, expected >= 5 - "
                         "the receding limb is missing").arg(az).arg(blue)));
        }
    }

    void testAnnulusDopplerBeamingRatio() {
        // Regression surface for the Doppler beaming term on the accretion
        // disk (dopplerBeaming in celestialFragSrc, reached via NORMAL debug
        // mode). The term multiplies the disk's display-encoded colour by
        // D^4 with D = 1 / (gamma * (1 - beta * los)), beta =
        // sqrt(M_VISUAL / r), M_VISUAL = 0.12 - the bolometric
        // Stefan-Boltzmann convention (T_obs = T * delta), the same one the
        // volumetric disk uses.
        //
        // Prediction at the identified sample points (r = 1.326,
        // los = -+0.844, beta = 0.301): D_app = 1.341, D_rec = 0.797,
        // ratio_base = 1.681, and ratio_base^4 = 7.99. D^3 would read 4.75
        // and an un-beamed disk 1.0, so one clean measurement identifies the
        // exponent.
        //
        // Measured 2026-10-13 at el=30 dist=150 config (0,0,0) 724x432 with
        // --beaming-gain 0.3: approaching (357,216) = (199,205,223), receding
        // (366,216) = (25,25,28); per-channel ratios 7.96 / 8.20 / 7.96,
        // pixel-identical across repeat captures. At the shipped gain of 1.0 the
        // approaching sample reads (255,255,255) - clipped - which is exactly
        // the condition the gain override exists to avoid.
        QString error;
        const QImage pos = captureAnnulus(m_binary, QStringLiteral("position"), 90,
                                          QStringLiteral("smoke_annulus_beam_pos.png"), error);
        QVERIFY2(!pos.isNull(), qPrintable(error));
        // Measurement capture at gain 0.3: the shipped default of 1.0 evaluates
        // base * D^4 to ~660-744 and clips the approaching limb to pure white,
        // which is why the CLI override exists and why the position capture
        // above runs at the default (its encode is unaffected by the gain).
        const QImage beam = captureAnnulus(m_binary, QStringLiteral("normal"), 90,
                                           QStringLiteral("smoke_annulus_beam.png"), error,
                                           0.3);
        QVERIFY2(!beam.isNull(), qPrintable(error));

        const QVector3D near(0.0f, 0.0f, 1.294f);
        const QVector3D far(0.0f, 0.0f, -1.294f);
        QPoint pxNear, pxFar;
        QVector3D wNear, wFar;
        QVERIFY2(findNearestAnnulusPixel(pos, near, pxNear, wNear),
                 "No annulus fragment found near (0,0,+1.294) in the position capture");
        QVERIFY2(findNearestAnnulusPixel(pos, far, pxFar, wFar),
                 "No annulus fragment found near (0,0,-1.294) in the position capture");

        const QRgb sNear = pixelAt(beam, pxNear);
        const QRgb sFar = pixelAt(beam, pxFar);
        qWarning() << "Beaming az=90 near px" << pxNear << "RGB" << qRed(sNear)
                   << qGreen(sNear) << qBlue(sNear) << "| far px" << pxFar << "RGB"
                   << qRed(sFar) << qGreen(sFar) << qBlue(sFar);

        // Sampling channel: red. Blue approaches the clip boundary closest
        // (223 at gain 0.3), so it has the least headroom for a slight
        // miscalibration; all three channels are still reported so a future
        // failure names which one clipped.
        const int nearR = qRed(sNear);
        const int farR = qRed(sFar);

        // Both bounds on the approaching sample: below 255 means not clipped
        // (a clipped sample flattens the ratio to ~2-3x), above 50 means the
        // sample is not degenerate (a dark or mislocated sample would make
        // any ratio meaningless).
        QVERIFY2(nearR < 255, qPrintable(QStringLiteral(
                     "Approaching limb sample is clipped (R=%1) - the display gain no "
                     "longer keeps base * D^4 inside [0,1] and the ratio is not "
                     "measurable").arg(nearR)));
        QVERIFY2(nearR > 50, qPrintable(QStringLiteral(
                     "Approaching limb sample is degenerate (R=%1) - outside the "
                     "beamed disk region").arg(nearR)));
        QVERIFY2(farR > 5, qPrintable(QStringLiteral(
                     "Receding limb sample is black (R=%1) - the beaming term "
                     "attenuated it below the measurable range").arg(farR)));

        // The D^4 band. The ratio is gain-agnostic by construction - a ratio
        // within one channel cancels the display gain - so this assertion holds
        // at any gain. Measured 7.96 / 8.20 / 7.96, prediction 7.99; D^3 would
        // read 4.75 and clipping collapses the ratio to ~2-3x, so [6.0, 10.5]
        // identifies the exponent uniquely.
        //
        // Channel spread note: the shader multiplies all three channels by
        // the same D^4, so the ratios should agree up to 8-bit rounding, and
        // rounding alone at these magnitudes (approaching ~199, receding ~25)
        // is at most ~+-0.2. G reads 8.20, 2.6% above R and B - a sub-pixel
        // blend straddling two fragments with slightly different D values.
        // Do not tighten the band toward 1% around 7.99; the spread is
        // structural, not a measurement error.
        const int nearCh[3] = {qRed(sNear), qGreen(sNear), qBlue(sNear)};
        const int farCh[3] = {qRed(sFar), qGreen(sFar), qBlue(sFar)};
        for (int c = 0; c < 3; ++c) {
            const double ratio = static_cast<double>(nearCh[c]) / farCh[c];
            qWarning() << "Beaming ratio channel" << c << ":" << ratio;
            QVERIFY2(ratio >= 6.0 && ratio <= 10.5, qPrintable(QStringLiteral(
                         "Channel %1 limb ratio %2 outside the D^4 band [6.0, 10.5] "
                         "(near=%3 far=%4) - the beaming term is not the bolometric "
                         "D^4 convention").arg(c).arg(ratio).arg(nearCh[c]).arg(farCh[c])));
        }
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
