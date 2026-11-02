#pragma once

/**
 * @file WCDMProvider.h
 * @brief wCDM cosmology with Chevallier-Polarski-Linder (CPL) dark energy.
 *
 * E²(z) = Ωₘ(1+z)³ + Ωᵣ(1+z)⁴ + Ωₖ(1+z)² + Ω_Λ·DE(z)
 * where DE(z) = (1+z)^(3(1+w0+wa)) · exp(-3·wa·z/(1+z))
 *
 * w0=-1, wa=0 → exactly flat ΛCDM (degenerate case).
 * w0≠-1 or wa≠0 → dark energy equation of state evolves with redshift,
 * changing H(z) at intermediate and high z (peaking around z~1-3).
 *
 * Phenomenological toy model — proves the provider interface discriminates
 * between expansion-history theories.
 */

#ifndef QUANTUMVERSE_WCDM_PROVIDER_H
#define QUANTUMVERSE_WCDM_PROVIDER_H

#include "CosmologyProvider.h"

namespace quantumverse {

class WCDMProvider : public CosmologyProvider {
public:
    struct Params {
        double H0 = 67.66;     // km/s/Mpc
        double Om = 0.3111;    // matter
        double Ob = 0.049;     // baryons
        double Or = 9.1e-5;    // radiation
        double OL = 0.6889;    // dark energy
        double Ok = 0.0;       // curvature
        double Tcmb = 2.7255;  // K
        double w0 = -1.0;      // DE equation of state today
        double wa = 0.0;       // DE equation of state evolution
    };

    /** Planck 2018 cosmology with w0=-1, wa=0 (identical to flat ΛCDM). */
    WCDMProvider();

    /** Arbitrary cosmology from a parameter bundle. */
    explicit WCDMProvider(Params p);

    /** Arbitrary cosmology, all quantities in CosmologyProvider units. */
    WCDMProvider(double H0, double OmegaM, double OmegaB, double OmegaR,
                 double OmegaLambda, double OmegaK, double Tcmb0,
                 double w0, double wa);

    QString name()             const override { return QStringLiteral("wCDM"); }
    QString parametersSource() const override { return m_parametersSource; }

    double H0()          const override { return m_H0; }
    double OmegaM()      const override { return m_Om; }
    double OmegaB()      const override { return m_Ob; }
    double OmegaR()      const override { return m_Or; }
    double OmegaLambda() const override { return m_OL; }
    double OmegaK()      const override { return m_Ok; }
    double Tcmb0()       const override { return m_Tcmb; }
    double w0()          const { return m_w0; }   // DE equation of state today
    double wa()          const { return m_wa; }   // DE equation of state evolution

    double H(double z)                        const override;
    double comovingDistance(double z)         const override;
    double luminosityDistance(double z)       const override;
    double angularDiameterDistance(double z)  const override;
    double distanceModulus(double z)          const override;
    double ageAtRedshift(double z)            const override;
    double lookbackTime(double z)             const override;
    double decelerationParameter(double z)    const override;
    double soundHorizon()                     const override;
    double growthFactor(double z)             const override;
    double criticalDensity()                  const override;

private:
    double E(double z) const;
    template<typename F>
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
    double m_w0 = -1.0;      // DE equation of state today
    double m_wa = 0.0;       // DE equation of state evolution
};

} // namespace quantumverse

#endif // QUANTUMVERSE_WCDM_PROVIDER_H
