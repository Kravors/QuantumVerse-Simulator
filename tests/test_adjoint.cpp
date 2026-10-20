// QuantumVerse Test: Reverse-Mode Adjoint AD
// Validates the tape-based adjoint system in src/math/AutoDiff.h and
// the AdjointGeodesicIntegrator in src/physics/AdjointGeodesicIntegrator.h
//
// Note: ADTape is a static global (class-scope static std::vector,
// AutoDiff.h line 451). Nothing in production calls it from multiple
// threads yet (optimizeWithGradient() is sequential). Test 8 verifies
// clear()+record() isolation across repeated forward passes — the pattern
// optimizeWithGradient() uses.

#include <iostream>
#include <cmath>
#include <array>
#include <vector>
#include <memory>
#include <stdexcept>

#include "math/AutoDiff.h"
#include "physics/AdjointGeodesicIntegrator.h"
#include "physics/GeodesicIntegrator.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/KerrMetric.h"
#include "spacetime/Event4D.h"
#include "test_assert.h"

using namespace quantumverse;
using namespace math;
using namespace physics;

int main() {
    std::cout << "=== Adjoint AD Tests ===" << std::endl;
    std::cout.flush();

    // -----------------------------------------------------------------------
    // Test 1: TapeBasic — y = x^2, x = 3, dL/dx = 2*x = 6
    // Uses mul's default backward: a->grad += dc*b->value, b->grad += dc*a->value.
    // With a==b==x: x->grad = dc*x->value + dc*x->value = 2*1.0*3.0 = 6.0
    // -----------------------------------------------------------------------
    {
        ADTape::clear();

        ADVar* x = ADTape::record(3.0, nullptr);
        ADVar* y = mul(x, x);

        auto grads = ADTape::compute_gradients(*y);
        double dx = ADTape::get_gradient(x, grads);
        QV_CHECK_NEAR(dx, 6.0, 1e-9);

        std::cout << "[PASS] Test 1: TapeBasic — d(x^2)/dx at x=3 = " << dx << std::endl;
        std::cout.flush();
    }

    // -----------------------------------------------------------------------
    // Test 2: TapeChain — y = sin(x^2), dL/dx = 2*x*cos(x^2)
    // mul gives x->grad = 2*x (from x^2), sin gives x2->grad += dc*cos(x2->value)
    // Chain: x->grad = (dc*cos(x^2)) * (2*x) = 2*x*cos(x^2)
    // -----------------------------------------------------------------------
    {
        ADTape::clear();

        ADVar* x = ADTape::record(3.0, nullptr);
        ADVar* x2 = mul(x, x);
        ADVar* y = sin(x2);

        // mul's backward already set: x->grad += dc_x2 * x->value (twice = 2*x*dc_x2)
        // sin's backward sets: x2->grad += dc_y * cos(x2->value)
        // After backprop: dc_x2 = cos(9), dc_x = 2*3*cos(9) = 6*cos(9)
        auto grads = ADTape::compute_gradients(*y);
        double dx = ADTape::get_gradient(x, grads);
        double expected = 2.0 * 3.0 * std::cos(9.0);
        QV_CHECK_NEAR(dx, expected, 1e-9);

        std::cout << "[PASS] Test 2: TapeChain — d(sin(x^2))/dx at x=3 = " << dx << std::endl;
        std::cout.flush();
    }

    // -----------------------------------------------------------------------
    // Test 3: TapeClear — clear() gives independent forward passes
    // -----------------------------------------------------------------------
    {
        ADTape::clear();
        ADVar* a = ADTape::record(2.0, nullptr);
        (void)mul(a, a);
        ADTape::clear();

        ADVar* c = ADTape::record(3.0, nullptr);
        ADVar* d = mul(c, c);
        QV_CHECK_NEAR(d->value, 9.0, 1e-9);

        auto& vars = ADTape::getVariables();
        QV_CHECK(vars.size() == 2);

        std::cout << "[PASS] Test 3: TapeClear — tape size after clear = " << vars.size() << std::endl;
        std::cout.flush();
    }

    // -----------------------------------------------------------------------
    // Test 4: AdjointKretschmannVsFD — dK/dM via adjoint vs central FD
    // K = 48 (2GM/c^2)^2 / r^6 = 192 G^2 M^2 / (c^4 r^6)
    // dK/dM = 384 G^2 M / (c^4 r^6)
    // -----------------------------------------------------------------------
    {
        double mass = 1.0;
        Event4D location(0.0, 10.0, 0.0, 0.0);  // r=10, equator

        AdjointGeodesicIntegrator integrator(mass);
        double dK_adjoint = integrator.computeCurvatureGradient(mass, location);

        double r = 10.0;
        double G2 = Event4D::G * Event4D::G;
        double C4 = Event4D::C2 * Event4D::C2;
        auto K = [&](double m) { return 192.0 * G2 * m * m / (C4 * r * r * r * r * r * r); };
        double eps = 1e-6;
        double dK_fd = (K(mass + eps) - K(mass - eps)) / (2.0 * eps);
        double dK_exact = 384.0 * G2 * mass / (C4 * r * r * r * r * r * r);

        QV_CHECK_NEAR(dK_adjoint, dK_exact, 1e-6 * dK_exact);
        QV_CHECK_NEAR(dK_fd, dK_exact, 1e-6 * dK_exact);

        std::cout << "[PASS] Test 4: AdjointKretschmannVsFD — dK/dM = " << dK_adjoint
                  << " (FD = " << dK_fd << ", exact = " << dK_exact << ")" << std::endl;
        std::cout.flush();
    }

    // -----------------------------------------------------------------------
    // Test 5: AdjointStateGradientVsFD — d(r_final)/dM via adjoint vs FD
    // -----------------------------------------------------------------------
    {
        double mass = 1.0;
        Event4D start(0.0, 6.0, 0.0, 0.0);
        std::array<double, 4> vel = {0.0, 0.0, 0.0, 0.0};
        double targetPT = 10.0;

        AdjointGeodesicIntegrator integrator(mass);
        auto result = integrator.integrate(start, vel, GeodesicType::TIMELIKE, targetPT);
        QV_CHECK(result.success);

        double dr_dM_adjoint = result.dState_dParams[1][0];

        auto fd_grad = integrator.computePositionGradientFD(start, vel, GeodesicType::TIMELIKE, targetPT);
        double dr_dM_fd = fd_grad[1];

        QV_CHECK_NEAR(dr_dM_adjoint, dr_dM_fd, 1e-4);

        std::cout << "[PASS] Test 5: AdjointStateGradientVsFD — dr/dM adjoint = " << dr_dM_adjoint
                  << ", FD = " << dr_dM_fd << std::endl;
        std::cout.flush();
    }

    // -----------------------------------------------------------------------
    // Test 6: AdjointVacuumRicci — Kretschmann gradient matches analytic
    // -----------------------------------------------------------------------
    {
        double mass = 1.0;
        Event4D location(0.0, 10.0, 0.0, 0.0);

        AdjointGeodesicIntegrator integrator(mass);
        double dK_dM = integrator.computeCurvatureGradient(mass, location);

        double G2 = Event4D::G * Event4D::G;
        double C4 = Event4D::C2 * Event4D::C2;
        double expected = 384.0 * G2 * mass / (C4 * 10.0 * 10.0 * 10.0 * 10.0 * 10.0 * 10.0);
        QV_CHECK_NEAR(dK_dM, expected, 1e-9 * expected);

        std::cout << "[PASS] Test 6: AdjointVacuumRicci — dK/dM = " << dK_dM
                  << " (expected " << expected << ")" << std::endl;
        std::cout.flush();
    }

    // -----------------------------------------------------------------------
    // Test 7: AdjointMemory — clear() releases tape nodes
    // -----------------------------------------------------------------------
    {
        ADTape::clear();
        ADVar* a = ADTape::record(1.0, nullptr);
        (void)mul(a, a);
        size_t before = ADTape::getVariables().size();
        QV_CHECK(before >= 2);

        ADTape::clear();
        size_t after = ADTape::getVariables().size();
        QV_CHECK(after == 0);

        std::cout << "[PASS] Test 7: AdjointMemory — tape nodes before=" << before
                  << ", after clear=" << after << std::endl;
        std::cout.flush();
    }

    // -----------------------------------------------------------------------
    // Test 8: AdjointTapeIsolation — sequential clear()+record() isolation
    // ADTape is a static global (class-scope static std::vector, no
    // thread_local or mutex — see AutoDiff.h line 451). Nothing in production
    // calls it from multiple threads yet (optimizeWithGradient() is
    // sequential), but every forward pass must start from a clean tape.
    // This test verifies that clear()+record() produces correct,
    // independent gradients across repeated forward passes — exactly the
    // pattern optimizeWithGradient() uses.
    // -----------------------------------------------------------------------
    {
        struct Case {
            double x;
            double expected_grad;
        };
        Case cases[] = {
            { 3.0, 6.0 },     // d(x^2)/dx at x=3 = 6
            { 5.0, 10.0 },    // d(x^2)/dx at x=5 = 10
            { -2.0, -4.0 },   // d(x^2)/dx at x=-2 = -4
            { 0.0, 0.0 },     // d(x^2)/dx at x=0 = 0
        };

        for (const auto& c : cases) {
            ADTape::clear();
            ADVar* x = ADTape::record(c.x, nullptr);
            ADVar* y = mul(x, x);
            auto grads = ADTape::compute_gradients(*y);
            double dx = ADTape::get_gradient(x, grads);
            QV_CHECK_NEAR(dx, c.expected_grad, 1e-9);

            // Tape should contain exactly x and y (2 nodes) after this pass
            auto& vars = ADTape::getVariables();
            QV_CHECK(vars.size() == 2);

            // clear() must fully reset — next pass starts empty
            ADTape::clear();
            QV_CHECK(ADTape::getVariables().size() == 0);
        }

        std::cout << "[PASS] Test 8: AdjointTapeIsolation — " << sizeof(cases)/sizeof(cases[0])
                  << " sequential forward passes, each with correct independent gradient" << std::endl;
        std::cout.flush();
    }

    std::cout << "=== ALL ADJOINT AD TESTS PASSED ===" << std::endl;
    return 0;
}
