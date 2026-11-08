/**
 * @file QuantumGeometryExperiment.cpp
 * @brief Spectral dimension vs diffusion scale using the CDT engine.
 */

#include "QuantumGeometryExperiment.h"

#include <QDateTime>
#include <QDir>
#include <cmath>

#include "quantumgravity/CDTEngine.h"

namespace quantumverse {

QVector<ExperimentParameter> QuantumGeometryExperiment::parameters() const {
    QVector<ExperimentParameter> p;
    p.append({QStringLiteral("mu_min"), QStringLiteral("Diffusion scale min"),
              QStringLiteral("double"), QVariant(0.1), QVariant(0.01), QVariant(100.0),
              QStringList(), QStringLiteral(""), QStringLiteral("Lower bound of mu.")});
    p.append({QStringLiteral("mu_max"), QStringLiteral("Diffusion scale max"),
              QStringLiteral("double"), QVariant(100.0), QVariant(0.01), QVariant(1000.0),
              QStringList(), QStringLiteral(""), QStringLiteral("Upper bound of mu.")});
    p.append({QStringLiteral("steps"), QStringLiteral("Steps"), QStringLiteral("int"), QVariant(50),
              QVariant(2), QVariant(10000), QStringList(), QStringLiteral(""),
              QStringLiteral("Number of scale samples.")});
    p.append({QStringLiteral("vertices"), QStringLiteral("Initial simplices"),
              QStringLiteral("int"), QVariant(1000), QVariant(100), QVariant(100000), QStringList(),
              QStringLiteral(""), QStringLiteral("CDT manifold vertex count.")});
    return p;
}

ExperimentResult QuantumGeometryExperiment::run(const QVariantMap& params,
                                                std::function<void(double)> progress,
                                                std::function<bool()> shouldAbort) {
    ExperimentResult out;
    const double muMin = params.value(QStringLiteral("mu_min"), 0.1).toDouble();
    const double muMax = params.value(QStringLiteral("mu_max"), 100.0).toDouble();
    const int steps = params.value(QStringLiteral("steps"), 50).toInt();
    const int vertices = params.value(QStringLiteral("vertices"), 1000).toInt();

    out.scalars[QStringLiteral("mu_min")] = muMin;
    out.scalars[QStringLiteral("mu_max")] = muMax;
    out.scalars[QStringLiteral("steps")] = steps;
    out.scalars[QStringLiteral("vertices")] = vertices;

    if (steps < 2 || muMin <= 0.0 || muMax <= muMin) {
        out.success = false;
        return out;
    }

    quantumgravity::CDTEngine cdt(vertices);

    const int n = std::max(1, steps);
    double dsAtMaxMu = 0.0;

    for (int i = 0; i < n; ++i) {
        if (shouldAbort()) {
            out.success = false;
            return out;
        }

        const double t = (n > 1) ? static_cast<double>(i) / static_cast<double>(n - 1) : 0.0;
        const double mu = muMin + (muMax - muMin) * t;

        double ds = cdt.computeSpectralDimension(mu);
        if (!std::isfinite(ds))
            ds = 0.0;

        dsAtMaxMu = ds;
        out.curve.append(QPointF(mu, ds));
        out.logLines << QString("mu=%1  d_s=%2").arg(mu, 0, 'g', 4).arg(ds, 0, 'g', 4);
        progress(static_cast<double>(i + 1) / static_cast<double>(n));
    }

    out.scalars[QStringLiteral("d_s_at_mu_max")] = dsAtMaxMu;
    out.outputDir = QDir::currentPath();
    return out;
}

}  // namespace quantumverse
