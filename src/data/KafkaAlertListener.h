/**
 * @file KafkaAlertListener.h
 * @brief librdkafka-based GCN Kafka consumer
 *
 * Subscribes to one or more GCN Kafka topics and emits alertReceived()
 * for each successfully parsed JSON message.  The consumer runs in a
 * dedicated QThread so the UI event loop is never blocked.
 */

#ifndef QUANTUMVERSE_KAFKA_ALERT_LISTENER_H
#define QUANTUMVERSE_KAFKA_ALERT_LISTENER_H

#include "AlertListener.h"

#include <QObject>
#include <QStringList>
#include <QThread>
#include <QMutex>
#include <QJsonObject>
#include <QByteArray>

#ifdef HAVE_LIBRDKAFKA
#include <rdkafka.h>
#endif

namespace quantumverse {

/**
 * @brief Kafka-backed alert listener for GCN notice streams.
 *
 * Requires librdkafka at link time.  When HAVE_LIBRDKAFKA is not defined
 * the class is declared but start() will emit an error and return.
 */
class KafkaAlertListener : public AlertListener
{
    Q_OBJECT
public:
    explicit KafkaAlertListener(const QString& brokers,
                                const QStringList& topics,
                                QObject* parent = nullptr);
    ~KafkaAlertListener() override;

    void start() override;
    void stop() override;

    /** @brief Current broker list (e.g. "gcn-kafka.nasa.gov:9092"). */
    QString brokers() const { return m_brokers; }

signals:
    // Inherits alertReceived(const QJsonObject&) from AlertListener
    void consumerError(const QString& message);
    void consumerStopped();

private slots:
    void consumeLoop();
    void onRawMessage(const QByteArray& payload);

private:
    Q_DISABLE_COPY(KafkaAlertListener)

    QString m_brokers;
    QThread* m_thread = nullptr;
    QMutex m_mutex;

    bool m_running = false;

#ifdef HAVE_LIBRDKAFKA
    rd_kafka_t* m_consumer = nullptr;
    rd_kafka_conf_t* m_conf = nullptr;
    rd_kafka_topic_partition_list_t* m_topics_list = nullptr;
#endif
};

} // namespace quantumverse

#endif // QUANTUMVERSE_KAFKA_ALERT_LISTENER_H
