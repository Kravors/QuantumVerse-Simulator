#pragma once

/**
 * @file MassSweepExperiment.h
 * @brief Sweep black-hole mass and record Kretschmann scalar + redshift at a
 * fixed probe radius.
 */

#ifndef QUANTUMVERSE_MASS_SWEEP_EXPERIMENT_H
#define QUANTUMVERSE_MASS_SWEEP_EXPERIMENT_H

#include "../Experiment.h"

namespace quantumverse {

class MassSweepExperiment : public Experiment {
public:
    QString id() const override { return QStringLiteral("mass_sweep"); }
    QString name() const override { return QStringLiteral("Black Hole Mass Sweep"); }
    QString category() const override { return QStringLiteral("Sweep"); }
    QString description() const override {
        return QStringLiteral("Kretschmann scalar and redshift vs black-hole mass at a fixed probe radius.");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params,
                         std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_MASS_SWEEP_EXPERIMENT_H
