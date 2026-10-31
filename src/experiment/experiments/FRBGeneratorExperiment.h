#pragma once

#ifndef QUANTUMVERSE_FRB_GENERATOR_EXPERIMENT_H
#define QUANTUMVERSE_FRB_GENERATOR_EXPERIMENT_H

#include "../Experiment.h"

namespace quantumverse {

class FRBGeneratorExperiment : public Experiment {
public:
    QString id()          const override { return QStringLiteral("frb_generator"); }
    QString name()        const override { return QStringLiteral("FRB Dispersion Measure Generator"); }
    QString category()    const override { return QStringLiteral("Cosmology"); }
    QString description() const override {
        return QStringLiteral("Dispersion measure of FRB-like sources vs source redshift, "
                              "computed from the cosmic IGM electron column using the Macquart "
                              "2020 formula. Fixed Planck 2018 parameters.");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params,
                         std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_FRB_GENERATOR_EXPERIMENT_H
