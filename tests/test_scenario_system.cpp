/**
 * @file test_scenario_system.cpp
 * @brief Unit tests for the scriptable scenario system
 *
 * Tests Scenario JSON serialization, camera interpolation,
 * event processing, and ScenarioManager file loading.
 */

#include "scenario/Scenario.h"
#include "scenario/ScenarioManager.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace quantumverse;

namespace fs = std::filesystem;

// ============================================================================
// Test helpers
// ============================================================================

static int test_count = 0;
static int pass_count = 0;

#define TEST_ASSERT(cond, msg) do { \
    test_count++; \
    if (!(cond)) { \
        std::cerr << "[FAIL] " << msg << " (line " << __LINE__ << ")" << std::endl; \
    } else { \
        pass_count++; \
        std::cout << "[PASS] " << msg << std::endl; \
    } \
} while(0)

#define TEST_ASSERT_NEAR(a, b, tol, msg) do { \
    test_count++; \
    if (std::abs((a) - (b)) > (tol)) { \
        std::cerr << "[FAIL] " << msg << " (got " << (a) << ", expected " << (b) << ", tol " << (tol) << ")" << std::endl; \
    } else { \
        pass_count++; \
        std::cout << "[PASS] " << msg << std::endl; \
    } \
} while(0)

// ============================================================================
// Tests
// ============================================================================

void test_scenario_serialization() {
    std::cout << "\n--- Scenario Serialization ---" << std::endl;

    Scenario original;
    original.name = "Test Scenario";
    original.description = "A test scenario for unit testing";
    original.version = "1.0";
    original.duration = 120.0;
    original.time_scale = 2.0;
    original.central_body = {"TestStar", 2.0e30, 7.0e8, {1.0, 0.9, 0.6}, true};
    original.physics = {6.67430e-11, 299792458.0, 6.626e-34, false};

    ScenarioBodyConfig body;
    body.name = "TestPlanet";
    body.mass_kg = 5.0e24;
    body.radius_m = 6.0e6;
    body.semi_major_axis_au = 1.0;
    original.bodies.push_back(body);

    ScenarioEvent event;
    event.time = 10.0;
    event.type = ScenarioEventType::INJECT_ASTEROID;
    event.params[0] = 1.0e11;
    event.params[1] = 0.0;
    event.params[2] = 0.0;
    event.params[3] = 10000.0;
    event.params[4] = 0.0;
    event.params[5] = 0.0;
    event.params[6] = 1e15;
    original.events.push_back(event);

    ScenarioCameraKeyframe kf1, kf2;
    kf1.time = 0.0;
    kf1.position[2] = 500.0;
    kf1.fov = 60.0;
    kf2.time = 60.0;
    kf2.position[0] = 200.0;
    kf2.position[1] = 100.0;
    kf2.position[2] = 300.0;
    kf2.fov = 45.0;
    original.camera_path.push_back(kf1);
    original.camera_path.push_back(kf2);

    json j = original;
    Scenario restored = j.get<Scenario>();

    TEST_ASSERT(restored.name == original.name, "Name preserved through serialization");
    TEST_ASSERT(restored.description == original.description, "Description preserved");
    TEST_ASSERT_NEAR(restored.duration, original.duration, 1e-10, "Duration preserved");
    TEST_ASSERT_NEAR(restored.time_scale, original.time_scale, 1e-10, "Time scale preserved");
    TEST_ASSERT(restored.bodies.size() == 1, "Body count preserved");
    TEST_ASSERT(restored.events.size() == 1, "Event count preserved");
    TEST_ASSERT(restored.camera_path.size() == 2, "Camera keyframe count preserved");
    TEST_ASSERT(restored.events[0].type == ScenarioEventType::INJECT_ASTEROID, "Event type preserved");
    TEST_ASSERT_NEAR(restored.events[0].params[0], 1.0e11, 1e-5, "Event params preserved");
    TEST_ASSERT_NEAR(restored.camera_path[1].fov, 45.0, 1e-10, "Camera FOV preserved");
}

void test_camera_interpolation() {
    std::cout << "\n--- Camera Interpolation ---" << std::endl;

    std::vector<ScenarioCameraKeyframe> keyframes;
    ScenarioCameraKeyframe kf1, kf2, kf3;
    kf1.time = 0.0;
    kf1.position[2] = 500.0;
    kf1.fov = 60.0;
    kf2.time = 50.0;
    kf2.position[0] = 200.0;
    kf2.position[2] = 300.0;
    kf2.fov = 45.0;
    kf3.time = 100.0;
    kf3.position[1] = 400.0;
    kf3.position[2] = 200.0;
    kf3.fov = 30.0;
    keyframes.push_back(kf1);
    keyframes.push_back(kf2);
    keyframes.push_back(kf3);

    ScenarioCameraState s0 = interpolateCamera(keyframes, 0.0);
    TEST_ASSERT_NEAR(s0.position[2], 500.0, 1e-6, "Camera at t=0 matches first keyframe");
    TEST_ASSERT_NEAR(s0.fov, 60.0, 1e-6, "FOV at t=0 matches first keyframe");

    ScenarioCameraState s100 = interpolateCamera(keyframes, 100.0);
    TEST_ASSERT_NEAR(s100.position[1], 400.0, 1e-6, "Camera at t=100 matches last keyframe");
    TEST_ASSERT_NEAR(s100.position[2], 200.0, 1e-6, "Camera Z at t=100 matches last keyframe");

    ScenarioCameraState s25 = interpolateCamera(keyframes, 25.0);
    TEST_ASSERT(s25.position[2] > 300.0 && s25.position[2] < 500.0,
                "Camera Z at t=25 is interpolated between keyframes");

    ScenarioCameraState sNeg = interpolateCamera(keyframes, -10.0);
    TEST_ASSERT_NEAR(sNeg.position[2], 500.0, 1e-6, "Camera before start clamps to first keyframe");

    ScenarioCameraState sOver = interpolateCamera(keyframes, 200.0);
    TEST_ASSERT_NEAR(sOver.position[1], 400.0, 1e-6, "Camera after end clamps to last keyframe");

    std::vector<ScenarioCameraKeyframe> empty;
    ScenarioCameraState sEmpty = interpolateCamera(empty, 50.0);
    TEST_ASSERT_NEAR(sEmpty.position[2], 500.0, 1e-6, "Empty keyframes returns default");
}

void test_event_sorting() {
    std::cout << "\n--- Event Sorting ---" << std::endl;

    Scenario scenario;
    ScenarioEvent e1, e2, e3;
    e1.time = 30.0;
    e1.type = ScenarioEventType::SET_TIME_SCALE;
    e2.time = 10.0;
    e2.type = ScenarioEventType::INJECT_ASTEROID;
    e3.time = 20.0;
    e3.type = ScenarioEventType::SET_CAMERA;
    scenario.events.push_back(e1);
    scenario.events.push_back(e2);
    scenario.events.push_back(e3);

    scenario.sortEvents();

    TEST_ASSERT(scenario.events[0].time == 10.0, "First event has smallest time");
    TEST_ASSERT(scenario.events[1].time == 20.0, "Second event has middle time");
    TEST_ASSERT(scenario.events[2].time == 30.0, "Third event has largest time");
}

void test_event_type_strings() {
    std::cout << "\n--- Event Type Strings ---" << std::endl;

    TEST_ASSERT(eventTypeToString(ScenarioEventType::INJECT_ASTEROID) == "INJECT_ASTEROID",
                "INJECT_ASTEROID string conversion");
    TEST_ASSERT(eventTypeToString(ScenarioEventType::SET_CAMERA) == "SET_CAMERA",
                "SET_CAMERA string conversion");
    TEST_ASSERT(eventTypeToString(ScenarioEventType::SET_CONSTANT) == "SET_CONSTANT",
                "SET_CONSTANT string conversion");

    TEST_ASSERT(stringToEventType("INJECT_ASTEROID") == ScenarioEventType::INJECT_ASTEROID,
                "INJECT_ASTEROID parse");
    TEST_ASSERT(stringToEventType("SET_CAMERA") == ScenarioEventType::SET_CAMERA,
                "SET_CAMERA parse");
    TEST_ASSERT(stringToEventType("UNKNOWN_TYPE") == ScenarioEventType::WAIT,
                "Unknown type defaults to WAIT");
}

void test_scenario_validation() {
    std::cout << "\n--- Scenario Validation ---" << std::endl;

    std::string error;

    Scenario valid;
    valid.name = "Valid";
    valid.duration = 60.0;
    valid.time_scale = 1.0;
    valid.central_body.mass_kg = 1.0e30;
    TEST_ASSERT(valid.validate(&error), "Valid scenario passes validation");

    Scenario no_name;
    no_name.duration = 60.0;
    no_name.central_body.mass_kg = 1.0e30;
    TEST_ASSERT(!no_name.validate(&error), "Empty name fails validation");

    Scenario bad_duration;
    bad_duration.name = "Bad";
    bad_duration.duration = -1.0;
    bad_duration.central_body.mass_kg = 1.0e30;
    TEST_ASSERT(!bad_duration.validate(&error), "Negative duration fails validation");

    Scenario bad_mass;
    bad_mass.name = "Bad";
    bad_mass.duration = 60.0;
    bad_mass.central_body.mass_kg = 0.0;
    TEST_ASSERT(!bad_mass.validate(&error), "Zero central mass fails validation");
}

void test_scenario_file_save_load() {
    std::cout << "\n--- Scenario File Save/Load ---" << std::endl;

    Scenario original;
    original.name = "FileTest";
    original.description = "Test file I/O";
    original.duration = 90.0;
    original.central_body = {"TestStar", 1.5e30, 5.0e8, {1.0, 0.8, 0.4}, true};

    ScenarioEvent ev;
    ev.time = 5.0;
    ev.type = ScenarioEventType::INJECT_ASTEROID;
    ev.params[0] = 2.0e11;
    original.events.push_back(ev);

    std::string tmp_path = "test_scenario_tmp.json";
    bool saved = original.saveToFile(tmp_path);
    TEST_ASSERT(saved, "Scenario saved to file");

    Scenario loaded;
    bool success = loaded.loadFromFile(tmp_path);
    TEST_ASSERT(success, "Scenario loaded from file");
    TEST_ASSERT(loaded.name == "FileTest", "Name matches after load");
    TEST_ASSERT(loaded.events.size() == 1, "Event count matches after load");

    if (fs::exists(tmp_path)) {
        fs::remove(tmp_path);
    }
}

void test_scenario_manager() {
    std::cout << "\n--- Scenario Manager ---" << std::endl;

    std::string test_dir = "test_scenarios_dir";
    fs::create_directories(test_dir);

    {
        Scenario s1;
        s1.name = "Manager Test 1";
        s1.duration = 60.0;
        s1.central_body.mass_kg = 1.0e30;
        s1.saveToFile(test_dir + "/test1.json");
    }
    {
        Scenario s2;
        s2.name = "Manager Test 2";
        s2.duration = 120.0;
        s2.central_body.mass_kg = 2.0e30;
        s2.saveToFile(test_dir + "/test2.json");
    }

    auto& manager = ScenarioManager::instance();
    manager.initialize(test_dir);

    TEST_ASSERT(manager.count() == 2, "Manager loaded 2 scenarios");
    TEST_ASSERT(manager.hasScenario("Manager Test 1"), "Manager has scenario 1");
    TEST_ASSERT(manager.hasScenario("Manager Test 2"), "Manager has scenario 2");

    auto s1 = manager.getScenario("Manager Test 1");
    TEST_ASSERT(s1 != nullptr, "Retrieved scenario 1");
    if (s1) {
        TEST_ASSERT_NEAR(s1->duration, 60.0, 1e-10, "Scenario 1 duration correct");
    }

    auto names = manager.listScenarios();
    TEST_ASSERT(names.size() == 2, "List scenarios returns 2");

    manager.clear();
    TEST_ASSERT(manager.count() == 0, "Manager cleared");

    fs::remove_all(test_dir);
}

void test_event_range_query() {
    std::cout << "\n--- Event Range Query ---" << std::endl;

    Scenario scenario;
    for (int i = 0; i < 10; i++) {
        ScenarioEvent ev;
        ev.time = i * 10.0;
        ev.type = ScenarioEventType::INJECT_ASTEROID;
        scenario.events.push_back(ev);
    }
    scenario.sortEvents();

    auto in_range = scenario.getEventsInRange(25.0, 55.0);
    TEST_ASSERT(in_range.size() == 3, "Events in range [25,55]: 30,40,50");

    auto all_events = scenario.getEventsOfType(ScenarioEventType::INJECT_ASTEROID);
    TEST_ASSERT(all_events.size() == 10, "All events are INJECT_ASTEROID");
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "=== Scenario System Unit Tests ===" << std::endl;

    test_scenario_serialization();
    test_camera_interpolation();
    test_event_sorting();
    test_event_type_strings();
    test_scenario_validation();
    test_scenario_file_save_load();
    test_scenario_manager();
    test_event_range_query();

    std::cout << "\n=== Results: " << pass_count << "/" << test_count << " passed ===" << std::endl;

    if (pass_count == test_count) {
        std::cout << "=== ALL TESTS PASSED ===" << std::endl;
        return 0;
    } else {
        std::cerr << "=== SOME TESTS FAILED ===" << std::endl;
        return 1;
    }
}
