#pragma once

#ifndef QUANTUMVERSE_LENSING_TIME_DELAY_EXPERIMENT_H
#define QUANTUMVERSE_LENSING_TIME_DELAY_EXPERIMENT_H

#include "../Experiment.h"

namespace quantumverse {

class LensingTimeDelayExperiment : public Experiment {
public:
    QString id() const override { return QStringLiteral("lensing_time_delay"); }
    QString name() const override { return QStringLiteral("Strong-Lensing Time Delay (Point Mass)"); }
    QString category() const override { return QStringLiteral("Cosmology"); }
    QString description() const override {
        return QStringLiteral("Point-mass thin lens: Einstein radius, image positions, and "
                              "time delay vs source offset beta. Fixed cosmological distances; "
                              "analytic thin-lens formulas.");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params,
                         std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_LENSING_TIME_DELAY_EXPERIMENT_H
