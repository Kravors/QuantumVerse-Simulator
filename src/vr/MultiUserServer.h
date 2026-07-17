/**
 * @file MultiUserServer.h
 * @brief Multi-user VR collaboration server
 *
 * Provides authoritative state management for multi-user VR sessions,
 * enabling collaborative exploration of 4D spacetime.
 */

#ifndef QUANTUMVERSE_MULTI_USER_SERVER_H
#define QUANTUMVERSE_MULTI_USER_SERVER_H

#include "../vr/VRCommon.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace quantumverse {
namespace vr {

/**
 * @brief Participant in a multi-user VR session
 */
struct Participant {
    std::string id;
    std::string name;
    HeadPose headPose;
    ControllerState leftController;
    ControllerState rightController;
    bool isPresent = false;
    double lastUpdateTime = 0.0;
};

/**
 * @brief Message types for multi-user communication
 */
enum class MessageType {
    HeadPoseUpdate,
    ControllerUpdate,
    ChatMessage,
    SpacetimeAnnotation,
    TeleportRequest,
    SessionControl
};

/**
 * @brief Base message structure
 */
struct Message {
    MessageType type;
    std::string senderId;
    double timestamp = 0.0;
};

/**
 * @brief Multi-user VR session server
 *
 * Manages authoritative state for a multi-user VR session,
 * synchronizing head poses, controller states, and annotations
 * across all connected participants.
 */
class MultiUserServer {
public:
    using ParticipantCallback = std::function<void(const Participant& participant)>;
    using MessageCallback = std::function<void(const Message& message)>;

    MultiUserServer();
    ~MultiUserServer();

    // Non-copyable
    MultiUserServer(const MultiUserServer&) = delete;
    MultiUserServer& operator=(const MultiUserServer&) = delete;
    MultiUserServer(MultiUserServer&&) = default;
    MultiUserServer& operator=(MultiUserServer&&) = default;

    /**
     * @brief Start the server on a specified port
     * @param port Port number to listen on
     * @return true if server started successfully
     */
    bool start(uint16_t port = 7777);

    /**
     * @brief Stop the server
     */
    void stop();

    /**
     * @brief Check if the server is running
     */
    bool isRunning() const { return m_isRunning; }

    /**
     * @brief Update server state (call regularly)
     */
    void update();

    /**
     * @brief Get list of connected participants
     */
    std::vector<Participant> getParticipants() const;

    /**
     * @brief Send a message to all participants
     */
    void broadcastMessage(const Message& message);

    /**
     * @brief Set callback for participant join/leave events
     */
    void setParticipantCallback(ParticipantCallback callback) {
        m_participantCallback = std::move(callback);
    }

    /**
     * @brief Set callback for incoming messages
     */
    void setMessageCallback(MessageCallback callback) {
        m_messageCallback = std::move(callback);
    }

    /**
     * @brief Get current participant count
     */
    size_t getParticipantCount() const { return m_participants.size(); }

private:
    bool m_isRunning = false;
    std::vector<Participant> m_participants;
    ParticipantCallback m_participantCallback;
    MessageCallback m_messageCallback;
    uint16_t m_port = 7777;
    double m_lastUpdateTime = 0.0;
};

} // namespace vr
} // namespace quantumverse

#endif // QUANTUMVERSE_MULTI_USER_SERVER_H
