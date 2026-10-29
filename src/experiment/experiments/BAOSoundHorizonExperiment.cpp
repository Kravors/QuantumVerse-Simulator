/**
 * @file BAOSoundHorizonExperiment.cpp
 * @brief Cosmology validation: sound horizon at drag r_drag vs Planck 2018.
 */

#include "BAOSoundHorizonExperiment.h"

#include <cmath>

namespace quantumverse {

QVector<ExperimentParameter> BAOSoundHorizonExperiment::parameters() const {
    return {};
}

ExperimentResult BAOSoundHorizonExperiment::run(
    const QVariantMap& /*params*/,
    std::function<void(double)> progress,
    std::function<bool()> shouldAbort)
{
    ExperimentResult out;

    auto check = makeCheck(QStringLiteral("r_drag_Mpc"),
                           m_project->soundHorizon(),
                           m_reference->soundHorizon(),
                           CosmologyRefs::SoundHorizon,
                           /*rel*/ 1.0, /*σ*/ 3.0);
    recordCheck(out, check);

    // Context curve: H(z) over the drag-epoch range (cosmological, not the
    // sound-speed integral itself, which is not exposed by the provider).
    const int n = 200;
    for (int i = 0; i < n; ++i) {
        if (shouldAbort()) { out.success = false; return out; }
        const double z = i * 10.0; // z = 0 .. 1990
        out.curve.append(QPointF(z, m_project->H(z)));
        if (progress) progress(double(i + 1) / n);
    }

    out.success = check.pass;
    out.logLines << QString("r_drag: project=%1 Mpc  ref=%2 Mpc  Planck=%3 Mpc  Δ=%4σ  %5")
                    .arg(check.projectValue, 0, 'f', 2)
                    .arg(check.referenceValue, 0, 'f', 2)
                    .arg(check.observedValue, 0, 'f', 2)
                    .arg(check.tensionSigma, 0, 'f', 2)
                    .arg(check.pass ? QStringLiteral("PASS") : QStringLiteral("FAIL"));
    return out;
}

} // namespace quantumverse
