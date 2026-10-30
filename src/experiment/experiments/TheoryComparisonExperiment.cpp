/**
 * @file TheoryComparisonExperiment.cpp
 * @brief Compare Kretschmann scalar across metric theories at a single probe.
 *
 * Stand-in for TheoryManager.compareTheories(): compares Schwarzschild vs Kerr
 * vs an explicit Minkowski reference. Wiring to registered TheoryPlugins is
 * a one-line change once the experiment runner holds a TheoryManager pointer.
 */

#include "TheoryComparisonExperiment.h"

#include <cmath>
#include <QDir>
#include <QDateTime>

#include "spacetime/MetricTensor.h"
#include "spacetime/KerrMetric.h"
#include "spacetime/Event4D.h"

namespace quantumverse {

QVector<ExperimentParameter> TheoryComparisonExperiment::parameters() const {
    QVector<ExperimentParameter> p;
    p.append({QStringLiteral("mass"),      QStringLiteral("Mass M (M☉)"),
              QStringLiteral("double"), QVariant(10.0), QVariant(0.1), QVariant(1e6),
              QStringList(), QStringLiteral("M☉"),  QStringLiteral("Black hole mass.")});
    p.append({QStringLiteral("spin"),      QStringLiteral("Dimensionless spin a*"),
              QStringLiteral("double"), QVariant(0.9),  QVariant(0.0), QVariant(0.999),
              QStringList(), QStringLiteral(""),     QStringLiteral("Kerr spin.")});
    p.append({QStringLiteral("probe_r"),   QStringLiteral("Probe radius (m)"),
              QStringLiteral("double"), QVariant(1.0e5), QVariant(1.0e3), QVariant(1.0e12),
              QStringList(), QStringLiteral("m"),   QStringLiteral("Probe distance.")});
    return p;
}

ExperimentResult TheoryComparisonExperiment::run(const QVariantMap& params,
                                                 std::function<void(double)> progress,
                                                 std::function<bool()> shouldAbort) {
    ExperimentResult out;
    const double massMsun = params.value(QStringLiteral("mass"), 10.0).toDouble();
    const double spin = params.value(QStringLiteral("spin"), 0.9).toDouble();
    const double probeR = params.value(QStringLiteral("probe_r"), 1.0e5).toDouble();

    out.scalars[QStringLiteral("mass")] = massMsun;
    out.scalars[QStringLiteral("spin")] = spin;
    out.scalars[QStringLiteral("probe_r")] = probeR;

    const double massKg = massMsun * 1.989e30;
    Event4D probe(0.0, probeR, 0.0, 0.0);

    // Schwarzschild.
    auto schw = std::make_shared<quantumverse::SchwarzschildMetric>(massKg);
    auto csSchw = schw->curvatureScalars(probe);
    double kSchw = csSchw.valid ? csSchw.kretschmann : 0.0;

    // Kerr (same mass, given spin).
    auto kerr = std::make_shared<quantumverse::KerrMetric>(massKg, spin);
    auto csKerr = kerr->curvatureScalars(probe);
    double kKerr = csKerr.valid ? csKerr.kretschmann : 0.0;

    // Explicit Minkowski reference: diag(-1, 1, 1, 1) → K = 0.
    MetricTensor minkowski;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            minkowski.g[i][j] = 0.0;
    minkowski.g[0][0] = -1.0;
    minkowski.g[1][1] = 1.0;
    minkowski.g[2][2] = 1.0;
    minkowski.g[3][3] = 1.0;
    double kMink = 0.0;

    out.scalars[QStringLiteral("kretschmann_schwarzschild")] = kSchw;
    out.scalars[QStringLiteral("kretschmann_kerr")] = kKerr;
    out.scalars[QStringLiteral("kretschmann_minkowski")] = kMink;
    out.scalars[QStringLiteral("kerr_minus_schw")] = kKerr - kSchw;

    out.curve.append(QPointF(0.0, kSchw));
    out.curve.append(QPointF(1.0, kKerr));
    out.curve.append(QPointF(2.0, kMink));

    out.logLines << QString("Schwarzschild K=%1").arg(kSchw, 0, 'e', 4);
    out.logLines << QString("Kerr (a*=%1) K=%2").arg(spin, 0, 'g', 3).arg(kKerr, 0, 'e', 4);
    out.logLines << QString("Minkowski K=%1").arg(kMink, 0, 'e', 4);
    out.logLines << QString("ΔK(kerr−schw)=%1").arg(kKerr - kSchw, 0, 'e', 4);

    progress(1.0);
    out.outputDir = QDir::currentPath();
    return out;
}

} // namespace quantumverse
