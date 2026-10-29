#pragma once

/**
 * @file CosmologyValidationSuite.h
 * @brief Meta-experiment: runs all six cosmology validation experiments and
 *        returns the concatenated project / reference / observed checks.
 *
 * The suite builds a project provider (ProjectCosmologyAdapter wrapping the
 * project's FRWMetric) and a first-principles LambdaCDMReferenceProvider from
 * the same user parameters, so every check isolates code/parameter agreement
 * (project vs reference at identical parameters) from observed tension
 * (project vs Planck / SH0ES / SNe Ia).
 *
 * NOTE: until the project's FLRW solver is complete, the derived quantities are
 * computed with first-principles integrals inside the adapter, so the suite is
 * a regression test for the reference harness, not yet a validation of
 * independent project physics.
 */

#ifndef QUANTUMVERSE_COSMOLOGY_VALIDATION_SUITE_H
#define QUANTUMVERSE_COSMOLOGY_VALIDATION_SUITE_H

#include "../Experiment.h"
#include "../cosmology/CosmologyProvider.h"
#include "../cosmology/LambdaCDMReferenceProvider.h"
#include "../cosmology/ProjectCosmologyAdapter.h"
#include "../experiments/CosmologyExperimentBase.h"

#include <memory>

namespace quantumverse {

class AgeOfUniverseExperiment;
class DistanceLadderExperiment;
class CMBTemperatureExperiment;
class BAOSoundHorizonExperiment;
class HubbleEvolutionExperiment;
class GrowthFactorExperiment;

class CosmologyValidationSuite : public Experiment {
public:
    CosmologyValidationSuite();

    QString id() const override { return QStringLiteral("cosmology_validation_suite"); }
    QString name() const override { return QStringLiteral("Cosmology Validation Suite"); }
    QString category() const override { return QStringLiteral("Cosmology"); }
    QString description() const override {
        return QStringLiteral("Run all six cosmology experiments and export a fixed-schema validation report (CSV).");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params,
                         std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;

private:
    double m_H0 = 67.66, m_Om = 0.3111, m_Ob = 0.049, m_Or = 9.1e-5,
           m_OL = 0.6889, m_Tcmb = 2.7255;
    double m_passSigma = 3.0;
    double m_Ok() const { return 1.0 - m_Om - m_Or - m_OL; }
};

} // namespace quantumverse

#endif // QUANTUMVERSE_COSMOLOGY_VALIDATION_SUITE_H
