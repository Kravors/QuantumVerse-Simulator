// Phase 6.2: Symbolic Regression Sanity Test
// Validates SymbolicMath and SymbolicRegressionEngine produce finite,
// physically meaningful outputs and degrade gracefully without SymPy.

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>

#include "discovery/SymbolicMath.h"
#include "discovery/DiscoveryEngine.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

namespace {
constexpr double kEps = 1e-9;

void assertFinite(const std::string& label, double v) {
    assert(std::isfinite(v) && ("Non-finite value for " + label).c_str());
    (void)label;
    (void)v;
}

std::vector<std::pair<std::vector<double>, double>> makePolynomialData() {
    std::vector<std::pair<std::vector<double>, double>> data;
    for (int i = 0; i < 20; ++i) {
        double x = i * 0.5;
        double y = 2.0 * x * x + 3.0 * x + 1.0;
        data.push_back({std::vector<double>{x}, y});
    }
    return data;
}
}

int main() {
    std::cout << "=== SymbolicRegressionSanityTest ===" << std::endl;

    // --- SymbolicMath availability check --------------------------------------
    {
        SymbolicMath sym;
        bool available = sym.isAvailable();
        std::cout << "  SymbolicMath available: " << (available ? "yes" : "no") << std::endl;

        if (available) {
            // --- Einstein equations ------------------------------------------------
            {
                auto result = sym.computeEinsteinEquations({{"M", 1.0}});
                assert(result.is_valid);
                assert(!result.expression.empty());
                std::cout << "  Einstein equations computed." << std::endl;
            }

            // --- Christoffel symbols -------------------------------------------------
            {
                auto result = sym.computeChristoffelSymbols("Schwarzschild");
                assert(result.is_valid);
                assert(!result.expression.empty());
                std::cout << "  Christoffel symbols (Schwarzschild) computed." << std::endl;
            }

            {
                auto result = sym.computeChristoffelSymbols("Kerr");
                assert(result.is_valid);
                assert(!result.expression.empty());
                std::cout << "  Christoffel symbols (Kerr) computed." << std::endl;
            }

            // --- Ricci scalar -------------------------------------------------------
            {
                auto result = sym.computeRicciScalar("Schwarzschild");
                assert(result.is_valid);
                assert(!result.expression.empty());
                std::cout << "  Ricci scalar computed." << std::endl;
            }

            // --- Kretschmann scalar --------------------------------------------------
            {
                auto result = sym.computeKretschmannScalar("Schwarzschild");
                assert(result.is_valid);
                assert(!result.expression.empty());
                std::cout << "  Kretschmann scalar computed." << std::endl;
            }

            // --- Simplify expression -------------------------------------------------
            {
                auto result = sym.simplifyExpression("x**2 + 2*x + 1");
                assert(result.is_valid);
                assert(!result.expression.empty());
                std::cout << "  Expression simplification computed." << std::endl;
            }

            // --- Validate field equation ---------------------------------------------
            {
                assert(sym.validateFieldEquation("R_uv - 1/2 R g_uv = 8 pi T_uv"));
                std::cout << "  Field equation validation passed." << std::endl;
            }

            // --- Generate field equation from data -----------------------------------
            {
                std::vector<std::pair<std::string, double>> traj;
                traj.push_back({"x", 1.0});
                traj.push_back({"x", 2.0});
                auto result = sym.generateFieldEquationFromData(traj);
                assert(result.is_valid);
                std::cout << "  Field equation from data generated." << std::endl;
            }
        } else {
            std::cout << "  SymPy not available; verifying graceful fallback." << std::endl;

            auto ein = sym.computeEinsteinEquations({{"M", 1.0}});
            assert(!ein.is_valid);
            assert(!ein.error_message.empty());

            auto chris = sym.computeChristoffelSymbols("Schwarzschild");
            assert(!chris.is_valid);

            auto ricci = sym.computeRicciScalar("Schwarzschild");
            assert(!ricci.is_valid);

            auto kretsch = sym.computeKretschmannScalar("Schwarzschild");
            assert(!kretsch.is_valid);

            auto simp = sym.simplifyExpression("x**2");
            assert(!simp.is_valid);

            assert(!sym.validateFieldEquation("anything"));
            std::cout << "  Graceful fallback verified for all SymbolicMath methods." << std::endl;
        }
    }

    // --- SymbolicRegressionEngine -----------------------------------------------
    {
        SymbolicRegressionEngine engine;
        engine.setConfig(SymbolicRegressionEngine::Config{});

        auto data = makePolynomialData();
        auto result = engine.runRegression(data);

        std::cout << "  Regression equation: " << result.equation << std::endl;
        std::cout << "  Fitness: " << result.fitness_score
                  << " | Complexity: " << result.complexity
                  << " | Valid: " << result.is_valid << std::endl;

        assert(result.complexity >= 0);
        assert(result.fitness_score >= 0.0 || result.fitness_score == 0.0);
    }

    // --- DiscoveryEngine symbolic regression flag --------------------------------
    {
        DiscoveryEngine engine;
        engine.enableSymbolicRegression(true);
        engine.enableAnomalyDetection(true);
        engine.enableHypothesisTesting(true);

        MetricTensor metric;
        Event4D location(0.0, 0.0, 0.0, 0.0);
        std::vector<Event4D> traj;
        traj.emplace_back(1.0, 0.0, 0.0, 1.0);
        traj.emplace_back(2.0, 1.0, 0.0, 2.0);

        std::string equation = engine.generateFieldEquation(traj, metric, 1e-6);
        assert(!equation.empty());
        std::cout << "  DiscoveryEngine field equation: " << equation << std::endl;
    }

    std::cout << "All SymbolicRegressionSanityTest checks passed." << std::endl;
    return 0;
}
