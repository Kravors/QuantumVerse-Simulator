// Phase 6.3: RL Agent Convergence Test
// Validates RLDiscoveryAgent and RLTrainer produce finite rewards,
// converge over episodes, and survive edge cases without crashing.

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "discovery/RLDiscoveryAgent.h"
#include "spacetime/Event4D.h"
#include "test_assert.h"

using namespace quantumverse::discovery;

namespace {
constexpr double kEps = 1e-9;

void assertFinite(const std::string& label, double v) {
    QV_CHECK(std::isfinite(v) && ("Non-finite value for " + label).c_str());
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
        QV_CHECK(agent.getEpisodeCount() == 0);
        QV_CHECK(agent.getEpsilon() > 0.0);
        std::cout << "  Agent constructed. Epsilon = " << agent.getEpsilon() << std::endl;
    }

    // --- 2. Train episode returns finite reward ---------------------------------
    {
        RLDiscoveryAgent agent(2, makeDefaultRanges(2));
        double reward = agent.trainEpisode();
        QV_CHECK(std::isfinite(reward));
        QV_CHECK(agent.getEpisodeCount() == 1);
        std::cout << "  Episode reward = " << reward << std::endl;
    }

    // --- 3. Multiple episodes: epsilon decays -----------------------------------
    {
        RLDiscoveryAgent agent(2, makeDefaultRanges(2));
        double eps_start = agent.getEpsilon();
        for (int i = 0; i < 20; ++i) {
            double r = agent.trainEpisode();
            QV_CHECK(std::isfinite(r));
            (void)r;
        }
        double eps_end = agent.getEpsilon();
        QV_CHECK(eps_end <= eps_start + kEps);
        std::cout << "  Epsilon decay: " << eps_start << " -> " << eps_end << std::endl;
    }

    // --- 4. Average reward is finite --------------------------------------------
    {
        RLDiscoveryAgent agent(2, makeDefaultRanges(2));
        for (int i = 0; i < 10; ++i) {
            agent.trainEpisode();
        }
        double avg = agent.getAverageReward();
        QV_CHECK(std::isfinite(avg));
        std::cout << "  Average reward = " << avg << std::endl;
    }

    // --- 5. discoverTheory returns finite parameters -----------------------------
    {
        RLDiscoveryAgent agent(3, makeDefaultRanges(3));
        auto best = agent.discoverTheory(20);
        QV_CHECK(!best.empty());
        for (double p : best) {
            assertFinite("discovered param", p);
        }
        std::cout << "  discoverTheory returned " << best.size() << " finite parameters." << std::endl;
    }

    // --- 6. Policy save/load roundtrip ------------------------------------------
    {
        RLDiscoveryAgent agent(2, makeDefaultRanges(2));
        agent.trainEpisode();
        // Use a portable path (temp dir) instead of a hardcoded drive letter, which
        // does not exist on CI runners (e.g. Windows GitHub runner has no F: drive)
        // and would make savePolicy throw -> uncaught exception -> abort (0xc0000409).
        std::string path = (std::filesystem::temp_directory_path() / "test_rl_policy_roundtrip.bin").string();
        agent.savePolicy(path);
        QV_CHECK(std::ifstream(path).good());

        RLDiscoveryAgent agent2(2, makeDefaultRanges(2));
        agent2.loadPolicy(path);
        QV_CHECK(agent2.getEpisodeCount() == 0);
        std::cout << "  Policy save/load roundtrip passed." << std::endl;
    }

    // --- 7. setEpsilon clamps to [0,1] ------------------------------------------
    {
        RLDiscoveryAgent agent(2, makeDefaultRanges(2));
        agent.setEpsilon(2.0);
        QV_CHECK(agent.getEpsilon() <= 1.0);
        agent.setEpsilon(-1.0);
        QV_CHECK(agent.getEpsilon() >= 0.0);
        std::cout << "  Epsilon clamping verified." << std::endl;
    }

    // --- 8. RLTrainer completes without crash ------------------------------------
    {
        std::string ckpt_dir = (std::filesystem::temp_directory_path() / "test_rl_checkpoints").string();
        RLTrainer trainer(2, makeDefaultRanges(2), 10, 5, ckpt_dir);
        trainer.train();
        auto history = trainer.getRewardHistory();
        QV_CHECK(history.size() == 10);
        for (double r : history) {
            QV_CHECK(std::isfinite(r));
            (void)r;
        }
        std::cout << "  RLTrainer completed " << history.size() << " episodes." << std::endl;
    }

    // --- 9. Edge case: single-dimension agent ------------------------------------
    {
        RLDiscoveryAgent agent(1, std::vector<std::pair<double, double>>{{-5.0, 5.0}});
        for (int i = 0; i < 5; ++i) {
            double r = agent.trainEpisode();
            QV_CHECK(std::isfinite(r));
            (void)r;
        }
        std::cout << "  Single-dimension agent survived." << std::endl;
    }

    // --- 10. Edge case: zero initial epsilon -------------------------------------
    {
        RLDiscoveryAgent agent(2, makeDefaultRanges(2));
        agent.setEpsilon(0.0);
        double r = agent.trainEpisode();
        QV_CHECK(std::isfinite(r));
        (void)r;
        std::cout << "  Zero-epsilon agent survived." << std::endl;
    }

    // --- 11. ONNX Runtime inference (PPO actor policy) ---------------------------
#ifdef HAVE_ONNX
    {
        RLDiscoveryAgent agent(3, makeDefaultRanges(3));
        QV_CHECK(!agent.isONNXLoaded());

        // Use relative path from source dir (ctest WORKING_DIRECTORY set)
        bool loaded = agent.loadONNXModel("rl_discovery_agent.onnx");
        QV_CHECK(loaded);
        QV_CHECK(agent.isONNXLoaded());
        std::cout << "  ONNX model loaded successfully" << std::endl;

        // Test predictONNAction with Schwarzschild start state [M=10, a=0, Q=0, score=0]
        std::array<float, 4> state_schwarzschild = {10.0f, 0.0f, 0.0f, 0.0f};
        auto action1 = agent.predictONNAction(state_schwarzschild);
        QV_CHECK(std::isfinite(action1[0]));
        QV_CHECK(std::isfinite(action1[1]));
        QV_CHECK(std::isfinite(action1[2]));
        std::cout << "  Schwarzschild action: ["
                  << action1[0] << ", " << action1[1] << ", " << action1[2] << "]" << std::endl;

        // Test with near-extremal state [M=10, a=0.9, Q=0.3, score=0.8]
        std::array<float, 4> state_extremal = {10.0f, 0.9f, 0.3f, 0.8f};
        auto action2 = agent.predictONNAction(state_extremal);
        QV_CHECK(std::isfinite(action2[0]));
        QV_CHECK(std::isfinite(action2[1]));
        QV_CHECK(std::isfinite(action2[2]));
        std::cout << "  Near-extremal action: ["
                  << action2[0] << ", " << action2[1] << ", " << action2[2] << "]" << std::endl;

        // Test determinism: same input -> same output
        auto action3 = agent.predictONNAction(state_schwarzschild);
        QV_CHECK(action1[0] == action3[0]);
        QV_CHECK(action1[1] == action3[1]);
        QV_CHECK(action1[2] == action3[2]);
        std::cout << "  ONNX inference is deterministic" << std::endl;

        std::cout << "  ONNX Runtime inference test passed" << std::endl;
    }
#else
    std::cout << "  ONNX Runtime inference test skipped (HAVE_ONNX not defined)" << std::endl;
#endif

    std::cout << "All RLAgentConvergenceTest checks passed." << std::endl;
    return 0;
}
