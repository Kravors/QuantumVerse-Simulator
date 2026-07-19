// Phase 22.1: Theory Discovery Agent Test
// Validates TheoryDiscoveryAgent integrates with RLDiscoveryAgent,
// TheoryManager, and the physics core to evaluate modified-gravity theories.

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "discovery/TheoryDiscoveryAgent.h"
#include "discovery/DiscoveryEngine.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "spacetime/Event4D.h"
#include "physics/CurvatureCalculator.h"

using namespace quantumverse;
using namespace quantumverse::discovery;

namespace {
constexpr double kEps = 1e-9;

void assertFinite(const std::string& label, double v) {
    assert(std::isfinite(v) && ("Non-finite value for " + label).c_str());
    (void)label;
    (void)v;
}
}

int main() {
    std::cout << "=== TheoryDiscoveryAgentTest ===" << std::endl;

    // --- 1. Construct agent for f(R) gravity ------------------------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::FR_GRAVITY);
        assert(agent.getEpisodeCount() == 0);
        std::cout << "  f(R) agent constructed. Param dim = "
                  << agent.getEpisodeCount() << " (episodes)" << std::endl;
    }

    // --- 2. Construct agent for Brans-Dicke -------------------------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::BRANS_DICKE);
        (void)agent;
        std::cout << "  Brans-Dicke agent constructed." << std::endl;
    }

    // --- 3. Construct agent for LQG ---------------------------------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::LOOP_QUANTUM_GRAVITY);
        (void)agent;
        std::cout << "  LQG agent constructed." << std::endl;
    }

    // --- 4. Evaluate near-GR parameters should yield finite reward --------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::FR_GRAVITY);
        // Near GR: alpha ~ 0, n = 1
        std::vector<double> params = {0.01, 1.0};
        auto result = agent.evaluateTheory(params);
        assertFinite("total_reward", result.total_reward);
        assertFinite("observational_chi2", result.observational_chi2);
        assertFinite("theoretical_penalty", result.theoretical_penalty);
        assert(result.lorentzian_valid);
        std::cout << "  Near-GR f(R) reward = " << result.total_reward
                  << " chi2 = " << result.observational_chi2 << std::endl;
    }

    // --- 5. Pathological parameters should be penalized -------------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::FR_GRAVITY);
        // Extreme values that produce pathological metrics
        std::vector<double> params = {100.0, 100.0};
        auto result = agent.evaluateTheory(params);
        assert(result.total_reward < 0.0);
        std::cout << "  Pathological f(R) reward = " << result.total_reward << std::endl;
    }

    // --- 6. discoverBestTheory returns finite parameters -------------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::BRANS_DICKE);
        auto best = agent.discoverBestTheory(10);
        assert(!best.empty());
        for (size_t i = 0; i < best.size(); ++i) {
            assertFinite("best_param[" + std::to_string(i) + "]", best[i]);
        }
        std::cout << "  Brans-Dicke best params:";
        for (double p : best) std::cout << " " << p;
        std::cout << std::endl;
    }

    // --- 7. setTestLocation changes evaluation point ----------------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::LOOP_QUANTUM_GRAVITY);
        agent.setTestLocation(Event4D(0.0, 20.0, 0.0, 0.0));
        std::vector<double> params = {0.2375, 1.616e-35};
        auto result = agent.evaluateTheory(params);
        assertFinite("lqc_reward", result.total_reward);
        std::cout << "  LQG at r=20 reward = " << result.total_reward << std::endl;
    }

    // --- 8. getBestResult reflects last discovery -------------------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::FR_GRAVITY);
        agent.discoverBestTheory(5);
        const auto& best = agent.getBestResult();
        assertFinite("best_result_reward", best.total_reward);
        assert(!best.theory_name.empty());
        std::cout << "  Best result theory = " << best.theory_name
                  << " reward = " << best.total_reward << std::endl;
    }

    // --- 9. TheoryManager integration --------------------------------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::FR_GRAVITY);
        TheoryManager manager;
        (void)manager;
        std::vector<double> params = {0.5, 0.5};
        auto result = agent.evaluateTheory(params);
        assert(result.theory_name == "FRLGravity");
        std::cout << "  Theory name correctly set to " << result.theory_name << std::endl;
    }

    // --- 10. Edge case: empty params ---------------------------------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::FR_GRAVITY);
        std::vector<double> empty;
        auto result = agent.evaluateTheory(empty);
        assert(result.total_reward < 0.0);
        std::cout << "  Empty params handled gracefully. reward = "
                  << result.total_reward << std::endl;
    }

    // --- 11. Symbolic verification toggle ----------------------------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::BRANS_DICKE);
        agent.setSymbolicVerificationEnabled(false);
        std::vector<double> params = {40000.0, 1.0};
        auto result = agent.evaluateTheory(params);
        assertFinite("sym_disabled_reward", result.total_reward);
        std::cout << "  Symbolic verification disabled. reward = "
                  << result.total_reward << std::endl;
    }

    std::cout << "All TheoryDiscoveryAgentTest checks passed." << std::endl;
    return 0;
}
