#include "test_assert.h"
#include "experiment/experiments/PTASpectrumExperiment.h"
#include <QVariantMap>

using namespace quantumverse;

static QVariantMap defaultParams() {
    QVariantMap p;
    p["A_amplitude"]  = 2.4e-15;
    p["alpha"]        = -2.0/3.0;
    p["f_min_nHz"]    = 1.0;
    p["f_max_nHz"]    = 100.0;
    p["n_steps"]      = 40;
    return p;
}

int main() {
    PTASpectrumExperiment e;

    // Reference values (NANOGrav 15-year, A = 2.4e-15, alpha = -2/3, f_yr = 1/year):
    // h_c(1 nHz)  = 2.4e-15 * (1e-9 * 31557600)^(-2/3) ≈ 2.403e-14
    // h_c(10 nHz) = 2.4e-15 * (1e-8 * 31557600)^(-2/3) ≈ 5.184e-15
    // h_c(10 nHz)/h_c(1 nHz) = 10^(-2/3) ≈ 0.2154
    // slope_loglog = -2/3 ≈ -0.6667

    // Test 1: h_c at 1 nHz pin (~2.403e-14, ±1%)
    {
        auto r = e.run(defaultParams(), [](double){}, [](){ return false; });
        QV_CHECK(r.success);
        const double hc1 = r.scalars["h_c_at_1nHz"].toDouble();
        QV_CHECK_NEAR(hc1, 2.403e-14, 2.5e-16);
    }

    // Test 2: h_c(10 nHz) / h_c(1 nHz) = 10^(-2/3) ≈ 0.2154
    {
        auto r = e.run(defaultParams(), [](double){}, [](){ return false; });
        QV_CHECK(r.success);
        const double hc1  = r.scalars["h_c_at_1nHz"].toDouble();
        const double hc10 = r.scalars["h_c_at_10nHz"].toDouble();
        QV_CHECK_NEAR(hc10 / hc1, 0.2154, 0.003);
    }

    // Test 3 (non-vacuous): slope_loglog fitted from curve ≈ -2/3 (±0.01)
    {
        auto r = e.run(defaultParams(), [](double){}, [](){ return false; });
        QV_CHECK(r.success);
        const double slope = r.scalars["slope_loglog"].toDouble();
        QV_CHECK_NEAR(slope, -2.0/3.0, 0.01);
    }

    // Test 4 (non-vacuous): doubling A doubles h_c at all frequencies
    {
        auto p1 = defaultParams();
        auto r1 = e.run(p1, [](double){}, [](){ return false; });
        auto p2 = defaultParams();
        p2["A_amplitude"] = 4.8e-15;   // 2x
        auto r2 = e.run(p2, [](double){}, [](){ return false; });
        QV_CHECK(r1.success && r2.success);
        const double ratio = r2.scalars["h_c_at_1nHz"].toDouble()
                           / r1.scalars["h_c_at_1nHz"].toDouble();
        QV_CHECK_NEAR(ratio, 2.0, 0.01);
    }

    return 0;
}
