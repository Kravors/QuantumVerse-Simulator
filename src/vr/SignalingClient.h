/**
 * @file SignalingClient.h
 * @brief WebSocket-based signaling client for multi-user collaboration
 *
 * Connects to a signaling server to exchange session metadata and
 * broadcast SharedState updates between collaborators.
 */

#pragma once

#include <QObject>
#include <QWebSocket>
#include <QAbstractSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include <QUuid>
#include <string>
#include <vector>

namespace quantumverse {
namespace vr {

/**
 * @brief Signaling client for multi-user collaboration
 *
 * Manages a WebSocket connection to a central signaling server.
 * The server acts as a message broker, forwarding state updates
 * between all clients in the same session.
 */
class SignalingClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY isConnectedChanged)
    Q_PROPERTY(size_t peerCount READ peerCount NOTIFY peerCountChanged)
public:
    /**
     * @brief Construct a new SignalingClient
     * @param parent QObject parent
     */
    explicit SignalingClient(QObject* parent = nullptr);

    /**
     * @brief Destroy the SignalingClient and close connection
     */
    ~SignalingClient();

    /**
     * @brief Connect to the signaling server
     * @param url WebSocket URL (e.g., ws://localhost:8080)
     * @param sessionId Session identifier for grouping collaborators
     * @param clientId Unique client identifier (generated if empty)
     * @param clientName Human-readable client name
     * @return true if connection initiated successfully
     */
    bool connectToServer(const QString& url,
                         const QString& sessionId,
                         const QString& clientId = QString(),
                         const QString& clientName = QString());

    /**
     * @brief Disconnect from the signaling server
     */
    void disconnectFromServer();

    /**
     * @brief Check if connected to the server
     */
    bool isConnected() const { return m_connected; }

    /**
     * @brief Get current session ID
     */
    QString sessionId() const { return m_sessionId; }

    /**
     * @brief Get current client ID
     */
    QString clientId() const { return m_clientId; }

    /**
     * @brief Get list of connected peer client IDs
     */
    std::vector<std::string> peerIds() const { return m_peerIds; }

    /**
     * @brief Get number of connected peers
     */
    size_t peerCount() const { return m_peerIds.size(); }

    /**
     * @brief Broadcast SharedState to all peers in the session
     *
     * Serializes state to JSON and sends it to the signaling server,
     * which forwards it to all other clients in the same session.
     *
     * @param stateJson Serialized SharedState as QJsonObject
     */
    void broadcastState(const QJsonObject& stateJson);

    /**
     * @brief Set human-readable client name
     */
    void setClientName(const QString& name) { m_clientName = name; }

signals:
    /**
     * @brief Emitted when connection to server is established
     */
    void connected();

    /**
     * @brief Emitted when connection to server is lost
     * @param reason Human-readable disconnect reason
     */
    void disconnected(const QString& reason);

    /**
     * @brief Emitted when a new peer joins the session
     * @param clientId Peer's unique identifier
     * @param clientName Peer's human-readable name
     */
    void peerJoined(const QString& clientId, const QString& clientName);

    /**
     * @brief Emitted when a peer leaves the session
     * @param clientId Peer's unique identifier
     */
    void peerLeft(const QString& clientId);

    /**
     * @brief Emitted when a state update is received from a peer
     * @param senderId ID of the client that sent the update
     * @param stateJson Serialized SharedState
     */
    void stateReceived(const QString& senderId, const QJsonObject& stateJson);

    /**
     * @brief Emitted on connection error
     * @param errorMessage Description of the error
     */
    void connectionError(const QString& errorMessage);

    /**
     * @brief Emitted when connection state changes
     */
    void isConnectedChanged();

    /**
     * @brief Emitted when peer count changes
     */
    void peerCountChanged();

private slots:
    /**
     * @brief Handle WebSocket connection established
     */
    void onConnected();

    /**
     * @brief Handle WebSocket disconnection
     */
    void onDisconnected();

    /**
     * @brief Handle incoming WebSocket text message
     * @param message Raw JSON message from server
     */
    void onTextMessageReceived(const QString& message);

    /**
     * @brief Handle WebSocket error
     * @param error Socket error type
     */
    void onSocketError(QAbstractSocket::SocketError error);

public:
    /**
     * @brief Generate a unique client ID
     * @return UUID-like string
     */
    static QString generateClientId() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }

private:
    /**
     * @brief Send a JSON message to the server
     * @param obj JSON object to send
     */
    void sendJson(const QJsonObject& obj);

    /**
     * @brief Handle a parsed JSON message from the server
     * @param obj Parsed message
     */
    void handleServerMessage(const QJsonObject& obj);

    QWebSocket* m_socket = nullptr;
    QTimer* m_reconnectTimer = nullptr;
    QString m_url;
    QString m_sessionId;
    QString m_clientId;
    QString m_clientName;
    bool m_connected = false;
    std::vector<std::string> m_peerIds;
    int m_reconnectAttempts = 0;
    static constexpr int kMaxReconnectAttempts = 10;
    static constexpr int kReconnectDelayMs = 2000;
};

} // namespace vr
} // namespace quantumverse
