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
#include <QStandardPaths>
#include <algorithm>
#include <thread>
#include <cmath>

namespace quantumverse {

ExperimentRunner::ExperimentRunner(QObject* parent)
    : QAbstractListModel(parent)
{
    m_pool.setMaxThreadCount(std::max<size_t>(1, std::thread::hardware_concurrency() / 2));

    m_paperGenerator = std::make_unique<PaperGenerator>(this);
    connect(m_paperGenerator.get(), &PaperGenerator::paperGenerated,
        this, &ExperimentRunner::pdfFinished);
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

QString ExperimentRunner::exportPDF(const QString& id, const QString& path /*= QString()*/) {
    // Find the latest history entry for this experiment id
    ExperimentResult result;
    QString experimentName;
    {
        QMutexLocker lock(&m_mutex);
        for (int i = static_cast<int>(m_history.size()) - 1; i >= 0; --i) {
            if (m_history[i].id == id) {
                result = m_history[i].result;
                experimentName = m_history[i].name;
                break;
            }
        }
    }

    if (result.scalars.isEmpty()) {
        qWarning() << "exportPDF: no result for experiment" << id;
        return QString();
    }

    // Compute a unique output path: <experimentId>_<timestamp>.pdf
    QString pdfPath = path;
    if (pdfPath.isEmpty()) {
        QString baseDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                        + "/QuantumVerse/output";
        QDir().mkpath(baseDir);
        const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        pdfPath = baseDir + QStringLiteral("/") + id + QStringLiteral("_") + stamp + QStringLiteral(".pdf");
    }

    // Map experiment scalars to PaperGenerator parameters.
    // Lens mass (M_sun) is the natural "mass"; redshift is directly available.
    // Spin/charge are N/A for lensing (0). Kretschmann uses Einstein radius as
    // a curvature proxy since lensing doesn't directly compute K.
    double mass = result.scalars.value(QStringLiteral("lens_mass_msun"), 0.0).toDouble();
    double redshift = result.scalars.value(QStringLiteral("lens_redshift"), 0.0).toDouble();
    double einsteinRadius = result.scalars.value(QStringLiteral("einstein_radius_arcsec"), 0.0).toDouble();
    double delayDays = result.scalars.value(QStringLiteral("reference_delay_days"), 0.0).toDouble();

    // Build a plain-English abstract from the scalars
    QString abstract = QStringLiteral(
        "Strong gravitational lensing by a point-mass lens of %1 M_sun at z = %2. "
        "The Einstein radius is %3 arcsec. The reference time delay at "
        "beta = 0.5 theta_E is %4 days.")
        .arg(QString::number(mass, 'f', 2))
        .arg(QString::number(redshift, 'f', 3))
        .arg(QString::number(einsteinRadius, 'f', 4))
        .arg(QString::number(delayDays, 'f', 2));

    // PaperGenerator::generatePaper is async (QProcess). We fire it and let
    // the paperGenerated signal propagate via pdfFinished. The caller can
    // listen to pdfFinished for the actual path.
    m_paperGenerator->generatePaper(
        mass, 0.0, 0.0, einsteinRadius, redshift,
        abstract, QStringLiteral("lensing"), pdfPath);

    return pdfPath;
}

const ExperimentResult* ExperimentRunner::lastResultFor(const QString& id) const {
    for (int i = static_cast<int>(m_history.size()) - 1; i >= 0; --i) {
        if (m_history[i].id == id && m_history[i].result.success) {
            return &m_history[i].result;
        }
    }
    return nullptr;
}

QString ExperimentRunner::abstractForExperiment(const QString& id,
                                                 const ExperimentResult& result) const {
    if (id == QStringLiteral("photon_sphere")) {
        const double mass = result.scalars.value(QStringLiteral("mass_Msun"), 0.0).toDouble();
        const double spin = result.scalars.value(QStringLiteral("spin_a_max"), 0.0).toDouble();
        const double kret = result.scalars.value(QStringLiteral("kretschmann_at_r_ph_pro"), 0.0).toDouble();
        return QStringLiteral(
            "Kerr black hole discovery scan: mass = %1 M_sun, spin a* = %2, "
            "Kretschmann scalar at prograde photon sphere = %3.")
            .arg(QString::number(mass, 'f', 2))
            .arg(QString::number(spin, 'f', 4))
            .arg(QString::number(kret, 'e', 4));
    }
    if (id == QStringLiteral("redshift_profile")) {
        const double mass = result.scalars.value(QStringLiteral("mass"), 0.0).toDouble();
        const double rMin = result.scalars.value(QStringLiteral("r_min"), 0.0).toDouble();
        const double z = result.scalars.value(QStringLiteral("z_at_r_min"), 0.0).toDouble();
        const double kret = result.scalars.value(QStringLiteral("kretschmann_at_r_min"), 0.0).toDouble();
        return QStringLiteral(
            "Gravitational redshift profile of a Schwarzschild black hole of mass "
            "%1 M_sun. At the minimum sampled radius r = %2 m, the observed redshift "
            "is z = %3. The Kretschmann curvature invariant at this radius is K = %4, "
            "consistent with the Schwarzschild vacuum solution K = 12 r_s^2 / r^6.")
            .arg(QString::number(mass, 'f', 2))
            .arg(QString::number(rMin, 'e', 4))
            .arg(QString::number(z, 'e', 4))
            .arg(QString::number(kret, 'e', 4));
    }
    if (id == QStringLiteral("frame_dragging")) {
        const double mass = result.scalars.value(QStringLiteral("mass_Msun"), 0.0).toDouble();
        const double spin = result.scalars.value(QStringLiteral("spin_a_star"), 0.0).toDouble();
        const double rOrb = result.scalars.value(QStringLiteral("orbit_radius_m"), 0.0).toDouble();
        const double kret = result.scalars.value(QStringLiteral("kretschmann_at_r_orbit"), 0.0).toDouble();
        const double masYr = result.scalars.value(QStringLiteral("last_mas_per_year"), 0.0).toDouble();
        return QStringLiteral(
            "Lense-Thirring frame-dragging precession around a Kerr black hole of "
            "mass %1 M_sun with dimensionless spin a* = %2. At orbital radius r = %3 m, "
            "the precession rate corresponds to %4 mas/year. The Kretschmann invariant "
            "at this radius is %5 m^-4.")
            .arg(QString::number(mass, 'f', 2))
            .arg(QString::number(spin, 'f', 4))
            .arg(QString::number(rOrb, 'e', 4))
            .arg(QString::number(masYr, 'f', 3))
            .arg(QString::number(kret, 'e', 4));
    }
    if (id == QStringLiteral("gw_ringdown")) {
        const double mass = result.scalars.value(QStringLiteral("mass_Msun"), 0.0).toDouble();
        const double spin = result.scalars.value(QStringLiteral("spin_a"), 0.0).toDouble();
        const double fR = result.scalars.value(QStringLiteral("f_R_Hz"), 0.0).toDouble();
        const double tau = result.scalars.value(QStringLiteral("tau_ms"), 0.0).toDouble();
        const double Q = result.scalars.value(QStringLiteral("Q_factor"), 0.0).toDouble();
        const double kret = result.scalars.value(QStringLiteral("kretschmann_at_r_plus"), 0.0).toDouble();
        return QStringLiteral(
            "GW ringdown detection: l=m=2, n=0 quasinormal mode of a Kerr black hole "
            "of mass %1 M_sun with dimensionless spin a* = %2. The fundamental mode has "
            "frequency $f_R = %3$ Hz and damping time $\\tau = %4$ ms (Q = %5). The "
            "Kretschmann invariant just outside the horizon is K = %6 m^-4, consistent "
            "with the Kerr vacuum solution.")
            .arg(QString::number(mass, 'f', 2))
            .arg(QString::number(spin, 'f', 4))
            .arg(QString::number(fR, 'e', 4))
            .arg(QString::number(tau, 'e', 4))
            .arg(QString::number(Q, 'f', 2))
            .arg(QString::number(kret, 'e', 4));
    }
    return QStringLiteral("Discovery paper for experiment %1.").arg(id);
}

bool ExperimentRunner::canBuildPaper(const QString& id) const {
    return ScanSummaryBuilder::canBuildFrom(id);
}

QString ExperimentRunner::generateDiscoveryPaper(const QString& id) {
    if (!ScanSummaryBuilder::canBuildFrom(id)) return QString();

    const auto* result = lastResultFor(id);
    if (!result) return QString();

    const auto summary = ScanSummaryBuilder::fromExperiment(id, *result);
    if (summary.mass <= 0.0) return QString(); // invalid sentinel

    const QString abstract = abstractForExperiment(id, *result);

    // Compute a unique output path: <id>_discovery_<timestamp>.pdf
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                    + "/QuantumVerse/output";
    QDir().mkpath(baseDir);
    const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    const QString pdfPath = baseDir + QStringLiteral("/") + id
                          + QStringLiteral("_discovery_") + stamp + QStringLiteral(".pdf");

    // Fire async pdflatex via PaperGenerator; pdfFinished signal carries
    // the actual result path. Caller polls for file existence.
    m_paperGenerator->generatePaper(summary, abstract, pdfPath);

    return pdfPath;
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
    // Iterate backwards to get the LATEST entry with this id, not the first.
    for (int i = static_cast<int>(m_history.size()) - 1; i >= 0; --i) {
        const auto& h = m_history[i];
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
