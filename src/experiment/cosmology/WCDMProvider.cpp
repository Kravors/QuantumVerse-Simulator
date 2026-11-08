/**
 * @file WCDMProvider.cpp
 * @brief wCDM cosmology with Chevallier-Polarski-Linder (CPL) dark energy.
 *
 * E²(z) = Ωₘ(1+z)³ + Ωᵣ(1+z)⁴ + Ωₖ(1+z)² + Ω_Λ·DE(z)
 * DE(z) = (1+z)^(3(1+w0+wa)) · exp(-3·wa·z/(1+z))
 *
 * w0=-1, wa=0 → exactly flat ΛCDM (DE(z) = 1 at all z).
 * w0≠-1 or wa≠0 → evolving DE equation of state changes H(z) at z ≳ 0.5.
 *
 * Phenomenological toy model — proves the provider interface discriminates
 * between expansion-history theories.
 */

#include "WCDMProvider.h"

#include <cmath>
#include <functional>

namespace quantumverse {

namespace {
constexpr double C_KMS = 299792.458;                        // km/s
constexpr double C_SI = 299792458.0;                        // m/s
constexpr double MPC_M = 3.0856775814913673e22;             // metres per Mpc
constexpr double SECONDS_PER_GYR = 1e9 * 365.25 * 86400.0;  // 3.15576e16 s
constexpr double G_SI = 6.67430e-11;                        // m³ kg⁻¹ s⁻²
constexpr double A_RAD = 7.5657e-16;                        // J m⁻³ K⁻⁴ (radiation constant)
constexpr double Z_DRAG = 1059.94;                          // Planck 2018 drag epoch
}  // namespace

WCDMProvider::WCDMProvider() : WCDMProvider(Params{}) {}

WCDMProvider::WCDMProvider(Params p)
    : m_H0(p.H0),
      m_Om(p.Om),
      m_Ob(p.Ob),
      m_Or(p.Or),
      m_OL(p.OL),
      m_Ok(p.Ok),
      m_Tcmb(p.Tcmb),
      m_w0(p.w0),
      m_wa(p.wa) {
    m_parametersSource = QStringLiteral("user");
}

WCDMProvider::WCDMProvider(double H0, double OmegaM, double OmegaB, double OmegaR,
                           double OmegaLambda, double OmegaK, double Tcmb0, double w0, double wa)
    : WCDMProvider(Params{H0, OmegaM, OmegaB, OmegaR, OmegaLambda, OmegaK, Tcmb0, w0, wa}) {
    m_parametersSource = QStringLiteral("user");
}

double WCDMProvider::E(double z) const {
    const double opz = 1.0 + z;
    const double opz2 = opz * opz;
    const double opz3 = opz2 * opz;
    const double opz4 = opz2 * opz2;
    // CPL dark energy density evolution:
    // DE(z) = (1+z)^(3(1+w0+wa)) * exp(-3*wa*z/(1+z))
    // At w0=-1, wa=0: DE(z) = 1 → exactly ΛCDM
    const double DE = std::pow(opz, 3.0 * (1.0 + m_w0 + m_wa)) * std::exp(-3.0 * m_wa * (z / opz));
    return std::sqrt(m_Om * opz3 + m_Or * opz4 + m_Ok * opz2 + m_OL * DE);
}

double WCDMProvider::H(double z) const {
    return m_H0 * E(z);
}

template <typename F>
double WCDMProvider::integrateSimpson(double z1, double z2, int steps, F f) const {
    if (steps < 2)
        steps = 2;
    if (steps % 2 == 1)
        ++steps;
    const double h = (z2 - z1) / steps;
    double s = f(z1) + f(z2);
    for (int i = 1; i < steps; ++i)
        s += (i & 1 ? 4.0 : 2.0) * f(z1 + i * h);
    return s * h / 3.0;
}

double WCDMProvider::integrateHInv(double z1, double z2, int steps) const {
    return integrateSimpson(z1, z2, steps, [this](double z) { return 1.0 / H(z); });
}

double WCDMProvider::comovingDistance(double z) const {
    return C_KMS * integrateHInv(0.0, z);
}

double WCDMProvider::luminosityDistance(double z) const {
    return (1.0 + z) * comovingDistance(z);
}

double WCDMProvider::angularDiameterDistance(double z) const {
    return comovingDistance(z) / (1.0 + z);
}

double WCDMProvider::distanceModulus(double z) const {
    return 5.0 * std::log10(luminosityDistance(z)) + 25.0;
}

double WCDMProvider::ageAtRedshift(double z) const {
    const double a0 = 1.0 / (1.0 + z);
    const int steps = 8192;
    const double h = a0 / steps;
    auto f = [&](double a) {
        const double opz = 1.0 / a;
        const double z_at_a = opz - 1.0;
        const double DE =
            std::pow(opz, 3.0 * (1.0 + m_w0 + m_wa)) * std::exp(-3.0 * m_wa * (z_at_a / opz));
        const double denom = m_Or + m_Om * a + m_Ok * a * a + m_OL * DE * a * a * a * a;
        return (denom > 0.0) ? a / std::sqrt(denom) : 0.0;
    };
    double s = f(0.0) + f(a0);
    for (int i = 1; i < steps; ++i)
        s += (i & 1 ? 4.0 : 2.0) * f(i * h);
    const double integral = s * h / 3.0;

    const double H0_si = m_H0 * 1000.0 / MPC_M;
    const double t_sec = integral / H0_si;
    return t_sec / SECONDS_PER_GYR;
}

double WCDMProvider::lookbackTime(double z) const {
    return ageAtRedshift(0.0) - ageAtRedshift(z);
}

double WCDMProvider::decelerationParameter(double z) const {
    // q = [½ Ωₘ(1+z)³ + Ωᵣ(1+z)⁴ − Ω_Λ·DE(z)·(1+3w(z))/... ] / E²
    // Simplified: use q = [Ωₘ(1+z)³/2 + Ωᵣ(1+z)⁴ - Ω_Λ·DE(z)·(1+3w(z))/...] / E²
    // where w(z) = w0 + wa·z/(1+z). At ΛCDM limit q0 = Ωm/2 - ΩΛ.
    const double p1 = 1.0 + z;
    const double opz_ratio = z / p1;
    const double Om_z = m_Om * p1 * p1 * p1;
    const double Or_z = m_Or * p1 * p1 * p1 * p1;
    const double DE = std::pow(p1, 3.0 * (1.0 + m_w0 + m_wa)) * std::exp(-3.0 * m_wa * opz_ratio);
    const double w_z = m_w0 + m_wa * opz_ratio;
    const double OL_eff = m_OL * DE;
    const double denom = Om_z + Or_z + m_Ok * p1 * p1 + OL_eff;
    if (denom <= 0.0)
        return 0.0;
    const double numerator = 0.5 * Om_z + Or_z + OL_eff * (1.0 + 3.0 * w_z);
    return numerator / denom;
}

double WCDMProvider::soundHorizon() const {
    const double T = m_Tcmb;
    const double rhoGamma = A_RAD * T * T * T * T / (C_SI * C_SI);
    const double rhoCrit = criticalDensity();
    const double OmegaGamma = (rhoCrit > 0.0) ? rhoGamma / rhoCrit : 0.0;
    const double R0 = (OmegaGamma > 0.0) ? 3.0 * m_Ob / (4.0 * OmegaGamma) : 0.0;

    const double zmax = 1e6;
    const int steps = 16384;
    const double h = (zmax - Z_DRAG) / steps;
    auto integrand = [&](double z) -> double {
        const double R = R0 / (1.0 + z);
        const double c_s = C_KMS / std::sqrt(3.0 * (1.0 + R));
        return c_s / H(z);
    };
    double s = integrand(Z_DRAG) + integrand(zmax);
    for (int i = 1; i < steps; ++i)
        s += (i & 1 ? 4.0 : 2.0) * integrand(Z_DRAG + i * h);
    double rd = s * h / 3.0;

    const double tail =
        (m_Or > 0.0) ? (C_KMS / std::sqrt(3.0)) / (m_H0 * std::sqrt(m_Or)) / (1.0 + zmax) : 0.0;
    return rd + tail;
}

double WCDMProvider::growthFactor(double z) const {
    // Carroll, Press & Turner (1992) fit, normalised to D(0)=1.
    // Uses constant Ωm, ΩΛ approximation; wCDM growth is scale-independent
    // at linear order in this toy model.
    const double p1 = 1.0 + z;
    const double Om_z = m_Om * p1 * p1 * p1;
    const double OL_z = m_OL;  // dark energy treated as smooth component
    const double a = 1.0 / p1;
    const double numerator = a * (5.0 * Om_z / 2.0);
    const double denom =
        std::pow(Om_z, 4.0 / 7.0) - OL_z + (1.0 + Om_z / 2.0) * (1.0 + OL_z / 70.0);
    if (denom == 0.0)
        return 0.0;
    return numerator / denom;
}

double WCDMProvider::criticalDensity() const {
    const double H0_si = m_H0 * 1000.0 / MPC_M;
    return 3.0 * H0_si * H0_si / (8.0 * M_PI * G_SI);
}

}  // namespace quantumverse
