#pragma once

/**
 * @file KerrGeodesic.h
 * @brief Equatorial Kerr null geodesic reference integrator.
 *
 * Implements the equatorial (theta = pi/2) Kerr null geodesic orbit equation
 * in u = 1/r form:
 *
 *   (du/dphi)^2 = (1 - 2Mu + a^2 u^2)^2 * [1 - (b^2 - a^2)u^2 + 2M(b-a)^2 u^3]
 *                / [b + 2M(a-b)u]^2
 *
 * where u = 1/r, b = L/E is the impact parameter, M is the geometric mass
 * (M = GM/c^2), and a = a*M is the specific angular momentum.
 *
 * Derived from the Kerr metric null geodesic equations:
 *   Sigma^2 (dr/dlambda)^2 = R(r) = [E(r^2 + a^2) - aL]^2 - Delta(L - aE)^2
 *   Sigma^2 (dphi/dlambda) = P(r) ... (angular equation)
 *
 * Eliminating lambda gives the single first-order ODE in u = 1/r above.
 *
 * Scope: Equatorial plane only (theta = pi/2). For full 3D Kerr geodesics
 * (out-of-plane photon orbits), use GeodesicIntegrator with KerrMetric directly.
 *
 * Cross-check: This 2D reduction must agree with 4D GeodesicIntegrator +
 * KerrMetric integration of equatorial photon orbits (theta = pi/2,
 * dtheta/dlambda = 0).
 */

#ifndef QUANTUMVERSE_KERR_GEODESIC_H
#define QUANTUMVERSE_KERR_GEODESIC_H

#include <array>
#include <cmath>
#include <utility>
#include <vector>

namespace quantumverse {

/**
 * @brief Equatorial Kerr null geodesic solver.
 *
 * Provides the orbit equation (du/dphi)^2 = F(u, a, b), photon sphere
 * radius computation, and RK4 orbit integration for equatorial Kerr
 * null geodesics. All quantities in geometric units (G = c = 1).
 */
class KerrGeodesic {
public:
    /**
     * @brief Construct equatorial Kerr null geodesic solver.
     * @param M Geometric mass M = GM/c^2 (in meters).
     * @param aDimless Dimensionless spin a* = a/M in [0, 1).
     */
    KerrGeodesic(double M, double aDimless) noexcept;

    /**
     * @brief Orbit equation: F(u) = (du/dphi)^2.
     *
     * Equatorial Kerr null geodesic in u = 1/r form:
     *
     *   F(u) = (1 - 2Mu + a^2 u^2)^2 * [1 - (b^2 - a^2)u^2 + 2M(b-a)^2 u^3]
     *        / [b + 2M(a-b)u]^2
     *
     * @param u Inverse radius 1/r (1/meters).
     * @param M Geometric mass (meters).
     * @param a Specific angular momentum a = a*M (meters).
     * @param b Impact parameter L/E (meters).
     * @return Value of (du/dphi)^2 (1/meters^2).
     */
    static double orbitEquation(double u, double M, double a, double b) noexcept;

    /**
     * @brief Derivative of orbit equation: F'(u) = d/du[(du/dphi)^2].
     *
     * Used for photon sphere condition F'(u) = 0 and for computing
     * d^2u/dphi^2 = F'(u)/2 in orbit integration.
     *
     * @param u Inverse radius 1/r (1/meters).
     * @param M Geometric mass (meters).
     * @param a Specific angular momentum a = a*M (meters).
     * @param b Impact parameter L/E (meters).
     * @return Value of d/du[(du/dphi)^2] (1/meters^3).
     */
    static double orbitEquationDerivative(double u, double M, double a, double b) noexcept;

    /**
     * @brief RK4 right-hand side: d^2u/dphi^2 = F'(u)/2.
     *
     * State vector: [u, du/dphi]. Derivative: [du/dphi, F'(u)/2].
     *
     * @param u Inverse radius 1/r.
     * @param dup du/dphi.
     * @return {du/dphi, d^2u/dphi^2}.
     */
    static std::array<double, 2> orbitRhs(double u, double dup,
                                          double M, double a, double b) noexcept;

    /**
     * @brief Compute photon sphere radius for equatorial Kerr.
     *
     * Solves for circular null orbit: F(u) = 0 and F'(u) = 0.
     * Uses the analytic relations:
     *   b^2 = a^2 + 3r^2
     *   r^3 = M(b - a)^2
     *
     * Solved via bisection on r in [M, 10M].
     *
     * @param prograde True for prograde (co-rotating, b > 0),
     *                 false for retrograde (counter-rotating, b < 0).
     * @return Photon sphere radius r_ph in geometric units (meters).
     */
    double photonSphereRadius(bool prograde) const noexcept;

    /**
     * @brief Impact parameter at the photon sphere.
     *
     * @param prograde True for prograde, false for retrograde.
     * @return Critical impact parameter b_crit in geometric units (meters).
     */
    double photonSphereImpactParameter(bool prograde) const noexcept;

    /**
     * @brief Integrate equatorial null geodesic orbit using RK4.
     *
     * Integrates (du/dphi)^2 = F(u, a, b) from initial conditions.
     * Stops when r < horizon (captured) or r > maxR (escaped).
     *
     * @param b Impact parameter L/E (meters).
     * @param rStart Starting radius (meters).
     * @param phiStart Starting azimuthal angle (radians).
     * @param phiMax Maximum azimuthal sweep (radians).
     * @param dPhi Angular step size (radians).
     * @return Vector of (phi, r) pairs along the orbit.
     */
    std::vector<std::pair<double, double>> integrateOrbit(
        double b, double rStart, double phiStart, double phiMax, double dPhi) const noexcept;

    /**
     * @brief Trace a null geodesic from r=30M to escape, computing deflection.
     *
     * Used for end-to-end ODE validation: at a=0, b=12M the deflection should
     * match the Schwarzschild value (~0.45 rad), and at a=0, b=6M it should
     * match (~1.72 rad). This catches coefficient errors in the reduction
     * that fixed-point checks (F(u)=0, F'(u)=0) cannot catch.
     *
     * @param b Impact parameter L/E (meters).
     * @return Deflection angle in radians, or -1.0 if captured by horizon.
     */
    double traceRay(double b) const noexcept;

    // Accessors
    double mass() const noexcept { return M_; }
    double spin() const noexcept { return aDimless_; }
    double specificAngularMomentum() const noexcept { return a_; }
    double horizonRadius() const noexcept { return M_ + std::sqrt(M_ * M_ - a_ * a_); }

private:
    double M_;         ///< Geometric mass M = GM/c^2 (meters)
    double aDimless_;  ///< Dimensionless spin a* = a/M
    double a_;         ///< Specific angular momentum a = a*M (meters)

    /**
     * @brief Evaluate (du/dphi)^2 = F(u, a, b) for this instance's M and a.
     */
    double orbitEquationInstance(double u, double b) const noexcept {
        return orbitEquation(u, M_, a_, b);
    }

    /**
     * @brief Evaluate d/du[(du/dphi)^2] for this instance's M and a.
     */
    double orbitEquationDerivativeInstance(double u, double b) const noexcept {
        return orbitEquationDerivative(u, M_, a_, b);
    }
};

} // namespace quantumverse

#endif // QUANTUMVERSE_KERR_GEODESIC_H
