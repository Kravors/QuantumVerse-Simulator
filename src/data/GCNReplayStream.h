/**
 * @file GCNReplayStream.h
 * @brief Reads archived GCN JSON files and replays them in chronological order
 *
 * Scans a directory of archived GCN JSON notices, sorts them by
 * timestamp, and replays them through a configurable-speed timer.
 * Each emitted alert is a QJsonObject fed into the same AlertRouter
 * used for live Kafka ingest.
 */

#ifndef QUANTUMVERSE_GCN_REPLAY_STREAM_H
#define QUANTUMVERSE_GCN_REPLAY_STREAM_H

#include <QObject>
#include <QJsonObject>
#include <QStringList>
#include <QTimer>

namespace quantumverse {

/**
 * @brief Streams archived GCN JSON files in chronological order
 */
class GCNReplayStream : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int totalFiles READ totalFiles NOTIFY totalFilesChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)

public:
    explicit GCNReplayStream(QObject* parent = nullptr);
    ~GCNReplayStream() override = default;

    /** @brief Set the directory containing archived GCN JSON files. */
    Q_INVOKABLE void setDirectory(const QString& dirPath);

    /** @brief Set replay speed multiplier (1.0 = real time, 10.0 = 10x). */
    void setSpeed(double multiplier);

    /** @brief Current speed multiplier. */
    double speed() const { return m_speed; }

    /** @brief Index of the currently playing file. */
    int currentIndex() const { return m_currentIndex; }

    /** @brief Total number of files loaded for replay. */
    int totalFiles() const { return m_files.size(); }

    /** @brief Whether the replay is currently running. */
    bool running() const { return m_timer->isActive(); }

    /** @brief List of loaded file paths (sorted by timestamp). */
    QStringList files() const { return m_files; }

    /** @brief Start replay from the beginning. */
    Q_INVOKABLE void start();

    /** @brief Stop replay and reset to the beginning. */
    Q_INVOKABLE void stop();

    /** @brief Pause replay (keeps current position). */
    Q_INVOKABLE void pause();

    /** @brief Resume a paused replay. */
    Q_INVOKABLE void resume();

signals:
    void speedChanged();
    void currentIndexChanged();
    void totalFilesChanged();
    void runningChanged();
    /** @brief Emitted when the next alert is ready for ingestion. */
    void alertAvailable(const QJsonObject& alert);
    /** @brief Emitted when all archived alerts have been replayed. */
    void replayFinished();

private slots:
    void onTimerTick();

private:
    /** @brief Scan @p dirPath for JSON files and sort by embedded timestamp. */
    void loadAndSortFiles(const QString& dirPath);

    /** @brief Extract timestamp from a GCN JSON file (mtime fallback). */
    static qint64 extractTimestamp(const QString& filePath);

    QStringList m_files;
    int m_currentIndex = 0;
    double m_speed = 1.0;
    QTimer* m_timer = nullptr;
    qint64 m_lastEmitMs = 0;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_GCN_REPLAY_STREAM_H
