/**
 * @file MultiUserServer.cpp
 * @brief Multi-user VR collaboration server implementation
 *
 * Provides real WebSocket-based server for multi-user VR sessions,
 * enabling collaborative exploration of 4D spacetime.
 */

#include "MultiUserServer.h"
#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QUuid>
#include <QDebug>
#include <QAbstractSocket>

namespace quantumverse {
namespace vr {

MultiUserServer::MultiUserServer(QObject* parent)
    : QObject(parent)
    , m_server(new QWebSocketServer("QuantumVerse VR Server", QWebSocketServer::NonSecureMode, this))
    , m_cleanupTimer(new QTimer(this))
{
    connect(m_server, &QWebSocketServer::newConnection,
            this, &MultiUserServer::onNewConnection);
    connect(m_server, &QWebSocketServer::serverError,
            this, [this](QWebSocketProtocol::CloseCode code) {
                qWarning() << "WebSocket server error:" << code;
            });
    connect(m_cleanupTimer, &QTimer::timeout, this, &MultiUserServer::cleanupStaleParticipants);
    m_cleanupTimer->setInterval(5000);  // Check for stale participants every 5s
    m_cleanupTimer->setSingleShot(false);
}

MultiUserServer::~MultiUserServer()
{
    stop();
}

bool MultiUserServer::start(uint16_t port)
{
    if (m_isRunning) return true;

    if (!m_server->listen(QHostAddress::Any, port)) {
        qWarning() << "Failed to start WebSocket server on port" << port
                   << ":" << m_server->errorString();
        return false;
    }

    m_port = port;
    m_isRunning = true;
    m_cleanupTimer->start();

    qInfo() << "MultiUserServer started on port" << port;
    return true;
}

void MultiUserServer::stop()
{
    if (!m_isRunning) return;

    m_cleanupTimer->stop();

    // Notify all clients and close connections
    for (auto& client : m_clients) {
        if (client.socket && client.socket->state() == QAbstractSocket::ConnectedState) {
            QJsonObject msg;
            msg["type"] = "server_shutdown";
            msg["reason"] = "Server is shutting down";
            client.socket->sendTextMessage(QJsonDocument(msg).toJson(QJsonDocument::Compact));
            client.socket->disconnect();
        }
    }

    m_clients.clear();
    m_sessions.clear();

    m_server->close();
    m_isRunning = false;

    qInfo() << "MultiUserServer stopped";
}

std::vector<Participant> MultiUserServer::getParticipants() const
{
    std::vector<Participant> result;
    result.reserve(m_clients.size());

    for (const auto& client : m_clients) {
        if (client.isPresent) {
            result.push_back(client.toParticipant());
        }
    }

    return result;
}

void MultiUserServer::broadcastMessage(const Message& message)
{
    QJsonObject msg = serializeMessage(message);
    QByteArray data = QJsonDocument(msg).toJson(QJsonDocument::Compact);

    for (auto& client : m_clients) {
        if (client.socket && client.socket->state() == QAbstractSocket::ConnectedState) {
            client.socket->sendTextMessage(data);
        }
    }
}

void MultiUserServer::onNewConnection()
{
    QWebSocket* socket = m_server->nextPendingConnection();
    if (!socket) return;

    connect(socket, &QWebSocket::textMessageReceived,
            this, &MultiUserServer::onTextMessageReceived);
    connect(socket, &QWebSocket::disconnected,
            this, &MultiUserServer::onClientDisconnected);
    connect(socket, &QWebSocket::errorOccurred,
            this, [this, socket](QAbstractSocket::SocketError error) {
                Q_UNUSED(error);
                qWarning() << "WebSocket error for client"
                           << socket->property("clientId").toString()
                           << ":" << socket->errorString();
            });

    // Create client entry
    ClientEntry client;
    client.socket = socket;
    client.id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    client.name = "User-" + client.id.substr(0, std::min<size_t>(8, client.id.size()));
    client.isPresent = false;
    client.lastUpdateTime = QDateTime::currentSecsSinceEpoch();
    client.sessionId.clear();

    socket->setProperty("clientId", QString::fromStdString(client.id));

    m_clients.push_back(client);

    // Send welcome message
    QJsonObject welcome;
    welcome["type"] = "welcome";
    welcome["clientId"] = QString::fromStdString(client.id);
    welcome["serverTime"] = QDateTime::currentMSecsSinceEpoch();
    socket->sendTextMessage(QJsonDocument(welcome).toJson(QJsonDocument::Compact));

    qInfo() << "New client connected:" << client.id;
}

void MultiUserServer::onClientDisconnected()
{
    QWebSocket* socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) return;

    QString clientId = socket->property("clientId").toString();
    std::string clientIdStr = clientId.toStdString();

    // Find and remove client
    auto it = std::find_if(m_clients.begin(), m_clients.end(),
                           [&clientIdStr](const ClientEntry& c) {
                               return c.id == clientIdStr;
                           });

    if (it != m_clients.end()) {
        std::string sessionId = it->sessionId;

        // Notify session peers
        if (!sessionId.empty()) {
            QJsonObject leaveMsg;
            leaveMsg["type"] = "peer_left";
            leaveMsg["clientId"] = QString::fromStdString(it->id);
            leaveMsg["clientName"] = QString::fromStdString(it->name);
            broadcastToSession(sessionId, leaveMsg, it->id);

            // Remove from session
            m_sessions[sessionId].erase(it->id);
            if (m_sessions[sessionId].empty()) {
                m_sessions.erase(sessionId);
            }
        }

        qInfo() << "Client disconnected:" << it->id << "from session" << sessionId;
        m_clients.erase(it);
    }

    socket->deleteLater();
}

void MultiUserServer::onTextMessageReceived(const QString& message)
{
    QWebSocket* socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) return;

    QString clientId = socket->property("clientId").toString();

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON received from client" << clientId;
        return;
    }

    QJsonObject obj = doc.object();
    QString type = obj.value("type").toString();

    if (type == "join") {
        handleJoin(clientId, obj);
    } else if (type == "leave") {
        handleLeave(clientId, obj);
    } else if (type == "state_update") {
        handleStateUpdate(clientId, obj);
    } else if (type == "head_pose_update") {
        handleHeadPoseUpdate(clientId, obj);
    } else if (type == "controller_update") {
        handleControllerUpdate(clientId, obj);
    } else {
        qWarning() << "Unknown message type:" << type << "from" << clientId;
    }
}

void MultiUserServer::handleJoin(const QString& clientId, const QJsonObject& obj)
{
    auto clientIt = findClient(clientId.toStdString());
    if (clientIt == m_clients.end()) return;

    QString sessionId = obj.value("sessionId").toString();
    QString clientName = obj.value("clientName").toString();

    // Leave previous session if any
    if (!clientIt->sessionId.empty() && clientIt->sessionId != sessionId.toStdString()) {
        handleLeave(clientId, QJsonObject{{"sessionId", QString::fromStdString(clientIt->sessionId)}});
    }

    // Join new session
    clientIt->sessionId = sessionId.toStdString();
    clientIt->name = clientName.isEmpty() ? clientIt->name : clientName.toStdString();
    clientIt->isPresent = true;
    clientIt->lastUpdateTime = QDateTime::currentSecsSinceEpoch();

    m_sessions[sessionId.toStdString()].insert(clientIt->id);

    // Send join confirmation
    QJsonObject confirm;
    confirm["type"] = "join_confirmed";
    confirm["sessionId"] = sessionId;
    confirm["clientId"] = clientId;
    confirm["clientName"] = QString::fromStdString(clientIt->name);
    clientIt->socket->sendTextMessage(QJsonDocument(confirm).toJson(QJsonDocument::Compact));

    // Notify existing peers
    QJsonObject peerMsg;
    peerMsg["type"] = "peer_joined";
    peerMsg["clientId"] = clientId;
    peerMsg["clientName"] = QString::fromStdString(clientIt->name);
    broadcastToSession(clientIt->sessionId, peerMsg, clientIt->id);

    // Send current peer list to new client
    QJsonObject peers;
    peers["type"] = "peers";
    peers["sessionId"] = sessionId;
    QJsonArray peerList;
    for (const auto& peerId : m_sessions[clientIt->sessionId]) {
        if (peerId != clientIt->id) {
            auto peerIt = findClient(peerId);
            if (peerIt != m_clients.end()) {
                QJsonObject peerInfo;
                peerInfo["clientId"] = QString::fromStdString(peerIt->id);
                peerInfo["clientName"] = QString::fromStdString(peerIt->name);
                peerList.append(peerInfo);
            }
        }
    }
    peers["peers"] = peerList;
    clientIt->socket->sendTextMessage(QJsonDocument(peers).toJson(QJsonDocument::Compact));

    qInfo() << "Client" << clientIt->id << "joined session" << sessionId
            << "as" << clientIt->name;
}

void MultiUserServer::handleLeave(const QString& clientId, const QJsonObject& obj)
{
    auto clientIt = findClient(clientId.toStdString());
    if (clientIt == m_clients.end()) return;

    QString sessionId = obj.value("sessionId").toString();
    if (sessionId.isEmpty()) {
        sessionId = QString::fromStdString(clientIt->sessionId);
    }

    if (clientIt->sessionId.empty()) return;

    // Notify session peers
    QJsonObject leaveMsg;
    leaveMsg["type"] = "peer_left";
    leaveMsg["clientId"] = clientId;
    leaveMsg["clientName"] = QString::fromStdString(clientIt->name);
    broadcastToSession(clientIt->sessionId, leaveMsg, clientIt->id);

    // Remove from session
    m_sessions[clientIt->sessionId].erase(clientIt->id);
    if (m_sessions[clientIt->sessionId].empty()) {
        m_sessions.erase(clientIt->sessionId);
    }

    clientIt->sessionId.clear();
    clientIt->isPresent = false;

    qInfo() << "Client" << clientIt->id << "left session" << sessionId;
}

void MultiUserServer::handleStateUpdate(const QString& clientId, const QJsonObject& obj)
{
    auto clientIt = findClient(clientId.toStdString());
    if (clientIt == m_clients.end() || clientIt->sessionId.empty()) return;

    clientIt->lastUpdateTime = QDateTime::currentSecsSinceEpoch();

    // Broadcast state to all other peers in session
    QJsonObject forward = obj;
    forward["senderId"] = clientId;
    forward["senderName"] = QString::fromStdString(clientIt->name);
    forward["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    broadcastToSession(clientIt->sessionId, forward, clientIt->id);
}

void MultiUserServer::handleHeadPoseUpdate(const QString& clientId, const QJsonObject& obj)
{
    auto clientIt = findClient(clientId.toStdString());
    if (clientIt == m_clients.end() || clientIt->sessionId.empty()) return;

    clientIt->lastUpdateTime = QDateTime::currentSecsSinceEpoch();

    // Broadcast head pose to all other peers in session
    QJsonObject forward;
    forward["type"] = "head_pose_update";
    forward["senderId"] = clientId;
    forward["senderName"] = QString::fromStdString(clientIt->name);
    forward["position"] = obj.value("position");
    forward["orientation"] = obj.value("orientation");
    forward["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    broadcastToSession(clientIt->sessionId, forward, clientIt->id);
}

void MultiUserServer::handleControllerUpdate(const QString& clientId, const QJsonObject& obj)
{
    auto clientIt = findClient(clientId.toStdString());
    if (clientIt == m_clients.end() || clientIt->sessionId.empty()) return;

    clientIt->lastUpdateTime = QDateTime::currentSecsSinceEpoch();

    // Broadcast controller state to all other peers in session
    QJsonObject forward;
    forward["type"] = "controller_update";
    forward["senderId"] = clientId;
    forward["senderName"] = QString::fromStdString(clientIt->name);
    forward["leftController"] = obj.value("leftController");
    forward["rightController"] = obj.value("rightController");
    forward["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    broadcastToSession(clientIt->sessionId, forward, clientIt->id);
}

void MultiUserServer::broadcastToSession(const std::string& sessionId,
                                         const QJsonObject& message,
                                         const std::string& excludeClientId)
{
    QByteArray data = QJsonDocument(message).toJson(QJsonDocument::Compact);

    for (auto& client : m_clients) {
        if (client.sessionId == sessionId && client.id != excludeClientId) {
            if (client.socket && client.socket->state() == QAbstractSocket::ConnectedState) {
                client.socket->sendTextMessage(data);
            }
        }
    }
}

void MultiUserServer::cleanupStaleParticipants()
{
    qint64 now = QDateTime::currentSecsSinceEpoch();
    const qint64 staleTimeout = 60;  // 60 seconds without update = stale

    for (auto it = m_clients.begin(); it != m_clients.end();) {
        if (it->isPresent && (now - it->lastUpdateTime) > staleTimeout) {
            qWarning() << "Removing stale participant:" << it->id;

            std::string sessionId = it->sessionId;
            if (!sessionId.empty()) {
                QJsonObject leaveMsg;
                leaveMsg["type"] = "peer_left";
                leaveMsg["clientId"] = QString::fromStdString(it->id);
                leaveMsg["clientName"] = QString::fromStdString(it->name);
                leaveMsg["reason"] = "timeout";
                broadcastToSession(sessionId, leaveMsg, it->id);

                m_sessions[sessionId].erase(it->id);
                if (m_sessions[sessionId].empty()) {
                    m_sessions.erase(sessionId);
                }
            }

            if (it->socket && it->socket->state() == QAbstractSocket::ConnectedState) {
                it->socket->disconnect();
            }

            it = m_clients.erase(it);
        } else {
            ++it;
        }
    }
}

std::vector<MultiUserServer::ClientEntry>::iterator MultiUserServer::findClient(const std::string& clientId)
{
    return std::find_if(m_clients.begin(), m_clients.end(),
                        [&clientId](const ClientEntry& c) {
                            return c.id == clientId;
                        });
}

QJsonObject MultiUserServer::serializeMessage(const Message& message) const
{
    QJsonObject obj;
    obj["type"] = static_cast<int>(message.type);
    obj["senderId"] = QString::fromStdString(message.senderId);
    obj["timestamp"] = message.timestamp;
    return obj;
}

} // namespace vr
} // namespace quantumverse
