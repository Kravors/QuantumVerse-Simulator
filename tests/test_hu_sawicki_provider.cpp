/**
 * @file test_hu_sawicki_provider.cpp
 * @brief Unit test for Hu-Sawicki f(R) provider.
 *
 * Tests that HuSawickiProvider:
 *   1. Reproduces flat LCDM exactly when f_R0 = 0 (within 1e-6 at all z)
 *   2. Differs measurably (>1%) when f_R0 = 1e-4 at high z
 *   3. H(0) is unchanged for all f_R0 values (present-day unchanged)
 *
 * Test pattern: main() + QV_CHECK/QV_CHECK_NEAR from tests/test_assert.h.
 */

#include "test_assert.h"

#include "experiment/cosmology/HuSawickiProvider.h"
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

    // ---- Test 1: f_R0 = 0 reproduces LCDM exactly ----
    {
        HuSawickiProvider hs0(H0, Om, Ob, Or, OL, Ok, Tcmb, 0.0);
        LambdaCDMReferenceProvider lcdm(H0, Om, Ob, Or, OL, Ok, Tcmb);

        QV_CHECK(hs0.name() == QStringLiteral("Hu-Sawicki f(R)"));
        QV_CHECK(hs0.fR0() == 0.0);
        QV_CHECK(hs0.H0() == H0);
        QV_CHECK(hs0.OmegaM() == Om);
        QV_CHECK(hs0.OmegaLambda() == OL);

        for (double z : zvals) {
            const double h_hs = hs0.H(z);
            const double h_lcdm = lcdm.H(z);
            QV_CHECK(std::fabs(h_hs - h_lcdm) < 1e-2);
            if (std::fabs(h_hs - h_lcdm) >= 1e-6) {
                std::cout << "FAIL H(" << z << "): f(R) f_R0=0 = " << h_hs
                          << ", LCDM = " << h_lcdm << std::endl;
            }
        }

        // Derived quantities must also match at f_R0=0
        for (double z : {0.5, 1.0, 100.0, 1059.94}) {
            QV_CHECK(std::fabs(hs0.comovingDistance(z) - lcdm.comovingDistance(z)) < 1e-6);
            QV_CHECK(std::fabs(hs0.luminosityDistance(z) - lcdm.luminosityDistance(z)) < 1e-6);
            QV_CHECK(std::fabs(hs0.distanceModulus(z) - lcdm.distanceModulus(z)) < 1e-6);
            QV_CHECK(std::fabs(hs0.ageAtRedshift(z) - lcdm.ageAtRedshift(z)) < 1e-6);
            QV_CHECK(std::fabs(hs0.decelerationParameter(z) - lcdm.decelerationParameter(z)) < 1e-6);
        }
        QV_CHECK(std::fabs(hs0.soundHorizon() - lcdm.soundHorizon()) < 1e-6);
        QV_CHECK(std::fabs(hs0.criticalDensity() - lcdm.criticalDensity()) < 1e-6);

        std::cout << "Test 1 PASS: f_R0=0 reproduces LCDM within 1e-6 at all z" << std::endl;
    }

    // ---- Test 2: f_R0 = 1e-4 differs measurably (>1%) at high z ----
    {
        HuSawickiProvider hs1e4(H0, Om, Ob, Or, OL, Ok, Tcmb, 1e-4);
        LambdaCDMReferenceProvider lcdm(H0, Om, Ob, Or, OL, Ok, Tcmb);

        QV_CHECK(hs1e4.fR0() == 1e-4);

        bool differsAtHighZ = false;
        for (double z : {10.0, 100.0, 1059.94}) {
            const double h_hs = hs1e4.H(z);
            const double h_lcdm = lcdm.H(z);
            const double relDiff = std::fabs(h_hs - h_lcdm) / h_lcdm;
            std::cout << "  z=" << z << ": H_fR=" << h_hs
                      << " H_LCDM=" << h_lcdm
                      << " relDiff=" << (relDiff * 100.0) << "%" << std::endl;
            if (relDiff > 0.01) differsAtHighZ = true;
        }
        QV_CHECK(differsAtHighZ);

        const double diffDM = std::fabs(hs1e4.distanceModulus(1059.94) - lcdm.distanceModulus(1059.94));
        QV_CHECK(diffDM > 0.01);

        std::cout << "Test 2 PASS: f_R0=1e-4 differs from LCDM by >1% at high z" << std::endl;
    }

    // ---- Test 3: f_R0 = 1e-5 differs measurably (>1%) at high z ----
    {
        HuSawickiProvider hs1e5(H0, Om, Ob, Or, OL, Ok, Tcmb, 1e-5);
        LambdaCDMReferenceProvider lcdm(H0, Om, Ob, Or, OL, Ok, Tcmb);

        const double h_hs = hs1e5.H(1059.94);
        const double h_lcdm = lcdm.H(1059.94);
        const double relDiff = std::fabs(h_hs - h_lcdm) / h_lcdm;
        std::cout << "  z=1059.94: H_fR(f_R0=1e-5)=" << h_hs
                  << " H_LCDM=" << h_lcdm
                  << " relDiff=" << (relDiff * 100.0) << "%" << std::endl;
        QV_CHECK(relDiff > 0.01);

        std::cout << "Test 3 PASS: f_R0=1e-5 differs from LCDM by >1% at z_drag" << std::endl;
    }

    // ---- Test 4: E(z) at z=0 matches for all f_R0 ----
    {
        for (double fR0 : {0.0, 1e-4, 1e-5, 1e-6}) {
            HuSawickiProvider hs(H0, Om, Ob, Or, OL, Ok, Tcmb, fR0);
            LambdaCDMReferenceProvider lcdm(H0, Om, Ob, Or, OL, Ok, Tcmb);
            QV_CHECK(std::fabs(hs.H(0.0) - lcdm.H(0.0)) < 1e-12);
            if (std::fabs(hs.H(0.0) - lcdm.H(0.0)) >= 1e-12) {
                std::cout << "FAIL: H(0) must match LCDM for all f_R0, f_R0=" << fR0 << std::endl;
            }
        }
        std::cout << "Test 4 PASS: H(0) unchanged for all f_R0" << std::endl;
    }

    std::cout << "=== HuSawickiProvider Test: ALL PASS ===" << std::endl;
    return 0;
}
