/**
 * @file MassSweepExperiment.cpp
 * @brief Black-hole mass sweep implementation.
 */

#include "MassSweepExperiment.h"

#include <cmath>
#include <memory>
#include <QDir>
#include <QDateTime>

#include "spacetime/MetricTensor.h"
#include "spacetime/KerrMetric.h"
#include "spacetime/Event4D.h"

namespace quantumverse {

QVector<ExperimentParameter> MassSweepExperiment::parameters() const {
    QVector<ExperimentParameter> p;
    p.append({QStringLiteral("mass_min"), QStringLiteral("Mass min (M\u2299)"),
              QStringLiteral("double"), QVariant(0.1), QVariant(0.01), QVariant(1000.0),
              QStringList(), QStringLiteral("M\u2299"), QStringLiteral("Lower bound of the swept mass.")});
    p.append({QStringLiteral("mass_max"), QStringLiteral("Mass max (M\u2299)"),
              QStringLiteral("double"), QVariant(10.0), QVariant(0.01), QVariant(1000.0),
              QStringList(), QStringLiteral("M\u2299"), QStringLiteral("Upper bound of the swept mass.")});
    p.append({QStringLiteral("steps"), QStringLiteral("Steps"),
              QStringLiteral("int"), QVariant(50), QVariant(2), QVariant(10000),
              QStringList(), QStringLiteral(""), QStringLiteral("Number of mass samples.")});
    p.append({QStringLiteral("spin"), QStringLiteral("Dimensionless spin a*"),
              QStringLiteral("double"), QVariant(0.0), QVariant(0.0), QVariant(0.999),
              QStringList(), QStringLiteral(""), QStringLiteral("Kerr spin held fixed during the sweep.")});
    p.append({QStringLiteral("probe_r"), QStringLiteral("Probe radius (m)"),
              QStringLiteral("double"), QVariant(1.0e5), QVariant(1.0e3), QVariant(1.0e12),
              QStringList(), QStringLiteral("m"), QStringLiteral("Radius at which curvature/redshift are sampled.")});
    return p;
}

ExperimentResult MassSweepExperiment::run(const QVariantMap& params,
                                          std::function<void(double)> progress,
                                          std::function<bool()> shouldAbort) {
    ExperimentResult out;
    const double massMin = params.value(QStringLiteral("mass_min"), 0.1).toDouble();
    const double massMax = params.value(QStringLiteral("mass_max"), 10.0).toDouble();
    const int steps = params.value(QStringLiteral("steps"), 50).toInt();
    const double spin = params.value(QStringLiteral("spin"), 0.0).toDouble();
    const double probeR = params.value(QStringLiteral("probe_r"), 1.0e5).toDouble();

    out.scalars[QStringLiteral("mass_min")] = massMin;
    out.scalars[QStringLiteral("mass_max")] = massMax;
    out.scalars[QStringLiteral("steps")] = steps;
    out.scalars[QStringLiteral("spin")] = spin;
    out.scalars[QStringLiteral("probe_r")] = probeR;

    const double Msun = 1.989e30;
    Event4D probe(0.0, probeR, 0.0, 0.0);
    double maxK = 0.0;
    double massAtMaxK = 0.0;

    const int n = std::max(1, steps);
    for (int i = 0; i < n; ++i) {
        if (shouldAbort()) { out.success = false; return out; }

        const double t = (n > 1) ? static_cast<double>(i) / static_cast<double>(n - 1) : 0.0;
        const double massMsun = massMin + (massMax - massMin) * t;
        const double massKg = massMsun * Msun;

        std::shared_ptr<quantumverse::MetricTensor> metric;
        if (std::abs(spin) > 1e-12) {
            metric = std::make_shared<quantumverse::KerrMetric>(massKg, spin);
        } else {
            metric = std::make_shared<quantumverse::SchwarzschildMetric>(massKg);
        }

        auto cs = metric->curvatureScalars(probe);
        double k = cs.valid ? cs.kretschmann : 0.0;
        if (!std::isfinite(k)) k = 0.0;

        if (k > maxK) { maxK = k; massAtMaxK = massMsun; }

        // Gravitational redshift from g_tt at the probe.
        auto g = metric->evaluate(probe);
        double z = 0.0;
        if (g[0][0] < 0.0 && std::isfinite(g[0][0])) {
            z = 1.0 / std::sqrt(-g[0][0]) - 1.0;
            if (!std::isfinite(z)) z = 0.0;
        }

        out.curve.append(QPointF(massMsun, k));
        out.logLines << QString("M=%1 M\u2299  K=%2  z=%3  valid=%4")
                          .arg(massMsun, 0, 'g', 5)
                          .arg(k, 0, 'e', 4)
                          .arg(z, 0, 'g', 5)
                          .arg(cs.valid ? QStringLiteral("true") : QStringLiteral("false"));

        if (progress) progress(static_cast<double>(i + 1) / static_cast<double>(n));
    }

    out.scalars[QStringLiteral("max_kretschmann")] = maxK;
    out.scalars[QStringLiteral("mass_at_max_kretschmann")] = massAtMaxK;
    out.outputDir = QDir::currentPath();
    return out;
}

} // namespace quantumverse
