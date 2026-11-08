/**
 * @file TheorySensitivityExperiment.cpp
 * @brief Theory × experiment σ-tension matrix for the discovery paper.
 *
 * Runs CosmologyValidationSuite against each theory (project ΛCDM,
 * f(R) b=0.3, wCDM w0=-0.9, Brans-Dicke ω=50) and records every
 * sub-experiment's σ-tension in scalars keyed by
 * "theory_parameter_tension".
 *
 * The matrix output feeds PaperGenerator: abstract text + table values
 * for the theory-sensitivity narrative paper.
 */

#include "TheorySensitivityExperiment.h"

#include <QDateTime>

namespace quantumverse {

TheorySensitivityExperiment::TheorySensitivityExperiment() : Experiment() {}

QVector<ExperimentParameter> TheorySensitivityExperiment::parameters() const {
    QVector<ExperimentParameter> p;
    p.append({QStringLiteral("passSigma"), QStringLiteral("Pass tolerance (σ)"),
              QStringLiteral("double"), QVariant(3.0), QVariant(1.0), QVariant(5.0), QStringList(),
              QStringLiteral("σ"), QStringLiteral("Tolerance for σ-referenced checks.")});
    return p;
}

ExperimentResult TheorySensitivityExperiment::run(const QVariantMap& params,
                                                  std::function<void(double)> progress,
                                                  std::function<bool()> shouldAbort) {
    ExperimentResult out;
    out.success = true;

    double passSigma = params.value(QStringLiteral("passSigma"), 3.0).toDouble();

    CosmologyValidationSuite suite;

    const int nTheories = static_cast<int>(m_theories.size());
    const int nSub = 6;  // age, distance, cmb, bao, hubble, growth
    const int total = nTheories * nSub;
    int completed = 0;

    for (const QString& theory : m_theories) {
        if (shouldAbort()) {
            out.success = false;
            return out;
        }

        QVariantMap suiteParams;
        suiteParams[QStringLiteral("theory_choice")] = theory;
        suiteParams[QStringLiteral("passSigma")] = passSigma;

        ExperimentResult suiteResult = suite.run(suiteParams, nullptr, shouldAbort);
        if (shouldAbort()) {
            out.success = false;
            return out;
        }

        // Record each sub-experiment's scalars with theory prefix.
        for (auto it = suiteResult.scalars.cbegin(); it != suiteResult.scalars.cend(); ++it) {
            const QString key = it.key();
            // Only keep tension, pass, and rel_err scalars — skip timestamps,
            // provider names, and other metadata.
            if (key.contains(QStringLiteral("_tension")) || key.contains(QStringLiteral("_pass")) ||
                key.contains(QStringLiteral("_rel_err"))) {
                out.scalars[theory + QStringLiteral("_") + key] = it.value();
            }
        }

        // Also record which theory ran.
        out.scalars[theory + QStringLiteral("_suite_theory")] = theory;
        out.scalars[theory + QStringLiteral("_suite_provider")] =
            suiteResult.scalars.value(QStringLiteral("suite_provider"));

        completed += nSub;
        if (progress)
            progress(static_cast<double>(completed) / total);

        out.logLines << QString("Theory %1: %2 sub-experiments complete").arg(theory).arg(nSub);
    }

    const QString stamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    out.scalars[QStringLiteral("matrix_timestamp")] = stamp;
    out.scalars[QStringLiteral("matrix_theories")] = m_theories.join(QStringLiteral(","));
    out.scalars[QStringLiteral("matrix_n_theories")] = nTheories;
    out.scalars[QStringLiteral("matrix_n_sub_experiments")] = nSub;
    out.scalars[QStringLiteral("matrix_pass_sigma")] = passSigma;

    out.logLines << QString("Theory sensitivity matrix complete: %1 theories × %2 experiments")
                        .arg(nTheories)
                        .arg(nSub);
    out.logLines << QString("Generated: %1").arg(stamp);

    return out;
}

}  // namespace quantumverse
