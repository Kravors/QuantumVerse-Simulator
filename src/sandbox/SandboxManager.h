/**
 * @file SandboxManager.h
 * @brief QML-facing manager for the V1 Sandbox scene: editable static bodies
 * with JSON save/load round-trip.
 *
 * SandboxManager owns a live, user-editable list of BodyDescriptor objects.
 * It is distinct from ScenarioManager (which loads preset experiment scripts)
 * and is exposed to QML as a context property so the SandboxPanel can add,
 * edit, delete, save, and load bodies.
 *
 * Bodies are pushed to QmlGlViewport for rendering via setViewport(); the
 * viewport re-adds sandbox bodies each frame alongside N-body data.
 */

#pragma once

#include <QObject>
#include <QVariant>
#include <QColor>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <vector>
#include <string>
#include <random>
#include <QPointer>

namespace quantumverse {

class QmlGlViewport;
struct BodyDescriptor {
    QString id;            // uuid
    QString type;          // "star" | "planet" | "black_hole"
    QString name;          // display label
    double mass_kg = 0.0;
    double radius_m = 0.0;
    double x = 0.0, y = 0.0, z = 0.0;
    QColor color;

    QJsonObject toJson() const;
    static BodyDescriptor fromJson(const QJsonObject& obj);
};

class SandboxManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList bodies READ bodies NOTIFY bodiesChanged)
    Q_PROPERTY(QString currentFilePath READ currentFilePath NOTIFY currentFilePathChanged)
    Q_PROPERTY(bool isActive READ isActive NOTIFY isActiveChanged)

public:
    explicit SandboxManager(QObject* parent = nullptr);
    ~SandboxManager() override = default;

    QVariantList bodies() const;
    QString currentFilePath() const { return m_currentFilePath; }
    bool isActive() const { return m_isActive; }

    Q_INVOKABLE QString addBody(const QString& type, const QString& name = QString());
    Q_INVOKABLE void updateBody(const QString& id, const QVariantMap& changes);
    Q_INVOKABLE void removeBody(const QString& id);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void setActive(bool active);

    Q_INVOKABLE bool saveTo(const QString& path);
    Q_INVOKABLE bool loadFrom(const QString& path);
    Q_INVOKABLE QString defaultSaveDir() const;

    void setViewport(QmlGlViewport* viewport);

signals:
    void bodiesChanged();
    void bodyAdded(const QString& id);
    void bodyUpdated(const QString& id);
    void bodyRemoved(const QString& id);
    void currentFilePathChanged();
    void isActiveChanged();
    void errorOccurred(const QString& message);

private:
    std::vector<BodyDescriptor> m_bodies;
    QString m_currentFilePath;
    bool m_isActive = false;
    QPointer<QmlGlViewport> m_viewport;

    BodyDescriptor* findBody(const QString& id);
    QVariantList bodiesAsVariants() const;
    void pushToViewport();
    QString generateId();
};

} // namespace quantumverse
