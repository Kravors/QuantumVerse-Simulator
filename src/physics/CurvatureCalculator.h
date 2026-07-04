#ifndef QUANTUMVERSE_CURVATURE_CALCULATOR_H
#define QUANTUMVERSE_CURVATURE_CALCULATOR_H

/**
 * @file CurvatureCalculator.h
 * @brief QuantumVerse Simulator - Curvature Tensor Calculator
 *
 * Computes the full Riemann curvature tensor, Ricci tensor, Ricci scalar,
 * and Kretschmann scalar from a metric field using numerical differentiation
 * of Christoffel symbols.
 *
 * Pipeline: Metric → Christoffel → Riemann → Ricci → Kretschmann
 *
 * Based on: Spacetime.md - Curvature, Curved_spacetime.md
 * Signature: (-, +, +, +) East Coast convention
 */

#include "../spacetime/Event4D.h"
#include "../spacetime/MetricTensor.h"
#include "../physics/GeodesicIntegrator.h"
#include <array>
#include <cmath>
#include <memory>
#include <functional>
#include <stdexcept>

namespace quantumverse {

/**
 * @brief Results container for curvature computations at a single point.
 */
struct CurvatureResult {
    // Riemann tensor R^ρ_σμν (4 indices, first is upper)
    std::array<std::array<std::array<std::array<double, 4>, 4>, 4>, 4> riemann;

    // Ricci tensor R_μν = R^λ_μλν
    std::array<std::array<double, 4>, 4> ricci;

    // Ricci scalar R = g^μν R_μν
    double ricciScalar = 0.0;

    // Kretschmann scalar K = R^ρσμν R_ρσμν
    double kretschmann = 0.0;

    // Einstein tensor G_μν = R_μν - ½ g_μν R
    std::array<std::array<double, 4>, 4> einstein;

    // Weyl tensor squared (traceless part of Riemann)
    double weylSquared = 0.0;

    // Flag indicating if computation was at or near a singularity
    bool nearSingularity = false;

    // Maximum absolute value of Riemann components (for anomaly detection)
    double maxRiemannComponent = 0.0;
};

/**
 * @brief Computes curvature invariants and tensors from a metric field.
 *
 * Uses the GeodesicIntegrator's Christoffel symbol computation as a foundation,
 * then builds up to the full Riemann tensor via numerical differentiation.
 *
 * For analytical metrics (Schwarzschild, Kerr), finite differences with
 * h ~ 1e-6 converge to machine precision for curvature invariants.
 */
class CurvatureCalculator {
public:
    /**
     * @brief Construct a curvature calculator for the given metric.
     * @param metric Shared pointer to the metric tensor (can be position-dependent via setMetricField).
     */
    explicit CurvatureCalculator(std::shared_ptr<MetricTensor> metric = nullptr);

    /**
     * @brief Set the metric tensor for curvature computation.
     */
    void setMetric(std::shared_ptr<MetricTensor> metric);

    /**
     * @brief Set a custom metric field function for position-dependent metrics.
     *
     * Example for Schwarzschild:
     *   calculator.setMetricField([](const Event4D& evt) {
     *       double r = sqrt(evt.x*evt.x + evt.y*evt.y + evt.z*evt.z);
     *       return MetricTensor::schwarzschild(1.0, r, M_PI/2, 0.0);
     *   });
     */
    void setMetricField(std::function<MetricTensor(const Event4D&)> field);

    /**
     * @brief Compute Christoffel symbols Γ^λ_μν at a given position.
     *
     * Uses central finite differences on the metric field:
     *   Γ^λ_μν = ½ g^λσ (∂_μ g_σν + ∂_ν g_σμ - ∂_σ g_μν)
     *
     * @param position Spacetime event at which to compute Christoffel symbols.
     */
    void computeChristoffel(const Event4D& position) const;

    /**
     * @brief Compute the full Riemann tensor R^ρ_σμν at a given position.
     *
     * Uses numerical differentiation of Christoffel symbols:
     *   R^ρ_σμν = ∂_μ Γ^ρ_νσ - ∂_ν Γ^ρ_μσ + Γ^ρ_μλ Γ^λ_νσ - Γ^ρ_νλ Γ^λ_μσ
     *
     * @param position Spacetime event at which to compute the Riemann tensor.
     */
    void computeRiemann(const Event4D& position) const;

    /**
     * @brief Compute the Ricci tensor R_μν = R^λ_μλν at a given position.
     *
     * Contracts the Riemann tensor: R_μν = g^λρ R_ρμλν
     *
     * @param position Spacetime event.
     */
    void computeRicci(const Event4D& position) const;

    /**
     * @brief Compute the Ricci scalar R = g^μν R_μν at a given position.
     *
     * @param position Spacetime event.
     */
    void computeRicciScalar(const Event4D& position) const;

    /**
     * @brief Compute the Kretschmann scalar K = R^ρσμν R_ρσμν.
     *
     * This is a key curvature invariant — diverges at true singularities
     * but remains finite at coordinate singularities (e.g., Schwarzschild horizon).
     *
     * @param position Spacetime event.
     */
    void computeKretschmann(const Event4D& position) const;

    /**
     * @brief Compute the full curvature result at a given position.
     *
     * Convenience method that computes all curvature quantities in one call.
     *
     * @param position Spacetime event.
     * @return CurvatureResult containing all computed tensors and scalars.
     */
    CurvatureResult computeAll(const Event4D& position) const;

    /**
     * @brief Get the cached Christoffel symbols.
     */
    const std::array<std::array<std::array<double, 4>, 4>, 4>& getChristoffel() const;

    /**
     * @brief Get the cached Riemann tensor.
     */
    const std::array<std::array<std::array<std::array<double, 4>, 4>, 4>, 4>& getRiemann() const;

    /**
     * @brief Get the cached Ricci tensor.
     */
    const std::array<std::array<double, 4>, 4>& getRicci() const;

    /**
     * @brief Get the cached Ricci scalar.
     */
    double getRicciScalar() const;

    /**
     * @brief Get the cached Kretschmann scalar.
     */
    double getKretschmann() const;

    /**
     * @brief Set the finite-difference step size for numerical derivatives.
     * @param h Step size (default 1e-6).
     */
    void setFDStep(double h);

    /**
     * @brief Set the tolerance for singularity detection.
     * @param tol If Kretschmann > tol, the point is flagged as near-singular.
     */
    void setSingularityTolerance(double tol);

    /**
     * @brief Check if a position is near a curvature singularity.
     * @param position Spacetime event.
     * @return True if Kretschmann scalar exceeds the singularity tolerance.
     */
    bool isNearSingularity(const Event4D& position) const;

private:
    // Metric and field function
    std::shared_ptr<MetricTensor> currentMetric_;
    std::function<MetricTensor(const Event4D&)> metricField_;

    // Finite-difference step
    double fdStep_;

    // Singularity detection tolerance
    double singularityTolerance_;

    // Mutable caches (computed on demand)
    mutable std::array<std::array<std::array<double, 4>, 4>, 4> christoffelCache_;
    mutable std::array<std::array<std::array<std::array<double, 4>, 4>, 4>, 4> riemannCache_;
    mutable std::array<std::array<double, 4>, 4> ricciCache_;
    mutable double ricciScalarCache_;
    mutable double kretschmannCache_;
    mutable Event4D lastEvalPosition_;
    mutable bool christoffelValid_;
    mutable bool riemannValid_;
    mutable bool ricciValid_;

    /**
     * @brief Ensure Christoffel symbols are computed at the current position.
     */
    void ensureChristoffel(const Event4D& position) const;

    /**
     * @brief Ensure Riemann tensor is computed at the current position.
     */
    void ensureRiemann(const Event4D& position) const;

    /**
     * @brief Ensure Ricci tensor is computed at the current position.
     */
    void ensureRicci(const Event4D& position) const;

    /**
     * @brief Compute metric at a perturbed position.
     */
    MetricTensor metricAt(const Event4D& position, int mu, double delta) const;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_CURVATURE_CALCULATOR_H