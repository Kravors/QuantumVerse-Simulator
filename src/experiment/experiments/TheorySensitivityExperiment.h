#pragma once

/**
 * @file TheorySensitivityExperiment.h
 * @brief Meta-experiment: runs CosmologyValidationSuite against each
 * cosmology theory and collects the theory × experiment σ-tension matrix.
 *
 * Runs all 6 cosmology sub-experiments against each of 4 theories
 * (project ΛCDM, fR b=0.3, wCDM w0=-0.9, Brans-Dicke ω=50) and
 * records every sub-experiment's σ-tension in a flat scalar map keyed
 * by "theory_parameter_tension", e.g. "fR_r_drag_Mpc_tension = -37.07".
 *
 * This is the empirical content of the theory-sensitivity paper: a
 * 4×6 matrix showing which probe discriminates which theory.
 */

#ifndef QUANTUMVERSE_THEORY_SENSITIVITY_EXPERIMENT_H
#define QUANTUMVERSE_THEORY_SENSITIVITY_EXPERIMENT_H

#include "../Experiment.h"
#include "../experiments/CosmologyValidationSuite.h"

namespace quantumverse {

class TheorySensitivityExperiment : public Experiment {
public:
    TheorySensitivityExperiment();

    QString id() const override { return QStringLiteral("theory_sensitivity"); }
    QString name() const override { return QStringLiteral("Theory Sensitivity Matrix"); }
    QString category() const override { return QStringLiteral("Cosmology"); }
    QString description() const override {
        return QStringLiteral("Run all 6 cosmology experiments against 4 theories "
                              "(ΛCDM, f(R) b=0.3, wCDM w0=-0.9, Brans-Dicke ω=50) "
                              "and collect the theory × experiment σ-tension matrix "
                              "for the discovery paper.");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params,
                         std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;

private:
    QStringList m_theories = {
        QStringLiteral("project"),
        QStringLiteral("fR"),
        QStringLiteral("wcdm"),
        QStringLiteral("brans_dicke")
    };
};

} // namespace quantumverse

#endif // QUANTUMVERSE_THEORY_SENSITIVITY_EXPERIMENT_H
