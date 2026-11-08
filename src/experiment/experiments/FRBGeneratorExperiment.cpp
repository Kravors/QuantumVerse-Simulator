/**
 * @file FRBGeneratorExperiment.cpp
 * @brief Dispersion measure vs redshift for the cosmic IGM (Macquart 2020).
 *
 * DM_IGM(z) = (3 c H0 Omega_b) / (8 pi G m_p) * f_IGM * (1 - Y/2)
 *           * integral_0^z (1+z')/E(z') dz'
 *
 * where E(z) = sqrt(Omega_M (1+z)^3 + Omega_Lambda).
 * Output in pc/cm^3 after unit conversion (1 pc/cm^3 = 3.085677581e22 m^-2).
 *
 * Fixed Planck 2018 parameters; Simpson integration per sample.
 */

#include "FRBGeneratorExperiment.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include "../cosmology/LambdaCDMReferenceProvider.h"
#include "../cosmology/ProjectCosmologyAdapter.h"

namespace quantumverse {

QVector<ExperimentParameter> FRBGeneratorExperiment::parameters() const {
    QVector<ExperimentParameter> p;
    p.append({QStringLiteral("H0"), QStringLiteral("H0 (km/s/Mpc)"), QStringLiteral("double"),
              QVariant(67.66), QVariant(50.0), QVariant(90.0), QStringList(),
              QStringLiteral("km/s/Mpc"), QStringLiteral("Hubble constant.")});
    p.append({QStringLiteral("OmegaM"), QStringLiteral("Omega_M"), QStringLiteral("double"),
              QVariant(0.3111), QVariant(0.10), QVariant(0.60), QStringList(), QStringLiteral(""),
              QStringLiteral("Matter density.")});
    p.append({QStringLiteral("OmegaB"), QStringLiteral("Omega_b"), QStringLiteral("double"),
              QVariant(0.049), QVariant(0.03), QVariant(0.08), QStringList(), QStringLiteral(""),
              QStringLiteral("Baryon density.")});
    p.append({QStringLiteral("OmegaL"), QStringLiteral("Omega_Lambda"), QStringLiteral("double"),
              QVariant(0.6889), QVariant(0.30), QVariant(0.90), QStringList(), QStringLiteral(""),
              QStringLiteral("Dark energy density.")});
    p.append({QStringLiteral("f_IGM"), QStringLiteral("IGM baryon fraction"),
              QStringLiteral("double"), QVariant(0.83), QVariant(0.50), QVariant(1.00),
              QStringList(), QStringLiteral(""), QStringLiteral("Fraction of baryons in IGM.")});
    p.append({QStringLiteral("z_max"), QStringLiteral("Max source redshift"),
              QStringLiteral("double"), QVariant(3.0), QVariant(0.10), QVariant(6.00),
              QStringList(), QStringLiteral(""), QStringLiteral("Highest redshift to sweep.")});
    p.append({QStringLiteral("n_steps"), QStringLiteral("Redshift samples"), QStringLiteral("int"),
              QVariant(30), QVariant(3), QVariant(100), QStringList(), QStringLiteral(""),
              QStringLiteral("Number of z points on the curve.")});
    p.append({QStringLiteral("cosmology_source"), QStringLiteral("Cosmology source"),
              QStringLiteral("enum"), QVariant(QStringLiteral("reference")), QVariant(), QVariant(),
              QStringList({QStringLiteral("reference"), QStringLiteral("project")}),
              QStringLiteral(""),
              QStringLiteral("Which H(z) to use: reference ΛCDM or project FLRW.")});
    return p;
}

ExperimentResult FRBGeneratorExperiment::run(const QVariantMap& p,
                                             std::function<void(double)> progress,
                                             std::function<bool()> shouldAbort) {
    ExperimentResult out;

    const QString cosmology_source =
        p.value(QStringLiteral("cosmology_source"), QStringLiteral("reference")).toString();

    const double H0_kms = p.value(QStringLiteral("H0"), 67.66).toDouble();
    const double Om = p.value(QStringLiteral("OmegaM"), 0.3111).toDouble();
    const double Ob = p.value(QStringLiteral("OmegaB"), 0.049).toDouble();
    const double OL = p.value(QStringLiteral("OmegaL"), 0.6889).toDouble();
    const double f_igm = p.value(QStringLiteral("f_IGM"), 0.83).toDouble();
    const double z_max = p.value(QStringLiteral("z_max"), 3.0).toDouble();
    const int n_steps = p.value(QStringLiteral("n_steps"), 30).toInt();

    if (!(H0_kms > 0.0 && Om > 0.0 && Ob > 0.0 && OL >= 0.0 && f_igm > 0.0 && z_max > 0.0 &&
          n_steps >= 2)) {
        out.success = false;
        out.logLines
            << "Invalid parameters: need H0, Om, Ob, f_igm, z_max > 0; OmegaL >= 0; n_steps >= 2";
        return out;
    }

    // Build the cosmology provider: reference (first-principles LambdaCDM) or
    // project (FRWMetric via ProjectCosmologyAdapter).
    std::shared_ptr<CosmologyProvider> provider;
    if (cosmology_source == QStringLiteral("project")) {
        provider =
            std::make_shared<ProjectCosmologyAdapter>(H0_kms, Om, Ob, 9.1e-5, OL, 0.0, 2.7255);
    } else {
        LambdaCDMReferenceProvider::Params pp;
        pp.H0 = H0_kms;
        pp.Om = Om;
        pp.Ob = Ob;
        pp.OL = OL;
        pp.Or = 9.1e-5;
        pp.Ok = 0.0;
        pp.Tcmb = 2.7255;
        provider = std::make_shared<LambdaCDMReferenceProvider>(pp);
    }

    constexpr double C_SI = 299792458.0;
    constexpr double G_SI = 6.67430e-11;
    constexpr double M_P_KG = 1.67262192e-27;
    constexpr double MPC_M = 3.085677581e22;
    constexpr double PC_M = 3.085677581e16;
    constexpr double Y_HELIUM = 0.24;

    const double H0_si = provider->H0() * 1e3 / MPC_M;
    const double rho_c0 = 3.0 * H0_si * H0_si / (8.0 * M_PI * G_SI);
    const double n_e_com = provider->OmegaB() * rho_c0 * f_igm * (1.0 - Y_HELIUM / 2.0) / M_P_KG;
    const double prefactor_m2 = n_e_com * C_SI / H0_si;
    const double m2_to_pccm3 = 1.0 / (PC_M * 1.0e6);

    auto Ez = [&](double z) {
        return provider->H(z) / provider->H0();  // dimensionless E(z)
    };
    auto integral = [&](double z_end, int n) {
        // Simpson's rule on int_0^z (1+z')/E(z') dz'
        if (n % 2 != 0)
            ++n;
        const double h = z_end / n;
        double s = 1.0 / Ez(0.0) + (1.0 + z_end) / Ez(z_end);
        for (int i = 1; i < n; ++i) {
            const double z = i * h;
            const double w = (i % 2 == 0) ? 2.0 : 4.0;
            s += w * (1.0 + z) / Ez(z);
        }
        return s * h / 3.0;
    };

    out.scalars[QStringLiteral("H0_kms")] = provider->H0();
    out.scalars[QStringLiteral("OmegaM")] = provider->OmegaM();
    out.scalars[QStringLiteral("OmegaB")] = provider->OmegaB();
    out.scalars[QStringLiteral("OmegaL")] = provider->OmegaLambda();
    out.scalars[QStringLiteral("f_IGM")] = f_igm;
    out.scalars[QStringLiteral("n_e_com_per_cm3")] = n_e_com * 1e-6;
    out.scalars[QStringLiteral("DM_at_z0")] = 0.0;
    out.scalars[QStringLiteral("cosmology_source")] = cosmology_source;

    const int n = std::max(1, n_steps);
    for (int i = 0; i < n; ++i) {
        if (shouldAbort()) {
            out.success = false;
            return out;
        }

        const double z = z_max * static_cast<double>(i + 1) / static_cast<double>(n);
        const double I = integral(z, 200);
        const double DM_pccm3 = prefactor_m2 * I * m2_to_pccm3;

        out.curve.append(QPointF(z, DM_pccm3));
        out.logLines << QString("z=%1  DM=%2 pc/cm3").arg(z, 0, 'f', 3).arg(DM_pccm3, 0, 'f', 1);

        if (progress)
            progress(static_cast<double>(i + 1) / static_cast<double>(n));
    }

    // Reference pins for tests (Macquart 2020 Fig. 1)
    for (double z_ref : {0.5, 1.0, 2.0}) {
        const double I = integral(z_ref, 200);
        const double DM_pccm3 = prefactor_m2 * I * m2_to_pccm3;
        out.scalars[QString("DM_at_z%1").arg(z_ref, 0, 'f', 1)] = DM_pccm3;
    }

    out.success = true;
    return out;
}

}  // namespace quantumverse
