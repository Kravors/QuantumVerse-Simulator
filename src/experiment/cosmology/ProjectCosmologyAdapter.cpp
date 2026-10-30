/**
 * @file ProjectCosmologyAdapter.cpp
 * @brief Adapter exposing the project's FLRW cosmology through CosmologyProvider.
 */

#include "ProjectCosmologyAdapter.h"

#include <cmath>
#include <stdexcept>

namespace quantumverse {
namespace {
constexpr double C_KMS       = 299792.458;   // km/s
constexpr double MPC_PER_M   = 3.2407792895e-23;
constexpr double MPC_M       = 1.0 / MPC_PER_M;
constexpr double SECONDS_PER_GYR = 1e9 * 365.25 * 86400.0;
constexpr double G_SI        = 6.67430e-11;
constexpr double A_RAD       = 7.5657e-16;
constexpr double C_SI        = 299792458.0;
constexpr double Z_DRAG      = 1059.94;

double E(double z, double Om, double Or, double Ok, double OL) {
    const double a = 1.0 / (1.0 + z);
    return std::sqrt(Om / (a * a * a) + Or / (a * a * a * a)
                     + Ok / (a * a) + OL);
}
} // namespace

ProjectCosmologyAdapter::ProjectCosmologyAdapter(double H0, double OmegaM,
                                                 double OmegaB, double OmegaR,
                                                 double OmegaLambda, double OmegaK,
                                                 double Tcmb0)
    : m_H0(H0), m_Om(OmegaM), m_Ob(OmegaB), m_Or(OmegaR),
      m_OL(OmegaLambda), m_Ok(OmegaK), m_Tcmb(Tcmb0)
{
    // Linkage to the project's FLRW code: construct an FRWMetric from the same
    // parameters so the adapter is tied to the project's cosmology module.
    const double H0_si = m_H0 * 1000.0 / MPC_M;
    try {
        if (std::abs(m_Om + m_OL - 1.0) <= 0.01) {
            m_frw = std::make_shared<FRWMetric>(
                FRWMetric::lambdaCDM(m_Om, m_OL, H0_si));
        } else {
            m_frw = std::make_shared<FRWMetric>(
                FRWMetric::matterDominated(H0_si));
        }
    } catch (const std::exception&) {
        m_frw.reset();
    }
}

double ProjectCosmologyAdapter::H(double z) const {
    const double H0 = m_frw ? m_frw->hubbleConstant() * MPC_M / 1000.0 : m_H0;
    const double Om = m_frw ? m_frw->omegaM() : m_Om;
    const double OL = m_frw ? m_frw->omegaLambda() : m_OL;
    return H0 * E(z, Om, m_Or, m_Ok, OL);
}

double ProjectCosmologyAdapter::comovingDistance(double z) const {
    // χ(z) = c ∫₀ᶻ dz' / H(z')
    const int steps = 8192;
    const double h = z / steps;
    auto f = [this](double zp) { return 1.0 / H(zp); };
    double s = f(0.0) + f(z);
    for (int i = 1; i < steps; ++i)
        s += (i & 1 ? 4.0 : 2.0) * f(i * h);
    return C_KMS * s * h / 3.0;
}

double ProjectCosmologyAdapter::luminosityDistance(double z) const {
    return (1.0 + z) * comovingDistance(z);
}

double ProjectCosmologyAdapter::angularDiameterDistance(double z) const {
    return comovingDistance(z) / (1.0 + z);
}

double ProjectCosmologyAdapter::distanceModulus(double z) const {
    return 5.0 * std::log10(luminosityDistance(z)) + 25.0;
}

double ProjectCosmologyAdapter::ageAtRedshift(double z) const {
    // At z=0, delegate to FRWMetric's bisection (bit-exact, seconds → Gyr).
    if (z == 0.0 && m_frw) return m_frw->ageOfUniverse() / SECONDS_PER_GYR;

    // For z > 0, first-principles a-space transform.
    // Reads Ω from FRWMetric when available to avoid value drift.
    const double a0 = 1.0 / (1.0 + z);
    const int steps = 8192;
    const double h = a0 / steps;
    const double Om = m_frw ? m_frw->omegaM() : m_Om;
    const double OL = m_frw ? m_frw->omegaLambda() : m_OL;
    const double Or = m_Or;
    const double Ok = m_Ok;
    auto f = [Om, Or, Ok, OL](double a) {
        const double denom = Or + Om * a + Ok * a * a + OL * a * a * a * a;
        return (denom > 0.0) ? a / std::sqrt(denom) : 0.0;
    };
    double s = f(0.0) + f(a0);
    for (int i = 1; i < steps; ++i)
        s += (i & 1 ? 4.0 : 2.0) * f(i * h);
    const double integral = s * h / 3.0;
    const double H0 = m_frw ? m_frw->hubbleConstant() : m_H0;
    const double H0_si = H0 * 1000.0 / MPC_M;
    return (integral / H0_si) / SECONDS_PER_GYR;
}

double ProjectCosmologyAdapter::lookbackTime(double z) const {
    return ageAtRedshift(0.0) - ageAtRedshift(z);
}

double ProjectCosmologyAdapter::decelerationParameter(double z) const {
    const double p1 = 1.0 + z;
    const double Om = m_frw ? m_frw->omegaM() : m_Om;
    const double OL = m_frw ? m_frw->omegaLambda() : m_OL;
    const double Om_z = Om * p1 * p1 * p1;
    const double Or_z = m_Or * p1 * p1 * p1 * p1;
    const double denom = Om_z + Or_z + m_Ok * p1 * p1 + OL;
    if (denom <= 0.0) return 0.0;
    return (0.5 * Om_z + Or_z - OL) / denom;
}

double ProjectCosmologyAdapter::soundHorizon() const {
    const double T = m_Tcmb;
    const double rhoGamma = A_RAD * T * T * T * T / (C_SI * C_SI);
    const double H0 = m_frw ? m_frw->hubbleConstant() * MPC_M / 1000.0 : m_H0;
    const double H0_si = H0 * 1000.0 / MPC_M;
    const double rhoCrit = 3.0 * H0_si * H0_si / (8.0 * M_PI * G_SI);
    const double OmegaGamma = (rhoCrit > 0.0) ? rhoGamma / rhoCrit : 0.0;
    const double R0 = (OmegaGamma > 0.0) ? 3.0 * m_Ob / (4.0 * OmegaGamma) : 0.0;

    const double zmax = 1e6;
    const int steps = 16384;
    const double h = (zmax - Z_DRAG) / steps;
    auto integrand = [this, &R0](double z) -> double {
        const double R = R0 / (1.0 + z);
        const double c_s = C_KMS / std::sqrt(3.0 * (1.0 + R));
        return c_s / H(z);
    };
    double s = integrand(Z_DRAG) + integrand(zmax);
    for (int i = 1; i < steps; ++i)
        s += (i & 1 ? 4.0 : 2.0) * integrand(Z_DRAG + i * h);
    double rd = s * h / 3.0;
    const double tail = (m_Or > 0.0)
        ? (C_KMS / std::sqrt(3.0)) / (m_H0 * std::sqrt(m_Or)) / (1.0 + zmax)
        : 0.0;
    return rd + tail;
}

double ProjectCosmologyAdapter::growthFactor(double z) const {
    const double p1 = 1.0 + z;
    const double Om = m_frw ? m_frw->omegaM() : m_Om;
    const double OL = m_frw ? m_frw->omegaLambda() : m_OL;
    const double Om_z = Om * p1 * p1 * p1;
    const double OL_z = OL;
    const double a = 1.0 / p1;
    const double numerator = a * (5.0 * Om_z / 2.0);
    const double denom = std::pow(Om_z, 4.0 / 7.0) - OL_z
        + (1.0 + Om_z / 2.0) * (1.0 + OL_z / 70.0);
    if (denom == 0.0) return 0.0;
    return numerator / denom;
}

double ProjectCosmologyAdapter::criticalDensity() const {
    const double H0 = m_frw ? m_frw->hubbleConstant() : m_H0;
    const double H0_si = H0 * 1000.0 / MPC_M;
    return 3.0 * H0_si * H0_si / (8.0 * M_PI * G_SI);
}

} // namespace quantumverse
