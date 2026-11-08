#pragma once

/**
 * @file HuSawickiProvider.h
 * @brief Hu-Sawicki f(R) modified gravity cosmology.
 *
 * Hu-Sawicki f(R) model:
 *   f(R) = -M² c₁(R/M²)^n / [c₂(R/M²)^n + 1]
 *
 * Parameterized by the present-day deviation parameter |f_R0| = |df_R/dR|₀.
 * Current observational bound: |f_R0| < 2.3 × 10⁻⁵ (Planck 2018).
 *
 * For small |f_R0|, the modified Friedmann equation is approximated as:
 *   E²(z) = Ωₘ(1+z)³(1 + f_R(z)) + Ω_Λ(1+z)³(1 - f_R(z)/3) + Ωᵣ(1+z)⁴ + Ωₖ(1+z)²
 * where f_R(z) = -|f_R0| × Ωₘ(1+z)³ / [Ωₘ(1+z)³ + 4Ω_Λ]
 *
 * f_R0 = 0 → exactly flat ΛCDM (degenerate case).
 * f_R0 ≠ 0 → modified expansion history with screening at high z.
 *
 * Phenomenological toy model — proves the provider interface discriminates
 * between screened modified-gravity theories.
 */

#ifndef QUANTUMVERSE_HU_SAWICKI_PROVIDER_H
#define QUANTUMVERSE_HU_SAWICKI_PROVIDER_H

#include "CosmologyProvider.h"

namespace quantumverse {

class HuSawickiProvider : public CosmologyProvider {
   public:
    /** Parameter bundle. */
    struct Params {
        double H0 = 67.66;     // km/s/Mpc
        double Om = 0.3111;    // matter
        double Ob = 0.049;     // baryons
        double Or = 9.1e-5;    // radiation (photons + ν)
        double OL = 0.6889;    // dark energy
        double Ok = 0.0;       // curvature
        double Tcmb = 2.7255;  // K
        double fR0 = 0.0;      // |f_R0| screening parameter (f_R0 = 0 → ΛCDM)
    };

    /** Planck 2018 cosmology with f_R0 = 0 (identical to flat ΛCDM). */
    HuSawickiProvider();

    /** Arbitrary cosmology from a parameter bundle. */
    explicit HuSawickiProvider(Params p);

    /** Arbitrary cosmology, all quantities in CosmologyProvider units. */
    HuSawickiProvider(double H0, double OmegaM, double OmegaB, double OmegaR, double OmegaLambda,
                      double OmegaK, double Tcmb0, double fR0);

    QString name() const override {
        return QStringLiteral("Hu-Sawicki f(R)");
    }
    QString parametersSource() const override {
        return m_parametersSource;
    }

    double H0() const override {
        return m_H0;
    }
    double OmegaM() const override {
        return m_Om;
    }
    double OmegaB() const override {
        return m_Ob;
    }
    double OmegaR() const override {
        return m_Or;
    }
    double OmegaLambda() const override {
        return m_OL;
    }
    double OmegaK() const override {
        return m_Ok;
    }
    double Tcmb0() const override {
        return m_Tcmb;
    }
    double fR0() const {
        return m_fR0;
    }  // |f_R0| screening parameter

    double H(double z) const override;
    double comovingDistance(double z) const override;
    double luminosityDistance(double z) const override;
    double angularDiameterDistance(double z) const override;
    double distanceModulus(double z) const override;
    double ageAtRedshift(double z) const override;
    double lookbackTime(double z) const override;
    double decelerationParameter(double z) const override;
    double soundHorizon() const override;
    double growthFactor(double z) const override;
    double criticalDensity() const override;

   private:
    double E(double z) const;
    double fR(double z) const;  // f_R(z) = -|f_R0| × Ωₘ(1+z)³ / [Ωₘ(1+z)³ + 4Ω_Λ]
    template <typename F>
    double integrateSimpson(double z1, double z2, int steps, F f) const;
    double integrateHInv(double z1, double z2, int steps = 4096) const;

    QString m_parametersSource = QStringLiteral("user");
    double m_H0 = 67.66;     // km/s/Mpc
    double m_Om = 0.3111;    // matter
    double m_Ob = 0.049;     // baryons
    double m_Or = 9.1e-5;    // radiation
    double m_OL = 0.6889;    // dark energy
    double m_Ok = 0.0;       // curvature
    double m_Tcmb = 2.7255;  // K
    double m_fR0 = 0.0;      // |f_R0| screening parameter
};

}  // namespace quantumverse

#endif  // QUANTUMVERSE_HU_SAWICKI_PROVIDER_H
