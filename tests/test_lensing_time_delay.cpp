#include "test_assert.h"
#include "experiment/experiments/LensingTimeDelayExperiment.h"
#include <QVariantMap>

using namespace quantumverse;

static QVariantMap refParams() {
    QVariantMap p;
    p["lens_mass_msun"]   = 1.0e11;
    p["lens_redshift"]    = 0.3;
    p["D_L_Gpc"]          = 1.0;
    p["D_S_Gpc"]          = 2.0;
    p["D_LS_Gpc"]         = 1.5;
    p["source_positions"] = 20;
    return p;
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    LensingTimeDelayExperiment e;

    // Test 1: Einstein radius matches Python reference (0.782")
    {
        auto r = e.run(refParams(), [](double){}, [](){ return false; });
        QV_CHECK(r.success);
        const double theta_E = r.scalars["einstein_radius_arcsec"].toDouble();
        QV_CHECK_NEAR(theta_E, 0.782, 0.005);
    }

    // Test 2: reference delay matches Python reference (29.96 days)
    {
        auto r = e.run(refParams(), [](double){}, [](){ return false; });
        QV_CHECK(r.success);
        const double dt = r.scalars["reference_delay_days"].toDouble();
        QV_CHECK_NEAR(dt, 29.96, 0.5);
    }

    // Test 3 (non-vacuous): Δt ∝ M — doubling mass doubles delay
    {
        auto p1 = refParams();
        auto r1 = e.run(p1, [](double){}, [](){ return false; });

        auto p2 = refParams();
        p2["lens_mass_msun"] = 2.0e11;
        auto r2 = e.run(p2, [](double){}, [](){ return false; });

        QV_CHECK(r1.success && r2.success);
        const double ratio = r2.scalars["reference_delay_days"].toDouble()
                           / r1.scalars["reference_delay_days"].toDouble();
        QV_CHECK_NEAR(ratio, 2.0, 0.1);
    }

    // Test 4 (non-vacuous): Δt → 0 at perfect alignment (β → 0)
    {
        auto p = refParams();
        p["source_positions"] = 3;   // first sample is β = 0.01 θ_E
        auto r = e.run(p, [](double){}, [](){ return false; });
        QV_CHECK(r.success);
        QV_CHECK(!r.curve.isEmpty());
        const double dt_tiny = r.curve.first().y();
        QV_CHECK(dt_tiny < 1.0);   // << 29.96; catches sign / log-term bugs
    }

    return 0;
}
