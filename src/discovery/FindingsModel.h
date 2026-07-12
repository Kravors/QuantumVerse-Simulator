/**
 * @file FindingsModel.h
 * @brief QAbstractListModel exposing discovery findings to QML
 *
 * Wraps the discovery instruments' findings in a list model so the QML
 * Anomaly Feed can bind to it directly and receive incremental updates as
 * instruments produce results. Each row exposes the instrument name,
 * description, severity, confidence, and location of a finding.
 */

#ifndef QUANTUMVERSE_FINDINGS_MODEL_H
#define QUANTUMVERSE_FINDINGS_MODEL_H

#include <QAbstractListModel>
#include <vector>
#include "DiscoveryInstrument.h"

namespace quantumverse {

/**
 * @brief List model of discovery findings for QML consumption
 *
 * Replaces the (read-only QVariantList) findingsList property with a proper
 * QAbstractListModel so the Anomaly Feed updates incrementally and efficiently.
 * Populate it via addFinding() (incremental) or setFindings() (full refresh),
 * and clear() to empty it.
 */
class FindingsModel : public QAbstractListModel
{
    Q_OBJECT
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
        ZRole
    };
    Q_ENUM(Roles)

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    /** @brief Number of findings currently in the model (QML-friendly). */
    Q_INVOKABLE int count() const { return static_cast<int>(m_findings.size()); }

    /** @brief Append a single finding and notify the view. */
    void addFinding(const InstrumentFinding& finding);

    /** @brief Replace the whole model contents with @p findings. */
    void setFindings(const std::vector<InstrumentFinding>& findings);

    /** @brief Remove all findings. */
    void clear();

private:
    std::vector<InstrumentFinding> m_findings;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_FINDINGS_MODEL_H
