#include "ScanSummaryBuilder.h"

#include <cmath>
#include <limits>

#include <QVariant>

namespace quantumverse {

bool ScanSummaryBuilder::canBuildFrom(const QString& id) {
    static const QStringList kSupported = { QStringLiteral("photon_sphere") };
    return kSupported.contains(id);
}

double ScanSummaryBuilder::requireScalar(const ExperimentResult& r,
                                         const QString& key, bool* ok) {
    if (!r.scalars.contains(key)) {
        *ok = false;
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double v = r.scalars.value(key).toDouble();
    if (!std::isfinite(v)) {
        *ok = false;
        return v;
    }
    *ok = true;
    return v;
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

    bool ok = false;

    // mass — echoed into scalars by PhotonSphereExperiment::run()
    const double mass = requireScalar(r, QStringLiteral("mass_Msun"), &ok);
    if (!ok) return s;

    // spin — real measurement from the result
    const double spin = requireScalar(r, QStringLiteral("spin_a_max"), &ok);
    if (!ok) return s;

    // kretschmann — real curvature scalar from the experiment
    const double kret = requireScalar(r,
                                      QStringLiteral("kretschmann_at_r_ph_pro"),
                                      &ok);
    if (!ok) return s;

    s.mass = mass;
    s.spin = spin;
    s.charge = 0.0;   // Kerr is vacuum — documentable constant
    s.kretschmann = kret;
    s.redshift = 0.0; // static observer at infinity — documentable constant
    s.bhType = QStringLiteral("Kerr");
    s.discoveryType = id;
    s.discoveryId = QStringLiteral("exp_") + id;
    s.candidatesExplored = static_cast<int>(r.curve.size());
    s.extremalityMargin = spin; // a* → 1 is extremal

    return s;
}

} // namespace quantumverse
