/**
 * @file FrameDraggingExperiment.cpp
 * @brief Lense–Thirring frame dragging: Ω_LT = 2 G J / (c² r³),
 *        J = a* G M² / c  (a* = dimensionless spin).
 *
 * Result is converted rad/s → milliarcsec/year. GP-B measured ≈ 39 mas/yr
 * (reported as a reference scalar, not a pass/fail target for the defaults).
 */

#include "FrameDraggingExperiment.h"

#include <QDateTime>
#include <QDir>
#include <cmath>

#include "spacetime/Event4D.h"
#include "spacetime/KerrMetric.h"

namespace quantumverse {

QVector<ExperimentParameter> FrameDraggingExperiment::parameters() const {
    QVector<ExperimentParameter> p;
    p.append({QStringLiteral("mass_Msun"), QStringLiteral("Mass (M☉)"), QStringLiteral("double"),
              QVariant(1.0), QVariant(0.01), QVariant(1e6), QStringList(), QStringLiteral("M☉"),
              QStringLiteral("Central body mass.")});
    p.append({QStringLiteral("spin_a_star"), QStringLiteral("Dimensionless spin a*"),
              QStringLiteral("double"), QVariant(0.9), QVariant(0.0), QVariant(0.999),
              QStringList(), QStringLiteral(""), QStringLiteral("Kerr spin parameter.")});
    p.append({QStringLiteral("orbit_radius_m"), QStringLiteral("Orbit radius (m)"),
              QStringLiteral("double"), QVariant(7.0e6), QVariant(1.0e5), QVariant(1.0e12),
              QStringList(), QStringLiteral("m"), QStringLiteral("Orbit radius.")});
    p.append({QStringLiteral("steps"), QStringLiteral("Steps"), QStringLiteral("int"), QVariant(50),
              QVariant(2), QVariant(10000), QStringList(), QStringLiteral(""),
              QStringLiteral("Number of radius samples.")});
    return p;
}

ExperimentResult FrameDraggingExperiment::run(const QVariantMap& params,
                                              std::function<void(double)> progress,
                                              std::function<bool()> shouldAbort) {
    ExperimentResult out;
    const double massMsun = params.value(QStringLiteral("mass_Msun"), 1.0).toDouble();
    const double aStar = params.value(QStringLiteral("spin_a_star"), 0.9).toDouble();
    const double rOrb = params.value(QStringLiteral("orbit_radius_m"), 7.0e6).toDouble();
    const int n = params.value(QStringLiteral("steps"), 50).toInt();

    out.scalars[QStringLiteral("mass_Msun")] = massMsun;
    out.scalars[QStringLiteral("spin_a_star")] = aStar;
    out.scalars[QStringLiteral("orbit_radius_m")] = rOrb;
    out.scalars[QStringLiteral("steps")] = n;

    const double massKg = massMsun * 1.989e30;
    const double G = quantumverse::Event4D::G;
    const double c = quantumverse::Event4D::C;
    const double secondsPerYear = 365.25 * 86400.0;
    const double arcsecPerRad = 180.0 * 3600.0 / M_PI;

    // Angular momentum J = a* G M² / c.
    const double J = aStar * G * massKg * massKg / c;

    const int steps = std::max(1, n);
    double lastMasPerYear = 0.0;

    for (int i = 0; i < steps; ++i) {
        if (shouldAbort()) {
            out.success = false;
            return out;
        }

        const double t =
            (steps > 1) ? static_cast<double>(i) / static_cast<double>(steps - 1) : 0.0;
        // Sweep radius from rOrb down toward the horizon region for the curve.
        const double r = rOrb * (0.1 + 0.9 * t);  // 0.1·rOrb .. rOrb

        // Lense–Thirring angular velocity (weak-field, leading order).
        double omega = 2.0 * G * J / (c * c * std::pow(r, 3.0));
        if (!std::isfinite(omega))
            omega = 0.0;

        const double masPerYear = omega * secondsPerYear * arcsecPerRad * 1000.0;

        lastMasPerYear = masPerYear;
        out.curve.append(QPointF(r, masPerYear));
        out.logLines << QString("r=%1 m  Ω_LT=%2 rad/s  → %3 mas/yr")
                            .arg(r, 0, 'e', 4)
                            .arg(omega, 0, 'e', 4)
                            .arg(masPerYear, 0, 'f', 3);
        progress(static_cast<double>(i + 1) / static_cast<double>(steps));
    }

    out.scalars[QStringLiteral("last_mas_per_year")] = lastMasPerYear;
    out.scalars[QStringLiteral("expected_gpb_mas_per_year")] = 39.0;

    // Kretschmann scalar at the orbit radius — Kerr with spin a*.
    KerrMetric km(massKg, aStar);
    const auto curv = km.curvatureScalars(Event4D(0.0, rOrb, 0.0, 0.0));
    if (curv.valid) {
        out.scalars[QStringLiteral("kretschmann_at_r_orbit")] = curv.kretschmann;
    }

    out.outputDir = QDir::currentPath();
    return out;
}

}  // namespace quantumverse
