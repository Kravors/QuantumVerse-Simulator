#pragma once

/**
 * @file ExperimentRunner.h
 * @brief QML-facing model + executor for experiments.
 *
 * Holds the registered experiments as a QAbstractListModel (for the list
 * view), runs the selected experiment on QThreadPool so the UI stays
 * responsive, and marshals progress/results back to the GUI thread.
 */

#ifndef QUANTUMVERSE_EXPERIMENT_RUNNER_H
#define QUANTUMVERSE_EXPERIMENT_RUNNER_H

#include <QAbstractListModel>
#include <QThreadPool>
#include <QMutex>
#include <QVector>
#include <QHash>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QPointF>
#include <atomic>
#include <memory>
#include <vector>

#include "Experiment.h"
#include "discovery/PaperGenerator.h"
#include "discovery/LLMCoPilot.h"
#include "experiment/ScanSummaryBuilder.h"
#include "discovery/ScanSummary.h"

namespace quantumverse {

struct ExperimentRunOutcome {
    QString id;
    QString name;
    ExperimentResult result;
};

class ExperimentRunner : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString currentId READ currentId NOTIFY currentIdChanged)
    Q_PROPERTY(QString currentName READ currentName NOTIFY currentNameChanged)
    Q_PROPERTY(QString currentLog READ currentLog NOTIFY currentLogChanged)
    Q_PROPERTY(QVariantList currentCurve READ currentCurve NOTIFY currentCurveChanged)
    Q_PROPERTY(QVariantMap currentScalars READ currentScalars NOTIFY currentScalarsChanged)
    Q_PROPERTY(int historyCount READ historyCount NOTIFY historyChanged)

public:
    enum Roles { IdRole = Qt::UserRole + 1, NameRole, CategoryRole, DescriptionRole };
    Q_ENUM(Roles)

    explicit ExperimentRunner(QObject* parent = nullptr);
    ~ExperimentRunner() override;

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    /** Register an experiment (runner takes ownership). */
    void registerExperiment(std::unique_ptr<Experiment> exp);

    /** Declared parameters for the given experiment id (for the QML form). */
    Q_INVOKABLE QVariantList parametersFor(const QString& id) const;

    /** Start running the experiment with the given parameter values. */
    Q_INVOKABLE void start(const QString& id, const QVariantMap& params);
    /** Cancel the in-flight run. */
    Q_INVOKABLE void cancel();
    /** Export the last result to csv/json, return the path. */
    Q_INVOKABLE void exportLast(const QString& format);
    /** Export a fixed-schema cosmology validation report (CSV) from the last
     *  result's scalars. Returns the file path, or empty on failure. */
    Q_INVOKABLE QString exportValidationReport(const QString& format = QStringLiteral("csv"));

    /** Generate a PDF paper from the last result of experiment `id` via
     *  PaperGenerator. Writes to `path` if non-empty, else to the default
     *  output directory. Returns the PDF path on success, empty on failure. */
    Q_INVOKABLE QString exportPDF(const QString& id, const QString& path = QString());
    /** Generate a discovery paper from the last result of experiment `id`.
     *  Returns the PDF path on success, empty on failure (unsupported id,
     *  no prior run, or builder returned invalid sentinel). Synchronous from
     *  the caller's perspective (pdflatex runs on the worker thread). */
    Q_INVOKABLE QString generateDiscoveryPaper(const QString& id);
    /** Whether `generateDiscoveryPaper(id)` can produce a paper — forwards
     *  to ScanSummaryBuilder::canBuildFrom(id). Used to gate the QML button. */
    Q_INVOKABLE bool canBuildPaper(const QString& id) const;
    /** Core CSV-writing logic: takes an ExperimentResult directly (testable
     *  without QML/event loop). Returns the file path, or empty on failure. */
    static QString writeValidationReportCSV(const ExperimentResult& result,
                                            const QString& path = QString());
    /** History of completed runs (id -> summary maps). */
    /** Re-load a history entry into currentScalars/currentCurve/currentLog. */
    Q_INVOKABLE void showHistoryEntry(const QString& id);
    Q_INVOKABLE QVariantList history() const;
    Q_INVOKABLE void clearHistory();

    bool running() const { return m_running.load(); }
    double progress() const { return m_progress.load(); }
    QString currentId() const { return m_currentId; }
    QString currentName() const { return m_currentName; }
    QString currentLog() const { return m_currentLog; }
    QVariantList currentCurve() const;
    QVariantMap currentScalars() const { return m_currentScalars; }
    int count() const { return static_cast<int>(m_experiments.size()); }
    int historyCount() const { return static_cast<int>(m_history.size()); }

signals:
    void runningChanged();
    void progressChanged();
    void currentIdChanged();
    void currentNameChanged();
    void currentLogChanged();
    void currentCurveChanged();
    void currentScalarsChanged();
    void historyChanged();
    void experimentFinished(const QString& id, bool ok);
    void exportFinished(const QString& path);
    void pdfFinished(const QString& path);

public slots:
    // Marshalled back from the worker thread.
    void setProgress(double p);
    void setLog(const QString& log);
    void setResult(const ExperimentRunOutcome& outcome);

private:
    struct Slot {
        std::unique_ptr<Experiment> exp;
        QVariantMap lastParams;
        ExperimentResult lastResult;
    };
    std::vector<Slot> m_experiments;
    QHash<QString, int> m_indexById;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_abort{false};
    std::atomic<double> m_progress{0.0};
    QString m_currentId;
    QString m_currentName;
    QString m_currentLog;
    QVariantList m_currentCurve;
    QVariantMap m_currentScalars;
    QVector<ExperimentRunOutcome> m_history;

    QThreadPool m_pool;
    mutable QMutex m_mutex;
    std::unique_ptr<PaperGenerator> m_paperGenerator;
    std::unique_ptr<LLMCoPilot> m_llmCoPilot;

    /** Most recent successful result for the given experiment id, or nullptr. */
    const ExperimentResult* lastResultFor(const QString& id) const;
    /** Plain-English abstract for the discovery paper, keyed by experiment id. */
    QString abstractForExperiment(const QString& id,
                                  const ExperimentResult& result) const;

    class RunJob : public QRunnable {
    public:
        RunJob(ExperimentRunner* runner, Experiment* exp, QVariantMap params)
            : m_runner(runner), m_exp(exp), m_params(std::move(params)) {}
        void run() override;
    private:
        ExperimentRunner* m_runner;
        Experiment* m_exp;
        QVariantMap m_params;
    };
};

} // namespace quantumverse

#endif // QUANTUMVERSE_EXPERIMENT_RUNNER_H
