/**
 * @file GrowthFactorExperiment.cpp
 * @brief Cosmology validation: linear growth factor D(z) vs Planck 2018.
 */

#include "GrowthFactorExperiment.h"

#include <cmath>

namespace quantumverse {

QVector<ExperimentParameter> GrowthFactorExperiment::parameters() const {
    return {};
}

ExperimentResult GrowthFactorExperiment::run(
    const QVariantMap& /*params*/,
    std::function<void(double)> progress,
    std::function<bool()> shouldAbort)
{
    ExperimentResult out;

    auto check = makeCheck(QStringLiteral("D_z1"),
                           m_project->growthFactor(1.0),
                           m_reference->growthFactor(1.0),
                           CosmologyRefs::GrowthFactorAtZ1,
                           /*rel*/ 2.0, /*σ*/ 3.0);
    recordCheck(out, check);

    const int n = 200;
    for (int i = 0; i < n; ++i) {
        if (shouldAbort()) { out.success = false; return out; }
        const double z = i * 0.05; // z = 0 .. 9.95
        out.curve.append(QPointF(z, m_project->growthFactor(z)));
        if (progress) progress(double(i + 1) / n);
    }

    out.success = check.pass;
    out.logLines << QString("D(z=1): project=%1  ref=%2  Planck=%3  Δ=%4%%  %5")
                    .arg(check.projectValue, 0, 'f', 4)
                    .arg(check.referenceValue, 0, 'f', 4)
                    .arg(check.observedValue, 0, 'f', 4)
                    .arg(check.relErrorPct, 0, 'f', 2)
                    .arg(check.pass ? QStringLiteral("PASS") : QStringLiteral("FAIL"));
    return out;
}

} // namespace quantumverse
