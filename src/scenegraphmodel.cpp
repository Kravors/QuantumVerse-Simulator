/**
 * @file scenegraphmodel.cpp
 * @brief Implementation of SceneGraphModel for QML integration
 */

// Qt headers MUST be included before opening namespace quantumverse,
// because Qt uses QT_BEGIN_NAMESPACE / QT_END_NAMESPACE macros internally.
#include <QColor>
#include <QDebug>
#include <fstream>

#include "scenegraphmodel.h"

namespace quantumverse {

SceneGraphModel::SceneGraphModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

void SceneGraphModel::setSceneGraphManager(std::shared_ptr<SceneGraphManager> manager)
{
    beginResetModel();
    m_manager = std::move(manager);
    endResetModel();
    emit countChanged();
}

int SceneGraphModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid() || !m_manager) {
        return 0;
    }
    return static_cast<int>(m_manager->getScene().objects.size());
}

QVariant SceneGraphModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || !m_manager) {
        return QVariant();
    }

    int row = index.row();
    const auto& objects = m_manager->getScene().objects;

    if (row < 0 || row >= static_cast<int>(objects.size())) {
        return QVariant();
    }

    const auto& obj = objects[row];
    if (!obj) return QVariant();

    static int s_dbg = 0;
    if (s_dbg < 40) {
        std::ofstream("model_debug.log", std::ios::app)
            << "row=" << row << " name=[" << obj->name << "] type="
            << objectTypeToString(obj->type).toStdString() << " id=[" << obj->id << "]\n";
        ++s_dbg;
    }

    switch (role) {
    case NameRole: {
        QString name = QString::fromStdString(obj->name);
        return name;
    }
    case TypeRole: {
        QString type = objectTypeToString(obj->type);
        return type;
    }
    case MassRole:
        return obj->mass;
    case PositionRole: {
        QVariantMap pos;
        pos["t"] = obj->position.t;
        pos["x"] = obj->position.x;
        pos["y"] = obj->position.y;
        pos["z"] = obj->position.z;
        return pos;
    }
    case VelocityRole: {
        QVariantMap vel;
        vel["t"] = obj->velocity.t;
        vel["x"] = obj->velocity.x;
        vel["y"] = obj->velocity.y;
        vel["z"] = obj->velocity.z;
        return vel;
    }
    case ColorRole:
        return QColor::fromRgbF(obj->color[0], obj->color[1], obj->color[2]);
    case IdRole:
        return QString::fromStdString(obj->id);
    case IsSelectedRole:
        return obj->id == m_manager->getScene().selectedObjectId;
    default:
        qDebug() << "SceneGraphModel: unknown role" << role << "for object" << QString::fromStdString(obj->name);
        return QVariant();
    }
}

QHash<int, QByteArray> SceneGraphModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[TypeRole] = "type";
    roles[MassRole] = "mass";
    roles[PositionRole] = "position";
    roles[VelocityRole] = "velocity";
    roles[ColorRole] = "color";
    roles[IdRole] = "objectId";
    roles[IsSelectedRole] = "isSelected";
    return roles;
}

QString SceneGraphModel::selectedObjectId() const
{
    if (!m_manager) return QString();
    return QString::fromStdString(m_manager->getScene().selectedObjectId);
}

void SceneGraphModel::setSelectedObjectId(const QString& id)
{
    if (!m_manager) return;

    std::string stdId = id.toStdString();
    if (m_manager->getScene().selectedObjectId != stdId) {
        m_manager->selectObject(stdId);
        emit selectedObjectIdChanged();
        emit dataChanged(index(0), index(rowCount() - 1), {IsSelectedRole});
    }
}

void SceneGraphModel::addObject(const QString& type, const QString& name,
                                double x, double y, double z)
{
    if (!m_manager) return;

    ObjectType objType = ObjectType::CelestialBody;
    QString typeLower = type.toLower();
    if (typeLower == "blackhole") objType = ObjectType::BlackHole;
    else if (typeLower == "particle") objType = ObjectType::Particle;
    else if (typeLower == "quantumobject") objType = ObjectType::QuantumObject;
    else if (typeLower == "singularity") objType = ObjectType::Singularity;

    Event4D pos(0.0, x, y, z);
    std::string id = m_manager->addObject(objType, name.toStdString(), pos);

    beginInsertRows(QModelIndex(), rowCount() - 1, rowCount() - 1);
    endInsertRows();
    emit countChanged();
    emit objectAdded(QString::fromStdString(id), name);
}

void SceneGraphModel::removeObject(const QString& id)
{
    if (!m_manager) return;

    m_manager->removeObject(id.toStdString());
    beginResetModel();
    endResetModel();
    emit countChanged();
    emit objectRemoved(id);
}

void SceneGraphModel::selectObject(const QString& id)
{
    if (!m_manager) return;

    m_manager->selectObject(id.toStdString());
    m_selectedObjectId = id;
    emit selectedObjectIdChanged();
    emit selectionChanged(id);
    emit dataChanged(index(0), index(rowCount() - 1), {IsSelectedRole});
}

void SceneGraphModel::clearSelection()
{
    if (!m_manager) return;

    m_manager->clearSelection();
    m_selectedObjectId.clear();
    emit selectedObjectIdChanged();
    emit dataChanged(index(0), index(rowCount() - 1), {IsSelectedRole});
}

QVariantMap SceneGraphModel::selectedObjectPosition() const
{
    QVariantMap empty;
    if (!m_manager) return empty;

    const auto& obj = m_manager->getSelectedObject();
    if (!obj) return empty;

    QVariantMap pos;
    pos["t"] = obj->position.t;
    pos["x"] = obj->position.x;
    pos["y"] = obj->position.y;
    pos["z"] = obj->position.z;
    return pos;
}

void SceneGraphModel::refresh()
{
    beginResetModel();
    endResetModel();
    emit countChanged();
}

void SceneGraphModel::createSolarSystem()
{
    if (!m_manager) return;

    m_manager->createSolarSystem();
    beginResetModel();
    endResetModel();
    emit countChanged();
}

void SceneGraphModel::focusCameraOn(const QString& objectId)
{
    if (!m_manager) return;

    m_manager->focusCameraOn(objectId.toStdString());
}

QString SceneGraphModel::objectTypeToString(ObjectType type)
{
    switch (type) {
    case ObjectType::CelestialBody: return "Celestial Body";
    case ObjectType::BlackHole: return "Black Hole";
    case ObjectType::Particle: return "Particle";
    case ObjectType::QuantumObject: return "Quantum Object";
    case ObjectType::Singularity: return "Singularity";
    case ObjectType::Custom: return "Custom";
    default: return "Unknown";
    }
}

} // namespace quantumverse