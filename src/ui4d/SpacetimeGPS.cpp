/**
 * @file SpacetimeGPS.cpp
 * @brief Implementation of the Spacetime GPS object registry + telemetry feed.
 */

#include "SpacetimeGPS.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <cmath>

#include "ui4d/UI4D.h"
#include "spacetime/MetricTensor.h"

namespace quantumverse {

SpacetimeGPS::SpacetimeGPS(QObject* parent)
    : QAbstractListModel(parent)
{
    // Initialise to 0 so the first updateFromSimulation() always runs; later
    // calls are throttled to m_updateHz.
    m_lastUpdateMs = 0;
}

SpacetimeGPS::~SpacetimeGPS() = default;

int SpacetimeGPS::rowCount(const QModelIndex& /*parent*/) const {
    return static_cast<int>(m_objects.size());
}

QVariant SpacetimeGPS::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return QVariant();
    int row = index.row();
    if (row < 0 || row >= static_cast<int>(m_objects.size())) return QVariant();
    const Tracked& t = m_objects[row];

    switch (role) {
        case IdRole:          return t.id;
        case NameRole:        return t.name;
        case TypeRole:        return t.type;
        case ColorRole:       return t.color;
        case TRole:           return t.pos.t;
        case XRole:           return t.pos.x;
        case YRole:           return t.pos.y;
        case ZRole:           return t.pos.z;
        case VxRole:          return t.vel.x;
        case VyRole:          return t.vel.y;
        case VzRole:          return t.vel.z;
        case SpeedRole:       return t.vel.spatialLength();
        case SpinXRole:       return t.spinX;
        case SpinYRole:       return t.spinY;
        case SpinZRole:       return t.spinZ;
        case SpinRateRole:    return t.spinRate;
        case RRole:           return t.r;
        case ThetaRole:       return t.theta;
        case PhiRole:         return t.phi;
        case RedshiftRole:    return t.redshift;
        case ProperTimeRole:  return t.properTime;
        case PinnedRole:      return t.pinned;
        default:              return QVariant();
    }
}

QHash<int, QByteArray> SpacetimeGPS::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[NameRole] = "name";
    roles[TypeRole] = "type";
    roles[ColorRole] = "color";
    roles[TRole] = "t";
    roles[XRole] = "x";
    roles[YRole] = "y";
    roles[ZRole] = "z";
    roles[VxRole] = "vx";
    roles[VyRole] = "vy";
    roles[VzRole] = "vz";
    roles[SpeedRole] = "speed";
    roles[SpinXRole] = "spinX";
    roles[SpinYRole] = "spinY";
    roles[SpinZRole] = "spinZ";
    roles[SpinRateRole] = "spinRate";
    roles[RRole] = "r";
    roles[ThetaRole] = "theta";
    roles[PhiRole] = "phi";
    roles[RedshiftRole] = "redshift";
    roles[ProperTimeRole] = "properTime";
    roles[PinnedRole] = "pinned";
    return roles;
}

void SpacetimeGPS::setTrackingEnabled(bool enabled) {
    if (m_trackingEnabled == enabled) return;
    m_trackingEnabled = enabled;
    emit trackingEnabledChanged();
}

void SpacetimeGPS::setUpdateHz(int hz) {
    if (hz < 1) hz = 1;
    if (m_updateHz == hz) return;
    m_updateHz = hz;
    emit updateHzChanged();
}

void SpacetimeGPS::setSelectedId(const QString& id) {
    if (m_selectedId == id) return;
    m_selectedId = id;
    emit selectedIdChanged();
}

void SpacetimeGPS::setViewportScale(double scale) {
    if (scale <= 0.0) scale = 1.0;
    if (std::abs(m_viewportScale - scale) < 1e-12) return;
    m_viewportScale = scale;
    emit viewportScaleChanged();
}

void SpacetimeGPS::pin(const QString& id, bool pinned) {
    auto it = m_indexById.constFind(id);
    if (it == m_indexById.constEnd()) return;
    int row = it.value();
    Tracked& t = m_objects[row];
    if (t.pinned == pinned) return;
    t.pinned = pinned;
    QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {PinnedRole});
}

void SpacetimeGPS::focusOn(const QString& id) {
    auto it = m_indexById.constFind(id);
    if (it == m_indexById.constEnd()) return;
    const Tracked& t = m_objects[it.value()];
    // Emit scaled viewport coordinates so the camera lands on the visible grid.
    double sx = t.pos.x * m_viewportScale;
    double sy = t.pos.y * m_viewportScale;
    double sz = t.pos.z * m_viewportScale;
    emit focusRequested(id, sx, sy, sz);
}

QVariantMap SpacetimeGPS::details(const QString& id) const {
    QVariantMap map;
    auto it = m_indexById.constFind(id);
    if (it == m_indexById.constEnd()) return map;
    const Tracked& t = m_objects[it.value()];
    map["id"] = t.id;
    map["name"] = t.name;
    map["type"] = t.type;
    map["t"] = t.pos.t;
    map["x"] = t.pos.x;
    map["y"] = t.pos.y;
    map["z"] = t.pos.z;
    map["vx"] = t.vel.x;
    map["vy"] = t.vel.y;
    map["vz"] = t.vel.z;
    map["speed"] = t.vel.spatialLength();
    map["spinX"] = t.spinX;
    map["spinY"] = t.spinY;
    map["spinZ"] = t.spinZ;
    map["spinRate"] = t.spinRate;
    map["r"] = t.r;
    map["theta"] = t.theta;
    map["phi"] = t.phi;
    map["redshift"] = t.redshift;
    map["properTime"] = t.properTime;
    map["pinned"] = t.pinned;
    return map;
}

QVariantList SpacetimeGPS::track(const QString& id, int maxSamples) const {
    QVariantList list;
    auto it = m_indexById.constFind(id);
    if (it == m_indexById.constEnd()) return list;
    const Tracked& t = m_objects[it.value()];
    int n = static_cast<int>(t.trackXY.size());
    int start = (n > maxSamples) ? (n - maxSamples) : 0;
    for (int i = start; i < n; ++i) {
        QVariantMap pt;
        pt["x"] = t.trackXY[i].x();
        pt["y"] = t.trackXY[i].y();
        list.append(pt);
    }
    return list;
}

QStringList SpacetimeGPS::types() const {
    QStringList out;
    out << QStringLiteral("All");
    for (const auto& t : m_objects) {
        if (!out.contains(t.type)) out << t.type;
    }
    return out;
}

void SpacetimeGPS::registerObject(const Tracked& t) {
    beginInsertRows(QModelIndex(), static_cast<int>(m_objects.size()),
                    static_cast<int>(m_objects.size()));
    m_objects.push_back(t);
    m_indexById[t.id] = static_cast<int>(m_objects.size()) - 1;
    endInsertRows();
    emit countChanged();
}

QColor SpacetimeGPS::typeColor(const QString& type) {
    if (type == QStringLiteral("BlackHole")) return QColor("#ff6b6b");
    if (type == QStringLiteral("Star")) return QColor("#ffd166");
    if (type == QStringLiteral("Planet")) return QColor("#4af");
    if (type == QStringLiteral("Moon")) return QColor("#aaaaaa");
    if (type == QStringLiteral("Probe")) return QColor("#00ffcc");
    if (type == QStringLiteral("Geodesic")) return QColor("#ff9ff3");
    if (type == QStringLiteral("Vertex")) return QColor("#888");
    if (type == QStringLiteral("Annulus")) return QColor("#f44");
    return QColor("#bbb");
}

double SpacetimeGPS::redshiftAt(const MetricTensor* metric, const Event4D& event) {
    if (!metric) return 0.0;
    auto g = metric->evaluate(event);
    double gtt = g[0][0];
    if (gtt < 0.0 && std::isfinite(gtt)) {
        double z = 1.0 / std::sqrt(-gtt) - 1.0;
        return std::isfinite(z) ? z : 0.0;
    }
    return 0.0;
}

void SpacetimeGPS::refreshTracked(Tracked& tr, UI4D* ui4d, MetricTensor* metric, double dt) {
    // Position / velocity come from the simulation where available.
    if (ui4d) {
        const auto& ss = ui4d->getSolarSystem();
        auto bit = ss.bodies.find(tr.name.toStdString());
        if (bit != ss.bodies.end()) {
            const auto& b = bit->second;
            tr.pos = b.position;
            tr.vel = b.velocity;
        }
    }

    // Spherical coordinates in the metric's natural frame.
    tr.r = tr.pos.spatialLength();
    tr.theta = tr.pos.theta();
    tr.phi = tr.pos.phi();

    // Gravitational redshift from g_tt at the object's position.
    tr.redshift = redshiftAt(metric, tr.pos);

    // Proper-time accumulation: d tau = dt * sqrt(-g_tt) (stationary-observer approx).
    if (metric) {
        auto g = metric->evaluate(tr.pos);
        double gtt = g[0][0];
        double dTau = (gtt < 0.0) ? std::sqrt(-gtt) : 1.0;
        tr.properTime += dt * dTau;
    } else {
        tr.properTime += dt;
    }

    // Append to the history ring buffer (scaled for the viewport grid).
    QPointF scaled(static_cast<float>(tr.pos.x * m_viewportScale),
                   static_cast<float>(tr.pos.y * m_viewportScale));
    tr.trackXY.push_back(scaled);
    while (tr.trackXY.size() > 2000) tr.trackXY.pop_front();
}

void SpacetimeGPS::updateFromSimulation(UI4D* ui4d, MetricTensor* metric,
                                        double /*simTime*/, double dt) {
    if (!m_trackingEnabled) return;

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    int minIntervalMs = (m_updateHz > 0) ? (1000 / m_updateHz) : 100;
    if (now - m_lastUpdateMs < minIntervalMs) return;
    m_lastUpdateMs = now;

    // Refresh each registered object.
    bool anyChanged = false;
    for (auto& tr : m_objects) {
        Tracked before = tr;
        refreshTracked(tr, ui4d, metric, dt);
        // positions change constantly for orbiting bodies; mark data changed so
        // the QML ListView delegates refresh their live readouts.
        int row = m_indexById.value(tr.id, -1);
        if (row >= 0) {
            QModelIndex idx = index(row);
            QModelIndex last = index(row);
            emit dataChanged(idx, last,
                             {TRole, XRole, YRole, ZRole, VxRole, VyRole, VzRole,
                              SpeedRole, RRole, ThetaRole, PhiRole,
                              RedshiftRole, ProperTimeRole});
        }
        (void)before;
        anyChanged = true;
    }
    (void)anyChanged;
}

const SpacetimeGPS::Tracked* SpacetimeGPS::objectById(const QString& id) const {
    auto it = m_indexById.constFind(id);
    if (it == m_indexById.constEnd()) return nullptr;
    return &m_objects[it.value()];
}

} // namespace quantumverse
