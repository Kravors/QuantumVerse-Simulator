/**
 * @file HubbleEvolutionExperiment.cpp
 * @brief Cosmology validation: H(z) evolution and the H₀ tension.
 */

#include "HubbleEvolutionExperiment.h"

#include <cmath>

namespace quantumverse {

QVector<ExperimentParameter> HubbleEvolutionExperiment::parameters() const {
    return {};
}

ExperimentResult HubbleEvolutionExperiment::run(
    const QVariantMap& /*params*/,
    std::function<void(double)> progress,
    std::function<bool()> shouldAbort)
{
    ExperimentResult out;

    const double h0 = m_project->H0();
    auto cH0P = makeCheck(QStringLiteral("H0_Planck"),
                          h0, m_reference->H0(),
                          CosmologyRefs::H0_Planck,
                          /*rel*/ 1.0, /*σ*/ 3.0);
    auto cH0S = makeCheck(QStringLiteral("H0_SH0ES"),
                          h0, m_reference->H0(),
                          CosmologyRefs::H0_SH0ES,
                          /*rel*/ 1.0, /*σ*/ 3.0);
    recordCheck(out, cH0P);
    recordCheck(out, cH0S);

    // q(0) is analytic: q0 = Ωₘ/2 − Ω_Λ
    auto cQ0 = makeCheck(QStringLiteral("q0"),
                         m_project->decelerationParameter(0.0),
                         m_reference->decelerationParameter(0.0),
                         CosmologyRefs::q0,
                         /*rel*/ 1.0, /*σ*/ 0.0);
    recordCheck(out, cQ0);

    // H₀ cannot satisfy both Planck and SH0ES simultaneously; the test
    // documents the tension and passes if at least one reference agrees.
    const bool h0ok = cH0P.pass || cH0S.pass;

    const int n = 200;
    for (int i = 0; i < n; ++i) {
        if (shouldAbort()) { out.success = false; return out; }
        const double z = i * 0.025; // z = 0 .. 4.975
        out.curve.append(QPointF(z, m_project->H(z)));
        if (progress) progress(double(i + 1) / n);
    }

    out.success = h0ok && cQ0.pass;
    out.logLines << QString("H₀: project=%1 km/s/Mpc  Planck=%2 (%3σ)  SH0ES=%4 (%5σ)  %6")
                    .arg(h0, 0, 'f', 2)
                    .arg(cH0P.observedValue, 0, 'f', 2)
                    .arg(cH0P.tensionSigma, 0, 'f', 2)
                    .arg(cH0S.observedValue, 0, 'f', 2)
                    .arg(cH0S.tensionSigma, 0, 'f', 2)
                    .arg(h0ok ? QStringLiteral("PASS") : QStringLiteral("FAIL"));
    out.logLines << QString("q(0): project=%1  analytic=%2  %3")
                    .arg(cQ0.projectValue, 0, 'f', 4)
                    .arg(cQ0.referenceValue, 0, 'f', 4)
                    .arg(cQ0.pass ? QStringLiteral("PASS") : QStringLiteral("FAIL"));
    return out;
}

} // namespace quantumverse
