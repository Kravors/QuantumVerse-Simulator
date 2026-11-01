/**
 * @file BayesianEvidenceCalculator.cpp
 * @brief BIC-based Bayesian evidence calculator for model comparison.
 *
 * Convention (matches TheoryDiscoveryAgent::computeBayesFactor):
 *   logBF = (BIC_reference − BIC_candidate) / 2
 *   logBF > 0 → candidate favored (candidate fits better)
 *   logBF < 0 → reference favored
 *   |logBF| < 1 → inconclusive
 *
 * Invalid inputs → NaN (not silent 0), consistent with
 * DiscoveryEngine::calculateBayesFactor returning NaN as an unimplemented stub.
 */

#include "BayesianEvidenceCalculator.h"

#include <cmath>
#include <limits>

namespace quantumverse {

ModelEvidence BayesianEvidenceCalculator::computeModelEvidence(
    const std::string& name, const BICInputs& inputs)
{
    ModelEvidence ev;
    ev.name = name;
    ev.inputs = inputs;

    if (inputs.k < 0 || inputs.n <= 0 || !std::isfinite(inputs.logLik)) {
        ev.bic = std::numeric_limits<double>::quiet_NaN();
        ev.logEvidence = std::numeric_limits<double>::quiet_NaN();
        return ev;
    }

    ev.bic = static_cast<double>(inputs.k) * std::log(static_cast<double>(inputs.n))
           - 2.0 * inputs.logLik;
    ev.logEvidence = -0.5 * ev.bic; // −BIC/2 (up to model-independent constant)

    return ev;
}

BayesFactorResult BayesianEvidenceCalculator::compareModels(
    const ModelEvidence& reference, const ModelEvidence& candidate)
{
    BayesFactorResult r;
    r.reference = reference.name;
    r.candidate = candidate.name;

    if (!std::isfinite(reference.bic) || !std::isfinite(candidate.bic)) {
        r.logBF_natural = std::numeric_limits<double>::quiet_NaN();
        r.logBF_base10  = std::numeric_limits<double>::quiet_NaN();
        r.interpretation = "invalid";
        return r;
    }

    // Convention: logBF > 0 means CANDIDATE is favored.
    // BIC is −2 ln L̂ scale (lower is better). Candidate fits better iff
    // BIC_candidate < BIC_reference, i.e. (BIC_ref − BIC_cand) > 0.
    r.logBF_natural = (reference.bic - candidate.bic) / 2.0;
    r.logBF_base10  = r.logBF_natural / std::log(10.0);
    r.interpretation = interpret(r.logBF_natural);

    return r;
}

BayesFactorResult BayesianEvidenceCalculator::compareFromInputs(
    const std::string& refName, const BICInputs& refIn,
    const std::string& candName, const BICInputs& candIn)
{
    return compareModels(computeModelEvidence(refName, refIn),
                         computeModelEvidence(candName, candIn));
}

std::string BayesianEvidenceCalculator::interpret(double logBF_natural) {
    if (!std::isfinite(logBF_natural)) return "invalid";
    const double abs = std::abs(logBF_natural);
    if (abs < 1.0) return "inconclusive";
    if (abs < 3.0) return "positive";
    if (abs < 5.0) return "strong";
    return "very strong";
}

} // namespace quantumverse
