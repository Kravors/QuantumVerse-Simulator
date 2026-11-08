#pragma once

/**
 * @file CosmologyExperimentBase.h
 * @brief Shared scaffolding for the cosmology validation experiments.
 *
 * Every check carries project_value, reference_value (computed on the same
 * parameters the project uses), observed_value (+σ from Planck/SH0ES/SNe
 * Ia), a relative error, a σ-tension, and a pass/fail flag.
 */

#ifndef QUANTUMVERSE_COSMOLOGY_EXPERIMENT_BASE_H
#define QUANTUMVERSE_COSMOLOGY_EXPERIMENT_BASE_H

#include "../Experiment.h"
#include "../cosmology/CosmologyProvider.h"
#include "../cosmology/CosmologyReferences.h"
#include "../cosmology/LambdaCDMReferenceProvider.h"

#include <memory>

namespace quantumverse {

/**
 * @brief One parameter check: computed vs reference vs observed.
 */
struct CosmologyCheck {
    QString parameter;          // e.g. "age_universe_Gyr"
    double projectValue = 0;    // project's cosmology
    double referenceValue = 0;  // reference impl, same parameters as project
    double observedValue = 0;   // Planck / SH0ES / SNe Ia / analytic
    double observedSigma = 0;   // 1σ (0 = exact / analytic)
    QString source;             // "Planck2018", ...
    QString unit;               // "Gyr", "Mpc", ...
    // derived, filled by makeCheck:
    double relErrorPct = 0;   // (project − reference) / reference · 100
    double tensionSigma = 0;  // (project − observed) / σ
    bool pass = false;        // per-experiment tolerance
};

/**
 * @brief Base class for validation experiments.
 *
 * Subclasses call makeCheck() for each quantity, then flatten the checks into
 * ExperimentResult::scalars with prefixed keys and append a log line.
 */
class CosmologyExperimentBase : public Experiment {
   public:
    explicit CosmologyExperimentBase(std::shared_ptr<CosmologyProvider> project)
        : m_project(std::move(project)) {}

   protected:
    /**
     * @brief Build a validated check.
     * @param param        stable key, e.g. "age_universe_Gyr"
     * @param projectVal   value from the project provider
     * @param referenceVal value from the reference provider at identical parameters
     * @param ref          observed reference (value, σ, source, unit)
     * @param passRelativePct tolerance for exact/analytic references (σ = 0)
     * @param passSigma     tolerance in σ for references with uncertainty
     */
    CosmologyCheck makeCheck(const QString& param, double projectVal, double referenceVal,
                             const CosmologyReference& ref, double passRelativePct = 1.0,
                             double passSigma = 3.0) const;

    /** Flatten a check into the result's scalars with prefixed keys. */
    void recordCheck(ExperimentResult& out, const CosmologyCheck& c) const;

   public:
    /** Reconfigure the reference provider (e.g. to match the project's parameters). */
    void setReferenceProvider(std::shared_ptr<CosmologyProvider> ref);

    std::shared_ptr<CosmologyProvider> m_project;
    std::shared_ptr<CosmologyProvider> m_reference = std::make_shared<LambdaCDMReferenceProvider>();

   protected:
    // No additional state.
};

}  // namespace quantumverse

#endif  // QUANTUMVERSE_COSMOLOGY_EXPERIMENT_BASE_H
