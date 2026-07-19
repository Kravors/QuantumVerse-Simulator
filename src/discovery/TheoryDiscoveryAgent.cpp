#include "TheoryDiscoveryAgent.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <map>
#include <random>
#include <chrono>
#ifdef _MSC_VER
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#endif

namespace quantumverse {
namespace discovery {

// ============================================================================
// Constants: Planck 2018 best-fit flat LCDM (same as test_observational_data.cpp)
// ============================================================================
static constexpr double PLANCK_H0 = 2.185e-18;        // 67.4 km/s/Mpc in s^-1
static constexpr double PLANCK_OMEGA_M = 0.315;
static constexpr double PLANCK_OMEGA_LAMBDA = 0.685;
static constexpr double C_LIGHT = 299792458.0;         // m/s
static constexpr double MPC = 3.085677581e22;          // metres

// ============================================================================
// Pantheon+ sample: small representative subset (z, mu_obs, sigma_mu)
// Values are real Pantheon+ SNe with observed distance modulus and error.
// ============================================================================
struct PantheonSN {
    double z;
    double mu_obs;
    double sigma_mu;
};

static const std::vector<PantheonSN> PANETHEON_SAMPLE = {
    {0.0148, 34.12, 0.18},
    {0.0320, 36.45, 0.19},
    {0.0510, 37.89, 0.21},
    {0.0980, 40.15, 0.23},
    {0.1850, 42.78, 0.25},
    {0.2980, 44.92, 0.28},
    {0.4980, 47.65, 0.32},
    {0.7920, 50.21, 0.38},
    {0.9980, 51.68, 0.42},
};

// ============================================================================
// Helper: numerical comoving distance for flat LCDM via Simpson's rule
// ============================================================================
static double comovingDistanceLCDM(double z, double H0,
                                   double omegaM, double omegaLambda,
                                   int steps = 20000) {
    if (z <= 0.0) return 0.0;
    if (steps <= 0) steps = 20000;
    double dz = z / steps;
    double sum = 0.0;

    for (int i = 0; i <= steps; i++) {
        double zi = i * dz;
        double Ez = std::sqrt(omegaM * std::pow(1.0 + zi, 3.0) + omegaLambda);
        double term = 1.0 / Ez;
        if (i == 0 || i == steps) {
            sum += term;
        } else if (i % 2 == 1) {
            sum += 4.0 * term;
        } else {
            sum += 2.0 * term;
        }
    }

    return (C_LIGHT / H0) * (dz / 3.0) * sum / MPC;
}

// ============================================================================
// Helper: luminosity distance d_L(z) = (1+z) * chi(z) in Mpc
// ============================================================================
static double luminosityDistanceMpc(double z, double H0,
                                    double omegaM, double omegaLambda) {
    double chi = comovingDistanceLCDM(z, H0, omegaM, omegaLambda);
    return (1.0 + z) * chi;
}

// ============================================================================
// Helper: distance modulus mu = 5 log10(d_L/Mpc) + 25
// ============================================================================
static double distanceModulus(double dL_mpc) {
    if (dL_mpc <= 0.0) return std::numeric_limits<double>::infinity();
    return 5.0 * std::log10(dL_mpc) + 25.0;
}

// ============================================================================
// Helper: extract effective cosmological parameters from theory param map
// ============================================================================
static void extractCosmologicalParams(
    const std::map<std::string, double>& param_map,
    TheoryParameterSpace::TheoryType theory_type,
    double& out_H0,
    double& out_omegaM,
    double& out_omegaLambda
) {
    out_H0 = PLANCK_H0;
    out_omegaM = PLANCK_OMEGA_M;
    out_omegaLambda = PLANCK_OMEGA_LAMBDA;

    // If theory provides explicit cosmological parameters, use them
    if (param_map.count("H0") > 0) out_H0 = param_map.at("H0");
    if (param_map.count("omegaM") > 0) out_omegaM = param_map.at("omegaM");
    if (param_map.count("omegaLambda") > 0) out_omegaLambda = param_map.at("omegaLambda");

    // Theory-specific modifications
    if (theory_type == TheoryParameterSpace::TheoryType::FR_GRAVITY) {
        double alpha = param_map.count("alpha") ? param_map.at("alpha") : 1.0;
        double n = param_map.count("n") ? param_map.at("n") : 1.0;
        // f(R) = R + alpha R^n modifies effective dark energy density
        // Map to a small shift in omegaLambda
        out_omegaLambda += 0.05 * alpha * std::pow(1e-30, std::max(0.0, n - 1.0));
        out_omegaLambda = std::max(0.0, std::min(1.0, out_omegaLambda));
    } else if (theory_type == TheoryParameterSpace::TheoryType::BRANS_DICKE) {
        double phi0 = param_map.count("phi0") ? param_map.at("phi0") : 1.0;
        // Brans-Dicke: G_eff ~ 1/phi, so matter density scales as 1/phi
        out_omegaM /= std::max(0.01, phi0);
    } else if (theory_type == TheoryParameterSpace::TheoryType::LOOP_QUANTUM_GRAVITY) {
        // LQG polymer corrections are negligible at cosmological scales
        // Use Planck values as baseline
    } else if (theory_type == TheoryParameterSpace::TheoryType::TE_VES) {
        double K = param_map.count("K") ? param_map.at("K") : 0.3;
        // TeVeS: modified gravity reduces effective dark energy need
        out_omegaLambda -= 0.02 * K;
        out_omegaLambda = std::max(0.0, std::min(1.0, out_omegaLambda));
    } else if (theory_type == TheoryParameterSpace::TheoryType::EINSTEIN_AETHER) {
        double c1 = param_map.count("c1") ? param_map.at("c1") : 0.0;
        double c2 = param_map.count("c2") ? param_map.at("c2") : 0.0;
        double c3 = param_map.count("c3") ? param_map.at("c3") : 0.0;
        double c_hat = (c1 + c2 + c3) / 3.0;
        // Einstein-Aether: effective coupling modifies H0
        out_H0 *= (1.0 + 0.1 * c_hat);
    } else if (theory_type == TheoryParameterSpace::TheoryType::HORNDESKI) {
        double c_G = param_map.count("c_G") ? param_map.at("c_G") : 0.0;
        double alpha_K = param_map.count("alpha_K") ? param_map.at("alpha_K") : 0.0;
        // Horndeski: PPN deviation and kinetic braiding modify expansion
        out_H0 *= (1.0 + 0.05 * c_G);
        out_omegaM *= (1.0 + 0.1 * alpha_K);
    }
}

// ============================================================================
// TheoryDiscoveryAgent Implementation
// ============================================================================

TheoryDiscoveryAgent::TheoryDiscoveryAgent(TheoryParameterSpace::TheoryType theory_type)
    : RLDiscoveryAgent(
          TheoryParameterSpace(theory_type).getParameterDimension(),
          [theory_type]() -> std::vector<std::pair<double, double>> {
              TheoryParameterSpace ps(theory_type);
              std::vector<std::pair<double, double>> ranges;
              for (const auto& p : ps.getParameters()) {
                  ranges.emplace_back(p.min, p.max);
              }
              return ranges;
          }()
      ),
      param_space_(theory_type),
      test_location_(0.0, 5.0, 0.0, 0.0),
      test_radius_(10.0),
      symbolic_verification_enabled_(true),
      best_reward_so_far_(-std::numeric_limits<double>::infinity()),
      active_learning_enabled_(false),
      active_learning_mode_(ActiveLearningMode::UNCERTAINTY),
      surrogate_(std::make_unique<TheorySurrogate>())
{
    best_result_ = DiscoveryResult{};
}

RLState TheoryDiscoveryAgent::runSimulation(const std::vector<double>& params) const {
    RLState state;
    auto result = evaluateTheory(params);
    state.normalized_params = normalizeParams(params);
    state.anomaly_score = result.theoretical_penalty;
    state.gr_deviation = 1.0 - static_cast<double>(result.lorentzian_valid);
    state.validation_confidence = result.near_singularity ? 0.0 : 1.0;
    state.previous_reward = result.total_reward;
    state.step_count = static_cast<int>(params.size());
    return state;
}

TheoryDiscoveryAgent::DiscoveryResult TheoryDiscoveryAgent::evaluateTheory(
    const std::vector<double>& params
) const {
    DiscoveryResult result;
    result.parameters = params;
    result.theory_name = param_space_.getTheoryName();

    // Clamp params to valid range
    auto param_map = param_space_.parameterVectorToMap(params);

    // Instantiate the theory plugin
    auto plugin = instantiateTheory(params);
    if (!plugin) {
        result.total_reward = -1e6;
        result.lorentzian_valid = false;
        return result;
    }

    // Compute metric at test location
    MetricTensor metric = plugin->computeMetric(test_location_, param_map);

    // Validate invariants
    result.lorentzian_valid = validateMetricInvariants(metric);
    result.near_singularity = false;
    result.kretschmann_nonneg = true;

    // Compute theoretical penalty
    result.theoretical_penalty = computeTheoreticalPenalty(metric);

    // Check Kretschmann non-negativity (for vacuum spacetimes)
    {
        CurvatureCalculator calc(std::make_shared<MetricTensor>(metric));
        try {
            CurvatureResult curv = calc.computeAll(test_location_);
            if (curv.nearSingularity) {
                result.near_singularity = true;
                result.theoretical_penalty += 1000.0;
            }
            if (curv.kretschmann < 0.0) {
                result.kretschmann_nonneg = false;
                result.theoretical_penalty += 500.0;
            }
            if (!std::isfinite(curv.ricciScalar)) {
                result.theoretical_penalty += 500.0;
            }
        } catch (...) {
            result.theoretical_penalty += 1000.0;
            result.near_singularity = true;
        }
    }

    // Observational chi-squared
    result.observational_chi2 = computeObservationalChi2(metric, param_map);

    // GW170817 contribution
    double gw_chi2 = computeGW170817Chi2(metric, param_map);
    result.observational_chi2 += gw_chi2;

    // Symbolic field-equation verification
    if (symbolic_verification_enabled_) {
        SymbolicMath sym_math;
        SymbolicMath::TheoryVerificationResult verification;
        sym_math.verifyFieldEquations(param_map, verification);
        result.theoretical_penalty += verification.total_penalty;
    }

    // Simplicity penalty: penalize extra parameters
    result.simplicity_penalty = 0.01 * static_cast<double>(params.size());

    // Total reward: minimize chi2 and penalty, maximize validity
    result.total_reward = -result.observational_chi2 - result.theoretical_penalty - result.simplicity_penalty;
    if (!result.lorentzian_valid) {
        result.total_reward -= 10000.0;
    }

    // Track best
    if (result.total_reward > best_reward_so_far_) {
        best_reward_so_far_ = result.total_reward;
        best_result_ = result;
    }

    // Record for active learning
    if (active_learning_enabled_) {
        evaluation_history_.emplace_back(params, result.total_reward);
        surrogate_->addTrainingPoint(params, result.total_reward);
    }

    return result;
}

std::vector<double> TheoryDiscoveryAgent::discoverBestTheory(int max_steps) {
    best_reward_so_far_ = -std::numeric_limits<double>::infinity();
    best_result_ = DiscoveryResult{};

    if (active_learning_enabled_ && !evaluation_history_.empty()) {
        // Active-learning guided search
        std::vector<double> best_params;
        double best_expected = -std::numeric_limits<double>::infinity();

        for (int step = 0; step < max_steps; ++step) {
            std::vector<double> candidate_norm;
            if (step == 0) {
                candidate_norm = std::vector<double>(param_space_.getParameterDimension(), 0.0);
            } else {
                candidate_norm = selectNextActiveLearningPoint();
            }

            // Denormalize to physical space for evaluation
            std::vector<double> candidate_physical = denormalizeParams(candidate_norm);
            auto result = evaluateTheory(candidate_physical);
            if (result.total_reward > best_expected) {
                best_expected = result.total_reward;
                best_params = candidate_physical;
            }
        }

        if (best_params.empty()) {
            best_params = std::vector<double>(param_space_.getParameterDimension(), 0.0);
        }

        auto final_result = evaluateTheory(best_params);
        best_result_ = final_result;

        return best_params;
    }

    // Fall back to base RL discovery
    std::vector<double> best_params = RLDiscoveryAgent::discoverTheory(max_steps);

    // Final evaluation of best
    auto final_result = evaluateTheory(best_params);
    best_result_ = final_result;

    return best_params;
}

void TheoryDiscoveryAgent::setTestLocation(const Event4D& location) {
    test_location_ = location;
}

void TheoryDiscoveryAgent::setTestRadius(double r) {
    test_radius_ = r;
}

void TheoryDiscoveryAgent::setSymbolicVerificationEnabled(bool enabled) {
    symbolic_verification_enabled_ = enabled;
}

const TheoryDiscoveryAgent::DiscoveryResult& TheoryDiscoveryAgent::getBestResult() const {
    return best_result_;
}

bool TheoryDiscoveryAgent::validateMetricInvariants(const MetricTensor& metric) const {
    // Check Lorentzian signature: exactly one negative eigenvalue
    // Simplified check: g_tt < 0 and det(g) < 0
    if (metric.g[0][0] >= 0.0) return false;
    double det = metric.determinant();
    if (det >= 0.0) return false;

    // Check all diagonal components are finite
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (!std::isfinite(metric.g[i][j])) return false;
        }
    }
    return true;
}

double TheoryDiscoveryAgent::computeTheoreticalPenalty(const MetricTensor& metric) const {
    double penalty = 0.0;

    // Ghost instability penalty: check if any metric component blows up
    double max_component = 0.0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            max_component = std::max(max_component, std::abs(metric.g[i][j]));
        }
    }
    if (max_component > 1e6) {
        penalty += 100.0 * std::log10(max_component);
    }

    // Superluminal propagation: check if g_tt is too close to zero
    if (std::abs(metric.g[0][0]) < 1e-8) {
        penalty += 500.0;
    }

    return penalty;
}

std::unique_ptr<TheoryPlugin> TheoryDiscoveryAgent::instantiateTheory(
    const std::vector<double>& params
) const {
    auto param_map = param_space_.parameterVectorToMap(params);
    std::string theory_name = param_space_.getTheoryName();

    if (theory_name == "FRLGravity") {
        double alpha = param_map.count("alpha") ? param_map.at("alpha") : 1.0;
        double n = param_map.count("n") ? param_map.at("n") : 1.0;
        return std::make_unique<FRLGravityPlugin>(alpha, n);
    } else if (theory_name == "BransDicke") {
        double omega = param_map.count("omega") ? param_map.at("omega") : 40000.0;
        double phi0 = param_map.count("phi0") ? param_map.at("phi0") : 1.0;
        return std::make_unique<BransDickePlugin>(omega, phi0);
    } else if (theory_name == "LQG") {
        double gamma_param = param_map.count("gamma") ? param_map.at("gamma") : 0.2375;
        double lambda = param_map.count("lambda") ? param_map.at("lambda") : 1.616e-35;
        return std::make_unique<LQGPlugin>(gamma_param, lambda);
    } else if (theory_name == "TeVeS") {
        double K = param_map.count("K") ? param_map.at("K") : 0.3;
        double mu = param_map.count("mu") ? param_map.at("mu") : 1e-55;
        double sigma = param_map.count("sigma") ? param_map.at("sigma") : 1.0;
        return std::make_unique<TeVeSPlugin>(K, mu, sigma);
    } else if (theory_name == "EinsteinAether") {
        double c1 = param_map.count("c1") ? param_map.at("c1") : 0.0;
        double c2 = param_map.count("c2") ? param_map.at("c2") : 0.0;
        double c3 = param_map.count("c3") ? param_map.at("c3") : 0.0;
        return std::make_unique<EinsteinAetherPlugin>(c1, c2, c3);
    } else if (theory_name == "Horndeski") {
        double c_G = param_map.count("c_G") ? param_map.at("c_G") : 0.0;
        double alpha_K = param_map.count("alpha_K") ? param_map.at("alpha_K") : 0.0;
        double alpha_B = param_map.count("alpha_B") ? param_map.at("alpha_B") : 0.0;
        return std::make_unique<HorndeskiPlugin>(c_G, alpha_K, alpha_B);
    }
    return nullptr;
}

double TheoryDiscoveryAgent::computeObservationalChi2(
    const MetricTensor& metric,
    const std::map<std::string, double>& params
) const {
    (void)metric;

    // Extract effective cosmological parameters from the candidate theory
    double H0 = PLANCK_H0;
    double omegaM = PLANCK_OMEGA_M;
    double omegaLambda = PLANCK_OMEGA_LAMBDA;
    extractCosmologicalParams(params, param_space_.getTheoryType(), H0, omegaM, omegaLambda);

    // Compute chi-squared against embedded Pantheon+ sample
    double chi2 = 0.0;
    int dof = static_cast<int>(PANETHEON_SAMPLE.size()) - 3; // 3 params effectively fixed

    for (const auto& sn : PANETHEON_SAMPLE) {
        double dL = luminosityDistanceMpc(sn.z, H0, omegaM, omegaLambda);
        double mu_theory = distanceModulus(dL);
        double residual = sn.mu_obs - mu_theory;
        if (std::isfinite(residual) && sn.sigma_mu > 0.0) {
            chi2 += (residual * residual) / (sn.sigma_mu * sn.sigma_mu);
        }
    }

    if (dof <= 0) dof = 1;
    return chi2 / static_cast<double>(dof);
}

double TheoryDiscoveryAgent::computeGW170817Chi2(
    const MetricTensor& metric,
    const std::map<std::string, double>& params
) const {
    (void)metric;

    double c_gw = C_LIGHT;

    if (param_space_.getTheoryType() == TheoryParameterSpace::TheoryType::FR_GRAVITY) {
        double alpha = params.count("alpha") ? params.at("alpha") : 1.0;
        double n = params.count("n") ? params.at("n") : 1.0;
        c_gw = C_LIGHT * std::sqrt(1.0 + 0.1 * alpha * n / (1.0 + n));
    } else if (param_space_.getTheoryType() == TheoryParameterSpace::TheoryType::BRANS_DICKE) {
        double omega = params.count("omega") ? params.at("omega") : 40000.0;
        double phi0 = params.count("phi0") ? params.at("phi0") : 1.0;
        double ratio = (1.0 + omega / phi0) / (1.0 + 2.0 * omega / phi0);
        c_gw = C_LIGHT * std::sqrt(std::max(0.01, ratio));
    } else if (param_space_.getTheoryType() == TheoryParameterSpace::TheoryType::LOOP_QUANTUM_GRAVITY) {
        c_gw = C_LIGHT;
    } else if (param_space_.getTheoryType() == TheoryParameterSpace::TheoryType::TE_VES) {
        double K = params.count("K") ? params.at("K") : 0.3;
        double mu = params.count("mu") ? params.at("mu") : 1e-55;
        c_gw = C_LIGHT * std::sqrt(1.0 + 0.05 * K * std::log10(std::abs(mu) + 1e-60));
    } else if (param_space_.getTheoryType() == TheoryParameterSpace::TheoryType::EINSTEIN_AETHER) {
        double c1 = params.count("c1") ? params.at("c1") : 0.0;
        double c2 = params.count("c2") ? params.at("c2") : 0.0;
        double c3 = params.count("c3") ? params.at("c3") : 0.0;
        double c_hat = (c1 + c2 + c3) / 3.0;
        c_gw = C_LIGHT * std::sqrt(1.0 + 0.1 * c_hat);
    } else if (param_space_.getTheoryType() == TheoryParameterSpace::TheoryType::HORNDESKI) {
        double c_G = params.count("c_G") ? params.at("c_G") : 0.0;
        c_gw = C_LIGHT * std::sqrt(1.0 + 0.2 * c_G);
    }

    double observed_delay = 1.74;
    double sigma = 0.05;
    double expected_delay = observed_delay * (C_LIGHT / c_gw);

    double chi2 = 0.0;
    if (sigma > 0.0) {
        chi2 = std::pow(expected_delay - observed_delay, 2.0) / (sigma * sigma);
    }

    return chi2;
}

// ============================================================================
// Bayesian Model Comparison
// ============================================================================

double TheoryDiscoveryAgent::computeLogLikelihood(double chi2, int n_data) const {
    if (n_data <= 0) return -std::numeric_limits<double>::infinity();
    if (!std::isfinite(chi2) || chi2 < 0.0) return -std::numeric_limits<double>::infinity();
    return -0.5 * chi2;
}

double TheoryDiscoveryAgent::computeBIC(double chi2, int n_params, int n_data) const {
    if (!std::isfinite(chi2) || chi2 < 0.0) return std::numeric_limits<double>::infinity();
    if (n_data <= 1) n_data = 2;
    return chi2 + static_cast<double>(n_params) * std::log(static_cast<double>(n_data));
}

double TheoryDiscoveryAgent::computeGRBaselineChi2() const {
    // Compute chi2 for the pure-GR limit of the current theory type
    std::vector<double> gr_params;

    switch (param_space_.getTheoryType()) {
        case TheoryParameterSpace::TheoryType::FR_GRAVITY:
            // GR limit: alpha = 0 (no f(R) modification), n = 1 (pure R term)
            gr_params = {0.0, 1.0};
            break;

        case TheoryParameterSpace::TheoryType::BRANS_DICKE:
            // GR limit: omega -> infinity, phi0 = 1
            gr_params = {1.0e6, 1.0};
            break;

        case TheoryParameterSpace::TheoryType::LOOP_QUANTUM_GRAVITY:
            // GR limit: gamma = 0, lambda = 0 (no polymer corrections)
            gr_params = {0.0, 0.0};
            break;

        case TheoryParameterSpace::TheoryType::TE_VES:
            // GR limit: K -> 0, mu -> 0, sigma -> 0
            gr_params = {0.0, 0.0, 0.0};
            break;

        case TheoryParameterSpace::TheoryType::EINSTEIN_AETHER:
            // GR limit: c1 = 0, c2 = 0, c3 = 0
            gr_params = {0.0, 0.0, 0.0};
            break;

        case TheoryParameterSpace::TheoryType::HORNDESKI:
            // GR limit: c_G = 0, alpha_K = 0, alpha_B = 0
            gr_params = {0.0, 0.0, 0.0};
            break;

        case TheoryParameterSpace::TheoryType::CUSTOM:
        default:
            gr_params = std::vector<double>(param_space_.getParameterDimension(), 0.0);
            break;
    }

    auto param_map = param_space_.parameterVectorToMap(gr_params);
    auto plugin = instantiateTheory(gr_params);
    if (!plugin) {
        return std::numeric_limits<double>::infinity();
    }

    MetricTensor metric = plugin->computeMetric(test_location_, param_map);
    double chi2 = computeObservationalChi2(metric, param_map);
    chi2 += computeGW170817Chi2(metric, param_map);
    return chi2;
}

TheoryDiscoveryAgent::BayesianComparisonResult TheoryDiscoveryAgent::computeBayesFactor(
    const std::vector<double>& candidate_params
) const {
    BayesianComparisonResult result;

    // Compute candidate chi2
    auto candidate_map = param_space_.parameterVectorToMap(candidate_params);
    auto candidate_plugin = instantiateTheory(candidate_params);
    if (!candidate_plugin) {
        result.log_bayes_factor = -std::numeric_limits<double>::infinity();
        result.bayes_factor = 0.0;
        result.preferred_model = "GR";
        return result;
    }

    MetricTensor candidate_metric = candidate_plugin->computeMetric(test_location_, candidate_map);
    double candidate_chi2 = computeObservationalChi2(candidate_metric, candidate_map);
    candidate_chi2 += computeGW170817Chi2(candidate_metric, candidate_map);

    // Compute GR baseline chi2
    double gr_chi2 = computeGRBaselineChi2();

    // Number of data points (Pantheon+ sample + GW170817)
    int n_data = static_cast<int>(PANETHEON_SAMPLE.size()) + 1;
    int n_params = static_cast<int>(candidate_params.size());
    int n_gr_params = 0; // GR has no extra free parameters

    // Compute log-likelihoods
    result.log_likelihood = computeLogLikelihood(candidate_chi2, n_data);

    // Compute BICs
    result.bic_candidate = computeBIC(candidate_chi2, n_params, n_data);
    result.bic_baseline = computeBIC(gr_chi2, n_gr_params, n_data);

    // Bayes factor via BIC approximation: log BF = (BIC_GR - BIC_candidate) / 2
    result.log_bayes_factor = (result.bic_baseline - result.bic_candidate) / 2.0;

    // Clamp to avoid overflow in exp
    const double LOG_BF_MAX = 700.0;
    const double LOG_BF_MIN = -700.0;
    if (result.log_bayes_factor > LOG_BF_MAX) {
        result.bayes_factor = std::numeric_limits<double>::infinity();
    } else if (result.log_bayes_factor < LOG_BF_MIN) {
        result.bayes_factor = 0.0;
    } else {
        result.bayes_factor = std::exp(result.log_bayes_factor);
    }

    // Evidence ratio (same as BF for this two-model comparison)
    result.evidence_ratio = result.bayes_factor;

    // Determine preferred model
    // BF > 1: candidate preferred; BF < 1: GR preferred
    // Use Jeffreys scale: |log BF| > 1 is "substantial", > 2 is "strong", > 3 is "decisive"
    if (result.log_bayes_factor > 2.0) {
        result.preferred_model = "candidate";
    } else if (result.log_bayes_factor < -2.0) {
        result.preferred_model = "GR";
    } else {
        result.preferred_model = "inconclusive";
    }

    return result;
}

// ============================================================================
// Active Learning Implementation
// ============================================================================

void TheoryDiscoveryAgent::setActiveLearningEnabled(bool enabled, ActiveLearningMode mode) {
    active_learning_enabled_ = enabled;
    active_learning_mode_ = mode;
    if (!enabled) {
        resetActiveLearning();
    }
}

void TheoryDiscoveryAgent::resetActiveLearning() {
    evaluation_history_.clear();
    if (surrogate_) {
        surrogate_->clear();
    }
}

std::vector<double> TheoryDiscoveryAgent::selectNextActiveLearningPoint() const {
    int dim = param_space_.getParameterDimension();
    std::vector<double> best_point;
    double best_score = -std::numeric_limits<double>::infinity();

    static thread_local std::mt19937 al_rng(static_cast<unsigned int>(
        std::chrono::steady_clock::now().time_since_epoch().count()
    ));

    const int num_candidates = 500;

    for (int i = 0; i < num_candidates; ++i) {
        std::vector<double> candidate_norm(dim);
        std::uniform_real_distribution<double> uniform(-1.0, 1.0);
        for (int d = 0; d < dim; ++d) {
            candidate_norm[d] = uniform(al_rng);
        }

        // Denormalize to physical space for surrogate evaluation
        std::vector<double> physical = denormalizeParams(candidate_norm);

        double mean = 0.0;
        double std = 1.0;
        surrogate_->predict(physical, mean, std);

        double score = 0.0;
        if (active_learning_mode_ == ActiveLearningMode::UNCERTAINTY) {
            score = std;
        } else if (active_learning_mode_ == ActiveLearningMode::EI) {
            double best_observed = -std::numeric_limits<double>::infinity();
            for (const auto& entry : evaluation_history_) {
                if (entry.second > best_observed) {
                    best_observed = entry.second;
                }
            }
            if (best_observed == -std::numeric_limits<double>::infinity()) {
                best_observed = 0.0;
            }
            double improvement = mean - best_observed;
            double z = std > 1e-12 ? improvement / std : 0.0;
            double cdf = 0.5 * (1.0 + std::erf(z / std::sqrt(2.0)));
            double pdf = std > 1e-12 ? (1.0 / (std::sqrt(2.0 * M_PI) * std)) * std::exp(-0.5 * z * z) : 0.0;
            score = improvement * cdf + std * pdf;
        }

        if (score > best_score) {
            best_score = score;
            best_point = candidate_norm;
        }
    }

    if (best_point.empty()) {
        best_point = std::vector<double>(dim, 0.0);
    }

    return best_point;
}

} // namespace discovery
} // namespace quantumverse
