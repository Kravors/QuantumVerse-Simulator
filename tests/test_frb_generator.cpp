#include "test_assert.h"
#include "experiment/experiments/FRBGeneratorExperiment.h"
#include <QVariantMap>

using namespace quantumverse;

static QVariantMap defaultParams() {
    QVariantMap p;
    p["H0"]      = 67.66;
    p["OmegaM"]  = 0.3111;
    p["OmegaB"]  = 0.049;
    p["OmegaL"]  = 0.6889;
    p["f_IGM"]   = 0.83;
    p["z_max"]   = 3.0;
    p["n_steps"] = 30;
    return p;
}

int main() {
    FRBGeneratorExperiment e;

    // Test 1: reference DM at z=1.0 (Macquart pin ~905 pc/cm^3, ±10%)
    {
        auto r = e.run(defaultParams(), [](double){}, [](){ return false; });
        QV_CHECK(r.success);
        const double dm = r.scalars["DM_at_z1.0"].toDouble();
        QV_CHECK(dm > 800.0);
        QV_CHECK(dm < 1000.0);
    }

    // Test 2: DM at z=0.5 vs z=1.0 ratio (~0.49 in LCDM)
    {
        auto r = e.run(defaultParams(), [](double){}, [](){ return false; });
        QV_CHECK(r.success);
        const double dm05 = r.scalars["DM_at_z0.5"].toDouble();
        const double dm10 = r.scalars["DM_at_z1.0"].toDouble();
        QV_CHECK_NEAR(dm05 / dm10, 0.49, 0.05);
    }

    // Test 3 (non-vacuous): DM scales linearly with OmegaB (OmegaB x2 => DM x2)
    {
        auto p1 = defaultParams();
        auto r1 = e.run(p1, [](double){}, [](){ return false; });
        auto p2 = defaultParams();
        p2["OmegaB"] = 0.098;   // 2x
        auto r2 = e.run(p2, [](double){}, [](){ return false; });
        QV_CHECK(r1.success && r2.success);
        const double ratio = r2.scalars["DM_at_z1.0"].toDouble()
                           / r1.scalars["DM_at_z1.0"].toDouble();
        QV_CHECK_NEAR(ratio, 2.0, 0.05);
    }

    // Test 4 (non-vacuous): DM scales linearly with f_IGM (f_IGM x0.5 => DM x0.5)
    {
        auto p1 = defaultParams();
        auto r1 = e.run(p1, [](double){}, [](){ return false; });
        auto p2 = defaultParams();
        p2["f_IGM"] = 0.415;   // half
        auto r2 = e.run(p2, [](double){}, [](){ return false; });
        QV_CHECK(r1.success && r2.success);
        const double ratio = r2.scalars["DM_at_z1.0"].toDouble()
                           / r1.scalars["DM_at_z1.0"].toDouble();
        QV_CHECK_NEAR(ratio, 0.5, 0.02);
    }

    // Test 5 (non-vacuous): DM monotonic increasing in z
    {
        auto r = e.run(defaultParams(), [](double){}, [](){ return false; });
        QV_CHECK(r.success);
        QV_CHECK(r.curve.size() >= 3);
        for (int i = 1; i < r.curve.size(); ++i) {
            QV_CHECK(r.curve[i].y() > r.curve[i-1].y());
        }
    }

    // Test 6 (non-vacuous): DM(z=0) is exactly zero by construction
    {
        auto r = e.run(defaultParams(), [](double){}, [](){ return false; });
        QV_CHECK(r.success);
        QV_CHECK_NEAR(r.scalars["DM_at_z0"].toDouble(), 0.0, 1e-12);
    }

    return 0;
}
