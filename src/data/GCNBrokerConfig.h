/**
 * @file GCNBrokerConfig.h
 * @brief Configuration for GCN Kafka broker connection
 *
 * Encapsulates broker URL, topic subscriptions, and authentication
 * parameters for both the public GCN test stream and production broker.
 */

#ifndef QUANTUMVERSE_GCN_BROKER_CONFIG_H
#define QUANTUMVERSE_GCN_BROKER_CONFIG_H

#include <QString>
#include <QStringList>

namespace quantumverse {

/**
 * @brief GCN Kafka broker configuration
 */
struct GCNBrokerConfig {
    QString brokers;
    QStringList topics;
    QString groupId;
    QString securityProtocol;
    QString saslMechanism;
    QString saslUsername;
    QString saslPassword;
    bool autoOffsetResetLatest;

    static GCNBrokerConfig testStream() {
        GCNBrokerConfig cfg;
        cfg.brokers = QStringLiteral("test.gcn.nasa.gov:9092");
        cfg.topics = {
            QStringLiteral("gcn.notices.LVC"),
            QStringLiteral("gcn.notices.ICECUBE"),
            QStringLiteral("gcn.notices.TESS"),
            QStringLiteral("gcn.notices.FERMI_GBM"),
            QStringLiteral("gcn.notices.SWIFT_BAT")
        };
        cfg.groupId = QStringLiteral("quantumverse_test");
        cfg.securityProtocol = QStringLiteral("plaintext");
        cfg.autoOffsetResetLatest = true;
        return cfg;
    }

    static GCNBrokerConfig production() {
        GCNBrokerConfig cfg;
        cfg.brokers = QStringLiteral("gcn-kafka.nasa.gov:9092");
        cfg.topics = {
            QStringLiteral("gcn.notices.LVC"),
            QStringLiteral("gcn.notices.ICECUBE"),
            QStringLiteral("gcn.notices.TESS"),
            QStringLiteral("gcn.notices.FERMI_GBM"),
            QStringLiteral("gcn.notices.SWIFT_BAT")
        };
        cfg.groupId = QStringLiteral("quantumverse_ingest");
        cfg.securityProtocol = QStringLiteral("sasl_ssl");
        cfg.saslMechanism = QStringLiteral("scram-sha-512");
        cfg.autoOffsetResetLatest = true;
        return cfg;
    }
};

} // namespace quantumverse

#endif // QUANTUMVERSE_GCN_BROKER_CONFIG_H
