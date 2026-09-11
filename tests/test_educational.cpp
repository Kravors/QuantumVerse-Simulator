// tests/test_educational.cpp
// Educational Mode TDD suite - 6 groups, no Qt Test dependency.
#include "education/EducationalTour.h"
#include "education/TourController.h"
#include "education/TourManager.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace quantumverse;

namespace {

int g_checks = 0;
int g_failures = 0;

#define CHECK(cond) do {                                                     \
    ++g_checks;                                                              \
    if (!(cond)) {                                                           \
        ++g_failures;                                                        \
        std::fprintf(stderr, "FAIL %s:%d: %s\n",                             \
                     __FILE__, __LINE__, #cond);                             \
    }                                                                        \
} while (0)

#define CHECK_NEAR(a, b, eps) do {                                           \
    ++g_checks;                                                              \
    double _a = (a), _b = (b);                                               \
    if (std::fabs(_a - _b) > (eps)) {                                        \
        ++g_failures;                                                        \
        std::fprintf(stderr, "FAIL %s:%d: |%.9f - %.9f| > %.3g\n",           \
                     __FILE__, __LINE__, _a, _b, (double)(eps));             \
    }                                                                        \
} while (0)

// ---- 1. JSON round-trip -----------------------------------------------------
void test_json_round_trip() {
    EducationalTour t;
    t.id = "test_tour";
    t.title = "Test Tour";
    t.description = "three-step round trip";

    TourStep s1;
    s1.title = "Step One";
    s1.body  = "body one";
    s1.duration_sec = 5.0;
    s1.camera.azimuth   = 45.0;
    s1.camera.elevation = 20.0;
    s1.camera.distance  = 200.0;
    s1.camera.target    = {0.0, 0.0, 0.0};

    TourStep s2;
    s2.title = "Step Two";
    s2.duration_sec = 6.5;
    s2.camera.azimuth  = 90.0;
    s2.camera.distance = 150.0;
    s2.actions.push_back(TourAction{"enable_lensing", nlohmann::json(true)});

    TourStep s3;
    s3.title = "Step Three";
    s3.duration_sec = 0.0;                 // manual advance

    t.steps = {s1, s2, s3};
    CHECK(t.validate());

    const std::string path = "test_tour_roundtrip.json";
    CHECK(t.saveToFile(path));

    EducationalTour r;
    CHECK(r.loadFromFile(path));
    CHECK(r.id          == t.id);
    CHECK(r.title       == t.title);
    CHECK(r.description == t.description);
    CHECK(r.steps.size() == 3u);
    if (r.steps.size() == 3u) {
        CHECK(r.steps[0].title == "Step One");
        CHECK_NEAR(r.steps[0].camera.azimuth,  45.0, 1e-9);
        CHECK_NEAR(r.steps[0].camera.distance, 200.0, 1e-9);
        CHECK(r.steps[1].actions.size() == 1u);
        if (r.steps[1].actions.size() == 1u) {
            CHECK(r.steps[1].actions[0].type == "enable_lensing");
            CHECK(r.steps[1].actions[0].value.get<bool>() == true);
        }
        CHECK(r.steps[2].duration_sec == 0.0);
    }
    std::remove(path.c_str());
}

// ---- 2. State machine -------------------------------------------------------
void test_state_machine() {
    EducationalTour t;
    t.id = "state_test";
    t.title = "State Test";
    for (int i = 0; i < 3; ++i) {
        TourStep s;
        s.title = "S" + std::to_string(i);
        s.duration_sec = 0.0;
        t.steps.push_back(s);
    }

    TourController c;
    int completes = 0;
    c.setCameraGetter([] { TourCameraState k; k.azimuth = 0.0; k.distance = 100.0; return k; });
    c.setCameraSetter([](const TourCameraState&) {});
    c.setOnComplete([&] { ++completes; });

    CHECK(c.state() == TourState::Stopped);

    c.loadTour(t);
    c.start();
    CHECK(c.state() == TourState::Running);
    CHECK(c.currentStepIndex() == 0);

    c.next();
    CHECK(c.currentStepIndex() == 1);
    c.next();
    CHECK(c.currentStepIndex() == 2);

    // next() past last step -> completes and stops
    c.next();
    CHECK(c.state()       == TourState::Stopped);
    CHECK(completes       == 1);

    // restart, walk forward then back
    c.loadTour(t);
    c.start();
    c.next();
    c.prev();
    CHECK(c.currentStepIndex() == 0);
    // prev() at 0 stays at 0
    c.prev();
    CHECK(c.currentStepIndex() == 0);

    c.stop();
    CHECK(c.state() == TourState::Stopped);
}

// ---- 2b. Pause/resume on user input ----------------------------------------
void test_pause_resume() {
    EducationalTour t;
    t.id = "pause_resume";
    t.title = "Pause Resume";
    TourStep s0; s0.title = "S0"; s0.duration_sec = 0.0;
    TourStep s1; s1.title = "S1"; s1.duration_sec = 0.0;
    t.steps = {s0, s1};

    TourController c;
    c.setCameraGetter([] { TourCameraState k; return k; });
    c.setCameraSetter([](const TourCameraState&) {});

    c.loadTour(t);
    c.start();
    CHECK(c.state() == TourState::Running);

    c.onUserInput();
    CHECK(c.state() == TourState::Paused);

    c.resume();
    CHECK(c.state() == TourState::Running);

    c.onUserInput();
    CHECK(c.state() == TourState::Paused);
    c.stop();
    CHECK(c.state() == TourState::Stopped);
}

// ---- 3. Camera interpolation -----------------------------------------------
void test_camera_interpolation() {
    TourCameraState a; a.azimuth =  0.0; a.elevation =  0.0; a.distance = 100.0;
    TourCameraState b; b.azimuth = 90.0; b.elevation = 30.0; b.distance = 200.0;

    TourCameraState at0 = TourController::interpolateCamera(a, b, 0.0);
    CHECK_NEAR(at0.azimuth,   0.0, 1e-9);
    CHECK_NEAR(at0.elevation, 0.0, 1e-9);
    CHECK_NEAR(at0.distance, 100.0, 1e-9);

    TourCameraState at1 = TourController::interpolateCamera(a, b, 1.0);
    CHECK_NEAR(at1.azimuth,   90.0, 1e-9);
    CHECK_NEAR(at1.elevation, 30.0, 1e-9);
    CHECK_NEAR(at1.distance,  200.0, 1e-9);

    // smoothstep(0.5) == 0.5, so midpoint lerps exactly to the mean
    TourCameraState atHalf = TourController::interpolateCamera(a, b, 0.5);
    CHECK_NEAR(atHalf.azimuth,   45.0, 1e-6);
    CHECK_NEAR(atHalf.elevation, 15.0, 1e-6);
    CHECK_NEAR(atHalf.distance, 150.0, 1e-6);
}

// ---- 4. Azimuth short path --------------------------------------------------
void test_azimuth_short_path() {
    CHECK_NEAR(TourController::shortestAzimuthDelta(350.0,  10.0),  20.0, 1e-9);
    CHECK_NEAR(TourController::shortestAzimuthDelta( 10.0, 350.0), -20.0, 1e-9);
    // Antipodal: +180 and -180 are the same length; accept either sign.
    {
        double d = TourController::shortestAzimuthDelta(0.0, 180.0);
        CHECK(std::fabs(std::fabs(d) - 180.0) < 1e-9);
    }
    CHECK_NEAR(TourController::shortestAzimuthDelta( 90.0,  90.0),   0.0, 1e-9);
    CHECK_NEAR(TourController::shortestAzimuthDelta(359.0,   1.0),   2.0, 1e-9);
}

// ---- 5. Provider lambdas ----------------------------------------------------
void test_provider_lambdas() {
    EducationalTour t;
    t.id = "providers";
    t.title = "Providers";
    TourStep s0; s0.title = "S0"; s0.camera.azimuth = 10.0; s0.camera.distance = 100.0; s0.duration_sec = 0.0;
    TourStep s1; s1.title = "S1"; s1.camera.azimuth = 90.0; s1.camera.distance = 200.0; s1.duration_sec = 0.0;
    t.steps = {s0, s1};

    int setterCalls = 0;
    TourCameraState lastSet;
    int getterCalls = 0;

    TourController c;
    c.setCameraGetter([&] { ++getterCalls; TourCameraState k; return k; });
    c.setCameraSetter([&](const TourCameraState& cam) { ++setterCalls; lastSet = cam; });

    c.loadTour(t);
    c.start();
    CHECK(setterCalls >= 1);
    CHECK_NEAR(lastSet.azimuth,   10.0, 1e-6);
    CHECK_NEAR(lastSet.distance, 100.0, 1e-6);

    int afterStart = setterCalls;
    c.next();
    CHECK(setterCalls > afterStart);
    CHECK_NEAR(lastSet.azimuth,   90.0, 1e-6);
    CHECK_NEAR(lastSet.distance, 200.0, 1e-6);
}

// ---- 6. Action dispatch -----------------------------------------------------
void test_action_dispatch() {
    EducationalTour t;
    t.id = "actions";
    t.title = "Actions";
    TourStep s0;
    s0.title = "S0";
    s0.duration_sec = 0.0;
    s0.actions.push_back(TourAction{"enable_lensing",  nlohmann::json(true)});
    s0.actions.push_back(TourAction{"set_bh_mass",     nlohmann::json(10.0)});
    s0.actions.push_back(TourAction{"set_disk_density",nlohmann::json(2.5)});
    t.steps = {s0};

    std::vector<std::pair<std::string, nlohmann::json>> handled;
    TourController c;
    c.setCameraGetter([] { TourCameraState k; return k; });
    c.setCameraSetter([](const TourCameraState&) {});
    c.setActionHandler([&](const TourAction& a) { handled.push_back({a.type, a.value}); });

    c.loadTour(t);
    c.start();
    CHECK(handled.size() == 3u);
    if (handled.size() == 3u) {
        CHECK(handled[0].first == "enable_lensing");
        CHECK(handled[0].second.get<bool>() == true);
        CHECK(handled[1].first == "set_bh_mass");
        CHECK_NEAR(handled[1].second.get<double>(), 10.0, 1e-9);
        CHECK(handled[2].first == "set_disk_density");
        CHECK_NEAR(handled[2].second.get<double>(), 2.5, 1e-9);
    }
}

} // namespace

int main() {
    test_json_round_trip();
    test_state_machine();
    test_pause_resume();
    test_camera_interpolation();
    test_azimuth_short_path();
    test_provider_lambdas();
    test_action_dispatch();

    std::printf("Educational tests: %d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}