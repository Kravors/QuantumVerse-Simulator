#pragma once

/**
 * @file LambdaCDMReferenceProvider.h
 * @brief First-principles flat ΛCDM background cosmology.
 *
 * Self-contained solver: every derived quantity is either a closed-form
 * expression or a one-dimensional numerical integral over H(z). Used by the
 * cosmology validation experiments as the reference implementation against
 * which the project's cosmology is compared.
 *
 * Default parameters: Planck 2018 (TT,TE,EE+lowE+lensing+BAO).
 */

#ifndef QUANTUMVERSE_LAMBDA_CDM_REFERENCE_PROVIDER_H
#define QUANTUMVERSE_LAMBDA_CDM_REFERENCE_PROVIDER_H

#include "CosmologyProvider.h"

namespace quantumverse {

class LambdaCDMReferenceProvider : public CosmologyProvider {
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
    };

    /** Planck 2018 (TT,TE,EE+lowE+lensing+BAO) cosmology. */
    LambdaCDMReferenceProvider();

    /** Arbitrary cosmology from a parameter bundle. */
    explicit LambdaCDMReferenceProvider(Params p);

    /** Arbitrary cosmology, all quantities in the units listed in CosmologyProvider. */
    LambdaCDMReferenceProvider(double H0, double OmegaM, double OmegaB, double OmegaR,
                               double OmegaLambda, double OmegaK, double Tcmb0);

    QString name() const override {
        return QStringLiteral("reference-ΛCDM");
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
    double integrateHInv(double z1, double z2, int steps = 4096) const;  // ∫ dz/H(z)
    template <typename F>
    double integrateSimpson(double z1, double z2, int steps, F f) const;

    QString m_parametersSource = QStringLiteral("Planck2018");
    double m_H0 = 67.66;     // km/s/Mpc
    double m_Om = 0.3111;    // matter
    double m_Ob = 0.049;     // baryons
    double m_Or = 9.1e-5;    // radiation (photons + ν)
    double m_OL = 0.6889;    // dark energy
    double m_Ok = 0.0;       // curvature
    double m_Tcmb = 2.7255;  // K
};

}  // namespace quantumverse

#endif  // QUANTUMVERSE_LAMBDA_CDM_REFERENCE_PROVIDER_H
