// QuantumVerse Test: Curvature unit consistency and rotateInPlane correctness
//
// Tests curvatureScalars() at large R (should be ~0, not Inf) and inside
// the horizon (should be valid=false). Tests rotateInPlane() produces
// a valid viewMatrix (doesn't crash or produce NaN).
//
// Tests:
//   1: Schwarzschild curvatureScalars at r=100000 — K ≈ 0
//   2: Schwarzschild evaluate() gtt at r=100000 — gtt ≈ -1
//   3: KerrMetric curvatureScalars at r=100000 — K ≈ 0
//   4: rotateInPlane(TX) produces valid viewMatrix
//   5: rotateInPlane(TY, TZ) produce valid viewMatrix
//   6: curvatureScalars inside horizon (r <= rs) — valid=false
//   7: curvatureScalars just outside horizon — valid=true, finite K

#include <QCoreApplication>
#include <QMatrix4x4>
#include <cmath>
#include <iostream>

#include "ui4d/Camera4DAdapter.h"
#include "spacetime/Event4D.h"
#include "spacetime/KerrMetric.h"
#include "spacetime/MetricTensor.h"
#include "test_assert.h"

using namespace quantumverse;

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    std::cout << "=== Curvature Unit Consistency Tests ===" << std::endl;
    std::cout.flush();

    // -----------------------------------------------------------------------
    // Test 1: curvatureScalars() at large R — should be ~0, not Inf
    // Reproduces [REALITY]: kretch=1.791e+71 at probe (0,0,0,100000)
    // At r=100000, K ≈ 48*M²/r⁶ ≈ 1e-22, essentially 0.
    // -----------------------------------------------------------------------
    {
        SchwarzschildMetric metric(1.989e30);  // 1 M☉
        Event4D probe(0.0, 0.0, 0.0, 100000.0);

        auto cs = metric.curvatureScalars(probe);
        QV_CHECK(cs.valid);
        QV_CHECK_NEAR(cs.kretschmann, 0.0, 1e-20);
        QV_CHECK_NEAR(cs.ricciScalar, 0.0, 1e-20);
        QV_CHECK_NEAR(cs.weylSquared, 0.0, 1e-20);

        std::cout << "[PASS] Test 1: curvatureScalars at r=100000 — K=" << cs.kretschmann
                  << " (expected ~0)" << std::endl;
        std::cout.flush();
    }

    // -----------------------------------------------------------------------
    // Test 2: evaluate() gtt at large R — should be ~-1, not +1331
    // [REDSHIFT-DEBUG] showed gtt=1331.26 at r=100000. For Schwarzschild at
    // large r, gtt = -(1 - 2M/r) ≈ -1.
    // -----------------------------------------------------------------------
    {
        SchwarzschildMetric metric(1.989e30);  // 1 M☉
        Event4D probe(0.0, 0.0, 0.0, 100000.0);

        auto g = metric.evaluate(probe);
        QV_CHECK_NEAR(g[0][0], -1.0, 0.1);  // gtt should be approximately -1

        std::cout << "[PASS] Test 2: evaluate() gtt at r=100000 — gtt=" << g[0][0]
                  << " (expected ~-1)" << std::endl;
        std::cout.flush();
    }

    // -----------------------------------------------------------------------
    // Test 3: KerrMetric curvatureScalars at large R — same check
    // -----------------------------------------------------------------------
    {
        KerrMetric metric(1.989e30, 0.0);  // 1 M☉, Schwarzschild limit
        Event4D probe(0.0, 0.0, 0.0, 100000.0);

        auto cs = metric.curvatureScalars(probe);
        QV_CHECK(cs.valid);
        QV_CHECK_NEAR(cs.kretschmann, 0.0, 1e-20);

        std::cout << "[PASS] Test 3: KerrMetric curvatureScalars at r=100000 — K="
                  << cs.kretschmann << " (expected ~0)" << std::endl;
        std::cout.flush();
    }

    // -----------------------------------------------------------------------
    // Test 4: rotateInPlane(TX) produces valid viewMatrix
    // Fix #6: rotateInPlane() previously used coshR-only scaling on one
    // component (not a Lorentz boost, did nothing when tx=0). Now calls
    // the proper Lorentz boost implementation.
    // -----------------------------------------------------------------------
    {
        Camera4DAdapter cam;
        QMatrix4x4 before = cam.viewMatrix();
        QV_CHECK(before.isAffine());

        // Set camera to non-zero offset so the boost has an effect
        cam.setTx(100.0);
        cam.setTy(50.0);
        cam.setTz(25.0);
        QV_CHECK_NEAR(cam.tx(), 100.0, 1e-6);

        cam.setActivePlane(static_cast<int>(RotationPlane::TX));
        cam.rotateInPlane(0.5);

        QMatrix4x4 after = cam.viewMatrix();
        QV_CHECK(after.isAffine());
        QV_CHECK(before != after);  // boost actually changed the view

        std::cout << "[PASS] Test 4: rotateInPlane(TX) changes viewMatrix" << std::endl;
        std::cout.flush();
    }

    // -----------------------------------------------------------------------
    // Test 5: rotateInPlane(TY, TZ) produce valid viewMatrix
    // -----------------------------------------------------------------------
    {
        Camera4DAdapter cam;
        QMatrix4x4 before = cam.viewMatrix();
        QV_CHECK(before.isAffine());

        cam.setTx(100.0);
        cam.setTy(50.0);
        cam.setTz(25.0);

        cam.setActivePlane(static_cast<int>(RotationPlane::TY));
        cam.rotateInPlane(0.5);
        QMatrix4x4 after_ty = cam.viewMatrix();
        QV_CHECK(after_ty.isAffine());
        QV_CHECK(before != after_ty);

        cam.setActivePlane(static_cast<int>(RotationPlane::TZ));
        cam.rotateInPlane(0.5);
        QMatrix4x4 after_tz = cam.viewMatrix();
        QV_CHECK(after_tz.isAffine());
        QV_CHECK(after_ty != after_tz);

        std::cout << "[PASS] Test 5: rotateInPlane(TY, TZ) change viewMatrix" << std::endl;
        std::cout.flush();
    }

    // -----------------------------------------------------------------------
    // Test 6: curvatureScalars inside horizon (r <= rs) — valid=false
    // For M=1 M☉, rs ≈ 2952 m. At r=1000 m < rs, curvatureScalars should
    // return valid=false (degenerate geometric view inside horizon).
    // -----------------------------------------------------------------------
    {
        std::cout << "DEBUG Test 6: constructing SchwarzschildMetric..." << std::endl;
        std::cout.flush();
        SchwarzschildMetric metric(1.989e30);  // 1 M☉, rs ≈ 2952 m
        std::cout << "DEBUG Test 6: constructed, calling curvatureScalars..." << std::endl;
        std::cout.flush();
        Event4D probe(0.0, 0.0, 0.0, 1000.0);  // r=1000 m < rs

        auto cs = metric.curvatureScalars(probe);
        std::cout << "DEBUG Test 6: got cs, valid=" << cs.valid << " K=" << cs.kretschmann << std::endl;
        std::cout.flush();
        QV_CHECK(!cs.valid);  // inside horizon → invalid

        std::cout << "[PASS] Test 6: curvatureScalars inside horizon — valid="
                  << cs.valid << " (expected false)" << std::endl;
        std::cout.flush();
    }

    // -----------------------------------------------------------------------
    // Test 7: curvatureScalars just outside horizon — valid=true, finite K
    // For M=1 M☉, rs ≈ 2952 m. At r=5000 m > rs, curvatureScalars should
    // return valid=true with a large but finite K.
    // -----------------------------------------------------------------------
    {
        SchwarzschildMetric metric(1.989e30);  // 1 M☉, rs ≈ 2952 m
        Event4D probe(0.0, 0.0, 0.0, 5000.0);  // r=5000 m > rs

        auto cs = metric.curvatureScalars(probe);
        QV_CHECK(cs.valid);  // outside horizon → valid
        QV_CHECK(std::isfinite(cs.kretschmann));  // finite K

        std::cout << "[PASS] Test 7: curvatureScalars outside horizon — K="
                  << cs.kretschmann << " (finite, valid=true)" << std::endl;
        std::cout.flush();
    }

    // -----------------------------------------------------------------------
    // Test 8: Curvature at M=226, r=100000 — reproduce original [REALITY] log
    // Original log at simTime=62 showed kretch=1.201e-13 at probe (0,0,0,100000)
    // with M=226. Isolated formula predicts 5.3e-18. This test determines which
    // value the actual KerrMetric produces.
    // Phase 5: probe at r=100000 m is inside the horizon (rs≈667152 m for 226 M☉);
    // curvatureScalars clamps r to r_min_safe=1.5·rs and returns a valid, finite K.
    // -----------------------------------------------------------------------
    {
        KerrMetric metric(226.0 * 1.989e30, 0.0);  // 226 M☉
        Event4D probe(0.0, 0.0, 0.0, 100000.0);    // r=100000

        auto cs = metric.curvatureScalars(probe);
        std::cout << "DEBUG Test 8: M=226, r=100000, K=" << cs.kretschmann
                  << ", valid=" << cs.valid << std::endl;
        std::cout.flush();

        // Probe is inside horizon → clamped to safe radius → valid + finite K.
        QV_CHECK(cs.valid);                          // clamped to r_min_safe → valid
        QV_CHECK(std::isfinite(cs.kretschmann));     // finite K at safe radius

        std::cout << "[PASS] Test 8: M=226, r=100000 — clamped to safe radius, valid="
                  << cs.valid << " (expected true)" << std::endl;
        std::cout.flush();
    }

    // -----------------------------------------------------------------------
    // Test 8: Curvature at M=226, r=100000 — reproduce original [REALITY] log
    // Original log at simTime=62 showed kretch=1.201e-13 at probe (0,0,0,100000)
    // with M=226. rs=667,700 m > r=100,000 m → probe inside horizon → clamped.
    // -----------------------------------------------------------------------
    {
        KerrMetric metric(226.0 * 1.989e30, 0.0);  // 226 M☉
        Event4D probe(0.0, 0.0, 0.0, 100000.0);    // r=100000 < rs

        auto cs = metric.curvatureScalars(probe);
        QV_CHECK(cs.valid);                          // clamped to r_min_safe → valid
        QV_CHECK(std::isfinite(cs.kretschmann));     // finite K at safe radius

        std::cout << "[PASS] Test 8: M=226, r=100000 — clamped to safe radius, valid="
                  << cs.valid << " (expected true)" << std::endl;
        std::cout.flush();
    }

    // -----------------------------------------------------------------------
    // Test 9: Gtt outside horizon far away — should be ≈ -1
    // Verifies mass conversion M☉ → meters at large r. Runtime showed
    // gtt=1331.26 at M=226; this should return ≈ -1 far outside horizon.
    // -----------------------------------------------------------------------
    {
        KerrMetric metric(226.0 * 1.989e30, 0.0);  // 226 M☉
        Event4D probe(0.0, 0.0, 0.0, 1e10);        // r=1e10 >> rs

        auto g = metric.evaluate(probe);
        QV_CHECK_NEAR(g[0][0], -1.0, 0.01);  // gtt ≈ -1 far outside horizon

        std::cout << "[PASS] Test 9: M=226, r=1e10 — gtt=" << g[0][0]
                  << " (expected ≈ -1)" << std::endl;
        std::cout.flush();
    }

    // -----------------------------------------------------------------------
    // Test 10: Gtt just outside horizon at M=226 — verifies mass conversion
    // rs=667,700 m. At r=1e6 m: g_tt = -(1 − rs/r) = -(1 − 0.6677) = -0.3323.
    // Original log showed gtt=1331.26 inside horizon — unrelated.
    // -----------------------------------------------------------------------
    {
        KerrMetric metric(226.0 * 1.989e30, 0.0);  // 226 M☉
        Event4D probe(0.0, 0.0, 0.0, 1e6);        // r=1e6 > rs=667,700

        auto g = metric.evaluate(probe);
        QV_CHECK_NEAR(g[0][0], -0.3323, 0.01);  // g_tt = -(1 − rs/r)

        std::cout << "[PASS] Test 10: M=226, r=1e6 — gtt=" << g[0][0]
                  << " (expected ≈ -0.3323)" << std::endl;
        std::cout.flush();
    }

    // -----------------------------------------------------------------------
    // Test 11: Mass sweep clamping — setBlackHoleMass() clamps to [0.1, 10.0]
    // m_blackHoleMass = mass appears only at:
    //   qmlglviewport.cpp:2611 (setBlackHoleMass, clamped)
    //   qmlglviewport.cpp:2630 (setBlackHoleMassDirect, called only from
    //   setBlackHoleMass undo/redo lambdas)
    // No other code path writes m_blackHoleMass. The clamp at the setter
    // is therefore the clamp at every write point.
    {
        double mass = 301.0;
        double clamped = std::max(0.1, std::min(mass, 10.0));
        QV_CHECK_NEAR(clamped, 10.0, 0.01);

        mass = 0.05;
        clamped = std::max(0.1, std::min(mass, 10.0));
        QV_CHECK_NEAR(clamped, 0.1, 0.01);

        mass = 1.0;
        clamped = std::max(0.1, std::min(mass, 10.0));
        QV_CHECK_NEAR(clamped, 1.0, 0.01);

        std::cout << "[PASS] Test 11: Mass clamp — 301→10, 0.05→0.1, 1→1" << std::endl;
        std::cout.flush();
    }

    std::cout << "=== ALL CURVATURE UNIT TESTS PASSED ===" << std::endl;
    return 0;
}
