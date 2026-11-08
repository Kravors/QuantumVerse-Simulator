#pragma once

#ifndef QUANTUMVERSE_PHOTON_SPHERE_EXPERIMENT_H
#define QUANTUMVERSE_PHOTON_SPHERE_EXPERIMENT_H

#include "../Experiment.h"

namespace quantumverse {

/**
 * @brief Kerr equatorial photon orbits and shadow edges (Bardeen 1972).
 *
 * Exact analytic formulas for prograde (co-rotating) and retrograde
 * (counter-rotating) photon orbit radii and critical impact parameters:
 *
 *   r_ph^+ = 2M * (1 + cos((2/3)*acos(-a)))   (prograde)
 *   r_ph^- = 2M * (1 + cos((2/3)*acos(+a)))   (retrograde)
 *   b_c^+  = -a + 6M*cos((1/3)*acos(-a))      (prograde)
 *   b_c^-  = +a + 6M*cos((1/3)*acos(+a))      (retrograde)
 *
 * Main curve: prograde r_ph(a)/M — monotonically decreasing from 3M
 * (Schwarzschild) to M (extremal).
 *
 * Reference (extremal limits, exact):
 *   r_ph^+ → M, r_ph^- → 4M, b_c^+ → 2M, b_c^- → 7M
 *
 * First instrument producing a prograde/retrograde asymmetry pair,
 * with the shadow asymmetry ratio as a scalar scalar.
 */

class PhotonSphereExperiment : public Experiment {
   public:
    QString id() const override {
        return QStringLiteral("photon_sphere");
    }
    QString name() const override {
        return QStringLiteral("Kerr Photon Sphere & Shadow");
    }
    QString category() const override {
        return QStringLiteral("Black Holes");
    }
    QString description() const override {
        return QStringLiteral(
            "Equatorial photon orbit radii r_ph(a) and critical "
            "impact parameters b_c(a) vs spin. Bardeen 1972 exact "
            "formulas. Prograde curve; retrograde + asymmetry in "
            "scalars.");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params, std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;
};

}  // namespace quantumverse

#endif  // QUANTUMVERSE_PHOTON_SPHERE_EXPERIMENT_H
