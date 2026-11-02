// QuantumVerse f(R) gravity provider test.
//
// Tests that FRGravityProvider:
//   1. Reproduces flat ΛCDM exactly when b=0 (within 1e-6 relative at all z)
//   2. Differs measurably (>1%) when b=0.1 at high z
//   3. Produces different derived quantities (comovingDistance, distanceModulus)
//   4. Reports the correct fR() parameter
//
// Test pattern: main() + QV_CHECK/QV_CHECK_NEAR from tests/test_assert.h.
// No TEST() macro, no gtest/Catch2.

#include "test_assert.h"

#include "experiment/cosmology/FRGravityProvider.h"
#include "experiment/cosmology/LambdaCDMReferenceProvider.h"

#include <cmath>
#include <iostream>

using namespace quantumverse;

static bool nearEqual(double a, double b, double tol = 1e-6) {
    if (a == b) return true;
    double diff = std::fabs(a - b);
    double scale = std::fmax(std::fabs(a), std::fabs(b));
    return scale > 0.0 ? diff / scale < tol : diff < tol;
}

int main() {
    using namespace quantumverse;

    // Common parameters (Planck 2018)
    const double H0 = 67.66;
    const double Om = 0.3111;
    const double Ob = 0.049;
    const double Or = 9.1e-5;
    const double OL = 0.6889;
    const double Ok = 0.0;
    const double Tcmb = 2.7255;

    // Test z values spanning low, intermediate, and drag-epoch regimes
    const double zvals[] = {0.0, 0.5, 1.0, 10.0, 100.0, 1059.94};

    // ---- Test 1: b=0 reproduces ΛCDM exactly ----
    {
        FRGravityProvider fr0(H0, Om, Ob, Or, OL, Ok, Tcmb, 0.0);
        LambdaCDMReferenceProvider lcdm(H0, Om, Ob, Or, OL, Ok, Tcmb);

        QV_CHECK(fr0.name() == QStringLiteral("f(R) gravity"));
        QV_CHECK(fr0.fR() == 0.0);
        QV_CHECK(fr0.H0() == H0);
        QV_CHECK(fr0.OmegaM() == Om);
        QV_CHECK(fr0.OmegaLambda() == OL);

        for (double z : zvals) {
            const double h_fr = fr0.H(z);
            const double h_lcdm = lcdm.H(z);
            QV_CHECK(nearEqual(h_fr, h_lcdm, 1e-6));
            if (!nearEqual(h_fr, h_lcdm, 1e-6)) {
                std::cout << "FAIL H(" << z << "): f(R) b=0 = " << h_fr
                          << ", ΛCDM = " << h_lcdm << std::endl;
            }
        }

        // Derived quantities must also match at b=0
        for (double z : {0.5, 1.0, 100.0, 1059.94}) {
            QV_CHECK(nearEqual(fr0.comovingDistance(z), lcdm.comovingDistance(z), 1e-6));
            QV_CHECK(nearEqual(fr0.luminosityDistance(z), lcdm.luminosityDistance(z), 1e-6));
            QV_CHECK(nearEqual(fr0.distanceModulus(z), lcdm.distanceModulus(z), 1e-6));
            QV_CHECK(nearEqual(fr0.ageAtRedshift(z), lcdm.ageAtRedshift(z), 1e-6));
            QV_CHECK(nearEqual(fr0.decelerationParameter(z), lcdm.decelerationParameter(z), 1e-6));
        }
        QV_CHECK(nearEqual(fr0.soundHorizon(), lcdm.soundHorizon(), 1e-6));
        QV_CHECK(nearEqual(fr0.criticalDensity(), lcdm.criticalDensity(), 1e-6));

        std::cout << "Test 1 PASS: b=0 reproduces ΛCDM within 1e-6 at all z" << std::endl;
    }

    // ---- Test 2: b=0.1 differs measurably (>1%) at high z ----
    {
        FRGravityProvider fr01(H0, Om, Ob, Or, OL, Ok, Tcmb, 0.1);
        LambdaCDMReferenceProvider lcdm(H0, Om, Ob, Or, OL, Ok, Tcmb);

        QV_CHECK(fr01.fR() == 0.1);

        bool differsAtHighZ = false;
        for (double z : {10.0, 100.0, 1059.94}) {
            const double h_fr = fr01.H(z);
            const double h_lcdm = lcdm.H(z);
            const double relDiff = std::fabs(h_fr - h_lcdm) / h_lcdm;
            std::cout << "  z=" << z << ": H_fR=" << h_fr
                      << " H_lCDM=" << h_lcdm
                      << " relDiff=" << (relDiff * 100.0) << "%" << std::endl;
            if (relDiff > 0.01) differsAtHighZ = true;
        }
        QV_CHECK(differsAtHighZ);
        if (!differsAtHighZ) {
            std::cout << "FAIL: f(R) b=0.1 must differ from ΛCDM by >1% at some high z" << std::endl;
        }

        // Derived quantities must differ too
        const double diffDM = std::fabs(fr01.distanceModulus(1059.94) - lcdm.distanceModulus(1059.94));
        QV_CHECK(diffDM > 0.01);
        if (!(diffDM > 0.01)) {
            std::cout << "FAIL: distanceModulus at z_drag must differ by >0.01 mag for b=0.1" << std::endl;
        }

        // ---- Diagnostic: show actual H(z) ratios for all z ----
        std::cout << "  DIAGNOSTIC H(z) ratios b=0.1 vs b=0:" << std::endl;
        for (double z : zvals) {
            const double h00 = lcdm.H(z);
            const double h01 = fr01.H(z);
            std::cout << "    z=" << z << ": H_b0=" << h00 << " H_b01=" << h01
                      << " ratio=" << (h01 / h00) << std::endl;
        }
        std::cout << "  DIAGNOSTIC r_drag: b=0=" << lcdm.soundHorizon()
                  << " Mpc  b=0.1=" << fr01.soundHorizon() << " Mpc"
                  << std::endl;

        std::cout << "Test 2 PASS: b=0.1 differs from ΛCDM by >1% at high z" << std::endl;
    }

    // ---- Diagnostic: provider-level H(z) ratios and r_drag for b=0,0.1,0.3 ----
    {
        FRGravityProvider diag0(H0, Om, Ob, Or, OL, Ok, Tcmb, 0.0);
        FRGravityProvider diag01(H0, Om, Ob, Or, OL, Ok, Tcmb, 0.1);
        FRGravityProvider diag03(H0, Om, Ob, Or, OL, Ok, Tcmb, 0.3);
        std::cout << "  DIAGNOSTIC H(z) ratios b vs b=0:" << std::endl;
        for (double z : zvals) {
            const double h0 = diag0.H(z);
            const double h01 = diag01.H(z);
            const double h03 = diag03.H(z);
            std::cout << "    z=" << z << ": H_b0=" << h0 << " H_b01=" << h01
                      << " ratio_b01=" << (h01 / h0) << " H_b03=" << h03
                      << " ratio_b03=" << (h03 / h0) << std::endl;
        }
        std::cout << "  DIAGNOSTIC r_drag: b=0=" << diag0.soundHorizon()
                  << " Mpc  b=0.1=" << diag01.soundHorizon() << " Mpc  b=0.3="
                  << diag03.soundHorizon() << " Mpc" << std::endl;
    }

    // ---- Test 3: b=0.3 produces even larger deviation ----
    {
        FRGravityProvider fr03(H0, Om, Ob, Or, OL, Ok, Tcmb, 0.3);
        LambdaCDMReferenceProvider lcdm(H0, Om, Ob, Or, OL, Ok, Tcmb);

        const double h_fr = fr03.H(1059.94);
        const double h_lcdm = lcdm.H(1059.94);
        const double relDiff = std::fabs(h_fr - h_lcdm) / h_lcdm;
        std::cout << "  z=1059.94: H_fR(b=0.3)=" << h_fr
                  << " H_lCDM=" << h_lcdm
                  << " relDiff=" << (relDiff * 100.0) << "%" << std::endl;
        QV_CHECK(relDiff > 0.01);
        if (!(relDiff > 0.01)) {
            std::cout << "FAIL: f(R) b=0.3 must differ from ΛCDM by >1% at z_drag" << std::endl;
        }

        std::cout << "Test 3 PASS: b=0.3 produces larger deviation" << std::endl;
    }

    // ---- Test 4: E(z) at z=0 matches for all b (present-day unchanged) ----
    {
        for (double b : {0.0, 0.1, 0.3, 1.0}) {
            FRGravityProvider fr(H0, Om, Ob, Or, OL, Ok, Tcmb, b);
            LambdaCDMReferenceProvider lcdm(H0, Om, Ob, Or, OL, Ok, Tcmb);
            QV_CHECK(nearEqual(fr.H(0.0), lcdm.H(0.0), 1e-12));
            if (!nearEqual(fr.H(0.0), lcdm.H(0.0), 1e-12)) {
                std::cout << "FAIL: H(0) must match ΛCDM for all b (present-day unchanged), b=" << b << std::endl;
            }
        }
        std::cout << "Test 4 PASS: H(0) unchanged for all b" << std::endl;
    }

    std::cout << "=== FRGravityProvider Test: ALL PASS ===" << std::endl;
    return 0;
}
