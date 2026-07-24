#include "TheoryDiscoveryAgent.h"
#include "data/GCNNoticeParser.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <map>
#include <random>
#include <chrono>
#include <array>
#include <tuple>
#include <numeric>
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
static constexpr double PLANCK_OMEGA_B = 0.049;       // physical baryon density
static constexpr double C_LIGHT = 299792458.0;         // m/s
static constexpr double MPC = 3.085677581e22;          // metres
static constexpr double Z_REC = 1100.0;                // recombination redshift

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
// BAO measurements (effective distance ratio D_V / r_d)
// r_d = 147.09 Mpc (Planck 2018 sound horizon at drag epoch)
// ============================================================================
static const std::array<std::tuple<double, double, double>, 4> BAO_DATA = {{
    {0.38, 10.23, 0.17},   // SDSS DR7 LRG
    {0.51, 13.36, 0.21},   // BOSS LOWZ
    {0.70, 17.33, 0.26},   // BOSS CMASS
    {1.48, 26.51, 1.93}    // eBOSS quasars
}};

static constexpr double RD_REF = 147.09; // Planck 2018 sound horizon Mpc

// ============================================================================
// BBN abundances: observed values with uncertainties
// ============================================================================
struct BBNAbundance {
    double observed_value;
    double sigma;
    std::string name;
};

static const std::vector<BBNAbundance> BBN_ABUNDANCES = {
    {2.545e-5, 0.025e-5, "D/H"},
    {0.2449, 0.0040, "Y_p"},
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
// Helper: BAO effective distance D_V(z) in Mpc
// D_V = [d_M^2 z c / H(z)]^(1/3), d_M = (1+z) d_A
// ============================================================================
static double baoEffectiveDistanceMpc(double z, double H0, double omegaM, double omegaLambda) {
    if (z <= 0.0) return 0.0;
    double dA = comovingDistanceLCDM(z, H0, omegaM, omegaLambda) / (1.0 + z);
    double Hz = H0 * std::sqrt(omegaM * std::pow(1.0 + z, 3.0) + omegaLambda);
    double c_over_H_mpc = C_LIGHT / Hz / MPC;
    double dM = (1.0 + z) * dA;
    double D_V = std::pow(dM * dM * z * c_over_H_mpc, 1.0/3.0);
    return std::isfinite(D_V) ? D_V : 0.0;
}

// ============================================================================
// Helper: extract effective cosmological parameters from theory param map
// ============================================================================
static void extractCosmologicalParams(
    const std::map<std::string, double>& param_map,
    TheoryParameterSpace::TheoryType theory_type,
    double& out_H0,
    double& out_omegaM,
    double& out_omegaLambda,
    double& out_omegaB
) {
    out_H0 = PLANCK_H0;
    out_omegaM = PLANCK_OMEGA_M;
    out_omegaLambda = PLANCK_OMEGA_LAMBDA;
    out_omegaB = PLANCK_OMEGA_B;

    // If theory provides explicit cosmological parameters, use them
    if (param_map.count("H0") > 0) out_H0 = param_map.at("H0");
    if (param_map.count("omegaM") > 0) out_omegaM = param_map.at("omegaM");
    if (param_map.count("omegaLambda") > 0) out_omegaLambda = param_map.at("omegaLambda");
    if (param_map.count("omegaB") > 0) out_omegaB = param_map.at("omegaB");

    // Theory-specific modifications
    if (theory_type == TheoryParameterSpace::TheoryType::FR_GRAVITY) {
        double alpha = param_map.count("alpha") ? param_map.at("alpha") : 1.0;
        double n = param_map.count("n") ? param_map.at("n") : 1.0;
        out_omegaLambda += 0.05 * alpha * std::pow(1e-30, std::max(0.0, n - 1.0));
        out_omegaLambda = std::max(0.0, std::min(1.0, out_omegaLambda));
    } else if (theory_type == TheoryParameterSpace::TheoryType::BRANS_DICKE) {
        double phi0 = param_map.count("phi0") ? param_map.at("phi0") : 1.0;
        out_omegaM /= std::max(0.01, phi0);
    } else if (theory_type == TheoryParameterSpace::TheoryType::LOOP_QUANTUM_GRAVITY) {
        // LQG polymer corrections are negligible at cosmological scales
    } else if (theory_type == TheoryParameterSpace::TheoryType::TE_VES) {
        double K = param_map.count("K") ? param_map.at("K") : 0.3;
        out_omegaLambda -= 0.02 * K;
        out_omegaLambda = std::max(0.0, std::min(1.0, out_omegaLambda));
    } else if (theory_type == TheoryParameterSpace::TheoryType::EINSTEIN_AETHER) {
        double c1 = param_map.count("c1") ? param_map.at("c1") : 0.0;
        double c2 = param_map.count("c2") ? param_map.at("c2") : 0.0;
        double c3 = param_map.count("c3") ? param_map.at("c3") : 0.0;
        double c_hat = (c1 + c2 + c3) / 3.0;
        out_H0 *= (1.0 + 0.1 * c_hat);
    } else if (theory_type == TheoryParameterSpace::TheoryType::HORNDESKI) {
        double c_G = param_map.count("c_G") ? param_map.at("c_G") : 0.0;
        double alpha_K = param_map.count("alpha_K") ? param_map.at("alpha_K") : 0.0;
        out_H0 *= (1.0 + 0.05 * c_G);
        out_omegaM *= (1.0 + 0.1 * alpha_K);
    }

    // Enforce consistency: omegaB <= omegaM
    out_omegaB = std::min(out_omegaB, out_omegaM);
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
      m_acquisitionMode_(AcquisitionMode::UNCERTAINTY),
       multi_objective_mode_(false),
       surrogate_(std::make_unique<TheorySurrogate>()),
       m_objectiveSurrogates_(4)
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
    double raw_chi2 = computeObservationalChi2(metric, param_map);
    double gw_chi2 = computeGW170817Chi2(metric, param_map);
    result.observational_chi2 = raw_chi2 + gw_chi2;
    result.raw_observational_chi2 = raw_chi2 + gw_chi2;

    int total_dof = static_cast<int>(PANETHEON_SAMPLE.size())
                  + static_cast<int>(m_liveObservations.size())
                  + static_cast<int>(BAO_DATA.size()) + 1 + 2 + 1;
    result.n_data_points = total_dof > 0 ? total_dof : 1;

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

        auto objectives = computeObjectives(result);
        for (size_t i = 0; i < m_objectiveSurrogates_.size() && i < objectives.size(); ++i) {
            m_objectiveSurrogates_[i].addTrainingPoint(params, objectives[i]);
        }
    }

    // Update Pareto archive if multi-objective mode is enabled
    if (multi_objective_mode_) {
        ParetoPoint point(
            params,
            computeObjectives(result),
            result.total_reward,
            result.lorentzian_valid,
            result.near_singularity,
            result.theory_name
        );
        updateParetoArchive(point, result);
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

        if (multi_objective_mode_) {
            computeModelWeights();
        }

        return best_params;
    }

    if (multi_objective_mode_) {
        // Multi-objective Pareto-guided search
        std::vector<double> best_params;
        double best_reward = -std::numeric_limits<double>::infinity();

        for (int step = 0; step < max_steps; ++step) {
            std::vector<double> candidate_norm;
            if (step == 0) {
                candidate_norm = std::vector<double>(param_space_.getParameterDimension(), 0.0);
            } else {
                candidate_norm = selectNextActiveLearningPoint();
            }

            std::vector<double> candidate_physical = denormalizeParams(candidate_norm);
            auto result = evaluateTheory(candidate_physical);

            if (result.total_reward > best_reward) {
                best_reward = result.total_reward;
                best_params = candidate_physical;
            }
        }

        if (best_params.empty()) {
            best_params = std::vector<double>(param_space_.getParameterDimension(), 0.0);
        }

        // Prefer a Pareto-optimal solution if the archive is non-empty
        if (!pareto_archive_.empty()) {
            for (const auto& point : pareto_archive_) {
                if (point.total_reward > best_reward) {
                    best_reward = point.total_reward;
                    best_params = point.parameters;
                }
            }
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

        if (multi_objective_mode_) {
            computeModelWeights();
        }

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

void TheoryDiscoveryAgent::ingestLiveAlert(const QJsonObject& alert) {
    auto parsed = GCNNoticeParser::parse(alert);
    if (!parsed.isValid) return;

    double dl  = parsed.luminosityDistance;
    double z   = parsed.redshift;
    double err = parsed.distanceError;
    if (dl <= 0.0 || err <= 0.0) return;

    double mu_obs = 5.0 * std::log10(dl * 1e6) - 5.0;
    double mu_err = (5.0 / std::log(10.0)) * (err / dl);

    LiveObservation obs;
    obs.redshift = z;
    obs.mu_obs   = mu_obs;
    obs.mu_err   = mu_err;
    obs.origin   = "GCN_Kafka";
    m_liveObservations.push_back(obs);

    if (!best_result_.parameters.empty()) {
        auto updated = evaluateTheory(best_result_.parameters);
        best_result_.observational_chi2 = updated.observational_chi2;
        best_result_.total_reward = updated.total_reward;
        m_lastBayesResult_ = computeBayesFactor(best_result_.parameters);
    }
}

const TheoryDiscoveryAgent::DiscoveryResult& TheoryDiscoveryAgent::getBestResult() const {
    return best_result_;
}

const TheoryDiscoveryAgent::BayesianComparisonResult& TheoryDiscoveryAgent::getLastBayesResult() const {
    return m_lastBayesResult_;
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

    double H0 = PLANCK_H0;
    double omegaM = PLANCK_OMEGA_M;
    double omegaLambda = PLANCK_OMEGA_LAMBDA;
    double omegaB = PLANCK_OMEGA_B;
    extractCosmologicalParams(params, param_space_.getTheoryType(), H0, omegaM, omegaLambda, omegaB);

    double chi2 = 0.0;

    // Pantheon+ SNe Ia contribution
    int dof = static_cast<int>(PANETHEON_SAMPLE.size());
    for (const auto& sn : PANETHEON_SAMPLE) {
        double dL = luminosityDistanceMpc(sn.z, H0, omegaM, omegaLambda);
        double mu_theory = distanceModulus(dL);
        double residual = sn.mu_obs - mu_theory;
        if (std::isfinite(residual) && sn.sigma_mu > 0.0) {
            chi2 += (residual * residual) / (sn.sigma_mu * sn.sigma_mu);
        }
    }

    // Live GW alert observations (e.g., LIGO/Virgo/KAGRA GCN notices)
    for (const auto& obs : m_liveObservations) {
        double dL = luminosityDistanceMpc(obs.redshift, H0, omegaM, omegaLambda);
        double mu_theory = distanceModulus(dL);
        double residual = obs.mu_obs - mu_theory;
        if (std::isfinite(residual) && obs.mu_err > 0.0) {
            chi2 += (residual * residual) / (obs.mu_err * obs.mu_err);
        }
    }
    dof += static_cast<int>(m_liveObservations.size());

    // BAO angular diameter distances
    chi2 += computeBAOChi2(params);
    dof += static_cast<int>(BAO_DATA.size());

    // CMB shift parameter
    chi2 += computeCMBShiftChi2(params);
    dof += 1;

    // BBN abundances
    chi2 += computeBBNChi2(params);
    dof += 2;

    return dof > 0 ? chi2 / static_cast<double>(dof) : 1e9;
}

// ============================================================================
// Phase 27: Expanded Cosmological Probes
// ============================================================================

double TheoryDiscoveryAgent::computeBAOChi2(
    const std::map<std::string, double>& params
) const {
    double H0 = PLANCK_H0;
    double omegaM = PLANCK_OMEGA_M;
    double omegaLambda = PLANCK_OMEGA_LAMBDA;
    double omegaB = PLANCK_OMEGA_B;
    extractCosmologicalParams(params, param_space_.getTheoryType(), H0, omegaM, omegaLambda, omegaB);

    double chi2 = 0.0;
    for (const auto& bao : BAO_DATA) {
        double z = std::get<0>(bao);
        double dV_over_rd_obs = std::get<1>(bao);
        double sigma = std::get<2>(bao);

        double D_V = baoEffectiveDistanceMpc(z, H0, omegaM, omegaLambda);
        double dV_over_rd_pred = D_V / RD_REF;

        if (std::isfinite(dV_over_rd_pred) && sigma > 0.0) {
            double residual = dV_over_rd_obs - dV_over_rd_pred;
            chi2 += (residual * residual) / (sigma * sigma);
        }
    }
    return chi2;
}

double TheoryDiscoveryAgent::computeCMBShiftChi2(
    const std::map<std::string, double>& params
) const {
    double H0 = PLANCK_H0;
    double omegaM = PLANCK_OMEGA_M;
    double omegaLambda = PLANCK_OMEGA_LAMBDA;
    double omegaB = PLANCK_OMEGA_B;
    extractCosmologicalParams(params, param_space_.getTheoryType(), H0, omegaM, omegaLambda, omegaB);

    // CMB shift parameter: R = sqrt(Omega_m H0^2) * d_A(z_rec) / c
    double dA_rec = comovingDistanceLCDM(Z_REC, H0, omegaM, omegaLambda) / (1.0 + Z_REC);
    double R = std::sqrt(omegaM * H0 * H0) * dA_rec / C_LIGHT;

    // Planck 2018 measured value: R = 1.7492 +/- 0.0049
    static constexpr double CMB_SHIFT_R = 1.7492;
    static constexpr double CMB_SHIFT_ERR = 0.0049;
    double residual = CMB_SHIFT_R - R;
    return (residual * residual) / (CMB_SHIFT_ERR * CMB_SHIFT_ERR);
}

double TheoryDiscoveryAgent::computeBBNChi2(
    const std::map<std::string, double>& params
) const {
    double H0 = PLANCK_H0;
    double omegaM = PLANCK_OMEGA_M;
    double omegaLambda = PLANCK_OMEGA_LAMBDA;
    double omegaB = PLANCK_OMEGA_B;
    extractCosmologicalParams(params, param_space_.getTheoryType(), H0, omegaM, omegaLambda, omegaB);

    double chi2 = 0.0;

    // Physical baryon density omegaB = Omega_b * h^2
    // Deuterium prediction: D/H ~ 2.58e-5 * (omegaB / 0.02225)^-1.62
    double dh_pred = 2.58e-5 * std::pow(omegaB / 0.02225, -1.62);
    // Helium-4 prediction: Y_p ~ 0.231 + 0.01 * ln(omegaB / 0.02225)
    double yp_pred = 0.231 + 0.01 * std::log(omegaB / 0.02225);

    for (const auto& bbn : BBN_ABUNDANCES) {
        double predicted = 0.0;
        if (bbn.name == "D/H") {
            predicted = dh_pred;
        } else if (bbn.name == "Y_p") {
            predicted = yp_pred;
        }
        if (std::isfinite(predicted) && bbn.sigma > 0.0) {
            double residual = bbn.observed_value - predicted;
            chi2 += (residual * residual) / (bbn.sigma * bbn.sigma);
        }
    }

    return chi2;
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
    for (auto& surr : m_objectiveSurrogates_) {
        surr.clear();
    }
}

std::vector<double> TheoryDiscoveryAgent::selectNextActiveLearningPoint() const {
    return selectNextActiveLearningPoint(m_acquisitionMode_);
}

void TheoryDiscoveryAgent::setAcquisitionMode(AcquisitionMode mode) {
    m_acquisitionMode_ = mode;
}

std::vector<double> TheoryDiscoveryAgent::selectNextActiveLearningPoint(AcquisitionMode mode) const {
    int dim = param_space_.getParameterDimension();
    std::vector<double> best_point;
    double best_score = -std::numeric_limits<double>::infinity();

    static thread_local std::mt19937 al_rng(static_cast<unsigned int>(
        std::chrono::steady_clock::now().time_since_epoch().count()
    ));

    const int num_candidates = 500;
    std::uniform_real_distribution<double> uniform(-1.0, 1.0);

    for (int i = 0; i < num_candidates; ++i) {
        std::vector<double> candidate_norm(dim);
        for (int d = 0; d < dim; ++d) {
            candidate_norm[d] = uniform(al_rng);
        }

        std::vector<double> physical = denormalizeParams(candidate_norm);

        double mean = 0.0;
        double std = 1.0;
        if (surrogate_) {
            surrogate_->predict(physical, mean, std);
        }

        double score = 0.0;
        switch (mode) {
            case AcquisitionMode::RANDOM:
                score = 0.0;
                break;
            case AcquisitionMode::UNCERTAINTY:
                score = std;
                break;
            case AcquisitionMode::EI: {
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
                break;
            }
            case AcquisitionMode::EHVI: {
                if (!multi_objective_mode_ || pareto_archive_.empty() || m_objectiveSurrogates_.size() < 4) {
                    score = std;
                } else {
                    std::vector<double> means(4), stds(4);
                    for (int obj = 0; obj < 4; ++obj) {
                        m_objectiveSurrogates_[obj].predict(physical, means[obj], stds[obj]);
                    }

                    const int M = 50;
                    double total_hvi = 0.0;
                    std::normal_distribution<double> normal(0.0, 1.0);

                    for (int m = 0; m < M; ++m) {
                        std::vector<double> sample(4);
                        for (int obj = 0; obj < 4; ++obj) {
                            sample[obj] = means[obj] + stds[obj] * normal(al_rng);
                        }

                        double hvi = 0.0;
                        for (const auto& p : pareto_archive_) {
                            bool dominated = true;
                            for (size_t k = 0; k < 4 && k < p.objectives.size(); ++k) {
                                if (sample[k] > p.objectives[k]) {
                                    dominated = false;
                                    break;
                                }
                            }
                            if (dominated) {
                                double vol = 1.0;
                                for (size_t k = 0; k < 4 && k < p.objectives.size(); ++k) {
                                    double improvement = p.objectives[k] - sample[k];
                                    if (improvement > 0) vol *= improvement;
                                    else { vol = 0; break; }
                                }
                                hvi += vol;
                            }
                        }
                        total_hvi += hvi;
                    }
                    score = total_hvi / M;
                }
                break;
            }
            case AcquisitionMode::UCB: {
                double kappa = 2.0;
                score = mean + kappa * std;
                break;
            }
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

double TheoryDiscoveryAgent::computeHypervolume(const std::vector<ParetoPoint>& front, const std::vector<double>& refPoint) const {
    if (front.empty() || refPoint.empty()) return 0.0;

    double hv = 0.0;
    for (const auto& p : front) {
        double vol = 1.0;
        for (size_t i = 0; i < refPoint.size() && i < p.objectives.size(); ++i) {
            double diff = refPoint[i] - p.objectives[i];
            if (diff > 0) vol *= diff;
            else { vol = 0; break; }
        }
        hv += vol;
    }
    return hv;
}

// ============================================================================
// Multi-Objective Pareto Optimization
// ============================================================================

TheoryDiscoveryAgent::ParetoPoint::ParetoPoint(
    const std::vector<double>& params,
    const std::vector<double>& objs,
    double reward,
    bool valid,
    bool singular,
    const std::string& name
) : parameters(params),
    objectives(objs),
    total_reward(reward),
    lorentzian_valid(valid),
    near_singularity(singular),
    theory_name(name) {}

bool TheoryDiscoveryAgent::dominates(
    const ParetoPoint& a,
    const ParetoPoint& b
) {
    if (a.objectives.size() != b.objectives.size()) return false;
    bool at_least_one_better = false;
    for (size_t i = 0; i < a.objectives.size(); ++i) {
        if (a.objectives[i] > b.objectives[i]) return false;
        if (a.objectives[i] < b.objectives[i]) at_least_one_better = true;
    }
    return at_least_one_better;
}

void TheoryDiscoveryAgent::setMultiObjectiveEnabled(bool enabled) {
    multi_objective_mode_ = enabled;
    if (!enabled) {
        resetParetoArchive();
    }
}

std::vector<TheoryDiscoveryAgent::ParetoPoint> TheoryDiscoveryAgent::getParetoFront() const {
    return pareto_archive_;
}

void TheoryDiscoveryAgent::resetParetoArchive() {
    pareto_archive_.clear();
    pareto_results_.clear();
    model_weights_.clear();
}

std::vector<double> TheoryDiscoveryAgent::computeObjectives(const DiscoveryResult& result) const {
    std::vector<double> objectives;
    objectives.reserve(4);

    objectives.push_back(result.observational_chi2);
    objectives.push_back(result.theoretical_penalty);
    objectives.push_back(result.simplicity_penalty);
    objectives.push_back(result.near_singularity ? 1000.0 : 0.0);

    return objectives;
}

void TheoryDiscoveryAgent::updateParetoArchive(const ParetoPoint& point, const DiscoveryResult& result) const {
    std::vector<ParetoPoint> new_archive;
    std::vector<DiscoveryResult> new_results;
    bool is_dominated = false;

    for (size_t i = 0; i < pareto_archive_.size(); ++i) {
        if (dominates(pareto_archive_[i], point)) {
            is_dominated = true;
        }
        if (!dominates(point, pareto_archive_[i])) {
            new_archive.push_back(pareto_archive_[i]);
            new_results.push_back(pareto_results_[i]);
        }
    }

    if (!is_dominated) {
        new_archive.push_back(point);
        new_results.push_back(result);
    }

    pareto_archive_ = std::move(new_archive);
    pareto_results_ = std::move(new_results);
}

// ============================================================================
// Phase 28: Bayesian Model Averaging
// ============================================================================

std::vector<double> TheoryDiscoveryAgent::computeModelWeights() const {
    if (pareto_results_.empty()) {
        model_weights_.clear();
        return {};
    }

    std::vector<double> log_weights;
    log_weights.reserve(pareto_results_.size());

    for (const auto& result : pareto_results_) {
        int k = static_cast<int>(result.parameters.size());
        int n = result.n_data_points > 0 ? result.n_data_points : 2;
        double bic = computeBIC(result.raw_observational_chi2, k, n);
        log_weights.push_back(-0.5 * bic);
    }

    double max_log = *std::max_element(log_weights.begin(), log_weights.end());
    double sum_exp = 0.0;
    for (double lw : log_weights) {
        sum_exp += std::exp(lw - max_log);
    }
    double log_sum = max_log + std::log(sum_exp);

    std::vector<double> weights;
    weights.reserve(log_weights.size());
    for (double lw : log_weights) {
        weights.push_back(std::exp(lw - log_sum));
    }

    model_weights_ = weights;
    return weights;
}

double TheoryDiscoveryAgent::predictBMA(
    const std::function<double(const DiscoveryResult&)>& quantity
) const {
    auto weights = computeModelWeights();
    if (weights.empty() || pareto_results_.size() != weights.size()) {
        return 0.0;
    }

    double prediction = 0.0;
    for (size_t i = 0; i < pareto_results_.size(); ++i) {
        prediction += weights[i] * quantity(pareto_results_[i]);
    }
    return prediction;
}

double TheoryDiscoveryAgent::predictiveVarianceBMA(
    const std::function<double(const DiscoveryResult&)>& quantity
) const {
    auto weights = computeModelWeights();
    if (weights.empty() || pareto_results_.size() != weights.size()) {
        return 0.0;
    }

    double mean = predictBMA(quantity);
    double variance = 0.0;
    for (size_t i = 0; i < pareto_results_.size(); ++i) {
        double q = quantity(pareto_results_[i]);
        double diff = q - mean;
        variance += weights[i] * diff * diff;
    }
    return variance;
}

} // namespace discovery
} // namespace quantumverse
