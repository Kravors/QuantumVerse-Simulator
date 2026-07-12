/**
 * @file FindingsModel.cpp
 * @brief Implementation of the QML-facing findings list model
 */

#include "FindingsModel.h"

namespace quantumverse {

FindingsModel::FindingsModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int FindingsModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_findings.size());
}

QHash<int, QByteArray> FindingsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[InstrumentNameRole] = "instrumentName";
    roles[DescriptionRole] = "description";
    roles[SeverityRole] = "severity";
    roles[ConfidenceRole] = "confidence";
    roles[TimestampRole] = "timestamp";
    roles[XRole] = "x";
    roles[YRole] = "y";
    roles[ZRole] = "z";
    return roles;
}

QVariant FindingsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 ||
        index.row() >= static_cast<int>(m_findings.size())) {
        return QVariant();
    }

    const InstrumentFinding& f = m_findings[index.row()];
    switch (role) {
    case IdRole:
        return QString::fromStdString(f.id);
    case InstrumentNameRole:
        return QString::fromStdString(f.instrumentName);
    case DescriptionRole:
        return QString::fromStdString(f.description);
    case SeverityRole: {
        switch (f.severity) {
        case AlertSeverity::INFO:     return "INFO";
        case AlertSeverity::LOW:      return "LOW";
        case AlertSeverity::MEDIUM:   return "MEDIUM";
        case AlertSeverity::HIGH:     return "HIGH";
        case AlertSeverity::CRITICAL: return "CRITICAL";
        }
        return "UNKNOWN";
    }
    case ConfidenceRole:
        return f.confidence;
    case TimestampRole:
        return f.timestamp;
    case XRole:
        return f.location.x;
    case YRole:
        return f.location.y;
    case ZRole:
        return f.location.z;
    default:
        return QVariant();
    }
}

void FindingsModel::addFinding(const InstrumentFinding& finding)
{
    const int row = static_cast<int>(m_findings.size());
    beginInsertRows(QModelIndex(), row, row);
    m_findings.push_back(finding);
    endInsertRows();
}

void FindingsModel::setFindings(const std::vector<InstrumentFinding>& findings)
{
    beginResetModel();
    m_findings = findings;
    endResetModel();
}

void FindingsModel::clear()
{
    beginResetModel();
    m_findings.clear();
    endResetModel();
}

} // namespace quantumverse
