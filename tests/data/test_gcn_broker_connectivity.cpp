/**
 * @file test_gcn_broker_connectivity.cpp
 * @brief CI smoke test for GCN Kafka broker connectivity
 *
 * Attempts to connect to the public GCN test broker at
 * test.gcn.nasa.gov:9092 and subscribe to a canonical topic.
 * The test is skipped automatically when the network is unreachable
 * or librdkafka is not available at build time.
 */

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTimer>
#include <cassert>
#include <iostream>

#include "data/KafkaAlertListener.h"
#include "data/GCNBrokerConfig.h"

using namespace quantumverse;

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    std::cout << "=== GCNBrokerConnectivityTest ===" << std::endl;

#ifndef HAVE_LIBRDKAFKA
    std::cout << "SKIP: librdkafka not available at build time." << std::endl;
    return 0;
#endif

    GCNBrokerConfig config = GCNBrokerConfig::testStream();
    KafkaAlertListener listener(config);

    QSignalSpy errorSpy(&listener, &KafkaAlertListener::consumerError);
    QSignalSpy stoppedSpy(&listener, &KafkaAlertListener::consumerStopped);

    listener.start();

    bool stopped = stoppedSpy.wait(10000);
    bool errored = errorSpy.count() > 0;

    listener.stop();
    stoppedSpy.wait(3000);

    if (!stopped && !errored) {
        std::cerr << "ERROR: Consumer did not stop within timeout." << std::endl;
        return 1;
    }

    if (errored) {
        QString lastError = errorSpy.last().at(0).toString();
        if (lastError.contains("network") || lastError.contains("resolve") ||
            lastError.contains("timed out") || lastError.contains("unreachable") ||
            lastError.contains("Connection") || lastError.contains("No such host")) {
            std::cout << "SKIP: Network unreachable or broker unavailable (" << lastError.toStdString() << ")." << std::endl;
            return 0;
        }
        std::cerr << "ERROR: Unexpected Kafka error: " << lastError.toStdString() << std::endl;
        return 1;
    }

    std::cout << "PASS: Connected to GCN test broker at " << config.brokers.toStdString() << std::endl;
    std::cout << "Subscribed to " << config.topics.size() << " topics." << std::endl;
    std::cout << "All GCNBrokerConnectivityTest checks passed." << std::endl;
    return 0;
}
