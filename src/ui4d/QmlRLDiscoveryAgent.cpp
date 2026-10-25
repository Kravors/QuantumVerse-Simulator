#include "QmlRLDiscoveryAgent.h"
#include <QVariant>
#include <cmath>
#include <QDateTime>
#include "spacetime/KerrMetric.h"

namespace quantumverse {

QmlRLDiscoveryAgent::QmlRLDiscoveryAgent(QObject* parent)
    : QObject(parent)
{
    // Default parameter ranges for [Mass, Spin, Charge]
    std::vector<std::pair<double, double>> ranges = {
        {0.1, 100.0},   // Mass in solar masses
        {0.0, 1.5},     // Spin
        {0.0, 1.5},     // Charge
    };
    m_agent = std::make_unique<quantumverse::discovery::RLDiscoveryAgent>(3, ranges);
}

bool QmlRLDiscoveryAgent::loadONNXModel(const QString& onnxPath)
{
    if (!m_agent) return false;
    m_loaded = m_agent->loadONNXModel(onnxPath.toStdString());
    if (m_loaded) {
        appendLog("RL Discovery Agent: ONNX model loaded successfully");
    } else {
        appendLog("RL Discovery Agent: FAILED to load ONNX model");
    }
    return m_loaded;
}

bool QmlRLDiscoveryAgent::isONNXLoaded() const
{
    return m_loaded;
}

void QmlRLDiscoveryAgent::setPhysicsValidator(PhysicsValidator* validator)
{
    m_physicsValidator = validator;
}

void QmlRLDiscoveryAgent::setLLMCoPilot(LLMCoPilot* coPilot)
{
    m_llmCoPilot = coPilot;
}

QVariantList QmlRLDiscoveryAgent::runDiscoveryStep(double mass, double spin, double charge)
{
    QVariantList result;
    if (!m_loaded || !m_agent) {
        result.append(0.0);
        result.append(0.0);
        result.append(0.0);
        return result;
    }

    std::array<float, 4> state = {
        static_cast<float>(mass),
        static_cast<float>(spin),
        static_cast<float>(charge),
        0.0f  // anomaly score
    };

    auto action = m_agent->predictONNAction(state);
    double deltaM = static_cast<double>(action[0]);
    double deltaA = static_cast<double>(action[1]);
    double deltaQ = static_cast<double>(action[2]);

    double newMass = mass + deltaM * 5.0;
    double newSpin = spin + deltaA * 2.0;
    double newCharge = charge + deltaQ * 2.0;

    // Clamp
    newMass = std::clamp(newMass, 0.1, 100.0);
    newSpin = std::clamp(newSpin, 0.0, 1.5);
    newCharge = std::clamp(newCharge, 0.0, 1.5);

    // Compute extremality
    double extremality = (newSpin * newSpin + newCharge * newCharge) / (newMass * newMass);

    // --- AI VETO: Validate proposed state with PhysicsValidator ---
    if (m_physicsValidator) {
        // Save the current metric on the validator so we can restore it
        auto originalMetric = std::make_shared<KerrMetric>(mass, spin);

        // Build a test metric from the proposed state
        auto testMetric = std::make_shared<KerrMetric>(newMass, newSpin);
        m_physicsValidator->setMetric(testMetric);

        auto report = m_physicsValidator->validate(Event4D(0.0, 0.0, 0.0, 0.0));
        bool isValid = report.failedChecks == 0;

        // Restore the original metric
        m_physicsValidator->setMetric(originalMetric);

        if (!isValid) {
            QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
            appendLog(QString("[AI VETO] t=%1 Proposed M=%.2f a=%.3f Q=%.3f failed physics validation (%d checks). Action vetoed.")
                .arg(timestamp).arg(newMass).arg(newSpin).arg(newCharge).arg(report.failedChecks));

            // Send to LLM Co-Pilot for plain-English explanation
            if (m_llmCoPilot) {
                QString anomalyData = QString("VETO: Proposed state M=%1, a=%2, Q=%3. "
                    "Violates cosmic censorship (naked singularity). "
                    "PhysicsValidator reported %d failed checks.")
                    .arg(newMass).arg(newSpin).arg(newCharge).arg(report.failedChecks);
                m_llmCoPilot->analyzeAnomaly(anomalyData);
            }

            result.append(0.0);
            result.append(0.0);
            result.append(0.0);
            return result;
        }
    }

    // Log the action
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    QString logMsg = QString("[AI] t=%1 M=%.2f a=%.3f Q=%.3f -> dM=%+.3f da=%+.3f dQ=%+.3f")
        .arg(timestamp).arg(mass).arg(spin).arg(charge)
        .arg(deltaM).arg(deltaA).arg(deltaQ);
    appendLog(logMsg);

    // Check for cosmic censorship violation
    if ((newSpin * newSpin + newCharge * newCharge) > (newMass * newMass)) {
        appendLog("🚨 COSMIC CENSORSHIP VIOLATION: Naked singularity detected! AI backed off.");
    }

    // Check for extremal black hole
    if (extremality > 0.99) {
        appendLog(QString("🚨 DISCOVERY ALERT: Extremal black hole! M=%.2f a=%.3f Q=%.3f Extremality=%.4f")
            .arg(newMass).arg(newSpin).arg(newCharge).arg(extremality));
    }

    result.append(deltaM);
    result.append(deltaA);
    result.append(deltaQ);
    return result;
}

QStringList QmlRLDiscoveryAgent::discoveryLog() const
{
    return m_discoveryLog;
}

void QmlRLDiscoveryAgent::clearLog()
{
    m_discoveryLog.clear();
    emit discoveryLogChanged();
}

void QmlRLDiscoveryAgent::appendLog(const QString& msg)
{
    m_discoveryLog.append(msg);
    if (m_discoveryLog.size() > 100) {
        m_discoveryLog.removeFirst();
    }
    emit discoveryLogChanged();
}

} // namespace quantumverse
