#pragma once

#include <cmath>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace quantumverse {

/**
 * @class DilatonMetric
 * @brief Represents the 2D dilaton gravity metric in conformal gauge, based on the CGHS model.
 *
 * The metric is given by:
 *   ds² = -e^{2ρ} dx⁺ dx⁻,
 * where the conformal factor e^{2ρ} = 1 + M e^{2σ} x⁺ x⁻.
 * The dilaton field is:
 *   φ = -σ + ½ ln(1 + M e^{2σ} x⁺ x⁻)   (for M > 0)
 *   φ = -σ                              (for M = 0, linear dilaton vacuum)
 *
 * This describes a static black hole of mass M (M=0 gives flat vacuum).
 * The apparent horizon is located at x⁻ = 0 (future horizon) and x⁺ = 0 (past horizon).
 * The curvature singularity occurs where 1 + M e^{2σ} x⁺ x⁻ = 0.
 *
 * All quantities are in geometric units (c = G = 1). The parameter σ is related to the
 * dilaton potential λ by λ = e^{-σ}. In the limit λ=1, σ=0.
 *
 * @note This implementation follows the conventions of Giddings' Trieste lectures on
 * 2D dilaton gravity and black hole information paradox.
 */
class DilatonMetric {
public:
    /**
     * @brief Construct a dilaton metric with given sigma and mass.
     * @param sigma The dilaton vacuum parameter σ (λ = e^{-σ}).
     * @param mass The black hole mass M (M ≥ 0). Use 0 for linear dilaton vacuum.
     */
    DilatonMetric(double sigma = 0.0, double mass = 0.0) noexcept;

    /** @brief Destructor (default). */
    ~DilatonMetric() = default;

    // Delete copy semantics (RAII)
    DilatonMetric(const DilatonMetric&) = delete;
    DilatonMetric& operator=(const DilatonMetric&) = delete;

    // Allow move semantics
    DilatonMetric(DilatonMetric&&) noexcept = default;
    DilatonMetric& operator=(DilatonMetric&&) noexcept = default;

    /**
     * @brief Compute the conformal factor e^{2ρ}.
     * @param x_plus Advanced time coordinate x⁺.
     * @param x_minus Retarded time coordinate x⁻.
     * @return e^{2ρ} = 1 + M e^{2σ} x⁺ x⁻.
     */
    double conformalFactor(double x_plus, double x_minus) const noexcept;

    /**
     * @brief Compute the dilaton field φ.
     * @param x_plus Advanced time coordinate x⁺.
     * @param x_minus Retarded time coordinate x⁻.
     * @return φ = -σ + ½ ln(1 + M e^{2σ} x⁺ x⁻) for M>0; -σ for M=0.
     */
    double dilaton(double x_plus, double x_minus) const noexcept;

    /**
     * @brief Compute a metric component g_{μν}.
     * @param mu Row index (0 for x⁺, 1 for x⁻).
     * @param nu Column index (0 for x⁺, 1 for x⁻).
     * @param x_plus Advanced time coordinate.
     * @param x_minus Retarded time coordinate.
     * @return g_{μν}. The metric is symmetric with g_{00}=g_{11}=0 and g_{01}=g_{10}= -e^{2ρ}/2.
     */
    double metricComponent(int mu, int nu, double x_plus, double x_minus) const noexcept;

    /**
     * @brief Compute the Ricci scalar curvature R.
     * @param x_plus Advanced time coordinate.
     * @param x_minus Retarded time coordinate.
     * @return R = 4 M e^{2σ} / (1 + M e^{2σ} x⁺ x⁻)³.
     *
     * For the linear dilaton vacuum (M=0), R = 0.
     */
    double ricciScalar(double x_plus, double x_minus) const noexcept;

    /**
     * @brief Check if a point lies on the apparent horizon.
     * The apparent horizon is defined by ∂₊φ = 0 (future horizon) or ∂₋φ = 0 (past horizon).
     * For this metric, ∂₊φ = ½ (M e^{2σ} x⁻) / (1 + M e^{2σ} x⁺ x⁻). So the horizon occurs at x⁻ = 0.
     * @param x_plus Advanced time coordinate.
     * @param x_minus Retarded time coordinate.
     * @param tol Tolerance for floating point comparison.
     * @return true if |x⁻| < tol (future horizon) or |x⁺| < tol (past horizon).
     */
    bool isApparentHorizon(double x_plus, double x_minus, double tol = 1e-10) const noexcept;

    /**
     * @brief Check if a point is at the curvature singularity.
     * The singularity occurs where the conformal factor vanishes: 1 + M e^{2σ} x⁺ x⁻ = 0.
     * @param x_plus Advanced time coordinate.
     * @param x_minus Retarded time coordinate.
     * @param tol Tolerance for detecting near-singularity.
     * @return true if |1 + M e^{2σ} x⁺ x⁻| < tol.
     */
    bool isSingularity(double x_plus, double x_minus, double tol = 1e-10) const noexcept;

    /**
     * @brief Get the dilaton vacuum parameter σ.
     * @return σ.
     */
    double getSigma() const noexcept { return sigma; }

    /**
     * @brief Get the black hole mass M.
     * @return M (≥ 0).
     */
    double getMass() const noexcept { return mass; }

    /**
     * @brief Check if this is the linear dilaton vacuum (M=0).
     * @return true if mass is zero.
     */
    bool isVacuum() const noexcept { return mass == 0.0; }

    /**
     * @brief Compute the Hawking temperature T = κ/(2π).
     * For this black hole, the surface gravity κ = 1/(2M) (in units where c=1).
     * @return T = 1/(4π M). For M=0, returns 0.
     */
    double hawkingTemperature() const noexcept;

private:
    double sigma; ///< Dilaton vacuum parameter σ (λ = e^{-σ})
    double mass;  ///< Black hole mass M (M=0 for vacuum)
};

} // namespace quantumverse
