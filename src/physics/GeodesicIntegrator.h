#ifndef QUANTUMVERSE_GEODESIC_INTEGRATOR_H
#define QUANTUMVERSE_GEODESIC_INTEGRATOR_H

/**
 * @file GeodesicIntegrator.h
 * @brief QuantumVerse Simulator - Geodesic Integrator
 *
 * Adaptive step-size Runge-Kutta integration for geodesic equations
 * in curved spacetime (4D Lorentzian manifold).
 *
 * Based on: Spacetime.md - Geodesics, Curved_spacetime.md
 * Method: 4th-order Runge-Kutta with adaptive step size control
 *
 * Christoffel symbols are computed via numerical differentiation of the
 * metric field g_μν(x). For analytical Schwarzschild/Kerr metrics, the
 * finite-difference approach converges to machine precision with h ~ 1e-8.
 *
 * Geodesic equation:
 *   d²x^λ/dτ² + Γ^λ_μν (dx^μ/dτ)(dx^ν/dτ) = 0
 *
 * @note All coordinates use Lorentz signature (-,+,+,+).
 */

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <vector>
#include <cmath>
#include <memory>
#include <functional>
#include <array>
#include <algorithm>
#include "../spacetime/Event4D.h"
#include "../spacetime/MetricTensor.h"

namespace quantumverse {

// Geodesic integration result
struct GeodesicStep {
    Event4D event;      ///< Spacetime position
    double properTime;  ///< Proper time along geodesic
    double stepSize;    ///< Integration step size used
    bool valid;         ///< Integration success flag
};

// Geodesic type enumeration
enum class GeodesicType {
    TIMELIKE,   ///< Massive particle trajectory
    LIGHTLIKE,  ///< Light ray (photon)
    SPACELIKE   ///< Not physically realizable (for analysis)
};

class GeodesicIntegrator {
private:
    // Integration parameters
    double tolerance;           ///< Error tolerance for adaptive step
    double minStepSize;         ///< Minimum allowed step size
    double maxStepSize;         ///< Maximum allowed step size
    double safetyFactor;        ///< Safety factor for step adjustment
    int maxIterations;          ///< Maximum integration steps

    // Current metric (can be updated for dynamic spacetime)
    std::shared_ptr<MetricTensor> currentMetric;

    /**
     * @brief Metric field function: returns metric tensor at any spacetime point.
     *
     * By default, returns the stored currentMetric (constant spacetime).
     * For position-dependent metrics (Schwarzschild, Kerr, etc.), set this
     * to a lambda that constructs the appropriate metric at each position.
     *
     * Example for Schwarzschild:
     *   integrator.setMetricField([](const Event4D& evt) {
     *       double r = sqrt(evt.x*evt.x + evt.y*evt.y + evt.z*evt.z);
     *       return MetricTensor::schwarzschild(mass, r, M_PI/2, 0.0);
     *   });
     */
    std::function<MetricTensor(const Event4D&)> metricField;

    // Christoffel symbols cache
    mutable std::array<std::array<std::array<double, 4>, 4>, 4> christoffelCache;
    mutable bool cacheValid;
    mutable Event4D lastEvalPosition;  ///< Position at which cache was computed

    // Finite-difference step for numerical derivatives
    static constexpr double FD_H = 1e-6;

public:
    /**
     * @brief Constructor with default parameters.
     * @param tol Error tolerance for adaptive step
     * @param minStep Minimum allowed step size
     * @param maxStep Maximum allowed step size
     * @param safety Safety factor for step adjustment
     * @param maxIter Maximum integration steps
     */
    GeodesicIntegrator(
        double tol = 1e-8,
        double minStep = 1e-10,
        double maxStep = 1.0,
        double safety = 0.9,
        int maxIter = 1000000
    ) : tolerance(tol), minStepSize(minStep), maxStepSize(maxStep),
        safetyFactor(safety), maxIterations(maxIter),
        cacheValid(false) {
        currentMetric = std::make_shared<MetricTensor>();
        // Default metric field: returns constant metric everywhere
        metricField = [this](const Event4D&) -> MetricTensor {
            return *currentMetric;
        };
    }

    /**
     * @brief Set metric for geodesic integration.
     * @param metric Shared pointer to the metric tensor.
     */
    void setMetric(std::shared_ptr<MetricTensor> metric) {
        currentMetric = metric;
        cacheValid = false;
    }

    /**
     * @brief Get current metric.
     * @return Shared pointer to the current metric tensor.
     */
    std::shared_ptr<MetricTensor> getMetric() const {
        return currentMetric;
    }

    /**
     * @brief Set a custom metric field function for position-dependent metrics.
     * @param field Function returning MetricTensor at any spacetime event.
     *
     * This enables numerical computation of Christoffel symbols for arbitrary
     * spacetime geometries by providing the metric as a function of position.
     */
    void setMetricField(std::function<MetricTensor(const Event4D&)> field) {
        metricField = field;
        cacheValid = false;
    }

    /**
     * @brief Compute Christoffel symbols Γ^λ_μν at a given position.
     *
     * Uses central finite differences on the metric field:
     *   Γ^λ_μν = ½ g^λσ (∂_μ g_σν + ∂_ν g_σμ - ∂_σ g_μν)
     *
     * Each partial derivative ∂_μ g_σν is computed as:
     *   ∂_μ g_σν ≈ [g_σν(x + h·e_μ) - g_σν(x - h·e_μ)] / (2h)
     *
     * @param position Spacetime event at which to compute Christoffel symbols.
     */
    void computeChristoffelSymbols(const Event4D& position) const {
        if (!currentMetric) return;

        const double h = FD_H;

        // Get metric at the central position and at ±h along each coordinate
        MetricTensor g0 = metricField(position);
        MetricTensor inv = g0.inverse();

        // Reset cache
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                for (int k = 0; k < 4; k++)
                    christoffelCache[i][j][k] = 0.0;

        // For each derivative direction μ
        for (int mu = 0; mu < 4; mu++) {
            // Create perturbed positions: x + h·e_μ and x - h·e_μ
            Event4D pos_plus = position;
            Event4D pos_minus = position;

            // Perturb the appropriate coordinate
            switch (mu) {
                case 0: pos_plus.t += h; pos_minus.t -= h; break;
                case 1: pos_plus.x += h; pos_minus.x -= h; break;
                case 2: pos_plus.y += h; pos_minus.y -= h; break;
                case 3: pos_plus.z += h; pos_minus.z -= h; break;
            }

            MetricTensor g_plus = metricField(pos_plus);
            MetricTensor g_minus = metricField(pos_minus);

            // ∂_μ g_σν for all σ, ν
            double dg[4][4];
            for (int sigma = 0; sigma < 4; sigma++) {
                for (int nu = 0; nu < 4; nu++) {
                    dg[sigma][nu] = (g_plus.g[sigma][nu] - g_minus.g[sigma][nu]) / (2.0 * h);
                }
            }

            // Γ^λ_μν = ½ g^λσ (∂_μ g_σν + ∂_ν g_σμ - ∂_σ g_μν)
            for (int lambda = 0; lambda < 4; lambda++) {
                for (int nu = 0; nu < 4; nu++) {
                    double sum = 0.0;
                    for (int sigma = 0; sigma < 4; sigma++) {
                        // Note: g^λσ is the inverse metric (raised index)
                        double dg_mu_sigma_nu = dg[sigma][nu];           // ∂_μ g_σν
                        double dg_nu_sigma_mu = dg[sigma][mu];           // ∂_ν g_σμ (swap μ↔ν in dg)
                        // For ∂_σ g_μν, we need derivative in σ direction
                        // We already have dg for the μ direction; we need it for σ direction
                        // Recompute: ∂_σ g_μν
                        double dg_sigma_mu_nu;
                        if (sigma == mu) {
                            dg_sigma_mu_nu = dg[mu][nu];  // Same direction we already computed
                        } else {
                            // Need derivative in σ direction for g_μν
                            Event4D pos_p = position;
                            Event4D pos_m = position;
                            switch (sigma) {
                                case 0: pos_p.t += h; pos_m.t -= h; break;
                                case 1: pos_p.x += h; pos_m.x -= h; break;
                                case 2: pos_p.y += h; pos_m.y -= h; break;
                                case 3: pos_p.z += h; pos_m.z -= h; break;
                            }
                            MetricTensor gp = metricField(pos_p);
                            MetricTensor gm = metricField(pos_m);
                            dg_sigma_mu_nu = (gp.g[mu][nu] - gm.g[mu][nu]) / (2.0 * h);
                        }
                        sum += inv.g[lambda][sigma] *
                               (dg_mu_sigma_nu + dg_nu_sigma_mu - dg_sigma_mu_nu);
                    }
                    christoffelCache[lambda][mu][nu] = 0.5 * sum;
                }
            }
        }

        cacheValid = true;
        lastEvalPosition = position;
    }

    /**
     * @brief Get cached Christoffel symbol Γ^λ_μν.
     * @param lambda Upper index (0-3).
     * @param mu Lower index 1 (0-3).
     * @param nu Lower index 2 (0-3).
     * @return Value of the Christoffel symbol.
     */
    double getChristoffel(int lambda, int mu, int nu) const {
        if (!cacheValid) return 0.0;
        return christoffelCache[lambda][mu][nu];
    }

    /**
     * @brief Get the full cached Christoffel symbol array.
     * @return Const reference to the 4x4x4 Christoffel symbol array.
     */
    const std::array<std::array<std::array<double, 4>, 4>, 4>& getChristoffelSymbols() const {
        return christoffelCache;
    }

    // Geodesic equation: d²x^λ/dτ² + Γ^λ_μν (dx^μ/dτ)(dx^ν/dτ) = 0
    // Convert to first-order system: dy/dτ = f(τ, y)
    // where y = (x^0, x^1, x^2, x^3, u^0, u^1, u^2, u^3)
    // and u^μ = dx^μ/dτ (four-velocity)

    /**
     * @brief Compute the derivative of the geodesic state vector.
     * @param tau Current proper time parameter.
     * @param y State vector [x^0..x^3, u^0..u^3].
     * @return Derivative dy/dτ.
     */
    std::vector<double> geodesicDerivative(double tau, const std::vector<double>& y) const {
        std::vector<double> dydtau(8, 0.0);

        // Current position from state vector
        Event4D position(y[0], y[1], y[2], y[3]);

        // Position components (first 4 elements)
        for (int i = 0; i < 4; i++) {
            dydtau[i] = y[i + 4];  // dx^μ/dτ = u^μ
        }

        // Velocity components (last 4 elements) - geodesic equation
        // Recompute Christoffel symbols at current position
        computeChristoffelSymbols(position);

        for (int lambda = 0; lambda < 4; lambda++) {
            double acceleration = 0.0;

            for (int mu = 0; mu < 4; mu++) {
                for (int nu = 0; nu < 4; nu++) {
                    acceleration -= christoffelCache[lambda][mu][nu]
                                   * y[mu + 4] * y[nu + 4];
                }
            }

            dydtau[lambda + 4] = acceleration;
        }

        return dydtau;
    }

    /**
     * @brief 4th-order Runge-Kutta integration step.
     * @param tau Current proper time.
     * @param y Current state vector.
     * @param h Step size.
     * @return New state vector after step.
     */
    std::vector<double> rungeKutta4Step(
        double tau,
        const std::vector<double>& y,
        double h
    ) const {
        std::vector<double> k1 = geodesicDerivative(tau, y);

        std::vector<double> y2(8);
        for (int i = 0; i < 8; i++) {
            y2[i] = y[i] + 0.5 * h * k1[i];
        }
        std::vector<double> k2 = geodesicDerivative(tau + 0.5 * h, y2);

        std::vector<double> y3(8);
        for (int i = 0; i < 8; i++) {
            y3[i] = y[i] + 0.5 * h * k2[i];
        }
        std::vector<double> k3 = geodesicDerivative(tau + 0.5 * h, y3);

        std::vector<double> y4(8);
        for (int i = 0; i < 8; i++) {
            y4[i] = y[i] + h * k3[i];
        }
        std::vector<double> k4 = geodesicDerivative(tau + h, y4);

        std::vector<double> result(8);
        for (int i = 0; i < 8; i++) {
            result[i] = y[i] + (h / 6.0) *
                (k1[i] + 2.0 * k2[i] + 2.0 * k3[i] + k4[i]);
        }

        return result;
    }

    /**
     * @brief Adaptive step-size Runge-Kutta (RK45 via Richardson extrapolation).
     * @param tau Current proper time.
     * @param y Current state vector.
     * @param h Current step size.
     * @return Pair of (new state, suggested new step size).
     */
    std::pair<std::vector<double>, double> adaptiveStep(
        double tau,
        const std::vector<double>& y,
        double h
    ) const {
        // Take full step
        std::vector<double> y_full = rungeKutta4Step(tau, y, h);

        // Take two half steps
        std::vector<double> y_half = rungeKutta4Step(tau, y, h / 2.0);
        std::vector<double> y_twoHalf = rungeKutta4Step(tau + h / 2.0, y_half, h / 2.0);

        // Estimate error via Richardson extrapolation
        double error = 0.0;
        for (int i = 0; i < 8; i++) {
            double diff = y_twoHalf[i] - y_full[i];
            error += diff * diff;
        }
        error = std::sqrt(error) / 15.0;

        // Adjust step size
        double newStep = h;
        if (error > tolerance && h > minStepSize) {
            newStep = safetyFactor * h * std::pow(tolerance / error, 0.2);
            newStep = std::max(newStep, minStepSize);
        } else if (error < tolerance / 10.0 && h < maxStepSize) {
            newStep = safetyFactor * h * std::pow(tolerance / error, 0.25);
            newStep = std::min(newStep, maxStepSize);
        }

        // Use Richardson extrapolation for more accurate estimate
        if (error < tolerance) {
            for (int i = 0; i < 8; i++) {
                y_twoHalf[i] = y_twoHalf[i] + (y_twoHalf[i] - y_full[i]) / 15.0;
            }
            return {y_twoHalf, newStep};
        } else {
            return {y, newStep};  // Reject step, suggest smaller step
        }
    }

    /**
     * @brief Integrate geodesic from initial conditions.
     * @param startEvent Starting spacetime event.
     * @param initialVelocity Initial four-velocity (dx^μ/dτ).
     * @param type Geodesic type (timelike/lightlike/spacelike).
     * @param targetProperTime Target proper time for integration.
     * @param adaptive Use adaptive step size if true.
     * @return Vector of GeodesicStep records.
     */
    std::vector<GeodesicStep> integrate(
        const Event4D& startEvent,
        const std::array<double, 4>& initialVelocity,
        GeodesicType type,
        double targetProperTime,
        bool adaptive = true
    ) {
        std::vector<GeodesicStep> trajectory;

        // Initial conditions
        std::vector<double> y(8);
        y[0] = startEvent.t;
        y[1] = startEvent.x;
        y[2] = startEvent.y;
        y[3] = startEvent.z;
        y[4] = initialVelocity[0];
        y[5] = initialVelocity[1];
        y[6] = initialVelocity[2];
        y[7] = initialVelocity[3];

        // Normalize four-velocity for timelike geodesics
        if (type == GeodesicType::TIMELIKE) {
            MetricTensor metricAtStart = metricField(startEvent);
            double norm = 0.0;
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    norm += metricAtStart.g[i][j] * initialVelocity[i] * initialVelocity[j];
                }
            }
            if (norm < 0) {
                double scale = 1.0 / std::sqrt(-norm);
                for (int i = 4; i < 8; i++) {
                    y[i] *= scale;
                }
            }
        }

        double tau = 0.0;
        double stepSize = 0.01;  // Initial step size

        // Store initial point
        GeodesicStep initialStep;
        initialStep.event = startEvent;
        initialStep.properTime = 0.0;
        initialStep.stepSize = 0.0;
        initialStep.valid = true;
        trajectory.push_back(initialStep);

        int iterations = 0;

        while (tau < targetProperTime && iterations < maxIterations) {
            std::vector<double> y_new;
            double newStep;

            if (adaptive) {
                auto result = adaptiveStep(tau, y, stepSize);
                y_new = result.first;
                newStep = result.second;
            } else {
                y_new = rungeKutta4Step(tau, y, stepSize);
                newStep = stepSize;
            }

            // Check if step was rejected
            if (y_new == y && adaptive) {
                stepSize = newStep;
                if (stepSize < minStepSize) break;
                continue;
            }

            // Update state
            y = y_new;
            tau += stepSize;
            stepSize = newStep;
            iterations++;

            // Create event from position
            Event4D event(y[0], y[1], y[2], y[3]);

            GeodesicStep step;
            step.event = event;
            step.properTime = tau;
            step.stepSize = stepSize;
            step.valid = true;

            trajectory.push_back(step);

            // Check for singularities (large curvature)
            if (isNearSingularity(event)) {
                break;
            }
        }

        return trajectory;
    }

    /**
     * @brief Simplified geodesic integration (non-adaptive).
     * @param start Starting event.
     * @param velocity Initial four-velocity.
     * @param steps Number of integration steps.
     * @param deltaTau Proper time per step.
     * @return Vector of events along the geodesic.
     */
    std::vector<Event4D> integrateSimple(
        const Event4D& start,
        const std::array<double, 4>& velocity,
        int steps,
        double deltaTau
    ) {
        std::vector<Event4D> path;
        path.push_back(start);

        std::vector<double> y(8);
        y[0] = start.t; y[1] = start.x; y[2] = start.y; y[3] = start.z;
        y[4] = velocity[0]; y[5] = velocity[1]; y[6] = velocity[2]; y[7] = velocity[3];

        for (int i = 0; i < steps; i++) {
            y = rungeKutta4Step(i * deltaTau, y, deltaTau);
            Event4D event(y[0], y[1], y[2], y[3]);
            path.push_back(event);

            if (isNearSingularity(event)) break;
        }

        return path;
    }

private:
    /**
     * @brief Check if event is near a gravitational singularity.
     * Uses coordinate distance from origin as a proxy; override for specific spacetimes.
     */
    bool isNearSingularity(const Event4D& event) const {
        double r = std::sqrt(event.x * event.x + event.y * event.y + event.z * event.z);
        double rs = 1e-6;  // Example Schwarzschild radius scale
        return r < 10.0 * rs;
    }
};

} // namespace quantumverse

#endif // QUANTUMVERSE_GEODESIC_INTEGRATOR_H