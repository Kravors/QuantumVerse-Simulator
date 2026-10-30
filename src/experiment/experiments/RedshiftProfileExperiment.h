#pragma once

/**
 * @file RedshiftProfileExperiment.h
 * @brief Gravitational redshift profile z(r) for a Schwarzschild metric.
 */

#ifndef QUANTUMVERSE_REDSHIFT_PROFILE_EXPERIMENT_H
#define QUANTUMVERSE_REDSHIFT_PROFILE_EXPERIMENT_H

#include "../Experiment.h"

namespace quantumverse {

class RedshiftProfileExperiment : public Experiment {
public:
    QString id() const override { return QStringLiteral("redshift_profile"); }
    QString name() const override { return QStringLiteral("Gravitational Redshift Profile"); }
    QString category() const override { return QStringLiteral("GR Validation"); }
    QString description() const override {
        return QStringLiteral("z(r) = 1/sqrt(-g_tt) - 1 vs radius for a Schwarzschild black hole.");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params,
                         std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_REDSHIFT_PROFILE_EXPERIMENT_H
