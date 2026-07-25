/**
 * @file test_shared_session.cpp
 * @brief Unit tests for SharedSession serialization and state management
 *
 * Tests TheoryParams, SharedState, and SharedSession round-trips,
 * merges, and event handling.
 */

#include "vr/SharedSession.h"
#include <cmath>
#include <cassert>
#include <iostream>
#include <algorithm>

using namespace quantumverse;

static bool approxEqual(double a, double b, double eps = 1e-9) {
    return std::abs(a - b) < eps;
}

int g_failures = 0;

static void check(bool cond, const char* msg) {
    if (!cond) {
        std::cerr << "[FAIL] " << msg << std::endl;
        ++g_failures;
    }
}

static void assertTheoryParamsEqual(const TheoryParams& a, const TheoryParams& b) {
    check(a.name == b.name, "TheoryParams name mismatch");
    check(a.parameters.size() == b.parameters.size(), "TheoryParams parameter count mismatch");
    for (const auto& [key, val] : a.parameters) {
        auto it = b.parameters.find(key);
        check(it != b.parameters.end(), "TheoryParams missing parameter");
        check(approxEqual(it->second, val), "TheoryParams parameter value mismatch");
    }
}

static void assertLiveAlertEqual(const LiveAlert& a, const LiveAlert& b) {
    check(a.origin == b.origin, "LiveAlert origin mismatch");
    check(approxEqual(a.timestamp, b.timestamp), "LiveAlert timestamp mismatch");
    check(a.summary == b.summary, "LiveAlert summary mismatch");
}

int main()
{
    std::cout << "=== SharedSessionTest ===" << std::endl;

    // --- TheoryParams round-trip -----------------------------------------------
    {
        TheoryParams params;
        params.name = "Schwarzschild";
        params.parameters["mass"] = 1.989e30;
        params.parameters["spin"] = 0.0;
        params.parameters["charge"] = 0.0;

        QJsonObject obj = params.toJson();
        TheoryParams restored = TheoryParams::fromJson(obj);

        assertTheoryParamsEqual(params, restored);
        std::cout << "[PASS] TheoryParams round-trip" << std::endl;
    }

    // --- LiveAlert round-trip --------------------------------------------------
    {
        LiveAlert alert;
        alert.origin = "LIGO";
        alert.timestamp = 1700000000.0;
        alert.summary = "Test alert";

        QJsonObject obj = alert.toJson();
        LiveAlert restored = LiveAlert::fromJson(obj);

        assertLiveAlertEqual(alert, restored);
        std::cout << "[PASS] LiveAlert round-trip" << std::endl;
    }

    // --- SharedState round-trip ------------------------------------------------
    {
        SharedState state;
        Participant p1;
        p1.id = "peer1";
        p1.name = "Alice";
        p1.cameraMatrix = {1.0,0,0,0, 0,1.0,0,0, 0,0,1.0,0, 0,0,0,1.0};
        p1.vrActive = true;
        state.participants.push_back(p1);

        state.activeTheory.name = "Kerr";
        state.activeTheory.parameters["mass"] = 2.0;
        state.activeTheory.parameters["a"] = 0.5;

        state.bmaWeights = {0.6, 0.4};
        state.recentAlerts.push_back(LiveAlert{"IceCube", 1700000001.0, "Neutrino"});

        QJsonObject obj = state.toJson();
        SharedState restored = SharedState::fromJson(obj);

        check(restored.participants.size() == 1, "SharedState participant count");
        check(restored.participants[0].id == "peer1", "SharedState participant id");
        check(restored.participants[0].vrActive == true, "SharedState participant vrActive");
        check(restored.activeTheory.name == "Kerr", "SharedState activeTheory name");
        check(approxEqual(restored.activeTheory.parameters["a"], 0.5), "SharedState activeTheory a");
        check(restored.bmaWeights.size() == 2, "SharedState bmaWeights count");
        check(approxEqual(restored.bmaWeights[0], 0.6), "SharedState bmaWeights[0]");
        check(restored.recentAlerts.size() == 1, "SharedState recentAlerts count");
        assertLiveAlertEqual(restored.recentAlerts[0], state.recentAlerts[0]);
        std::cout << "[PASS] SharedState round-trip" << std::endl;
    }

    // --- SharedSession state updates -------------------------------------------
    {
        SharedSession session;
        check(!session.isActive(), "SharedSession initial inactive");

        TheoryParams t;
        t.name = "Test";
        t.parameters["x"] = 1.0;
        session.updateActiveTheory(t);
        check(session.state().activeTheory.name == "Test", "SharedSession theory name");
        check(approxEqual(session.state().activeTheory.parameters["x"], 1.0), "SharedSession theory x");

        DiscoveryResult dr;
        dr.confidence = 0.95;
        dr.description = "Result";
        session.addDiscoveryResult(dr);
        check(session.state().paretoFront.size() == 1, "SharedSession paretoFront count");
        check(session.state().paretoFront[0].confidence == 0.95, "SharedSession paretoFront confidence");

        LiveAlert la;
        la.origin = "Test";
        la.timestamp = 1.0;
        la.summary = "Alert";
        session.addLiveAlert(la);
        check(session.state().recentAlerts.size() == 1, "SharedSession recentAlerts count");
        check(session.state().recentAlerts[0].origin == "Test", "SharedSession recentAlerts origin");

        std::cout << "[PASS] SharedSession state updates" << std::endl;
    }

    // --- SharedSession applyPeerState merge ------------------------------------
    {
        SharedSession session;
        session.setSessionId("demo");

        TheoryParams localTheory;
        localTheory.name = "Local";
        localTheory.parameters["p"] = 1.0;
        session.updateActiveTheory(localTheory);

        DiscoveryResult localDR;
        localDR.confidence = 0.5;
        localDR.description = "local";
        session.addDiscoveryResult(localDR);

        SharedState remote;
        remote.activeTheory.name = "Remote";
        remote.activeTheory.parameters["p"] = 2.0;
        remote.activeTheory.parameters["q"] = 3.0;

        DiscoveryResult remoteDR;
        remoteDR.confidence = 0.9;
        remoteDR.description = "remote";
        remote.paretoFront.push_back(remoteDR);

        QJsonObject remoteJson = remote.toJson();
        session.applyPeerState("peer1", remoteJson);

        check(session.state().activeTheory.name == "Remote", "applyPeerState theory name");
        check(approxEqual(session.state().activeTheory.parameters["p"], 2.0), "applyPeerState theory p");
        check(approxEqual(session.state().activeTheory.parameters["q"], 3.0), "applyPeerState theory q");
        check(session.state().paretoFront.size() == 2, "applyPeerState paretoFront count");

        std::cout << "[PASS] SharedSession applyPeerState merge" << std::endl;
    }

    // --- Alert cap -------------------------------------------------------------
    {
        SharedSession session;
        for (int i = 0; i < 60; ++i) {
            LiveAlert la;
            la.origin = "src" + std::to_string(i);
            la.timestamp = static_cast<double>(i);
            la.summary = "Alert " + std::to_string(i);
            session.addLiveAlert(la);
        }
        check(session.state().recentAlerts.size() == 50, "SharedSession alert cap");
        std::cout << "[PASS] SharedSession alert cap" << std::endl;
    }

    if (g_failures == 0) {
        std::cout << "All SharedSession tests passed." << std::endl;
        return 0;
    } else {
        std::cerr << g_failures << " test(s) failed." << std::endl;
        return 1;
    }
}
