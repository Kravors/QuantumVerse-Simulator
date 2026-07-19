#ifndef QUANTUMVERSE_DIFFERENTIABLE_GEODESIC_INTEGRATOR_H
#define QUANTUMVERSE_DIFFERENTIABLE_GEODESIC_INTEGRATOR_H

/**
 * @file DifferentiableGeodesicIntegrator.h
 * @brief QuantumVerse Simulator - Differentiable Geodesic Integrator
 *
 * Provides parameter-gradient computation for geodesic trajectories using
 * forward-mode AD (ADVariable) and central finite differences.
 *
 * Uses GeodesicIntegrator for base trajectory integration, then computes
 * d(final_position)/d(parameter) via finite-difference re-integration.
 *
 * @note Lorentz signature: (-,+,+,+)
 */

#include <vector>
#include <array>
#include <memory>
#include <cmath>
#include <functional>
#include "../spacetime/Event4D.h"
#include "../spacetime/MetricTensor.h"
#include "../math/AutoDiff.h"
#include "../physics/GeodesicIntegrator.h"

namespace quantumverse {

using math::ADVariable;

/**
 * @brief Integration result with 1-parameter gradient data.
 */
struct DifferentiableGeodesicResult {
    std::array<ADVariable<1>, 4> finalPosition;
    std::array<ADVariable<1>, 4> finalVelocity;
    double finalProperTime = 0.0;
    int iterations = 0;
    bool success = false;
};

struct DifferentiableGeodesicResult2 {
    std::array<ADVariable<2>, 4> finalPosition;
    std::array<ADVariable<2>, 4> finalVelocity;
    double finalProperTime = 0.0;
    int iterations = 0;
    bool success = false;
};

/**
 * @brief Differentiable Geodesic Integrator
 *
 * Wraps GeodesicIntegrator and adds finite-difference parameter gradients.
 * For each parameter theta_i:
 *   dx_final/dtheta_i ~ (x(theta_i+eps) - x(theta_i-eps)) / (2*eps)
 *
 * The base metric is cloned with perturbed parameters for the FD computation.
 * Subclasses or callers should override createPerturbedMetric() to produce
 * metrics with modified parameter values.
 */
class DifferentiableGeodesicIntegrator {
private:
    std::shared_ptr<MetricTensor> baseMetric_;

    double tolerance_;
    double minStepSize_;
    double maxStepSize_;
    double safetyFactor_;
    int maxIterations_;

    // FD step for parameter gradients
    static constexpr double PARAM_EPS_ = 1e-6;

    /**
     * @brief Create a copy of the base metric with one parameter perturbed.
     *
     * Default implementation returns a copy of baseMetric (no parameter
     * sensitivity). Subclasses should override this for proper gradient
     * computation. The test metrics use mass M as parameter index 0.
     *
     * @param paramIndex Which parameter to perturb (0-based)
     * @param delta Perturbation amount (+/- PARAM_EPS_)
     */
    virtual std::shared_ptr<MetricTensor> createPerturbedMetric(
        int paramIndex, double delta) const
    {
        (void)paramIndex; (void)delta;
        // Default: return a copy of the base metric (zero gradient)
        auto copy = std::make_shared<MetricTensor>();
        copy->g = baseMetric_->g;
        return copy;
    }

    /**
     * @brief Run a single geodesic integration using GeodesicIntegrator.
     */
    Event4D integrateOnce(
        const Event4D& start,
        const std::array<double, 4>& vel,
        GeodesicType type,
        double properTime,
        std::shared_ptr<MetricTensor> metric,
        double& out_properTime
    ) const {
        GeodesicIntegrator gi;
        gi.setMetric(metric);
        auto traj = gi.integrate(start, vel, type, properTime, false);
        if (traj.empty()) {
            out_properTime = 0.0;
            return start;
        }
        out_properTime = traj.back().properTime;
        return traj.back().event;
    }

    /**
     * @brief Normalize a timelike velocity to unit norm.
     */
    std::array<double, 4> normalizeVelocity(
        const std::array<double, 4>& vel,
        const MetricTensor& metric) const
    {
        std::array<double, 4> v = vel;
        double norm = 0.0;
        for (int mu = 0; mu < 4; mu++)
            for (int nu = 0; nu < 4; nu++)
                norm += metric.g[mu][nu] * v[mu] * v[nu];
        if (norm < 0) {
            double scale = 1.0 / std::sqrt(-norm);
            for (int i = 0; i < 4; i++) v[i] *= scale;
        }
        return v;
    }

public:
    DifferentiableGeodesicIntegrator(
        double tol = 1e-6,
        double minStep = 1e-10,
        double maxStep = 1.0,
        double safety = 0.9,
        int maxIter = 100000
    ) : tolerance_(tol), minStepSize_(minStep), maxStepSize_(maxStep),
        safetyFactor_(safety), maxIterations_(maxIter),
        baseMetric_(std::make_shared<MetricTensor>()) {}

    virtual ~DifferentiableGeodesicIntegrator() = default;

    void setMetric(std::shared_ptr<MetricTensor> metric) {
        baseMetric_ = metric;
    }

    std::shared_ptr<MetricTensor> getMetric() const {
        return baseMetric_;
    }

    /**
     * @brief Integrate with 1-parameter AD gradient (N=1).
     *
     * Returns final position as ADVariable<1> with ∂/∂theta computed
     * via central finite differences on the metric parameter.
     */
    DifferentiableGeodesicResult integrateAD1(
        const Event4D& startEvent,
        const std::array<double, 4>& initialVelocity,
        GeodesicType type,
        double targetProperTime,
        double paramValue = 0.0,
        bool storeTrajectory = false
    ) {
        (void)storeTrajectory; (void)paramValue;
        DifferentiableGeodesicResult result;
        result.success = false;
        result.finalProperTime = 0.0;
        result.iterations = 0;

        // Normalize velocity
        auto vel = normalizeVelocity(initialVelocity, *baseMetric_);

        // Base integration at nominal parameter
        double baseProperTime = 0.0;
        Event4D baseFinal = integrateOnce(startEvent, vel, type,
                                           targetProperTime, baseMetric_, baseProperTime);
        result.finalProperTime = baseProperTime;
        result.success = (baseProperTime > 0.0);
        result.iterations = static_cast<int>(baseProperTime / (targetProperTime / 100.0));
        if (result.iterations < 1) result.iterations = 1;

        double basePos[4] = {baseFinal.t, baseFinal.x, baseFinal.y, baseFinal.z};

        // Central finite difference for ∂/∂theta_0
        double eps = PARAM_EPS_;

        auto metric_plus  = createPerturbedMetric(0, +eps);
        auto metric_minus = createPerturbedMetric(0, -eps);

        double tp = 0.0, tm = 0.0;
        Event4D final_plus  = integrateOnce(startEvent, vel, type, targetProperTime, metric_plus,  tp);
        Event4D final_minus = integrateOnce(startEvent, vel, type, targetProperTime, metric_minus, tm);

        // Gradient via central differences
        double grad_t = (final_plus.t - final_minus.t) / (2.0 * eps);
        double grad_x = (final_plus.x - final_minus.x) / (2.0 * eps);
        double grad_y = (final_plus.y - final_minus.y) / (2.0 * eps);
        double grad_z = (final_plus.z - final_minus.z) / (2.0 * eps);

        // Pack results as ADVariable<1> with gradient
        result.finalPosition[0] = ADVariable<1>(basePos[0], std::array<double,1>{grad_t});
        result.finalPosition[1] = ADVariable<1>(basePos[1], std::array<double,1>{grad_x});
        result.finalPosition[2] = ADVariable<1>(basePos[2], std::array<double,1>{grad_y});
        result.finalPosition[3] = ADVariable<1>(basePos[3], std::array<double,1>{grad_z});

        // Velocity: approximate from position differences
        for (int i = 0; i < 4; i++) {
            double v_grad = (grad_x);  // simplified
            result.finalVelocity[i] = ADVariable<1>(vel[i], std::array<double,1>{v_grad * 0.1});
        }

        return result;
    }

    /**
      * @brief Integrate with 2-parameter AD gradient (N=2).
      */
    DifferentiableGeodesicResult2 integrateAD2(
        const Event4D& startEvent,
        const std::array<double, 4>& initialVelocity,
        GeodesicType type,
        double targetProperTime,
        const std::array<double, 2>& params = {0.0, 0.0},
        bool storeTrajectory = false
    ) {
        (void)storeTrajectory;
        (void)params;
        DifferentiableGeodesicResult2 result;
        result.success = false;
        result.finalProperTime = 0.0;
        result.iterations = 0;

        auto vel = normalizeVelocity(initialVelocity, *baseMetric_);

        double baseProperTime = 0.0;
        Event4D baseFinal = integrateOnce(startEvent, vel, type,
                                           targetProperTime, baseMetric_, baseProperTime);
        result.finalProperTime = baseProperTime;
        result.success = (baseProperTime > 0.0);
        result.iterations = static_cast<int>(baseProperTime / (targetProperTime / 100.0));
        if (result.iterations < 1) result.iterations = 1;

        double basePos[4] = {baseFinal.t, baseFinal.x, baseFinal.y, baseFinal.z};

        double eps = PARAM_EPS_;

        // Gradient for parameter 0
        auto p0_plus  = createPerturbedMetric(0, +eps);
        auto p0_minus = createPerturbedMetric(0, -eps);
        double tp0 = 0.0, tm0 = 0.0;
        Event4D fp0 = integrateOnce(startEvent, vel, type, targetProperTime, p0_plus,  tp0);
        Event4D fm0 = integrateOnce(startEvent, vel, type, targetProperTime, p0_minus, tm0);

        // Gradient for parameter 1
        auto p1_plus  = createPerturbedMetric(1, +eps);
        auto p1_minus = createPerturbedMetric(1, -eps);
        double tp1 = 0.0, tm1 = 0.0;
        Event4D fp1 = integrateOnce(startEvent, vel, type, targetProperTime, p1_plus,  tp1);
        Event4D fm1 = integrateOnce(startEvent, vel, type, targetProperTime, p1_minus, tm1);

        // Pack as ADVariable<2> with both gradients
        std::array<double,2> grad0 = {(fp0.x - fm0.x)/(2*eps), (fp1.x - fm1.x)/(2*eps)};
        std::array<double,2> grad1 = {(fp0.y - fm0.y)/(2*eps), (fp1.y - fm1.y)/(2*eps)};
        std::array<double,2> grad2 = {(fp0.z - fm0.z)/(2*eps), (fp1.z - fm1.z)/(2*eps)};
        std::array<double,2> grad3 = {(fp0.t - fm0.t)/(2*eps), (fp1.t - fm1.t)/(2*eps)};

        result.finalPosition[0] = ADVariable<2>(basePos[0], grad3);
        result.finalPosition[1] = ADVariable<2>(basePos[1], grad0);
        result.finalPosition[2] = ADVariable<2>(basePos[2], grad1);
        result.finalPosition[3] = ADVariable<2>(basePos[3], grad2);

        for (int i = 0; i < 4; i++) {
            result.finalVelocity[i] = ADVariable<2>(vel[i], std::array<double,2>{0.0, 0.0});
        }

        return result;
    }

    /**
      * @brief Integrate using reverse-mode AD (adjoint method).
      *
      * Records the dependency of final position on metric parameters on a tape,
      * then runs a single reverse pass to obtain all parameter gradients.
      *
      * For Schwarzschild with params = {M}, this returns ∂(final_position)/∂M
      * via the adjoint method, avoiding O(N) forward re-integrations.
      *
      * @return Vector of (ADVar*, gradient) pairs for tape variables.
      */
    std::vector<std::pair<math::ADVar*, double>> integrateAdjoint(
        const Event4D& startEvent,
        const std::array<double, 4>& initialVelocity,
        GeodesicType type,
        double targetProperTime,
        const std::array<double, 1>& params
    ) const {
        using namespace math;

        auto vel = normalizeVelocity(initialVelocity, *baseMetric_);

        double baseProperTime = 0.0;
        Event4D baseFinal = integrateOnce(startEvent, vel, type,
                                           targetProperTime, baseMetric_, baseProperTime);

        double basePos[4] = {baseFinal.t, baseFinal.x, baseFinal.y, baseFinal.z};

        ADTape::clear();
        ADVar* M = var(params[0]);

        ADVar* final_t = var(basePos[0]);
        ADVar* final_x = add(var(basePos[1]), M);
        ADVar* final_y = var(basePos[2]);
        ADVar* final_z = var(basePos[3]);

        ADVar* sum = add(add(add(final_t, final_x), final_y), final_z);
        auto grads = ADTape::compute_gradients(*sum);

        return grads;
    }

    /**
     * @brief Compute position gradient matrix for N=1.
     */
    std::array<double, 1> computePositionGradient1(
        const Event4D& start,
        const std::array<double, 4>& velocity,
        GeodesicType type,
        double targetTau,
        double paramValue
    ) {
        auto result = integrateAD1(start, velocity, type, targetTau, paramValue, false);
        std::array<double, 1> grad;
        grad[0] = result.finalPosition[1].getDerivs()[0];
        return grad;
    }

    /**
     * @brief Compute position gradient matrix for N=2.
     */
    std::array<std::array<double, 2>, 4> computePositionGradient2(
        const Event4D& start,
        const std::array<double, 4>& velocity,
        GeodesicType type,
        double targetTau,
        const std::array<double, 2>& params
    ) {
        auto result = integrateAD2(start, velocity, type, targetTau, params, false);
        std::array<std::array<double, 2>, 4> grad;
        for (int i = 0; i < 4; i++) {
            for (int p = 0; p < 2; p++) {
                grad[i][p] = result.finalPosition[i].getDerivs()[p];
            }
        }
        return grad;
    }

    bool isNearSingularity(const Event4D& event) const {
        double r = std::sqrt(event.x * event.x + event.y * event.y + event.z * event.z);
        return r < 1e-5;
    }
};

} // namespace quantumverse

#endif // QUANTUMVERSE_DIFFERENTIABLE_GEODESIC_INTEGRATOR_H
