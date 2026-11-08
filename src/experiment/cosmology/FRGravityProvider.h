#pragma once

/**
 * @file FRGravityProvider.h
 * @brief Modified-gravity cosmology: f(R)-inspired modification to H(z).
 *
 * Parameterizes the dark-energy component as Ω_Λ_eff(z) = Ω_Λ × (1 + b·z/(1+z)),
 * so that b=0 reproduces flat ΛCDM exactly and b≠0 modifies the expansion at
 * high z. All derived quantities are computed by first-principles integration
 * over H(z), exactly as LambdaCDMReferenceProvider, so the only difference
 * between this provider and ΛCDM is the b parameter in E(z).
 *
 * This is a phenomenological toy model — the point is to prove the provider
 * interface can discriminate between theories, not to claim f(R) is correct.
 */

#ifndef QUANTUMVERSE_FRGRAVITY_PROVIDER_H
#define QUANTUMVERSE_FRGRAVITY_PROVIDER_H

#include "CosmologyProvider.h"

namespace quantumverse {

class FRGravityProvider : public CosmologyProvider {
   public:
    /** Parameter bundle: makes broken-provider tests write the actual member
     *  vars, not just override accessors that the computation bypasses. */
    struct Params {
        double H0 = 67.66;     // km/s/Mpc
        double Om = 0.3111;    // matter
        double Ob = 0.049;     // baryons
        double Or = 9.1e-5;    // radiation (photons + ν)
        double OL = 0.6889;    // dark energy
        double Ok = 0.0;       // curvature
        double Tcmb = 2.7255;  // K
        double b = 0.0;        // f(R) modification parameter (b=0 → ΛCDM)
    };

    /** Planck 2018 cosmology with b=0 (identical to flat ΛCDM). */
    FRGravityProvider();

    /** Arbitrary cosmology from a parameter bundle. */
    explicit FRGravityProvider(Params p);

    /** Arbitrary cosmology, all quantities in the units listed in CosmologyProvider. */
    FRGravityProvider(double H0, double OmegaM, double OmegaB, double OmegaR, double OmegaLambda,
                      double OmegaK, double Tcmb0, double b);

    QString name() const override {
        return QStringLiteral("f(R) gravity");
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
    double fR() const {
        return m_b;
    }  // f(R) parameter, not in base interface

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
    template <typename F>
    double integrateSimpson(double z1, double z2, int steps, F f) const;
    double integrateHInv(double z1, double z2, int steps = 4096) const;

    QString m_parametersSource = QStringLiteral("user");
    double m_H0 = 67.66;     // km/s/Mpc
    double m_Om = 0.3111;    // matter
    double m_Ob = 0.049;     // baryons
    double m_Or = 9.1e-5;    // radiation (photons + ν)
    double m_OL = 0.6889;    // dark energy
    double m_Ok = 0.0;       // curvature
    double m_Tcmb = 2.7255;  // K
    double m_b = 0.0;        // f(R) modification parameter
};

}  // namespace quantumverse

#endif  // QUANTUMVERSE_FRGRAVITY_PROVIDER_H
