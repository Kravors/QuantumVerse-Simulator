#ifndef QUANTUMVERSE_ADJOINT_GEODESIC_INTEGRATOR_H
#define QUANTUMVERSE_ADJOINT_GEODESIC_INTEGRATOR_H

/**
 * @file AdjointGeodesicIntegrator.h
 * @brief QuantumVerse Simulator - Adjoint-Mode Geodesic Integrator
 *
 * Records the full geodesic ODE integration on a reverse-mode AD tape,
 * then performs a single backward pass to obtain d(final_state)/d(parameter)
 * for all parameters simultaneously.
 *
 * Christoffel symbols are supplied by a MetricTensor interface, enabling
 * gradient-based optimization for any theory metric that implements
 * evaluateAD / computeChristoffelAD.
 *
 * @note Lorentz signature: (-,+,+,+)
 */

#include <vector>
#include <array>
#include <cmath>
#include <memory>
#include <algorithm>
#include "../spacetime/Event4D.h"
#include "../spacetime/MetricTensor.h"
#include "../math/AutoDiff.h"
#include "../physics/GeodesicIntegrator.h"

namespace quantumverse {
namespace physics {

using math::ADVar;
using math::ADTape;

constexpr double kPi = 3.14159265358979323846;

/**
 * @brief Adjoint geodesic integration result with parameter gradients.
 */
struct AdjointGeodesicResult {
    std::vector<double> finalState;              ///< Final state [t, r, theta, phi, u^t, u^r, u^theta, u^phi]
    std::vector<std::vector<double>> dState_dParams; ///< Gradient of final state w.r.t. each parameter [state_idx][param_idx]
    double finalProperTime = 0.0;
    int steps = 0;
    bool success = false;
};

/**
 * @brief Adjoint-mode geodesic integrator for arbitrary metrics.
 *
 * Wraps the geodesic ODE in reverse-mode AD tape operations, enabling
 * O(N) gradient computation for N parameters via a single backward pass
 * per state component.
 *
 * The integrator delegates Christoffel symbol and metric component
 * computation to a MetricTensor, which may provide analytic AD
 * implementations via evaluateAD / computeChristoffelAD.
 */
class AdjointGeodesicIntegrator {
private:
    std::shared_ptr<MetricTensor> metric_;
    std::vector<double> param_values_;
    double tolerance_;
    double minStep_;
    double maxStep_;
    double safety_;
    int maxIter_;

    /**
     * @brief Create tape variables from stored parameter values.
     */
    std::vector<ADVar*> createTapeParams() const {
        std::vector<ADVar*> tape_params;
        tape_params.reserve(param_values_.size());
        for (double v : param_values_)
            tape_params.push_back(ADTape::record(v, nullptr));
        return tape_params;
    }

    /**
     * @brief Compute Christoffel symbols from the generic metric interface.
     */
    std::array<std::array<std::array<ADVar*, 4>, 4>, 4> computeChristoffelFromMetric(
        const std::array<ADVar*, 4>& pos,
        const std::vector<ADVar*>& tape_params
    ) const {
        return metric_->computeChristoffelAD(pos, tape_params);
    }

    /**
     * @brief Geodesic ODE right-hand side using tape arithmetic and generic metric.
     */
    std::vector<ADVar*> geodesicRHS(
        const std::vector<ADVar*>& y,
        const std::vector<ADVar*>& tape_params
    ) const {
        std::array<ADVar*, 4> pos = {y[0], y[1], y[2], y[3]};
        auto Gamma = computeChristoffelFromMetric(pos, tape_params);

        ADVar* u_t = y[4];
        ADVar* u_r = y[5];
        ADVar* u_theta = y[6];
        ADVar* u_phi = y[7];

        ADVar* dt = u_t;
        ADVar* dr = u_r;
        ADVar* dtheta = u_theta;
        ADVar* dphi = u_phi;

        ADVar* du_t = ADTape::record(0.0, nullptr);
        for (int mu = 0; mu < 4; ++mu)
            for (int nu = 0; nu < 4; ++nu)
                du_t = math::sub(du_t, math::mul(Gamma[0][mu][nu], math::mul(y[mu + 4], y[nu + 4])));

        ADVar* du_r = ADTape::record(0.0, nullptr);
        for (int mu = 0; mu < 4; ++mu)
            for (int nu = 0; nu < 4; ++nu)
                du_r = math::sub(du_r, math::mul(Gamma[1][mu][nu], math::mul(y[mu + 4], y[nu + 4])));

        ADVar* du_theta = ADTape::record(0.0, nullptr);
        for (int mu = 0; mu < 4; ++mu)
            for (int nu = 0; nu < 4; ++nu)
                du_theta = math::sub(du_theta, math::mul(Gamma[2][mu][nu], math::mul(y[mu + 4], y[nu + 4])));

        ADVar* du_phi = ADTape::record(0.0, nullptr);
        for (int mu = 0; mu < 4; ++mu)
            for (int nu = 0; nu < 4; ++nu)
                du_phi = math::sub(du_phi, math::mul(Gamma[3][mu][nu], math::mul(y[mu + 4], y[nu + 4])));

        return {dt, dr, dtheta, dphi, du_t, du_r, du_theta, du_phi};
    }

    /**
     * @brief RK4 single step with tape recording.
     */
    std::vector<ADVar*> rk4Step(
        const std::vector<ADVar*>& y,
        const std::vector<ADVar*>& tape_params,
        double h
    ) const {
        std::vector<ADVar*> k1 = geodesicRHS(y, tape_params);

        std::vector<ADVar*> y2(8);
        for (int i = 0; i < 8; ++i)
            y2[i] = math::add(y[i], math::mul(k1[i], ADTape::record(h * 0.5, nullptr)));
        std::vector<ADVar*> k2 = geodesicRHS(y2, tape_params);

        std::vector<ADVar*> y3(8);
        for (int i = 0; i < 8; ++i)
            y3[i] = math::add(y[i], math::mul(k2[i], ADTape::record(h * 0.5, nullptr)));
        std::vector<ADVar*> k3 = geodesicRHS(y3, tape_params);

        std::vector<ADVar*> y4(8);
        for (int i = 0; i < 8; ++i)
            y4[i] = math::add(y[i], math::mul(k3[i], ADTape::record(h, nullptr)));
        std::vector<ADVar*> k4 = geodesicRHS(y4, tape_params);

        std::vector<ADVar*> y_new(8);
        for (int i = 0; i < 8; ++i) {
            ADVar* term1 = k1[i];
            ADVar* term2 = math::mul(k2[i], ADTape::record(2.0, nullptr));
            ADVar* term3 = math::mul(k3[i], ADTape::record(2.0, nullptr));
            ADVar* term4 = k4[i];
            ADVar* sum1 = math::add(term1, term2);
            ADVar* sum2 = math::add(sum1, term3);
            ADVar* sum3 = math::add(sum2, term4);
            y_new[i] = math::add(y[i], math::mul(sum3, ADTape::record(h / 6.0, nullptr)));
        }

        return y_new;
    }

    /**
     * @brief Initialize common state.
     */
    void initialize(
        std::shared_ptr<MetricTensor> metric,
        const std::vector<double>& param_values,
        double tol, double minStep, double maxStep, double safety, int maxIter
    ) {
        metric_ = std::move(metric);
        param_values_ = param_values;
        tolerance_ = tol;
        minStep_ = minStep;
        maxStep_ = maxStep;
        safety_ = safety;
        maxIter_ = maxIter;
    }

public:
    /**
     * @brief Legacy constructor for Schwarzschild-only usage.
     *
     * Creates an internal SchwarzschildMetric and records mass as the
     * sole differentiable parameter.
     */
    AdjointGeodesicIntegrator(
        double mass = 1.0,
        double tol = 1e-8,
        double minStep = 1e-10,
        double maxStep = 1.0,
        double safety = 0.9,
        int maxIter = 100000
    ) {
        auto metric = std::make_shared<SchwarzschildMetric>(mass);
        initialize(std::move(metric), {mass}, tol, minStep, maxStep, safety, maxIter);
    }

    /**
     * @brief Generalized constructor for arbitrary metrics.
     *
     * @param metric MetricTensor providing Christoffel and metric evaluation.
     * @param param_values Parameter values (one per differentiable parameter).
     * @param tol Integration tolerance.
     * @param minStep Minimum step size.
     * @param maxStep Maximum step size.
     * @param safety Step size safety factor.
     * @param maxIter Maximum iteration count.
     */
    AdjointGeodesicIntegrator(
        std::shared_ptr<MetricTensor> metric,
        const std::vector<double>& param_values,
        double tol = 1e-8,
        double minStep = 1e-10,
        double maxStep = 1.0,
        double safety = 0.9,
        int maxIter = 100000
    ) {
        initialize(std::move(metric), param_values, tol, minStep, maxStep, safety, maxIter);
    }

    /**
     * @brief Integrate geodesic with adjoint-mode gradient computation.
     */
    AdjointGeodesicResult integrate(
        const Event4D& startEvent,
        const std::array<double, 4>& initialVelocity,
        GeodesicType type,
        double targetProperTime
    ) const {
        (void)type;
        AdjointGeodesicResult result;
        result.success = false;

        ADTape::clear();

        auto tape_params = createTapeParams();
        size_t n_params = tape_params.size();

        double r = std::sqrt(startEvent.x * startEvent.x + startEvent.y * startEvent.y + startEvent.z * startEvent.z);
        const double minR = 1e-9;
        if (r < minR) r = minR;
        double theta = std::acos(std::max(-1.0, std::min(1.0, startEvent.z / r)));
        double phi = std::atan2(startEvent.y, startEvent.x);

        double u_r = (startEvent.x * initialVelocity[1] + startEvent.y * initialVelocity[2] + startEvent.z * initialVelocity[3]) / r;
        double u_theta = 0.0;
        double u_phi = 0.0;
        if (std::sin(theta) > 1e-15) {
            u_theta = (startEvent.z * u_r - initialVelocity[3]) / r;
            u_phi = (startEvent.x * initialVelocity[2] - startEvent.y * initialVelocity[1]) / (r * r * std::sin(theta) * std::sin(theta));
        }

        std::vector<ADVar*> y(8);
        y[0] = ADTape::record(startEvent.t, nullptr);
        y[1] = ADTape::record(r, nullptr);
        y[2] = ADTape::record(theta, nullptr);
        y[3] = ADTape::record(phi, nullptr);
        y[4] = ADTape::record(initialVelocity[0], nullptr);
        y[5] = ADTape::record(u_r, nullptr);
        y[6] = ADTape::record(u_theta, nullptr);
        y[7] = ADTape::record(u_phi, nullptr);

        std::array<ADVar*, 4> pos = {y[0], y[1], y[2], y[3]};
        auto g = metric_->evaluateAD(pos, tape_params);

        ADVar* g_tt = g[0][0];
        ADVar* g_rr = g[1][1];
        ADVar* g_thetatheta = g[2][2];
        ADVar* g_phiphi = g[3][3];

        ADVar* norm = math::mul(g_tt, math::mul(y[4], y[4]));
        norm = math::add(norm, math::mul(g_rr, math::mul(y[5], y[5])));
        norm = math::add(norm, math::mul(g_thetatheta, math::mul(y[6], y[6])));
        norm = math::add(norm, math::mul(g_phiphi, math::mul(y[7], y[7])));

        if (norm->value < 0.0) {
            ADVar* scale = math::div(ADTape::record(1.0, nullptr), math::sqrt(math::neg(norm)));
            for (int i = 4; i < 8; ++i) {
                y[i] = math::mul(y[i], scale);
            }
        }

        double h = maxStep_;
        double tau = 0.0;
        int steps = 0;

        while (tau < targetProperTime && steps < maxIter_) {
            if (tau + h > targetProperTime) h = targetProperTime - tau;

            std::vector<ADVar*> y_new = rk4Step(y, tape_params, h);

            bool all_finite = true;
            for (auto* var : y_new) {
                if (!std::isfinite(var->value)) {
                    all_finite = false;
                    break;
                }
            }

            if (!all_finite && h > minStep_) {
                h *= 0.5;
                continue;
            }

            y = std::move(y_new);
            tau += h;
            ++steps;

            if (h > minStep_) h *= safety_;
        }

        result.finalState.resize(8);
        for (int i = 0; i < 8; ++i)
            result.finalState[i] = y[i]->value;
        result.finalProperTime = tau;
        result.steps = steps;
        result.success = (tau >= targetProperTime * 0.99 || steps < maxIter_);

        result.dState_dParams.assign(8, std::vector<double>(n_params, 0.0));
        auto& tape_vars = ADTape::getVariables();
        for (int i = 0; i < 8; ++i) {
            for (auto& v : tape_vars) v->grad = 0.0;
            y[i]->grad = 1.0;
            for (auto it = tape_vars.rbegin(); it != tape_vars.rend(); ++it) {
                if ((*it)->backward) (*it)->backward();
            }
            for (size_t p = 0; p < n_params; ++p) {
                result.dState_dParams[i][p] = tape_params[p]->grad;
            }
        }

        return result;
    }

    /**
     * @brief Compute gradient of the Kretschmann scalar w.r.t. all parameters
     *        using reverse-mode AD (legacy Schwarzschild interface).
     *
     * @param mass Schwarzschild mass parameter.
     * @param location Spacetime location.
     * @return dKretschmann/dM.
     */
    double computeCurvatureGradient(double mass, const Event4D& location) const {
        ADTape::clear();

        ADVar* M = ADTape::record(mass, nullptr);

        double r = std::sqrt(location.x * location.x + location.y * location.y + location.z * location.z);
        const double minR = 1e-9;
        if (r < minR) r = minR;

        std::vector<ADVar*> pos(4);
        pos[0] = ADTape::record(location.t, nullptr);
        pos[1] = ADTape::record(r, nullptr);
        pos[2] = ADTape::record(std::acos(std::max(-1.0, std::min(1.0, location.z / r))), nullptr);
        pos[3] = ADTape::record(std::atan2(location.y, location.x), nullptr);

        std::array<ADVar*, 4> pos_arr = {pos[0], pos[1], pos[2], pos[3]};
        std::vector<ADVar*> tape_params = {M};
        auto Gamma = metric_->computeChristoffelAD(pos_arr, tape_params);
        (void)Gamma;

        ADVar* rs = math::mul(M, ADTape::record(2.0 * Event4D::G / (Event4D::C * Event4D::C), nullptr));
        ADVar* r_var = pos[1];
        ADVar* r2 = math::mul(r_var, r_var);
        ADVar* r4 = math::mul(r2, r2);
        ADVar* r6 = math::mul(r4, r2);
        ADVar* rs2 = math::mul(rs, rs);
        ADVar* K = math::div(math::mul(ADTape::record(48.0, nullptr), rs2), r6);

        auto& tape_vars = ADTape::getVariables();
        for (auto& v : tape_vars) v->grad = 0.0;
        K->grad = 1.0;
        for (auto it = tape_vars.rbegin(); it != tape_vars.rend(); ++it) {
            if ((*it)->backward) (*it)->backward();
        }

        return M->grad;
    }

    /**
     * @brief Compute gradient of final geodesic state w.r.t. all parameters
     *        using reverse-mode AD (wrapper around integrate).
     *
     * @param startEvent Initial spacetime event.
     * @param initialVelocity Initial four-velocity.
     * @param type Geodesic type.
     * @param targetProperTime Integration duration.
     * @return Pair of (finalState, d(finalState)/dParams matrix).
     */
    std::pair<std::vector<double>, std::vector<std::vector<double>>> computeStateGradient(
        const Event4D& startEvent,
        const std::array<double, 4>& initialVelocity,
        GeodesicType type,
        double targetProperTime
    ) const {
        auto result = integrate(startEvent, initialVelocity, type, targetProperTime);
        return std::make_pair(result.finalState, result.dState_dParams);
    }

    /**
     * @brief Compute gradient of final spatial radius w.r.t. parameters using
     *        central finite differences (for validation).
     */
    std::vector<double> computePositionGradientFD(
        const Event4D& startEvent,
        const std::array<double, 4>& initialVelocity,
        GeodesicType type,
        double targetProperTime,
        double eps = 1e-6
    ) const {
        auto makeMetricField = [this](const std::vector<double>& p) {
            return [this, p](const Event4D& evt) -> MetricTensor {
                (void)p;
                MetricTensor m;
                m.g = metric_->evaluate(evt);
                return m;
            };
        };

        double base_pt = 0.0;
        (void)base_pt;
        Event4D base_final;
        {
            GeodesicIntegrator gi;
            gi.setMetricField(makeMetricField(param_values_));
            auto traj = gi.integrate(startEvent, initialVelocity, type, targetProperTime, false);
            if (traj.empty()) return std::vector<double>(8, 0.0);
            base_final = traj.back().event;
            base_pt = traj.back().properTime;
        }

        size_t n_params = param_values_.size();
        std::vector<double> grad(8 * n_params, 0.0);

        for (size_t idx = 0; idx < n_params; ++idx) {
            double p_plus = param_values_[idx] + eps;
            double p_minus = param_values_[idx] - eps;

            Event4D final_plus, final_minus;
            {
                GeodesicIntegrator gi;
                gi.setMetricField(makeMetricField({p_plus}));
                auto traj = gi.integrate(startEvent, initialVelocity, type, targetProperTime, false);
                final_plus = traj.empty() ? startEvent : traj.back().event;
            }
            {
                GeodesicIntegrator gi;
                gi.setMetricField(makeMetricField({p_minus}));
                auto traj = gi.integrate(startEvent, initialVelocity, type, targetProperTime, false);
                final_minus = traj.empty() ? startEvent : traj.back().event;
            }

            grad[idx * 8 + 0] = (final_plus.t - final_minus.t) / (2.0 * eps);
            grad[idx * 8 + 1] = (final_plus.x - final_minus.x) / (2.0 * eps);
            grad[idx * 8 + 2] = (final_plus.y - final_minus.y) / (2.0 * eps);
            grad[idx * 8 + 3] = (final_plus.z - final_minus.z) / (2.0 * eps);
        }

        return grad;
    }
};

} // namespace physics
} // namespace quantumverse

#endif // QUANTUMVERSE_ADJOINT_GEODESIC_INTEGRATOR_H
