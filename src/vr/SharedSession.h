/**
 * @file SharedSession.h
 * @brief Shared session manager for multi-user collaboration
 *
 * Owns the current SharedState and optionally a SignalingClient.
 * When active, local state changes are broadcast to peers, and
 * incoming peer state updates are merged into the local state.
 */

#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <array>
#include <map>
#include <string>
#include <vector>
#include "spacetime/Event4D.h"
#include "discovery/DiscoveryResultJson.h"

namespace quantumverse {
namespace vr {
class SignalingClient;
}

struct TheoryParams {
    std::string name;
    std::map<std::string, double> parameters;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["name"] = QString::fromStdString(name);
        QJsonObject params;
        for (const auto& [key, val] : parameters) {
            params[QString::fromStdString(key)] = val;
        }
        obj["parameters"] = params;
        return obj;
    }

    static TheoryParams fromJson(const QJsonObject& obj) {
        TheoryParams p;
        p.name = obj["name"].toString().toStdString();
        QJsonObject params = obj["parameters"].toObject();
        for (auto it = params.begin(); it != params.end(); ++it) {
            p.parameters[it.key().toStdString()] = it.value().toDouble();
        }
        return p;
    }
};

struct LiveAlert {
    std::string origin;
    double timestamp;
    std::string summary;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["origin"] = QString::fromStdString(origin);
        obj["timestamp"] = timestamp;
        obj["summary"] = QString::fromStdString(summary);
        return obj;
    }

    static LiveAlert fromJson(const QJsonObject& obj) {
        LiveAlert a;
        a.origin = obj["origin"].toString().toStdString();
        a.timestamp = obj["timestamp"].toDouble();
        a.summary = obj["summary"].toString().toStdString();
        return a;
    }
};

struct Participant {
    std::string id;
    std::string name;
    std::array<double, 16> cameraMatrix;
    std::vector<Event4D> probes;
    bool vrActive = false;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = QString::fromStdString(id);
        obj["name"] = QString::fromStdString(name);
        QJsonArray cam;
        for (double v : cameraMatrix) cam.append(v);
        obj["cameraMatrix"] = cam;
        QJsonArray pr;
        for (const auto& p : probes) {
            QJsonObject pt;
            pt["t"] = p.t; pt["x"] = p.x; pt["y"] = p.y; pt["z"] = p.z;
            pr.append(pt);
        }
        obj["probes"] = pr;
        obj["vrActive"] = vrActive;
        return obj;
    }

    static Participant fromJson(const QJsonObject& obj) {
        Participant p;
        p.id = obj["id"].toString().toStdString();
        p.name = obj["name"].toString().toStdString();
        QJsonArray cam = obj["cameraMatrix"].toArray();
        for (int i = 0; i < 16; ++i) p.cameraMatrix[i] = cam[i].toDouble();
        QJsonArray pr = obj["probes"].toArray();
        for (const auto& val : pr) {
            QJsonObject pt = val.toObject();
            p.probes.emplace_back(pt["t"].toDouble(), pt["x"].toDouble(),
                                  pt["y"].toDouble(), pt["z"].toDouble());
        }
        p.vrActive = obj["vrActive"].toBool();
        return p;
    }
};

struct SharedState {
    std::vector<Participant> participants;
    TheoryParams activeTheory;
    std::vector<DiscoveryResult> paretoFront;
    std::vector<double> bmaWeights;
    std::vector<LiveAlert> recentAlerts;

    QJsonObject toJson() const {
        QJsonObject obj;
        QJsonArray parts;
        for (const auto& p : participants) parts.append(p.toJson());
        obj["participants"] = parts;
        obj["activeTheory"] = activeTheory.toJson();
        QJsonArray front;
        for (const auto& r : paretoFront) front.append(discoveryResultToJson(r));
        obj["paretoFront"] = front;
        QJsonArray weights;
        for (double w : bmaWeights) weights.append(w);
        obj["bmaWeights"] = weights;
        QJsonArray alerts;
        for (const auto& a : recentAlerts) alerts.append(a.toJson());
        obj["recentAlerts"] = alerts;
        return obj;
    }

    static SharedState fromJson(const QJsonObject& obj) {
        SharedState s;
        QJsonArray parts = obj["participants"].toArray();
        for (const auto& val : parts)
            s.participants.push_back(Participant::fromJson(val.toObject()));
        s.activeTheory = TheoryParams::fromJson(obj["activeTheory"].toObject());
        QJsonArray front = obj["paretoFront"].toArray();
        for (const auto& val : front)
            s.paretoFront.push_back(discoveryResultFromJson(val.toObject()));
        QJsonArray weights = obj["bmaWeights"].toArray();
        for (const auto& w : weights) s.bmaWeights.push_back(w.toDouble());
        QJsonArray alerts = obj["recentAlerts"].toArray();
        for (const auto& a : alerts)
            s.recentAlerts.push_back(LiveAlert::fromJson(a.toObject()));
        return s;
    }
};

/**
 * @brief Shared session manager for multi-user collaboration
 *
 * Owns the current SharedState and optionally a SignalingClient.
 * When active, local state changes are broadcast to peers, and
 * incoming peer state updates are merged into the local state.
 */
class SharedSession : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString sessionId READ sessionId WRITE setSessionId NOTIFY sessionIdChanged)
    Q_PROPERTY(bool isActive READ isActive WRITE setActive NOTIFY isActiveChanged)
public:
    /**
     * @brief Construct a new SharedSession
     * @param parent QObject parent (optional, unused for non-QObject base)
     */
    explicit SharedSession(QObject* parent = nullptr);

    /**
     * @brief Destroy the SharedSession
     */
    ~SharedSession();

    /**
     * @brief Set the signaling client used for broadcasting
     * @param client SignalingClient instance (not owned)
     */
    void setSignalingClient(vr::SignalingClient* client) { m_signalingClient = client; }

    /**
     * @brief Get current shared state
     */
    const SharedState& state() const { return m_state; }

    /**
     * @brief Get mutable shared state
     */
    SharedState& state() { return m_state; }

    /**
     * @brief Check if session is active (broadcasting enabled)
     */
    bool isActive() const { return m_active; }

    /**
     * @brief Activate or deactivate session broadcasting
     * @param active true to start broadcasting, false to stop
     */
    void setActive(bool active);

signals:
    void sessionIdChanged();
    void isActiveChanged();

    /**
     * @brief Update the active theory parameters and broadcast if active
     * @param theory New theory parameters
     */
    void updateActiveTheory(const TheoryParams& theory);

    /**
     * @brief Append a discovery result to the Pareto front and broadcast if active
     * @param result New discovery result
     */
    void addDiscoveryResult(const DiscoveryResult& result);

    /**
     * @brief Add a live alert and broadcast if active
     * @param alert New live alert
     */
    void addLiveAlert(const LiveAlert& alert);

    /**
     * @brief Apply a state update received from a peer
     *
     * Merges the remote state into the local state. The merge
     * strategy appends new discovery results and alerts rather than
     * replacing the local lists entirely.
     *
     * @param senderId ID of the peer that sent the update
     * @param stateJson Serialized SharedState from peer
     */
    void applyPeerState(const QString& senderId, const QJsonObject& stateJson);

    /**
     * @brief Set session ID for display purposes
     */
    void setSessionId(const QString& sessionId) { if (m_sessionId != sessionId) { m_sessionId = sessionId; emit sessionIdChanged(); } }

    /**
     * @brief Get current session ID
     */
    QString sessionId() const { return m_sessionId; }

private:
    /**
     * @brief Broadcast the current full state to peers
     */
    void broadcastFullState();

    SharedState m_state;
    vr::SignalingClient* m_signalingClient = nullptr;
    bool m_active = false;
    QString m_sessionId;
    std::vector<std::string> m_knownPeerIds;
};

} // namespace quantumverse
