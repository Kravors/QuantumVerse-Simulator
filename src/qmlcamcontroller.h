/**
 * @file qmlcamcontroller.h
 * @brief QML-accessible camera controller for 4D viewport navigation
 *
 * Bridges QML input events (mouse, keyboard, touch) to the Camera4D
 * and QmlGlRenderer, providing a Solar System Scope-like interaction model.
 */

#ifndef QMLCAMCONTROLLER_H
#define QMLCAMCONTROLLER_H

#include <QObject>
#include <QVector3D>
#include <QMatrix4x4>
#include <QQmlEngine>
#include <memory>

// Camera4D is defined in UI4D.h
namespace quantumverse {
class Camera4D;
}

namespace quantumverse {

/**
 * @brief Camera controller exposed to QML for viewport navigation
 *
 * Handles orbit, pan, zoom, and 4D rotation interactions.
 * Designed for Solar System Scope-like intuitive manipulation.
 */
class QmlCamController : public ::QObject
{
    Q_OBJECT
    Q_PROPERTY(float distance READ distance WRITE setDistance NOTIFY distanceChanged)
    Q_PROPERTY(float azimuth READ azimuth WRITE setAzimuth NOTIFY azimuthChanged)
    Q_PROPERTY(float elevation READ elevation WRITE setElevation NOTIFY elevationChanged)
    Q_PROPERTY(::QVector3D target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(float fov READ fov WRITE setFov NOTIFY fovChanged)
    Q_PROPERTY(bool isDragging READ isDragging NOTIFY isDraggingChanged)

public:
    explicit QmlCamController(QObject* parent = nullptr);
    ~QmlCamController() override;

    // Property accessors
    float distance() const { return m_distance; }
    float azimuth() const { return m_azimuth; }
    float elevation() const { return m_elevation; }
    QVector3D target() const { return m_target; }
    float fov() const { return m_fov; }
    bool isDragging() const { return m_isDragging; }

    // Get computed view matrix
    QMatrix4x4 viewMatrix() const;
    QMatrix4x4 projectionMatrix(int width, int height) const;

    // Reset to default view
    Q_INVOKABLE void reset();

    // Focus on a specific world-space position
    Q_INVOKABLE void focusOn(const QVector3D& position);

    // Dolly zoom
    Q_INVOKABLE void dolly(float amount);

    // 4D rotation
    Q_INVOKABLE void rotate4D(int planeA, int planeB, double angle);

signals:
    void distanceChanged();
    void azimuthChanged();
    void elevationChanged();
    void targetChanged();
    void fovChanged();
    void isDraggingChanged();
    void viewChanged();

public slots:
    void setDistance(float dist);
    void setAzimuth(float az);
    void setElevation(float el);
    void setTarget(const QVector3D& pos);
    void setFov(float f);

    // Input handlers called from QML
    Q_INVOKABLE void onMousePressed(float x, float y, int button);
    Q_INVOKABLE void onMouseMoved(float x, float y, int buttons);
    Q_INVOKABLE void onMouseReleased(float x, float y, int button);
    Q_INVOKABLE void onWheelDelta(float delta);
    Q_INVOKABLE void onPinchDelta(float scale);
    Q_INVOKABLE void onKeyPressed(int key);

private:
    // Spherical orbit coordinates
    float m_distance;    ///< Distance from target
    float m_azimuth;     ///< Horizontal angle (radians)
    float m_elevation;   ///< Vertical angle (radians)
    QVector3D m_target;  ///< Look-at point
    float m_fov;         ///< Field of view (degrees)

    // Interaction state
    bool m_isDragging;
    float m_lastMouseX;
    float m_lastMouseY;
    int m_activeButton;

    // 4D camera state
    std::unique_ptr<Camera4D> m_camera4d;
};

} // namespace quantumverse

#endif // QMLCAMCONTROLLER_H