#include "net/WebRTCManager.h"

namespace quantumverse {
namespace net {

WebRTCManager::WebRTCManager() = default;

WebRTCManager::~WebRTCManager() = default;

bool WebRTCManager::initialize(const std::string& signalingUrl, const std::string& roomId) {
    m_signalingUrl = signalingUrl;
    m_roomId = roomId;
    m_initialized = true;
    m_connected = false;
    return true;
}

void WebRTCManager::shutdown() {
    m_connected = false;
    m_peers.clear();
    m_initialized = false;
}

void WebRTCManager::setLocalParticipant(const Participant& participant) {
    m_localParticipant = participant;
}

void WebRTCManager::broadcastState(const SharedState& state) {
    (void)state;
}

void WebRTCManager::sendStateTo(const std::string& peerId, const SharedState& state) {
    (void)peerId;
    (void)state;
}

std::vector<PeerInfo> WebRTCManager::getConnectedPeers() const {
    return m_peers;
}

} // namespace net
} // namespace quantumverse
