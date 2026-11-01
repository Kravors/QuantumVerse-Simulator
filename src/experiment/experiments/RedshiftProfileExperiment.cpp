/**
 * @file RedshiftProfileExperiment.cpp
 * @brief Gravitational redshift z(r) = 1/sqrt(-g_tt) - 1 vs radius.
 */

#include "RedshiftProfileExperiment.h"

#include <cmath>
#include <QDir>
#include <QDateTime>

#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "spacetime/KerrMetric.h"

namespace quantumverse {

QVector<ExperimentParameter> RedshiftProfileExperiment::parameters() const {
    QVector<ExperimentParameter> p;
    p.append({QStringLiteral("mass"),      QStringLiteral("Mass M (M☉)"),
              QStringLiteral("double"), QVariant(10.0), QVariant(0.1), QVariant(1e6),
              QStringList(), QStringLiteral("M☉"),  QStringLiteral("Black hole mass.")});
    p.append({QStringLiteral("r_min"),     QStringLiteral("Inner radius (m)"),
              QStringLiteral("double"), QVariant(3000.0), QVariant(1.0), QVariant(1e9),
              QStringList(), QStringLiteral("m"),  QStringLiteral("Start just outside r_s.")});
    p.append({QStringLiteral("r_max"),     QStringLiteral("Outer radius (m)"),
              QStringLiteral("double"), QVariant(1.0e6), QVariant(1.0e3), QVariant(1e12),
              QStringList(), QStringLiteral("m"),  QStringLiteral("Far-field radius.")});
    p.append({QStringLiteral("steps"),     QStringLiteral("Steps"),
              QStringLiteral("int"),    QVariant(50),   QVariant(2), QVariant(10000),
              QStringList(), QStringLiteral(""),     QStringLiteral("Number of radius samples.")});
    return p;
}

ExperimentResult RedshiftProfileExperiment::run(const QVariantMap& params,
                                                std::function<void(double)> progress,
                                                std::function<bool()> shouldAbort) {
    ExperimentResult out;
    const double massMsun = params.value(QStringLiteral("mass"), 10.0).toDouble();
    const double rMin = params.value(QStringLiteral("r_min"), 3000.0).toDouble();
    const double rMax = params.value(QStringLiteral("r_max"), 1.0e6).toDouble();
    const int steps = params.value(QStringLiteral("steps"), 50).toInt();

    out.scalars[QStringLiteral("mass")] = massMsun;
    out.scalars[QStringLiteral("r_min")] = rMin;
    out.scalars[QStringLiteral("r_max")] = rMax;
    out.scalars[QStringLiteral("steps")] = steps;

    if (steps < 2 || rMin <= 0.0 || rMax <= rMin) {
        out.success = false;
        return out;
    }

    const double massKg = massMsun * 1.989e30;
    auto metric = std::make_shared<quantumverse::SchwarzschildMetric>(massKg);

    double zAtRmin = 0.0;

    const int n = std::max(1, steps);
    for (int i = 0; i < n; ++i) {
        if (shouldAbort()) { out.success = false; return out; }

        const double t = (n > 1) ? static_cast<double>(i) / static_cast<double>(n - 1) : 0.0;
        const double r = rMin + (rMax - rMin) * t;

        Event4D probe(0.0, r, 0.0, 0.0);
        auto g = metric->evaluate(probe);
        double gtt = g[0][0];
        double z = 0.0;
        if (gtt < 0.0 && std::isfinite(gtt)) {
            z = 1.0 / std::sqrt(-gtt) - 1.0;
            if (!std::isfinite(z)) z = 0.0;
        }

        if (i == 0) zAtRmin = z;
        out.curve.append(QPointF(r, z));
        out.logLines << QString("r=%1 m  g_tt=%2  z=%3")
                          .arg(r, 0, 'e', 4).arg(gtt, 0, 'e', 4).arg(z, 0, 'g', 5);
        progress(static_cast<double>(i + 1) / static_cast<double>(n));
    }

    out.scalars[QStringLiteral("z_at_r_min")] = zAtRmin;

    // Kretschmann scalar at r_min — Schwarzschild (spin = 0).
    KerrMetric km(massKg, 0.0);
    const double r_min_geo = rMin; // r_min already in meters
    const auto curv = km.curvatureScalars(Event4D(0.0, r_min_geo, 0.0, 0.0));
    if (curv.valid) {
        out.scalars[QStringLiteral("kretschmann_at_r_min")] = curv.kretschmann;
    }

    out.outputDir = QDir::currentPath();
    return out;
}

} // namespace quantumverse
