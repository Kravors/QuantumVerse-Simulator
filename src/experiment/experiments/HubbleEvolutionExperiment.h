#pragma once

/**
 * @file HubbleEvolutionExperiment.h
 * @brief Cosmology validation: H(z) evolution and the H₀ tension.
 */

#ifndef QUANTUMVERSE_HUBBLE_EVOLUTION_EXPERIMENT_H
#define QUANTUMVERSE_HUBBLE_EVOLUTION_EXPERIMENT_H

#include "../Experiment.h"
#include "../cosmology/CosmologyProvider.h"
#include "CosmologyExperimentBase.h"

namespace quantumverse {

class HubbleEvolutionExperiment : public CosmologyExperimentBase {
   public:
    using CosmologyExperimentBase::CosmologyExperimentBase;

    QString id() const override {
        return QStringLiteral("hubble_evolution");
    }
    QString name() const override {
        return QStringLiteral("Hubble Evolution");
    }
    QString category() const override {
        return QStringLiteral("Cosmology");
    }
    QString description() const override {
        return QStringLiteral(
            "H₀ vs Planck and SH0ES (tension made explicit), q(0), plus H(z) and q(z) curves.");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params, std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;
};

}  // namespace quantumverse

#endif  // QUANTUMVERSE_HUBBLE_EVOLUTION_EXPERIMENT_H
