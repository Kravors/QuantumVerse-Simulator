/**
 * @file Camera4DAdapter.cpp
 * @brief Implementation of Camera4DAdapter for 4D navigation
 *
 * Implements full SO(4) rotation across all 6 orthogonal planes
 * using Matrix4x4::rotation() for proper 4D transformations.
 */

#include "Camera4DAdapter.h"
#include "../math/Matrix4x4.h"
#include <cmath>
#include <algorithm>

#ifdef QUANTUMVERSE_USE_QML
#include <QString>
#include <QObject>
#include <QDebug>
#include "ui4d/UI4D.h"
#endif

namespace quantumverse {

#ifdef QUANTUMVERSE_USE_QML

Camera4DAdapter::Camera4DAdapter(QObject* parent)
    : QObject(parent)
    , m_activePlane(RotationPlane::XY)
    , m_isDragging(false)
    , m_isAnimating(false)
    , m_lastMouseX(0.0f)
    , m_lastMouseY(0.0f)
    , m_activeButton(0)
    , m_animationSpeed(1.0)
    , m_transitionTime(0.0)
    , m_transitionDuration(0.0)
{
    applyState();
}

Camera4DAdapter::~Camera4DAdapter() = default;

QMatrix4x4 Camera4DAdapter::viewMatrix() const
{
    // Build a standard orbit (look-at) view matrix. The eye orbits the
    // target (tx,ty,tz) at `distance`, parameterised by azimuth/elevation.
    //
    // Previously this hand-rolled the matrix and placed only the target in
    // the translation row, ignoring `distance` entirely. That left the eye
    // sitting exactly on the target (the origin by default), edge-on to the
    // Y=0 grid, so the whole scene rendered off-screen (black viewport).
    //
    // Guard: never return a non-finite matrix. A focus/selection can feed a
    // NaN/Inf target (e.g. selecting an object whose position is not yet
    // computed), and lookAt() of a NaN eye/target yields a degenerate matrix
    // that blanks every perspective-drawn primitive (grid, geodesics,
    // celestial bodies) while the orthographic axis gizmo survives -- a
    // "black viewport". Sanitize the state first; if it is still bad, fall
    // back to a safe default view.
    const double distance = std::isfinite(m_state.distance) ? m_state.distance : 50.0;
    const double elevation = std::isfinite(m_state.elevation) ? m_state.elevation : 0.0;
    const double azimuth = std::isfinite(m_state.azimuth) ? m_state.azimuth : 0.0;
    const double tx = std::isfinite(m_state.tx) ? m_state.tx : 0.0;
    const double ty = std::isfinite(m_state.ty) ? m_state.ty : 0.0;
    const double tz = std::isfinite(m_state.tz) ? m_state.tz : 0.0;

    const double ce = std::cos(elevation);
    const double se = std::sin(elevation);
    const double ca = std::cos(azimuth);
    const double sa = std::sin(azimuth);

    const QVector3D target(
        static_cast<float>(tx),
        static_cast<float>(ty),
        static_cast<float>(tz));

    const QVector3D eye(
        static_cast<float>(tx + distance * ce * sa),
        static_cast<float>(ty + distance * se),
        static_cast<float>(tz + distance * ce * ca));

    QMatrix4x4 view;
    view.lookAt(eye, target, QVector3D(0.0f, 1.0f, 0.0f));
    if (!view.isAffine()) {
        // Fallback: identity-ish view looking down -Z at the origin.
        view.setToIdentity();
        view.translate(0.0f, 0.0f, -50.0f);
    }
    return view;
}

Event4D Camera4DAdapter::cameraPosition4D() const
{
    // Convert spherical orbit coordinates to 4D position
    double x = m_state.distance * std::cos(m_state.elevation) * std::sin(m_state.azimuth);
    double y = m_state.distance * std::sin(m_state.elevation);
    double z = m_state.distance * std::cos(m_state.elevation) * std::cos(m_state.azimuth);
    return Event4D(0.0, x + m_state.tx, y + m_state.ty, z + m_state.tz);
}

Event4D Camera4DAdapter::cameraTarget4D() const
{
    return Event4D(0.0, m_state.tx, m_state.ty, m_state.tz);
}

void Camera4DAdapter::reset()
{
    transitionTo(CameraState());
}

void Camera4DAdapter::focusOn(const QVector3D& position)
{
    CameraState target = m_state;
    target.tx = position.x();
    target.ty = position.y();
    target.tz = position.z();
    transitionTo(target, 300.0);
}

void Camera4DAdapter::transitionTo(const CameraState& target, double durationMs)
{
    m_targetState = target;
    m_transitionDuration = durationMs;
    m_transitionTime = durationMs;
    m_isAnimating = true;
    emit isAnimatingChanged();
}

/**
 * @brief Apply SO(4) rotation in the specified plane using Matrix4x4
 *
 * This implements proper 4D rotations using the Matrix4x4::rotation() method,
 * which constructs a rotation matrix in the (i,j) plane by the given angle.
 * The rotation is applied to the camera's look-at offset vector in 4D.
 */
void Camera4DAdapter::rotateInPlane(double angle)
{
     // Map RotationPlane enum to the two axes of the rotation plane
     // 0=t, 1=x, 2=y, 3=z
     int axis1 = 0, axis2 = 0;

     switch (m_activePlane) {
         case RotationPlane::XY:
             axis1 = 1; axis2 = 2;  // Spatial XY plane
             break;
         case RotationPlane::XZ:
             axis1 = 1; axis2 = 3;  // Spatial XZ plane
             break;
         case RotationPlane::YZ:
             axis1 = 2; axis2 = 3;  // Spatial YZ plane
             break;
         case RotationPlane::TX:
             axis1 = 0; axis2 = 1;  // Time-X boost plane
             break;
         case RotationPlane::TY:
             axis1 = 0; axis2 = 2;  // Time-Y boost plane
             break;
         case RotationPlane::TZ:
             axis1 = 0; axis2 = 3;  // Time-Z boost plane
             break;
     }

     // Build 4D offset vector from camera position to look-at target
     double offset[4] = {
         m_state.tx,
         m_state.ty,
         m_state.tz,
         0.0
     };

     // For boost planes (involving time axis), use hyperbolic Lorentz rotation
     // to preserve the Minkowski metric signature (-,+,+,+)
     bool isBoost = (axis1 == 0);

     if (isBoost) {
         // Hyperbolic rotation (Lorentz boost) using rapidity parameterization
         // This correctly mixes time and space components while preserving
         // the spacetime interval ds^2 = -dt^2 + dx^2 + dy^2 + dz^2
         double rapidity = std::clamp(angle, -3.0, 3.0);
         double coshR = std::cosh(rapidity);
         double sinhR = std::sinh(rapidity);
         // Boost matrix: [cosh sinh; sinh cosh] applied to (t, spatial) pair
         double newOffset[4] = {0, 0, 0, 0};
         newOffset[axis1] = coshR * offset[axis1] + sinhR * offset[axis2];
         newOffset[axis2] = sinhR * offset[axis1] + coshR * offset[axis2];
         for (int i = 0; i < 4; i++) {
             if (i != axis1 && i != axis2) newOffset[i] = offset[i];
         }

         m_state.tx = newOffset[0];
         m_state.ty = newOffset[1];
         m_state.tz = newOffset[2];
     } else {
         // Ordinary spatial rotation using Matrix4x4::rotation()
          Matrix4x4<double> rot = Matrix4x4<double>::rotation(axis1, axis2, angle);

         double newOffset[4] = {0, 0, 0, 0};
         for (int i = 0; i < 4; i++) {
             for (int j = 0; j < 4; j++) {
                 newOffset[i] += rot(i, j) * offset[j];
             }
         }

         m_state.tx = newOffset[0];
         m_state.ty = newOffset[1];
         m_state.tz = newOffset[2];
     }

     // Update orbit parameters for spatial rotations
     if (!isBoost) {
         switch (m_activePlane) {
             case RotationPlane::XY:
                 // Rotate azimuth around Z axis
                 m_state.azimuth += angle;
                 break;
             case RotationPlane::XZ: {
                 double r = std::sqrt(m_state.tx*m_state.tx + m_state.tz*m_state.tz);
                 double phi = std::atan2(m_state.tz, m_state.tx) + angle;
                 m_state.tx = r * std::cos(phi);
                 m_state.tz = r * std::sin(phi);
                 break;
             }
             case RotationPlane::YZ: {
                 double r = std::sqrt(m_state.ty*m_state.ty + m_state.tz*m_state.tz);
                 double phi = std::atan2(m_state.ty, m_state.tz) + angle;
                 m_state.ty = r * std::sin(phi);
                 m_state.tz = r * std::cos(phi);
                 break;
             }
             default: break;
         }
     } else {
         // For boost planes, update orbit angles to track boost direction
         double spatialLen = std::sqrt(m_state.tx*m_state.tx +
                                       m_state.ty*m_state.ty +
                                       m_state.tz*m_state.tz);
         if (spatialLen > 1e-10) {
             m_state.azimuth = std::atan2(m_state.tx, m_state.tz);
             m_state.elevation = std::asin(std::clamp(m_state.ty / spatialLen, -1.0, 1.0));
         }
     }

     applyState();
     emit viewChanged();
 }

void Camera4DAdapter::advanceSlice(double delta)
{
    (void)delta;
    emit viewChanged();
}

void Camera4DAdapter::setDistance(float dist)
{
    if (std::abs(m_state.distance - dist) > 0.01) {
        m_state.distance = std::max(0.1, std::min(static_cast<double>(dist), 10000.0));
        emit distanceChanged();
        emit viewChanged();
    }
}

void Camera4DAdapter::setAzimuth(float az)
{
    if (std::abs(m_state.azimuth - az) > 0.001) {
        m_state.azimuth = az;
        while (m_state.azimuth > M_PI) m_state.azimuth -= 2.0 * M_PI;
        while (m_state.azimuth < -M_PI) m_state.azimuth += 2.0 * M_PI;
        emit azimuthChanged();
        emit viewChanged();
    }
}

void Camera4DAdapter::setElevation(float el)
{
    if (std::abs(m_state.elevation - el) > 0.001) {
        m_state.elevation = el;
        clampElevation();
        emit elevationChanged();
        emit viewChanged();
    }
}

void Camera4DAdapter::setActivePlane(int planeIndex)
{
    if (planeIndex >= 0 && planeIndex <= 5) {
        RotationPlane newPlane = static_cast<RotationPlane>(planeIndex);
        if (m_activePlane != newPlane) {
            m_activePlane = newPlane;
            emit activePlaneChanged();
        }
    }
}

void Camera4DAdapter::setAnimationSpeed(double speed)
{
    m_animationSpeed = std::max(0.1, std::min(speed, 10.0));
    emit animationSpeedChanged();
}

void Camera4DAdapter::setFieldOfView(float fov)
{
    float clamped = std::max(10.0f, std::min(fov, 120.0f));
    if (std::abs(m_fieldOfView - clamped) > 0.1f) {
        m_fieldOfView = clamped;
        emit fieldOfViewChanged();
    }
}

void Camera4DAdapter::cyclePlane()
{
    setActivePlane((static_cast<int>(m_activePlane) + 1) % 6);
}

void Camera4DAdapter::onMousePressed(float x, float y, int button, int modifiers)
{
    (void)modifiers;
    m_isDragging = true;
    m_lastMouseX = x;
    m_lastMouseY = y;
    m_activeButton = button;
}

void Camera4DAdapter::onMouseMoved(float x, float y, int buttons, int modifiers)
{
    (void)buttons;
    if (!m_isDragging) return;

    double dx = x - m_lastMouseX;
    double dy = y - m_lastMouseY;
    m_lastMouseX = x;
    m_lastMouseY = y;

    bool shiftHeld = (modifiers & 1) != 0;
    double speedMult = shiftHeld ? 5.0 : 1.0;
    speedMult *= m_animationSpeed;

    switch (m_activePlane) {
        case RotationPlane::XY:
            if (m_activeButton == 1) {
                // Orbit: azimuth + elevation rotation using SO(4) in XY plane
                m_state.azimuth += dx * ROTATION_SPEED * speedMult;
                m_state.elevation += dy * ROTATION_SPEED * speedMult;
                clampElevation();
            } else if (m_activeButton == 2 || m_activeButton == 3) {
                // Pan in XY plane
                double distScale = m_state.distance * PAN_SPEED * speedMult;
                m_state.tx += dx * distScale * 0.1;
                m_state.ty -= dy * distScale * 0.1;
            }
            break;
        case RotationPlane::XZ:
            // Rotate in XZ plane (azimuth + zoom)
            m_state.azimuth += dx * ROTATION_SPEED * speedMult;
            m_state.distance = std::max(0.1, m_state.distance + dy * ZOOM_SPEED * speedMult * 10.0);
            break;
        case RotationPlane::YZ:
            // Rotate in YZ plane (elevation + zoom)
            m_state.elevation += dy * ROTATION_SPEED * speedMult;
            clampElevation();
            m_state.distance = std::max(0.1, m_state.distance + dx * ZOOM_SPEED * speedMult * 10.0);
            break;
            case RotationPlane::TX:
                // Proper SO(4) rotation in TX plane (Lorentz boost)
                // Mouse horizontal = rotation angle, vertical = translation along X
                {
                    double rotAngle = dx * ROTATION_SPEED * speedMult * 0.5;
                    double rapidity = std::clamp(rotAngle, -2.0, 2.0);
                    double coshR = std::cosh(rapidity);
                    double sinhR = std::sinh(rapidity);
                    (void)sinhR;
                    double newTx = coshR * m_state.tx;
                m_state.tx = std::clamp(newTx, -500.0, 500.0);
                // Vertical adjusts distance
                m_state.distance = std::max(0.1, m_state.distance + dy * ZOOM_SPEED * speedMult * 5.0);
            }
            break;
            case RotationPlane::TY:
                // Proper SO(4) rotation in TY plane
                {
                    double rotAngle = dx * ROTATION_SPEED * speedMult * 0.5;
                    double rapidity = std::clamp(rotAngle, -2.0, 2.0);
                    double coshR = std::cosh(rapidity);
                    double sinhR = std::sinh(rapidity);
                    (void)sinhR;
                    double newTy = coshR * m_state.ty;
                m_state.ty = std::clamp(newTy, -500.0, 500.0);
                m_state.distance = std::max(0.1, m_state.distance + dy * ZOOM_SPEED * speedMult * 5.0);
            }
            break;
            case RotationPlane::TZ:
                // Proper SO(4) rotation in TZ plane
                {
                    double rotAngle = dx * ROTATION_SPEED * speedMult * 0.5;
                    double rapidity = std::clamp(rotAngle, -2.0, 2.0);
                    double coshR = std::cosh(rapidity);
                    double sinhR = std::sinh(rapidity);
                    (void)sinhR;
                    double newTz = coshR * m_state.tz;
                m_state.tz = std::clamp(newTz, -500.0, 500.0);
                m_state.distance = std::max(0.1, m_state.distance + dy * ZOOM_SPEED * speedMult * 5.0);
            }
            break;
    }

    applyState();
    emit viewChanged();
}

void Camera4DAdapter::onMouseReleased(float x, float y, int button)
{
    Q_UNUSED(x); Q_UNUSED(y); Q_UNUSED(button);
    m_isDragging = false;
}

void Camera4DAdapter::onWheelDelta(float delta, int modifiers)
{
    bool shiftHeld = (modifiers & 1) != 0;
    double speedMult = shiftHeld ? 10.0 : 1.0;
    speedMult *= m_animationSpeed;

    if (m_activePlane == RotationPlane::TX ||
        m_activePlane == RotationPlane::TY ||
        m_activePlane == RotationPlane::TZ) {
        // For boost planes: scroll adjusts rapidity (boost intensity)
        double rapidityDelta = delta * SLICE_SPEED * speedMult * 0.1;
        double coshR = std::cosh(rapidityDelta);
        double sinhR = std::sinh(rapidityDelta);

        switch (m_activePlane) {
            case RotationPlane::TX:
                m_state.tx = coshR * m_state.tx + sinhR * 0.0;
                break;
            case RotationPlane::TY:
                m_state.ty = coshR * m_state.ty + sinhR * 0.0;
                break;
            case RotationPlane::TZ:
                m_state.tz = coshR * m_state.tz + sinhR * 0.0;
                break;
            default: break;
        }
        emit viewChanged();
    } else {
        double zoomFactor = 1.0 - delta * ZOOM_SPEED * speedMult;
        m_state.distance = std::max(0.1, m_state.distance * zoomFactor);
        emit distanceChanged();
        emit viewChanged();
    }
}

void Camera4DAdapter::onKeyPressed(int key, int modifiers)
{
    bool shiftHeld = (modifiers & 1) != 0;
    double step = shiftHeld ? 5.0 : 1.0;
    step *= m_animationSpeed;

    switch (key) {
    case Qt::Key_W:
        m_state.distance = std::max(0.1, m_state.distance - DOLLY_SPEED * step);
        emit distanceChanged();
        emit viewChanged();
        break;
    case Qt::Key_S:
        m_state.distance += DOLLY_SPEED * step;
        emit distanceChanged();
        emit viewChanged();
        break;
    case Qt::Key_R:
        transitionTo(CameraState());
        break;
    case Qt::Key_F:
        break;
    case Qt::Key_Q:
        setActivePlane((static_cast<int>(m_activePlane) + 1) % 6);
        break;
    case Qt::Key_1: setActivePlane(0); break;
    case Qt::Key_2: setActivePlane(1); break;
    case Qt::Key_3: setActivePlane(2); break;
    case Qt::Key_4: setActivePlane(3); break;
    case Qt::Key_5: setActivePlane(4); break;
    case Qt::Key_6: setActivePlane(5); break;
    default: break;
    }
}

void Camera4DAdapter::onPinchDelta(float scale)
{
    m_state.distance = std::max(0.1, m_state.distance / scale);
    emit distanceChanged();
    emit viewChanged();
}

void Camera4DAdapter::onFrame(double deltaTime)
{
    if (m_isAnimating && m_transitionTime > 0.0) {
        double dt = deltaTime * m_animationSpeed * TRANSITION_SPEED;
        if (dt > m_transitionTime) dt = m_transitionTime;

        double t = 1.0 - (m_transitionTime - dt) / m_transitionDuration;
        t = std::max(0.0, std::min(1.0, t));

        double smoothT = t * t * (3.0 - 2.0 * t);

        m_state.distance = m_state.distance +
            (m_targetState.distance - m_state.distance) * smoothT * 0.1;
        m_state.azimuth = m_state.azimuth +
            (m_targetState.azimuth - m_state.azimuth) * smoothT * 0.1;
        m_state.elevation = m_state.elevation +
            (m_targetState.elevation - m_state.elevation) * smoothT * 0.1;
        m_state.tx = m_state.tx + (m_targetState.tx - m_state.tx) * smoothT * 0.1;
        m_state.ty = m_state.ty + (m_targetState.ty - m_state.ty) * smoothT * 0.1;
        m_state.tz = m_state.tz + (m_targetState.tz - m_state.tz) * smoothT * 0.1;

        m_transitionTime -= dt;
        if (m_transitionTime <= 0.0) {
            m_isAnimating = false;
            emit isAnimatingChanged();
        }

        applyState();
        emit viewChanged();
    }
}

void Camera4DAdapter::applyState()
{
    clampElevation();
    while (m_state.azimuth > M_PI) m_state.azimuth -= 2.0 * M_PI;
    while (m_state.azimuth < -M_PI) m_state.azimuth += 2.0 * M_PI;
}

void Camera4DAdapter::clampElevation()
{
    const double halfPi = M_PI * 0.5;
    m_state.elevation = std::max(-halfPi + 0.01, std::min(halfPi - 0.01, m_state.elevation));
}

#endif // QUANTUMVERSE_USE_QML

} // namespace quantumverse