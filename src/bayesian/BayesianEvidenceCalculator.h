#pragma once

#ifndef QUANTUMVERSE_BAYESIAN_EVIDENCE_CALCULATOR_H
#define QUANTUMVERSE_BAYESIAN_EVIDENCE_CALCULATOR_H

#include <string>

namespace quantumverse {

/**
 * @brief BIC-based Bayesian evidence for model comparison.
 *
 * BIC = k · ln(n) − 2 · ln(L̂)   (k = free parameters, n = data points,
 * L̂ = max likelihood)
 *
 * Bayes factor convention (matches TheoryDiscoveryAgent::computeBayesFactor):
 *   logBF = (BIC_reference − BIC_candidate) / 2
 *   logBF > 0   → candidate favored (candidate fits better)
 *   logBF < 0   → reference favored
 *   |logBF| < 1 → inconclusive (Kass & Raftery 1995)
 *
 * This sign convention is the single most common bug source. It is pinned
 * explicitly in compareModels() and tested in both directions.
 */

struct BICInputs {
    int     k       = 0;    // number of free parameters
    int     n       = 0;    // number of data points
    double  logLik  = 0.0;  // maximum log-likelihood at best-fit parameters
};

struct ModelEvidence {
    std::string  name;
    BICInputs    inputs;
    double       bic        = 0.0;
    double       logEvidence = 0.0;   // −BIC/2 (up to model-independent constant)
};

struct BayesFactorResult {
    std::string  reference;
    std::string  candidate;
    double       logBF_natural = 0.0; // ln BF; >0 → candidate favored
    double       logBF_base10  = 0.0; // log10 BF = logBF_natural / ln(10)
    std::string  interpretation;      // "inconclusive" | "positive" | "strong" | "very strong"
};

class BayesianEvidenceCalculator {
public:
    // Compute a single model's BIC-based evidence.
    // Invalid inputs (k<0, n<=0, non-finite logLik) → NaN bic/logEvidence.
    // k=0 is valid (e.g. GR baseline with no free parameters).
    static ModelEvidence computeModelEvidence(const std::string& name,
                                              const BICInputs& inputs);

    // Compare reference vs candidate models.
    // Convention: logBF_natural > 0 means CANDIDATE is favored.
    static BayesFactorResult compareModels(const ModelEvidence& reference,
                                           const ModelEvidence& candidate);

    // Convenience: build ModelEvidence from raw inputs then compare.
    static BayesFactorResult compareFromInputs(const std::string& refName,
                                               const BICInputs& refInputs,
                                               const std::string& candName,
                                               const BICInputs& candInputs);

    // Kass & Raftery (1995) interpretation on |ln BF|.
    static std::string interpret(double logBF_natural);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_BAYESIAN_EVIDENCE_CALCULATOR_H
