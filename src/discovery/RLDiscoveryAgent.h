#ifndef QUANTUMVERSE_RL_DISCOVERY_AGENT_H
#define QUANTUMVERSE_RL_DISCOVERY_AGENT_H

#include <vector>
#include <random>
#include <memory>
#include <string>
#include <cmath>
#include <map>

namespace quantumverse {
namespace discovery {

struct RLState {
    std::vector<double> normalized_params;
    double anomaly_score = 0.0;
    double gr_deviation = 0.0;
    double validation_confidence = 0.0;
    double previous_reward = 0.0;
    int step_count = 0;
    
    std::vector<double> toVector() const;
    static RLState fromVector(const std::vector<double>& v, int param_dim);
    RLState() = default;
};

struct RLAction {
    enum ActionType { ADJUST_PARAMETER, EXPLORE, EXPLOIT, TERMINATE };
    ActionType type = ADJUST_PARAMETER;
    int param_index = 0;
    double param_delta = 0.0;
    
    void apply(std::vector<double>& params) const;
    RLAction() = default;
    RLAction(ActionType t, int idx, double delta) : type(t), param_index(idx), param_delta(delta) {}
};

struct RewardComponents {
    double anomaly_bonus = 0.0;
    double gr_consistency_bonus = 0.0;
    double novelty_bonus = 0.0;
    double simplicity_penalty = 0.0;
    double validation_bonus = 0.0;
    double total = 0.0;
    RewardComponents() = default;
};

class PolicyNetwork {
private:
    std::vector<int> layer_sizes;
    std::vector<std::vector<double>> weights;
    std::vector<std::vector<double>> biases;
    std::mt19937 rng;
    
    double tanh(double x) const { return std::tanh(x); }
    std::vector<double> forward(const std::vector<double>& state) const;
    
public:
    PolicyNetwork(int state_dim, int action_dim);
    PolicyNetwork(const PolicyNetwork& other) = default;
    
    RLAction sampleAction(const std::vector<double>& state, double epsilon);
    void mutate(double rate, double scale);
    std::vector<double> getParameters() const;
    void setParameters(const std::vector<double>& params);
};

class RLDiscoveryAgent {
private:
    std::unique_ptr<PolicyNetwork> policy;
    std::vector<std::pair<double, double>> param_ranges;
    int param_dim = 0;
    double learning_rate = 0.01;
    double gamma = 0.99;
    double epsilon = 1.0;
    double total_reward = 0.0;
    int episode_count = 0;
    mutable std::mt19937 rng;
    
protected:
    std::vector<double> normalizeParams(const std::vector<double>& params) const;
    std::vector<double> denormalizeParams(const std::vector<double>& normalized) const;
    double computeReward(const RLState& state) const;
    RLState runSimulation(const std::vector<double>& params) const;
    
public:
    RLDiscoveryAgent(int param_dim, const std::vector<std::pair<double, double>>& ranges);
    
    double trainEpisode();
    std::vector<double> discoverTheory(int max_steps = 100);
    void setEpsilon(double eps);
    double getEpsilon() const;
    int getEpisodeCount() const;
    double getAverageReward(int window_size = 100) const;
    void savePolicy(const std::string& path) const;
    void loadPolicy(const std::string& path);
};

class RLTrainer {
private:
    RLDiscoveryAgent agent;
    int total_episodes = 0;
    int checkpoint_interval = 10;
    std::string checkpoint_dir;
    std::vector<double> reward_history;
    std::vector<double> best_reward_history;
    
public:
    RLTrainer(int param_dim, const std::vector<std::pair<double, double>>& ranges,
              int episodes, int checkpoint_interval_ = 10, std::string checkpoint_dir_ = "./checkpoints");
    
    void train();
    std::vector<double> evaluate();
    const std::vector<double>& getRewardHistory() const { return reward_history; }
    const std::vector<double>& getBestRewardHistory() const { return best_reward_history; }
    void saveFinal(const std::string& path);
    void loadAndResume(const std::string& path, int additional_episodes);
};

} // namespace discovery
} // namespace quantumverse

#endif
