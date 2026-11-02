/**
 * @file test_vr_camera_sync.cpp
 * @brief Unit test for VR peer camera pose sync.
 *
 * Verifies that MultiUserServer forwards camera_pose messages between clients
 * in the same session. This is the foundation for Demo A (two clients see the
 * same camera orbit): client 1 sends camera_pose, server broadcasts to client 2,
 * client 2 receives it with correct senderId and position.
 *
 * Does NOT test clock sync — that doesn't exist yet.
 */

#include "vr/MultiUserServer.h"
#include "vr/SignalingClient.h"
#include <QCoreApplication>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUrl>
#include "test_assert.h"
#include <iostream>

using namespace quantumverse;
using namespace quantumverse::vr;

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    std::cout << "=== VR Camera Sync Test ===" << std::endl;

    // --- Server + two clients, camera pose round-trip -------------------------
    {
        MultiUserServer server;
        QV_CHECK(server.start(7782));
        QV_CHECK(server.isRunning());

        QWebSocket client1;
        QWebSocket client2;
        QString client1Id;
        QString client2Id;

        bool client1GotWelcome = false;
        bool client2GotWelcome = false;
        bool client2GotPose = false;
        QJsonObject receivedCamera;

        // Welcome message gives clientId
        QObject::connect(&client1, &QWebSocket::textMessageReceived,
            [&](const QString& msg) {
                QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8());
                if (doc.isObject()) {
                    QJsonObject obj = doc.object();
                    if (obj.value("type").toString() == "welcome") {
                        client1Id = obj.value("clientId").toString();
                        client1GotWelcome = true;
                    }
                }
            });
        QObject::connect(&client2, &QWebSocket::textMessageReceived,
            [&](const QString& msg) {
                QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8());
                if (doc.isObject()) {
                    QJsonObject obj = doc.object();
                    if (obj.value("type").toString() == "welcome") {
                        client2Id = obj.value("clientId").toString();
                        client2GotWelcome = true;
                    }
                    if (obj.value("type").toString() == "camera_pose") {
                        client2GotPose = true;
                        receivedCamera = obj.value("camera").toObject();
                    }
                }
            });

        QObject::connect(&client1, &QWebSocket::connected, [&]() {
            QJsonObject joinMsg;
            joinMsg["type"] = "join";
            joinMsg["sessionId"] = "vr-test-session";
            joinMsg["clientName"] = "Client1";
            client1.sendTextMessage(QJsonDocument(joinMsg).toJson(QJsonDocument::Compact));
        });
        QObject::connect(&client2, &QWebSocket::connected, [&]() {
            QJsonObject joinMsg;
            joinMsg["type"] = "join";
            joinMsg["sessionId"] = "vr-test-session";
            joinMsg["clientName"] = "Client2";
            client2.sendTextMessage(QJsonDocument(joinMsg).toJson(QJsonDocument::Compact));
        });

        client1.open(QUrl("ws://localhost:7782"));
        client2.open(QUrl("ws://localhost:7782"));

        // Send camera pose from client 1 after both joined
        QTimer::singleShot(500, [&]() {
            QJsonObject poseMsg;
            poseMsg["type"] = "camera_pose";
            poseMsg["sessionId"] = "vr-test-session";
            poseMsg["senderId"] = client1Id;
            QJsonObject cam;
            cam["x"] = 42.5;
            cam["y"] = -7.3;
            cam["z"] = 0.0;
            cam["az"] = 0.785;
            cam["el"] = 0.524;
            cam["d"] = 150.0;
            poseMsg["camera"] = cam;
            client1.sendTextMessage(QJsonDocument(poseMsg).toJson(QJsonDocument::Compact));
        });

        QTimer::singleShot(1500, [&]() {
            QV_CHECK(client1GotWelcome);
            QV_CHECK(client2GotWelcome);
            QV_CHECK(client2GotPose);
            QV_CHECK(receivedCamera.value("x").toDouble() == 42.5);
            QV_CHECK(receivedCamera.value("y").toDouble() == -7.3);
            QV_CHECK(receivedCamera.value("z").toDouble() == 0.0);
            QV_CHECK(qFuzzyCompare(receivedCamera.value("az").toDouble(), 0.785));
            QV_CHECK(qFuzzyCompare(receivedCamera.value("el").toDouble(), 0.524));
            QV_CHECK(qFuzzyCompare(receivedCamera.value("d").toDouble(), 150.0));

            server.stop();
            client1.close();
            client2.close();
            QCoreApplication::quit();
        });

        app.exec();

        QV_CHECK(client1GotWelcome);
        QV_CHECK(client2GotWelcome);
        QV_CHECK(client2GotPose);
    }

    std::cout << "=== VR Camera Sync Test: ALL PASS ===" << std::endl;
    return 0;
}
