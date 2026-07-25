/**
 * @file test_signaling_client.cpp
 * @brief Unit tests for SignalingClient message handling
 *
 * Tests SignalingClient message construction, parsing, and state
 * handling without requiring a live WebSocket server.
 */

#include "vr/SignalingClient.h"
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

static bool approxEqual(double a, double b, double eps = 1e-9) {
    return std::abs(a - b) < eps;
}

int main()
{
    std::cout << "=== SignalingClientTest ===" << std::endl;

    // --- Client ID generation ------------------------------------------------
    {
        QString id1 = SignalingClient::generateClientId();
        QString id2 = SignalingClient::generateClientId();
        check(!id1.isEmpty(), "Client ID not empty");
        check(!id2.isEmpty(), "Client ID not empty");
        check(id1 != id2, "Client IDs unique");
        std::cout << "[PASS] Client ID generation" << std::endl;
    }

    // --- SignalingClient initial state ---------------------------------------
    {
        SignalingClient client;
        QString url = "ws://localhost:8080";
        QString session = "test-session";
        QString clientId = "client-123";
        QString name = "Tester";

        client.connectToServer(url, session, clientId, name);

        check(client.sessionId() == session, "SignalingClient sessionId");
        check(client.clientId() == clientId, "SignalingClient clientId");
        check(!client.isConnected(), "SignalingClient not connected before open");
        std::cout << "[PASS] SignalingClient initial state" << std::endl;
    }

    // --- SignalingClient initial peer count ----------------------------------
    {
        SignalingClient client;
        check(client.peerCount() == 0, "SignalingClient initial peer count");
        std::cout << "[PASS] SignalingClient initial peer count" << std::endl;
    }

    // --- State update JSON round-trip ----------------------------------------
    {
        SignalingClient client;
        client.connectToServer("ws://localhost:8080", "s1", "c1", "Tester");

        QJsonObject theoryObj;
        theoryObj["name"] = "Schwarzschild";
        QJsonObject params;
        params["mass"] = 1.989e30;
        theoryObj["parameters"] = params;

        QJsonObject stateObj;
        stateObj["activeTheory"] = theoryObj;

        QJsonDocument doc(stateObj);
        QString jsonStr = doc.toJson(QJsonDocument::Compact);
        QJsonDocument parsed = QJsonDocument::fromJson(jsonStr.toUtf8());
        check(parsed.isObject(), "State JSON parse success");
        QJsonObject restored = parsed.object();
        check(restored["activeTheory"].toObject()["name"].toString() == "Schwarzschild", "State JSON theory name");
        check(approxEqual(restored["activeTheory"].toObject()["parameters"].toObject()["mass"].toDouble(), 1.989e30), "State JSON theory mass");
        std::cout << "[PASS] State JSON round-trip" << std::endl;
    }

    // --- Peer join/leave message format --------------------------------------
    {
        QJsonObject joinMsg;
        joinMsg["type"] = "peer_joined";
        joinMsg["clientId"] = "new-peer";
        joinMsg["clientName"] = "New Peer";

        QJsonDocument doc(joinMsg);
        QString json = doc.toJson(QJsonDocument::Compact);
        QJsonDocument parsed = QJsonDocument::fromJson(json.toUtf8());
        check(parsed.isObject(), "Join message parse success");
        check(parsed.object()["type"].toString() == "peer_joined", "Join message type");
        check(parsed.object()["clientId"].toString() == "new-peer", "Join message clientId");
        std::cout << "[PASS] Peer join message format" << std::endl;

        QJsonObject leaveMsg;
        leaveMsg["type"] = "peer_left";
        leaveMsg["clientId"] = "old-peer";

        QJsonDocument leaveDoc(leaveMsg);
        QString leaveJson = leaveDoc.toJson(QJsonDocument::Compact);
        QJsonDocument leaveParsed = QJsonDocument::fromJson(leaveJson.toUtf8());
        check(leaveParsed.object()["type"].toString() == "peer_left", "Leave message type");
        std::cout << "[PASS] Peer leave message format" << std::endl;
    }

    // --- State update message format ------------------------------------------
    {
        QJsonObject stateUpdate;
        stateUpdate["type"] = "state_update";
        stateUpdate["sessionId"] = "s1";
        stateUpdate["senderId"] = "client-1";
        stateUpdate["senderName"] = "Alice";

        QJsonObject innerState;
        innerState["activeTheory"] = QJsonObject();
        stateUpdate["state"] = innerState;
        stateUpdate["timestamp"] = QDateTime::currentMSecsSinceEpoch();

        QJsonDocument doc(stateUpdate);
        QString json = doc.toJson(QJsonDocument::Compact);
        QJsonDocument parsed = QJsonDocument::fromJson(json.toUtf8());
        check(parsed.object()["type"].toString() == "state_update", "State update type");
        check(parsed.object()["senderId"].toString() == "client-1", "State update senderId");
        std::cout << "[PASS] State update message format" << std::endl;
    }

    if (g_failures == 0) {
        std::cout << "All SignalingClient tests passed." << std::endl;
        return 0;
    } else {
        std::cerr << g_failures << " test(s) failed." << std::endl;
        return 1;
    }
}
