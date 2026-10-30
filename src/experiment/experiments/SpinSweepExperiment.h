#pragma once

#ifndef QUANTUMVERSE_SPIN_SWEEP_EXPERIMENT_H
#define QUANTUMVERSE_SPIN_SWEEP_EXPERIMENT_H

#include "../Experiment.h"

namespace quantumverse {

class SpinSweepExperiment : public Experiment {
public:
    QString id() const override { return QStringLiteral("spin_sweep"); }
    QString name() const override { return QStringLiteral("Black Hole Spin Sweep"); }
    QString category() const override { return QStringLiteral("Sweep"); }
    QString description() const override {
        return QStringLiteral("Frame-dragging angular velocity and Kretschmann scalar vs dimensionless spin at a fixed radius.");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params,
                         std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_SPIN_SWEEP_EXPERIMENT_H
