#ifndef QUANTUMVERSE_KERR_METRIC_H
#define QUANTUMVERSE_KERR_METRIC_H

#include <array>
#include <cmath>
#include <memory>
#include "MetricTensor.h"
#include "Event4D.h"
#include "../math/AutoDiff.h"
#include "../physics/PhysicsConstants.h"

namespace quantumverse {

/**
 * @brief Kerr metric for rotating black holes in Boyer-Lindquist coordinates.
 *
 * Implements the Kerr geometry with frame-dragging effects. Supports
 * dimensionless spin parameter a* in [0, 1), where a* = 0 reduces to
 * Schwarzschild and a* → 1 approaches the extremal Kerr limit.
 *
 * Line element (geometric units, G = c = 1):
 * ds² = -(1 - 2Mr/Σ) dt² - (4Mar sin²θ/Σ) dt dφ
 *       + (Σ/Δ) dr² + Σ dθ² + (r² + a² + 2Ma²r sin²θ/Σ) sin²θ dφ²
 * where Σ = r² + a²cos²θ, Δ = r² - 2Mr + a², a = J/M.
 */
class KerrMetric : public MetricTensor {
public:
    /**
     * @brief Construct a Kerr metric.
     * @param massKg Black hole mass in kilograms.
     * @param spinDimensionless Dimensionless spin a* = Jc/(GM²), range [0, 1).
     */
    KerrMetric(double massKg, double spinDimensionless);

    double mass() const { return m_mass; }
    double spin() const { return m_spin; }
    double geometricMass() const { return m_M; }
    double specificAngularMomentum() const { return m_a; }
    double schwarzschildRadius() const { return m_rs; }

    /**
     * @brief Outer event horizon radius in meters.
     * r_+ = M + sqrt(M² - a²) in geometric units.
     */
    double outerHorizonRadius() const;

    /**
     * @brief Inner event horizon (Cauchy horizon) radius in meters.
     * r_- = M - sqrt(M² - a²) in geometric units.
     */
    double innerHorizonRadius() const;

    /**
     * @brief Ergosphere (static limit) radius at a given polar angle.
     * r_E(θ) = M + sqrt(M² - a²cos²θ) in geometric units.
     */
    double ergosphereRadius(double theta) const;

    /**
     * @brief Equatorial ergosphere radius (= rs, the Schwarzschild radius).
     */
    double ergosphereRadiusEquatorial() const { return m_rs; }

    /**
     * @brief Frame-dragging angular velocity ω(r, θ) in rad/s.
     * ω = -g_{tφ}/g_{φφ} at the given Boyer-Lindquist position.
     */
    double frameDraggingOmega(double r, double theta) const override;

    /**
     * @brief Evaluate metric at a Cartesian event.
     *
     * Converts (x,y,z) → Boyer-Lindquist (r,θ,φ), computes the Kerr metric
     * in BL coordinates, then transforms back to the (t,x,y,z) Cartesian basis.
     */
    std::array<std::array<double, 4>, 4> evaluate(const Event4D& event) const override;

    /**
     * @brief Curvature invariants (vacuum: Ricci = 0, Weyl = Riemann).
     */
    CurvatureScalars curvatureScalars(const Event4D& event) const override;

    std::array<std::array<std::array<double, 4>, 4>, 4> computeFullChristoffel(
        const Event4D& event
    ) const override;

    std::array<std::array<RVar, 4>, 4> evaluateAD(
        const std::array<RVar, 4>& pos,
        const std::vector<RVar>& params
    ) const override;

    std::array<std::array<std::array<RVar, 4>, 4>, 4> computeChristoffelAD(
        const std::array<RVar, 4>& pos,
        const std::vector<RVar>& params
    ) const override;

private:
    double m_mass;  ///< Mass in kg
    double m_M;     ///< Geometric mass M = GM/c²
    double m_a;     ///< Specific angular momentum a = spin * M
    double m_rs;    ///< Schwarzschild radius = 2M
    double m_spin;  ///< Dimensionless spin parameter a* = a/M

    /**
     * @brief Convert Cartesian (x,y,z) to Boyer-Lindquist radial coordinate.
     *
     * Solves r⁴ - (x²+y²+z²-a²)r² - a²z² = 0 for r ≥ 0.
     */
    double cartesianToBLRadial(double x, double y, double z) const;

    /**
     * @brief Transform BL metric components to Cartesian basis.
     *
     * Given BL metric g_BL(t,r,θ,φ) and Cartesian position, compute
     * the equivalent metric in (t,x,y,z) coordinates.
     */
    void transformBLToCartesian(
        double g_tt, double g_tphi, double g_rr, double g_thth, double g_pp,
        double x, double y, double z, double rBL, double theta, double phi,
        std::array<std::array<double, 4>, 4>& out
    ) const;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_KERR_METRIC_H
