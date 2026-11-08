#pragma once

/**
 * @file CosmologyProvider.h
 * @brief Abstract interface for a cosmology background (H(z), distances, age,
 *        growth, ...).
 *
 * Experiments call ONLY the derived quantities below, so the same experiment
 * code validates either a first-principles reference implementation or the
 * project's own cosmology by swapping the provider. All distances are in Mpc,
 * times in Gyr, H in km/s/Mpc, densities in kg/m^3.
 */

#ifndef QUANTUMVERSE_COSMOLOGY_PROVIDER_H
#define QUANTUMVERSE_COSMOLOGY_PROVIDER_H

#include <QString>

namespace quantumverse {

class CosmologyProvider {
   public:
    virtual ~CosmologyProvider() = default;

    /** Human-readable provider name, e.g. "reference-ΛCDM" | "project-FLRW". */
    virtual QString name() const = 0;
    /** Origin of the input parameters, e.g. "Planck2018" | "user". */
    virtual QString parametersSource() const = 0;

    // Cosmological parameters (all in the units listed).
    virtual double H0() const = 0;           // km/s/Mpc
    virtual double OmegaM() const = 0;       // matter (baryons + DM)
    virtual double OmegaB() const = 0;       // baryons
    virtual double OmegaR() const = 0;       // radiation (photons + ν)
    virtual double OmegaLambda() const = 0;  // dark energy (Λ)
    virtual double OmegaK() const = 0;       // curvature
    virtual double Tcmb0() const = 0;        // K

    // Derived quantities — the experiments call ONLY these.
    virtual double H(double z) const = 0;                        // km/s/Mpc
    virtual double comovingDistance(double z) const = 0;         // Mpc
    virtual double luminosityDistance(double z) const = 0;       // Mpc
    virtual double angularDiameterDistance(double z) const = 0;  // Mpc
    virtual double distanceModulus(double z) const = 0;          // mag
    virtual double ageAtRedshift(double z) const = 0;            // Gyr
    virtual double lookbackTime(double z) const = 0;             // Gyr
    virtual double decelerationParameter(double z) const = 0;
    virtual double soundHorizon() const = 0;          // Mpc (r_drag)
    virtual double growthFactor(double z) const = 0;  // D(z)/D(0)
    virtual double criticalDensity() const = 0;       // kg/m³
};

}  // namespace quantumverse

#endif  // QUANTUMVERSE_COSMOLOGY_PROVIDER_H
