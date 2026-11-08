#pragma once

/**
 * @file BAOSoundHorizonExperiment.h
 * @brief Cosmology validation: sound horizon at drag r_drag vs Planck 2018.
 */

#ifndef QUANTUMVERSE_BAO_SOUND_HORIZON_EXPERIMENT_H
#define QUANTUMVERSE_BAO_SOUND_HORIZON_EXPERIMENT_H

#include "../Experiment.h"
#include "../cosmology/CosmologyProvider.h"
#include "CosmologyExperimentBase.h"

namespace quantumverse {

class BAOSoundHorizonExperiment : public CosmologyExperimentBase {
   public:
    using CosmologyExperimentBase::CosmologyExperimentBase;

    QString id() const override {
        return QStringLiteral("bao_sound_horizon");
    }
    QString name() const override {
        return QStringLiteral("BAO Sound Horizon");
    }
    QString category() const override {
        return QStringLiteral("Cosmology");
    }
    QString description() const override {
        return QStringLiteral(
            "Sound horizon at the drag epoch r_drag vs Planck 2018; c_s(z)/H(z) curve.");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params, std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;
};

}  // namespace quantumverse

#endif  // QUANTUMVERSE_BAO_SOUND_HORIZON_EXPERIMENT_H
