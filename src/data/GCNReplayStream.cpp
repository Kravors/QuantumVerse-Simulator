/**
 * @file GCNReplayStream.cpp
 * @brief Implementation of the GCN archive replay stream
 */

#include "GCNReplayStream.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>
#include <algorithm>

namespace quantumverse {

GCNReplayStream::GCNReplayStream(QObject* parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    m_timer->setSingleShot(false);
    connect(m_timer, &QTimer::timeout, this, &GCNReplayStream::onTimerTick);
}

void GCNReplayStream::setDirectory(const QString& dirPath)
{
    loadAndSortFiles(dirPath);
    emit totalFilesChanged();
}

void GCNReplayStream::setSpeed(double multiplier)
{
    if (multiplier <= 0.0) multiplier = 1.0;
    if (qAbs(m_speed - multiplier) < 1e-9) return;
    m_speed = multiplier;
    emit speedChanged();
}

void GCNReplayStream::start()
{
    if (m_files.isEmpty()) {
        qWarning() << "GCNReplayStream: no files loaded; call setDirectory() first.";
        return;
    }
    m_currentIndex = 0;
    m_lastEmitMs = QDateTime::currentMSecsSinceEpoch();
    m_timer->start(0);
    emit runningChanged();
    emit currentIndexChanged();
}

void GCNReplayStream::stop()
{
    m_timer->stop();
    m_currentIndex = 0;
    emit runningChanged();
    emit currentIndexChanged();
}

void GCNReplayStream::pause()
{
    if (m_timer->isActive()) {
        m_timer->stop();
        emit runningChanged();
    }
}

void GCNReplayStream::resume()
{
    if (!m_timer->isActive() && m_currentIndex < m_files.size()) {
        m_lastEmitMs = QDateTime::currentMSecsSinceEpoch();
        m_timer->start(0);
        emit runningChanged();
    }
}

void GCNReplayStream::onTimerTick()
{
    if (m_currentIndex >= m_files.size()) {
        m_timer->stop();
        emit replayFinished();
        emit runningChanged();
        return;
    }

    const QString path = m_files[m_currentIndex];
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "GCNReplayStream: failed to open" << path;
        ++m_currentIndex;
        m_timer->start(0);
        emit currentIndexChanged();
        return;
    }

    QJsonParseError err;
    const QByteArray raw = f.readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    f.close();

    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "GCNReplayStream: invalid JSON in" << path << err.errorString();
        ++m_currentIndex;
        m_timer->start(0);
        emit currentIndexChanged();
        return;
    }

    emit alertAvailable(doc.object());

    ++m_currentIndex;
    emit currentIndexChanged();

    if (m_currentIndex >= m_files.size()) {
        m_timer->stop();
        emit replayFinished();
        emit runningChanged();
        return;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 nextTs = extractTimestamp(m_files[m_currentIndex]);
    const qint64 prevTs = extractTimestamp(m_files[m_currentIndex - 1]);
    qint64 deltaMs = nextTs - prevTs;
    if (deltaMs < 0) deltaMs = 0;
    if (deltaMs > 60000) deltaMs = 60000;

    const int interval = qMax(1, static_cast<int>(deltaMs / m_speed));
    m_lastEmitMs = now;
    m_timer->start(interval);
}

void GCNReplayStream::loadAndSortFiles(const QString& dirPath)
{
    m_files.clear();
    m_currentIndex = 0;

    QDir dir(dirPath);
    if (!dir.exists()) {
        qWarning() << "GCNReplayStream: directory does not exist:" << dirPath;
        return;
    }

    const QStringList entries = dir.entryList(QStringList() << "*.json", QDir::Files, QDir::Time);
    if (entries.isEmpty()) {
        qWarning() << "GCNReplayStream: no JSON files found in" << dirPath;
        return;
    }

    struct Entry {
        QString path;
        qint64 ts;
    };
    QList<Entry> sorted;
    sorted.reserve(entries.size());
    for (const QString& name : entries) {
        const QString full = dir.absoluteFilePath(name);
        sorted.append({ full, extractTimestamp(full) });
    }
    std::sort(sorted.begin(), sorted.end(), [](const Entry& a, const Entry& b) {
        return a.ts < b.ts;
    });

    for (const Entry& e : sorted) {
        m_files.append(e.path);
    }
}

qint64 GCNReplayStream::extractTimestamp(const QString& filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QFileInfo(filePath).lastModified().toMSecsSinceEpoch();
    }
    QJsonParseError err;
    const QByteArray raw = f.readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    f.close();

    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return QFileInfo(filePath).lastModified().toMSecsSinceEpoch();
    }

    const QJsonObject obj = doc.object();
    const QStringList preferredKeys = {
        "timestamp", "time", "datetime", "detection_time",
        "alert_time", "gcnd_time", "created_at"
    };
    for (const QString& key : preferredKeys) {
        if (obj.contains(key)) {
            const QJsonValue v = obj.value(key);
            if (v.isDouble()) return static_cast<qint64>(v.toDouble() * 1000.0);
            if (v.isString()) return QDateTime::fromString(v.toString(), Qt::ISODate).toMSecsSinceEpoch();
        }
    }

    return QFileInfo(filePath).lastModified().toMSecsSinceEpoch();
}

} // namespace quantumverse
