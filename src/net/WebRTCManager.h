#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "vr/SharedSession.h"

namespace quantumverse {
namespace net {

struct PeerInfo {
    std::string id;
    std::string name;
    std::string endpoint; // ip:port or ws url
};

using StateUpdateCallback = std::function<void(const SharedState& state)>;
using PeerJoinedCallback = std::function<void(const PeerInfo& peer)>;
using PeerLeftCallback = std::function<void(const std::string& peerId)>;

class WebRTCManager {
public:
    WebRTCManager();
    ~WebRTCManager();

    WebRTCManager(const WebRTCManager&) = delete;
    WebRTCManager& operator=(const WebRTCManager&) = delete;
    WebRTCManager(WebRTCManager&&) = default;
    WebRTCManager& operator=(WebRTCManager&&) = default;

    bool initialize(const std::string& signalingUrl, const std::string& roomId);
    void shutdown();

    void setLocalParticipant(const Participant& participant);
    const Participant& getLocalParticipant() const { return m_localParticipant; }

    void broadcastState(const SharedState& state);
    void sendStateTo(const std::string& peerId, const SharedState& state);

    std::vector<PeerInfo> getConnectedPeers() const;

    void setOnStateUpdate(StateUpdateCallback cb) { m_onStateUpdate = std::move(cb); }
    void setOnPeerJoined(PeerJoinedCallback cb) { m_onPeerJoined = std::move(cb); }
    void setOnPeerLeft(PeerLeftCallback cb) { m_onPeerLeft = std::move(cb); }

    bool isConnected() const { return m_connected; }
    size_t getPeerCount() const { return m_peers.size(); }

private:
    bool m_initialized = false;
    bool m_connected = false;
    std::string m_signalingUrl;
    std::string m_roomId;
    Participant m_localParticipant;
    std::vector<PeerInfo> m_peers;

    StateUpdateCallback m_onStateUpdate;
    PeerJoinedCallback m_onPeerJoined;
    PeerLeftCallback m_onPeerLeft;
};

} // namespace net
} // namespace quantumverse
