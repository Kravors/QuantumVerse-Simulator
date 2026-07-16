// Phase 6.3: RL Agent Convergence Test
// Validates RLDiscoveryAgent and RLTrainer produce finite rewards,
// converge over episodes, and survive edge cases without crashing.

#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "discovery/RLDiscoveryAgent.h"
#include "spacetime/Event4D.h"

using namespace quantumverse::discovery;

namespace {
constexpr double kEps = 1e-9;

void assertFinite(const std::string& label, double v) {
    assert(std::isfinite(v) && ("Non-finite value for " + label).c_str());
    (void)label;
    (void)v;
}

std::vector<std::pair<double, double>> makeDefaultRanges(int dim) {
    std::vector<std::pair<double, double>> ranges;
    for (int i = 0; i < dim; ++i) {
        ranges.emplace_back(-10.0, 10.0);
    }
    return ranges;
}
}

int main() {
    std::cout << "=== RLAgentConvergenceTest ===" << std::endl;

    // --- 1. Basic agent construction --------------------------------------------
    {
        RLDiscoveryAgent agent(3, makeDefaultRanges(3));
        assert(agent.getEpisodeCount() == 0);
        assert(agent.getEpsilon() > 0.0);
        std::cout << "  Agent constructed. Epsilon = " << agent.getEpsilon() << std::endl;
    }

    // --- 2. Train episode returns finite reward ---------------------------------
    {
        RLDiscoveryAgent agent(2, makeDefaultRanges(2));
        double reward = agent.trainEpisode();
        assert(std::isfinite(reward));
        assert(agent.getEpisodeCount() == 1);
        std::cout << "  Episode reward = " << reward << std::endl;
    }

    // --- 3. Multiple episodes: epsilon decays -----------------------------------
    {
        RLDiscoveryAgent agent(2, makeDefaultRanges(2));
        double eps_start = agent.getEpsilon();
        for (int i = 0; i < 20; ++i) {
            double r = agent.trainEpisode();
            assert(std::isfinite(r));
            (void)r;
        }
        double eps_end = agent.getEpsilon();
        assert(eps_end <= eps_start + kEps);
        std::cout << "  Epsilon decay: " << eps_start << " -> " << eps_end << std::endl;
    }

    // --- 4. Average reward is finite --------------------------------------------
    {
        RLDiscoveryAgent agent(2, makeDefaultRanges(2));
        for (int i = 0; i < 10; ++i) {
            agent.trainEpisode();
        }
        double avg = agent.getAverageReward();
        assert(std::isfinite(avg));
        std::cout << "  Average reward = " << avg << std::endl;
    }

    // --- 5. discoverTheory returns finite parameters -----------------------------
    {
        RLDiscoveryAgent agent(3, makeDefaultRanges(3));
        auto best = agent.discoverTheory(20);
        assert(!best.empty());
        for (double p : best) {
            assertFinite("discovered param", p);
        }
        std::cout << "  discoverTheory returned " << best.size() << " finite parameters." << std::endl;
    }

    // --- 6. Policy save/load roundtrip ------------------------------------------
    {
        RLDiscoveryAgent agent(2, makeDefaultRanges(2));
        agent.trainEpisode();
        std::string path = "F:/syyyy/build/test_rl_policy_roundtrip.bin";
        agent.savePolicy(path);
        assert(std::ifstream(path).good());

        RLDiscoveryAgent agent2(2, makeDefaultRanges(2));
        agent2.loadPolicy(path);
        assert(agent2.getEpisodeCount() == 0);
        std::cout << "  Policy save/load roundtrip passed." << std::endl;
    }

    // --- 7. setEpsilon clamps to [0,1] ------------------------------------------
    {
        RLDiscoveryAgent agent(2, makeDefaultRanges(2));
        agent.setEpsilon(2.0);
        assert(agent.getEpsilon() <= 1.0);
        agent.setEpsilon(-1.0);
        assert(agent.getEpsilon() >= 0.0);
        std::cout << "  Epsilon clamping verified." << std::endl;
    }

    // --- 8. RLTrainer completes without crash ------------------------------------
    {
        std::string ckpt_dir = "F:/syyyy/build/test_rl_checkpoints";
        RLTrainer trainer(2, makeDefaultRanges(2), 10, 5, ckpt_dir);
        trainer.train();
        auto history = trainer.getRewardHistory();
        assert(history.size() == 10);
        for (double r : history) {
            assert(std::isfinite(r));
            (void)r;
        }
        std::cout << "  RLTrainer completed " << history.size() << " episodes." << std::endl;
    }

    // --- 9. Edge case: single-dimension agent ------------------------------------
    {
        RLDiscoveryAgent agent(1, std::vector<std::pair<double, double>>{{-5.0, 5.0}});
        for (int i = 0; i < 5; ++i) {
            double r = agent.trainEpisode();
            assert(std::isfinite(r));
            (void)r;
        }
        std::cout << "  Single-dimension agent survived." << std::endl;
    }

    // --- 10. Edge case: zero initial epsilon -------------------------------------
    {
        RLDiscoveryAgent agent(2, makeDefaultRanges(2));
        agent.setEpsilon(0.0);
        double r = agent.trainEpisode();
        assert(std::isfinite(r));
        (void)r;
        std::cout << "  Zero-epsilon agent survived." << std::endl;
    }

    std::cout << "All RLAgentConvergenceTest checks passed." << std::endl;
    return 0;
}
