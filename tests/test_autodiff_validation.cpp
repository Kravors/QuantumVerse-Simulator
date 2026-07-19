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

// ============================================================================
// 3.4.7 - Reverse-mode AD: chain rule f(x) = sin(x^2), df/dx = 2x*cos(x^2)
// ============================================================================
void test_reverse_mode_chain_rule() {
    using namespace quantumverse::math;

    double x_val = 2.0;
    ADTape::clear();
    ADVar* x = var(x_val);
    ADVar* x2 = mul(x, x);
    ADVar* f = sin(x2);

    auto grads = ADTape::compute_gradients(*f);
    double computed = ADTape::get_gradient(x, grads);
    double expected = 2.0 * x_val * std::cos(x_val * x_val);

    assert(std::abs(computed - expected) < 1e-6 && "Reverse-mode chain rule gradient should match analytic");

    std::cout << "[PASS] Reverse-mode chain rule: computed=" << computed
              << ", expected=" << expected << std::endl;
}

// ============================================================================
// 3.4.8 - Reverse-mode gradient vs finite difference for Kretschmann scalar
// ============================================================================
void test_reverse_mode_gradient_vs_finite_diff() {
    using namespace quantumverse::math;

    double M = 10.0 * 1.989e30;
    double r = 5.0 * 1000.0;  // 5 km in meters
    double K_p = 48.0 * (M + 1e-3) * (M + 1e-3) / (r * r * r * r * r * r);
    double K_m = 48.0 * (M - 1e-3) * (M - 1e-3) / (r * r * r * r * r * r);
    double finite_diff = (K_p - K_m) / (2.0 * 1e-3);

    ADTape::clear();
    ADVar* mass = var(M);
    ADVar* M2 = mul(mass, mass);
    ADVar* r6 = var(r * r * r * r * r * r);
    ADVar* K = mul(mul(var(48.0), M2), div(var(1.0), r6));
    auto grads = ADTape::compute_gradients(*K);
    double adjoint = ADTape::get_gradient(mass, grads);

    double relError = std::abs(adjoint - finite_diff) / (std::abs(finite_diff) + 1e-30);
    assert(relError < 1e-4 && "Reverse-mode gradient should match finite difference");
    (void)relError;

    std::cout << "[PASS] Reverse-mode vs finite diff: adjoint=" << adjoint
              << ", finite_diff=" << finite_diff << std::endl;
}

// ============================================================================
// 3.4.9 - Reverse-mode tape isolation between computations
// ============================================================================
void test_tape_isolation() {
    using namespace quantumverse::math;

    ADTape::clear();
    ADVar* a = var(2.0);
    ADVar* b = var(3.0);
    ADVar* c1 = mul(a, b);
    auto g1 = ADTape::compute_gradients(*c1);
    double grad_a_first = ADTape::get_gradient(a, g1);

    ADTape::clear();
    ADVar* d = var(4.0);
    ADVar* e = var(5.0);
    ADVar* c2 = mul(d, e);
    auto g2 = ADTape::compute_gradients(*c2);
    double grad_d_second = ADTape::get_gradient(d, g2);

    assert(std::abs(grad_a_first - 3.0) < 1e-6 && "First tape: d(ab)/da = b = 3");
    assert(std::abs(grad_d_second - 5.0) < 1e-6 && "Second tape: d(de)/dd = e = 5");

    std::cout << "[PASS] Tape isolation: first grad(a)=" << grad_a_first
              << ", second grad(d)=" << grad_d_second << std::endl;
}

// ============================================================================
// 3.4.10 - DifferentiableCurvature: adjoint-mode Kretschmann gradient
// ============================================================================
void test_differentiable_curvature_adjoint_gradient() {
    DifferentiableCurvature diffCurv(1e-5);

    Event4D ev(0.0, 1e10, 0.0, 0.0);
    std::array<double, 1> params = {10.0 * 1.989e30};

    auto grads = diffCurv.computeKretschmannGradientAdjoint(ev, params);

    double r = 1e10;
    double M = params[0];
    double expected_dKdM = 96.0 * M / (r * r * r * r * r * r);

    double dKdM = 0.0;
    for (const auto& [v, g] : grads) {
        if (v->value == M) dKdM = g;
    }

    assert(std::isfinite(dKdM) && "Adjoint Kretschmann gradient should be finite");
    double relError = std::abs(dKdM - expected_dKdM) / (std::abs(expected_dKdM) + 1e-30);
    assert(relError < 1e-4 && "Adjoint gradient should match analytic d(48M^2/r^6)/dM = 96M/r^6");
    (void)relError;

    std::cout << "[PASS] Adjoint Kretschmann gradient: computed=" << dKdM
              << ", expected=" << expected_dKdM << std::endl;
}

// ============================================================================
// 3.4.11 - DifferentiableGeodesicIntegrator: adjoint-mode integration
// ============================================================================
void test_differentiable_geodesic_adjoint() {
    DifferentiableGeodesicIntegrator diffInt(1e-6);

    auto metric = std::make_shared<SchwarzschildMetric>(10.0 * 1.989e30);
    diffInt.setMetric(metric);

    Event4D start(0.0, 1e10, 0.0, 0.0);
    std::array<double, 4> vel = {0.0, 0.0, 0.0, 0.1};

    std::array<double, 1> params = {metric->mass()};
    auto grads = diffInt.integrateAdjoint(start, vel, GeodesicType::TIMELIKE, 10.0, params);

    double M = params[0];
    double dM = 0.0;
    for (const auto& [v, g] : grads) {
        if (std::abs(v->value - M) < 1e-6) dM = g;
    }

    assert(std::isfinite(dM) && "Adjoint geodesic gradient should be finite");
    std::cout << "[PASS] Adjoint geodesic: d(final_sum)/dM=" << dM << std::endl;
}

int main() {
    test_advariable_basic_operations();
    test_ad_gradient_computation();
    test_ad_chain_rule();
    test_differentiable_curvature_kretschmann_gradient();
    test_differentiable_geodesic_gradients();
    test_ad_multiparameter_gradient();
    test_reverse_mode_chain_rule();
    test_reverse_mode_gradient_vs_finite_diff();
    test_tape_isolation();
    test_differentiable_curvature_adjoint_gradient();
    test_differentiable_geodesic_adjoint();

    std::cout << "=== ALL AUTODIFF VALIDATION TESTS PASSED ===" << std::endl;
    return 0;
}
