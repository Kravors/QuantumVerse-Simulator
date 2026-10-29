#pragma once

/**
 * @file DistanceLadderExperiment.h
 * @brief Cosmology validation: distance modulus μ(z) vs SNe Ia (Union2.1).
 */

#ifndef QUANTUMVERSE_DISTANCE_LADDER_EXPERIMENT_H
#define QUANTUMVERSE_DISTANCE_LADDER_EXPERIMENT_H

#include "../Experiment.h"
#include "../cosmology/CosmologyProvider.h"
#include "CosmologyExperimentBase.h"

namespace quantumverse {

class DistanceLadderExperiment : public CosmologyExperimentBase {
public:
    using CosmologyExperimentBase::CosmologyExperimentBase;

    QString id() const override { return QStringLiteral("distance_ladder"); }
    QString name() const override { return QStringLiteral("Distance Ladder"); }
    QString category() const override { return QStringLiteral("Cosmology"); }
    QString description() const override {
        return QStringLiteral("Distance modulus μ(z) at z = 0.1, 0.5, 1.0 vs Union2.1 SNe Ia fit; full d_L, d_A, d_C curves.");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params,
                         std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_DISTANCE_LADDER_EXPERIMENT_H
