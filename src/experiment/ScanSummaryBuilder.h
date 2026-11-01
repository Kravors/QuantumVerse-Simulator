#pragma once

/**
 * @file ScanSummaryBuilder.h
 * @brief Converts an ExperimentResult into a discovery ScanSummary.
 *
 * Bridges the experiment framework (ExperimentResult: scalars + curve)
 * with the discovery pipeline (ScanSummary: mass, spin, charge, ...).
 *
 * Design discipline (mirrors TheoryDiscoveryAgent::calculateBayesFactor → NaN):
 *   * Explicit capability gate (canBuildFrom) — never fabricate a paper for
 *     an experiment that doesn't emit BH observables.
 *   * requireScalar fails loudly if a needed scalar is absent — no silent
 *     defaults that would make every paper say "Kerr, 10 M☉, K = 0".
 */

#include <QString>

#include "experiment/Experiment.h"
#include "discovery/ScanSummary.h"

namespace quantumverse {

class ScanSummaryBuilder {
public:
    /** True only for experiments that populate BH observables in scalars. */
    static bool canBuildFrom(const QString& experimentId);

    /**
     * @brief Produce a ScanSummary from experiment output.
     * @pre canBuildFrom(experimentId) == true
     * @return An "invalid" ScanSummary (mass == 0, kretschmann NaN) if the
     *         experiment failed or a required scalar is missing — never a
     *         plausible-but-wrong summary.
     */
    static ScanSummary fromExperiment(const QString& experimentId,
                                      const ExperimentResult& result);

private:
    /** Returns scalars[key] if present and finite; sets *ok=false otherwise. */
    static double requireScalar(const ExperimentResult& r,
                                const QString& key, bool* ok);
};

} // namespace quantumverse
