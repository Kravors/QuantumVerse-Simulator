/**
 * @file TourController.cpp
 * @brief TourController implementation.
 *
 * The camera interpolation math (interpolateCamera / shortestAzimuthDelta) is
 * pure and lives in the header as statics so tests can assert on exact
 * values without instantiating the controller.
 */

#include "TourController.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

// ============================================================================
// Pure camera math
// ============================================================================

double TourController::shortestAzimuthDelta(double from, double to) {
    // Bring the difference into (-180, 180] via a 360-degree window centred on
    // from + 540.  Adding 540 shifts the window to (180, 540); fmod 360 maps it
    // back to (0, 360); subtracting 180 lands it in (-180, 180].
    double d = std::fmod(to - from + 540.0, 360.0) - 180.0;
    // fmod can return -0.0; normalise to a plain 0.0 for stable comparisons.
    if (std::abs(d) < 1e-12) d = 0.0;
    // Antipodal boundary: when from and to are exactly 180 deg apart, the
    // shortest path is not unique -- +180 and -180 both traverse a
    // hemisphere and reach the same endpoint.  The sign here is an
    // implementation artefact of the fmod window; callers must not depend on
    // it.  interpolateCamera() is unaffected because the two hemispheres
    // converge to the same target at t == 1.
    return d;
}

TourCameraState TourController::interpolateCamera(const TourCameraState& from,
                                              const TourCameraState& to,
                                              double t) {
    t = std::clamp(t, 0.0, 1.0);
    // Smoothstep for eased motion (ease-in-out).
    double smooth = t * t * (3.0 - 2.0 * t);

    TourCameraState out;
    out.azimuth   = from.azimuth   + shortestAzimuthDelta(from.azimuth, to.azimuth) * smooth;
    out.elevation = from.elevation + (to.elevation - from.elevation) * smooth;
    out.distance  = from.distance  + (to.distance  - from.distance)  * smooth;
    for (int i = 0; i < 3; i++) {
        out.target[i] = from.target[i] + (to.target[i] - from.target[i]) * smooth;
    }
    return out;
}

// ============================================================================
// Construction / providers
// ============================================================================

TourController::TourController(QObject* parent)
    : QObject(parent) {
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &TourController::advance);
}

TourController& TourController::setCameraGetter(std::function<TourCameraState()> getter) {
    m_cameraGetter = std::move(getter);
    return *this;
}

TourController& TourController::setCameraSetter(std::function<void(const TourCameraState&)> setter) {
    m_cameraSetter = std::move(setter);
    return *this;
}

TourController& TourController::setActionHandler(std::function<void(const TourAction&)> handler) {
    m_actionHandler = std::move(handler);
    return *this;
}

TourController& TourController::setOnComplete(std::function<void()> callback) {
    m_onComplete = std::move(callback);
    return *this;
}

TourController& TourController::setOnUserInput(std::function<void()> callback) {
    m_onUserInput = std::move(callback);
    return *this;
}

// ============================================================================
// Tour loading
// ============================================================================

bool TourController::loadTour(const EducationalTour& tour) {
    if (!tour.validate()) {
        std::cerr << "[TourController] Refusing to load invalid tour" << std::endl;
        return false;
    }
    m_tour = std::make_shared<EducationalTour>(tour);
    m_tourId = tour.id;
    if (m_state == TourState::Running || m_state == TourState::Paused) stop();
    m_stepIndex = -1;
    m_stepElapsed = 0.0;
    m_stepDuration = 0.0;
    return true;
}

bool TourController::loadTourFromFile(const std::string& filepath) {
    EducationalTour tour;
    if (!tour.loadFromFile(filepath)) {
        return false;
    }
    return loadTour(tour);
}

const TourStep* TourController::currentStep() const {
    if (!m_tour || m_stepIndex < 0 ||
        static_cast<size_t>(m_stepIndex) >= m_tour->steps.size()) {
        return nullptr;
    }
    return &m_tour->steps[m_stepIndex];
}

// ============================================================================
// State machine
// ============================================================================

void TourController::applyStepActions(const TourStep& step) {
    if (!m_actionHandler) return;
    for (const auto& a : step.actions) {
        m_actionHandler(a);
    }
}

void TourController::gotoStep(int index) {
    if (!m_tour) return;
    if (index < 0 || static_cast<size_t>(index) >= m_tour->steps.size()) {
        // Past the end -> complete.
        stop();
        if (m_onComplete) m_onComplete();
        return;
    }

    m_stepIndex = index;
    const TourStep& step = m_tour->steps[index];

    // Move the camera to the step's target.
    if (m_cameraSetter) {
        TourCameraState target;
        target.azimuth   = step.camera.azimuth;
        target.elevation = step.camera.elevation;
        target.distance  = step.camera.distance;
        target.target    = step.camera.target;
        m_cameraSetter(target);
    }

    // Apply the step's state mutations.
    applyStepActions(step);

    // Duration: 0 means "wait for the user".
    m_stepDuration = step.duration_sec;
    m_stepElapsed = 0.0;

    if (m_stepDuration > 0.0) {
        m_timer->start(static_cast<int>(m_stepDuration * 1000.0));
    }
}

void TourController::start() {
    if (!m_tour) {
        std::cerr << "[TourController] Cannot start: no tour loaded" << std::endl;
        return;
    }
    m_state = TourState::Running;
    gotoStep(0);
}

void TourController::next() {
    if (m_state == TourState::Stopped) return;
    if (m_state == TourState::Paused) m_state = TourState::Running;
    m_timer->stop();
    gotoStep(m_stepIndex + 1);
}

void TourController::prev() {
    if (m_state == TourState::Stopped) return;
    if (m_state == TourState::Paused) m_state = TourState::Running;
    m_timer->stop();
    // Clamp at the first step: going before step 0 is a no-op, not a
    // completion.  The tour is a linear script; there is nowhere to go.
    if (m_stepIndex <= 0) return;
    gotoStep(m_stepIndex - 1);
}

void TourController::pause() {
    if (m_state != TourState::Running) return;
    m_state = TourState::Paused;
    m_timer->stop();
    if (m_onUserInput) m_onUserInput();
}

void TourController::resume() {
    if (m_state != TourState::Paused) return;
    m_state = TourState::Running;
    if (m_stepDuration > 0.0) {
        m_timer->start(static_cast<int>((m_stepDuration - m_stepElapsed) * 1000.0));
    }
}

void TourController::stop() {
    m_timer->stop();
    m_state = TourState::Stopped;
    m_stepIndex = -1;
    m_stepElapsed = 0.0;
    m_stepDuration = 0.0;
    // Camera ownership: deliberately NOT restored.  The controller leaves the
    // camera where the tour last put it, so the user keeps what they explored
    // while paused.  Callers that want a reset must do it explicitly.
}

void TourController::onUserInput() {
    if (m_state == TourState::Running) {
        pause();
    }
}

void TourController::advance() {
    if (m_state != TourState::Running) return;
    next();
}

} // namespace quantumverse