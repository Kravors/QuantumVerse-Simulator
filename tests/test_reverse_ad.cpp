// QuantumVerse Test: Reverse-Mode AD
// Validates ReverseTape<N> / ReverseAD<N> in src/math/AutoDiff.h
// Isolated AD tests before integration with the geodesic integrator.

#include <iostream>
#include <cmath>
#include <array>
#include "math/AutoDiff.h"
#include "test_assert.h"

using namespace quantumverse::math;

int main() {
    std::cout << "=== Reverse AD Tests ===" << std::endl;

    // Test 1: f(x) = x^2 at x=3 -> df/dx = 2x = 6 (exact)
    {
        ReverseTape<1> tape;
        tape.reserve(64);
        auto x = ReverseAD<1>::param(tape, 3.0, 0);
        auto f = x * x;
        auto grad = tape.backward(f.node);
        QV_CHECK(std::fabs(grad[0] - 6.0) < 1e-10);
        std::cout << "[PASS] f(x)=x^2 at x=3: df/dx = " << grad[0] << std::endl;
    }

    // Test 2: f(x,y) = x*y + sin(x) at (2, 5)
    //   df/dx = y + cos(x) = 5 + cos(2)
    //   df/dy = x = 2
    {
        ReverseTape<2> tape;
        tape.reserve(64);
        auto x = ReverseAD<2>::param(tape, 2.0, 0);
        auto y = ReverseAD<2>::param(tape, 5.0, 1);
        auto f = x * y + sin(x);
        auto grad = tape.backward(f.node);
        QV_CHECK(std::fabs(grad[0] - (5.0 + std::cos(2.0))) < 1e-10);
        QV_CHECK(std::fabs(grad[1] - 2.0) < 1e-10);
        std::cout << "[PASS] f(x,y)=x*y+sin(x): df/dx = " << grad[0]
                  << ", df/dy = " << grad[1] << std::endl;
    }

    // Test 3: f(x,y,z) = (x+y)^2 - z at (1, 2, 3)
    //   df/dx = 2*(x+y) = 6
    //   df/dy = 2*(x+y) = 6
    //   df/dz = -1
    {
        ReverseTape<3> tape;
        tape.reserve(64);
        auto x = ReverseAD<3>::param(tape, 1.0, 0);
        auto y = ReverseAD<3>::param(tape, 2.0, 1);
        auto z = ReverseAD<3>::param(tape, 3.0, 2);
        auto s = x + y;
        auto f = s * s - z;
        auto grad = tape.backward(f.node);
        QV_CHECK(std::fabs(grad[0] - 6.0) < 1e-10);
        QV_CHECK(std::fabs(grad[1] - 6.0) < 1e-10);
        QV_CHECK(std::fabs(grad[2] - (-1.0)) < 1e-10);
        std::cout << "[PASS] f(x,y,z)=(x+y)^2-z: grad = [" << grad[0] << ", "
                  << grad[1] << ", " << grad[2] << "]" << std::endl;
    }

    // Test 4: Chain rule: f(g(x)) where g(x)=sin(x), f(y)=y^2
    //   df/dx = 2*sin(x)*cos(x) = sin(2x)
    {
        ReverseTape<1> tape;
        tape.reserve(64);
        auto x = ReverseAD<1>::param(tape, 0.7, 0);
        auto g = sin(x);
        auto f = g * g;
        auto grad = tape.backward(f.node);
        double expected = 2.0 * std::sin(0.7) * std::cos(0.7);
        QV_CHECK(std::fabs(grad[0] - expected) < 1e-10);
        std::cout << "[PASS] Chain rule f(g(x)): df/dx = " << grad[0] << std::endl;
    }

    // Test 5: Gradient vs central FD on a 4-param scalar function
    //   f(p) = p0^2 + p1*p2 + exp(p3)
    //   df/dp0 = 2*p0, df/dp1 = p2, df/dp2 = p1, df/dp3 = exp(p3)
    {
        ReverseTape<4> tape;
        tape.reserve(128);
        auto p0 = ReverseAD<4>::param(tape, 1.5, 0);
        auto p1 = ReverseAD<4>::param(tape, 2.0, 1);
        auto p2 = ReverseAD<4>::param(tape, 3.0, 2);
        auto p3 = ReverseAD<4>::param(tape, 0.5, 3);
        auto f = p0 * p0 + p1 * p2 + exp(p3);
        auto grad = tape.backward(f.node);

        std::array<double, 4> params = {1.5, 2.0, 3.0, 0.5};
        const double eps = 1e-7;
        auto func = [](const std::array<double, 4>& p) {
            return p[0] * p[0] + p[1] * p[2] + std::exp(p[3]);
        };
        for (size_t i = 0; i < 4; i++) {
            auto pPlus = params; auto pMinus = params;
            pPlus[i] += eps; pMinus[i] -= eps;
            double fd = (func(pPlus) - func(pMinus)) / (2.0 * eps);
            QV_CHECK(std::fabs(grad[i] - fd) < 1e-6);
        }
        std::cout << "[PASS] 4-param gradient matches central FD to 1e-6" << std::endl;
    }

    // Test 6: Backward pass reusability (adjoint reset works)
    {
        ReverseTape<1> tape;
        tape.reserve(64);
        auto x = ReverseAD<1>::param(tape, 3.0, 0);
        auto f = x * x;
        auto grad1 = tape.backward(f.node);
        auto grad2 = tape.backward(f.node);
        QV_CHECK(std::fabs(grad1[0] - grad2[0]) < 1e-12);
        QV_CHECK(std::fabs(grad1[0] - 6.0) < 1e-10);
        std::cout << "[PASS] Backward pass reusable: two calls give same gradient" << std::endl;
    }

    std::cout << "=== All Reverse AD tests passed ===" << std::endl;
    std::cout.flush();
    return 0;
}
