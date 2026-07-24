#ifndef QUANTUMVERSE_THEORY_DISCOVERY_AGENT_H
#define QUANTUMVERSE_THEORY_DISCOVERY_AGENT_H

#include "RLDiscoveryAgent.h"
#include "TheoryParameterSpace.h"
#include "discovery/DiscoveryEngine.h"
#include "discovery/SymbolicMath.h"
#include "discovery/TheorySurrogate.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "physics/CurvatureCalculator.h"
#include <QJsonObject>
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
        double raw_observational_chi2 = 0.0;
        int n_data_points = 0;
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
     * @brief Denormalize parameters from [-1,1] to physical space.
     * @param normalized Normalized parameter vector.
     * @return Physical parameter vector.
     */
    std::vector<double> denormalizeParams(const std::vector<double>& normalized) const {
        return RLDiscoveryAgent::denormalizeParams(normalized);
    }

    /**
     * @brief Update the agent's dataset with a new multi-messenger alert.
     * @param alert JSON object following the GCN schema (LIGO, Fermi, etc.)
     *
     * The method parses the alert, extracts any usable cosmological
     * information (luminosity distance, redshift, time delay), appends
     * it to the internal dataset, and recomputes the Bayesian evidence
     * for the current best theory.
     */
    void ingestLiveAlert(const QJsonObject& alert);

    /**
     * @brief Get the best discovery result from the last run.
     */
    const DiscoveryResult& getBestResult() const;

    /**
     * @brief Active-learning configuration.
     */
    enum class ActiveLearningMode {
        RANDOM,        ///< Pure random exploration (default)
        UNCERTAINTY,   ///< Select points with highest predictive variance
        EI             ///< Expected improvement over current best
    };

    /**
     * @brief Acquisition function for active learning and multi-objective optimization.
     */
    enum class AcquisitionMode {
        RANDOM,        ///< Pure random exploration
        UNCERTAINTY,   ///< Select points with highest predictive variance
        EI,            ///< Expected improvement over current best
        EHVI,          ///< Expected hypervolume improvement (multi-objective)
        UCB            ///< Upper confidence bound (scalarized)
    };

    /**
     * @brief Enable or disable active learning.
     * When enabled, discoverBestTheory uses the surrogate to guide search.
     * @param enabled Whether to use active learning.
     * @param mode Selection strategy (uncertainty sampling or EI).
     */
    void setActiveLearningEnabled(bool enabled, ActiveLearningMode mode = ActiveLearningMode::UNCERTAINTY);

    /**
     * @brief Set the acquisition mode for active learning point selection.
     * @param mode Acquisition strategy (EHVI for multi-objective, UCB for scalarized).
     */
    void setAcquisitionMode(AcquisitionMode mode);

    /**
     * @brief Check if active learning is enabled.
     */
    bool isActiveLearningEnabled() const { return active_learning_enabled_; }

    /**
     * @brief Number of evaluated theories in the current active-learning history.
     */
    size_t getEvaluationCount() const { return evaluation_history_.size(); }

    /**
     * @brief Get the surrogate model used for active learning.
     */
    const class TheorySurrogate* getSurrogate() const { return surrogate_.get(); }

    /**
     * @brief Clear active-learning history and retrain from scratch.
     */
    void resetActiveLearning();

    /**
     * @brief Select next point to evaluate using the active-learning acquisition function.
     * Uses the currently configured acquisition mode.
     * @return Candidate parameter vector in normalized [0,1] space.
     */
    std::vector<double> selectNextActiveLearningPoint() const;

    /**
     * @brief Select next point to evaluate using the specified acquisition mode.
     * @param mode Acquisition strategy (EHVI for multi-objective, UCB for scalarized).
     * @return Candidate parameter vector in normalized [0,1] space.
     */
    std::vector<double> selectNextActiveLearningPoint(AcquisitionMode mode) const;

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
     * @brief Get the last Bayesian comparison result from a live alert update.
     */
    const BayesianComparisonResult& getLastBayesResult() const;

    /**
     * @brief Number of live observations currently ingested.
     */
    size_t getLiveObservationCount() const { return m_liveObservations.size(); }

    /**
     * @brief Compute Bayesian posterior model weights for the current Pareto front.
     * Uses BIC approximation: w_i = exp(-BIC_i/2) / sum_j exp(-BIC_j/2).
     * @return Vector of posterior weights (sum to 1).
     */
    std::vector<double> computeModelWeights() const;

    /**
     * @brief Get the last computed model weights.
     */
    std::vector<double> getModelWeights() const { return model_weights_; }

    /**
     * @brief BMA prediction: weighted average of a scalar quantity across the Pareto front.
     * @param quantity Lambda extracting a scalar from a DiscoveryResult.
     * @return BMA-averaged prediction.
     */
    double predictBMA(const std::function<double(const DiscoveryResult&)>& quantity) const;

    /**
     * @brief BMA predictive variance: model uncertainty in the prediction.
     * @param quantity Lambda extracting a scalar from a DiscoveryResult.
     * @return Variance of the BMA prediction.
     */
    double predictiveVarianceBMA(const std::function<double(const DiscoveryResult&)>& quantity) const;

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

    /**
     * @brief Multi-objective Pareto point.
     */
    struct ParetoPoint {
        std::vector<double> parameters;
        std::vector<double> objectives;
        double total_reward = 0.0;
        bool lorentzian_valid = false;
        bool near_singularity = false;
        std::string theory_name;

        ParetoPoint() = default;
        ParetoPoint(
            const std::vector<double>& params,
            const std::vector<double>& objs,
            double reward,
            bool valid,
            bool singular,
            const std::string& name
         );
    };

    /**
     * @brief Compute approximate hypervolume of a Pareto front.
     * @param front Pareto front points.
     * @param refPoint Reference point (worse than all objectives).
     * @return Approximate hypervolume.
     */
    double computeHypervolume(const std::vector<ParetoPoint>& front, const std::vector<double>& refPoint) const;

    /**
     * @brief BAO distance data point.
     */
    struct BAODataPoint {
        double z;
        double dA_mpc;
        double sigma_dA_mpc;
    };

    /**
     * @brief Live multi-messenger observation (e.g., GW170817-like alert).
     */
    struct LiveObservation {
        double redshift;
        double mu_obs;    ///< Distance modulus (mag)
        double mu_err;    ///< 1-sigma uncertainty on distance modulus (mag)
        std::string origin;
    };

    /**
     * @brief BBN abundance data point.
     */
    struct BBNDataPoint {
        double observed_value;
        double sigma;
        std::string isotope;
    };

    /**
     * @brief Enable or disable multi-objective Pareto optimization.
     * When enabled, discoverBestTheory maintains a Pareto archive of
     * non-dominated solutions across multiple objectives.
     */
    void setMultiObjectiveEnabled(bool enabled);

    /**
     * @brief Check if multi-objective Pareto optimization is enabled.
     */
    bool isMultiObjectiveEnabled() const { return multi_objective_mode_; }

    /**
     * @brief Get the current Pareto front (non-dominated solutions).
     * @return Vector of ParetoPoint representing the current front.
     */
    std::vector<ParetoPoint> getParetoFront() const;

    /**
     * @brief Clear the Pareto archive.
     */
    void resetParetoArchive();

    /**
     * @brief Compute objective vector from a DiscoveryResult.
     * Objectives (all minimized):
     *   0: observational chi-squared
     *   1: theoretical penalty
     *   2: simplicity penalty
     *   3: near-singularity penalty (0 or 1000)
     * @param result DiscoveryResult from evaluateTheory.
     * @return Vector of objective values.
     */
    std::vector<double> computeObjectives(const DiscoveryResult& result) const;

    /**
     * @brief Compute BAO distance chi-squared contribution.
     * Compares predicted angular diameter distances at z=0.38, 0.51, 0.70, 1.48
     * against representative SDSS/BOSS/eBOSS measurements.
     * @param params Theory parameter map.
     * @return Chi-squared value for BAO distances.
     */
    double computeBAOChi2(const std::map<std::string, double>& params) const;

    /**
     * @brief Compute CMB shift parameter R chi-squared contribution.
     * R = sqrt(Omega_m H0^2) * d_A(z_rec) / c
     * @param params Theory parameter map.
     * @return Chi-squared value for CMB shift parameter.
     */
    double computeCMBShiftChi2(const std::map<std::string, double>& params) const;

    /**
     * @brief Compute BBN abundance chi-squared contribution.
     * Validates deuterium D/H and helium-4 Y_p against observed values.
     * @param params Theory parameter map.
     * @return Chi-squared value for BBN abundances.
     */
    double computeBBNChi2(const std::map<std::string, double>& params) const;

    /**
     * @brief Compute the full observational chi-squared including all probes:
     * Pantheon+ SNe, GW170817, BAO, CMB shift, BBN.
     * @param metric Spacetime metric tensor.
     * @param params Theory parameter map.
     * @return Combined chi-squared value.
     */
    double computeObservationalChi2(
        const MetricTensor& metric,
        const std::map<std::string, double>& params
    ) const;

    /**
     * @brief Check Pareto dominance between two points.
     * Point a dominates b if a is no worse in all objectives and strictly better in at least one.
     * @param a First Pareto point.
     * @param b Second Pareto point.
     * @return True if a dominates b.
     */
    static bool dominates(const ParetoPoint& a, const ParetoPoint& b);

    /**
     * @brief Update Pareto archive with a new candidate point.
     * Removes any existing points dominated by the new point,
     * and discards the new point if it is dominated.
     * @param point New candidate Pareto point.
     * @param result Corresponding DiscoveryResult for BMA bookkeeping.
     */
    void updateParetoArchive(const ParetoPoint& point, const DiscoveryResult& result) const;

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

    // Multi-objective Pareto state
    bool multi_objective_mode_;
    mutable std::vector<ParetoPoint> pareto_archive_;
    mutable std::vector<DiscoveryResult> pareto_results_;
    mutable std::vector<double> model_weights_;

    // Active learning state
    bool active_learning_enabled_;
    ActiveLearningMode active_learning_mode_;
    AcquisitionMode m_acquisitionMode_;
    mutable std::vector<std::pair<std::vector<double>, double>> evaluation_history_;
    mutable std::unique_ptr<TheorySurrogate> surrogate_;
    mutable std::vector<TheorySurrogate> m_objectiveSurrogates_;

    // Live observational data
    std::vector<LiveObservation> m_liveObservations;
    mutable BayesianComparisonResult m_lastBayesResult_;
};

} // namespace discovery
} // namespace quantumverse

#endif // QUANTUMVERSE_THEORY_DISCOVERY_AGENT_H
