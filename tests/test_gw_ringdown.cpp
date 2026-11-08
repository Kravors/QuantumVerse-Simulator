#include <QVariantMap>
#include "experiment/experiments/GWRingdownExperiment.h"
#include "test_assert.h"

using namespace quantumverse;

static QVariantMap defaultParams() {
    QVariantMap p;
    p["mass_Msun"] = 10.0;
    p["spin_a"] = 0.9;
    p["amplitude"] = 1.0;
    p["duration_ms"] = 5.0;
    p["n_steps"] = 500;
    return p;
}

// Berti Table VIII reference pins:
//   M = 10 M_sun, a = 0.9: f_R ≈ 1700 Hz, tau ≈ 0.63 ms, Q ≈ 3.4
int main() {
    GWRingdownExperiment e;

    // Test 1: f_R pin at a=0.9, M=10 (Berti: 0.5260 * 32300/10 ≈ 1700 Hz)
    {
        auto r = e.run(defaultParams(), [](double) {}, []() { return false; });
        QV_CHECK(r.success);
        const double f_R = r.scalars["f_R_Hz"].toDouble();
        QV_CHECK_NEAR(f_R, 1700.0, 50.0);
    }

    // Test 2: tau pin at a=0.9, M=10 (Berti: tau ≈ 0.63 ms)
    {
        auto r = e.run(defaultParams(), [](double) {}, []() { return false; });
        QV_CHECK(r.success);
        const double tau = r.scalars["tau_ms"].toDouble();
        QV_CHECK_NEAR(tau, 0.63, 0.05);
    }

    // Test 3 (non-vacuous): f_R ∝ 1/M — double mass, half frequency
    {
        auto p1 = defaultParams();
        auto r1 = e.run(p1, [](double) {}, []() { return false; });

        auto p2 = defaultParams();
        p2["mass_Msun"] = 20.0;  // 2x
        auto r2 = e.run(p2, [](double) {}, []() { return false; });

        QV_CHECK(r1.success && r2.success);
        const double f_R1 = r1.scalars["f_R_Hz"].toDouble();
        const double f_R2 = r2.scalars["f_R_Hz"].toDouble();
        QV_CHECK_NEAR(f_R2 / f_R1, 0.5, 0.01);
    }

    // Test 4 (non-vacuous): tau ∝ M — double mass, double damping time
    {
        auto p1 = defaultParams();
        auto r1 = e.run(p1, [](double) {}, []() { return false; });

        auto p2 = defaultParams();
        p2["mass_Msun"] = 20.0;  // 2x
        auto r2 = e.run(p2, [](double) {}, []() { return false; });

        QV_CHECK(r1.success && r2.success);
        const double tau1 = r1.scalars["tau_ms"].toDouble();
        const double tau2 = r2.scalars["tau_ms"].toDouble();
        QV_CHECK_NEAR(tau2 / tau1, 2.0, 0.01);
    }

    // Test 5 (non-vacuous): Q decreases as spin a -> 0 (Schwarzschild damps faster)
    {
        auto p_high = defaultParams();
        p_high["spin_a"] = 0.9;
        auto r_high = e.run(p_high, [](double) {}, []() { return false; });

        auto p_low = defaultParams();
        p_low["spin_a"] = 0.0;
        auto r_low = e.run(p_low, [](double) {}, []() { return false; });

        QV_CHECK(r_high.success && r_low.success);
        const double Q_high = r_high.scalars["Q_factor"].toDouble();
        const double Q_low = r_low.scalars["Q_factor"].toDouble();
        QV_CHECK(Q_low < Q_high);  // high spin damps slower -> higher Q
    }

    return 0;
}
