/**
 * @file FindingsModel.cpp
 * @brief Implementation of the QML-facing findings list model
 */

#include "FindingsModel.h"

#include <algorithm>

namespace quantumverse {

FindingsModel::FindingsModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int FindingsModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_view.size());
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
        index.row() >= static_cast<int>(m_view.size())) {
        return QVariant();
    }

    const InstrumentFinding& f = m_view[index.row()];
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

std::string FindingsModel::severityToString(AlertSeverity sev)
{
    switch (sev) {
    case AlertSeverity::INFO:     return "INFO";
    case AlertSeverity::LOW:      return "LOW";
    case AlertSeverity::MEDIUM:   return "MEDIUM";
    case AlertSeverity::HIGH:     return "HIGH";
    case AlertSeverity::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}

int FindingsModel::severityRank(AlertSeverity sev)
{
    switch (sev) {
    case AlertSeverity::INFO:     return 0;
    case AlertSeverity::LOW:      return 1;
    case AlertSeverity::MEDIUM:   return 2;
    case AlertSeverity::HIGH:     return 3;
    case AlertSeverity::CRITICAL: return 4;
    }
    return -1;
}

void FindingsModel::refreshView()
{
    beginResetModel();
    m_view = m_findings;

    // Filter by severity.
    if (!m_filterSeverity.empty()) {
        m_view.erase(std::remove_if(m_view.begin(), m_view.end(),
            [this](const InstrumentFinding& f) {
                return severityToString(f.severity) != m_filterSeverity;
            }), m_view.end());
    }
    // Filter by instrument.
    if (!m_filterInstrument.empty()) {
        m_view.erase(std::remove_if(m_view.begin(), m_view.end(),
            [this](const InstrumentFinding& f) {
                return f.instrumentName != m_filterInstrument;
            }), m_view.end());
    }

    // Sort.
    if (m_sortRole == "severity") {
        std::sort(m_view.begin(), m_view.end(),
            [this](const InstrumentFinding& a, const InstrumentFinding& b) {
                int ra = severityRank(a.severity);
                int rb = severityRank(b.severity);
                return m_sortAscending ? (ra < rb) : (ra > rb);
            });
    } else if (m_sortRole == "confidence") {
        std::sort(m_view.begin(), m_view.end(),
            [this](const InstrumentFinding& a, const InstrumentFinding& b) {
                return m_sortAscending ? (a.confidence < b.confidence)
                                       : (a.confidence > b.confidence);
            });
    } else { // "timestamp" (default)
        std::sort(m_view.begin(), m_view.end(),
            [this](const InstrumentFinding& a, const InstrumentFinding& b) {
                return m_sortAscending ? (a.timestamp < b.timestamp)
                                       : (a.timestamp > b.timestamp);
            });
    }

    // Selection is no longer valid after a view rebuild.
    m_selected = -1;
    endResetModel();
    emit currentFindingChanged();
}

void FindingsModel::addFinding(const InstrumentFinding& finding)
{
    m_findings.push_back(finding);
    refreshView();
}

void FindingsModel::setFindings(const std::vector<InstrumentFinding>& findings)
{
    m_findings = findings;
    refreshView();
}

void FindingsModel::clear()
{
    m_findings.clear();
    refreshView();
}

void FindingsModel::setFilterSeverity(const QString& s)
{
    std::string v = s.toStdString();
    if (v != m_filterSeverity) {
        m_filterSeverity = v;
        refreshView();
        emit filterSeverityChanged();
    }
}

void FindingsModel::setFilterInstrument(const QString& s)
{
    std::string v = s.toStdString();
    if (v != m_filterInstrument) {
        m_filterInstrument = v;
        refreshView();
        emit filterInstrumentChanged();
    }
}

void FindingsModel::setSortRole(const QString& s)
{
    std::string v = s.toStdString();
    if (v != m_sortRole) {
        m_sortRole = v;
        refreshView();
        emit sortRoleChanged();
    }
}

void FindingsModel::setSortAscending(bool asc)
{
    if (asc != m_sortAscending) {
        m_sortAscending = asc;
        refreshView();
        emit sortAscendingChanged();
    }
}

void FindingsModel::select(int index)
{
    if (index < 0 || index >= static_cast<int>(m_view.size())) {
        if (m_selected != -1) {
            m_selected = -1;
            emit currentFindingChanged();
        }
        return;
    }
    if (index != m_selected) {
        m_selected = index;
        emit currentFindingChanged();
    }
}

QVariantMap FindingsModel::currentFinding() const
{
    QVariantMap map;
    if (m_selected < 0 || m_selected >= static_cast<int>(m_view.size())) {
        return map;
    }
    const InstrumentFinding& f = m_view[m_selected];
    map["id"] = QString::fromStdString(f.id);
    map["instrumentName"] = QString::fromStdString(f.instrumentName);
    map["description"] = QString::fromStdString(f.description);
    map["severity"] = QString::fromStdString(severityToString(f.severity));
    map["confidence"] = f.confidence;
    map["timestamp"] = f.timestamp;
    map["x"] = f.location.x;
    map["y"] = f.location.y;
    map["z"] = f.location.z;
    return map;
}

} // namespace quantumverse
