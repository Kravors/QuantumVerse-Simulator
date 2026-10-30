/**
 * @file ChargeSweepExperiment.cpp
 * @brief Reissner–Nordström charge sweep implementation.
 *
 * Notes:
 *   - No RN metric class exists in the codebase, so horizons are computed
 *     analytically in geometric units (G = c = 1):
 *         r± = M ± sqrt(M² − Q²),  Q = qm · M
 *   - Curvature at the probe radius is sampled with SchwarzschildMetric
 *     (charge does not affect Kretschmann in this codebase until an RN
 *     metric is added).
 */

#include "ChargeSweepExperiment.h"

#include <cmath>
#include <memory>
#include <QDir>
#include <QDateTime>

#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

namespace quantumverse {

QVector<ExperimentParameter> ChargeSweepExperiment::parameters() const {
    QVector<ExperimentParameter> p;
    p.append({QStringLiteral("mass"),       QStringLiteral("Mass M (M\u2299)"),
              QStringLiteral("double"), QVariant(1.0),  QVariant(0.1), QVariant(1000.0),
              QStringList(), QStringLiteral("M\u2299"), QStringLiteral("Black hole mass.")});
    p.append({QStringLiteral("charge_min"), QStringLiteral("Q/M min"),
              QStringLiteral("double"), QVariant(0.0),  QVariant(0.0), QVariant(0.99),
              QStringList(), QStringLiteral(""),       QStringLiteral("Charge ratio lower bound.")});
    p.append({QStringLiteral("charge_max"), QStringLiteral("Q/M max"),
              QStringLiteral("double"), QVariant(0.99), QVariant(0.0), QVariant(0.99),
              QStringList(), QStringLiteral(""),       QStringLiteral("Upper bound; 1.0 is extremal.")});
    p.append({QStringLiteral("steps"),     QStringLiteral("Steps"),
              QStringLiteral("int"),    QVariant(50),   QVariant(2),   QVariant(10000),
              QStringList(), QStringLiteral(""),       QStringLiteral("Number of samples.")});
    p.append({QStringLiteral("probe_r"),   QStringLiteral("Probe radius (m)"),
              QStringLiteral("double"), QVariant(1.0e5),QVariant(1.0e3), QVariant(1.0e12),
              QStringList(), QStringLiteral("m"),      QStringLiteral("Curvature probe distance.")});
    return p;
}

ExperimentResult ChargeSweepExperiment::run(const QVariantMap& params,
                                            std::function<void(double)> progress,
                                            std::function<bool()> shouldAbort) {
    ExperimentResult out;
    const double massMsun = params.value(QStringLiteral("mass"), 1.0).toDouble();
    const double qMin = params.value(QStringLiteral("charge_min"), 0.0).toDouble();
    const double qMax = params.value(QStringLiteral("charge_max"), 0.99).toDouble();
    const int steps = params.value(QStringLiteral("steps"), 50).toInt();
    const double probeR = params.value(QStringLiteral("probe_r"), 1.0e5).toDouble();

    out.scalars[QStringLiteral("mass")] = massMsun;
    out.scalars[QStringLiteral("charge_min")] = qMin;
    out.scalars[QStringLiteral("charge_max")] = qMax;
    out.scalars[QStringLiteral("steps")] = steps;
    out.scalars[QStringLiteral("probe_r")] = probeR;

    if (steps < 2) { out.success = false; return out; }

    const double massKg = massMsun * 1.989e30;
    const double G = quantumverse::Event4D::G;
    const double c = quantumverse::Event4D::C;
    // Geometric mass M = GM/c² (meters).
    const double M = G * massKg / (c * c);

    auto metric = std::make_unique<quantumverse::SchwarzschildMetric>(massKg);
    Event4D probe(0.0, probeR, 0.0, 0.0);

    double rPlusAtExtremal = 0.0;

    const int n = std::max(1, steps);
    for (int i = 0; i < n; ++i) {
        if (shouldAbort()) { out.success = false; return out; }

        const double t = (n > 1) ? static_cast<double>(i) / static_cast<double>(n - 1) : 0.0;
        const double qm = qMin + (qMax - qMin) * t;

        // RN horizons (geometric units, meters).
        double disc = M * M * (1.0 - qm * qm);
        if (disc < 0.0) disc = 0.0;
        const double sqrtDisc = std::sqrt(disc);
        const double rPlus  = M + sqrtDisc;
        const double rMinus = M - sqrtDisc;

        if (qm >= 1.0 - 1e-9) rPlusAtExtremal = rPlus;

        // Curvature probe (Schwarzschild; charge not modeled in K here).
        auto cs = metric->curvatureScalars(probe);
        double K = cs.valid ? cs.kretschmann : 0.0;
        if (!std::isfinite(K)) K = 0.0;

        out.curve.append(QPointF(qm, rPlus));
        out.logLines << QString("Q/M=%1  r₊=%2 m  r₋=%3 m  K=%4")
                          .arg(qm, 0, 'g', 5)
                          .arg(rPlus, 0, 'g', 5)
                          .arg(rMinus, 0, 'g', 5)
                          .arg(K, 0, 'e', 4);

        out.scalars[QStringLiteral("last_r_plus")]  = rPlus;
        out.scalars[QStringLiteral("last_r_minus")] = rMinus;
        progress(static_cast<double>(i + 1) / static_cast<double>(n));
    }

    out.scalars[QStringLiteral("r_plus_at_extremal")] = rPlusAtExtremal;
    out.outputDir = QDir::currentPath();
    return out;
}

} // namespace quantumverse
