/**
 * @file LensingTimeDelayExperiment.cpp
 * @brief Strong-lensing time delay for a point-mass lens (thin-lens, fixed distances).
 *
 * Thin-lens point-mass lensing (Narayan & Bartelmann 1996, "Lectures on
 * Gravitational Lensing", eqs. 43-50):
 *
 *   theta_E = sqrt( 4 G M / c^2 * D_LS / (D_L * D_S) )
 *   beta    = theta - theta_E^2 / theta
 *   theta_\pm = ( beta +/- sqrt(beta^2 + 4 theta_E^2) ) / 2
 *   Delta t = (1 + z_L) * D_L * D_S / (c * D_LS) * Delta tau
 *
 * Reports |Delta t| as the delay; the signed Delta tau identifies which
 * image leads.
 */

#include "LensingTimeDelayExperiment.h"

#include <cmath>
#include <algorithm>

namespace quantumverse {

QVector<ExperimentParameter> LensingTimeDelayExperiment::parameters() const {
    QVector<ExperimentParameter> p;
    p.append({QStringLiteral("lens_mass_msun"), QStringLiteral("Lens mass (M\u2299)"),
              QStringLiteral("double"), QVariant(1.0e11), QVariant(1.0e8), QVariant(1.0e15),
              QStringList(), QStringLiteral("M\u2299"), QStringLiteral("Point-mass lens mass.")});
    p.append({QStringLiteral("lens_redshift"), QStringLiteral("Lens redshift z_L"),
              QStringLiteral("double"), QVariant(0.3), QVariant(0.0), QVariant(2.0),
              QStringList(), QStringLiteral(""), QStringLiteral("Lens redshift for the (1+z_L) factor.")});
    p.append({QStringLiteral("D_L_Gpc"), QStringLiteral("D_L (Gpc)"),
              QStringLiteral("double"), QVariant(1.0), QVariant(0.01), QVariant(10.0),
              QStringList(), QStringLiteral("Gpc"), QStringLiteral("Lens distance.")});
    p.append({QStringLiteral("D_S_Gpc"), QStringLiteral("D_S (Gpc)"),
              QStringLiteral("double"), QVariant(2.0), QVariant(0.02), QVariant(20.0),
              QStringList(), QStringLiteral("Gpc"), QStringLiteral("Source distance.")});
    p.append({QStringLiteral("D_LS_Gpc"), QStringLiteral("D_LS (Gpc)"),
              QStringLiteral("double"), QVariant(1.5), QVariant(0.01), QVariant(15.0),
              QStringList(), QStringLiteral("Gpc"), QStringLiteral("Lens-source distance.")});
    p.append({QStringLiteral("source_positions"), QStringLiteral("Beta samples"),
              QStringLiteral("int"), QVariant(20), QVariant(3), QVariant(100),
              QStringList(), QStringLiteral(""), QStringLiteral("Source offsets 0.01 -> 1.0 theta_E.")});
    return p;
}

ExperimentResult LensingTimeDelayExperiment::run(const QVariantMap& params,
                                                 std::function<void(double)> progress,
                                                 std::function<bool()> shouldAbort) {
    ExperimentResult out;

    const double M_lens  = params.value(QStringLiteral("lens_mass_msun"),   1.0e11).toDouble();
    const double z_L     = params.value(QStringLiteral("lens_redshift"),    0.3   ).toDouble();
    const double D_L     = params.value(QStringLiteral("D_L_Gpc"),          1.0   ).toDouble() * 3.085677581e25;
    const double D_S     = params.value(QStringLiteral("D_S_Gpc"),          2.0   ).toDouble() * 3.085677581e25;
    const double D_LS    = params.value(QStringLiteral("D_LS_Gpc"),         1.5   ).toDouble() * 3.085677581e25;
    const int    n_steps = params.value(QStringLiteral("source_positions"), 20    ).toInt();

    if (!(D_L > 0.0 && D_S > 0.0 && D_LS > 0.0 && D_L < D_S && D_LS < D_S)) {
        out.success = false;
        out.logLines << "Invalid distance configuration: need 0 < D_L < D_S and 0 < D_LS < D_S";
        return out;
    }

    const double G   = 6.67430e-11;
    const double c   = 299792458.0;
    const double M_sun_kg = 1.98892e30;
    const double M_kg    = M_lens * M_sun_kg;
    const double arcsecPerRad = 206264.806;
    const double secPerDay    = 86400.0;

    const double theta_E = std::sqrt(4.0 * G * M_kg / (c * c) * D_LS / (D_L * D_S));

    out.scalars[QStringLiteral("einstein_radius_arcsec")] = theta_E * arcsecPerRad;
    out.scalars[QStringLiteral("einstein_radius_rad")]    = theta_E;
    out.scalars[QStringLiteral("lens_mass_msun")]         = M_lens;
    out.scalars[QStringLiteral("lens_redshift")]          = z_L;
    out.scalars[QStringLiteral("D_L_Gpc")]                = D_L / 3.085677581e25;
    out.scalars[QStringLiteral("D_S_Gpc")]                = D_S / 3.085677581e25;
    out.scalars[QStringLiteral("D_LS_Gpc")]               = D_LS / 3.085677581e25;

    const double dt_factor = (1.0 + z_L) * D_L * D_S / (c * D_LS);

    const int n = std::max(1, n_steps);
    for (int i = 0; i < n; ++i) {
        if (shouldAbort()) { out.success = false; return out; }

        const double beta_frac = 0.01 + 0.99 * static_cast<double>(i) / static_cast<double>(n > 1 ? n - 1 : 1);
        const double beta      = beta_frac * theta_E;

        const double disc        = std::sqrt(beta * beta + 4.0 * theta_E * theta_E);
        const double theta_plus  = ( beta + disc) / 2.0;
        const double theta_minus = ( beta - disc) / 2.0;

        const double tau_plus  = 0.5 * (theta_plus  - beta) * (theta_plus  - beta)
                               - theta_E * theta_E * std::log(std::abs(theta_plus));
        const double tau_minus = 0.5 * (theta_minus - beta) * (theta_minus - beta)
                               - theta_E * theta_E * std::log(std::abs(theta_minus));

        const double dt_days = std::abs(dt_factor * (tau_plus - tau_minus)) / secPerDay;

        out.curve.append(QPointF(beta_frac, dt_days));
        out.logLines << QString("beta/theta_E=%1  theta_+=%2\"  theta_-=%3\"  Delta t=%4 d")
                        .arg(beta_frac, 0, 'f', 3)
                        .arg(theta_plus  * arcsecPerRad, 0, 'f', 3)
                        .arg(theta_minus * arcsecPerRad, 0, 'f', 3)
                        .arg(dt_days, 0, 'f', 2);

        if (progress) progress(static_cast<double>(i + 1) / static_cast<double>(n));
    }

    // Reference case pinned in scalars for the test (beta = 0.5 theta_E)
    const double beta_ref   = 0.5 * theta_E;
    const double disc_ref   = std::sqrt(beta_ref * beta_ref + 4.0 * theta_E * theta_E);
    const double tp_ref     = ( beta_ref + disc_ref) / 2.0;
    const double tm_ref     = ( beta_ref - disc_ref) / 2.0;
    const double tau_p_ref  = 0.5 * (tp_ref - beta_ref) * (tp_ref - beta_ref)
                            - theta_E * theta_E * std::log(std::abs(tp_ref));
    const double tau_m_ref  = 0.5 * (tm_ref - beta_ref) * (tm_ref - beta_ref)
                            - theta_E * theta_E * std::log(std::abs(tm_ref));
    const double dt_ref_day = std::abs(dt_factor * (tau_p_ref - tau_m_ref)) / secPerDay;

    out.scalars[QStringLiteral("reference_beta_frac")]  = 0.5;
    out.scalars[QStringLiteral("reference_delay_days")] = dt_ref_day;

    out.success = true;
    return out;
}

} // namespace quantumverse
