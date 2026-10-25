#ifndef QML_RL_DISCOVERY_AGENT_H
#define QML_RL_DISCOVERY_AGENT_H

#include <QObject>
#include <QVariant>
#include <QVariantList>
#include <QStringList>
#include <array>
#include <memory>
#include <string>

#include "discovery/RLDiscoveryAgent.h"

#include "physics/PhysicsValidator.h"
#include "discovery/LLMCoPilot.h"

namespace quantumverse {

/**
 * @brief QObject wrapper around RLDiscoveryAgent for QML integration.
 *
 * Exposes the ONNX Runtime PPO actor policy to QML so the "Run Discovery Scan"
 * UI loop can autonomously adjust black hole parameters via ONNX inference.
 */
class QmlRLDiscoveryAgent : public QObject {
    Q_OBJECT

    Q_PROPERTY(QStringList discoveryLog READ discoveryLog NOTIFY discoveryLogChanged)

public:
    explicit QmlRLDiscoveryAgent(QObject* parent = nullptr);

    /// Load an ONNX model into the underlying RL agent.
    Q_INVOKABLE bool loadONNXModel(const QString& onnxPath);

    /// Whether an ONNX model is currently loaded.
    Q_INVOKABLE bool isONNXLoaded() const;

    /**
     * @brief Run one RL discovery step.
     *
     * Takes the current black hole state [mass, spin, charge] and returns
     * the PPO actor's suggested action deltas [deltaMass, deltaSpin, deltaCharge].
     */
    Q_INVOKABLE QVariantList runDiscoveryStep(double mass, double spin, double charge);

    /// Returns the discovery log as a QStringList for QML ListView binding.
    Q_INVOKABLE QStringList discoveryLog() const;

    /// Inject the PhysicsValidator so proposed AI actions can be vetoed.
    void setPhysicsValidator(PhysicsValidator* validator);

    /// Inject the LLMCoPilot so vetoed actions get a plain-English explanation.
    void setLLMCoPilot(LLMCoPilot* coPilot);

    /// Clear the discovery log.
    Q_INVOKABLE void clearLog();

signals:
    /// Emitted when a new log entry is appended.
    void discoveryLogChanged();

private:
    std::unique_ptr<quantumverse::discovery::RLDiscoveryAgent> m_agent;
    bool m_loaded = false;
    QStringList m_discoveryLog;
    PhysicsValidator* m_physicsValidator = nullptr;
    LLMCoPilot* m_llmCoPilot = nullptr;

    void appendLog(const QString& msg);
};

} // namespace quantumverse

#endif // QML_RL_DISCOVERY_AGENT_H
