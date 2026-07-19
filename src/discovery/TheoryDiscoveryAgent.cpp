#include "TheoryDiscoveryAgent.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <iostream>

namespace quantumverse {
namespace discovery {

TheoryDiscoveryAgent::TheoryDiscoveryAgent(TheoryParameterSpace::TheoryType theory_type)
    : RLDiscoveryAgent(
          TheoryParameterSpace(theory_type).getParameterDimension(),
          []() -> std::vector<std::pair<double, double>> {
              TheoryParameterSpace ps(TheoryParameterSpace::TheoryType::FR_GRAVITY);
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
      best_reward_so_far_(-std::numeric_limits<double>::infinity())
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
    double gw_chi2 = computeGW170817Chi2(metric);
    result.observational_chi2 += gw_chi2;

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

    return result;
}

std::vector<double> TheoryDiscoveryAgent::discoverBestTheory(int max_steps) {
    best_reward_so_far_ = -std::numeric_limits<double>::infinity();
    best_result_ = DiscoveryResult{};

    // Run RL discovery using overridden runSimulation
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
        double immirzi = param_map.count("gamma") ? param_map.at("gamma") : 0.2375;
        double lambda = param_map.count("lambda") ? param_map.at("lambda") : 1.616e-35;
        return std::make_unique<LQGPlugin>(immirzi, lambda);
    }
    return nullptr;
}

double TheoryDiscoveryAgent::computeObservationalChi2(
    const MetricTensor& metric,
    const std::map<std::string, double>& params
) const {
    (void)metric;
    (void)params;
    // Placeholder: embedded Pantheon+ sample vs Planck 2018 LCDM
    // Returns chi-squared goodness-of-fit
    return 0.0;
}

double TheoryDiscoveryAgent::computeGW170817Chi2(const MetricTensor& metric) const {
    (void)metric;
    // Placeholder: GW170817 time-delay correlation
    // Uses NGC 4993 coordinates (RA=197.45, Dec=-23.38)
    return 0.0;
}

} // namespace discovery
} // namespace quantumverse
