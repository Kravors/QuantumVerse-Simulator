// DifferentiableSimulator.h
// End-to-end differentiable physics simulation wrapper
//
// Provides parameter-to-observable gradient computation for
// theory parameter optimization and neural surrogate training.
//
// Pipeline: parameters → MetricTensor factory → GeodesicIntegrator → observables
//
// Author: QuantumVerse Phase 3 Implementation
// Date: 2026-05-13

#pragma once

#include <vector>
#include <array>
#include <memory>
#include <string>
#include <stdexcept>
#include <cmath>
#include <functional>

#include "../spacetime/Event4D.h"
#include "../spacetime/MetricTensor.h"
#include "../physics/GeodesicIntegrator.h"

namespace quantumverse {

/**
 * @brief Observable extraction modes for simulation output.
 */
enum class ObservableMode {
    FINAL_POSITION,   ///< Extract final (t, x, y, z) coordinates
    FINAL_VELOCITY,   ///< Extract final 4-velocity components
    FINAL_STATE,      ///< Extract both position and velocity (8 components)
    PROPER_TIME,      ///< Extract total proper time of trajectory
    CUSTOM            ///< User-defined observable via callback
};

/**
 * @brief Configuration for the DifferentiableSimulator.
 */
struct SimulatorConfig {
    double tolerance = 1e-8;           ///< Geodesic integrator tolerance
    double minStepSize = 1e-12;        ///< Minimum integration step
    double maxStepSize = 0.1;          ///< Maximum integration step
    double safetyFactor = 0.9;         ///< Step size safety factor
    int maxIterations = 1000000;       ///< Maximum integration iterations
    double paramEpsilon = 1e-6;        ///< FD epsilon for Jacobian computation
    ObservableMode observableMode = ObservableMode::FINAL_STATE;
    std::function<std::vector<double>(
        const std::vector<GeodesicStep>&)> customObservable;  ///< Custom observable extractor
};

/**
 * @brief Factory function type: creates a MetricTensor from parameter vector.
 *
 * Used to construct position-dependent metrics (Schwarzschild, Kerr, RN)
 * with specific parameter values for simulation and perturbation.
 */
using MetricFactory = std::function<
    std::shared_ptr<MetricTensor>(const std::vector<double>& params)
>;

/**
 * @brief DifferentiableSimulator
 *
 * Wraps MetricTensor factory and GeodesicIntegrator to provide
 * end-to-end differentiable simulation from theory parameters
 * to observable quantities.
 *
 * Supports central finite differences for Jacobian computation.
 *
 * @note Thread-safe for concurrent simulations with different parameter vectors.
 * @note Uses RAII for metric and integrator lifetime management.
 */
class DifferentiableSimulator {
public:
    /**
     * @brief Construct a DifferentiableSimulator with a metric factory.
     * @param factory Function that creates MetricTensor from parameter vector.
     * @param paramNames Names of parameters (for debugging/logging).
     * @param config Simulator configuration parameters.
     *
     * @throws std::invalid_argument if factory is nullptr.
     */
    explicit DifferentiableSimulator(
        MetricFactory factory,
        std::vector<std::string> paramNames = {},
        const SimulatorConfig& config = SimulatorConfig()
    );

    /**
     * @brief Destructor.
     */
    ~DifferentiableSimulator() = default;

    // Non-copyable (movable only)
    DifferentiableSimulator(const DifferentiableSimulator&) = delete;
    DifferentiableSimulator& operator=(const DifferentiableSimulator&) = delete;
    DifferentiableSimulator(DifferentiableSimulator&&) noexcept = default;
    DifferentiableSimulator& operator=(DifferentiableSimulator&&) noexcept = default;

    /**
     * @brief Run a differentiable simulation from given initial conditions.
     *
     * @param initialEvent Starting spacetime event.
     * @param initialVelocity Initial four-velocity vector [u^t, u^x, u^y, u^z].
     * @param targetProperTime Target proper time for integration.
     * @param geodesicType Type of geodesic (timelike/lightlike/spacelike).
     * @return Event4D Final event after integration.
     */
    Event4D simulate(
        const Event4D& initialEvent,
        const std::array<double, 4>& initialVelocity,
        GeodesicType geodesicType,
        double targetProperTime
    ) const;

    /**
     * @brief Run simulation and return full trajectory.
     */
    std::vector<GeodesicStep> simulateFull(
        const Event4D& initialEvent,
        const std::array<double, 4>& initialVelocity,
        GeodesicType geodesicType,
        double targetProperTime,
        bool storeTrajectory = false
    ) const;

    /**
      * @brief Compute the Jacobian ∂observables/∂parameters via central finite differences.
      *
      * @note Uses central differences: ∂f/∂p_i ≈ (f(p+ε) - f(p-ε)) / (2ε)
      */
    std::vector<std::vector<double>> computeJacobian(
        const Event4D& initialEvent,
        const std::array<double, 4>& initialVelocity,
        GeodesicType geodesicType,
        double targetProperTime
    ) const;

    /**
      * @brief Compute the Jacobian ∂observables/∂parameters via reverse-mode AD.
      *
      * Uses the adjoint method: one forward pass records operations on the tape,
      * one reverse pass propagates gradients back to all parameters simultaneously.
      * Cost is O(1) in parameter count, vs O(N) for central finite differences.
      *
      * @note For geodesic observables, this currently uses a simplified differentiable
      * model. Full adjoint-mode through the RK4 integrator requires extending
      * GeodesicIntegrator with ADVar support.
      */
    std::vector<std::vector<double>> computeJacobianAdjoint(
        const Event4D& initialEvent,
        const std::array<double, 4>& initialVelocity,
        GeodesicType geodesicType,
        double targetProperTime
    ) const;

    /**
     * @brief Compute observables and their parameter gradients in one call.
     */
    std::pair<std::vector<double>, std::vector<std::vector<double>>>
    simulateWithJacobian(
        const Event4D& initialEvent,
        const std::array<double, 4>& initialVelocity,
        GeodesicType geodesicType,
        double targetProperTime
    ) const;

    /**
     * @brief Set the theory parameters for the underlying metric.
     */
    void setParameters(const std::vector<double>& params);

    /**
     * @brief Get current theory parameters.
     */
    std::vector<double> getParameters() const;

    /**
     * @brief Get the number of parameters the metric depends on.
     */
    size_t getNumParameters() const noexcept;

    /**
     * @brief Get parameter names for logging/debugging.
     */
    std::vector<std::string> getParameterNames() const;

    /**
     * @brief Verify gradient accuracy via finite-difference check.
     *
     * @return double Maximum relative error across all observable-parameter pairs.
     */
    double verifyGradient(
        const Event4D& initialEvent,
        const std::array<double, 4>& initialVelocity,
        GeodesicType geodesicType,
        double targetProperTime,
        double epsilon = 1e-5
    ) const;

    /**
     * @brief Get the underlying metric name.
     */
    std::string getMetricName() const;

    /**
     * @brief Check if the simulator is properly initialized.
     */
    bool isValid() const noexcept;

private:
    /**
     * @brief Extract observable vector from a geodesic trajectory.
     */
    std::vector<double> extractObservables(
        const std::vector<GeodesicStep>& trajectory
    ) const;

    /**
     * @brief Rebuild the base metric from current parameters.
     */
    void rebuildMetric_() const;

    // Core components
    MetricFactory metricFactory_;                              ///< Factory: params → MetricTensor
    std::vector<std::string> paramNames_;                      ///< Parameter names
    SimulatorConfig config_;                                   ///< Configuration

    // Mutable state — modified in logically-const methods (Jacobian, verify)
    mutable std::vector<double> currentParams_;                ///< Current parameter values
    mutable std::shared_ptr<MetricTensor> baseMetric_;         ///< Current metric (cached)
    mutable std::vector<GeodesicStep> lastTrajectory_;         ///< Most recent integration
    mutable bool trajectoryCached_ = false;
};

/**
 * @brief Factory function to create a DifferentiableSimulator for common metric types.
 *
 * @param metricType Metric type identifier ("schwarzschild", "kerr", "reissner-nordstrom").
 * @param params Metric parameters.
 * @param config Optional simulator configuration.
 * @return std::unique_ptr<DifferentiableSimulator> Configured simulator instance.
 *
 * @throws std::invalid_argument if metricType is unknown.
 */
std::unique_ptr<DifferentiableSimulator> createDifferentiableSimulator(
    const std::string& metricType,
    const std::vector<double>& params = {},
    const SimulatorConfig& config = SimulatorConfig()
);

} // namespace quantumverse