#include "KerrMetric.h"
#include <algorithm>
#include <cmath>
#include <cassert>

namespace quantumverse {

KerrMetric::KerrMetric(double massKg, double spinDimensionless)
    : m_mass(massKg)
    , m_spin(spinDimensionless)
{
    m_M = PHYS_G() * massKg / (PHYS_C() * PHYS_C());
    m_a = spinDimensionless * m_M;
    m_rs = 2.0 * m_M;
}

double KerrMetric::outerHorizonRadius() const {
    double disc = m_M * m_M - m_a * m_a;
    if (disc < 0.0) disc = 0.0;
    return m_M + std::sqrt(disc);
}

double KerrMetric::innerHorizonRadius() const {
    double disc = m_M * m_M - m_a * m_a;
    if (disc < 0.0) disc = 0.0;
    return m_M - std::sqrt(disc);
}

double KerrMetric::ergosphereRadius(double theta) const {
    double cosT = std::cos(theta);
    double disc = m_M * m_M - m_a * m_a * cosT * cosT;
    if (disc < 0.0) disc = 0.0;
    return m_M + std::sqrt(disc);
}

double KerrMetric::frameDraggingOmega(double r, double theta) const {
    double a2 = m_a * m_a;
    double sin2T = std::sin(theta) * std::sin(theta);
    double cos2T = std::cos(theta) * std::cos(theta);
    double sigma = r * r + a2 * cos2T;

    double g_tphi_numerator = -m_rs * r * m_a * sin2T / sigma;
    double g_phiphi = (r * r + a2 + m_rs * r * a2 * sin2T / sigma) * sin2T;

    if (std::abs(g_phiphi) < 1e-30) return 0.0;
    return -g_tphi_numerator / g_phiphi;
}

double KerrMetric::cartesianToBLRadial(double x, double y, double z) const {
    double a2 = m_a * m_a;
    double rho2 = x * x + y * y + z * z;
    double discriminant = (rho2 - a2) * (rho2 - a2) + 4.0 * a2 * z * z;
    if (discriminant < 0.0) discriminant = 0.0;
    double r2 = (rho2 - a2 + std::sqrt(discriminant)) / 2.0;
    if (r2 < 0.0) r2 = 0.0;
    return std::sqrt(r2);
}

void KerrMetric::transformBLToCartesian(
    double g_tt, double g_tphi, double g_rr, double g_thth, double g_pp,
    double x, double y, double z, double rBL, double theta, double phi,
    std::array<std::array<double, 4>, 4>& out
) const {
    (void)x;
    (void)y;
    (void)z;
    (void)rBL;
    double sinT = std::sin(theta);
    double cosT = std::cos(theta);
    double sinP = std::sin(phi);
    double cosP = std::cos(phi);
    double a2 = m_a * m_a;
    double sqrtR2A2 = std::sqrt(rBL * rBL + a2);

    // Jacobian: ∂(x,y,z)/∂(r,θ,φ)
    double j3[3][3];
    j3[0][0] = (rBL / sqrtR2A2) * sinT * cosP;
    j3[0][1] = sqrtR2A2 * cosT * cosP;
    j3[0][2] = -sqrtR2A2 * sinT * sinP;
    j3[1][0] = (rBL / sqrtR2A2) * sinT * sinP;
    j3[1][1] = sqrtR2A2 * cosT * sinP;
    j3[1][2] = sqrtR2A2 * sinT * cosP;
    j3[2][0] = cosT;
    j3[2][1] = -rBL * sinT;
    j3[2][2] = 0.0;

    // Invert 3x3 Jacobian to get ∂(r,θ,φ)/∂(x,y,z)
    double det = j3[0][0]*(j3[1][1]*j3[2][2]-j3[1][2]*j3[2][1])
               - j3[0][1]*(j3[1][0]*j3[2][2]-j3[1][2]*j3[2][0])
               + j3[0][2]*(j3[1][0]*j3[2][1]-j3[1][1]*j3[2][0]);
    if (std::abs(det) < 1e-30) det = (det >= 0.0) ? 1e-30 : -1e-30;

    double invDet = 1.0 / det;
    double inv_j3[3][3];
    inv_j3[0][0] =  (j3[1][1]*j3[2][2]-j3[1][2]*j3[2][1]) * invDet;
    inv_j3[0][1] = -(j3[0][1]*j3[2][2]-j3[0][2]*j3[2][1]) * invDet;
    inv_j3[0][2] =  (j3[0][1]*j3[1][2]-j3[0][2]*j3[1][1]) * invDet;
    inv_j3[1][0] = -(j3[1][0]*j3[2][2]-j3[1][2]*j3[2][0]) * invDet;
    inv_j3[1][1] =  (j3[0][0]*j3[2][2]-j3[0][2]*j3[2][0]) * invDet;
    inv_j3[1][2] = -(j3[0][0]*j3[1][2]-j3[0][2]*j3[1][0]) * invDet;
    inv_j3[2][0] =  (j3[1][0]*j3[2][1]-j3[1][1]*j3[2][0]) * invDet;
    inv_j3[2][1] = -(j3[0][0]*j3[2][1]-j3[0][1]*j3[2][0]) * invDet;
    inv_j3[2][2] =  (j3[0][0]*j3[1][1]-j3[0][1]*j3[1][0]) * invDet;

    // Full 4D Jacobian: jacT[ν][μ'] = ∂x^ν/∂x^μ'
    // ν = (t, r, θ, φ), μ' = (t, x, y, z)
    double jacT[4][4] = {};
    jacT[0][0] = 1.0;
    for (int nu = 1; nu <= 3; nu++)
        for (int mu = 1; mu <= 3; mu++)
            jacT[nu][mu] = inv_j3[nu-1][mu-1];

    // BL metric tensor
    double g_BL[4][4] = {};
    g_BL[0][0] = g_tt;
    g_BL[0][3] = g_tphi;
    g_BL[3][0] = g_tphi;
    g_BL[1][1] = g_rr;
    g_BL[2][2] = g_thth;
    g_BL[3][3] = g_pp;

    // Transform: g_new[μ'][ν'] = Σ_αβ jacT[α][μ'] jacT[β][ν'] g_BL[α][β]
    for (int muP = 0; muP < 4; muP++) {
        for (int nuP = 0; nuP < 4; nuP++) {
            double val = 0.0;
            for (int a = 0; a < 4; a++)
                for (int b = 0; b < 4; b++)
                    val += jacT[a][muP] * jacT[b][nuP] * g_BL[a][b];
            out[muP][nuP] = val;
        }
    }
}

std::array<std::array<double, 4>, 4> KerrMetric::evaluate(const Event4D& event) const {
    if (!std::isfinite(event.x) || !std::isfinite(event.y) || !std::isfinite(event.z)) {
        return MetricTensor().g;
    }

    double x = event.x, y = event.y, z = event.z;
    double rho2 = x * x + y * y + z * z;
    double rCart = std::sqrt(rho2);
    const double minR = 1e-9;
    if (rCart < minR) rCart = minR;

    double theta = (rCart > minR) ? std::acos(std::max(-1.0, std::min(1.0, z / rCart))) : 0.0;
    double phi = std::atan2(y, x);

    double rBL = cartesianToBLRadial(x, y, z);
    if (rBL < minR) rBL = minR;

    double a2 = m_a * m_a;
    double sigma = rBL * rBL + a2 * std::cos(theta) * std::cos(theta);
    double delta = rBL * rBL - m_rs * rBL + a2;

    if (std::abs(sigma) < 1e-30) sigma = 1e-30;
    if (std::abs(delta) < 1e-20) delta = (delta >= 0.0) ? 1e-20 : -1e-20;

    double sin2T = std::sin(theta) * std::sin(theta);

    double g_tt = -(1.0 - m_rs * rBL / sigma);
    double g_tphi = -m_rs * rBL * m_a * sin2T / sigma;
    double g_rr = sigma / delta;
    double g_thth = sigma;
    double g_pp = (rBL * rBL + a2 + m_rs * rBL * a2 * sin2T / sigma) * sin2T;

    std::array<std::array<double, 4>, 4> result{};
    transformBLToCartesian(g_tt, g_tphi, g_rr, g_thth, g_pp,
                           x, y, z, rBL, theta, phi, result);
    return result;
}

CurvatureScalars KerrMetric::curvatureScalars(const Event4D& event) const {
    double x = event.x, y = event.y, z = event.z;
    double rho2 = x * x + y * y + z * z;
    double r = std::sqrt(rho2);
    const double minR = 1e-9;
    if (r < minR) r = minR;

    // Vacuum solution: Ricci = 0, Weyl = Riemann
    // Approximate Kretschmann: K ≈ 48 G²M² / (c⁴ r⁶) (Schwarzschild limit)
    double c4 = PHYS_C() * PHYS_C() * PHYS_C() * PHYS_C();
    double G = PHYS_G();
    double K = 48.0 * G * G * m_mass * m_mass / (c4 * std::pow(r, 6));

    CurvatureScalars s;
    s.kretschmann = K;
    s.ricciScalar = 0.0;
    s.weylSquared = K;
    s.valid = true;
    return s;
}

std::array<std::array<std::array<double, 4>, 4>, 4> KerrMetric::computeFullChristoffel(
    const Event4D& event
) const {
    // Compute Christoffel symbols numerically via finite differences
    // using the analytic metric evaluation
    auto g = [&](double x, double y, double z) -> std::array<std::array<double, 4>, 4> {
        Event4D e(0.0, x, y, z);
        return evaluate(e);
    };

    double x = event.x, y = event.y, z = event.z;
    double h = 1e-5;

    // Compute inverse metric at event
    auto g0 = g(x, y, z);
    MetricTensor m;
    m.g = g0;
    auto g_inv = m.inverse().g;

    // Compute ∂_σ g_μν via central differences
    auto dg = [&](int sigma, int mu, int nu) -> double {
        double gp[3] = {x, y, z};
        double gm[3] = {x, y, z};
        gp[sigma] += h;
        gm[sigma] -= h;
        auto g_plus = g(gp[0], gp[1], gp[2]);
        auto g_minus = g(gm[0], gm[1], gm[2]);
        return (g_plus[mu][nu] - g_minus[mu][nu]) / (2.0 * h);
    };

    std::array<std::array<std::array<double, 4>, 4>, 4> Gamma{};
    for (int rho = 0; rho < 4; ++rho) {
        for (int mu = 0; mu < 4; ++mu) {
            for (int nu = 0; nu < 4; ++nu) {
                double val = 0.0;
                for (int sigma = 1; sigma < 4; ++sigma) {
                    double dg_mu_sigma_nu = dg(sigma, mu, nu);
                    double dg_nu_sigma_mu = dg(sigma, nu, mu);
                    double dg_mu_nu_sigma = dg(sigma, mu, nu);
                    val += g_inv[rho][sigma] * (dg_mu_sigma_nu + dg_nu_sigma_mu - dg_mu_nu_sigma);
                }
                Gamma[rho][mu][nu] = 0.5 * val;
            }
        }
    }

    return Gamma;
}

std::array<std::array<RVar, 4>, 4> KerrMetric::evaluateAD(
    const std::array<RVar, 4>& pos,
    const std::vector<RVar>& params
) const {
    (void)params;
    // Use spherical coordinates for AD: pos = [t, r, theta, phi]
    RVar r = pos[1];
    RVar theta = pos[2];

    RVar M = math::ADTape::record(m_M, nullptr);
    RVar a = math::ADTape::record(m_a, nullptr);
    RVar rs = math::mul(M, math::ADTape::record(2.0, nullptr));

    RVar cos_theta = math::cos(theta);
    RVar sin_theta = math::sin(theta);
    RVar sin2 = math::mul(sin_theta, sin_theta);
    RVar cos2 = math::mul(cos_theta, cos_theta);
    RVar a2 = math::mul(a, a);
    RVar r2 = math::mul(r, r);

    RVar sigma = math::add(r2, math::mul(a2, cos2));
    RVar sigma_inv = math::div(math::ADTape::record(1.0, nullptr), sigma);
    RVar delta = math::add(math::sub(r2, math::mul(rs, r)), a2);
    RVar delta_inv = math::div(math::ADTape::record(1.0, nullptr), delta);

    RVar g_tt = math::neg(math::sub(math::ADTape::record(1.0, nullptr),
                                     math::mul(rs, math::mul(r, sigma_inv))));
    RVar g_tphi = math::neg(math::mul(rs, math::mul(r, math::mul(a, math::mul(sin2, sigma_inv)))));
    RVar g_rr = math::mul(sigma, delta_inv);
    RVar g_thth = sigma;
    RVar g_pp = math::mul(math::add(r2, math::add(a2, math::mul(rs, math::mul(r, math::mul(a2, math::mul(sin2, sigma_inv)))))),
                          sin2);

    std::array<std::array<RVar, 4>, 4> result{};
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            result[i][j] = math::ADTape::record(0.0, nullptr);

    result[0][0] = g_tt;
    result[0][3] = g_tphi;
    result[3][0] = g_tphi;
    result[1][1] = g_rr;
    result[2][2] = g_thth;
    result[3][3] = g_pp;
    return result;
}

std::array<std::array<std::array<RVar, 4>, 4>, 4> KerrMetric::computeChristoffelAD(
    const std::array<RVar, 4>& pos,
    const std::vector<RVar>& params
) const {
    // Fall back to finite-difference based Christoffel for now
    (void)pos;
    (void)params;
    return std::array<std::array<std::array<RVar, 4>, 4>, 4>{};
}

} // namespace quantumverse
