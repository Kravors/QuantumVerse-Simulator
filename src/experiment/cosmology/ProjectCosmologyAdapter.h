#pragma once

/**
 * @file ProjectCosmologyAdapter.h
 * @brief Adapter exposing the project's FLRW cosmology (src/spacetime/FRWMetric)
 *        through the CosmologyProvider interface.
 *
 * The project's FRWMetric is a toy metric: its ageOfUniverse() is hard-coded to
 * the matter-dominated value 2/(3H₀) and it carries no Ω_b, Ω_r or T_CMB state.
 * Until a full FLRW solver exists, this adapter constructs an FRWMetric to link
 * against the project's cosmology code, reads H₀/Ω from it, and computes the
 * derived quantities (H(z), distances, age, growth, ...) with first-principles
 * integrals so the validation suite stays self-consistent. Replace the derived
 * computations with real FRWMetric calls once the project's FLRW solver is
 * complete.
 */

#ifndef QUANTUMVERSE_PROJECT_COSMOLOGY_ADAPTER_H
#define QUANTUMVERSE_PROJECT_COSMOLOGY_ADAPTER_H

#include "../cosmology/CosmologyProvider.h"
#include "spacetime/FRWMetric.h"

#include <memory>

namespace quantumverse {

class ProjectCosmologyAdapter : public CosmologyProvider {
public:
    /** Build from user-supplied cosmological parameters (units per CosmologyProvider). */
    ProjectCosmologyAdapter(double H0, double OmegaM, double OmegaB, double OmegaR,
                            double OmegaLambda, double OmegaK, double Tcmb0);

    QString name()             const override { return QStringLiteral("project-FLRW"); }
    QString parametersSource() const override { return QStringLiteral("user"); }

    double H0()          const override { return m_H0; }
    double OmegaM()      const override { return m_Om; }
    double OmegaB()      const override { return m_Ob; }
    double OmegaR()      const override { return m_Or; }
    double OmegaLambda() const override { return m_OL; }
    double OmegaK()      const override { return m_Ok; }
    double Tcmb0()       const override { return m_Tcmb; }

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
    double m_H0, m_Om, m_Ob, m_Or, m_OL, m_Ok, m_Tcmb;
    std::shared_ptr<FRWMetric> m_frw; // linkage to the project's FLRW code
};

} // namespace quantumverse

#endif // QUANTUMVERSE_PROJECT_COSMOLOGY_ADAPTER_H
