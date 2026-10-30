#pragma once

/**
 * @file SpacetimeGPS.h
 * @brief Object registry + telemetry feed ("Spacetime GPS") for the simulator.
 *
 * Tracks every entity that moves or spins (solar-system bodies, black hole,
 * probe) and reports live coordinates in multiple frames:
 *   - world 4-position (t, x, y, z) and velocity
 *   - spin axis (sx, sy, sz) and spin rate (rad/s)
 *   - spherical coordinates (r, theta, phi) in the active metric's frame
 *   - gravitational redshift z and accumulated proper time tau
 * A per-object history ring buffer (trackXY) feeds the GPS trail plot.
 * pin(id) / focusOn(id) let a click in the QML panel re-centre the camera.
 *
 * Threading: updated from the GUI thread inside QmlGlViewport::updateSimulation(),
 * throttled internally to updateHz so the QAbstractListModel never chokes the
 * QML bindings.
 */

#ifndef QUANTUMVERSE_SPACETIME_GPS_H
#define QUANTUMVERSE_SPACETIME_GPS_H

#include <QAbstractListModel>
#include <QColor>
#include <QDateTime>
#include <QPointF>
#include <QVector>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>

#include "spacetime/Event4D.h"

namespace quantumverse {

class UI4D;
class MetricTensor;

class SpacetimeGPS : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool trackingEnabled READ trackingEnabled WRITE setTrackingEnabled NOTIFY trackingEnabledChanged)
    Q_PROPERTY(int updateHz READ updateHz WRITE setUpdateHz NOTIFY updateHzChanged)
    Q_PROPERTY(QString selectedId READ selectedId WRITE setSelectedId NOTIFY selectedIdChanged)
    Q_PROPERTY(double viewportScale READ viewportScale WRITE setViewportScale NOTIFY viewportScaleChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        TypeRole,
        ColorRole,
        TRole,
        XRole,
        YRole,
        ZRole,
        VxRole,
        VyRole,
        VzRole,
        SpeedRole,
        SpinXRole,
        SpinYRole,
        SpinZRole,
        SpinRateRole,
        RRole,
        ThetaRole,
        PhiRole,
        RedshiftRole,
        ProperTimeRole,
        PinnedRole
    };
    Q_ENUM(Roles)

    struct Tracked {
        QString id;
        QString name;
        QString type;
        QColor color;
        Event4D pos;      // world 4-position (t, x, y, z)
        Event4D vel;      // world velocity (vx, vy, vz), spatial part
        double spinX = 0.0;
        double spinY = 0.0;
        double spinZ = 0.0;
        double spinRate = 0.0; // rad/s
        double r = 0.0;
        double theta = 0.0;
        double phi = 0.0;
        double redshift = 0.0;
        double properTime = 0.0;
        bool pinned = false;
        std::deque<QPointF> trackXY; // scaled (x, y) trail for plotting
    };

    explicit SpacetimeGPS(QObject* parent = nullptr);
    ~SpacetimeGPS() override;

    // QAbstractListModel
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // ---- QML API ----
    Q_INVOKABLE void pin(const QString& id, bool pinned);
    Q_INVOKABLE void focusOn(const QString& id);
    Q_INVOKABLE QVariantMap details(const QString& id) const;
    Q_INVOKABLE QVariantList track(const QString& id, int maxSamples = 500) const;
    Q_INVOKABLE QStringList types() const;

    int count() const { return static_cast<int>(m_objects.size()); }
    bool trackingEnabled() const { return m_trackingEnabled; }
    int updateHz() const { return m_updateHz; }
    QString selectedId() const { return m_selectedId; }
    double viewportScale() const { return m_viewportScale; }

    Q_INVOKABLE void setTrackingEnabled(bool enabled);
    Q_INVOKABLE void setUpdateHz(int hz);
    Q_INVOKABLE void setSelectedId(const QString& id);
    Q_INVOKABLE void setViewportScale(double scale);

    static QColor typeColor(const QString& type);

    // ---- C++ side ----
    void registerObject(const Tracked& t);

    /**
     * @brief Pull live state from the simulation and refresh the model.
     * @param ui4d scene (solar-system bodies + kerr spin)
     * @param metric active metric (for redshift + horizon validity)
     * @param simTime current simulation time (seconds)
     * @param dt frame delta (seconds), used for proper-time accumulation
     *
     * Throttled to m_updateHz. Safe to call every frame.
     */
    void updateFromSimulation(UI4D* ui4d, MetricTensor* metric,
                              double simTime = 0.0, double dt = 0.0);

    const Tracked* objectById(const QString& id) const;

signals:
    void countChanged();
    void trackingEnabledChanged();
    void updateHzChanged();
    void selectedIdChanged();
    void viewportScaleChanged();
    void focusRequested(const QString& id, double x, double y, double z);

private:
    QVector<Tracked> m_objects;
    QHash<QString, int> m_indexById; // id -> row in m_objects
    QString m_selectedId;
    bool m_trackingEnabled = true;
    int m_updateHz = 10;
    double m_viewportScale = 1.0;
    qint64 m_lastUpdateMs = 0;

    static double redshiftAt(const MetricTensor* metric, const Event4D& event);
    void refreshTracked(Tracked& tr, UI4D* ui4d, MetricTensor* metric, double dt);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_SPACETIME_GPS_H
