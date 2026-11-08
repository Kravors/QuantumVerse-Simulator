/**
 * @file AgeOfUniverseExperiment.cpp
 * @brief Cosmology validation: age of the Universe vs Planck 2018.
 */

#include "AgeOfUniverseExperiment.h"

#include <cmath>

namespace quantumverse {

QVector<ExperimentParameter> AgeOfUniverseExperiment::parameters() const {
    return {};
}

ExperimentResult AgeOfUniverseExperiment::run(const QVariantMap& /*params*/,
                                              std::function<void(double)> progress,
                                              std::function<bool()> shouldAbort) {
    ExperimentResult out;

    auto check0 = makeCheck(QStringLiteral("age_universe_Gyr"), m_project->ageAtRedshift(0.0),
                            m_reference->ageAtRedshift(0.0), CosmologyRefs::AgeOfUniverse,
                            /*rel*/ 1.0, /*σ*/ 3.0);
    auto check1 = makeCheck(QStringLiteral("age_at_z1_Gyr"), m_project->ageAtRedshift(1.0),
                            m_reference->ageAtRedshift(1.0), CosmologyRefs::AgeAtZ1,
                            /*rel*/ 1.0, /*σ*/ 3.0);

    recordCheck(out, check0);
    recordCheck(out, check1);

    const int n = 100;
    for (int i = 0; i < n; ++i) {
        if (shouldAbort()) {
            out.success = false;
            return out;
        }
        const double z = i * 0.1;  // z = 0 .. 9.9
        out.curve.append(QPointF(z, m_project->ageAtRedshift(z)));
        if (progress)
            progress(double(i + 1) / n);
    }

    out.success = check0.pass && check1.pass;
    out.logLines << QString("Age:  project=%1  ref=%2  Planck=%3  Δ=%4σ  %5")
                        .arg(check0.projectValue, 0, 'f', 3)
                        .arg(check0.referenceValue, 0, 'f', 3)
                        .arg(check0.observedValue, 0, 'f', 3)
                        .arg(check0.tensionSigma, 0, 'f', 2)
                        .arg(check0.pass ? QStringLiteral("PASS") : QStringLiteral("FAIL"));
    out.logLines << QString("Age(z=1): project=%1  ref=%2  Planck=%3  %4")
                        .arg(check1.projectValue, 0, 'f', 3)
                        .arg(check1.referenceValue, 0, 'f', 3)
                        .arg(check1.observedValue, 0, 'f', 3)
                        .arg(check1.pass ? QStringLiteral("PASS") : QStringLiteral("FAIL"));
    return out;
}

}  // namespace quantumverse
