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

signals:
    /// Emitted when the LLM returns an explanation string.
    void analysisReady(const QString& explanation);

private:
    QNetworkAccessManager* m_network = nullptr;
};

} // namespace quantumverse
