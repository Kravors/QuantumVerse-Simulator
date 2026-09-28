/**
 * @file test_multi_user_server.cpp
 * @brief Unit tests for MultiUserServer session management
 *
 * Tests multi-user VR session server functionality including
 * client connections, session management, and message routing.
 */

#include "vr/MultiUserServer.h"
#include <QCoreApplication>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <cassert>
#include <iostream>
#include <algorithm>

using namespace quantumverse;
using namespace quantumverse::vr;

int g_failures = 0;

static void check(bool cond, const char* msg) {
    if (!cond) {
        std::cerr << "[FAIL] " << msg << std::endl;
        ++g_failures;
    }
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    std::cout << "=== MultiUserServerTest ===" << std::endl;

    // --- Server startup and shutdown -------------------------------------------
    {
        MultiUserServer server;
        check(!server.isRunning(), "Server not running initially");

        bool started = server.start(7777);
        check(started, "Server starts on available port");
        check(server.isRunning(), "Server is running after start");

        server.stop();
        check(!server.isRunning(), "Server stopped");
        std::cout << "[PASS] Server lifecycle" << std::endl;
    }

    // --- Client connection and session join -------------------------------------
    {
        MultiUserServer server;
        server.start(7778);

        QWebSocket socket;
        bool joinReceived = false;

        QObject::connect(&socket, &QWebSocket::connected, [&]() {
            QJsonObject joinMsg;
            joinMsg["type"] = "join";
            joinMsg["sessionId"] = "test-session";
            joinMsg["clientName"] = "Alice";
            socket.sendTextMessage(QJsonDocument(joinMsg).toJson(QJsonDocument::Compact));
        });

        QObject::connect(&socket, &QWebSocket::textMessageReceived, [&](const QString& msg) {
            QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8());
            if (doc.isObject() && doc.object().value("type").toString() == "join_confirmed") {
                joinReceived = true;
            }
        });

        socket.open(QUrl("ws://localhost:7778"));

        QTimer::singleShot(1000, [&]() {
            auto participants = server.getParticipants();
            check(participants.size() == 1, "One participant after join");
            if (participants.size() == 1) {
                check(participants[0].name == "Alice", "Participant name correct");
                check(participants[0].isPresent, "Participant is present");
            }

            server.stop();
            socket.close();
            QCoreApplication::quit();
        });

        app.exec();
        check(joinReceived, "Join confirmation received");
        std::cout << "[PASS] Client connection and join" << std::endl;
    }

    // --- Multiple clients in same session ---------------------------------------
    {
        MultiUserServer server;
        server.start(7779);

        QWebSocket socket1;
        QWebSocket socket2;
        int joinCount = 0;

        QObject::connect(&socket1, &QWebSocket::connected, [&]() {
            QJsonObject joinMsg;
            joinMsg["type"] = "join";
            joinMsg["sessionId"] = "shared-session";
            joinMsg["clientName"] = "Alice";
            socket1.sendTextMessage(QJsonDocument(joinMsg).toJson(QJsonDocument::Compact));
        });

        QObject::connect(&socket2, &QWebSocket::connected, [&]() {
            QJsonObject joinMsg;
            joinMsg["type"] = "join";
            joinMsg["sessionId"] = "shared-session";
            joinMsg["clientName"] = "Bob";
            socket2.sendTextMessage(QJsonDocument(joinMsg).toJson(QJsonDocument::Compact));
        });

        QObject::connect(&socket1, &QWebSocket::textMessageReceived, [&](const QString&) {
            joinCount++;
        });
        QObject::connect(&socket2, &QWebSocket::textMessageReceived, [&](const QString&) {
            joinCount++;
        });

        socket1.open(QUrl("ws://localhost:7779"));
        socket2.open(QUrl("ws://localhost:7779"));

        QTimer::singleShot(1000, [&]() {
            auto participants = server.getParticipants();
            check(participants.size() == 2, "Two participants in same session");

            server.stop();
            socket1.close();
            socket2.close();
            QCoreApplication::quit();
        });

        app.exec();
        check(joinCount >= 2, "Both clients received join confirmation");
        std::cout << "[PASS] Multiple clients in same session" << std::endl;
    }

    // --- Participants in different sessions -------------------------------------
    {
        MultiUserServer server;
        server.start(7780);

        QWebSocket socket1;
        QWebSocket socket2;
        int joinCount = 0;

        QObject::connect(&socket1, &QWebSocket::connected, [&]() {
            QJsonObject joinMsg;
            joinMsg["type"] = "join";
            joinMsg["sessionId"] = "session-a";
            joinMsg["clientName"] = "Alice";
            socket1.sendTextMessage(QJsonDocument(joinMsg).toJson(QJsonDocument::Compact));
        });

        QObject::connect(&socket2, &QWebSocket::connected, [&]() {
            QJsonObject joinMsg;
            joinMsg["type"] = "join";
            joinMsg["sessionId"] = "session-b";
            joinMsg["clientName"] = "Bob";
            socket2.sendTextMessage(QJsonDocument(joinMsg).toJson(QJsonDocument::Compact));
        });

        QObject::connect(&socket1, &QWebSocket::textMessageReceived, [&](const QString&) {
            joinCount++;
        });
        QObject::connect(&socket2, &QWebSocket::textMessageReceived, [&](const QString&) {
            joinCount++;
        });

        socket1.open(QUrl("ws://localhost:7780"));
        socket2.open(QUrl("ws://localhost:7780"));

        QTimer::singleShot(1000, [&]() {
            auto participants = server.getParticipants();
            check(participants.size() == 2, "Two participants in different sessions");

            server.stop();
            socket1.close();
            socket2.close();
            QCoreApplication::quit();
        });

        app.exec();
        check(joinCount >= 2, "Both clients received join confirmation");
        std::cout << "[PASS] Participants in different sessions" << std::endl;
    }

    if (g_failures == 0) {
        std::cout << "All MultiUserServer tests passed." << std::endl;
        return 0;
    } else {
        std::cerr << g_failures << " test(s) failed." << std::endl;
        return 1;
    }
}
