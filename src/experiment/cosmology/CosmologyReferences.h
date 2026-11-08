#pragma once

/**
 * @file CosmologyReferences.h
 * @brief Reference-value catalog for the cosmology validation experiments.
 *
 * Each reference carries a central value, a 1σ uncertainty (0 = exact /
 * analytic), a source, a unit, and a note. The σ is what makes a
 * σ-tension comparison possible: tension = (computed − observed) / σ.
 * For analytic references (σ = 0) a relative tolerance is used instead.
 */

#ifndef QUANTUMVERSE_COSMOLOGY_REFERENCES_H
#define QUANTUMVERSE_COSMOLOGY_REFERENCES_H

#include <QString>

namespace quantumverse {

struct CosmologyReference {
    double value;    // central value
    double sigma;    // 1σ uncertainty (0 = exact / analytic)
    QString source;  // "Planck2018" | "SH0ES2022" | "Union2.1-fit" | "COBE/FIRAS" | "analytic"
    QString unit;    // "Gyr", "Mpc", "mag", "km/s/Mpc", "kg/m³", ...
    QString note;    // free-form annotation
};

namespace CosmologyRefs {

// ---------- Planck 2018 (TT,TE,EE+lowE+lensing+BAO) ----------
const CosmologyReference AgeOfUniverse{13.787, 0.020, "Planck2018", "Gyr", "Age of the Universe"};
const CosmologyReference AgeAtZ1{5.851, 0.05, "Planck2018", "Gyr", "Age at z = 1"};
const CosmologyReference SoundHorizon{147.09, 0.26, "Planck2018", "Mpc",
                                      "Sound horizon at drag, r_drag"};
const CosmologyReference GrowthFactorAtZ1{0.778, 0.04, "Planck2018", "D(1)/D(0)",
                                          "Linear growth factor at z = 1"};

// ---------- Exact / analytic ----------
const CosmologyReference TcmbAtZ1100{3000.0, 0.0, "analytic", "K", "T(z=1100) = T₀(1+z), exact"};
const CosmologyReference q0{-0.53335, 0.0, "analytic", "dimensionless",
                            "Deceleration parameter q(0) = Ωₘ/2 − Ω_Λ"};
const CosmologyReference CriticalDensity{8.598e-27, 0.0, "analytic", "kg/m³", "3H₀²/(8πG)"};

// ---------- COBE/FIRAS ----------
const CosmologyReference Tcmb0{2.7255, 0.0006, "COBE/FIRAS", "K", "Present-day CMB temperature"};

// ---------- SNe Ia distance moduli for Planck ΛCDM (Union2.1 fit) ----------
const CosmologyReference MuAtZ01{38.23, 0.15, "Union2.1-fit", "mag", "μ(z = 0.1)"};
const CosmologyReference MuAtZ05{42.27, 0.03, "Union2.1-fit", "mag", "μ(z = 0.5)"};
const CosmologyReference MuAtZ1{44.16, 0.05, "Union2.1-fit", "mag", "μ(z = 1.0)"};

// ---------- H₀ tension: both values reported, tension made explicit ----------
const CosmologyReference H0_Planck{67.66, 0.42, "Planck2018", "km/s/Mpc",
                                   "Early-universe (CMB) value"};
const CosmologyReference H0_SH0ES{73.04, 1.04, "SH0ES2022", "km/s/Mpc",
                                  "Late-universe (distance ladder) value"};

}  // namespace CosmologyRefs

}  // namespace quantumverse

#endif  // QUANTUMVERSE_COSMOLOGY_REFERENCES_H
