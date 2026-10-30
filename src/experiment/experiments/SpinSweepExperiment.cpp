/**
 * @file SpinSweepExperiment.cpp
 * @brief Black-hole spin sweep implementation.
 */

#include "SpinSweepExperiment.h"

#include <cmath>
#include <memory>
#include <QDir>
#include <QDateTime>

#include "spacetime/MetricTensor.h"
#include "spacetime/KerrMetric.h"
#include "spacetime/Event4D.h"

namespace quantumverse {

QVector<ExperimentParameter> SpinSweepExperiment::parameters() const {
    QVector<ExperimentParameter> p;
    p.append({QStringLiteral("spin_min"), QStringLiteral("Spin min a*"),
              QStringLiteral("double"), QVariant(0.0), QVariant(0.0), QVariant(0.999),
              QStringList(), QStringLiteral(""), QStringLiteral("Lower bound of the swept spin.")});
    p.append({QStringLiteral("spin_max"), QStringLiteral("Spin max a*"),
              QStringLiteral("double"), QVariant(0.9), QVariant(0.0), QVariant(0.999),
              QStringList(), QStringLiteral(""), QStringLiteral("Upper bound of the swept spin.")});
    p.append({QStringLiteral("steps"), QStringLiteral("Steps"),
              QStringLiteral("int"), QVariant(50), QVariant(2), QVariant(10000),
              QStringList(), QStringLiteral(""), QStringLiteral("Number of spin samples.")});
    p.append({QStringLiteral("mass"), QStringLiteral("Black hole mass (M\u2299)"),
              QStringLiteral("double"), QVariant(10.0), QVariant(0.1), QVariant(10000.0),
              QStringList(), QStringLiteral("M\u2299"), QStringLiteral("Mass held fixed during the sweep.")});
    p.append({QStringLiteral("probe_r"), QStringLiteral("Probe radius (m)"),
              QStringLiteral("double"), QVariant(1.0e5), QVariant(1.0e3), QVariant(1.0e12),
              QStringList(), QStringLiteral("m"), QStringLiteral("Radius at which frame dragging is sampled.")});
    return p;
}

ExperimentResult SpinSweepExperiment::run(const QVariantMap& params,
                                          std::function<void(double)> progress,
                                          std::function<bool()> shouldAbort) {
    ExperimentResult out;
    const double spinMin = params.value(QStringLiteral("spin_min"), 0.0).toDouble();
    const double spinMax = params.value(QStringLiteral("spin_max"), 0.9).toDouble();
    const int steps = params.value(QStringLiteral("steps"), 50).toInt();
    const double massMsun = params.value(QStringLiteral("mass"), 10.0).toDouble();
    const double probeR = params.value(QStringLiteral("probe_r"), 1.0e5).toDouble();
    const double theta = M_PI / 2.0;

    out.scalars[QStringLiteral("spin_min")] = spinMin;
    out.scalars[QStringLiteral("spin_max")] = spinMax;
    out.scalars[QStringLiteral("steps")] = steps;
    out.scalars[QStringLiteral("mass_msun")] = massMsun;
    out.scalars[QStringLiteral("probe_r")] = probeR;

    const double Msun = 1.989e30;
    const double massKg = massMsun * Msun;
    Event4D probe(0.0, probeR, 0.0, 0.0);

    double maxOmega = 0.0;
    double spinAtMaxOmega = 0.0;

    const int n = std::max(1, steps);
    for (int i = 0; i < n; ++i) {
        if (shouldAbort()) { out.success = false; return out; }

        const double t = (n > 1) ? static_cast<double>(i) / static_cast<double>(n - 1) : 0.0;
        const double spin = spinMin + (spinMax - spinMin) * t;

        auto metric = std::make_shared<quantumverse::KerrMetric>(massKg, spin);
        auto cs = metric->curvatureScalars(probe);
        double k = cs.valid ? cs.kretschmann : 0.0;
        if (!std::isfinite(k)) k = 0.0;

        double omega = metric->frameDraggingOmega(probeR, theta);
        if (!std::isfinite(omega)) omega = 0.0;

        if (omega > maxOmega) { maxOmega = omega; spinAtMaxOmega = spin; }

        out.curve.append(QPointF(spin, omega));
        out.logLines << QString("a*=%1  omega=%2 rad/s  K=%3")
                          .arg(spin, 0, 'g', 5)
                          .arg(omega, 0, 'e', 4)
                          .arg(k, 0, 'e', 4);

        if (progress) progress(static_cast<double>(i + 1) / static_cast<double>(n));
    }

    out.scalars[QStringLiteral("max_frame_dragging_omega")] = maxOmega;
    out.scalars[QStringLiteral("spin_at_max_omega")] = spinAtMaxOmega;
    out.outputDir = QDir::currentPath();
    return out;
}

} // namespace quantumverse
