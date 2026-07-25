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
 * Current implementation targets Schwarzschild spacetime with mass M as the
 * differentiable parameter. Christoffel symbols are computed analytically.
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
    std::vector<double> finalState;            ///< Final state [t, r, theta, phi, u^t, u^r, u^theta, u^phi]
    std::vector<double> dState_dMass;          ///< Gradient of final state w.r.t. mass M
    double finalProperTime = 0.0;
    int steps = 0;
    bool success = false;
};

/**
 * @brief Adjoint-mode geodesic integrator for Schwarzschild spacetime.
 *
 * Wraps the geodesic ODE in reverse-mode AD tape operations, enabling
 * O(1) gradient computation for N parameters via a single backward pass.
 *
 * For Schwarzschild metric:
 *   ds^2 = -(1 - 2M/r) dt^2 + (1 - 2M/r)^{-1} dr^2 + r^2 dOmega^2
 *
 * Analytical Christoffel symbols are used to avoid finite-difference
 * metric derivatives, ensuring exact tape-based gradients.
 */
class AdjointGeodesicIntegrator {
private:
    double mass_;
    double tolerance_;
    double minStep_;
    double maxStep_;
    double safety_;
    int maxIter_;

    /**
     * @brief Create an ADVar* representing a constant on the tape.
     */
    static ADVar* constant(double v) {
        ADVar* var = new ADVar(v);
        ADTape::push(var);
        return var;
    }

    /**
     * @brief Compute analytical Christoffel symbols for Schwarzschild
     *        using tape arithmetic. All returned ADVar* are owned by the tape.
     *
     * State indices: 0=t, 1=r, 2=theta, 3=phi, 4=u^t, 5=u^r, 6=u^theta, 7=u^phi
     */
    void computeChristoffelSchwarzschild(
        const std::vector<ADVar*>& y,
        ADVar* M,
        std::vector<std::vector<std::vector<ADVar*>>>& Gamma
    ) const {
        ADVar* r = y[1];
        ADVar* theta = y[2];

        ADVar* rs = math::mul(M, constant(2.0 * Event4D::G / (Event4D::C * Event4D::C)));
        ADVar* r_minus_rs = math::sub(r, rs);
        ADVar* r_inv = math::div(constant(1.0), r);
        ADVar* r2 = math::mul(r, r);
        ADVar* r3 = math::mul(r2, r);
        ADVar* sin_theta = math::sin(theta);
        ADVar* cos_theta = math::cos(theta);
        ADVar* sin2 = math::mul(sin_theta, sin_theta);

        ADVar* two_r = math::mul(constant(2.0), r);
        ADVar* Gamma_t_tr = math::div(rs, math::mul(two_r, r_minus_rs));
        ADVar* Gamma_r_tt = math::div(math::mul(rs, r_minus_rs), math::mul(r3, constant(2.0)));
        ADVar* Gamma_r_rr = math::neg(Gamma_t_tr);
        ADVar* Gamma_r_thetatheta = math::neg(r_minus_rs);
        ADVar* Gamma_r_phiphi = math::neg(math::mul(r_minus_rs, sin2));
        ADVar* Gamma_theta_rtheta = r_inv;
        ADVar* Gamma_theta_phiphi = math::neg(math::mul(sin_theta, cos_theta));
        ADVar* Gamma_phi_rphi = r_inv;
        ADVar* Gamma_phi_thetaphi = math::div(cos_theta, sin_theta);

        for (int l = 0; l < 4; ++l)
            for (int m = 0; m < 4; ++m)
                for (int n = 0; n < 4; ++n)
                    Gamma[l][m][n] = constant(0.0);

        Gamma[0][1][0] = Gamma[0][0][1] = Gamma_t_tr;
        Gamma[1][0][0] = Gamma_r_tt;
        Gamma[1][1][1] = Gamma_r_rr;
        Gamma[1][2][2] = Gamma_r_thetatheta;
        Gamma[1][3][3] = Gamma_r_phiphi;
        Gamma[2][1][2] = Gamma[2][2][1] = Gamma_theta_rtheta;
        Gamma[2][3][3] = Gamma_theta_phiphi;
        Gamma[3][1][3] = Gamma[3][3][1] = Gamma_phi_rphi;
        Gamma[3][2][3] = Gamma[3][3][2] = Gamma_phi_thetaphi;
    }

    /**
     * @brief Geodesic ODE right-hand side using tape arithmetic.
     */
    std::vector<ADVar*> geodesicRHS(
        const std::vector<ADVar*>& y,
        ADVar* M
    ) const {
        std::vector<std::vector<std::vector<ADVar*>>> Gamma(4,
            std::vector<std::vector<ADVar*>>(4, std::vector<ADVar*>(4)));
        computeChristoffelSchwarzschild(y, M, Gamma);

        ADVar* u_t = y[4];
        ADVar* u_r = y[5];
        ADVar* u_theta = y[6];
        ADVar* u_phi = y[7];

        ADVar* dt = u_t;
        ADVar* dr = u_r;
        ADVar* dtheta = u_theta;
        ADVar* dphi = u_phi;

        ADVar* du_t = constant(0.0);
        for (int mu = 0; mu < 4; ++mu)
            for (int nu = 0; nu < 4; ++nu)
                du_t = math::sub(du_t, math::mul(Gamma[0][mu][nu], math::mul(y[mu + 4], y[nu + 4])));

        ADVar* du_r = constant(0.0);
        for (int mu = 0; mu < 4; ++mu)
            for (int nu = 0; nu < 4; ++nu)
                du_r = math::sub(du_r, math::mul(Gamma[1][mu][nu], math::mul(y[mu + 4], y[nu + 4])));

        ADVar* du_theta = constant(0.0);
        for (int mu = 0; mu < 4; ++mu)
            for (int nu = 0; nu < 4; ++nu)
                du_theta = math::sub(du_theta, math::mul(Gamma[2][mu][nu], math::mul(y[mu + 4], y[nu + 4])));

        ADVar* du_phi = constant(0.0);
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
        ADVar* M,
        double h
    ) const {
        std::vector<ADVar*> k1 = geodesicRHS(y, M);

        std::vector<ADVar*> y2(8);
        for (int i = 0; i < 8; ++i)
            y2[i] = math::add(y[i], math::mul(k1[i], constant(h * 0.5)));
        std::vector<ADVar*> k2 = geodesicRHS(y2, M);

        std::vector<ADVar*> y3(8);
        for (int i = 0; i < 8; ++i)
            y3[i] = math::add(y[i], math::mul(k2[i], constant(h * 0.5)));
        std::vector<ADVar*> k3 = geodesicRHS(y3, M);

        std::vector<ADVar*> y4(8);
        for (int i = 0; i < 8; ++i)
            y4[i] = math::add(y[i], math::mul(k3[i], constant(h)));
        std::vector<ADVar*> k4 = geodesicRHS(y4, M);

        std::vector<ADVar*> y_new(8);
        for (int i = 0; i < 8; ++i) {
            ADVar* term1 = k1[i];
            ADVar* term2 = math::mul(k2[i], constant(2.0));
            ADVar* term3 = math::mul(k3[i], constant(2.0));
            ADVar* term4 = k4[i];
            ADVar* sum1 = math::add(term1, term2);
            ADVar* sum2 = math::add(sum1, term3);
            ADVar* sum3 = math::add(sum2, term4);
            y_new[i] = math::add(y[i], math::mul(sum3, constant(h / 6.0)));
        }

        return y_new;
    }

public:
    AdjointGeodesicIntegrator(
        double mass = 1.0,
        double tol = 1e-8,
        double minStep = 1e-10,
        double maxStep = 1.0,
        double safety = 0.9,
        int maxIter = 100000
    ) : mass_(mass), tolerance_(tol), minStep_(minStep),
        maxStep_(maxStep), safety_(safety), maxIter_(maxIter) {}

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

        ADVar* M = ADTape::record(mass_, nullptr);

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

        ADVar* rs = math::mul(M, constant(2.0 * Event4D::G / (Event4D::C * Event4D::C)));
        ADVar* factor = math::sub(constant(1.0), math::div(rs, y[1]));
        ADVar* g_tt = math::neg(factor);
        ADVar* g_rr = math::div(constant(1.0), factor);
        ADVar* g_thetatheta = math::mul(y[1], y[1]);
        ADVar* sin_theta = math::sin(y[2]);
        ADVar* g_phiphi = math::mul(g_thetatheta, math::mul(sin_theta, sin_theta));

        ADVar* norm = math::mul(g_tt, math::mul(y[4], y[4]));
        norm = math::add(norm, math::mul(g_rr, math::mul(y[5], y[5])));
        norm = math::add(norm, math::mul(g_thetatheta, math::mul(y[6], y[6])));
        norm = math::add(norm, math::mul(g_phiphi, math::mul(y[7], y[7])));

        if (norm->value < 0.0) {
            ADVar* scale = math::div(constant(1.0), math::sqrt(math::neg(norm)));
            for (int i = 4; i < 8; ++i) {
                y[i] = math::mul(y[i], scale);
            }
        }

        double h = maxStep_;
        double tau = 0.0;
        int steps = 0;

        while (tau < targetProperTime && steps < maxIter_) {
            if (tau + h > targetProperTime) h = targetProperTime - tau;

            std::vector<ADVar*> y_new = rk4Step(y, M, h);

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

        result.dState_dMass.resize(8);
        for (int i = 0; i < 8; ++i) {
            auto& tape_vars = ADTape::getVariables();
            for (auto* var : tape_vars) var->grad = 0.0;
            y[i]->grad = 1.0;
            for (auto it = tape_vars.rbegin(); it != tape_vars.rend(); ++it) {
                if ((*it)->backward) (*it)->backward();
            }
            result.dState_dMass[i] = M->grad;
        }

        return result;
    }

    /**
     * @brief Compute gradient of the Kretschmann scalar w.r.t. mass at a location
     *        using reverse-mode AD.
     *
     * K = 48 * (GM/c^2)^2 / r^6 for Schwarzschild.
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

        ADVar* rs = math::mul(M, constant(2.0 * Event4D::G / (Event4D::C * Event4D::C)));
        ADVar* r_var = pos[1];
        ADVar* r2 = math::mul(r_var, r_var);
        ADVar* r4 = math::mul(r2, r2);
        ADVar* r6 = math::mul(r4, r2);
        ADVar* rs2 = math::mul(rs, rs);
        ADVar* K = math::div(math::mul(constant(48.0), rs2), r6);

        auto& tape_vars = ADTape::getVariables();
        for (auto* var : tape_vars) var->grad = 0.0;
        K->grad = 1.0;
        for (auto it = tape_vars.rbegin(); it != tape_vars.rend(); ++it) {
            if ((*it)->backward) (*it)->backward();
        }

        return M->grad;
    }

    /**
     * @brief Compute gradient of the final geodesic state w.r.t. mass using
     *        reverse-mode AD (wrapper around integrate).
     *
     * @param startEvent Initial spacetime event.
     * @param initialVelocity Initial four-velocity.
     * @param type Geodesic type.
     * @param targetProperTime Integration duration.
     * @return Pair of (finalState, d(finalState)/dM).
     */
    std::pair<std::vector<double>, std::vector<double>> computeStateGradient(
        const Event4D& startEvent,
        const std::array<double, 4>& initialVelocity,
        GeodesicType type,
        double targetProperTime
    ) const {
        auto result = integrate(startEvent, initialVelocity, type, targetProperTime);
        return std::make_pair(result.finalState, result.dState_dMass);
    }

    /**
     * @brief Compute gradient of final spatial radius w.r.t. mass using
     *        central finite differences (for validation).
     */
    std::vector<double> computePositionGradientFD(
        const Event4D& startEvent,
        const std::array<double, 4>& initialVelocity,
        GeodesicType type,
        double targetProperTime,
        double eps = 1e-6
    ) const {
        auto makeMetricField = [this](double mass) {
            return [mass](const Event4D& evt) -> MetricTensor {
                double r = std::sqrt(evt.x * evt.x + evt.y * evt.y + evt.z * evt.z);
                const double minR = 1e-9;
                if (r < minR) r = minR;
                return MetricTensor::schwarzschild(mass, r, kPi / 2, 0.0);
            };
        };

        double base_pt = 0.0;
        Event4D base_final;
        {
            GeodesicIntegrator gi;
            gi.setMetricField(makeMetricField(mass_));
            auto traj = gi.integrate(startEvent, initialVelocity, type, targetProperTime, false);
            if (traj.empty()) return std::vector<double>(8, 0.0);
            base_final = traj.back().event;
            base_pt = traj.back().properTime;
        }

        std::vector<double> grad(8, 0.0);
        double M_plus = mass_ + eps;
        double M_minus = mass_ - eps;

        Event4D final_plus, final_minus;
        {
            GeodesicIntegrator gi;
            gi.setMetricField(makeMetricField(M_plus));
            auto traj = gi.integrate(startEvent, initialVelocity, type, targetProperTime, false);
            final_plus = traj.empty() ? startEvent : traj.back().event;
        }
        {
            GeodesicIntegrator gi;
            gi.setMetricField(makeMetricField(M_minus));
            auto traj = gi.integrate(startEvent, initialVelocity, type, targetProperTime, false);
            final_minus = traj.empty() ? startEvent : traj.back().event;
        }

        grad[0] = (final_plus.t - final_minus.t) / (2.0 * eps);
        grad[1] = (final_plus.x - final_minus.x) / (2.0 * eps);
        grad[2] = (final_plus.y - final_minus.y) / (2.0 * eps);
        grad[3] = (final_plus.z - final_minus.z) / (2.0 * eps);
        return grad;
    }
};

} // namespace physics
} // namespace quantumverse

#endif // QUANTUMVERSE_ADJOINT_GEODESIC_INTEGRATOR_H
