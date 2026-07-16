#include "math/AutoDiff.h"
#include "physics/DifferentiableCurvature.h"
#include "physics/DifferentiableGeodesicIntegrator.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include <cmath>
#include <cassert>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace quantumverse;
using namespace quantumverse::math;

// ============================================================================
// 3.4.1 - ADVariable arithmetic and transcendental functions
// ============================================================================
void test_advariable_basic_operations() {
    ADVariable<1> x = ADVariable<1>::variable(2.0, 0);
    ADVariable<1> y = ADVariable<1>::variable(3.0, 0);

    auto sum = x + y;
    assert(std::abs(sum.getValue() - 5.0) < 1e-12 && "AD sum value");
    assert(std::abs(sum[0] - 0.0) < 1e-12 && "AD sum derivative w.r.t. independent var");

    auto prod = x * y;
    assert(std::abs(prod.getValue() - 6.0) < 1e-12 && "AD product value");

    auto sin_x = sin(x);
    assert(std::abs(sin_x.getValue() - std::sin(2.0)) < 1e-12 && "AD sin value");

    auto cos_x = cos(x);
    assert(std::abs(cos_x.getValue() - std::cos(2.0)) < 1e-12 && "AD cos value");

    auto exp_x = exp(x);
    assert(std::abs(exp_x.getValue() - std::exp(2.0)) < 1e-12 && "AD exp value");

    auto log_x = log(x);
    assert(std::abs(log_x.getValue() - std::log(2.0)) < 1e-12 && "AD log value");

    auto sqrt_x = sqrt(x);
    assert(std::abs(sqrt_x.getValue() - std::sqrt(2.0)) < 1e-12 && "AD sqrt value");

    std::cout << "[PASS] ADVariable basic operations and transcendental functions" << std::endl;
}

// ============================================================================
// 3.4.2 - AD gradient of f(x) = x^3 + sin(x)
// ============================================================================
void test_ad_gradient_computation() {
    auto f = [](const std::vector<ADVariable<1>>& vars) -> ADVariable<1> {
        ADVariable<1> x = vars[0];
        return x * x * x + sin(x);
    };

    std::vector<double> x = {1.5};
    auto grad = gradient<1>(f, x);

    double expected_dfdx = 3.0 * 1.5 * 1.5 + std::cos(1.5);
    assert(std::abs(grad[0] - expected_dfdx) < 1e-6 && "AD gradient should match analytic");

    std::cout << "[PASS] AD gradient: computed=" << grad[0] << ", expected=" << expected_dfdx << std::endl;
}

// ============================================================================
// 3.4.3 - ADVariable chain rule: f(g(x)) where g(x)=x^2
// ============================================================================
void test_ad_chain_rule() {
    auto g = [](const std::vector<ADVariable<1>>& vars) -> ADVariable<1> {
        return vars[0] * vars[0];
    };

    auto f = [](const std::vector<ADVariable<1>>& vars) -> ADVariable<1> {
        return exp(vars[0]);
    };

    auto composite = [&](const std::vector<ADVariable<1>>& vars) -> ADVariable<1> {
        auto gv = g(vars);
        return f({gv});
    };

    std::vector<double> x = {2.0};
    auto grad = gradient<1>(composite, x);

    double expected = std::exp(4.0) * 2.0 * 2.0;
    assert(std::abs(grad[0] - expected) < 1e-5 && "Chain rule gradient should match analytic");

    std::cout << "[PASS] AD chain rule: computed=" << grad[0] << ", expected=" << expected << std::endl;
}

// ============================================================================
// 3.4.4 - DifferentiableCurvature: Kretschmann gradient w.r.t. mass
// ============================================================================
void test_differentiable_curvature_kretschmann_gradient() {
    DifferentiableCurvature diffCurv(1e-5);

    SchwarzschildMetric sch(10.0 * 1.989e30);
    diffCurv.setMetric(std::make_shared<SchwarzschildMetric>(sch));

    Event4D ev(0.0, 1e10, 0.0, 0.0);
    std::array<double, 1> params = {sch.mass()};

    auto grad = diffCurv.template computeKretschmannGradient<1>(ev, params, 1e-4);

    double r = 1e10;
    double M = sch.mass();
    double expected_dKdM = 96.0 * M / (r * r * r * r * r * r);

    assert(std::isfinite(grad[0]) && "Kretschmann gradient should be finite");
    double relError = std::abs(grad[0] - expected_dKdM) / (std::abs(expected_dKdM) + 1e-30);
    assert(relError < 1e-3 && "Kretschmann gradient should match analytic d(48M^2/r^6)/dM = 96M/r^6");
    (void)relError;

    std::cout << "[PASS] Kretschmann gradient: computed=" << grad[0]
              << ", expected=" << expected_dKdM << std::endl;
}

// ============================================================================
// 3.4.5 - DifferentiableGeodesicIntegrator: finite gradients
// ============================================================================
void test_differentiable_geodesic_gradients() {
    DifferentiableGeodesicIntegrator diffInt(1e-6);

    auto metric = std::make_shared<SchwarzschildMetric>(10.0 * 1.989e30);
    diffInt.setMetric(metric);

    Event4D start(0.0, 1e10, 0.0, 0.0);
    std::array<double, 4> vel = {0.0, 0.0, 0.0, 0.1};

    auto result = diffInt.integrateAD1(start, vel, GeodesicType::TIMELIKE, 10.0, 0.0, false);

    assert(result.success && "Integration should succeed");
    assert(std::isfinite(result.finalPosition[0].getValue()) && "Final t should be finite");

    for (int i = 0; i < 4; ++i) {
        assert(std::isfinite(result.finalPosition[i].getValue()) && "Final position component should be finite");
        assert(std::isfinite(result.finalPosition[i][0]) && "Final position gradient should be finite");
    }

    std::cout << "[PASS] DifferentiableGeodesicIntegrator produces finite gradients" << std::endl;
}

// ============================================================================
// 3.4.6 - ADVariable<2> multi-parameter gradient
// ============================================================================
void test_ad_multiparameter_gradient() {
    auto f = [](const std::vector<ADVariable<2>>& vars) -> ADVariable<2> {
        ADVariable<2> x = vars[0];
        ADVariable<2> y = vars[1];
        return x * x + y * y;
    };

    std::vector<double> x = {3.0, 4.0};
    auto grad = math::gradient<2>(f, x);

    assert(std::abs(grad[0] - 6.0) < 1e-6 && "df/dx should be 6");
    assert(std::abs(grad[1] - 8.0) < 1e-6 && "df/dy should be 8");

    std::cout << "[PASS] AD multi-parameter gradient: [df/dx, df/dy] = [" << grad[0] << ", " << grad[1] << "]" << std::endl;
}

int main() {
    test_advariable_basic_operations();
    test_ad_gradient_computation();
    test_ad_chain_rule();
    test_differentiable_curvature_kretschmann_gradient();
    test_differentiable_geodesic_gradients();
    test_ad_multiparameter_gradient();

    std::cout << "=== ALL AUTODIFF VALIDATION TESTS PASSED ===" << std::endl;
    return 0;
}
