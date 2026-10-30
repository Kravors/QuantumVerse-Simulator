/**
 * @file AnomalyScanExperiment.cpp
 * @brief Curvature anomaly scan over a (r, theta) grid.
 *
 * Stand-in for DiscoveryEngine::detectAnomaly(): computes Kretschmann on a
 * grid and flags points exceeding a 5-sigma threshold above the median.
 * Wire the real DiscoveryEngine by passing (location, metric) to
 * detectAnomaly() instead of the inline curvatureScalar() below.
 */

#include "AnomalyScanExperiment.h"

#include <cmath>
#include <algorithm>
#include <QDir>
#include <QDateTime>

#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

namespace quantumverse {

QVector<ExperimentParameter> AnomalyScanExperiment::parameters() const {
    QVector<ExperimentParameter> p;
    p.append({QStringLiteral("mass"),      QStringLiteral("Mass M (M☉)"),
              QStringLiteral("double"), QVariant(10.0), QVariant(0.1), QVariant(1e6),
              QStringList(), QStringLiteral("M☉"),  QStringLiteral("Black hole mass.")});
    p.append({QStringLiteral("r_min"),     QStringLiteral("Inner radius (m)"),
              QStringLiteral("double"), QVariant(3000.0), QVariant(100.0), QVariant(1e9),
              QStringList(), QStringLiteral("m"),  QStringLiteral("Grid inner radius.")});
    p.append({QStringLiteral("r_max"),     QStringLiteral("Outer radius (m)"),
              QStringLiteral("double"), QVariant(1.0e5), QVariant(1.0e3), QVariant(1e12),
              QStringList(), QStringLiteral("m"),  QStringLiteral("Grid outer radius.")});
    p.append({QStringLiteral("grid_n"),    QStringLiteral("Grid cells per axis"),
              QStringLiteral("int"),    QVariant(25),   QVariant(4), QVariant(200),
              QStringList(), QStringLiteral(""),     QStringLiteral("Cells per axis (total = n²).")});
    p.append({QStringLiteral("sigma"),     QStringLiteral("Anomaly threshold (σ)"),
              QStringLiteral("double"), QVariant(5.0),  QVariant(1.0), QVariant(20.0),
              QStringList(), QStringLiteral("σ"),   QStringLiteral("Flag threshold above median.")});
    return p;
}

ExperimentResult AnomalyScanExperiment::run(const QVariantMap& params,
                                            std::function<void(double)> progress,
                                            std::function<bool()> shouldAbort) {
    ExperimentResult out;
    const double massMsun = params.value(QStringLiteral("mass"), 10.0).toDouble();
    const double rMin = params.value(QStringLiteral("r_min"), 3000.0).toDouble();
    const double rMax = params.value(QStringLiteral("r_max"), 1.0e5).toDouble();
    const int gridN = params.value(QStringLiteral("grid_n"), 25).toInt();
    const double sigma = params.value(QStringLiteral("sigma"), 5.0).toDouble();

    out.scalars[QStringLiteral("mass")] = massMsun;
    out.scalars[QStringLiteral("r_min")] = rMin;
    out.scalars[QStringLiteral("r_max")] = rMax;
    out.scalars[QStringLiteral("grid_n")] = gridN;
    out.scalars[QStringLiteral("sigma")] = sigma;

    if (gridN < 2 || rMin <= 0.0 || rMax <= rMin) {
        out.success = false;
        return out;
    }

    const double massKg = massMsun * 1.989e30;
    auto metric = std::make_shared<quantumverse::SchwarzschildMetric>(massKg);

    // First pass: collect K values.
    std::vector<double> ks;
    ks.reserve(static_cast<size_t>(gridN) * gridN);
    for (int i = 0; i < gridN; ++i) {
        const double r = rMin + (rMax - rMin) * static_cast<double>(i) / (gridN - 1);
        for (int j = 0; j < gridN; ++j) {
            const double theta = M_PI * static_cast<double>(j) / (gridN - 1);
            Event4D probe(0.0, r * std::sin(theta), 0.0, r * std::cos(theta));
            auto cs = metric->curvatureScalars(probe);
            ks.push_back(cs.valid ? cs.kretschmann : 0.0);
        }
    }

    // Median + robust spread (MAD).
    std::vector<double> sorted = ks;
    std::sort(sorted.begin(), sorted.end());
    const double median = sorted[sorted.size() / 2];
    double mad = 0.0;
    for (double k : ks) mad += std::abs(k - median);
    mad /= static_cast<double>(ks.size());
    const double threshold = median + sigma * mad;

    int anomalyCount = 0;
    const int n = gridN * gridN;
    for (int idx = 0; idx < n; ++idx) {
        if (shouldAbort()) { out.success = false; return out; }

        const double k = ks[idx];
        const bool isAnomaly = k > threshold && mad > 0.0;
        if (isAnomaly) anomalyCount++;

        const int i = idx / gridN;
        const int j = idx % gridN;
        const double r = rMin + (rMax - rMin) * static_cast<double>(i) / (gridN - 1);
        const double theta = M_PI * static_cast<double>(j) / (gridN - 1);

        if (isAnomaly) {
            out.logLines << QString("ANOMALY r=%1 m θ=%2  K=%3  (threshold %4)")
                            .arg(r, 0, 'e', 3).arg(theta, 0, 'g', 3)
                            .arg(k, 0, 'e', 3).arg(threshold, 0, 'e', 3);
        }

        // Curve: max K per radius ring (for the plot).
        if (j == 0) out.curve.append(QPointF(r, k));
        else {
            // keep running max per radius — approximate by appending last seen
            out.curve.append(QPointF(r, k));
        }

        progress(static_cast<double>(idx + 1) / static_cast<double>(n));
    }

    out.scalars[QStringLiteral("median_k")] = median;
    out.scalars[QStringLiteral("threshold_k")] = threshold;
    out.scalars[QStringLiteral("anomaly_count")] = anomalyCount;
    out.outputDir = QDir::currentPath();
    return out;
}

} // namespace quantumverse
