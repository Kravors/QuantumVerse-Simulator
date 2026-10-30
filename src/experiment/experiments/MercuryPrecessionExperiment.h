#pragma once

/**
 * @file MercuryPrecessionExperiment.h
 * @brief GR validation: Mercury perihelion precession (expect ~43 arcsec/century).
 */

#ifndef QUANTUMVERSE_MERCURY_PRECESSION_EXPERIMENT_H
#define QUANTUMVERSE_MERCURY_PRECESSION_EXPERIMENT_H

#include "../Experiment.h"

namespace quantumverse {

class MercuryPrecessionExperiment : public Experiment {
public:
    QString id() const override { return QStringLiteral("mercury_precession"); }
    QString name() const override { return QStringLiteral("Mercury Perihelion Precession"); }
    QString category() const override { return QStringLiteral("GR Validation"); }
    QString description() const override {
        return QStringLiteral("Schwarzschild perihelion advance per century via the analytic first-post-Newtonian formula (GR predicts 43.0 arcsec/century). Validates the formula, not the integrator — the integrator is validated separately in test_validation.cpp.");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params,
                         std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_MERCURY_PRECESSION_EXPERIMENT_H
