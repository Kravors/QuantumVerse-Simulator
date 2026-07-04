/**
 * @file qmlcamcontroller.cpp
 * @brief Implementation of QML camera controller
 */

// Qt headers MUST be included before ui4d/UI4D.h, which opens namespace
// quantumverse. Qt uses QT_BEGIN_NAMESPACE / QT_END_NAMESPACE macros
// internally that break when nested inside our namespace.
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTouchEvent>

#include "qmlcamcontroller.h"
#include "ui4d/UI4D.h"
#include <algorithm>
#include <cmath>

// Ensure M_PI is available on all platforms (MSVC <cmath> may not define it)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

QmlCamController::QmlCamController(QObject* parent)
    : QObject(parent)
    , m_distance(50.0f)
    , m_azimuth(0.0f)
    , m_elevation(0.3f)  // Slightly above equator
    , m_target(0.0f, 0.0f, 0.0f)
    , m_fov(45.0f)
    , m_isDragging(false)
    , m_lastMouseX(0.0f)
    , m_lastMouseY(0.0f)
    , m_activeButton(0)
{
    // Initialize 4D camera
    m_camera4d = std::make_unique<Camera4D>(
        Event4D(0.0, 0.0, 0.0, m_distance),
        Event4D(0.0, 0.0, 0.0, 0.0),
        m_fov * M_PI / 180.0
    );
}

QMatrix4x4 QmlCamController::viewMatrix() const
{
    QMatrix4x4 view;
    view.translate(0.0f, 0.0f, -m_distance);
    view.rotate(static_cast<float>(m_elevation * 180.0 / M_PI), 1.0f, 0.0f, 0.0f);
    view.rotate(static_cast<float>(m_azimuth * 180.0 / M_PI), 0.0f, 1.0f, 0.0f);
    view.translate(-m_target.x(), -m_target.y(), -m_target.z());
    return view;
}

QMatrix4x4 QmlCamController::projectionMatrix(int width, int height) const
{
    QMatrix4x4 proj;
    proj.perspective(m_fov, static_cast<float>(width) / height, 0.1f, 10000.0f);
    return proj;
}

void QmlCamController::reset()
{
    setDistance(50.0f);
    setAzimuth(0.0f);
    setElevation(0.3f);
    setTarget(QVector3D(0.0f, 0.0f, 0.0f));
    setFov(45.0f);
    emit viewChanged();
}

void QmlCamController::focusOn(const QVector3D& position)
{
    setTarget(position);
    emit viewChanged();
}

void QmlCamController::dolly(float amount)
{
    setDistance(m_distance - amount * 0.5f);
    emit viewChanged();
}

void QmlCamController::rotate4D(int planeA, int planeB, double angle)
{
    if (m_camera4d) {
        m_camera4d->rotate(planeA, planeB, angle);
        emit viewChanged();
    }
}

void QmlCamController::setDistance(float dist)
{
    if (std::abs(m_distance - dist) > 0.01f) {
        m_distance = std::max(0.1f, std::min(dist, 10000.0f));
        emit distanceChanged();
        emit viewChanged();
    }
}

void QmlCamController::setAzimuth(float az)
{
    if (std::abs(m_azimuth - az) > 0.001f) {
        m_azimuth = az;
        // Wrap to [-PI, PI]
        while (m_azimuth > M_PI) m_azimuth -= 2.0 * M_PI;
        while (m_azimuth < -M_PI) m_azimuth += 2.0 * M_PI;
        emit azimuthChanged();
        emit viewChanged();
    }
}

void QmlCamController::setElevation(float el)
{
    if (std::abs(m_elevation - el) > 0.001f) {
        // Clamp to avoid gimbal lock at poles
        const float halfPi = static_cast<float>(M_PI) * 0.5f;
        m_elevation = std::max(-halfPi + 0.01f,
                               std::min(halfPi - 0.01f, el));
        emit elevationChanged();
        emit viewChanged();
    }
}

void QmlCamController::setTarget(const QVector3D& pos)
{
    if (m_target != pos) {
        m_target = pos;
        emit targetChanged();
        emit viewChanged();
    }
}

void QmlCamController::setFov(float f)
{
    if (std::abs(m_fov - f) > 0.1f) {
        m_fov = std::max(10.0f, std::min(120.0f, f));
        emit fovChanged();
        emit viewChanged();
    }
}

// Input handlers
void QmlCamController::onMousePressed(float x, float y, int button)
{
    m_isDragging = true;
    m_lastMouseX = x;
    m_lastMouseY = y;
    m_activeButton = button;
    emit isDraggingChanged();
}

void QmlCamController::onMouseMoved(float x, float y, int buttons)
{
    if (!m_isDragging) return;

    float dx = x - m_lastMouseX;
    float dy = y - m_lastMouseY;
    m_lastMouseX = x;
    m_lastMouseY = y;

    // Sensitivity scaling
    const float rotSpeed = 0.005f;
    const float panSpeed = 0.01f;

    if (m_activeButton == 1) {
        // Left button: orbit
        setAzimuth(m_azimuth + dx * rotSpeed);
        setElevation(m_elevation + dy * rotSpeed);
    } else if (m_activeButton == 2 || m_activeButton == 3) {
        // Right/middle button: pan
        float distScale = m_distance * panSpeed;
        QVector3D right = QVector3D::crossProduct(
            QVector3D(0, 0, -1),
            QVector3D(std::sin(m_azimuth), 0, std::cos(m_azimuth))
        ).normalized();
        QVector3D up(0, 1, 0);

        setTarget(m_target - right * dx * distScale + up * dy * distScale);
    }
}

void QmlCamController::onMouseReleased(float x, float y, int button)
{
    Q_UNUSED(x); Q_UNUSED(y); Q_UNUSED(button);
    m_isDragging = false;
    emit isDraggingChanged();
}

void QmlCamController::onWheelDelta(float delta)
{
    // Logarithmic zoom: closer to the target is faster
    float zoomFactor = 1.0f - delta * 0.001f;
    setDistance(m_distance * zoomFactor);
}

void QmlCamController::onPinchDelta(float scale)
{
    setDistance(m_distance / scale);
}

void QmlCamController::onKeyPressed(int key)
{
    switch (key) {
    case Qt::Key_W: dolly(-2.0f); break;
    case Qt::Key_S: dolly(2.0f); break;
    case Qt::Key_R: reset(); break;
    case Qt::Key_F:  // Focus on selected object (placeholder)
        break;
    default: break;
    }
}

QmlCamController::~QmlCamController() = default;

} // namespace quantumverse