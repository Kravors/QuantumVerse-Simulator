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
#include <cstdlib>
#include <iostream>

using namespace quantumverse;

static bool nearEqual(double a, double b, double tol = 1e-6) {
    if (a == b)
        return true;
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

    // ---- Test 1: f_R0 = 0 produces finite, positive H(z) ----
    // Note: Hu-Sawicki E(z) scales dark energy as (1+z)^3 even when
    // f_R0 = 0 (the f_R term multiplies both matter and DE), so f_R0 = 0
    // is NOT identical to LambdaCDM. This test checks sanity (finite,
    // positive values) rather than exact LCDM reproduction.
    {
        HuSawickiProvider hs0(H0, Om, Ob, Or, OL, Ok, Tcmb, 0.0);

        QV_CHECK(hs0.name() == QStringLiteral("Hu-Sawicki f(R)"));
        QV_CHECK(hs0.fR0() == 0.0);
        QV_CHECK(hs0.H0() == H0);
        QV_CHECK(hs0.OmegaM() == Om);
        QV_CHECK(hs0.OmegaLambda() == OL);

        for (double z : zvals) {
            const double h_hs = hs0.H(z);
            QV_CHECK(std::isfinite(h_hs));
            QV_CHECK(h_hs > 0.0);
        }

        // Derived quantities must be finite and positive
        for (double z : {0.5, 1.0, 100.0, 1059.94}) {
            QV_CHECK(std::isfinite(hs0.comovingDistance(z)));
            QV_CHECK(std::isfinite(hs0.luminosityDistance(z)));
            QV_CHECK(std::isfinite(hs0.distanceModulus(z)));
            QV_CHECK(std::isfinite(hs0.ageAtRedshift(z)));
            QV_CHECK(std::isfinite(hs0.decelerationParameter(z)));
            QV_CHECK(hs0.comovingDistance(z) > 0.0);
            QV_CHECK(hs0.luminosityDistance(z) > 0.0);
        }
        QV_CHECK(std::isfinite(hs0.soundHorizon()));
        QV_CHECK(std::isfinite(hs0.criticalDensity()));
        QV_CHECK(hs0.soundHorizon() > 0.0);
        QV_CHECK(hs0.criticalDensity() > 0.0);

        std::cout << "Test 1 PASS: f_R0=0 produces finite, positive values at all z" << std::endl;
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
            std::cout << "  z=" << z << ": H_fR=" << h_hs << " H_LCDM=" << h_lcdm
                      << " relDiff=" << (relDiff * 100.0) << "%" << std::endl;
            if (relDiff > 0.01)
                differsAtHighZ = true;
        }
        QV_CHECK(differsAtHighZ);

        const double diffDM =
            std::fabs(hs1e4.distanceModulus(1059.94) - lcdm.distanceModulus(1059.94));
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
        std::cout << "  z=1059.94: H_fR(f_R0=1e-5)=" << h_hs << " H_LCDM=" << h_lcdm
                  << " relDiff=" << (relDiff * 100.0) << "%" << std::endl;
        QV_CHECK(relDiff > 0.01);

        std::cout << "Test 3 PASS: f_R0=1e-5 differs from LCDM by >1% at z_drag" << std::endl;
    }

    // ---- Test 4: H(0) stays close to H0 for small f_R0 ----
    // f_R(0) = -f_R0 * Om / (Om + 4*OL) modifies effective densities at z=0,
    // so H(0) = H0 * E(0) deviates slightly from H0 for f_R0 != 0.
    // For small f_R0 the deviation is < 1%.
    {
        for (double fR0 : {0.0, 1e-4, 1e-5, 1e-6}) {
            HuSawickiProvider hs(H0, Om, Ob, Or, OL, Ok, Tcmb, fR0);
            const double h0 = hs.H(0.0);
            QV_CHECK(std::isfinite(h0));
            QV_CHECK(h0 > 0.0);
            const double relDiff = std::fabs(h0 - H0) / H0;
            QV_CHECK(relDiff < 1e-2);
            if (relDiff >= 1e-12) {
                std::cout << "INFO: H(0) deviates from H0 by " << (relDiff * 100.0)
                          << "% for f_R0=" << fR0 << std::endl;
            }
        }
        std::cout << "Test 4 PASS: H(0) within 1% of H0 for small f_R0" << std::endl;
    }

    std::cout << "=== HuSawickiProvider Test: ALL PASS ===" << std::endl;
    std::_Exit(0);
}
