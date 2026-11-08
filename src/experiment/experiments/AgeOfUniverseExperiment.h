#pragma once

/**
 * @file AgeOfUniverseExperiment.h
 * @brief Cosmology validation: age of the Universe vs Planck 2018.
 */

#ifndef QUANTUMVERSE_AGE_OF_UNIVERSE_EXPERIMENT_H
#define QUANTUMVERSE_AGE_OF_UNIVERSE_EXPERIMENT_H

#include "../Experiment.h"
#include "../cosmology/CosmologyProvider.h"
#include "CosmologyExperimentBase.h"

namespace quantumverse {

class AgeOfUniverseExperiment : public CosmologyExperimentBase {
   public:
    using CosmologyExperimentBase::CosmologyExperimentBase;

    QString id() const override {
        return QStringLiteral("age_of_universe");
    }
    QString name() const override {
        return QStringLiteral("Age of the Universe");
    }
    QString category() const override {
        return QStringLiteral("Cosmology");
    }
    QString description() const override {
        return QStringLiteral("Age at z = 0 and z = 1 vs Planck 2018; full age curve z = 0..10.");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params, std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;
};

}  // namespace quantumverse

#endif  // QUANTUMVERSE_AGE_OF_UNIVERSE_EXPERIMENT_H
