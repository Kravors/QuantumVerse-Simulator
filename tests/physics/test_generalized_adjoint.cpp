// Phase 33.6: Generalized Adjoint Integrator Tests
// Validates that the adjoint integrator works with arbitrary MetricTensor
// implementations via the evaluateAD / computeChristoffelAD interface.

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>

#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "physics/AdjointGeodesicIntegrator.h"
#include "discovery/DiscoveryEngine.h"

using namespace quantumverse;
using namespace quantumverse::physics;

namespace {
constexpr double kEps = 1e-9;
constexpr double kPi = 3.14159265358979323846;
}

// ---------------------------------------------------------------------------
// Mock metric with known analytic AD: g_tt = -(1 - a/r), g_rr = 1/(1 - a/r)
// where a is a differentiable parameter. Christoffel symbols are the
// Schwarzschild-like ones with rs replaced by a.
// ---------------------------------------------------------------------------
class MockParametricMetric : public MetricTensor {
public:
    explicit MockParametricMetric(double a) : m_a(a) {}

    std::array<std::array<double, 4>, 4> evaluate(const Event4D& event) const override {
        double x = event.x, y = event.y, z = event.z;
        double r = std::sqrt(x * x + y * y + z * z);
        const double minR = 1e-9;
        if (r < minR) r = minR;

        double factor = 1.0 - m_a / r;
        std::array<std::array<double, 4>, 4> g4{};
        g4[0][0] = -factor;
        g4[1][1] = 1.0 / factor;
        g4[2][2] = r * r;
        g4[3][3] = r * r;
        return g4;
    }

    std::array<std::array<RVar, 4>, 4> evaluateAD(
        const std::array<RVar, 4>& pos,
        const std::vector<RVar>& params
    ) const override {
        RVar r = pos[1];
        RVar theta = pos[2];

        RVar a = params.empty() ? math::ADTape::record(m_a, nullptr) : params[0];
        RVar factor = math::sub(math::ADTape::record(1.0, nullptr), math::div(a, r));
        RVar g_tt = math::neg(factor);
        RVar g_rr = math::div(math::ADTape::record(1.0, nullptr), factor);
        RVar g_thetatheta = math::mul(r, r);
        RVar sin_theta = math::sin(theta);
        RVar g_phiphi = math::mul(g_thetatheta, math::mul(sin_theta, sin_theta));

        std::array<std::array<RVar, 4>, 4> result{};
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                result[i][j] = math::ADTape::record(0.0, nullptr);

        result[0][0] = g_tt;
        result[1][1] = g_rr;
        result[2][2] = g_thetatheta;
        result[3][3] = g_phiphi;
        return result;
    }

    std::array<std::array<std::array<RVar, 4>, 4>, 4> computeChristoffelAD(
        const std::array<RVar, 4>& pos,
        const std::vector<RVar>& params
    ) const override {
        RVar r = pos[1];
        RVar theta = pos[2];

        RVar a = params.empty() ? math::ADTape::record(m_a, nullptr) : params[0];
        RVar r_minus_a = math::sub(r, a);
        RVar r_inv = math::div(math::ADTape::record(1.0, nullptr), r);
        RVar r2 = math::mul(r, r);
        RVar r3 = math::mul(r2, r);
        RVar sin_theta = math::sin(theta);
        RVar cos_theta = math::cos(theta);
        RVar sin2 = math::mul(sin_theta, sin_theta);

        RVar two_r = math::mul(math::ADTape::record(2.0, nullptr), r);
        RVar Gamma_t_tr = math::div(a, math::mul(two_r, r_minus_a));
        RVar Gamma_r_tt = math::div(math::mul(a, r_minus_a), math::mul(r3, math::ADTape::record(2.0, nullptr)));
        RVar Gamma_r_rr = math::neg(Gamma_t_tr);
        RVar Gamma_r_thetatheta = math::neg(r_minus_a);
        RVar Gamma_r_phiphi = math::neg(math::mul(r_minus_a, sin2));
        RVar Gamma_theta_rtheta = r_inv;
        RVar Gamma_theta_phiphi = math::neg(math::mul(sin_theta, cos_theta));
        RVar Gamma_phi_rphi = r_inv;
        RVar Gamma_phi_thetaphi = math::div(cos_theta, sin_theta);

        std::array<std::array<std::array<RVar, 4>, 4>, 4> Gamma{};
        for (int l = 0; l < 4; ++l)
            for (int m = 0; m < 4; ++m)
                for (int n = 0; n < 4; ++n)
                    Gamma[l][m][n] = math::ADTape::record(0.0, nullptr);

        Gamma[0][1][0] = Gamma[0][0][1] = Gamma_t_tr;
        Gamma[1][0][0] = Gamma_r_tt;
        Gamma[1][1][1] = Gamma_r_rr;
        Gamma[1][2][2] = Gamma_r_thetatheta;
        Gamma[1][3][3] = Gamma_r_phiphi;
        Gamma[2][1][2] = Gamma[2][2][1] = Gamma_theta_rtheta;
        Gamma[2][3][3] = Gamma_theta_phiphi;
        Gamma[3][1][3] = Gamma[3][3][1] = Gamma_phi_rphi;
        Gamma[3][2][3] = Gamma[3][3][2] = Gamma_phi_thetaphi;

        return Gamma;
    }

    std::array<std::array<std::array<double, 4>, 4>, 4> computeFullChristoffel(
        const Event4D& event
    ) const override {
        double x = event.x, y = event.y, z = event.z;
        double r = std::sqrt(x * x + y * y + z * z);
        const double minR = 1e-9;
        if (r < minR) r = minR;

        double r_minus_a = r - m_a;
        double r_inv = 1.0 / r;
        double r2 = r * r;
        double r3 = r2 * r;
        double sin_theta = std::sin(std::acos(std::max(-1.0, std::min(1.0, z / r))));
        double cos_theta = std::cos(std::acos(std::max(-1.0, std::min(1.0, z / r))));
        double sin2 = sin_theta * sin_theta;

        double Gamma_t_tr = m_a / (2.0 * r * r_minus_a);
        double Gamma_r_tt = m_a * r_minus_a / (2.0 * r3);
        double Gamma_r_rr = -Gamma_t_tr;
        double Gamma_r_thetatheta = -r_minus_a;
        double Gamma_r_phiphi = -r_minus_a * sin2;
        double Gamma_theta_rtheta = r_inv;
        double Gamma_theta_phiphi = -sin_theta * cos_theta;
        double Gamma_phi_rphi = r_inv;
        double Gamma_phi_thetaphi = cos_theta / sin_theta;

        std::array<std::array<std::array<double, 4>, 4>, 4> Gamma{};
        for (int l = 0; l < 4; ++l)
            for (int m = 0; m < 4; ++m)
                for (int n = 0; n < 4; ++n)
                    Gamma[l][m][n] = 0.0;

        Gamma[0][1][0] = Gamma[0][0][1] = Gamma_t_tr;
        Gamma[1][0][0] = Gamma_r_tt;
        Gamma[1][1][1] = Gamma_r_rr;
        Gamma[1][2][2] = Gamma_r_thetatheta;
        Gamma[1][3][3] = Gamma_r_phiphi;
        Gamma[2][1][2] = Gamma[2][2][1] = Gamma_theta_rtheta;
        Gamma[2][3][3] = Gamma_theta_phiphi;
        Gamma[3][1][3] = Gamma[3][3][1] = Gamma_phi_rphi;
        Gamma[3][2][3] = Gamma[3][3][2] = Gamma_phi_thetaphi;

        return Gamma;
    }

private:
    double m_a;
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

void test_generalized_adjoint_schwarzschild() {
    std::cout << "  Testing generalized adjoint with SchwarzschildMetric...\n";

    auto metric = std::make_shared<SchwarzschildMetric>(1.0);
    AdjointGeodesicIntegrator integrator(metric, {1.0}, 1e-6, 1e-10, 0.5, 0.9, 100000);

    Event4D start(0.0, 10.0, 0.0, 0.0);
    std::array<double, 4> vel = {1.0, -0.1, 0.0, 0.0};

    auto result = integrator.integrate(start, vel, GeodesicType::TIMELIKE, 0.01);

    assert(result.success && "Integration should succeed");
    assert(result.finalState.size() == 8 && "Final state should have 8 components");
    assert(!result.dState_dParams.empty() && "Gradient matrix should not be empty");
    assert(result.dState_dParams.size() == 8 && "Gradient matrix should have 8 rows");
    assert(result.dState_dParams[0].size() == 1 && "Gradient matrix should have 1 column (mass)");
    assert(std::isfinite(result.dState_dParams[1][0]) && "dr/dM should be finite");

    std::cout << "    dr/dM = " << result.dState_dParams[1][0] << "\n";

    AdjointGeodesicIntegrator legacy_integrator(1.0, 1e-6, 1e-10, 0.5, 0.9, 100000);
    auto legacy_result = legacy_integrator.integrate(start, vel, GeodesicType::TIMELIKE, 0.01);

    assert(std::fabs(legacy_result.finalState[1] - result.finalState[1]) < 1e-10 &&
           "Generalized and legacy integrators should agree on final state");
    assert(std::fabs(legacy_result.dState_dMass[1] - result.dState_dParams[1][0]) < 1e-10 &&
           "Generalized and legacy integrators should agree on gradient");
}

void test_generalized_adjoint_fr_gravity() {
    std::cout << "  Testing generalized adjoint with FRLGravity (no AD yet)...\n";

    std::map<std::string, double> param_map;
    param_map["alpha"] = 1.0;
    param_map["n"] = 1.0;

    std::unique_ptr<TheoryPlugin> plugin = std::make_unique<FRLGravityPlugin>(1.0, 1.0);

    MetricTensor metric = plugin->computeMetric(Event4D(0.0, 10.0, 0.0, 0.0), param_map);
    auto metric_ptr = std::make_shared<MetricTensor>(std::move(metric));

    std::vector<double> params = {1.0, 1.0};
    AdjointGeodesicIntegrator integrator(metric_ptr, params, 1e-6, 1e-10, 0.5, 0.9, 100000);

    Event4D start(0.0, 10.0, 0.0, 0.0);
    std::array<double, 4> vel = {1.0, -0.1, 0.0, 0.0};

    auto result = integrator.integrate(start, vel, GeodesicType::TIMELIKE, 0.01);

    assert(result.success && "Integration should succeed");
    assert(result.dState_dParams[1][0] == 0.0 &&
           "Gradient w.r.t. alpha should be zero (metric does not depend on alpha)");
    assert(result.dState_dParams[1][1] == 0.0 &&
           "Gradient w.r.t. n should be zero (metric does not depend on n)");

    std::cout << "    FRL gradients are zero as expected (no AD support yet).\n";
}

void test_generalized_adjoint_parameter_gradient() {
    std::cout << "  Testing generalized adjoint with mock parametric metric...\n";

    double a = 1.0;
    auto metric = std::make_shared<MockParametricMetric>(a);
    AdjointGeodesicIntegrator integrator(metric, {a}, 1e-6, 1e-10, 0.5, 0.9, 100000);

    Event4D start(0.0, 10.0, 0.0, 0.0);
    std::array<double, 4> vel = {1.0, -0.1, 0.0, 0.0};

    auto result = integrator.integrate(start, vel, GeodesicType::TIMELIKE, 0.01);

    assert(result.success && "Integration should succeed");
    assert(result.dState_dParams.size() == 8 && "Should have 8 state components");
    assert(result.dState_dParams[0].size() == 1 && "Should have 1 parameter");
    assert(std::isfinite(result.dState_dParams[1][0]) && "dr/da should be finite");

    double dr_da = result.dState_dParams[1][0];
    std::cout << "    dr/da = " << dr_da << "\n";

    // Validate against central finite differences
    double eps = 1e-6;
    AdjointGeodesicIntegrator integrator_plus(metric, {a + eps}, 1e-6, 1e-10, 0.5, 0.9, 100000);
    AdjointGeodesicIntegrator integrator_minus(metric, {a - eps}, 1e-6, 1e-10, 0.5, 0.9, 100000);

    auto result_plus = integrator_plus.integrate(start, vel, GeodesicType::TIMELIKE, 0.01);
    auto result_minus = integrator_minus.integrate(start, vel, GeodesicType::TIMELIKE, 0.01);

    double fd_dr_da = (result_plus.finalState[1] - result_minus.finalState[1]) / (2.0 * eps);
    std::cout << "    FD dr/da = " << fd_dr_da << "\n";

    assert(std::fabs(dr_da - fd_dr_da) < 1e-4 &&
           "Adjoint gradient should match finite difference");
}

int main() {
    std::cout << "=== GeneralizedAdjointTest ===\n";

    test_generalized_adjoint_schwarzschild();
    test_generalized_adjoint_fr_gravity();
    test_generalized_adjoint_parameter_gradient();

    std::cout << "All GeneralizedAdjointTest checks passed." << std::endl;
    return 0;
}
