/**
 * @file MultiUserServer.cpp
 * @brief Stub implementation of multi-user VR collaboration server
 *
 * This implementation provides a no-op stub that compiles without
 * external networking dependencies. When networking support is needed,
 * this can be replaced with a real implementation using WebRTC or
 * similar technology.
 */

#include "MultiUserServer.h"
#include <algorithm>

namespace quantumverse {
namespace vr {

MultiUserServer::MultiUserServer() = default;

MultiUserServer::~MultiUserServer()
{
    stop();
}

bool MultiUserServer::start(uint16_t port)
{
    if (m_isRunning) return true;

    m_port = port;
    m_isRunning = true;
    m_lastUpdateTime = 0.0;

    return true;
}

void MultiUserServer::stop()
{
    if (!m_isRunning) return;

    m_isRunning = false;
    m_participants.clear();
}

void MultiUserServer::update()
{
    if (!m_isRunning) return;

    // Stub: no-op update
    m_lastUpdateTime = 0.0;
}

std::vector<Participant> MultiUserServer::getParticipants() const
{
    return m_participants;
}

void MultiUserServer::broadcastMessage(const Message& message)
{
    if (!m_isRunning) return;

    // Stub: no-op broadcast
    (void)message;
}

} // namespace vr
} // namespace quantumverse
