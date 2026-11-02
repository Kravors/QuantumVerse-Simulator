/**
 * @file GWRingdownExperiment.cpp
 * @brief Kerr quasinormal mode ringdown waveform (l=m=2, n=0).
 *
 * QNM frequencies from Berti, Cardoso, Will 2006, Table VIII.
 * Linear interpolation over spin a*. Reference pins:
 *   M = 10 M_sun, a = 0.9: f_R ≈ 1700 Hz, tau ≈ 0.63 ms, Q ≈ 3.4
 *
 * Conversion: f_R [Hz] = (M*omega_R) * c^3 / (2*pi*G*M_kg)
 *             = (M*omega_R) * 32300 / (M/M_sun)
 *             tau [s]   = 1 / (2*pi * |M*omega_I| * 32300 / (M/M_sun))
 */

#include "GWRingdownExperiment.h"

#include <cmath>
#include <algorithm>

#include "spacetime/Event4D.h"
#include "spacetime/KerrMetric.h"

namespace quantumverse {

struct QNM {
    double M_omega_R;   // dimensionless real part
    double M_omega_I;   // dimensionless imaginary part (negative)
};

// Berti, Cardoso, Will 2006, Table VIII: l=m=2, n=0
static constexpr int    kTableSize = 5;
static constexpr double kA[kTableSize]   = {0.00, 0.50, 0.70, 0.90, 0.99};
static constexpr double kWR[kTableSize]  = {0.3737, 0.4628, 0.4949, 0.5260, 0.5383};
static constexpr double kWI[kTableSize]  = {-0.0890, -0.0865, -0.0845, -0.0810, -0.0786};

static QNM interpolateQNM(double a) {
    a = std::clamp(a, kA[0], kA[kTableSize - 1]);
    for (int i = 1; i < kTableSize; ++i) {
        if (a <= kA[i]) {
            const double t = (a - kA[i - 1]) / (kA[i] - kA[i - 1]);
            return QNM{
                kWR[i - 1] + t * (kWR[i] - kWR[i - 1]),
                kWI[i - 1] + t * (kWI[i] - kWI[i - 1])
            };
        }
    }
    return QNM{kWR[kTableSize - 1], kWI[kTableSize - 1]};
}

QVector<ExperimentParameter> GWRingdownExperiment::parameters() const {
    QVector<ExperimentParameter> p;
    p.append({QStringLiteral("mass_Msun"),   QStringLiteral("M (M\u2299)"),
              QStringLiteral("double"), QVariant(10.0), QVariant(1.0), QVariant(100.0),
              QStringList(), QStringLiteral("M\u2299"), QStringLiteral("Black hole mass.")});
    p.append({QStringLiteral("spin_a"),      QStringLiteral("a* = J/M^2"),
              QStringLiteral("double"), QVariant(0.9), QVariant(0.0), QVariant(0.99),
              QStringList(), QStringLiteral(""), QStringLiteral("Dimensionless spin.")});
    p.append({QStringLiteral("amplitude"),   QStringLiteral("Strain amplitude A"),
              QStringLiteral("double"), QVariant(1.0), QVariant(0.01), QVariant(10.0),
              QStringList(), QStringLiteral(""), QStringLiteral("Overall strain normalization.")});
    p.append({QStringLiteral("duration_ms"), QStringLiteral("Duration (ms)"),
              QStringLiteral("double"), QVariant(5.0), QVariant(0.5), QVariant(50.0),
              QStringList(), QStringLiteral("ms"), QStringLiteral("Waveform duration.")});
    p.append({QStringLiteral("n_steps"),     QStringLiteral("Samples"),
              QStringLiteral("int"), QVariant(500), QVariant(50), QVariant(2000),
              QStringList(), QStringLiteral(""), QStringLiteral("Number of time samples.")});
    p.append({QStringLiteral("compareToPublished"), QStringLiteral("Compare to published GW150914 values"),
              QStringLiteral("bool"), QVariant(false), QVariant(), QVariant(),
              QStringList(), QStringLiteral(""), QStringLiteral("If true, compute sigma-tension vs Abbott et al. 2016 GW150914 values.")});
    return p;
}

ExperimentResult GWRingdownExperiment::run(const QVariantMap& p,
                                           std::function<void(double)> progress,
                                           std::function<bool()> shouldAbort) {
    ExperimentResult out;

    const double M_msun = p.value(QStringLiteral("mass_Msun"),  10.0).toDouble();
    const double a      = p.value(QStringLiteral("spin_a"),     0.9).toDouble();
    const double A      = p.value(QStringLiteral("amplitude"),  1.0).toDouble();
    const double dur_ms = p.value(QStringLiteral("duration_ms"),5.0).toDouble();
    const int    N      = p.value(QStringLiteral("n_steps"),   500).toInt();

    if (!(M_msun > 0.0 && A > 0.0 && dur_ms > 0.0 && N >= 2)) {
        out.success = false;
        out.logLines << "Invalid parameters: need M_msun > 0, A > 0, dur_ms > 0, N >= 2";
        return out;
    }

    const QNM q = interpolateQNM(a);

    constexpr double HZ_PER_M_SUN = 32300.0;  // c^3 / (2*pi*G*M_sun) in Hz
    const double HZ_PER_M         = HZ_PER_M_SUN / M_msun;

    const double f_R   = q.M_omega_R * HZ_PER_M;
    const double tau_s = 1.0 / (2.0 * M_PI * std::abs(q.M_omega_I) * HZ_PER_M);
    const double Q     = M_PI * f_R * tau_s;

    // GW150914 reproduction: Abbott et al. 2016, PRL 116, 061102
    // Published: f_R = 251 +/- 5 Hz, tau = 4.0 +/- 0.6 ms
    const double f_R_pub   = 251.0;
    const double f_R_err   = 5.0;
    const double tau_pub   = 4.0;
    const double tau_err   = 0.6;

    out.scalars[QStringLiteral("f_R_Hz")]         = f_R;
    out.scalars[QStringLiteral("tau_ms")]         = tau_s * 1e3;
    out.scalars[QStringLiteral("Q_factor")]       = Q;
    out.scalars[QStringLiteral("M_omega_R")]      = q.M_omega_R;
    out.scalars[QStringLiteral("M_omega_I")]      = q.M_omega_I;
    out.scalars[QStringLiteral("mass_Msun")]      = M_msun;
    out.scalars[QStringLiteral("spin_a")]         = a;
    out.scalars[QStringLiteral("reference")]        = QStringLiteral("Abbott et al. 2016, PRL 116, 061102");
    out.scalars[QStringLiteral("source_system")]    = QStringLiteral("GW150914");
    out.scalars[QStringLiteral("f_R_published_Hz")]   = f_R_pub;
    out.scalars[QStringLiteral("f_R_published_err_Hz")] = f_R_err;
    out.scalars[QStringLiteral("tau_published_ms")]     = tau_pub;
    out.scalars[QStringLiteral("tau_published_err_ms")] = tau_err;
    out.scalars[QStringLiteral("f_R_delta_sigma")]    = (f_R - f_R_pub) / f_R_err;
    out.scalars[QStringLiteral("tau_delta_sigma")]    = (tau_s * 1e3 - tau_pub) / tau_err;

    const double dur_s = dur_ms * 1e-3;
    const int    n     = std::max(1, N);

    // Kretschmann scalar just outside the horizon (r_+ * 1.01).
    const double massKg = M_msun * 1.989e30;
    KerrMetric km(massKg, a);
    const double r_horizon = km.outerHorizonRadius();
    const double r_just_outside = r_horizon * 1.01;
    const auto curv = km.curvatureScalars(Event4D(0.0, r_just_outside, 0.0, 0.0));
    if (curv.valid) {
        out.scalars[QStringLiteral("kretschmann_at_r_plus")] = curv.kretschmann;
    }

    for (int i = 0; i < n; ++i) {
        if (shouldAbort()) { out.success = false; return out; }

        const double t_s   = dur_s * static_cast<double>(i) / static_cast<double>(n > 1 ? n - 1 : 1);
        const double h_norm = A * std::exp(-t_s / tau_s) * std::cos(2.0 * M_PI * f_R * t_s) / A;

        out.curve.append(QPointF(t_s * 1e3, h_norm));   // time in ms, normalized strain

        if (progress) progress(static_cast<double>(i + 1) / static_cast<double>(n));
    }

    out.scalars[QStringLiteral("compareToPublished")]
        = p.value(QStringLiteral("compareToPublished"), false).toBool();

    out.success = true;
    return out;
}

} // namespace quantumverse
