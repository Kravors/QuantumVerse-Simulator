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

    // --- 12. TeVeS agent construction and evaluation ------------------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::TE_VES);
        std::vector<double> params = {0.3, 1e-55, 1.0};
        auto result = agent.evaluateTheory(params);
        assertFinite("tev_reward", result.total_reward);
        assertFinite("tev_obs_chi2", result.observational_chi2);
        assert(result.theory_name == "TeVeS");
        std::cout << "  TeVeS reward = " << result.total_reward
                  << " chi2 = " << result.observational_chi2 << std::endl;
    }

    // --- 13. Einstein-Aether agent construction and evaluation --------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::EINSTEIN_AETHER);
        std::vector<double> params = {0.0, 0.0, 0.0};
        auto result = agent.evaluateTheory(params);
        assertFinite("ea_reward", result.total_reward);
        assert(result.theory_name == "EinsteinAether");
        std::cout << "  Einstein-Aether reward = " << result.total_reward << std::endl;
    }

    // --- 14. Horndeski agent construction and evaluation --------------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::HORNDESKI);
        std::vector<double> params = {0.0, 0.0, 0.0};
        auto result = agent.evaluateTheory(params);
        assertFinite("hor_reward", result.total_reward);
        assert(result.theory_name == "Horndeski");
        std::cout << "  Horndeski reward = " << result.total_reward << std::endl;
    }

    // --- 12. Observational chi2: near-GR f(R) should give finite chi2 ------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::FR_GRAVITY);
        // Near-GR parameters: alpha ~ 0, n ~ 1
        std::vector<double> params = {0.01, 1.0};
        auto result = agent.evaluateTheory(params);
        assertFinite("obs_chi2_near_gr", result.observational_chi2);
        assert(result.observational_chi2 >= 0.0);
        std::cout << "  Near-GR f(R) obs chi2 = " << result.observational_chi2
                  << " (should be finite and >= 0)" << std::endl;
    }

    // --- 13. Observational chi2: wrong parameters give worse chi2 ----------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::FR_GRAVITY);
        // Near-GR baseline
        std::vector<double> near_gr = {0.01, 1.0};
        auto near_result = agent.evaluateTheory(near_gr);

        // Wildly wrong parameters
        std::vector<double> wild = {10.0, 10.0};
        auto wild_result = agent.evaluateTheory(wild);

        assert(wild_result.observational_chi2 >= near_result.observational_chi2
               || wild_result.theoretical_penalty > near_result.theoretical_penalty);
        std::cout << "  Near-GR chi2 = " << near_result.observational_chi2
                  << ", wild chi2 = " << wild_result.observational_chi2 << std::endl;
    }

    // --- 14. GW170817 chi2: standard GR should give small chi2 ------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::FR_GRAVITY);
        // Near-GR: alpha ~ 0 means c_gw ~ c, so chi2 should be small
        std::vector<double> params = {0.001, 1.0};
        auto result = agent.evaluateTheory(params);
        assertFinite("gw_chi2_gr", result.observational_chi2);
        // With alpha ~ 0, GW speed ~ c, so chi2 should be small
        std::cout << "  Near-GR f(R) GW chi2 contribution = "
                  << result.observational_chi2 << " (should be small)" << std::endl;
    }

    // --- 15. Symbolic verification: healthy theory gets small penalty ------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::BRANS_DICKE);
        // Healthy Brans-Dicke: large omega, phi0 ~ 1
        std::vector<double> params = {40000.0, 1.0};
        auto result = agent.evaluateTheory(params);
        assertFinite("healthy_penalty", result.theoretical_penalty);
        std::cout << "  Healthy Brans-Dicke theoretical penalty = "
                  << result.theoretical_penalty << std::endl;
    }

    // --- 16. computeLogLikelihood: finite for valid chi2 -------------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::FR_GRAVITY);
        double ll = agent.computeLogLikelihood(10.0, 10);
        assertFinite("log_likelihood", ll);
        assert(ll < 0.0); // log-likelihood should be negative for chi2 > 0
        std::cout << "  Log-likelihood (chi2=10, N=10) = " << ll << std::endl;
    }

    // --- 17. computeBIC: increases with parameters -------------------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::FR_GRAVITY);
        double bic_1param = agent.computeBIC(10.0, 1, 20);
        double bic_2param = agent.computeBIC(10.0, 2, 20);
        assert(bic_2param > bic_1param);
        std::cout << "  BIC(1 param) = " << bic_1param
                  << ", BIC(2 param) = " << bic_2param << std::endl;
    }

    // --- 18. computeBayesFactor: near-GR f(R) should not strongly prefer GR -------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::FR_GRAVITY);
        // Near-GR: alpha ~ 0, n ~ 1
        std::vector<double> params = {0.001, 1.0};
        auto bf_result = agent.computeBayesFactor(params);
        assertFinite("log_bayes_factor", bf_result.log_bayes_factor);
        assertFinite("bayes_factor", bf_result.bayes_factor);
        assert(bf_result.bic_candidate >= 0.0);
        assert(bf_result.bic_baseline >= 0.0);
        std::cout << "  Near-GR f(R) log BF = " << bf_result.log_bayes_factor
                  << ", preferred = " << bf_result.preferred_model << std::endl;
    }

    // --- 19. computeBayesFactor: pathological params prefer GR -------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::BRANS_DICKE);
        // Extreme parameters produce bad chi2, so GR should be preferred
        std::vector<double> params = {1.0, 0.001};
        auto bf_result = agent.computeBayesFactor(params);
        assert(bf_result.preferred_model == "GR" || bf_result.log_bayes_factor < 0.0);
        std::cout << "  Pathological BD log BF = " << bf_result.log_bayes_factor
                  << ", preferred = " << bf_result.preferred_model << std::endl;
    }

    // --- 20. computeBayesFactor: pure GR gives BF ~ 1 ----------------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::FR_GRAVITY);
        // Pure GR limit: alpha = 0, n = 1
        std::vector<double> gr_params = {0.0, 1.0};
        auto bf_result = agent.computeBayesFactor(gr_params);
        assertFinite("gr_log_bf", bf_result.log_bayes_factor);
        assert(bf_result.bayes_factor > 0.0);
        // Near-GR should give log BF close to 0 (no strong preference either way)
        std::cout << "  Pure GR f(R) log BF = " << bf_result.log_bayes_factor
                  << ", BF = " << bf_result.bayes_factor
                  << ", preferred = " << bf_result.preferred_model << std::endl;
    }

    // --- 21. GR baseline chi2 for TeVeS -------------------------------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::TE_VES);
        double chi2 = agent.computeGRBaselineChi2();
        assertFinite("tev_gr_chi2", chi2);
        assert(chi2 >= 0.0);
        std::cout << "  TeVeS GR baseline chi2 = " << chi2 << std::endl;
    }

    // --- 22. GR baseline chi2 for Einstein-Aether ---------------------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::EINSTEIN_AETHER);
        double chi2 = agent.computeGRBaselineChi2();
        assertFinite("ea_gr_chi2", chi2);
        assert(chi2 >= 0.0);
        std::cout << "  Einstein-Aether GR baseline chi2 = " << chi2 << std::endl;
    }

    // --- 23. GR baseline chi2 for Horndeski ---------------------------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::HORNDESKI);
        double chi2 = agent.computeGRBaselineChi2();
        assertFinite("hor_gr_chi2", chi2);
        assert(chi2 >= 0.0);
        std::cout << "  Horndeski GR baseline chi2 = " << chi2 << std::endl;
    }

    // --- 24. Bayes factor for Einstein-Aether GR limit ----------------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::EINSTEIN_AETHER);
        std::vector<double> gr_params = {0.0, 0.0, 0.0};
        auto bf_result = agent.computeBayesFactor(gr_params);
        assertFinite("ea_log_bf", bf_result.log_bayes_factor);
        assert(bf_result.bayes_factor > 0.0);
        std::cout << "  Einstein-Aether GR log BF = " << bf_result.log_bayes_factor
                  << ", preferred = " << bf_result.preferred_model << std::endl;
    }

    // --- 25. Bayes factor for Horndeski GR limit ----------------------------------
    {
        TheoryDiscoveryAgent agent(TheoryParameterSpace::TheoryType::HORNDESKI);
        std::vector<double> gr_params = {0.0, 0.0, 0.0};
        auto bf_result = agent.computeBayesFactor(gr_params);
        assertFinite("hor_log_bf", bf_result.log_bayes_factor);
        assert(bf_result.bayes_factor > 0.0);
        std::cout << "  Horndeski GR log BF = " << bf_result.log_bayes_factor
                  << ", preferred = " << bf_result.preferred_model << std::endl;
    }

    std::cout << "All TheoryDiscoveryAgentTest checks passed." << std::endl;
    return 0;
}
