/**
 * @file KafkaAlertListener.cpp
 * @brief librdkafka-based GCN Kafka consumer implementation
 */

#include "KafkaAlertListener.h"
#include "GCNBrokerConfig.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>
#include <QThread>
#include <QMutex>

namespace quantumverse {

#ifdef HAVE_LIBRDKAFKA

static QString kafkaErrorString(rd_kafka_resp_err_t err)
{
    return QString::fromLocal8Bit(rd_kafka_err2str(err));
}

void KafkaAlertListener::consumeLoop()
{
    rd_kafka_t* consumer = m_consumer;
    rd_kafka_topic_partition_list_t* topics = m_topics_list;

    if (rd_kafka_subscribe(consumer, topics) != RD_KAFKA_RESP_ERR_NO_ERROR) {
        emit consumerError(QStringLiteral("Failed to subscribe to Kafka topics"));
        emit consumerStopped();
        return;
    }

    while (m_running) {
        rd_kafka_message_t* msg = rd_kafka_consumer_poll(consumer, 500);
        if (!msg) continue;

        if (msg->err == RD_KAFKA_RESP_ERR_NO_ERROR) {
            const char* payload = static_cast<const char*>(msg->payload);
            const int len = static_cast<int>(msg->len);
            if (payload && len > 0) {
                QByteArray ba(payload, len);
                onRawMessage(ba);
            }
        } else if (msg->err == RD_KAFKA_RESP_ERR__PARTITION_EOF) {
            // End of partition - continue polling
        } else if (msg->err == RD_KAFKA_RESP_ERR__TIMED_OUT) {
            // Poll timeout - continue
        } else {
            qWarning() << "Kafka consume error:" << rd_kafka_message_errstr(msg);
        }

        rd_kafka_message_destroy(msg);
    }

    rd_kafka_consumer_close(consumer);
    emit consumerStopped();
}

void KafkaAlertListener::onRawMessage(const QByteArray& payload)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "GCN JSON parse error:" << err.errorString();
        return;
    }

    emit alertReceived(doc.object());
}

#else // HAVE_LIBRDKAFKA not defined

void KafkaAlertListener::consumeLoop()
{
    emit consumerError(QStringLiteral("librdkafka not available at build time"));
    emit consumerStopped();
}

void KafkaAlertListener::onRawMessage(const QByteArray& payload)
{
    Q_UNUSED(payload)
}

#endif

KafkaAlertListener::KafkaAlertListener(const GCNBrokerConfig& config,
                                       QObject* parent)
    : AlertListener(parent)
    , m_brokers(config.brokers)
    , m_thread(new QThread(this))
    , m_groupId(config.groupId)
    , m_securityProtocol(config.securityProtocol)
    , m_saslMechanism(config.saslMechanism)
    , m_saslUsername(config.saslUsername)
    , m_saslPassword(config.saslPassword)
    , m_autoOffsetResetLatest(config.autoOffsetResetLatest)
{
    m_topics = config.topics;
}

KafkaAlertListener::~KafkaAlertListener()
{
    stop();
    if (m_thread) {
        m_thread->quit();
        m_thread->wait(5000);
    }

#ifdef HAVE_LIBRDKAFKA
    if (m_topics_list) {
        rd_kafka_topic_partition_list_destroy(m_topics_list);
        m_topics_list = nullptr;
    }
    if (m_consumer) {
        rd_kafka_destroy(m_consumer);
        m_consumer = nullptr;
    }
    if (m_conf) {
        rd_kafka_conf_destroy(m_conf);
        m_conf = nullptr;
    }
#endif
}

void KafkaAlertListener::start()
{
    QMutexLocker locker(&m_mutex);
    if (m_running) return;

#ifdef HAVE_LIBRDKAFKA
    char errstr[512];

    m_conf = rd_kafka_conf_new();
    if (!m_conf) {
        emit consumerError(QStringLiteral("Failed to create librdkafka configuration"));
        return;
    }

    rd_kafka_conf_set(m_conf, "bootstrap.servers", m_brokers.toUtf8().constData(), errstr, sizeof(errstr));
    rd_kafka_conf_set(m_conf, "group.id", m_groupId.toUtf8().constData(), errstr, sizeof(errstr));
    rd_kafka_conf_set(m_conf, "auto.offset.reset", m_autoOffsetResetLatest ? "latest" : "earliest", errstr, sizeof(errstr));

    if (m_securityProtocol == QLatin1String("sasl_ssl")) {
        rd_kafka_conf_set(m_conf, "security.protocol", "sasl_ssl", errstr, sizeof(errstr));
        rd_kafka_conf_set(m_conf, "sasl.mechanism", m_saslMechanism.toUtf8().constData(), errstr, sizeof(errstr));
        if (!m_saslUsername.isEmpty()) {
            rd_kafka_conf_set(m_conf, "sasl.username", m_saslUsername.toUtf8().constData(), errstr, sizeof(errstr));
        }
        if (!m_saslPassword.isEmpty()) {
            rd_kafka_conf_set(m_conf, "sasl.password", m_saslPassword.toUtf8().constData(), errstr, sizeof(errstr));
        }
    } else {
        rd_kafka_conf_set(m_conf, "security.protocol", "plaintext", errstr, sizeof(errstr));
    }

    m_consumer = rd_kafka_new(RD_KAFKA_CONSUMER, m_conf, errstr, sizeof(errstr));
    if (!m_consumer) {
        emit consumerError(QStringLiteral("Failed to create Kafka consumer: %1").arg(QString::fromLocal8Bit(errstr)));
        rd_kafka_conf_destroy(m_conf);
        m_conf = nullptr;
        return;
    }

    m_topics_list = rd_kafka_topic_partition_list_new();
    for (const QString& topic : m_topics) {
        rd_kafka_topic_partition_list_add(m_topics_list, topic.toUtf8().constData(), RD_KAFKA_PARTITION_UA);
    }

    m_running = true;

    this->moveToThread(m_thread);
    connect(m_thread, &QThread::started, this, &KafkaAlertListener::consumeLoop);
    connect(this, &KafkaAlertListener::consumerStopped, m_thread, &QThread::quit);
    connect(m_thread, &QThread::finished, this, &QObject::deleteLater);
    m_thread->start();
#else
    Q_UNUSED(m_brokers)
    Q_UNUSED(m_topics)
    emit consumerError(QStringLiteral("librdkafka not available - Kafka listener disabled at build time"));
#endif
}

void KafkaAlertListener::stop()
{
    QMutexLocker locker(&m_mutex);
    if (!m_running) return;

    m_running = false;

#ifdef HAVE_LIBRDKAFKA
    if (m_consumer) {
        rd_kafka_consumer_close(m_consumer);
    }
#endif

    if (m_thread && m_thread->isRunning()) {
        m_thread->quit();
        if (!m_thread->wait(5000)) {
            qWarning() << "Kafka consumer thread did not stop cleanly";
            m_thread->terminate();
            m_thread->wait(2000);
        }
    }
}

} // namespace quantumverse

#include "KafkaAlertListener.moc"
