/**
 * @file SharedSession.cpp
 * @brief Implementation of SharedSession for multi-user collaboration
 *
 * Manages local shared state and coordinates broadcasting via SignalingClient.
 */

#include "SharedSession.h"
#include "SignalingClient.h"
#include <algorithm>
#include <QDebug>

namespace quantumverse {

SharedSession::SharedSession(QObject* /*parent*/)
{
}

SharedSession::~SharedSession() = default;

void SharedSession::setActive(bool active)
{
    if (m_active == active) return;
    m_active = active;
    emit isActiveChanged();

    if (m_active && m_signalingClient && m_signalingClient->isConnected()) {
        broadcastFullState();
    }
}

void SharedSession::updateActiveTheory(const TheoryParams& theory)
{
    m_state.activeTheory = theory;
    if (m_active && m_signalingClient && m_signalingClient->isConnected()) {
        broadcastFullState();
    }
}

void SharedSession::addDiscoveryResult(const DiscoveryResult& result)
{
    m_state.paretoFront.push_back(result);
    if (m_active && m_signalingClient && m_signalingClient->isConnected()) {
        broadcastFullState();
    }
}

void SharedSession::addLiveAlert(const LiveAlert& alert)
{
    constexpr size_t kMaxAlerts = 50;
    m_state.recentAlerts.push_back(alert);
    if (m_state.recentAlerts.size() > kMaxAlerts) {
        m_state.recentAlerts.erase(m_state.recentAlerts.begin());
    }
    if (m_active && m_signalingClient && m_signalingClient->isConnected()) {
        broadcastFullState();
    }
}

void SharedSession::applyPeerState(const QString& senderId, const QJsonObject& stateJson)
{
    Q_UNUSED(senderId);
    SharedState remote = SharedState::fromJson(stateJson);

    if (!remote.activeTheory.name.empty()) {
        m_state.activeTheory = remote.activeTheory;
    }

    for (const auto& result : remote.paretoFront) {
        bool exists = false;
        for (const auto& local : m_state.paretoFront) {
            if (local.confidence == result.confidence &&
                local.description == result.description) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            m_state.paretoFront.push_back(result);
        }
    }

    for (const auto& alert : remote.recentAlerts) {
        bool exists = false;
        for (const auto& local : m_state.recentAlerts) {
            if (local.origin == alert.origin &&
                qAbs(local.timestamp - alert.timestamp) < 1.0) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            m_state.recentAlerts.push_back(alert);
        }
    }

    constexpr size_t kMaxAlerts = 50;
    if (m_state.recentAlerts.size() > kMaxAlerts) {
        m_state.recentAlerts.erase(
            m_state.recentAlerts.begin(),
            m_state.recentAlerts.begin() + (m_state.recentAlerts.size() - kMaxAlerts));
    }
}

void SharedSession::broadcastFullState()
{
    if (!m_signalingClient || !m_signalingClient->isConnected()) return;

    QJsonObject stateJson = m_state.toJson();
    m_signalingClient->broadcastState(stateJson);
}

} // namespace quantumverse
