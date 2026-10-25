/**
 * @file KerrGeodesic.cpp
 * @brief Equatorial Kerr null geodesic reference integrator.
 *
 * Implements the equatorial (theta = pi/2) Kerr null geodesic orbit equation
 * in u = 1/r form:
 *
 *   (du/dphi)^2 = (1 - 2Mu + a^2 u^2)^2 * [1 - (b^2 - a^2)u^2 + 2M(b-a)^2 u^3]
 *                / [b + 2M(a-b)u]^2
 *
 * Derived from the Kerr metric null geodesic equations by eliminating the
 * affine parameter lambda between the radial and angular equations:
 *
 *   Sigma^2 (dr/dlambda)^2 = R(r) = [E(r^2 + a^2) - aL]^2 - Delta(L - aE)^2
 *
 * Cross-checked against 4D GeodesicIntegrator + KerrMetric integration of
 * equatorial photon orbits. Both methods must agree on photon sphere radii.
 *
 * Scope: Equatorial plane only (theta = pi/2). Full 3D Kerr geodesics
 * (out-of-plane photon orbits) require the Carter constant and coupled
 * r/theta ODEs - use GeodesicIntegrator with KerrMetric directly.
 */

#include "KerrGeodesic.h"

#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

KerrGeodesic::KerrGeodesic(double M, double aDimless) noexcept
    : M_(M), aDimless_(aDimless), a_(aDimless * M) {}

double KerrGeodesic::orbitEquation(double u, double M, double a, double b) noexcept {
    double u2 = u * u;
    double u3 = u2 * u;

    // A(u) = (1 - 2Mu + a^2 u^2)^2
    double A_inner = 1.0 - 2.0 * M * u + a * a * u2;
    double A = A_inner * A_inner;

    // B(u) = 1 - (b^2 - a^2)u^2 + 2M(b-a)^2 u^3
    double b2ma2 = b * b - a * a;
    double bma = b - a;
    double bma2 = bma * bma;
    double B = 1.0 - b2ma2 * u2 + 2.0 * M * bma2 * u3;

    // C(u) = b + 2M(a-b)u
    double C = b + 2.0 * M * (a - b) * u;

    if (std::abs(C) < 1e-300) return 0.0;  // Avoid division by zero

    return A * B / (C * C);
}

double KerrGeodesic::orbitEquationDerivative(double u, double M, double a, double b) noexcept {
    // Numerical differentiation: F'(u) ≈ [F(u+h) - F(u-h)] / (2h)
    // Central difference with relative step size for accuracy across scales.
    double h = 1e-8 * std::max(1.0, std::abs(u));
    double fPlus = orbitEquation(u + h, M, a, b);
    double fMinus = orbitEquation(u - h, M, a, b);
    return (fPlus - fMinus) / (2.0 * h);
}

double KerrGeodesic::photonSphereRadius(bool prograde) const noexcept {
    //   r^3 = M(b - a)^2        (radial force balance)
    //   b^2 = a^2 + 3r^2        (circular orbit condition)
    //
    // Eliminating b:
    //   Prograde (b > 0):   r^3 = M(sqrt(a^2 + 3r^2) - a)^2
    //   Retrograde (b < 0): r^3 = M(sqrt(a^2 + 3r^2) + a)^2
    //
    // Solved via bisection on r in [M, 20M].
    //
    // Known analytic values (M = 1):
    //   a = 0:    r_ph = 3.000 M
    //   a = 0.9 pro:  r_ph ≈ 1.561 M
    //   a = 0.9 retro: r_ph ≈ 3.911 M

    double rLo = M_;           // Below horizon for sub-extremal Kerr
    double rHi = 20.0 * M_;    // Well outside any photon sphere

    for (int iter = 0; iter < 100; iter++) {
        double rMid = 0.5 * (rLo + rHi);
        double b = prograde ? std::sqrt(a_ * a_ + 3.0 * rMid * rMid)
                            : -std::sqrt(a_ * a_ + 3.0 * rMid * rMid);
        double bma = b - a_;
        double rhs = M_ * bma * bma;   // M(b-a)^2
        double lhs = rMid * rMid * rMid;  // r^3
        double diff = lhs - rhs;

        if (std::abs(diff) < 1e-12 * M_ * M_ * M_) break;

        if (diff > 0.0) {
            rHi = rMid;  // r^3 too big, need smaller r
        } else {
            rLo = rMid;  // r^3 too small, need larger r
        }
    }

    return 0.5 * (rLo + rHi);
}

double KerrGeodesic::photonSphereImpactParameter(bool prograde) const noexcept {
    double rPh = photonSphereRadius(prograde);
    return prograde ? std::sqrt(a_ * a_ + 3.0 * rPh * rPh)
                    : -std::sqrt(a_ * a_ + 3.0 * rPh * rPh);
}

double KerrGeodesic::traceRay(double b) const noexcept {
    // End-to-end validation of the reduced orbit equation: trace a null
    // geodesic from r=30M to escape or capture, computing the deflection.
    //
    // At a=0, this must match the Schwarzschild deflection (~0.45 rad at b=12M,
    // ~1.72 rad at b=6M) computed by 4D GeodesicIntegrator + KerrMetric.
    // Any coefficient error in the reduced ODE shows up here as a deflection
    // that doesn't match the 4D reference.
    //
    // Uses reflection symmetry of Schwarzschild equatorial geodesics:
    // integrates only the ingoing half from r_start to periapsis, then
    // doubles the phi sweep to get the full deflection.
    //
    // Returns -1.0 if captured by horizon, deflection angle in radians if escaped.
    //
    // Steps in u with FIXED step size du. dPhi = du / sqrt(F(u)) varies
    // naturally: large steps where F is large (far from periapsis), small
    // steps where F is small (near periapsis). This ensures u reaches
    // periapsis in finite steps while the integral converges correctly.

    const double rStart = 30.0 * M_;
    const double uStart = 1.0 / rStart;
    const double uHorizon = 1.0 / horizonRadius();

    double u = uStart;
    double phi = 0.0;

    // Fixed step in u. dPhi = du / sqrt(F(u)) adapts automatically:
    // large steps where F is large (far from periapsis), small steps
    // where F is small (near periapsis).
    const double du = 0.00001;  // ~1e-5 in u per step

    while (true) {
        const double r = 1.0 / u;

        // Check capture by event horizon
        if (r < uHorizon) return -1.0;

        double F = orbitEquationInstance(u, b);
        if (F < 0.0) return -1.0;  // captured (b < b_crit)

        if (F < 1e-12) {
            // At periapsis! Use reflection symmetry:
            // φ_total = 2 * φ_periapsis
            // deflection = φ_total - 2 * acos(b/r_start)
            double straightSweep = 2.0 * std::acos(b / rStart);
            return 2.0 * phi - straightSweep;
        }

        double dPhi = du / std::sqrt(F);
        phi += dPhi;
        u += du;

        // Safety: stop if phi gets too large (shouldn't happen for b > b_crit)
        if (phi > 1000.0 * M_PI) return -1.0;
    }
}

} // namespace quantumverse
