/**
 * @file LensingDeflectionExperiment.cpp
 * @brief Light deflection vs impact parameter (weak-field Schwarzschild).
 */

#include "LensingDeflectionExperiment.h"

#include <cmath>
#include <QDir>
#include <QDateTime>

#include "spacetime/Event4D.h"

namespace quantumverse {

QVector<ExperimentParameter> LensingDeflectionExperiment::parameters() const {
    QVector<ExperimentParameter> p;
    p.append({QStringLiteral("b_min"), QStringLiteral("Impact parameter min (m)"),
              QStringLiteral("double"), QVariant(1.0e4), QVariant(1.0e3), QVariant(1.0e10),
              QStringList(), QStringLiteral("m"), QStringLiteral("Lower bound of the impact parameter.")});
    p.append({QStringLiteral("b_max"), QStringLiteral("Impact parameter max (m)"),
              QStringLiteral("double"), QVariant(1.0e6), QVariant(1.0e3), QVariant(1.0e10),
              QStringList(), QStringLiteral("m"), QStringLiteral("Upper bound of the impact parameter.")});
    p.append({QStringLiteral("steps"), QStringLiteral("Steps"),
              QStringLiteral("int"), QVariant(50), QVariant(2), QVariant(10000),
              QStringList(), QStringLiteral(""), QStringLiteral("Number of impact-parameter samples.")});
    p.append({QStringLiteral("mass"), QStringLiteral("Lens mass (M\u2299)"),
              QStringLiteral("double"), QVariant(1.0), QVariant(0.01), QVariant(1e6),
              QStringList(), QStringLiteral("M\u2299"), QStringLiteral("Mass of the lensing object.")});
    return p;
}

ExperimentResult LensingDeflectionExperiment::run(const QVariantMap& params,
                                                  std::function<void(double)> progress,
                                                  std::function<bool()> shouldAbort) {
    ExperimentResult out;
    const double bMin = params.value(QStringLiteral("b_min"), 1.0e4).toDouble();
    const double bMax = params.value(QStringLiteral("b_max"), 1.0e6).toDouble();
    const int steps = params.value(QStringLiteral("steps"), 50).toInt();
    const double massMsun = params.value(QStringLiteral("mass"), 1.0).toDouble();

    out.scalars[QStringLiteral("b_min")] = bMin;
    out.scalars[QStringLiteral("b_max")] = bMax;
    out.scalars[QStringLiteral("steps")] = steps;
    out.scalars[QStringLiteral("mass_msun")] = massMsun;

    const double massKg = massMsun * 1.989e30;
    const double G = quantumverse::Event4D::G;
    const double c = quantumverse::Event4D::C;
    const double arcsecPerRad = 206264.806;

    const int n = std::max(1, steps);
    for (int i = 0; i < n; ++i) {
        if (shouldAbort()) { out.success = false; return out; }

        const double t = (n > 1) ? static_cast<double>(i) / static_cast<double>(n - 1) : 0.0;
        const double b = bMin + (bMax - bMin) * t;

        // Weak-field deflection: alpha = 4 G M / (c^2 b) [radians]
        double alphaRad = 4.0 * G * massKg / (c * c * b);
        if (!std::isfinite(alphaRad)) alphaRad = 0.0;
        double alphaArcsec = alphaRad * arcsecPerRad;

        out.curve.append(QPointF(b, alphaArcsec));
        out.logLines << QString("b=%1 m  alpha=%2 arcsec (%3 rad)")
                          .arg(b, 0, 'e', 4)
                          .arg(alphaArcsec, 0, 'g', 5)
                          .arg(alphaRad, 0, 'e', 4);

        if (progress) progress(static_cast<double>(i + 1) / static_cast<double>(n));
    }

    out.scalars[QStringLiteral("deflection_at_bmin_arcsec")] =
        4.0 * G * massKg / (c * c * bMin) * arcsecPerRad;
    out.scalars[QStringLiteral("deflection_at_bmax_arcsec")] =
        4.0 * G * massKg / (c * c * bMax) * arcsecPerRad;
    out.outputDir = QDir::currentPath();
    return out;
}

} // namespace quantumverse
