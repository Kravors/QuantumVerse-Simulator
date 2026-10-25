/**
 * @file LLMCoPilot.h
 * @brief Local LLM bridge (Ollama) that explains spacetime anomalies in plain English.
 *
 * Sends Discovery Console anomaly/veto data to a locally running Ollama model
 * (e.g. llama3) and emits the physicist-grade explanation back to QML.
 */

#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QJsonObject>

namespace quantumverse {

/**
 * @brief Bridges the C++ simulator to a local Ollama LLM for anomaly explanation.
 *
 * When the RL agent hits a PhysicsValidator veto or the Discovery Engine logs
 * an anomaly, LLMCoPilot::analyzeAnomaly() POSTs the raw data to
 * http://localhost:11434/api/generate and emits analysisReady() with the
 * model's plain-English explanation.
 */
class LLMCoPilot : public QObject {
    Q_OBJECT

public:
    explicit LLMCoPilot(QObject* parent = nullptr);

    /// Send raw anomaly/veto data to the LLM for explanation.
    Q_INVOKABLE void analyzeAnomaly(const QString& anomalyData);

    /// Send discovery scan data to the LLM for a paper abstract.
    Q_INVOKABLE void generatePaperAbstract(const QJsonObject& scanData);

signals:
    /// Emitted when the LLM returns an explanation string.
    void analysisReady(const QString& explanation);

    /// Emitted when the LLM returns a paper abstract.
    void abstractReady(const QString& abstractText);

    /// Emitted when the abstract request fails.
    void abstractFailed(const QString& error);

private:
    QNetworkAccessManager* m_network = nullptr;

    // Tracks which signal the in-flight request should emit on completion.
    void (LLMCoPilot::*m_pendingSuccess)(const QString&) = nullptr;
    void (LLMCoPilot::*m_pendingFailure)(const QString&) = nullptr;
};

} // namespace quantumverse
