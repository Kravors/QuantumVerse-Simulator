#pragma once

/**
 * @file ChargeSweepExperiment.h
 * @brief Reissner–Nordström charge sweep: horizons vs Q/M.
 */

#ifndef QUANTUMVERSE_CHARGE_SWEEP_EXPERIMENT_H
#define QUANTUMVERSE_CHARGE_SWEEP_EXPERIMENT_H

#include "../Experiment.h"

namespace quantumverse {

class ChargeSweepExperiment : public Experiment {
   public:
    QString id() const override {
        return QStringLiteral("charge_sweep");
    }
    QString name() const override {
        return QStringLiteral("Charge Sweep (Reissner–Nordström)");
    }
    QString category() const override {
        return QStringLiteral("Sweep");
    }
    QString description() const override {
        return QStringLiteral(
            "Sweep charge ratio Q/M from 0 (Schwarzschild) to 1 (extremal); plots inner/outer "
            "horizons.");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params, std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;
};

}  // namespace quantumverse

#endif  // QUANTUMVERSE_CHARGE_SWEEP_EXPERIMENT_H
