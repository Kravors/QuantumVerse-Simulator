#pragma once

/**
 * @file GrowthFactorExperiment.h
 * @brief Cosmology validation: linear growth factor D(z) vs Planck 2018.
 */

#ifndef QUANTUMVERSE_GROWTH_FACTOR_EXPERIMENT_H
#define QUANTUMVERSE_GROWTH_FACTOR_EXPERIMENT_H

#include "../Experiment.h"
#include "../cosmology/CosmologyProvider.h"
#include "CosmologyExperimentBase.h"

namespace quantumverse {

class GrowthFactorExperiment : public CosmologyExperimentBase {
public:
    using CosmologyExperimentBase::CosmologyExperimentBase;

    QString id() const override { return QStringLiteral("growth_factor"); }
    QString name() const override { return QStringLiteral("Growth Factor"); }
    QString category() const override { return QStringLiteral("Cosmology"); }
    QString description() const override {
        return QStringLiteral("Linear growth factor D(1)/D(0) vs Planck 2018; full D(z) curve z = 0..10.");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params,
                         std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_GROWTH_FACTOR_EXPERIMENT_H
