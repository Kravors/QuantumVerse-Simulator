#ifndef QUANTUMVERSE_THEORY_DISCOVERY_AGENT_H
#define QUANTUMVERSE_THEORY_DISCOVERY_AGENT_H

#include "RLDiscoveryAgent.h"
#include "TheoryParameterSpace.h"
#include "discovery/DiscoveryEngine.h"
#include "discovery/SymbolicMath.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "physics/CurvatureCalculator.h"
#include <memory>

namespace quantumverse {
namespace discovery {

/**
 * @brief Autonomous theory-discovery agent.
 *
 * Extends RLDiscoveryAgent to propose and validate modified-gravity theories
 * against observational data and theoretical consistency constraints.
 *
 * Reward is composed of:
 *   - Observational chi-squared (Pantheon+, GW170817, SLACS)
 *   - Theoretical penalties (ghost instabilities, superluminal propagation,
 *     Lorentzian signature violations, singularities)
 *   - Simplicity bonus (Occam penalty for extra parameters)
 */
class TheoryDiscoveryAgent : public RLDiscoveryAgent {
public:
    struct DiscoveryResult {
        std::vector<double> parameters;
        double total_reward = 0.0;
        double observational_chi2 = 0.0;
        double theoretical_penalty = 0.0;
        double simplicity_penalty = 0.0;
        bool lorentzian_valid = false;
        bool kretschmann_nonneg = false;
        bool near_singularity = false;
        std::string theory_name;
    };

    explicit TheoryDiscoveryAgent(
        TheoryParameterSpace::TheoryType theory_type = TheoryParameterSpace::TheoryType::FR_GRAVITY
    );

    ~TheoryDiscoveryAgent() = default;

    // Non-copyable
    TheoryDiscoveryAgent(const TheoryDiscoveryAgent&) = delete;
    TheoryDiscoveryAgent& operator=(const TheoryDiscoveryAgent&) = delete;

    /**
     * @brief Run a single theory evaluation with full physics validation.
     * @param params Theory parameters in normalized [0,1] space.
     * @return DiscoveryResult with reward components and validity flags.
     */
    DiscoveryResult evaluateTheory(const std::vector<double>& params) const;

    /**
     * @brief Discover the best theory parameters over max_steps episodes.
     * @param max_steps Maximum training episodes.
     * @return Best parameters found (denormalized physical values).
     */
    std::vector<double> discoverBestTheory(int max_steps = 100);

    /**
     * @brief Set the test location for theory evaluation.
     */
    void setTestLocation(const Event4D& location);

    /**
     * @brief Set the test metric radius (for spherically symmetric metrics).
     */
    void setTestRadius(double r);

    /**
     * @brief Enable or disable symbolic verification of field equations.
     */
    void setSymbolicVerificationEnabled(bool enabled);

    /**
     * @brief Get the best discovery result from the last run.
     */
    const DiscoveryResult& getBestResult() const;

    /**
     * @brief Result of Bayesian model comparison.
     */
    struct BayesianComparisonResult {
        double log_likelihood = 0.0;       ///< Log-likelihood from chi-squared
        double bic_candidate = 0.0;        ///< Bayesian Information Criterion for candidate
        double bic_baseline = 0.0;         ///< BIC for GR baseline
        double log_bayes_factor = 0.0;     ///< log(BF) where BF = P(data|candidate)/P(data|GR)
        double bayes_factor = 1.0;         ///< Bayes factor (clamped to avoid overflow)
        std::string preferred_model;       ///< "candidate", "GR", or "inconclusive"
        double evidence_ratio = 1.0;       ///< P(data|candidate) / P(data|GR)
    };

    /**
     * @brief Compute log-likelihood from observational chi-squared.
     * Uses Gaussian approximation: log L = -0.5 * chi2 * N (up to constant).
     * @param chi2 Observational chi-squared value.
     * @param n_data Number of data points.
     * @return Log-likelihood (higher is better).
     */
    double computeLogLikelihood(double chi2, int n_data) const;

    /**
     * @brief Compute Bayesian Information Criterion (BIC).
     * BIC = chi2 + k * log(N), where k = number of parameters, N = data points.
     * Lower BIC indicates better model after penalizing complexity.
     * @param chi2 Chi-squared value.
     * @param n_params Number of free parameters.
     * @param n_data Number of data points.
     * @return BIC value.
     */
    double computeBIC(double chi2, int n_params, int n_data) const;

    /**
     * @brief Compute Bayes factor comparing candidate theory to GR baseline.
     * Uses BIC approximation: log BF = (BIC_GR - BIC_candidate) / 2.
     * BF > 1 favors candidate; BF < 1 favors GR.
     * @param candidate_params Candidate theory parameters (normalized).
     * @return BayesianComparisonResult with all metrics.
     */
    BayesianComparisonResult computeBayesFactor(
        const std::vector<double>& candidate_params
    ) const;

    /**
     * @brief Compute GR baseline chi-squared for current theory type.
     * Returns chi2 for the pure-GR limit of the theory.
     * @return Chi-squared for GR baseline.
     */
    double computeGRBaselineChi2() const;

    // Helper: compute observational chi-squared for a given theory metric.
    double computeObservationalChi2(
        const MetricTensor& metric,
        const std::map<std::string, double>& params
    ) const;

private:
    // Override to provide physics-grounded simulation
    RLState runSimulation(const std::vector<double>& params) const;

    // Helper: validate metric invariants
    bool validateMetricInvariants(const MetricTensor& metric) const;

    // Helper: compute theoretical penalty for pathological behaviour
    double computeTheoreticalPenalty(const MetricTensor& metric) const;

    // Helper: instantiate a theory plugin from parameter vector
    std::unique_ptr<TheoryPlugin> instantiateTheory(
        const std::vector<double>& params
    ) const;

    // Helper: compute GW170817 time-delay chi2 contribution
    double computeGW170817Chi2(
        const MetricTensor& metric,
        const std::map<std::string, double>& params
    ) const;

    TheoryParameterSpace param_space_;
    Event4D test_location_;
    double test_radius_;
    bool symbolic_verification_enabled_;
    mutable DiscoveryResult best_result_;
    mutable double best_reward_so_far_;
};

} // namespace discovery
} // namespace quantumverse

#endif // QUANTUMVERSE_THEORY_DISCOVERY_AGENT_H
