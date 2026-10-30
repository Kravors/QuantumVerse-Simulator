/**
 * @file ExperimentRunner.cpp
 * @brief QML-facing experiment model + executor.
 */

#include "ExperimentRunner.h"
#include "Experiment.h"

#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QMutexLocker>
#include <QMetaObject>
#include <QDebug>
#include <QMap>
#include <algorithm>
#include <thread>
#include <cmath>

namespace quantumverse {

ExperimentRunner::ExperimentRunner(QObject* parent)
    : QAbstractListModel(parent)
{
    m_pool.setMaxThreadCount(std::max<size_t>(1, std::thread::hardware_concurrency() / 2));
}

ExperimentRunner::~ExperimentRunner() {
    m_abort.store(true);
    m_pool.waitForDone(5000);
}

int ExperimentRunner::rowCount(const QModelIndex& /*parent*/) const {
    return static_cast<int>(m_experiments.size());
}

QVariant ExperimentRunner::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return QVariant();
    int row = index.row();
    if (row < 0 || row >= static_cast<int>(m_experiments.size())) return QVariant();
    const auto& slot = m_experiments[row];
    const Experiment* e = slot.exp.get();

    switch (role) {
        case IdRole:          return e->id();
        case NameRole:        return e->name();
        case CategoryRole:    return e->category();
        case DescriptionRole: return e->description();
        default:              return QVariant();
    }
}

QHash<int, QByteArray> ExperimentRunner::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[NameRole] = "name";
    roles[CategoryRole] = "category";
    roles[DescriptionRole] = "description";
    return roles;
}

void ExperimentRunner::registerExperiment(std::unique_ptr<Experiment> exp) {
    if (!exp) return;
    beginInsertRows(QModelIndex(), static_cast<int>(m_experiments.size()),
                    static_cast<int>(m_experiments.size()));
    Slot slot;
    slot.exp = std::move(exp);
    m_experiments.push_back(std::move(slot));
    m_indexById[m_experiments.back().exp->id()] = static_cast<int>(m_experiments.size()) - 1;
    endInsertRows();
}

QVariantList ExperimentRunner::parametersFor(const QString& id) const {
    QVariantList list;
    auto it = m_indexById.constFind(id);
    if (it == m_indexById.constEnd()) return list;
    const auto& params = m_experiments[it.value()].exp->parameters();
    for (const auto& p : params) {
        QVariantMap m;
        m["name"] = p.name;
        m["label"] = p.label;
        m["type"] = p.type;
        m["defaultValue"] = p.defaultValue;
        m["min"] = p.min;
        m["max"] = p.max;
        m["options"] = QVariant::fromValue(p.options);
        m["unit"] = p.unit;
        m["help"] = p.help;
        list.append(m);
    }
    return list;
}

void ExperimentRunner::start(const QString& id, const QVariantMap& params) {
    if (m_running.load()) return;
    auto it = m_indexById.constFind(id);
    if (it == m_indexById.constEnd()) return;

    int row = it.value();
    Experiment* exp = m_experiments[row].exp.get();
    m_experiments[row].lastParams = params;

    m_abort.store(false);
    m_running.store(true);
    m_progress.store(0.0);
    m_currentId = id;
    m_currentName = exp->name();
    m_currentLog.clear();
    m_currentCurve.clear();
    m_currentScalars.clear();

    emit runningChanged();
    emit currentIdChanged();
    emit currentNameChanged();
    emit currentLogChanged();
    emit currentCurveChanged();
    emit currentScalarsChanged();

    m_pool.start(new RunJob(this, exp, params));
}

void ExperimentRunner::cancel() {
    m_abort.store(true);
}

void ExperimentRunner::exportLast(const QString& format) {
    QString fmt = format.toLower();
    if (fmt != "csv" && fmt != "json") {
        qWarning() << "exportLast: unsupported format" << format;
        return;
    }

    ExperimentResult result;
    {
        QMutexLocker lock(&m_mutex);
        // find the latest history entry with this id
        for (int i = static_cast<int>(m_history.size()) - 1; i >= 0; --i) {
            if (m_history[i].id == m_currentId) {
                result = m_history[i].result;
                break;
            }
        }
    }

    QString dir = QDir::currentPath() + "/data/experiments";
    QDir().mkpath(dir);
    const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    const QString path = dir + QStringLiteral("/exp_") + m_currentId + QStringLiteral("_") + stamp + QStringLiteral(".") + fmt;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "exportLast: cannot open" << path;
        return;
    }
    QTextStream out(&file);
    if (fmt == "csv") {
        // header: scalar keys
        QStringList keys = result.scalars.keys();
        std::sort(keys.begin(), keys.end());
        out << keys.join(",") << "\n";
        for (const auto& k : keys) out << result.scalars.value(k).toString() << ",";
        out << "\n";
        out << "curve_x,curve_y\n";
        for (const auto& pt : result.curve) out << pt.x() << "," << pt.y() << "\n";
        out << "\n-- log --\n";
        for (const auto& line : result.logLines) out << line << "\n";
    } else {
        out << "{\n";
        out << "  \"id\": \"" << m_currentId << "\",\n";
        out << "  \"scalars\": {\n";
        QStringList keys = result.scalars.keys();
        std::sort(keys.begin(), keys.end());
        for (int i = 0; i < keys.size(); ++i) {
            out << "    \"" << keys[i] << "\": " << result.scalars.value(keys[i]).toString();
            if (i + 1 < keys.size()) out << ",";
            out << "\n";
        }
        out << "  },\n  \"curve\": [\n";
        for (int i = 0; i < result.curve.size(); ++i) {
            out << "    [" << result.curve[i].x() << ", " << result.curve[i].y() << "]";
            if (i + 1 < result.curve.size()) out << ",";
            out << "\n";
        }
        out << "  ],\n  \"log\": [\n";
        for (int i = 0; i < result.logLines.size(); ++i) {
            out << "    \"" << result.logLines[i] << "\"";
            if (i + 1 < result.logLines.size()) out << ",";
            out << "\n";
        }
        out << "  ]\n}\n";
    }
    file.close();

    QMutexLocker lock(&m_mutex);
    for (auto& h : m_history) {
        if (h.id == m_currentId) h.result.outputDir = path;
    }

    emit exportFinished(path);
    qDebug() << "Experiment exported to" << path;
}

QString ExperimentRunner::writeValidationReportCSV(const ExperimentResult& result,
                                                    const QString& path /*= QString()*/)
{
    QString outPath = path;
    if (outPath.isEmpty()) {
        QString dir = QDir::currentPath() + "/data/experiments";
        QDir().mkpath(dir);
        const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        outPath = dir + QStringLiteral("/cosmology_validation_") + stamp + QStringLiteral(".csv");
    }

    QFile file(outPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "writeValidationReportCSV: cannot open" << outPath;
        return QString();
    }
    QTextStream out(&file);
    out << QStringLiteral("# cosmology_validation_report.csv\n");
    out << QStringLiteral("# QuantumVerse Cosmology Validation Report\n");
    out << QStringLiteral("# Generated: %1\n")
           .arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    out << QStringLiteral("\n");
    out << QStringLiteral("parameter,project_value,reference_value,observed_value,observed_sigma,source,unit,rel_error_pct,tension_sigma,pass\n");

    struct Row {
        QString parameter;
        QVariant projectValue, referenceValue, observedValue, observedSigma;
        QString source, unit;
        double relErrorPct = 0.0;
        double tensionSigma = 0.0;
        bool pass = false;
    };

    const QStringList suffixes = {
        QStringLiteral("_project"), QStringLiteral("_ref"), QStringLiteral("_obs"),
        QStringLiteral("_sigma"), QStringLiteral("_source"), QStringLiteral("_unit"),
        QStringLiteral("_rel_err"), QStringLiteral("_tension"), QStringLiteral("_pass")
    };

    QMap<QString, Row> rows;
    QStringList keys = result.scalars.keys();
    std::sort(keys.begin(), keys.end());
    for (const auto& k : keys) {
        if (k.startsWith(QStringLiteral("suite_"))) continue;
        for (const auto& suf : suffixes) {
            if (k.endsWith(suf)) {
                QString param = k.left(k.size() - suf.size());
                Row& r = rows[param];
                r.parameter = param;
                if (suf == QStringLiteral("_project")) r.projectValue = result.scalars.value(k);
                else if (suf == QStringLiteral("_ref")) r.referenceValue = result.scalars.value(k);
                else if (suf == QStringLiteral("_obs")) r.observedValue = result.scalars.value(k);
                else if (suf == QStringLiteral("_sigma")) r.observedSigma = result.scalars.value(k);
                else if (suf == QStringLiteral("_source")) r.source = result.scalars.value(k).toString();
                else if (suf == QStringLiteral("_unit")) r.unit = result.scalars.value(k).toString();
                else if (suf == QStringLiteral("_rel_err")) r.relErrorPct = result.scalars.value(k).toDouble();
                else if (suf == QStringLiteral("_tension")) r.tensionSigma = result.scalars.value(k).toDouble();
                else if (suf == QStringLiteral("_pass")) r.pass = result.scalars.value(k).toBool();
                break;
            }
        }
    }

    for (auto it = rows.cbegin(); it != rows.cend(); ++it) {
        const Row& r = it.value();
        out << r.parameter << ",";
        out << r.projectValue.toString() << ",";
        out << r.referenceValue.toString() << ",";
        out << r.observedValue.toString() << ",";
        out << r.observedSigma.toString() << ",";
        out << r.source << ",";
        out << r.unit << ",";
        out << QString::number(r.relErrorPct, 'f', 4) << ",";
        out << QString::number(r.tensionSigma, 'f', 3) << ",";
        out << (r.pass ? QStringLiteral("PASS") : QStringLiteral("FAIL")) << "\n";
    }
    out << QStringLiteral("\n# log:\n");
    for (const auto& line : result.logLines) out << line << "\n";
    file.close();

    return outPath;
}

QString ExperimentRunner::exportValidationReport(const QString& format) {
    QString fmt = format.toLower();
    if (fmt != "csv") {
        qWarning() << "exportValidationReport: unsupported format" << format;
        return QString();
    }

    ExperimentResult result;
    {
        QMutexLocker lock(&m_mutex);
        for (int i = static_cast<int>(m_history.size()) - 1; i >= 0; --i) {
            if (m_history[i].id == m_currentId) {
                result = m_history[i].result;
                break;
            }
        }
    }

    QString path = writeValidationReportCSV(result);

    qInfo("[EXPORT] %s", qPrintable(path));

    QMutexLocker lock(&m_mutex);
    for (auto& h : m_history) {
        if (h.id == m_currentId) h.result.outputDir = path;
    }

    emit exportFinished(path);
    qDebug() << "Validation report exported to" << path;
    return path;
}

QVariantList ExperimentRunner::history() const {
    QVariantList list;
    QMutexLocker lock(&m_mutex);
    for (const auto& h : m_history) {
        QVariantMap m;
        m["id"] = h.id;
        m["name"] = h.name;
        m["success"] = h.result.success;
        m["outputDir"] = h.result.outputDir;
        list.append(m);
    }
    return list;
}

void ExperimentRunner::showHistoryEntry(const QString& id) {
    QMutexLocker lock(&m_mutex);
    for (const auto& h : m_history) {
        if (h.id == id) {
            m_currentScalars = h.result.scalars;
            QVariantList curve;
            for (const auto& pt : h.result.curve) {
                QVariantMap mm; mm["x"] = pt.x(); mm["y"] = pt.y(); curve.append(mm);
            }
            m_currentCurve = curve;
            QString log;
            for (const auto& line : h.result.logLines) log += line + "\n";
            m_currentLog = log;
            m_currentId = h.id;
            m_currentName = h.name;
            m_running.store(false);
            m_progress.store(1.0);
            break;
        }
    }
    emit currentScalarsChanged();
    emit currentCurveChanged();
    emit currentLogChanged();
    emit currentIdChanged();
    emit currentNameChanged();
    emit progressChanged();
}

void ExperimentRunner::clearHistory() {
    QMutexLocker lock(&m_mutex);
    m_history.clear();
    emit historyChanged();
}

QVariantList ExperimentRunner::currentCurve() const {
    return m_currentCurve;
}

void ExperimentRunner::setProgress(double p) {
    m_progress.store(p);
    emit progressChanged();
}

void ExperimentRunner::setLog(const QString& log) {
    m_currentLog = log;
    emit currentLogChanged();
}

void ExperimentRunner::setResult(const ExperimentRunOutcome& outcome) {
    m_running.store(false);
    m_abort.store(false);
    m_progress.store(1.0);

    m_currentScalars = outcome.result.scalars;
    QVariantList curve;
    for (const auto& pt : outcome.result.curve) {
        QVariantMap m; m["x"] = pt.x(); m["y"] = pt.y(); curve.append(m);
    }
    m_currentCurve = curve;

    QString log;
    for (const auto& line : outcome.result.logLines) log += line + "\n";
    m_currentLog = log;

    {
        QMutexLocker lock(&m_mutex);
        ExperimentRunOutcome copy = outcome;
        m_history.append(copy);
        while (m_history.size() > 200) m_history.removeFirst();
    }

    emit runningChanged();
    emit progressChanged();
    emit currentScalarsChanged();
    emit currentCurveChanged();
    emit currentLogChanged();
    emit historyChanged();
    emit experimentFinished(outcome.id, outcome.result.success);
}

void ExperimentRunner::RunJob::run() {
    auto progressCb = [runner = m_runner](double p) {
        QMetaObject::invokeMethod(runner, "setProgress", Qt::QueuedConnection,
                                  Q_ARG(double, p));
    };
    auto abortCb = [runner = m_runner]() -> bool {
        return runner->m_abort.load();
    };

    ExperimentResult res = m_exp->run(m_params, progressCb, abortCb);

    ExperimentRunOutcome outcome;
    outcome.id = m_exp->id();
    outcome.name = m_exp->name();
    outcome.result = res;

    QMetaObject::invokeMethod(m_runner, "setResult", Qt::QueuedConnection,
                              Q_ARG(ExperimentRunOutcome, outcome));
}

} // namespace quantumverse
