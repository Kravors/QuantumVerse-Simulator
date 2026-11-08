/**
 * @file CMBTemperatureExperiment.cpp
 * @brief Cosmology validation: CMB temperature T(z) = T₀(1+z).
 */

#include "CMBTemperatureExperiment.h"

#include <cmath>

namespace quantumverse {

QVector<ExperimentParameter> CMBTemperatureExperiment::parameters() const {
    return {};
}

ExperimentResult CMBTemperatureExperiment::run(const QVariantMap& /*params*/,
                                               std::function<void(double)> progress,
                                               std::function<bool()> shouldAbort) {
    ExperimentResult out;

    CosmologyReference t0ref = CosmologyRefs::Tcmb0;
    auto checkT0 =
        makeCheck(QStringLiteral("Tcmb_0_K"), m_project->Tcmb0(), m_reference->Tcmb0(), t0ref,
                  /*rel*/ 0.1, /*σ*/ 3.0);

    const double z1100 = 1100.0;
    const double t1100 = m_project->Tcmb0() * (1.0 + z1100);
    auto checkT1100 = makeCheck(QStringLiteral("Tcmb_z1100_K"), t1100,
                                m_reference->Tcmb0() * (1.0 + z1100), CosmologyRefs::TcmbAtZ1100,
                                /*rel*/ 1.0, /*σ*/ 0.0);

    recordCheck(out, checkT0);
    recordCheck(out, checkT1100);

    const int n = 200;
    for (int i = 0; i < n; ++i) {
        if (shouldAbort()) {
            out.success = false;
            return out;
        }
        const double z = i * 10.0;  // z = 0 .. 1990
        out.curve.append(QPointF(z, m_project->Tcmb0() * (1.0 + z)));
        if (progress)
            progress(double(i + 1) / n);
    }

    out.success = checkT0.pass && checkT1100.pass;
    out.logLines << QString("T_CMB(0):   project=%1 K  ref=%2 K  COBE=%3 K  %4")
                        .arg(checkT0.projectValue, 0, 'f', 4)
                        .arg(checkT0.referenceValue, 0, 'f', 4)
                        .arg(checkT0.observedValue, 0, 'f', 4)
                        .arg(checkT0.pass ? QStringLiteral("PASS") : QStringLiteral("FAIL"));
    out.logLines << QString("T_CMB(1100): project=%1 K  analytic=%2 K  %3")
                        .arg(checkT1100.projectValue, 0, 'f', 1)
                        .arg(checkT1100.referenceValue, 0, 'f', 1)
                        .arg(checkT1100.pass ? QStringLiteral("PASS") : QStringLiteral("FAIL"));
    return out;
}

}  // namespace quantumverse
