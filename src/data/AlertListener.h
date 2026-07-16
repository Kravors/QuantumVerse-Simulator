/**
 * @file AlertListener.h
 * @brief Abstract base class for external alert stream listeners
 *
 * Provides a common interface for starting/stopping a listener and
 * receiving parsed alerts as Qt signals. Concrete subclasses connect
 * to a specific transport (Kafka, HTTP webhook, etc.) and emit
 * alertReceived() for each incoming message.
 */

#ifndef QUANTUMVERSE_ALERT_LISTENER_H
#define QUANTUMVERSE_ALERT_LISTENER_H

#include <QObject>
#include <QJsonObject>
#include <QStringList>

namespace quantumverse {

/**
 * @brief Abstract base for alert stream listeners
 */
class AlertListener : public QObject
{
    Q_OBJECT
public:
    explicit AlertListener(QObject* parent = nullptr);
    virtual ~AlertListener() = default;

    /** @brief Begin consuming alerts. */
    virtual void start() = 0;

    /** @brief Stop consuming alerts and release resources. */
    virtual void stop() = 0;

    /** @brief Configure the alert topics/channels to subscribe to. */
    void setTopics(const QStringList& topics) { m_topics = topics; }
    QStringList topics() const { return m_topics; }

signals:
    /**
     * @brief Emitted when a new alert has been parsed from the stream.
     * @param alert Parsed alert payload as a JSON object.
     */
    void alertReceived(const QJsonObject& alert);

protected:
    QStringList m_topics;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_ALERT_LISTENER_H
