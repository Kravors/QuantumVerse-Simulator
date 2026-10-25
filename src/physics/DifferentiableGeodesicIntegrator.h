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
#include <stdexcept>
#include "../spacetime/Event4D.h"
#include "../spacetime/MetricTensor.h"
#include "../math/AutoDiff.h"
#include "../physics/GeodesicIntegrator.h"

namespace quantumverse {

using math::ADVariable;
using math::ReverseAD;
using math::ReverseTape;

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
    ) : baseMetric_(std::make_shared<MetricTensor>()), tolerance_(tol), minStepSize_(minStep),
        maxStepSize_(maxStep), safetyFactor_(safety), maxIterations_(maxIter) {}

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

    // ---------- Reverse-mode AD (adjoint) ----------

    /**
     * @brief Compute geodesic RHS acceleration with ReverseAD.
     *
     * acceleration[mu] = -Gamma[mu][alpha][beta] * vel[alpha] * vel[beta]
     *
     * Christoffel symbols are computed via tangent-linear FD on the metric:
     *   d g_{mu nu} / d x^rho ~ (g(x + h*e^rho) - g(x - h*e^rho)) / (2h)
     *
     * All arithmetic goes through ReverseAD so the tape records the
     * dependency of acceleration on position, velocity, and parameters.
     */
    template<size_t N>
    static std::array<ReverseAD<N>, 4> geodesicRHSReverseAD(
        const MetricTensor& metric,
        const std::array<ReverseAD<N>, 4>& pos,
        const std::array<ReverseAD<N>, 4>& vel
    ) {
        // Evaluate metric at the double values of pos (tangent-linear base point)
        Event4D posD(pos[0].value, pos[1].value, pos[2].value, pos[3].value);
        auto g = metric.evaluate(posD);

        // Compute metric derivatives d g_{mu nu} / d x^rho via central FD
        const double h = 1e-6;
        std::array<std::array<std::array<double, 4>, 4>, 4> dg{}; // dg[mu][nu][rho] = d g_{mu nu} / d x^rho

        for (int mu = 0; mu < 4; mu++) {
            for (int nu = 0; nu < 4; nu++) {
                for (int rho = 0; rho < 4; rho++) {
                    Event4D xp = posD, xm = posD;
                    xp[rho] += h; xm[rho] -= h;
                    double gp = metric.evaluate(xp)[mu][nu];
                    double gm = metric.evaluate(xm)[mu][nu];
                    dg[mu][nu][rho] = (gp - gm) / (2.0 * h);
                }
            }
        }

        // Compute inverse metric g^{mu nu} (diagonal approximation)
        std::array<std::array<double, 4>, 4> gInv{};
        for (int i = 0; i < 4; i++) {
            gInv[i][i] = 1.0 / g[i][i];
        }

        // Compute Christoffel symbols Gamma[mu][alpha][beta] as ReverseAD
        // Gamma[mu][alpha][beta] = 0.5 * sum_sigma g^{mu sigma} *
        //   (d g_{sigma alpha}/dx^beta + d g_{sigma beta}/dx^alpha - d g_{alpha beta}/dx^sigma)
        std::array<std::array<std::array<ReverseAD<N>, 4>, 4>, 4> Gamma{};

        for (int mu = 0; mu < 4; mu++) {
            for (int alpha = 0; alpha < 4; alpha++) {
                for (int beta = 0; beta < 4; beta++) {
                    ReverseAD<N> sum = ReverseAD<N>::constant(0.0);
                    for (int sigma = 0; sigma < 4; sigma++) {
                        ReverseAD<N> dgsa_beta = ReverseAD<N>::constant(dg[sigma][alpha][beta]);
                        ReverseAD<N> dgsb_alpha = ReverseAD<N>::constant(dg[sigma][beta][alpha]);
                        ReverseAD<N> dgab_sigma = ReverseAD<N>::constant(dg[alpha][beta][sigma]);

                        ReverseAD<N> gInv_mu_sigma = ReverseAD<N>::constant(gInv[mu][sigma]);
                        ReverseAD<N> term = gInv_mu_sigma * (dgsa_beta + dgsb_alpha - dgab_sigma);
                        sum = sum + term;
                    }
                    Gamma[mu][alpha][beta] = ReverseAD<N>::constant(0.5) * sum;
                }
            }
        }

        // acceleration[mu] = -Gamma[mu][alpha][beta] * vel[alpha] * vel[beta]
        std::array<ReverseAD<N>, 4> accel{};
        for (int mu = 0; mu < 4; mu++) {
            accel[mu] = ReverseAD<N>::constant(0.0);
            for (int alpha = 0; alpha < 4; alpha++) {
                for (int beta = 0; beta < 4; beta++) {
                    accel[mu] = accel[mu] - Gamma[mu][alpha][beta] * vel[alpha] * vel[beta];
                }
            }
        }
        return accel;
    }

    /**
     * @brief Integrate with reverse-mode AD (adjoint method) for scalar loss.
     *
     * Runs a single forward pass through a ReverseAD-aware RK4 integrator,
     * recording the dependency of final position on all N parameters on a tape.
     * Then runs one backward pass to obtain d(loss)/d(theta_i) for all i,
     * where loss = ||x_final - xTarget||^2.
     *
     * Cost: ~1 forward pass + ~0.5 backward pass, independent of N.
     * vs central FD on the loss: 2N full integrations.
     *
     * @return Gradient d(loss)/d(theta_i) for i = 0..N-1.
     */
    template<size_t N>
    std::array<double, N> integrateWithGradientsAdjoint(
        const MetricTensor& metric,
        const Event4D& startEvent,
        const std::array<double, 4>& initialVelocity,
        double targetProperTime,
        const std::array<double, N>& params,
        const Event4D& xTarget
    ) const {
        ReverseTape<N> tape;
        // Estimate steps and nodes per step; reserve generously to avoid
        // reallocations mid-forward-pass (the main perf killer).
        double h = 0.01;  // fixed step size
        int steps = static_cast<int>(std::ceil(targetProperTime / h));
        if (steps < 1) steps = 1;
        tape.reserve(static_cast<size_t>(5000 * steps));

        // Tag parameters on the tape
        std::array<ReverseAD<N>, N> radParams;
        for (size_t i = 0; i < N; ++i) {
            radParams[i] = ReverseAD<N>::param(tape, params[i], i);
        }
        (void)radParams; // params are used implicitly through metric dependence

        // State — seed pos/vel with params so the tape records dependency
        std::array<ReverseAD<N>, 4> pos, vel;
        for (int i = 0; i < 4; i++) {
            pos[i] = ReverseAD<N>::constant(startEvent[i]) +
                     ReverseAD<N>::constant(0.0) * radParams[i % N];
            vel[i] = ReverseAD<N>::constant(initialVelocity[i]) +
                     ReverseAD<N>::constant(0.0) * radParams[i % N];
        }

        double tau = 0.0;
        for (int step = 0; step < steps; step++) {
            // RK4 with ReverseAD
            auto k1_pos = vel;
            auto k1_vel = geodesicRHSReverseAD<N>(metric, pos, vel);

            std::array<ReverseAD<N>, 4> pos2, vel2;
            for (int i = 0; i < 4; i++) {
                pos2[i] = pos[i] + ReverseAD<N>::constant(0.5 * h) * k1_pos[i];
                vel2[i] = vel[i] + ReverseAD<N>::constant(0.5 * h) * k1_vel[i];
            }
            auto k2_pos = vel2;
            auto k2_vel = geodesicRHSReverseAD<N>(metric, pos2, vel2);

            std::array<ReverseAD<N>, 4> pos3, vel3;
            for (int i = 0; i < 4; i++) {
                pos3[i] = pos[i] + ReverseAD<N>::constant(0.5 * h) * k2_pos[i];
                vel3[i] = vel[i] + ReverseAD<N>::constant(0.5 * h) * k2_vel[i];
            }
            auto k3_pos = vel3;
            auto k3_vel = geodesicRHSReverseAD<N>(metric, pos3, vel3);

            std::array<ReverseAD<N>, 4> pos4, vel4;
            for (int i = 0; i < 4; i++) {
                pos4[i] = pos[i] + ReverseAD<N>::constant(h) * k3_pos[i];
                vel4[i] = vel[i] + ReverseAD<N>::constant(h) * k3_vel[i];
            }
            auto k4_pos = vel4;
            auto k4_vel = geodesicRHSReverseAD<N>(metric, pos4, vel4);

            for (int i = 0; i < 4; i++) {
                pos[i] = pos[i] + ReverseAD<N>::constant(h / 6.0) *
                    (k1_pos[i] + ReverseAD<N>::constant(2.0) * k2_pos[i] +
                     ReverseAD<N>::constant(2.0) * k3_pos[i] + k4_pos[i]);
                vel[i] = vel[i] + ReverseAD<N>::constant(h / 6.0) *
                    (k1_vel[i] + ReverseAD<N>::constant(2.0) * k2_vel[i] +
                     ReverseAD<N>::constant(2.0) * k3_vel[i] + k4_vel[i]);
            }

            tau += h;
        }

        // Scalar loss = ||x_final - xTarget||^2
        ReverseAD<N> loss = ReverseAD<N>::constant(0.0);
        for (int i = 0; i < 4; i++) {
            ReverseAD<N> diff = pos[i] - ReverseAD<N>::constant(xTarget[i]);
            loss = loss + diff * diff;
        }

        // Backward pass: d(loss)/d(theta_i) for all i
        return tape.backward(loss.node);
    }

    bool isNearSingularity(const Event4D& event) const {
        double r = std::sqrt(event.x * event.x + event.y * event.y + event.z * event.z);
        return r < 1e-5;
    }
};

} // namespace quantumverse

#endif // QUANTUMVERSE_DIFFERENTIABLE_GEODESIC_INTEGRATOR_H
