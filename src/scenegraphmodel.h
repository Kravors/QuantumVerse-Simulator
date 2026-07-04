/**
 * @file scenegraphmodel.h
 * @brief Qt model for exposing SceneGraph data to QML views
 *
 * Provides a QAbstractListModel bridge between the SceneGraphManager
 * and QML ListView/TableView components for the object browser panel.
 */

#ifndef SCENEGRAPHMODEL_H
#define SCENEGRAPHMODEL_H

#include <QAbstractListModel>
#include <QColor>
#include <QHash>
#include <QObject>
#include <QQmlEngine>
#include <QVariant>
#include <memory>
#include <string>

#include "ui4d/SceneGraphManager.h"

namespace quantumverse {

/**
 * @brief QML model exposing scene graph objects
 *
 * Wraps SceneGraphManager's object registry into a Qt model that
 * QML views can bind to for the object browser sidebar.
 */
class SceneGraphModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QString selectedObjectId READ selectedObjectId WRITE setSelectedObjectId NOTIFY selectedObjectIdChanged)

public:
    enum ObjectRoles {
        NameRole = Qt::UserRole + 1,
        TypeRole,
        MassRole,
        PositionRole,
        VelocityRole,
        ColorRole,
        IdRole,
        IsSelectedRole
    };
    Q_ENUM(ObjectRoles)

    explicit SceneGraphModel(QObject* parent = nullptr);
    ~SceneGraphModel() override = default;

    // QAbstractListModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Set the scene graph manager to pull data from
    void setSceneGraphManager(std::shared_ptr<SceneGraphManager> manager);

    // Access current manager
    std::shared_ptr<SceneGraphManager> sceneGraphManager() const;

    // Selected object
    QString selectedObjectId() const;
    void setSelectedObjectId(const QString& id);

    // Add/remove objects via model
    Q_INVOKABLE void addObject(const QString& type, const QString& name,
                               double x, double y, double z);
    Q_INVOKABLE void removeObject(const QString& id);
    Q_INVOKABLE void selectObject(const QString& id);
    Q_INVOKABLE void clearSelection();

    // Refresh model from scene graph
    Q_INVOKABLE void refresh();

    // Create solar system
    Q_INVOKABLE void createSolarSystem();

    // Focus camera on object
    Q_INVOKABLE void focusCameraOn(const QString& objectId);

signals:
    void countChanged();
    void selectedObjectIdChanged();
    void objectAdded(const QString& id, const QString& name);
    void objectRemoved(const QString& id);
    void selectionChanged(const QString& id);

private:
    std::shared_ptr<SceneGraphManager> m_manager;
    QString m_selectedObjectId;

    // Helper: convert ObjectType enum to string
    static QString objectTypeToString(ObjectType type);
};

} // namespace quantumverse

#endif // SCENEGRAPHMODEL_H