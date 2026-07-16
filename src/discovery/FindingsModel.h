/**
 * @file FindingsModel.h
 * @brief QAbstractListModel exposing discovery findings to QML
 *
 * Wraps the discovery instruments' (and multi-messenger) findings in a list
 * model so the QML Anomaly Feed can bind to it directly, with client-side
 * filtering (by severity / instrument), sorting (by time / severity /
 * confidence) and a selection/detail API.
 */

#ifndef QUANTUMVERSE_FINDINGS_MODEL_H
#define QUANTUMVERSE_FINDINGS_MODEL_H

#include <QAbstractListModel>
#include <vector>
#include <string>
#include "DiscoveryInstrument.h"

namespace quantumverse {

/**
 * @brief List model of discovery findings for QML consumption
 *
 * Maintains the full source list (m_findings) and a derived view
 * (m_view) that reflects the active filters and sort order. The view is
 * rebuilt whenever the source list or any filter/sort property changes.
 */
class FindingsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString filterSeverity READ filterSeverity WRITE setFilterSeverity NOTIFY filterSeverityChanged)
    Q_PROPERTY(QString filterInstrument READ filterInstrument WRITE setFilterInstrument NOTIFY filterInstrumentChanged)
    Q_PROPERTY(QString sortRole READ sortRole WRITE setSortRole NOTIFY sortRoleChanged)
    Q_PROPERTY(bool sortAscending READ sortAscending WRITE setSortAscending NOTIFY sortAscendingChanged)
    Q_PROPERTY(QVariantMap currentFinding READ currentFinding NOTIFY currentFindingChanged)

public:
    explicit FindingsModel(QObject* parent = nullptr);

    enum Roles {
        IdRole = Qt::UserRole + 1,
        InstrumentNameRole,
        DescriptionRole,
        SeverityRole,
        ConfidenceRole,
        TimestampRole,
        XRole,
        YRole,
        ZRole,
        IsAnomalyRole
    };
    Q_ENUM(Roles)

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    /** @brief Number of findings currently shown (after filter/sort). */
    Q_INVOKABLE int count() const { return static_cast<int>(m_view.size()); }

    /** @brief Append a single finding and refresh the view. */
    void addFinding(const InstrumentFinding& finding);

    /** @brief Replace the whole model contents with @p findings. */
    void setFindings(const std::vector<InstrumentFinding>& findings);

    /** @brief Remove all findings. */
    void clear();

    // --- Filter / sort properties ---
    QString filterSeverity() const { return QString::fromStdString(m_filterSeverity); }
    void setFilterSeverity(const QString& s);

    QString filterInstrument() const { return QString::fromStdString(m_filterInstrument); }
    void setFilterInstrument(const QString& s);

    QString sortRole() const { return QString::fromStdString(m_sortRole); }
    void setSortRole(const QString& s);

    bool sortAscending() const { return m_sortAscending; }
    void setSortAscending(bool asc);

    // --- Selection / detail ---
    /** @brief Select a row in the (filtered/sorted) view. */
    Q_INVOKABLE void select(int index);

    /** @brief Metadata of the selected finding (empty if none selected). */
    QVariantMap currentFinding() const;

signals:
    void filterSeverityChanged();
    void filterInstrumentChanged();
    void sortRoleChanged();
    void sortAscendingChanged();
    void currentFindingChanged();

private:
    static std::string severityToString(AlertSeverity sev);
    static int severityRank(AlertSeverity sev);

    /** @brief Rebuild m_view from m_findings using current filters/sort. */
    void refreshView();

    std::vector<InstrumentFinding> m_findings; // source (insertion order)
    std::vector<InstrumentFinding> m_view;     // filtered + sorted, shown to QML
    std::string m_filterSeverity;
    std::string m_filterInstrument;
    std::string m_sortRole = "timestamp";
    bool m_sortAscending = false;
    int m_selected = -1;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_FINDINGS_MODEL_H
