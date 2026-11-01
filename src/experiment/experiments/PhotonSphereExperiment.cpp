/**
 * @file PhotonSphereExperiment.cpp
 * @brief Kerr equatorial photon orbits vs spin (Bardeen 1972).
 *
 * Reference pins:
 *   Schwarzschild (a=0):   r_ph = 3M, b_c = 3√3 ≈ 5.196M
 *   Extremal (a→1):        r_ph^+ → M, r_ph^- → 4M, b_c^+ → 2M, b_c^- → 7M
 *   a=0.9:                 r_ph^+ ≈ 1.56M, r_ph^- ≈ 3.91M,
 *                          b_c^+ ≈ 2.84M, b_c^- ≈ 6.83M
 *
 * Curve: prograde r_ph/M vs spin a (monotonically decreasing).
 * Scalars: both directions at a=0 and a=a_max, plus asymmetry ratio.
 */

#include "PhotonSphereExperiment.h"

#include <cmath>
#include <algorithm>

namespace quantumverse {

QVector<ExperimentParameter> PhotonSphereExperiment::parameters() const {
    QVector<ExperimentParameter> p;
    p.append({QStringLiteral("spin_a_max"), QStringLiteral("Max spin a*"),
              QStringLiteral("double"), QVariant(0.99), QVariant(0.05), QVariant(0.999),
              QStringList(), QStringLiteral(""), QStringLiteral("Upper end of spin sweep.")});
    p.append({QStringLiteral("n_steps"),    QStringLiteral("Spin samples"),
              QStringLiteral("int"), QVariant(50), QVariant(10), QVariant(200),
              QStringList(), QStringLiteral(""), QStringLiteral("Number of spin samples on the curve.")});
    return p;
}

static double r_ph_over_M(double a, bool prograde) {
    // x = -a for prograde, x = +a for retrograde
    const double x = prograde ? -a : a;
    return 2.0 * (1.0 + std::cos((2.0 / 3.0) * std::acos(x)));
}

static double b_c_over_M(double a, bool prograde) {
    const double x = prograde ? -a : a;
    return prograde ? (-a + 6.0 * std::cos((1.0 / 3.0) * std::acos(x)))
                    : (+a + 6.0 * std::cos((1.0 / 3.0) * std::acos(x)));
}

ExperimentResult PhotonSphereExperiment::run(const QVariantMap& p,
                                             std::function<void(double)> progress,
                                             std::function<bool()> shouldAbort) {
    ExperimentResult out;

    const double a_max = p.value(QStringLiteral("spin_a_max"), 0.99).toDouble();
    const int    N     = p.value(QStringLiteral("n_steps"),   50).toInt();

    if (!(a_max > 0.0 && a_max < 1.0 && N >= 2)) {
        out.success = false;
        out.logLines << "Invalid parameters: need 0 < spin_a_max < 1, n_steps >= 2";
        return out;
    }

    const int n = std::max(1, N);

    // Main curve: prograde r_ph(a)/M vs spin a
    for (int i = 0; i < n; ++i) {
        if (shouldAbort()) { out.success = false; return out; }

        const double a = a_max * static_cast<double>(i) / static_cast<double>(n > 1 ? n - 1 : 1);
        const double r = r_ph_over_M(a, true);

        out.curve.append(QPointF(a, r));
        out.logLines << QString("a=%1  r_ph+/M=%2  r_ph-/M=%3")
                        .arg(a, 0, 'f', 4).arg(r, 0, 'f', 6)
                        .arg(r_ph_over_M(a, false), 0, 'f', 6);

        if (progress) progress(static_cast<double>(i + 1) / static_cast<double>(n));
    }

    // Schwarzschild limit scalars (a=0)
    out.scalars[QStringLiteral("r_ph_pro_a0")]    = r_ph_over_M(0.0,   true);
    out.scalars[QStringLiteral("r_ph_ret_a0")]    = r_ph_over_M(0.0,   false);
    out.scalars[QStringLiteral("b_c_pro_a0")]     = b_c_over_M(0.0,   true);
    out.scalars[QStringLiteral("b_c_ret_a0")]     = b_c_over_M(0.0,   false);

    // Extremal-limit scalars (a=a_max)
    out.scalars[QStringLiteral("r_ph_pro_a_max")] = r_ph_over_M(a_max, true);
    out.scalars[QStringLiteral("r_ph_ret_a_max")] = r_ph_over_M(a_max, false);
    out.scalars[QStringLiteral("b_c_pro_a_max")]  = b_c_over_M(a_max, true);
    out.scalars[QStringLiteral("b_c_ret_a_max")]  = b_c_over_M(a_max, false);

    const double b_c_pro  = b_c_over_M(a_max, true);
    const double b_c_ret  = b_c_over_M(a_max, false);
    out.scalars[QStringLiteral("shadow_asymmetry_a_max")] = b_c_pro / b_c_ret;
    out.scalars[QStringLiteral("spin_a_max")] = a_max;

    out.success = true;
    return out;
}

} // namespace quantumverse
