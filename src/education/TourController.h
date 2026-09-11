/**
 * @file TourController.h
 * @brief Runtime state machine for an educational tour.
 *
 * TourController is deliberately decoupled from QmlGlViewport.  It talks to
 * the world through provider lambdas:
 *
 *   - cameraGetter()  -> CameraState     read current camera
 *   - cameraSetter()  -> void            write a target camera
 *   - actionHandler() -> void            dispatch a TourAction to the sim
 *
 * That is the same provider-lambda pattern AnomalyMonitor uses, and it means
 * test_educational.cpp can inject spies and exercise the state machine
 * headlessly -- no Qt, no GL, no GUI event loop.
 *
 * Camera ownership on stop(): the controller does NOT restore the camera on
 * stop().  It leaves the camera wherever the tour last put it (or wherever
 * the user dragged it while paused).  Rationale: the mental model is "the
 * user takes over", so returning to the start point would discard the user's
 * exploration.  Callers that want a reset must do it explicitly.
 */

#ifndef QUANTUMVERSE_TOUR_CONTROLLER_H
#define QUANTUMVERSE_TOUR_CONTROLLER_H

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>

#include <QObject>
#include <QTimer>

#include "EducationalTour.h"

namespace quantumverse {

/**
 * @brief Camera snapshot used by the tour controller.
 *
 * Deliberately named TourCameraState to avoid an ODR clash with
 * ui4d/Camera4DAdapter.h's CameraState (which carries 4D translation
 * offsets).  Four doubles -- no Qt, no QmlGlViewport dependency.
 * Azimuth/elevation are in degrees, distance in geometric units.
 */
struct TourCameraState {
    double azimuth = 45.0;      ///< Degrees, about the vertical axis
    double elevation = 20.0;    ///< Degrees, above the equatorial plane
    double distance = 200.0;    ///< Geometric units from the look-at point
    std::array<double, 3> target = {0.0, 0.0, 0.0};
};

/**
 * @brief States of the tour state machine.
 */
enum class TourState {
    Stopped,    ///< No tour active; camera is the user's
    Running,    ///< Tour active; controller drives the camera
    Paused,     ///< User input interrupted; waiting for Resume
};

/**
 * @brief Drives a single EducationalTour from start to finish.
 *
 * Usage:
 *   TourController controller;
 *   controller.setCameraGetter([&]{ return readCamera(); });
 *   controller.setCameraSetter([&](const CameraState& c){ writeCamera(c); });
 *   controller.setActionHandler([&](const TourAction& a){ dispatch(a); });
 *   controller.loadTour(tour);
 *   controller.start();      // begins at step 0
 *
 * The controller owns a QTimer for auto-advance.  In tests the timer is
 * replaced by explicit advance() calls so the test is deterministic.
 */
class TourController : public QObject {
    Q_OBJECT
public:
    explicit TourController(QObject* parent = nullptr);

    // ----------------------------------------------------------------------
    // Provider lambdas -- decouple the controller from the viewport
    // ----------------------------------------------------------------------

    /**
     * @brief Set the provider that reads the current camera.
     * @return *this for chaining
     */
    TourController& setCameraGetter(std::function<TourCameraState()> getter);

    /**
     * @brief Set the provider that writes a target camera.
     */
    TourController& setCameraSetter(std::function<void(const TourCameraState&)> setter);

    /**
     * @brief Set the provider that dispatches a TourAction to the simulation.
     */
    TourController& setActionHandler(std::function<void(const TourAction&)> handler);

    /**
     * @brief Set the provider called when the tour reaches its end.
     */
    TourController& setOnComplete(std::function<void()> callback);

    /**
     * @brief Set the provider called when the user interrupts the tour.
     */
    TourController& setOnUserInput(std::function<void()> callback);

    // ----------------------------------------------------------------------
    // Tour loading
    // ----------------------------------------------------------------------

    bool loadTour(const EducationalTour& tour);
    bool loadTourFromFile(const std::string& filepath);
    const EducationalTour* currentTour() const { return m_tour.get(); }
    const std::string& currentTourId() const { return m_tourId; }

    // ----------------------------------------------------------------------
    // State machine
    // ----------------------------------------------------------------------

    TourState state() const { return m_state; }
    bool isRunning() const { return m_state == TourState::Running; }
    bool isPaused()  const { return m_state == TourState::Paused; }
    bool isStopped()  const { return m_state == TourState::Stopped; }

    int currentStepIndex() const { return m_stepIndex; }
    const TourStep* currentStep() const;

    void start();        ///< Begin the tour at step 0
    void next();         ///< Advance to the next step (or complete)
    void prev();         ///< Go back to the previous step
    void pause();        ///< Pause due to user input (camera no longer driven)
    void resume();       ///< Resume from Paused at the current step
    void stop();         ///< End the tour; camera is left where it is

    /**
     * @brief Called by the viewport when the user drags / scrolls.
     *
     * If the tour is Running, this pauses it.  The user can resume with
     * resume().  No camera state is changed here -- the user is already
     * controlling the camera.
     */
    void onUserInput();

    /**
     * @brief Advance the auto-advance timer by one step.
     *
     * In production the QTimer calls this; in tests it is called directly so
     * the test controls timing exactly.
     */
    void advance();

    /**
     * @brief Interpolate the camera from `from` to `to` at parameter t in
     * [0, 1].  Azimuth uses shortest-path interpolation.
     *
     * Pure function -- exposed as static so tests can assert on exact values.
     */
    static TourCameraState interpolateCamera(const TourCameraState& from,
                                         const TourCameraState& to,
                                         double t);

    /**
     * @brief Shortest signed angular delta from -> to, in degrees.
     *
     * 350 -> 10 returns +20 (passes through 0), not -340 (passes through 180).
     * Pure function.
     */
    static double shortestAzimuthDelta(double from, double to);

private:
    void applyStepActions(const TourStep& step);
    void gotoStep(int index);

    std::shared_ptr<EducationalTour> m_tour;
    std::string m_tourId;

    int m_stepIndex = -1;

    TourState m_state = TourState::Stopped;

    std::function<TourCameraState()> m_cameraGetter;
    std::function<void(const TourCameraState&)> m_cameraSetter;
    std::function<void(const TourAction&)> m_actionHandler;
    std::function<void()> m_onComplete;
    std::function<void()> m_onUserInput;

    // Auto-advance bookkeeping
    double m_stepElapsed = 0.0;   ///< Seconds spent on the current step
    double m_stepDuration = 0.0;  ///< Target duration for the current step

    QTimer* m_timer = nullptr;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_TOUR_CONTROLLER_H