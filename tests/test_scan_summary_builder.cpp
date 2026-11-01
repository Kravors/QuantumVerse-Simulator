#include "test_assert.h"
#include "experiment/ScanSummaryBuilder.h"
#include "experiment/experiments/PhotonSphereExperiment.h"
#include "experiment/experiments/RedshiftProfileExperiment.h"
#include "experiment/experiments/FrameDraggingExperiment.h"

using namespace quantumverse;

static ExperimentResult makePhotonSphereResult(double spin = 0.99, double mass = 10.0) {
    PhotonSphereExperiment e;
    QVariantMap p;
    p["spin_a_max"] = spin;
    p["mass_Msun"]  = mass;
    p["n_steps"]    = 20;
    return e.run(p, [](double){}, [](){ return false; });
}

static ExperimentResult makeRedshiftProfileResult(double mass = 42.0) {
    RedshiftProfileExperiment e;
    QVariantMap p;
    p["mass"]   = mass;
    p["r_min"]  = 3000.0;
    p["r_max"]  = 1.0e6;
    p["steps"]  = 20;
    return e.run(p, [](double){}, [](){ return false; });
}

static ExperimentResult makeFrameDraggingResult(double mass = 42.0, double spin = 0.5) {
    FrameDraggingExperiment e;
    QVariantMap p;
    p["mass_Msun"]     = mass;
    p["spin_a_star"]   = spin;
    p["orbit_radius_m"] = 7.0e6;
    p["steps"]         = 20;
    return e.run(p, [](double){}, [](){ return false; });
}

int main() {
    // 1. Capability gate — all three supported
    QV_CHECK(ScanSummaryBuilder::canBuildFrom(QStringLiteral("photon_sphere")));
    QV_CHECK(ScanSummaryBuilder::canBuildFrom(QStringLiteral("redshift_profile")));
    QV_CHECK(ScanSummaryBuilder::canBuildFrom(QStringLiteral("frame_dragging")));
    QV_CHECK(!ScanSummaryBuilder::canBuildFrom(QStringLiteral("frb_generator")));
    QV_CHECK(!ScanSummaryBuilder::canBuildFrom(QStringLiteral("unknown_experiment")));

    // 2. Unsupported experiment → invalid summary (mass == 0 sentinel)
    {
        auto r = makePhotonSphereResult();
        auto s = ScanSummaryBuilder::fromExperiment(QStringLiteral("frb_generator"), r);
        QV_CHECK_NEAR(s.mass, 0.0, 1e-12);
    }

    // 3. Failed experiment result → invalid summary
    {
        ExperimentResult bad;
        bad.success = false;
        auto s = ScanSummaryBuilder::fromExperiment(QStringLiteral("photon_sphere"), bad);
        QV_CHECK_NEAR(s.mass, 0.0, 1e-12);
    }

    // 4. NON-VACUOUS: photon_sphere fields propagate from result, not defaults.
    {
        auto r = makePhotonSphereResult(/*spin=*/0.5, /*mass=*/42.0);
        auto s = ScanSummaryBuilder::fromExperiment(QStringLiteral("photon_sphere"), r);
        QV_CHECK_NEAR(s.mass,   42.0, 1e-9);   // NOT the default 10.0
        QV_CHECK_NEAR(s.spin,   0.5,  1e-9);   // NOT 0.99
        QV_CHECK(std::isfinite(s.kretschmann));
        QV_CHECK(s.kretschmann > 0.0);         // Kerr Kretschmann > 0 everywhere
        QV_CHECK(s.bhType == QStringLiteral("Kerr"));  // spin > 0 → Kerr
    }

    // 5. NON-VACUOUS: redshift_profile — mass uses "mass" key, spin=0 → Schwarzschild
    {
        auto r = makeRedshiftProfileResult();
        auto s = ScanSummaryBuilder::fromExperiment(QStringLiteral("redshift_profile"), r);
        QV_CHECK_NEAR(s.mass,   42.0, 1e-9);   // NOT default 10.0
        QV_CHECK_NEAR(s.spin,   0.0,  1e-12);  // Schwarzschild → spin=0
        QV_CHECK(std::isfinite(s.kretschmann));
        QV_CHECK(s.kretschmann > 0.0);         // Kretschmann > 0 at r_min
        QV_CHECK(s.bhType == QStringLiteral("Schwarzschild"));  // spin=0 → Schwarzschild
    }

    // 6. NON-VACUOUS: frame_dragging — spin uses "spin_a_star" key (alternate key)
    {
        auto r = makeFrameDraggingResult(/*mass=*/42.0, /*spin=*/0.5);
        auto s = ScanSummaryBuilder::fromExperiment(QStringLiteral("frame_dragging"), r);
        QV_CHECK_NEAR(s.mass,   42.0, 1e-9);   // NOT default 10.0
        QV_CHECK_NEAR(s.spin,   0.5,  1e-9);   // from spin_a_star key
        QV_CHECK(std::isfinite(s.kretschmann));
        QV_CHECK(s.kretschmann > 0.0);
        QV_CHECK(s.bhType == QStringLiteral("Kerr"));  // spin > 0 → Kerr
    }

    // 7. Missing kretschmann scalar → invalid summary (no silent default)
    {
        auto r = makePhotonSphereResult();
        r.scalars.remove(QStringLiteral("kretschmann_at_r_ph_pro"));
        auto s = ScanSummaryBuilder::fromExperiment(QStringLiteral("photon_sphere"), r);
        QV_CHECK_NEAR(s.mass, 0.0, 1e-12);   // builder refuses to guess
    }

    return 0;
}
