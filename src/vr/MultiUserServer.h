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
#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QHostAddress>
#include <QTimer>
#include <string>
#include <vector>
#include <memory>
#include <unordered_set>
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
class MultiUserServer : public QObject {
    Q_OBJECT
public:
    explicit MultiUserServer(QObject* parent = nullptr);
    ~MultiUserServer() override;

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
    void update() { cleanupStaleParticipants(); }

    /**
     * @brief Get list of connected participants
     */
    std::vector<Participant> getParticipants() const;

    /**
     * @brief Send a message to all participants
     */
    void broadcastMessage(const Message& message);

signals:
    /**
     * @brief Emitted when a client connects
     */
    void clientConnected(const std::string& clientId);

    /**
     * @brief Emitted when a client disconnects
     */
    void clientDisconnected(const std::string& clientId);

    /**
     * @brief Emitted when a client joins a session
     */
    void clientJoinedSession(const std::string& clientId, const std::string& sessionId);

    /**
     * @brief Emitted when a client leaves a session
     */
    void clientLeftSession(const std::string& clientId, const std::string& sessionId);

private slots:
    /**
     * @brief Handle new WebSocket connection
     */
    void onNewConnection();

    /**
     * @brief Handle client disconnection
     */
    void onClientDisconnected();

    /**
     * @brief Handle incoming text message
     */
    void onTextMessageReceived(const QString& message);

private:
    struct ClientEntry {
        QWebSocket* socket = nullptr;
        std::string id;
        std::string name;
        std::string sessionId;
        bool isPresent = false;
        qint64 lastUpdateTime = 0;

        Participant toParticipant() const {
            Participant p;
            p.id = id;
            p.name = name;
            p.isPresent = isPresent;
            p.lastUpdateTime = static_cast<double>(lastUpdateTime);
            return p;
        }
    };

    /**
     * @brief Handle join message from client
     */
    void handleJoin(const QString& clientId, const QJsonObject& obj);

    /**
     * @brief Handle leave message from client
     */
    void handleLeave(const QString& clientId, const QJsonObject& obj);

    /**
     * @brief Handle state update from client
     */
    void handleStateUpdate(const QString& clientId, const QJsonObject& obj);

    /**
     * @brief Handle head pose update from client
     */
    void handleHeadPoseUpdate(const QString& clientId, const QJsonObject& obj);

    /**
     * @brief Handle controller update from client
     */
    void handleControllerUpdate(const QString& clientId, const QJsonObject& obj);

    /**
     * @brief Broadcast message to all clients in a session except sender
     */
    void broadcastToSession(const std::string& sessionId,
                           const QJsonObject& message,
                           const std::string& excludeClientId);

    /**
     * @brief Remove stale participants that haven't updated recently
     */
    void cleanupStaleParticipants();

    /**
     * @brief Find client by ID
     */
    std::vector<ClientEntry>::iterator findClient(const std::string& clientId);

    /**
     * @brief Serialize message to JSON
     */
    QJsonObject serializeMessage(const Message& message) const;

    bool m_isRunning = false;
    uint16_t m_port = 7777;
    QWebSocketServer* m_server = nullptr;
    QTimer* m_cleanupTimer = nullptr;

    std::vector<ClientEntry> m_clients;
    std::unordered_map<std::string, std::unordered_set<std::string>> m_sessions;
};

} // namespace vr
} // namespace quantumverse

#endif // QUANTUMVERSE_MULTI_USER_SERVER_H
