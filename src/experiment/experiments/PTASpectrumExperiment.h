#pragma once

#ifndef QUANTUMVERSE_PTA_SPECTRUM_EXPERIMENT_H
#define QUANTUMVERSE_PTA_SPECTRUM_EXPERIMENT_H

#include "../Experiment.h"

namespace quantumverse {

class PTASpectrumExperiment : public Experiment {
public:
    QString id()          const override { return QStringLiteral("pta_spectrum"); }
    QString name()        const override { return QStringLiteral("PTA Spectrum (SMBHB Background)"); }
    QString category()    const override { return QStringLiteral("Gravitational Waves"); }
    QString description() const override {
        return QStringLiteral("Pulsar Timing Array characteristic strain h_c(f) for the "
                              "supermassive black hole binary stochastic background. "
                              "Power law h_c(f) = A * (f/f_yr)^alpha with alpha = -2/3.");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params,
                         std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_PTA_SPECTRUM_EXPERIMENT_H
