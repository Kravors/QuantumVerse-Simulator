/**
 * @file DistanceLadderExperiment.cpp
 * @brief Cosmology validation: distance modulus μ(z) vs SNe Ia (Union2.1).
 */

#include "DistanceLadderExperiment.h"

#include <cmath>

namespace quantumverse {

QVector<ExperimentParameter> DistanceLadderExperiment::parameters() const {
    return {};
}

ExperimentResult DistanceLadderExperiment::run(
    const QVariantMap& /*params*/,
    std::function<void(double)> progress,
    std::function<bool()> shouldAbort)
{
    ExperimentResult out;

    const double zs[] = { 0.1, 0.5, 1.0 };
    const CosmologyReference refs[] = {
        CosmologyRefs::MuAtZ01, CosmologyRefs::MuAtZ05, CosmologyRefs::MuAtZ1
    };
    bool allPass = true;
    for (int i = 0; i < 3; ++i) {
        const double z = zs[i];
        auto c = makeCheck(QStringLiteral("mu_z%1").arg(z),
                           m_project->distanceModulus(z),
                           m_reference->distanceModulus(z),
                           refs[i],
                           /*rel*/ 1.0, /*σ*/ 3.0);
        recordCheck(out, c);
        allPass = allPass && c.pass;
        out.logLines << QString("μ(z=%1): %2 mag  (ref %3, SNe %4, %5)")
                        .arg(z, 0, 'f', 1)
                        .arg(c.projectValue, 0, 'f', 2)
                        .arg(c.referenceValue, 0, 'f', 2)
                        .arg(c.observedValue, 0, 'f', 2)
                        .arg(c.pass ? QStringLiteral("PASS") : QStringLiteral("FAIL"));
    }

    const int n = 200;
    for (int i = 0; i < n; ++i) {
        if (shouldAbort()) { out.success = false; return out; }
        const double z = 0.001 + i * 0.02; // z ≈ 0 .. 4
        out.curve.append(QPointF(z, m_project->distanceModulus(z)));
        if (progress) progress(double(i + 1) / n);
    }

    out.success = allPass;
    return out;
}

} // namespace quantumverse
