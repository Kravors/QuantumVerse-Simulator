/**
 * @file SignalingClient.cpp
 * @brief Implementation of WebSocket-based signaling client
 *
 * Provides connection management, message serialization, and
 * peer/state event handling for multi-user collaboration.
 */

#include "SignalingClient.h"
#include <QUrl>
#include <QUuid>
#include <QDateTime>
#include <QJsonArray>

namespace quantumverse {
namespace vr {

SignalingClient::SignalingClient(QObject* parent)
    : QObject(parent)
    , m_socket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
    , m_reconnectTimer(new QTimer(this))
{
    m_reconnectTimer->setSingleShot(true);
    m_reconnectTimer->setInterval(kReconnectDelayMs);

    connect(m_socket, &QWebSocket::connected,
            this, &SignalingClient::onConnected);
    connect(m_socket, &QWebSocket::disconnected,
            this, &SignalingClient::onDisconnected);
    connect(m_socket, &QWebSocket::textMessageReceived,
            this, &SignalingClient::onTextMessageReceived);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred),
            this, &SignalingClient::onSocketError);
    connect(m_reconnectTimer, &QTimer::timeout, this, [this]() {
        if (!m_connected && m_reconnectAttempts < kMaxReconnectAttempts) {
            m_socket->abort();
            m_socket->open(QUrl(m_url));
            ++m_reconnectAttempts;
        }
    });
}

SignalingClient::~SignalingClient()
{
    disconnectFromServer();
}

bool SignalingClient::connectToServer(const QString& url,
                                      const QString& sessionId,
                                      const QString& clientId,
                                      const QString& clientName)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->close();
    }

    m_url = url;
    m_sessionId = sessionId;
    m_clientId = clientId.isEmpty() ? generateClientId() : clientId;
    m_clientName = clientName;
    m_peerIds.clear();
    m_reconnectAttempts = 0;

    QUrl parsed(url);
    if (!parsed.isValid()) {
        emit connectionError("Invalid WebSocket URL: " + url);
        return false;
    }

    m_socket->open(parsed);
    return true;
}

void SignalingClient::disconnectFromServer()
{
    m_reconnectTimer->stop();
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QJsonObject leaveMsg;
        leaveMsg["type"] = "leave";
        leaveMsg["sessionId"] = m_sessionId;
        leaveMsg["clientId"] = m_clientId;
        sendJson(leaveMsg);
        m_socket->close();
    }
    m_connected = false;
    m_peerIds.clear();
}

void SignalingClient::broadcastState(const QJsonObject& stateJson)
{
    if (!m_connected) return;

    QJsonObject msg;
    msg["type"] = "state_update";
    msg["sessionId"] = m_sessionId;
    msg["senderId"] = m_clientId;
    msg["senderName"] = m_clientName;
    msg["state"] = stateJson;
    msg["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    sendJson(msg);
}

void SignalingClient::onConnected()
{
    m_connected = true;
    m_reconnectAttempts = 0;

    QJsonObject joinMsg;
    joinMsg["type"] = "join";
    joinMsg["sessionId"] = m_sessionId;
    joinMsg["clientId"] = m_clientId;
    joinMsg["clientName"] = m_clientName;
    joinMsg["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    sendJson(joinMsg);

    emit connected();
    emit isConnectedChanged();
}

void SignalingClient::onDisconnected()
{
    bool wasConnected = m_connected;
    m_connected = false;
    m_peerIds.clear();

    if (wasConnected) {
        emit disconnected("WebSocket disconnected");
        emit isConnectedChanged();
    }

    if (m_reconnectAttempts < kMaxReconnectAttempts && !m_url.isEmpty()) {
        m_reconnectTimer->start();
    }
}

void SignalingClient::onTextMessageReceived(const QString& message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        emit connectionError("Invalid JSON received from server");
        return;
    }

    handleServerMessage(doc.object());
}

void SignalingClient::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    emit connectionError("WebSocket error: " + m_socket->errorString());
}

void SignalingClient::sendJson(const QJsonObject& obj)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->sendTextMessage(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    }
}

void SignalingClient::handleServerMessage(const QJsonObject& obj)
{
    QString type = obj.value("type").toString();

    if (type == "peer_joined") {
        QString peerId = obj.value("clientId").toString();
        QString peerName = obj.value("clientName").toString();
        if (!peerId.isEmpty() && peerId != m_clientId) {
            m_peerIds.push_back(peerId.toStdString());
            emit peerJoined(peerId, peerName);
            emit peerCountChanged();
        }
    } else if (type == "peer_left") {
        QString peerId = obj.value("clientId").toString();
        if (!peerId.isEmpty()) {
            auto it = std::find(m_peerIds.begin(), m_peerIds.end(), peerId.toStdString());
            if (it != m_peerIds.end()) {
                m_peerIds.erase(it);
            }
            emit peerLeft(peerId);
            emit peerCountChanged();
        }
    } else if (type == "state_update") {
        QString senderId = obj.value("senderId").toString();
        QJsonObject state = obj.value("state").toObject();
        if (!senderId.isEmpty() && senderId != m_clientId) {
            emit stateReceived(senderId, state);
        }
    } else if (type == "welcome") {
        QJsonArray peers = obj.value("peers").toArray();
        m_peerIds.clear();
        for (const QJsonValue& val : peers) {
            if (val.isString()) {
                m_peerIds.push_back(val.toString().toStdString());
            }
        }
    }
}

} // namespace vr
} // namespace quantumverse
