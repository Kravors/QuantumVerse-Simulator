#pragma once

/**
 * @file BransDickeProvider.h
 * @brief Brans-Dicke scalar-tensor cosmology — growth-only modification.
 *
 * Background expansion H(z) is identical to flat ΛCDM (Jordan-frame metric
 * reduces to GR at background level in this toy parametrization).
 * Growth factor is enhanced: D(z) = D_GR(z) · (1 + 1/(2ω+3)).
 *
 * ω → ∞ → exactly GR/ΛCDM (degenerate case).
 * Finite ω → stronger gravity at structure-formation scales, changing only
 * the growth factor, not H(z), distances, or age.
 *
 * Phenomenological toy model — proves the provider interface discriminates
 * between growth-history theories separately from expansion-history theories.
 */

#ifndef QUANTUMVERSE_BRANS_DICKE_PROVIDER_H
#define QUANTUMVERSE_BRANS_DICKE_PROVIDER_H

#include "CosmologyProvider.h"

namespace quantumverse {

class BransDickeProvider : public CosmologyProvider {
public:
    struct Params {
        double H0 = 67.66;     // km/s/Mpc
        double Om = 0.3111;    // matter
        double Ob = 0.049;     // baryons
        double Or = 9.1e-5;    // radiation
        double OL = 0.6889;    // dark energy
        double Ok = 0.0;       // curvature
        double Tcmb = 2.7255;  // K
        double omega = 50.0;   // Brans-Dicke coupling (ω → ∞ → GR)
    };

    /** Planck 2018 cosmology with ω→∞ (identical to flat ΛCDM). */
    BransDickeProvider();

    /** Arbitrary cosmology from a parameter bundle. */
    explicit BransDickeProvider(Params p);

    /** Arbitrary cosmology, all quantities in CosmologyProvider units. */
    BransDickeProvider(double H0, double OmegaM, double OmegaB, double OmegaR,
                       double OmegaLambda, double OmegaK, double Tcmb0,
                       double omega);

    QString name()             const override { return QStringLiteral("Brans-Dicke"); }
    QString parametersSource() const override { return m_parametersSource; }

    double H0()          const override { return m_H0; }
    double OmegaM()      const override { return m_Om; }
    double OmegaB()      const override { return m_Ob; }
    double OmegaR()      const override { return m_Or; }
    double OmegaLambda() const override { return m_OL; }
    double OmegaK()      const override { return m_Ok; }
    double Tcmb0()       const override { return m_Tcmb; }
    double omega()       const { return m_omega; }   // Brans-Dicke coupling

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
    /** GR/ΛCDM growth factor (Carroll-Press-Turner fit, D(0)=1). */
    double grGrowthFactor(double z) const;
    double integrateHInv(double z1, double z2, int steps = 4096) const;

    QString m_parametersSource = QStringLiteral("user");
    double m_H0 = 67.66;     // km/s/Mpc
    double m_Om = 0.3111;    // matter
    double m_Ob = 0.049;     // baryons
    double m_Or = 9.1e-5;    // radiation
    double m_OL = 0.6889;    // dark energy
    double m_Ok = 0.0;       // curvature
    double m_Tcmb = 2.7255;  // K
    double m_omega = 50.0;   // Brans-Dicke coupling
};

} // namespace quantumverse

#endif // QUANTUMVERSE_BRANS_DICKE_PROVIDER_H
