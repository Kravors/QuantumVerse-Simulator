// QuantumVerse Test: Geodesic Integrator
// Tests the real GeodesicIntegrator class with edge cases

#include <iostream>
#include <cmath>
#include <vector>
#include <array>
#include <cassert>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "physics/GeodesicIntegrator.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

int main() {
    std::cout << "=== Geodesic Integrator Tests ===" << std::endl;

    // Test 1: Minkowski geodesic (straight line, particle at rest)
    {
        GeodesicIntegrator integrator(1e-6);
        auto metric = std::make_shared<MetricTensor>();
        metric->setToMinkowski();
        integrator.setMetric(metric);

        Event4D start(0.0, 1.0, 2.0, 3.0);
        std::array<double, 4> velocity = {1.0, 0.0, 0.0, 0.0};

        auto trajectory = integrator.integrate(start, velocity, GeodesicType::TIMELIKE, 0.1);

        assert(!trajectory.empty() && "Trajectory should not be empty");
        assert(trajectory.front().valid && "First step should be valid");
        std::cout << "[PASS] Minkowski geodesic produces non-empty trajectory" << std::endl;
    }

    // Test 2: Schwarzschild geodesic (non-flat spacetime)
    {
        GeodesicIntegrator integrator(1e-6);
        auto metric = std::make_shared<MetricTensor>(MetricTensor::schwarzschild(1.0, 10.0, M_PI / 2.0, 0.0));
        integrator.setMetric(metric);

        Event4D start(0.0, 10.0, 0.0, 0.0);
        std::array<double, 4> velocity = {1.0, 0.0, 0.0, 0.0};

        auto trajectory = integrator.integrate(start, velocity, GeodesicType::TIMELIKE, 0.1);

        assert(!trajectory.empty() && "Trajectory should not be empty");
        assert(trajectory.front().valid && "First step should be valid");
        std::cout << "[PASS] Schwarzschild geodesic integrates without crash" << std::endl;
    }

    // Test 3: Near-singularity behavior (r close to Schwarzschild radius)
    {
        GeodesicIntegrator integrator(1e-6);
        double mass = 1.0;
        double r_horizon = 2.0 * mass;
        double r_near = r_horizon * 1.01;
        auto metric = std::make_shared<MetricTensor>(MetricTensor::schwarzschild(mass, r_near, M_PI / 2.0, 0.0));
        integrator.setMetric(metric);

        Event4D start(0.0, r_near, 0.0, 0.0);
        std::array<double, 4> velocity = {1.0, 0.0, 0.0, 0.0};

        auto trajectory = integrator.integrate(start, velocity, GeodesicType::TIMELIKE, 0.01);

        assert(!trajectory.empty() && "Trajectory should not be empty near singularity");
        std::cout << "[PASS] Near-singularity integration terminates safely" << std::endl;
    }

    // Test 4: Changing metric field
    {
        GeodesicIntegrator integrator(1e-6);
        integrator.setMetricField([](const Event4D& evt) -> MetricTensor {
            double r = std::sqrt(evt.x * evt.x + evt.y * evt.y + evt.z * evt.z);
            double mass = 1.0 + 0.1 * evt.t;  // Time-varying mass
            return MetricTensor::schwarzschild(mass, std::max(r, 2.0 * mass), M_PI / 2.0, 0.0);
        });

        Event4D start(0.0, 10.0, 0.0, 0.0);
        std::array<double, 4> velocity = {1.0, 0.0, 0.0, 0.0};

        auto trajectory = integrator.integrate(start, velocity, GeodesicType::TIMELIKE, 0.1);

        assert(!trajectory.empty() && "Trajectory should not be empty with changing metric");
        std::cout << "[PASS] Changing metric field integrates without crash" << std::endl;
    }

    // Test 5: Lightlike geodesic
    {
        GeodesicIntegrator integrator(1e-6);
        auto metric = std::make_shared<MetricTensor>();
        metric->setToMinkowski();
        integrator.setMetric(metric);

        Event4D start(0.0, 0.0, 0.0, 0.0);
        std::array<double, 4> velocity = {1.0, 1.0, 0.0, 0.0};

        auto trajectory = integrator.integrate(start, velocity, GeodesicType::LIGHTLIKE, 0.1);

        assert(!trajectory.empty() && "Lightlike trajectory should not be empty");
        std::cout << "[PASS] Lightlike geodesic integrates without crash" << std::endl;
    }

    // Test 6: Spacelike geodesic
    {
        GeodesicIntegrator integrator(1e-6);
        auto metric = std::make_shared<MetricTensor>();
        metric->setToMinkowski();
        integrator.setMetric(metric);

        Event4D start(0.0, 0.0, 0.0, 0.0);
        std::array<double, 4> velocity = {0.0, 1.0, 0.0, 0.0};

        auto trajectory = integrator.integrate(start, velocity, GeodesicType::SPACELIKE, 0.1);

        assert(!trajectory.empty() && "Spacelike trajectory should not be empty");
        std::cout << "[PASS] Spacelike geodesic integrates without crash" << std::endl;
    }

    // Test 7: Very small step count (integrateSimple edge case)
    {
        GeodesicIntegrator integrator(1e-6);
        auto metric = std::make_shared<MetricTensor>();
        metric->setToMinkowski();
        integrator.setMetric(metric);

        Event4D start(0.0, 0.0, 0.0, 0.0);
        std::array<double, 4> velocity = {1.0, 0.0, 0.0, 0.0};

        auto path = integrator.integrateSimple(start, velocity, 1, 0.01);

        assert(path.size() >= 1 && "Path should have at least the start point");
        std::cout << "[PASS] integrateSimple with 1 step produces valid path" << std::endl;
    }

    // Test 8: Christoffel symbols in Minkowski (all zero)
    {
        GeodesicIntegrator integrator(1e-6);
        auto metric = std::make_shared<MetricTensor>();
        metric->setToMinkowski();
        integrator.setMetric(metric);

        Event4D pos(0.0, 0.0, 0.0, 0.0);
        integrator.computeChristoffelSymbols(pos);

        auto symbols = integrator.getChristoffelSymbols();
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                for (int k = 0; k < 4; k++)
                    assert(std::abs(symbols[i][j][k]) < 1e-6 && "Minkowski Christoffel should be zero");

        std::cout << "[PASS] Christoffel symbols are zero in Minkowski spacetime" << std::endl;
    }

    std::cout << "=== ALL GEODESIC INTEGRATOR TESTS PASSED ===" << std::endl;
    return 0;
}
