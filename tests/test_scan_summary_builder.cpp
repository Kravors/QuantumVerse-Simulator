#include "test_assert.h"
#include "experiment/ScanSummaryBuilder.h"
#include "experiment/experiments/PhotonSphereExperiment.h"

using namespace quantumverse;

static ExperimentResult makePhotonSphereResult(double spin = 0.99, double mass = 10.0) {
    PhotonSphereExperiment e;
    QVariantMap p;
    p["spin_a_max"] = spin;
    p["mass_Msun"]  = mass;
    p["n_steps"]    = 20;
    return e.run(p, [](double){}, [](){ return false; });
}

int main() {
    // 1. Capability gate
    QV_CHECK(ScanSummaryBuilder::canBuildFrom(QStringLiteral("photon_sphere")));
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

    // 4. NON-VACUOUS: fields must come from the result, not defaults.
    //    Run with NON-DEFAULT params and confirm they propagate.
    {
        auto r = makePhotonSphereResult(/*spin=*/0.5, /*mass=*/42.0);
        auto s = ScanSummaryBuilder::fromExperiment(QStringLiteral("photon_sphere"), r);
        QV_CHECK_NEAR(s.mass,   42.0, 1e-9);   // NOT the default 10.0
        QV_CHECK_NEAR(s.spin,   0.5,  1e-9);   // NOT 0.99
        QV_CHECK(std::isfinite(s.kretschmann));
        QV_CHECK(s.kretschmann > 0.0);         // Kerr Kretschmann > 0 everywhere
    }

    // 5. NON-VACUOUS: missing scalar in result → invalid summary (no silent default)
    {
        auto r = makePhotonSphereResult();
        r.scalars.remove(QStringLiteral("kretschmann_at_r_ph_pro")); // simulate experiment bug
        auto s = ScanSummaryBuilder::fromExperiment(QStringLiteral("photon_sphere"), r);
        QV_CHECK_NEAR(s.mass, 0.0, 1e-12);   // builder refuses to guess
    }

    return 0;
}
