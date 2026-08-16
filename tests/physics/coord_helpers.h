#ifndef QUANTUMVERSE_TEST_COORD_HELPERS_H
#define QUANTUMVERSE_TEST_COORD_HELPERS_H

// Shared test helpers for reconciling the deliberate Cartesian-coordinate
// evaluation of SchwarzschildMetric (see MetricTensor.h: SchwarzschildMetric::
// evaluate) with the spherical-coordinate physics intent of the test suite.
//
// Many tests were originally written assuming the metric is returned in
// spherical coordinates (t, r, theta, phi). Since evaluate() now returns the
// metric in the (t, x, y, z) Cartesian basis that the curvature/geodesic
// calculators finite-difference, these helpers let a test express a point in
// spherical coordinates and obtain the metric expressed in spherical
// coordinates without changing the physical point being probed.

#include "spacetime/Event4D.h"
#include "spacetime/MetricTensor.h"
#include <cmath>
#include <array>

namespace quantumverse {
namespace test_helpers {

/// @brief Convert a spherical-coordinate point (t, r, theta, phi) to the
/// Cartesian Event4D (t, x, y, z) the metric actually evaluates in.
inline Event4D sphericalToCartesian(double t, double r, double theta, double phi) {
    double st = std::sin(theta), ct = std::cos(theta);
    double sp = std::sin(phi), cp = std::cos(phi);
    return Event4D(t, r * st * cp, r * st * sp, r * ct);
}

/// @brief Evaluate the metric at the spherical point (r, theta, phi) but
/// expressed in spherical coordinates (t, r, theta, phi).
///
/// This evaluates the Cartesian metric at the Cartesian image of the point and
/// transforms it with the (t,r,theta,phi) -> (t,x,y,z) Jacobian, so the
/// resulting matrix is the standard Schwarzschild metric
/// diag(-(1-rs/r), 1/(1-rs/r), r^2, r^2 sin^2(theta)).
inline std::array<std::array<double, 4>, 4> metricInSpherical(
    const MetricTensor& metric, double r, double theta, double phi) {
    Event4D ev = sphericalToCartesian(0.0, r, theta, phi);
    auto gC = metric.evaluate(ev);

    // Jacobian J[A][a] = dX^A / du^a, with A in {0=t,1=x,2=y,3=z} and
    // a in {0=t,1=r,2=theta,3=phi}: x = r sinθ cosφ, y = r sinθ sinφ, z = r cosθ.
    double st = std::sin(theta), ct = std::cos(theta);
    double sp = std::sin(phi), cp = std::cos(phi);
    double J[4][4] = {
        {1.0, 0.0,     0.0,      0.0},
        {0.0, st * cp, r * ct * cp, -r * st * sp},
        {0.0, st * sp, r * ct * sp,  r * st * cp},
        {0.0, ct,     -r * st,      0.0}
    };

    std::array<std::array<double, 4>, 4> gS{};
    for (int a = 0; a < 4; a++) {
        for (int b = 0; b < 4; b++) {
            double s = 0.0;
            for (int A = 0; A < 4; A++) {
                for (int B = 0; B < 4; B++) {
                    s += gC[A][B] * J[A][a] * J[B][b];
                }
            }
            gS[a][b] = s;
        }
    }
    return gS;
}

} // namespace test_helpers
} // namespace quantumverse

#endif // QUANTUMVERSE_TEST_COORD_HELPERS_H
