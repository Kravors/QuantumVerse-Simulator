// Phase 9.1: Frame Diagnostics JSON Output Test
// Validates FrameDiagnostics produces valid JSON with expected keys.

#include <QTest>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QCoreApplication>

#include "../../src/utils/FrameDiagnostics.h"

using namespace quantumverse::utils;

class DiagnosticsOutputTest : public QObject
{
    Q_OBJECT

private slots:
    void test_empty_diagnostics_writes_valid_json()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        FrameDiagnostics diag;
        diag.setEnabled(true);

        QString filepath = dir.filePath("diagnostics.json");
        bool written = diag.writeJson(filepath.toStdString());
        QVERIFY(written);

        QFile file(filepath);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        QVERIFY(!doc.isNull());
        QVERIFY(doc.isObject());
        QJsonObject root = doc.object();
        QVERIFY(root.contains("frames"));
        QVERIFY(root.contains("total_frames"));
        QVERIFY(root.contains("summary"));
        QVERIFY(root["total_frames"].toInt() == 0);
    }

    void test_recorded_frames_appear_in_json()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        FrameDiagnostics diag;
        diag.setEnabled(true);

        for (int i = 0; i < 5; ++i) {
            FrameSnapshot snap;
            snap.frame_number = i;
            snap.timestamp_s = i * 0.016;
            snap.frame_time_ms = 16.0 + i;
            snap.physics_time_ms = 2.0;
            snap.render_time_ms = 10.0;
            snap.ui_time_ms = 1.0;
            snap.active_geodesics = 3;
            snap.vram_used_mb = 512.0;
            snap.gl_error_count = i % 2;
            diag.recordFrame(snap);
        }

        QString filepath = dir.filePath("diagnostics.json");
        bool written = diag.writeJson(filepath.toStdString());
        QVERIFY(written);

        QFile file(filepath);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        QVERIFY(!doc.isNull());
        QJsonObject root = doc.object();
        QJsonArray frames = root["frames"].toArray();
        QCOMPARE(frames.size(), 5);

        for (int i = 0; i < frames.size(); ++i) {
            QJsonObject frame = frames[i].toObject();
            QVERIFY(frame.contains("frame"));
            QVERIFY(frame.contains("timestamp_s"));
            QVERIFY(frame.contains("frame_time_ms"));
            QVERIFY(frame.contains("physics_time_ms"));
            QVERIFY(frame.contains("render_time_ms"));
            QVERIFY(frame.contains("ui_time_ms"));
            QVERIFY(frame.contains("active_geodesics"));
            QVERIFY(frame.contains("vram_used_mb"));
            QVERIFY(frame.contains("gl_error_count"));
            QCOMPARE(frame["frame"].toInt(), i);
            QVERIFY(frame["frame_time_ms"].toDouble() > 0.0);
        }

        QJsonObject summary = root["summary"].toObject();
        QVERIFY(summary.contains("avg_frame_ms"));
        QVERIFY(summary.contains("avg_fps"));
        QVERIFY(summary.contains("total_frames"));
        QCOMPARE(summary["total_frames"].toInt(), 5);
    }

    void test_disabled_diagnostics_does_not_write()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        FrameDiagnostics diag;
        diag.setEnabled(false);

        FrameSnapshot snap;
        snap.frame_number = 0;
        snap.frame_time_ms = 16.0;
        diag.recordFrame(snap);

        QString filepath = dir.filePath("diagnostics.json");
        bool written = diag.writeJson(filepath.toStdString());
        // Should still write valid JSON, but with 0 frames
        QVERIFY(written);

        QFile file(filepath);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        QJsonObject root = doc.object();
        QCOMPARE(root["total_frames"].toInt(), 0);
    }
};

QTEST_MAIN(DiagnosticsOutputTest)
#include "test_frame_diagnostics.moc"
