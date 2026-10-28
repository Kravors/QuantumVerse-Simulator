/**
 * @file SandboxManager.cpp
 * @brief Implementation of SandboxManager: editable static body scene with
 * JSON save/load and viewport rendering push.
 */

#include "sandbox/SandboxManager.h"

#include <QUuid>
#include <QGuiApplication>
#include <QCoreApplication>
#include <QColor>

#include "qmlglviewport.h"

namespace quantumverse {

// ---------------------------------------------------------------------------
// BodyDescriptor JSON
// ---------------------------------------------------------------------------

QJsonObject BodyDescriptor::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["type"] = type;
    obj["name"] = name;
    obj["mass_kg"] = mass_kg;
    obj["radius_m"] = radius_m;
    QJsonArray pos{ x, y, z };
    obj["position"] = pos;
    QJsonArray col{ static_cast<double>(color.redF()),
                    static_cast<double>(color.greenF()),
                    static_cast<double>(color.blueF()) };
    obj["color"] = col;
    return obj;
}

BodyDescriptor BodyDescriptor::fromJson(const QJsonObject& obj) {
    BodyDescriptor b;
    b.id = obj.value("id").toString();
    b.type = obj.value("type").toString();
    b.name = obj.value("name").toString();
    b.mass_kg = obj.value("mass_kg").toDouble();
    b.radius_m = obj.value("radius_m").toDouble();
    if (obj.value("position").isArray()) {
        QJsonArray pos = obj.value("position").toArray();
        if (pos.size() >= 1) b.x = pos[0].toDouble();
        if (pos.size() >= 2) b.y = pos[1].toDouble();
        if (pos.size() >= 3) b.z = pos[2].toDouble();
    }
    if (obj.value("color").isArray()) {
        QJsonArray col = obj.value("color").toArray();
        double r = col.size() >= 1 ? col[0].toDouble() : 1.0;
        double g = col.size() >= 2 ? col[1].toDouble() : 1.0;
        double bl = col.size() >= 3 ? col[2].toDouble() : 1.0;
        b.color = QColor::fromRgbF(r, g, bl);
    } else {
        b.color = QColor("#ffffff");
    }
    return b;
}

// ---------------------------------------------------------------------------
// SandboxManager
// ---------------------------------------------------------------------------

SandboxManager::SandboxManager(QObject* parent)
    : QObject(parent) {}

QString SandboxManager::generateId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

BodyDescriptor* SandboxManager::findBody(const QString& id) {
    for (auto& b : m_bodies) {
        if (b.id == id) return &b;
    }
    return nullptr;
}

QVariantList SandboxManager::bodiesAsVariants() const {
    QVariantList list;
    list.reserve(m_bodies.size());
    for (const auto& b : m_bodies) {
        QVariantMap m;
        m["id"] = b.id;
        m["type"] = b.type;
        m["name"] = b.name;
        m["mass_kg"] = b.mass_kg;
        m["radius_m"] = b.radius_m;
        m["x"] = b.x;
        m["y"] = b.y;
        m["z"] = b.z;
        m["color"] = b.color; // QColor as QVariant -> QML handles it
        list.append(m);
    }
    return list;
}

QVariantList SandboxManager::bodies() const {
    return bodiesAsVariants();
}

void SandboxManager::pushToViewport() {
    if (m_viewport) {
        m_viewport->setSandboxBodies(bodiesAsVariants());
    }
}

QString SandboxManager::addBody(const QString& type, const QString& name) {
    BodyDescriptor b;
    b.id = generateId();
    b.type = type;
    b.name = name.isEmpty() ? (type + " " + QString::number(m_bodies.size() + 1)) : name;
    b.color = QColor("#ffffff");
    m_bodies.push_back(b);
    emit bodyAdded(b.id);
    emit bodiesChanged();
    pushToViewport();
    return b.id;
}

void SandboxManager::updateBody(const QString& id, const QVariantMap& changes) {
    BodyDescriptor* b = findBody(id);
    if (!b) {
        emit errorOccurred(QString("updateBody: no body with id %1").arg(id));
        return;
    }
    if (changes.contains("name")) b->name = changes.value("name").toString();
    if (changes.contains("mass_kg")) b->mass_kg = changes.value("mass_kg").toDouble();
    if (changes.contains("radius_m")) b->radius_m = changes.value("radius_m").toDouble();
    if (changes.contains("x")) b->x = changes.value("x").toDouble();
    if (changes.contains("y")) b->y = changes.value("y").toDouble();
    if (changes.contains("z")) b->z = changes.value("z").toDouble();
    if (changes.contains("color")) {
        QVariant v = changes.value("color");
        if (v.userType() == QMetaType::QColor) {
            b->color = v.value<QColor>();
        } else {
            b->color = QColor(v.toString());
        }
    }
    emit bodyUpdated(id);
    emit bodiesChanged();
    pushToViewport();
}

void SandboxManager::removeBody(const QString& id) {
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(),
        [&](const BodyDescriptor& b) { return b.id == id; });
    if (it == m_bodies.end()) {
        emit errorOccurred(QString("removeBody: no body with id %1").arg(id));
        return;
    }
    m_bodies.erase(it);
    emit bodyRemoved(id);
    emit bodiesChanged();
    pushToViewport();
}

void SandboxManager::clear() {
    m_bodies.clear();
    m_currentFilePath.clear();
    emit bodiesChanged();
    emit currentFilePathChanged();
    pushToViewport();
}

void SandboxManager::setActive(bool active) {
    if (m_isActive == active) return;
    m_isActive = active;
    emit isActiveChanged();
}

void SandboxManager::setViewport(QmlGlViewport* viewport) {
    m_viewport = viewport;
}

bool SandboxManager::saveTo(const QString& path) {
    QJsonArray arr;
    for (const auto& b : m_bodies) {
        arr.append(b.toJson());
    }
    QJsonObject root;
    root["version"] = "1.0";
    root["bodies"] = arr;

    QString targetPath = path;
    if (targetPath.isEmpty()) {
        targetPath = m_currentFilePath;
    }
    if (targetPath.isEmpty()) {
        targetPath = defaultSaveDir() + "/sandbox.json";
    }

    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit errorOccurred(QString("saveTo: cannot open %1 for writing").arg(targetPath));
        return false;
    }
    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    m_currentFilePath = targetPath;
    emit currentFilePathChanged();
    return true;
}

bool SandboxManager::loadFrom(const QString& path) {
    QString targetPath = path;
    if (targetPath.isEmpty()) {
        targetPath = m_currentFilePath;
    }
    if (targetPath.isEmpty()) {
        emit errorOccurred("loadFrom: no path specified");
        return false;
    }

    QFile file(targetPath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred(QString("loadFrom: cannot open %1 for reading").arg(targetPath));
        return false;
    }
    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        emit errorOccurred(QString("loadFrom: invalid JSON in %1").arg(targetPath));
        return false;
    }

    QJsonObject root = doc.object();
    QJsonArray arr = root.value("bodies").toArray();
    std::vector<BodyDescriptor> loaded;
    loaded.reserve(arr.size());
    for (const QJsonValueRef v : arr) {
        loaded.push_back(BodyDescriptor::fromJson(v.toObject()));
    }

    m_bodies = std::move(loaded);
    m_currentFilePath = targetPath;
    emit bodiesChanged();
    emit currentFilePathChanged();
    pushToViewport();
    return true;
}

QString SandboxManager::defaultSaveDir() const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) {
        dir = QCoreApplication::applicationDirPath();
    }
    dir += "/scenarios";
    QDir().mkpath(dir);
    return dir;
}

} // namespace quantumverse
