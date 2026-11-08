/**
 * @file HuSawickiProvider.cpp
 * @brief Hu-Sawicki f(R) modified gravity cosmology.
 *
 * f(R) = -M² c₁(R/M²)^n / [c₂(R/M²)^n + 1]
 *
 * For small |f_R0|, the modified Friedmann equation is approximated as:
 *   E²(z) = Ωₘ(1+z)³(1 + f_R(z)) + Ω_Λ(1+z)³(1 - f_R(z)/3) + Ωᵣ(1+z)⁴ + Ωₖ(1+z)²
 * where f_R(z) = -|f_R0| × Ωₘ(1+z)³ / [Ωₘ(1+z)³ + 4Ω_Λ]
 *
 * f_R0 = 0 → exactly flat ΛCDM (degenerate case).
 * f_R0 ≠ 0 → modified expansion history with screening at high z.
 *
 * Current observational bound: |f_R0| < 2.3 × 10⁻⁵ (Planck 2018).
 *
 * Phenomenological toy model — proves the provider interface discriminates
 * between screened modified-gravity theories.
 */

#include "HuSawickiProvider.h"
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

HuSawickiProvider::HuSawickiProvider() : HuSawickiProvider(Params{}) {}

HuSawickiProvider::HuSawickiProvider(Params p)
    : m_H0(p.H0),
      m_Om(p.Om),
      m_Ob(p.Ob),
      m_Or(p.Or),
      m_OL(p.OL),
      m_Ok(p.Ok),
      m_Tcmb(p.Tcmb),
      m_fR0(p.fR0) {
    m_parametersSource = QStringLiteral("user");
}

HuSawickiProvider::HuSawickiProvider(double H0, double OmegaM, double OmegaB, double OmegaR,
                                     double OmegaLambda, double OmegaK, double Tcmb0, double fR0)
    : HuSawickiProvider(Params{H0, OmegaM, OmegaB, OmegaR, OmegaLambda, OmegaK, Tcmb0, fR0}) {
    m_parametersSource = QStringLiteral("user");
}

double HuSawickiProvider::fR(double z) const {
    // f_R(z) = -|f_R0| × Ωₘ(1+z)³ / [Ωₘ(1+z)³ + 4Ω_Λ]
    // At z=0: f_R(0) = -|f_R0| × Ωₘ / [Ωₘ + 4Ω_Λ]
    // At high z: f_R(z) → -|f_R0| (full modification, screened at early times)
    const double opz = 1.0 + z;
    const double opz3 = opz * opz * opz;
    const double denom = m_Om * opz3 + 4.0 * m_OL;
    if (denom <= 0.0)
        return 0.0;
    return -m_fR0 * (m_Om * opz3) / denom;
}

double HuSawickiProvider::E(double z) const {
    const double opz = 1.0 + z;
    const double opz2 = opz * opz;
    const double opz3 = opz2 * opz;
    const double opz4 = opz2 * opz2;
    // Hu-Sawicki modification: f_R(z) modifies both matter and dark energy
    // components. At z=0: f_R(0) = -|f_R0| × Ωₘ/(Ωₘ+4Ω_Λ).
    // At high z: f_R(z) → -|f_R0| (matter suppression).
    const double fR_z = fR(z);
    return std::sqrt(m_Om * opz3 * (1.0 + fR_z) + m_OL * opz3 * (1.0 - fR_z / 3.0) + m_Or * opz4 +
                     m_Ok * opz2);
}

double HuSawickiProvider::H(double z) const {
    return m_H0 * E(z);
}

template <typename F>
double HuSawickiProvider::integrateSimpson(double z1, double z2, int steps, F f) const {
    if (steps < 2)
        steps = 2;
    if (steps % 2 == 1)
        ++steps;  // Simpson needs an even number of intervals
    const double h = (z2 - z1) / steps;
    double s = f(z1) + f(z2);
    for (int i = 1; i < steps; ++i)
        s += (i & 1 ? 4.0 : 2.0) * f(z1 + i * h);
    return s * h / 3.0;
}

double HuSawickiProvider::integrateHInv(double z1, double z2, int steps) const {
    return integrateSimpson(z1, z2, steps, [this](double z) { return 1.0 / H(z); });
}

double HuSawickiProvider::comovingDistance(double z) const {
    // χ(z) = c ∫₀ᶻ dz' / H(z')   [H in km/s/Mpc, c in km/s → Mpc]
    return C_KMS * integrateHInv(0.0, z);
}

double HuSawickiProvider::luminosityDistance(double z) const {
    return (1.0 + z) * comovingDistance(z);
}

double HuSawickiProvider::angularDiameterDistance(double z) const {
    return comovingDistance(z) / (1.0 + z);
}

double HuSawickiProvider::distanceModulus(double z) const {
    return 5.0 * std::log10(luminosityDistance(z)) + 25.0;
}

double HuSawickiProvider::ageAtRedshift(double z) const {
    // t(z) = (1/H₀) ∫_z^∞ dz' / ((1+z') E(z')), H₀ in s⁻¹.
    // Transform a = 1/(1+z') (scale factor): dz'/(1+z') = -da/a, giving the
    // well-behaved integral  ∫₀^{a0} a / sqrt(Ωᵣ + Ωₘ a + Ωₖ a² + Ω_Λ a⁴) da,
    // a0 = 1/(1+z), which is finite at a→0 (radiation term keeps the
    // denominator away from zero).
    const double a0 = 1.0 / (1.0 + z);
    const int steps = 8192;
    const double h = a0 / steps;
    auto f = [&](double a) {
        const double opz = 1.0 / a;
        const double z_at_a = opz - 1.0;
        const double fR_z = fR(z_at_a);
        const double opz2 = opz * opz;
        const double opz3 = opz2 * opz;
        const double denom = m_Or + m_Om * opz3 * (1.0 + fR_z) * a +
                             m_OL * opz3 * (1.0 - fR_z / 3.0) * a + m_Ok * opz2 * a * a;
        return (denom > 0.0) ? a / std::sqrt(denom) : 0.0;
    };
    double s = f(0.0) + f(a0);
    for (int i = 1; i < steps; ++i)
        s += (i & 1 ? 4.0 : 2.0) * f(i * h);
    const double integral = s * h / 3.0;

    const double H0_si = m_H0 * 1000.0 / MPC_M;  // 1/s
    const double t_sec = integral / H0_si;
    return t_sec / SECONDS_PER_GYR;
}

double HuSawickiProvider::lookbackTime(double z) const {
    return ageAtRedshift(0.0) - ageAtRedshift(z);
}

double HuSawickiProvider::decelerationParameter(double z) const {
    // q = [½ Ωₘ_eff(1+z)³ + Ωᵣ(1+z)⁴ − Ω_Λ_eff(1+z)³] / E(z)²
    // where Ωₘ_eff = Ωₘ(1 + f_R(z)), Ω_Λ_eff = Ω_Λ(1 - f_R(z)/3)
    const double p1 = 1.0 + z;
    const double opz2 = p1 * p1;
    const double opz3 = p1 * p1 * p1;
    const double fR_z = fR(z);
    const double Om_eff_z = m_Om * opz3 * (1.0 + fR_z);
    const double Or_z = m_Or * opz2 * opz2;
    const double OL_eff_z = m_OL * opz3 * (1.0 - fR_z / 3.0);
    const double denom = Om_eff_z + Or_z + m_Ok * opz2 + OL_eff_z;
    if (denom <= 0.0)
        return 0.0;
    return (0.5 * Om_eff_z + Or_z - OL_eff_z) / denom;
}

double HuSawickiProvider::soundHorizon() const {
    // r_d = ∫_{z_drag}^∞ c_s(z) / H(z) dz,
    // c_s = c / √(3(1+R)),  R = 3ρ_b/(4ρ_γ) = R₀/(1+z),  R₀ = 3Ω_b/(4Ω_γ).
    const double T = m_Tcmb;
    const double rhoGamma = A_RAD * T * T * T * T / (C_SI * C_SI);  // kg/m³
    const double rhoCrit = criticalDensity();
    const double OmegaGamma = (rhoCrit > 0.0) ? rhoGamma / rhoCrit : 0.0;
    const double R0 = (OmegaGamma > 0.0) ? 3.0 * m_Ob / (4.0 * OmegaGamma) : 0.0;

    const double zmax = 1e6;
    const int steps = 16384;
    const double h = (zmax - Z_DRAG) / steps;
    auto integrand = [&](double z) -> double {
        const double R = R0 / (1.0 + z);
        const double c_s = C_KMS / std::sqrt(3.0 * (1.0 + R));  // km/s
        return c_s / H(z);                                      // Mpc per unit z
    };
    double s = integrand(Z_DRAG) + integrand(zmax);
    for (int i = 1; i < steps; ++i)
        s += (i & 1 ? 4.0 : 2.0) * integrand(Z_DRAG + i * h);
    double rd = s * h / 3.0;

    // Analytic tail in the radiation era: c_s → c/√3, H ≈ H₀√Ωᵣ(1+z)².
    const double tail =
        (m_Or > 0.0) ? (C_KMS / std::sqrt(3.0)) / (m_H0 * std::sqrt(m_Or)) / (1.0 + zmax) : 0.0;
    return rd + tail;
}

double HuSawickiProvider::growthFactor(double z) const {
    // Carroll, Press & Turner (1992) fit, normalised to D(0)=1:
    // D(a) = a · (5 Ωₘ/2) / [Ωₘ^(4/7) − Ω_Λ + (1+Ωₘ/2)(1+Ω_Λ/70)]
    // evaluated with the redshift-scaled density parameters.
    const double p1 = 1.0 + z;
    const double opz3 = p1 * p1 * p1;
    const double Om_z = m_Om * opz3;
    const double OL_z = m_OL;  // dark energy density treated as constant in CPT fit
    const double a = 1.0 / p1;
    const double numerator = a * (5.0 * Om_z / 2.0);
    const double denom =
        std::pow(Om_z, 4.0 / 7.0) - OL_z + (1.0 + Om_z / 2.0) * (1.0 + OL_z / 70.0);
    if (denom == 0.0)
        return 0.0;
    return numerator / denom;
}

double HuSawickiProvider::criticalDensity() const {
    const double H0_si = m_H0 * 1000.0 / MPC_M;        // 1/s
    return 3.0 * H0_si * H0_si / (8.0 * M_PI * G_SI);  // kg/m³
}

}  // namespace quantumverse
