// QuantumVerse Test: Kerr Metric
// Validates Kerr metric for rotating black holes

#include <iostream>
#include <cmath>
#include <array>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "spacetime/KerrMetric.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "physics/PhysicsConstants.h"
#include "test_assert.h"

using namespace quantumverse;

int main() {
    std::cout << "=== Kerr Metric Tests ===" << std::endl;

    // Test 1: Kerr reduces to Schwarzschild when spin = 0
    {
        double mass = 1.989e30;  // 1 solar mass
        KerrMetric kerr(mass, 0.0);

        Event4D event(0.0, 1.0e10, 0.0, 0.0);

        SchwarzschildMetric schwarzschild(mass);
        auto g_schwarzschild = schwarzschild.evaluate(event);

        double tol = 1e-6;
        auto g_kerr = kerr.evaluate(event);
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                QV_CHECK(std::abs(g_kerr[i][j] - g_schwarzschild[i][j]) /
                       std::max(std::abs(g_schwarzschild[i][j]), 1.0) < tol);
            }
        }
        (void)tol;
        (void)g_kerr;
        std::cout << "[PASS] Kerr reduces to Schwarzschild when spin = 0" << std::endl;
    }

    // Test 2: Horizon radius calculation
    {
        double mass = 1.989e30;
        KerrMetric kerr(mass, 0.5);

        double rs = kerr.schwarzschildRadius();
        double r_plus = kerr.outerHorizonRadius();
        double r_minus = kerr.innerHorizonRadius();

        // For spin = 0.5: r_+ = M + sqrt(M^2 - a^2) = M(1 + sqrt(1 - 0.25)) = M(1 + sqrt(0.75))
        double M = PHYS_G() * mass / (PHYS_C() * PHYS_C());
        double expected_r_plus = M * (1.0 + std::sqrt(1.0 - 0.25));
        double expected_r_minus = M * (1.0 - std::sqrt(1.0 - 0.25));

        QV_CHECK(std::abs(r_plus - expected_r_plus) / expected_r_plus < 1e-6);
        QV_CHECK(std::abs(r_minus - expected_r_minus) / expected_r_minus < 1e-6);
        (void)expected_r_plus;
        (void)expected_r_minus;
        QV_CHECK(r_plus < rs);
        QV_CHECK(r_minus < r_plus);

        std::cout << "[PASS] Horizon radius calculation correct" << std::endl;
        std::cout << "       r_s = " << rs << " m" << std::endl;
        std::cout << "       r_+ = " << r_plus << " m" << std::endl;
        std::cout << "       r_- = " << r_minus << " m" << std::endl;
    }

    // Test 3: Frame-dragging is non-zero for spin > 0
    {
        double mass = 1.989e30;
        KerrMetric kerr(mass, 0.7);

        double r = 5.0 * kerr.schwarzschildRadius();  // 5x Schwarzschild radius
        double theta = M_PI / 2.0;  // Equatorial plane

        double omega = kerr.frameDraggingOmega(r, theta);
        QV_CHECK(std::abs(omega) > 1e-20);
        QV_CHECK(omega > 0.0);

        // Schwarzschild should have zero frame-dragging
        SchwarzschildMetric schwarzschild(mass);
        double omega_schwarz = schwarzschild.frameDraggingOmega(r, theta);
        QV_CHECK(std::abs(omega_schwarz) < 1e-30);
        (void)omega_schwarz;

        std::cout << "[PASS] Frame-dragging is non-zero for spin > 0" << std::endl;
        std::cout << "       omega = " << omega << " rad/s" << std::endl;
    }

    // Test 4: Ergosphere calculation
    {
        double mass = 1.989e30;
        KerrMetric kerr(mass, 0.6);

        {
            double r_eq_ergo = kerr.ergosphereRadiusEquatorial();
            double r_eq_expected = kerr.schwarzschildRadius();
            QV_CHECK(std::abs(r_eq_ergo - r_eq_expected) / r_eq_expected < 1e-6);
            (void)r_eq_ergo;
            (void)r_eq_expected;
        }

        // At pole (theta=0), ergosphere = horizon
        {
            double r_pole_ergo = kerr.ergosphereRadius(0.0);
            double r_pole_horizon = kerr.outerHorizonRadius();
            QV_CHECK(std::abs(r_pole_ergo - r_pole_horizon) / r_pole_horizon < 1e-6);
            (void)r_pole_ergo;
            (void)r_pole_horizon;
        }

        // Ergosphere >= horizon everywhere
        for (int i = 0; i <= 10; ++i) {
            double theta = M_PI * i / 20.0;
            double r_ergo = kerr.ergosphereRadius(theta);
            double r_horizon = kerr.outerHorizonRadius();
            QV_CHECK(r_ergo >= r_horizon);
            (void)r_ergo;
            (void)r_horizon;
        }

        std::cout << "[PASS] Ergosphere calculation correct" << std::endl;
    }

    // Test 5: Metric is Lorentzian outside horizon
    {
        double mass = 1.989e30;
        KerrMetric kerr(mass, 0.8);

        double r = 10.0 * kerr.schwarzschildRadius();
        Event4D event(0.0, r, 0.0, 0.0);
        auto g = kerr.evaluate(event);

        // Construct MetricTensor from evaluated g to check Lorentzian signature
        MetricTensor m;
        m.g = g;
        QV_CHECK(m.isLorentzian());

        std::cout << "[PASS] Metric is Lorentzian outside horizon" << std::endl;
    }

    // Test 6: Curvature scalars (vacuum: Ricci = 0)
    {
        double mass = 1.989e30;
        KerrMetric kerr(mass, 0.5);

        Event4D event(0.0, 1.0e10, 1.0e10, 1.0e10);
        auto scalars = kerr.curvatureScalars(event);
        (void)scalars;

        QV_CHECK(scalars.valid);
        QV_CHECK(std::abs(scalars.ricciScalar) < 1e-10);
        QV_CHECK(scalars.kretschmann > 0.0);
        QV_CHECK(scalars.weylSquared > 0.0);

        std::cout << "[PASS] Curvature scalars are physically correct (vacuum)" << std::endl;
    }

    // Test 7: Energy conservation (frame-dragging sign convention)
    {
        double mass = 1.989e30;
        KerrMetric kerr(mass, 0.9);

        // Frame-dragging should decrease with distance
        double r1 = 5.0 * kerr.schwarzschildRadius();
        double r2 = 10.0 * kerr.schwarzschildRadius();
        double theta = M_PI / 2.0;

        double omega1 = kerr.frameDraggingOmega(r1, theta);
        double omega2 = kerr.frameDraggingOmega(r2, theta);

        QV_CHECK(omega1 > omega2);

        std::cout << "[PASS] Frame-dragging decreases with distance" << std::endl;
        std::cout << "       omega(5rs) = " << omega1 << " rad/s" << std::endl;
        std::cout << "       omega(10rs) = " << omega2 << " rad/s" << std::endl;
    }

    // Test 8: Extremal Kerr limit (spin -> 1)
    {
        double mass = 1.989e30;
        KerrMetric kerr(mass, 0.99999);

        double r_plus = kerr.outerHorizonRadius();
        double r_minus = kerr.innerHorizonRadius();
        (void)r_minus;

        // In extremal limit, r_+ = r_- = M
        double M = PHYS_G() * mass / (PHYS_C() * PHYS_C());
        QV_CHECK(std::abs(r_plus - M) / M < 0.01);
        QV_CHECK(std::abs(r_minus - M) / M < 0.01);

        std::cout << "[PASS] Extremal Kerr limit (spin -> 1) correct" << std::endl;
        std::cout << "       r_+ = " << r_plus << " m, M = " << M << " m" << std::endl;
    }

    std::cout << "=== ALL KERR METRIC TESTS PASSED ===" << std::endl;
    return 0;
}
