#include "RLDiscoveryAgent.h"
#include <algorithm>
#include <numeric>
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <filesystem>

namespace quantumverse {
namespace discovery {

// RLState implementation
std::vector<double> RLState::toVector() const {
    std::vector<double> v;
    v.reserve(normalized_params.size() + 5);
    v.insert(v.end(), normalized_params.begin(), normalized_params.end());
    v.push_back(anomaly_score);
    v.push_back(gr_deviation);
    v.push_back(validation_confidence);
    v.push_back(previous_reward);
    v.push_back(static_cast<double>(step_count));
    return v;
}

RLState RLState::fromVector(const std::vector<double>& v, int param_dim) {
    RLState s;
    if (v.size() >= static_cast<size_t>(param_dim + 5)) {
        s.normalized_params = std::vector<double>(v.begin(), v.begin() + param_dim);
        s.anomaly_score = v[param_dim];
        s.gr_deviation = v[param_dim + 1];
        s.validation_confidence = v[param_dim + 2];
        s.previous_reward = v[param_dim + 3];
        s.step_count = static_cast<int>(v[param_dim + 4]);
    }
    return s;
}

// RLAction implementation
void RLAction::apply(std::vector<double>& params) const {
    if (type == ADJUST_PARAMETER && param_index < static_cast<int>(params.size())) {
        params[param_index] += param_delta;
    }
}

// PolicyNetwork implementation
PolicyNetwork::PolicyNetwork(int state_dim, int action_dim) {
    layer_sizes = {state_dim, 128, 128, action_dim};
    std::normal_distribution<double> normal(0.0, 0.1);
    std::mt19937 gen(42);
    
    for (size_t i = 0; i < layer_sizes.size() - 1; ++i) {
        int rows = layer_sizes[i + 1];
        int cols = layer_sizes[i];
        std::vector<double> layer_weights(static_cast<size_t>(rows) * cols);
        for (auto& w : layer_weights) w = normal(gen);
        weights.push_back(layer_weights);
        
        std::vector<double> layer_biases(rows);
        for (auto& b : layer_biases) b = normal(gen) * 0.1;
        biases.push_back(layer_biases);
    }
    rng.seed(std::random_device{}());
}

std::vector<double> PolicyNetwork::forward(const std::vector<double>& state) const {
    std::vector<double> activations = state;
    
    for (size_t i = 0; i < weights.size(); ++i) {
        const auto& W = weights[i];
        const auto& b = biases[i];
        int rows = static_cast<int>(b.size());
        int cols = static_cast<int>(activations.size());
        
        std::vector<double> new_activations(rows);
        for (int r = 0; r < rows; ++r) {
            double sum = b[r];
            for (int c = 0; c < cols; ++c) {
                sum += W[r * cols + c] * activations[c];
            }
            if (i < weights.size() - 1) {
                new_activations[r] = tanh(sum);
            } else {
                new_activations[r] = sum;
            }
        }
        activations = new_activations;
    }
    return activations;
}

RLAction PolicyNetwork::sampleAction(const std::vector<double>& state, double epsilon) {
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    if (uniform(rng) < epsilon) {
        std::uniform_int_distribution<int> action_type(0, 3);
        std::uniform_int_distribution<int> param_idx(0, 9);
        std::uniform_real_distribution<double> delta(-0.1, 0.1);
        return RLAction(static_cast<RLAction::ActionType>(action_type(rng)), param_idx(rng), delta(rng));
    }
    
    auto logits = forward(state);
    int action_dim = static_cast<int>(logits.size() / 2);
    
    std::vector<double> probs(action_dim);
    double sum = 0.0;
    for (int i = 0; i < action_dim; ++i) {
        probs[i] = 1.0 / (1.0 + std::exp(-logits[i]));
        sum += probs[i];
    }
    for (auto& p : probs) p /= sum;
    
    std::discrete_distribution<int> dist(probs.begin(), probs.end());
    int param_idx = dist(rng);
    double delta = std::tanh(logits[action_dim + param_idx]) * 0.1;
    
    return RLAction(RLAction::ADJUST_PARAMETER, param_idx, delta);
}

void PolicyNetwork::mutate(double rate, double scale) {
    std::normal_distribution<double> normal(0.0, scale);
    for (auto& layer : weights) {
        for (auto& w : layer) {
            if (std::uniform_real_distribution<double>(0.0, 1.0)(rng) < rate) {
                w += normal(rng);
            }
        }
    }
    for (auto& layer : biases) {
        for (auto& b : layer) {
            if (std::uniform_real_distribution<double>(0.0, 1.0)(rng) < rate) {
                b += normal(rng);
            }
        }
    }
}

std::vector<double> PolicyNetwork::getParameters() const {
    std::vector<double> params;
    for (const auto& layer : weights) params.insert(params.end(), layer.begin(), layer.end());
    for (const auto& layer : biases) params.insert(params.end(), layer.begin(), layer.end());
    return params;
}

void PolicyNetwork::setParameters(const std::vector<double>& params) {
    size_t idx = 0;
    for (auto& layer : weights) {
        std::copy(params.begin() + idx, params.begin() + idx + layer.size(), layer.begin());
        idx += layer.size();
    }
    for (auto& layer : biases) {
        std::copy(params.begin() + idx, params.begin() + idx + layer.size(), layer.begin());
        idx += layer.size();
    }
}

// RLDiscoveryAgent implementation
RLDiscoveryAgent::RLDiscoveryAgent(int param_dim_, const std::vector<std::pair<double, double>>& ranges)
    : param_ranges(ranges), param_dim(param_dim_) {
    policy = std::make_unique<PolicyNetwork>(param_dim + 5, param_dim);
    std::random_device rd;
    rng.seed(rd());
}

std::vector<double> RLDiscoveryAgent::normalizeParams(const std::vector<double>& params) const {
    std::vector<double> normalized;
    normalized.reserve(param_dim);
    for (int i = 0; i < param_dim; ++i) {
        double min = param_ranges[i].first;
        double max = param_ranges[i].second;
        normalized.push_back(2.0 * (params[i] - min) / (max - min) - 1.0);
    }
    return normalized;
}

std::vector<double> RLDiscoveryAgent::denormalizeParams(const std::vector<double>& normalized) const {
    std::vector<double> params;
    params.reserve(param_dim);
    for (int i = 0; i < param_dim; ++i) {
        double min = param_ranges[i].first;
        double max = param_ranges[i].second;
        params.push_back(min + (normalized[i] + 1.0) * (max - min) / 2.0);
    }
    return params;
}

double RLDiscoveryAgent::computeReward(const RLState& state) const {
    RewardComponents comp;
    comp.anomaly_bonus = state.anomaly_score * 10.0;
    comp.gr_consistency_bonus = (1.0 - state.gr_deviation) * 5.0;
    comp.novelty_bonus = state.validation_confidence * 3.0;
    comp.simplicity_penalty = -0.01 * state.step_count;
    comp.validation_bonus = state.validation_confidence * 2.0;
    comp.total = comp.anomaly_bonus + comp.gr_consistency_bonus + comp.novelty_bonus +
                 comp.simplicity_penalty + comp.validation_bonus;
    return comp.total;
}

RLState RLDiscoveryAgent::runSimulation(const std::vector<double>& params) const {
    RLState state;
    state.normalized_params = normalizeParams(params);
    state.anomaly_score = std::abs(params[0]) * 0.1;
    state.gr_deviation = std::abs(params[1]) * 0.05;
    // Use step_count as a proxy for validation confidence to avoid out-of-bounds
    state.validation_confidence = 0.5 + 0.5 * std::sin(static_cast<double>(episode_count) * 0.1);
    state.step_count = episode_count;
    return state;
}

double RLDiscoveryAgent::trainEpisode() {
    std::vector<double> params(param_dim);
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    for (int i = 0; i < param_dim; ++i) {
        params[i] = param_ranges[i].first + uniform(rng) * (param_ranges[i].second - param_ranges[i].first);
    }
    
    double episode_reward = 0.0;
    int steps = 0;
    
    while (steps < 100) {
        RLState state = runSimulation(params);
        auto state_vec = state.toVector();
        RLAction action = policy->sampleAction(state_vec, epsilon);
        action.apply(params);
        
        double reward = computeReward(state);
        episode_reward += std::pow(gamma, steps) * reward;
        
        steps++;
        
        if (action.type == RLAction::TERMINATE && steps > 10) break;
    }
    
    total_reward += episode_reward;
    episode_count++;
    return episode_reward;
}

std::vector<double> RLDiscoveryAgent::discoverTheory(int max_steps) {
    std::vector<double> best_params(param_dim, 0.0);
    double best_reward = -1e10;
    
    for (int step = 0; step < max_steps; ++step) {
        double reward = trainEpisode();
        if (reward > best_reward) {
            best_reward = reward;
            RLState state = runSimulation(best_params);
            RLAction action = policy->sampleAction(state.toVector(), 0.0);
            if (action.type == RLAction::ADJUST_PARAMETER && action.param_index < param_dim) {
                best_params[action.param_index] += action.param_delta;
            }
        }
        if (epsilon > 0.05) epsilon *= 0.995;
    }
    
    return best_params;
}

void RLDiscoveryAgent::setEpsilon(double eps) { epsilon = std::max(0.0, std::min(1.0, eps)); }
double RLDiscoveryAgent::getEpsilon() const { return epsilon; }
int RLDiscoveryAgent::getEpisodeCount() const { return episode_count; }

double RLDiscoveryAgent::getAverageReward(int window_size) const {
    (void)window_size;
    return total_reward / static_cast<double>(episode_count + 1);
}

void RLDiscoveryAgent::savePolicy(const std::string& path) const {
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
    std::ofstream file(path);
    if (!file.is_open()) throw std::runtime_error("Cannot open file: " + path);
    auto params = policy->getParameters();
    for (size_t i = 0; i < params.size(); ++i) file << params[i] << (i + 1 == params.size() ? "" : " ");
    file.close();
}

void RLDiscoveryAgent::loadPolicy(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) throw std::runtime_error("Cannot open file: " + path);
    std::vector<double> params;
    double val;
    while (file >> val) params.push_back(val);
    file.close();
    policy->setParameters(params);
}

// RLTrainer implementation
RLTrainer::RLTrainer(int param_dim, const std::vector<std::pair<double, double>>& ranges,
                     int episodes, int checkpoint_interval_, std::string checkpoint_dir_)
    : agent(param_dim, ranges), total_episodes(episodes), checkpoint_interval(checkpoint_interval_),
      checkpoint_dir(std::move(checkpoint_dir_)) {}

void RLTrainer::train() {
    double best_reward = -1e10;
    for (int ep = 0; ep < total_episodes; ++ep) {
        double reward = agent.trainEpisode();
        reward_history.push_back(reward);
        if (reward > best_reward) {
            best_reward = reward;
            best_reward_history.push_back(reward);
        }
        
        if ((ep + 1) % checkpoint_interval == 0) {
            std::string path = checkpoint_dir + "/checkpoint_ep" + std::to_string(ep + 1) + ".bin";
            agent.savePolicy(path);
            std::cout << "[RLTrainer] Episode " << ep + 1 << "/" << total_episodes
                      << " | Reward: " << reward << " | Avg: " << agent.getAverageReward() << std::endl;
        }
    }
}

std::vector<double> RLTrainer::evaluate() {
    agent.setEpsilon(0.0);
    std::vector<double> theory = agent.discoverTheory(50);
    agent.setEpsilon(0.1);
    return theory;
}

void RLTrainer::saveFinal(const std::string& path) {
    agent.savePolicy(path);
}

void RLTrainer::loadAndResume(const std::string& path, int additional_episodes) {
    agent.loadPolicy(path);
    for (int i = 0; i < additional_episodes; ++i) {
        double reward = agent.trainEpisode();
        reward_history.push_back(reward);
    }
}

} // namespace discovery
} // namespace quantumverse
