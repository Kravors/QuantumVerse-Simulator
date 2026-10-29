#pragma once

/**
 * @file CMBTemperatureExperiment.h
 * @brief Cosmology validation: CMB temperature T(z) = T₀(1+z).
 */

#ifndef QUANTUMVERSE_CMB_TEMPERATURE_EXPERIMENT_H
#define QUANTUMVERSE_CMB_TEMPERATURE_EXPERIMENT_H

#include "../Experiment.h"
#include "../cosmology/CosmologyProvider.h"
#include "CosmologyExperimentBase.h"

namespace quantumverse {

class CMBTemperatureExperiment : public CosmologyExperimentBase {
public:
    using CosmologyExperimentBase::CosmologyExperimentBase;

    QString id() const override { return QStringLiteral("cmb_temperature"); }
    QString name() const override { return QStringLiteral("CMB Temperature"); }
    QString category() const override { return QStringLiteral("Cosmology"); }
    QString description() const override {
        return QStringLiteral("T_CMB today vs COBE/FIRAS and T(z=1100) vs T₀(1+z); full T(z) curve z = 0..2000.");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params,
                         std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_CMB_TEMPERATURE_EXPERIMENT_H
