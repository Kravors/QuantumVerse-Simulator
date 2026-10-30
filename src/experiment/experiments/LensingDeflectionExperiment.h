#pragma once

#ifndef QUANTUMVERSE_LENSING_DEFLECTION_EXPERIMENT_H
#define QUANTUMVERSE_LENSING_DEFLECTION_EXPERIMENT_H

#include "../Experiment.h"

namespace quantumverse {

class LensingDeflectionExperiment : public Experiment {
public:
    QString id() const override { return QStringLiteral("lensing_deflection"); }
    QString name() const override { return QStringLiteral("Light Deflection vs Impact Parameter"); }
    QString category() const override { return QStringLiteral("GR Validation"); }
    QString description() const override {
        return QStringLiteral("Schwarzschild weak-field deflection angle alpha = 4GM/(c^2 b) vs impact parameter b.");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params,
                         std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_LENSING_DEFLECTION_EXPERIMENT_H
