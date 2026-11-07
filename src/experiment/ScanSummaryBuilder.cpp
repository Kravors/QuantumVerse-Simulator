#include "ScanSummaryBuilder.h"

#include <cmath>
#include <limits>

#include <QVariant>

namespace quantumverse {

bool ScanSummaryBuilder::canBuildFrom(const QString& id) {
    static const QStringList kSupported = { QStringLiteral("photon_sphere"),
                                            QStringLiteral("redshift_profile"),
                                            QStringLiteral("frame_dragging"),
                                            QStringLiteral("gw_ringdown"),
                                            QStringLiteral("theory_sensitivity"),
                                            QStringLiteral("mercury_precession") };
    return kSupported.contains(id);
}

double ScanSummaryBuilder::requireScalarAny(
    const ExperimentResult& r, const QStringList& keys, bool* ok) {
    for (const auto& k : keys) {
        if (r.scalars.contains(k)) {
            const double v = r.scalars.value(k).toDouble();
            if (std::isfinite(v)) {
                *ok = true;
                return v;
            }
        }
    }
    *ok = false;
    return std::numeric_limits<double>::quiet_NaN();
}

ScanSummary ScanSummaryBuilder::fromExperiment(const QString& id,
                                               const ExperimentResult& r) {
    ScanSummary s;

    // Invalid sentinel: mass == 0, kretschmann NaN, everything else zeroed.
    s.mass = 0.0;
    s.spin = 0.0;
    s.charge = 0.0;
    s.kretschmann = std::numeric_limits<double>::quiet_NaN();
    s.redshift = 0.0;

    if (!canBuildFrom(id)) return s;
    if (!r.success) return s;

    bool ok_mass = false;
    const double mass = requireScalarAny(r,
        { QStringLiteral("mass_Msun"), QStringLiteral("mass"), QStringLiteral("mass_sun_Msun") }, &ok_mass);
    if (!ok_mass) return s;

    bool ok_spin = false;
    double spin = 0.0; // default: Schwarzschild (non-rotating)
    const double spin_found = requireScalarAny(r,
        { QStringLiteral("spin_a_max"), QStringLiteral("spin_a_star"),
          QStringLiteral("spin_a"),   QStringLiteral("spin") }, &ok_spin);
    if (ok_spin) spin = spin_found;

    bool ok_kret = false;
    const double kret = requireScalarAny(r,
        { QStringLiteral("kretschmann_at_r_ph_pro"),
          QStringLiteral("kretschmann_at_r_min"),
          QStringLiteral("kretschmann_at_r_orbit"),
          QStringLiteral("kretschmann_at_r_plus") }, &ok_kret);

    s.spin = spin;
    s.bhType = (std::abs(spin) < 1e-9)
        ? QStringLiteral("Schwarzschild")
        : QStringLiteral("Kerr");
    // bhType derived from spin only — correct for Schwarzschild (spin=0, charge=0)
    // and Kerr (spin>0, charge=0). Silently mislabels Reissner-Nordström
    // (spin=0, charge>0 → "Schwarzschild") and Kerr-Newman (spin>0, charge>0 → "Kerr").
    // When a charged-metric experiment joins the pipeline, either:
    //   (a) requireScalar "bhType" from each experiment, or
    //   (b) extend derivation: charge>0 ? (spin>0 ? "Kerr-Newman" : "Reissner-Nordström") : ...
    if (!ok_kret) return s;
    s.mass = mass;
    s.charge = 0.0;
    s.kretschmann = kret;
    s.redshift = 0.0; // static observer at infinity — documentable constant
    s.discoveryId = QStringLiteral("exp_") + id;
    s.candidatesExplored = static_cast<int>(r.curve.size());
    s.extremalityMargin = spin; // a* → 1 is extremal

    // Reproduction-paper metadata: cite the reference and carry the observed
    // value being reproduced (e.g. GW150914 f_R = 251 Hz).
    const QString ref = r.scalars.value(QStringLiteral("reference")).toString();
    if (!ref.isEmpty()) s.citation = ref;

    bool ok_obs = false;
    const double obs = requireScalarAny(r,
        { QStringLiteral("f_R_Hz"), QStringLiteral("observedValue"),
          QStringLiteral("precession_arcsec_per_century") }, &ok_obs);
    if (ok_obs) s.observedValue = obs;

    return s;
}

} // namespace quantumverse
