/**
 * @file test_gcn_replay.cpp
 * @brief Unit tests for GCNReplayStream
 *
 * Validates that archived GCN JSON files are discovered, sorted by
 * timestamp, and emitted at the correct interval when replay is active.
 */

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTimer>
#include <iostream>

#include "data/GCNReplayStream.h"
#include "test_assert.h"

using namespace quantumverse;

static void writeGcnFile(const QString& dir, const QString& name, double timestampSec)
{
    QFile f(QDir(dir).absoluteFilePath(name));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        std::cerr << "Failed to write test file:" << f.fileName().toStdString() << std::endl;
        return;
    }
    QJsonObject obj;
    obj.insert("alert_type", QJsonValue(QStringLiteral("LIGO/Virgo")));
    obj.insert("timestamp", QJsonValue(timestampSec));
    obj.insert("event_id", QJsonValue(QStringLiteral("TEST_EVENT")));
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    f.close();
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    std::cout << "=== GCNReplayStream Test ===" << std::endl;

    QTemporaryDir tmpDir;
    QV_CHECK(tmpDir.isValid());

    writeGcnFile(tmpDir.path(), "C.json", 100.0);
    writeGcnFile(tmpDir.path(), "A.json", 50.0);
    writeGcnFile(tmpDir.path(), "B.json", 75.0);

    GCNReplayStream stream;
    stream.setDirectory(tmpDir.path());

    QV_CHECK(stream.totalFiles() == 3);
    QV_CHECK(stream.files().size() == 3);

    const QStringList ordered = stream.files();
    QV_CHECK(ordered[0].endsWith("A.json"));
    QV_CHECK(ordered[1].endsWith("B.json"));
    QV_CHECK(ordered[2].endsWith("C.json"));

    QSignalSpy spy(&stream, &GCNReplayStream::alertAvailable);
    QSignalSpy finishedSpy(&stream, &GCNReplayStream::replayFinished);
    stream.setSpeed(10.0);
    stream.start();

    bool finished = finishedSpy.wait(10000);
    QV_CHECK(finished);
    (void)finished;

    QV_CHECK(spy.count() == 3);

    const QJsonObject first = spy.value(0).at(0).toJsonObject();
    QV_CHECK(first.value("event_id").toString() == "TEST_EVENT");

    QV_CHECK(stream.currentIndex() == 3);

    std::cout << "All GCNReplayStream tests passed." << std::endl;
    return 0;
}
