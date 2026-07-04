/**
 * @file Camera4DAdapter.h
 * @brief Camera4D adapter bridging QML input to 4D navigation
 *
 * Maps mouse/keyboard/touch events to Camera4D rotations in
 * the 6 orthogonal planes of SO(4), with smooth interpolation
 * and configurable rotation-plane selection via UI toolbar.
 *
 * References:
 * - plan9.md Phase 1: Core Viewport & Navigation
 */

#ifndef CAMERA4DADAPTER_H
#define CAMERA4DADAPTER_H

#include <string>
#include <memory>
#include <array>
#include <cmath>

#ifdef QUANTUMVERSE_USE_QML
#include <QObject>
#include <QVector3D>
#include <QMatrix4x4>
#endif

// Ensure M_PI is available on all platforms (MSVC <cmath> may not define it)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {
class Camera4D;
class Event4D;
}

namespace quantumverse {

/**
 * @brief Rotation plane identifiers for SO(4) navigation
 *
 * Six orthogonal rotation planes in 4D spacetime:
 * - TX: time-x (boost-like)
 * - TY: time-y
 * - TZ: time-z
 * - XY: spatial (standard azimuth/elevation)
 * - XZ: spatial
 * - YZ: spatial
 */
enum class RotationPlane {
    XY,  ///< Horizontal orbit (azimuth/elevation) - default
    XZ,  ///< Vertical tilt plane
    YZ,  ///< Side tilt plane
    TX,  ///< Time-x boost
    TY,  ///< Time-y boost
    TZ   ///< Time-z boost
};

/**
 * @brief Interpolates between two 4D camera states
 *
 * Uses spherical linear interpolation (slerp) for rotation
 * and linear interpolation for position/distance.
 */
struct CameraState {
    double distance;
    double azimuth;    // radians
    double elevation;  // radians
    double tx, ty, tz; // 4D translation offsets

    CameraState() : distance(50.0), azimuth(0.0), elevation(0.3),
                    tx(0.0), ty(0.0), tz(0.0) {}

    static CameraState lerp(const CameraState& a, const CameraState& b, double t) {
        CameraState result;
        result.distance = a.distance + (b.distance - a.distance) * t;
        // Slerp for azimuth/elevation
        double dot = std::cos(a.azimuth) * std::cos(b.azimuth) +
                     std::sin(a.azimuth) * std::sin(b.azimuth) *
                     std::cos(a.elevation) * std::cos(b.elevation) +
                     std::sin(a.elevation) * std::sin(b.elevation);
        dot = std::max(-1.0, std::min(1.0, dot));
        double omega = std::acos(dot);

        if (std::abs(omega) < 1e-6) {
            result.azimuth = a.azimuth + (b.azimuth - a.azimuth) * t;
            result.elevation = a.elevation + (b.elevation - a.elevation) * t;
        } else {
            double s0 = std::sin((1.0 - t) * omega) / std::sin(omega);
            double s1 = std::sin(t * omega) / std::sin(omega);
            result.azimuth = a.azimuth * s0 + b.azimuth * s1;
            result.elevation = a.elevation * s0 + b.elevation * s1;
        }

        result.tx = a.tx + (b.tx - a.tx) * t;
        result.ty = a.ty + (b.ty - a.ty) * t;
        result.tz = a.tz + (b.tz - a.tz) * t;
        return result;
    }
};

#ifdef QUANTUMVERSE_USE_QML

/**
 * @brief Adapter mapping QML input events to Camera4D SO(4) rotations
 *
 * Provides:
 * - 6-plane rotation selection via toolbar
 * - Smooth orbit/pan/zoom with inertia
 * - 4D slice advance via keyboard/mouse wheel modifiers
 * - Double-speed modifier (Shift) for fast navigation
 */
class Camera4DAdapter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(float distance READ distance WRITE setDistance NOTIFY distanceChanged)
    Q_PROPERTY(float azimuth READ azimuth WRITE setAzimuth NOTIFY azimuthChanged)
    Q_PROPERTY(float elevation READ elevation WRITE setElevation NOTIFY elevationChanged)
    Q_PROPERTY(int activePlane READ activePlane WRITE setActivePlane NOTIFY activePlaneChanged)
    Q_PROPERTY(bool isAnimating READ isAnimating NOTIFY isAnimatingChanged)
    Q_PROPERTY(double animationSpeed READ animationSpeed WRITE setAnimationSpeed NOTIFY animationSpeedChanged)
    Q_PROPERTY(float fieldOfView READ fieldOfView WRITE setFieldOfView NOTIFY fieldOfViewChanged)

public:
    explicit Camera4DAdapter(QObject* parent = nullptr);
    ~Camera4DAdapter() override;

    // Current camera state
    float distance() const { return static_cast<float>(m_state.distance); }
    float azimuth() const { return static_cast<float>(m_state.azimuth); }
    float elevation() const { return static_cast<float>(m_state.elevation); }
    int activePlane() const { return static_cast<int>(m_activePlane); }
    bool isAnimating() const { return m_isAnimating; }
    double animationSpeed() const { return m_animationSpeed; }
    float fieldOfView() const { return m_fieldOfView; }

    // Get computed view matrix for the QML viewport
    QMatrix4x4 viewMatrix() const;

    // Get 4D camera position and target
    Event4D cameraPosition4D() const;
    Event4D cameraTarget4D() const;

    // Reset to default view
    Q_INVOKABLE void reset();

    // Focus on a 4D position
    Q_INVOKABLE void focusOn(const QVector3D& position);

    // Smooth transition to a new camera state
    Q_INVOKABLE void transitionTo(const CameraState& target, double durationMs = 500.0);

    // 4D rotation in selected plane
    Q_INVOKABLE void rotateInPlane(double angle);

    // Advance 4D slice (time or spatial)
    Q_INVOKABLE void advanceSlice(double delta);

signals:
     void distanceChanged();
     void azimuthChanged();
     void elevationChanged();
     void activePlaneChanged();
     void isAnimatingChanged();
     void animationSpeedChanged();
     void fieldOfViewChanged();
     void viewChanged();

public slots:
     void setDistance(float dist);
     void setAzimuth(float az);
     void setElevation(float el);
     void setActivePlane(int planeIndex);
     void setAnimationSpeed(double speed);
     void setFieldOfView(float fov);

     // Cycle through rotation planes (Q key)
     Q_INVOKABLE void cyclePlane();

     // Input handlers called from QML
    Q_INVOKABLE void onMousePressed(float x, float y, int button, int modifiers);
    Q_INVOKABLE void onMouseMoved(float x, float y, int buttons, int modifiers);
    Q_INVOKABLE void onMouseReleased(float x, float y, int button);
    Q_INVOKABLE void onWheelDelta(float delta, int modifiers);
    Q_INVOKABLE void onKeyPressed(int key, int modifiers);
    Q_INVOKABLE void onPinchDelta(float scale);

     // Animation tick (call from QML Timer or render loop)
    void onFrame(double deltaTime);

private:
    // Apply current state to Camera4D
    void applyState();

    // Clamp elevation to avoid gimbal lock
    void clampElevation();

    // Rotation plane state
    RotationPlane m_activePlane;
    CameraState m_state;
    CameraState m_targetState;  // For smooth transitions

    // Interaction state
    bool m_isDragging;
    bool m_isAnimating;
    float m_lastMouseX;
    float m_lastMouseY;
    int m_activeButton;
    double m_animationSpeed;  ///< Multiplier for all animations
    double m_transitionTime;  ///< Remaining transition time in ms
    double m_transitionDuration; ///< Total transition duration in ms

    // Field of view (perspective projection)
    float m_fieldOfView = 45.0f;

    // Sensitivity
    static constexpr double ROTATION_SPEED = 0.005;
    static constexpr double PAN_SPEED = 0.01;
    static constexpr double ZOOM_SPEED = 0.001;
    static constexpr double SLICE_SPEED = 0.1;
    static constexpr double DOLLY_SPEED = 0.5;
    static constexpr double TRANSITION_SPEED = 3.0; // units per second
};

#else // QUANTUMVERSE_USE_QML not defined - stub version

/**
 * @brief Stub Camera4DAdapter (Qt unavailable)
 *
 * Minimal stub providing the same public interface when Qt6 is not available.
 */
class Camera4DAdapter
{
public:
    explicit Camera4DAdapter(void* parent = nullptr)
        : m_activePlane(RotationPlane::XY)
        , m_isDragging(false)
        , m_isAnimating(false)
        , m_lastMouseX(0.0f)
        , m_lastMouseY(0.0f)
        , m_activeButton(0)
        , m_animationSpeed(1.0)
        , m_transitionTime(0.0)
        , m_transitionDuration(0.0)
        , m_fieldOfView(45.0f)
    {}

    ~Camera4DAdapter() = default;

    float distance() const { return static_cast<float>(m_state.distance); }
    float azimuth() const { return static_cast<float>(m_state.azimuth); }
    float elevation() const { return static_cast<float>(m_state.elevation); }
    int activePlane() const { return static_cast<int>(m_activePlane); }
    bool isAnimating() const { return m_isAnimating; }
    double animationSpeed() const { return m_animationSpeed; }
    float fieldOfView() const { return m_fieldOfView; }

    void reset() { transitionTo(CameraState()); }
    void transitionTo(const CameraState& target, double durationMs = 500.0) {
        m_targetState = target;
        m_transitionDuration = durationMs;
        m_transitionTime = durationMs;
        m_isAnimating = true;
    }
    void rotateInPlane(double /*angle*/) {}
    void advanceSlice(double /*delta*/) {}
    void setDistance(float dist) { m_state.distance = std::max(0.1, std::min(static_cast<double>(dist), 10000.0)); }
    void setAzimuth(float az) { m_state.azimuth = az; }
    void setElevation(float el) { m_state.elevation = el; clampElevation(); }
    void setActivePlane(int planeIndex) {
        if (planeIndex >= 0 && planeIndex <= 5)
            m_activePlane = static_cast<RotationPlane>(planeIndex);
    }
    void setAnimationSpeed(double speed) { m_animationSpeed = std::max(0.1, std::min(speed, 10.0)); }
    void setFieldOfView(float fov) { m_fieldOfView = std::max(10.0f, std::min(fov, 120.0f)); }
    void cyclePlane() { setActivePlane((activePlane() + 1) % 6); }
    void onMousePressed(float, float, int, int) {}
    void onMouseMoved(float, float, int, int) {}
    void onMouseReleased(float, float, int) {}
    void onWheelDelta(float, int) {}
    void onKeyPressed(int, int) {}
    void onPinchDelta(float) {}
    void onFrame(double) {}

private:
    void applyState() {}
    void clampElevation() {
        const double halfPi = 1.57079632679;
        m_state.elevation = std::max(-halfPi + 0.01, std::min(halfPi - 0.01, m_state.elevation));
    }

    RotationPlane m_activePlane;
    CameraState m_state;
    CameraState m_targetState;
    bool m_isDragging;
    bool m_isAnimating;
    float m_lastMouseX;
    float m_lastMouseY;
    int m_activeButton;
    double m_animationSpeed;
    double m_transitionTime;
    double m_transitionDuration;
    float m_fieldOfView;
};

#endif // QUANTUMVERSE_USE_QML

} // namespace quantumverse

#endif // CAMERA4DADAPTER_H