#include "test_assert.h"
#include "experiment/experiments/PhotonSphereExperiment.h"
#include <QVariantMap>

using namespace quantumverse;

static QVariantMap defaultParams() {
    QVariantMap p;
    p["mass_Msun"]  = 10.0;
    p["spin_a_max"] = 0.99;
    p["n_steps"]    = 50;
    return p;
}

// Reference: Bardeen 1972 exact formulas
// Schwarzschild (a=0): r_ph = 3M both directions, b_c = 3√3 ≈ 5.196M
// At a=0.99 (near-extremal): r_ph+ ≈ 1.19M, r_ph- ≈ 3.99M,
//   b_c+ ≈ 2.26M, b_c- ≈ 6.98M, asymmetry ≈ 0.324
int main() {
    PhotonSphereExperiment e;

    // Test 1: Schwarzschild limit — both directions give r_ph = 3M
    {
        auto r = e.run(defaultParams(), [](double){}, [](){ return false; });
        QV_CHECK(r.success);
        QV_CHECK_NEAR(r.scalars["r_ph_pro_a0"].toDouble(),  3.0, 1e-6);
        QV_CHECK_NEAR(r.scalars["r_ph_ret_a0"].toDouble(),  3.0, 1e-6);
    }

    // Test 2: Schwarzschild limit — b_c = 3√3 ≈ 5.196M both directions
    {
        auto r = e.run(defaultParams(), [](double){}, [](){ return false; });
        QV_CHECK(r.success);
        QV_CHECK_NEAR(r.scalars["b_c_pro_a0"].toDouble(),  5.196152, 1e-4);
        QV_CHECK_NEAR(r.scalars["b_c_ret_a0"].toDouble(),  5.196152, 1e-4);
    }

    // Test 3 (non-vacuous): prograde < retrograde for a > 0 (asymmetry)
    {
        auto r = e.run(defaultParams(), [](double){}, [](){ return false; });
        QV_CHECK(r.success);
        QV_CHECK(r.scalars["r_ph_pro_a_max"].toDouble()
               < r.scalars["r_ph_ret_a_max"].toDouble());
    }

    // Test 4 (non-vacuous): near-extremal prograde r_ph → M
    {
        auto r = e.run(defaultParams(), [](double){}, [](){ return false; });
        QV_CHECK(r.success);
        QV_CHECK(r.scalars["r_ph_pro_a_max"].toDouble() > 1.0);
        QV_CHECK(r.scalars["r_ph_pro_a_max"].toDouble() < 1.5);
    }

    // Test 5 (non-vacuous): near-extremal retrograde r_ph → 4M
    {
        auto r = e.run(defaultParams(), [](double){}, [](){ return false; });
        QV_CHECK(r.success);
        QV_CHECK(r.scalars["r_ph_ret_a_max"].toDouble() > 3.5);
        QV_CHECK(r.scalars["r_ph_ret_a_max"].toDouble() < 4.0);
    }

    // Test 6 (non-vacuous): shadow asymmetry ratio ≈ 2/7 ≈ 0.286 at near-extremal
    {
        auto r = e.run(defaultParams(), [](double){}, [](){ return false; });
        QV_CHECK(r.success);
        const double asym = r.scalars["shadow_asymmetry_a_max"].toDouble();
        QV_CHECK(asym > 0.25);
        QV_CHECK(asym < 0.40);
    }

    // Test 7 (non-vacuous): Kretschmann K > 0 at prograde photon sphere —
    // real curvature, not fabricated. K = 0 would mean flat spacetime,
    // which Kerr is not.
    {
        auto r = e.run(defaultParams(), [](double){}, [](){ return false; });
        QV_CHECK(r.success);
        const double k_pro = r.scalars["kretschmann_at_r_ph_pro"].toDouble();
        QV_CHECK(std::isfinite(k_pro));
        QV_CHECK(k_pro > 0.0);
    }

    // Test 8 (non-vacuous): Kretschmann K > 0 at retrograde photon sphere.
    {
        auto r = e.run(defaultParams(), [](double){}, [](){ return false; });
        QV_CHECK(r.success);
        const double k_ret = r.scalars["kretschmann_at_r_ph_ret"].toDouble();
        QV_CHECK(std::isfinite(k_ret));
        QV_CHECK(k_ret > 0.0);
    }

    // Test 9 (non-vacuous): K at prograde photon sphere > K at Schwarzschild
    // (r_ph+ < 3M for a* > 0, and K ~ 1/r^6 on equator).
    {
        auto r_prj = e.run(defaultParams(), [](double){}, [](){ return false; });
        QV_CHECK(r_prj.success);
        auto p_sch = defaultParams();
        p_sch["spin_a_max"] = 0.0;
        auto r_sch = e.run(p_sch, [](double){}, [](){ return false; });
        QV_CHECK(r_sch.success);
        QV_CHECK(r_prj.scalars["kretschmann_at_r_ph_pro"].toDouble()
               > r_sch.scalars["kretschmann_at_r_ph_pro"].toDouble());
    }

    return 0;
}
