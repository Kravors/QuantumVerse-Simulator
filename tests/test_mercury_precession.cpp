/**
 * @file test_mercury_precession.cpp
 * @brief MercuryPrecessionExperiment: GR predicts ~43 arcsec/century.
 */

#include <QCoreApplication>
#include <QVariantMap>
#include <cmath>
#include <iostream>

#include "experiment/experiments/MercuryPrecessionExperiment.h"
#include "physics/SingularityHandler.h"
#include "test_assert.h"

using namespace quantumverse;

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    std::cout << "=== MercuryPrecessionExperiment Test ===" << std::endl;

    MercuryPrecessionExperiment e;

    // Mercury default parameters should reproduce ~43 arcsec/century.
    QVariantMap p;
    p["semi_major_m"] = 5.79e10;
    p["eccentricity"] = 0.2056;
    p["orbits"] = 100;
    p["mass_sun_Msun"] = 1.0;
    auto r = e.run(p, [](double) {}, []() { return false; });
    QV_CHECK(r.success);
    const double prec = r.scalars["precession_arcsec_per_century"].toDouble();
    QV_CHECK(std::abs(prec - 43.0) < 1.0);
    QV_CHECK(r.scalars["expected"].toDouble() == 43.0);
    QV_CHECK(r.scalars["error_arcsec"].toDouble() < 1.0);
    std::cout << "[PASS] Mercury precession = " << prec << " arcsec/century (expected 43.0)"
              << std::endl;

    // --- Kerr event horizon: units fix regression test ---
    // computeKerrEventHorizon previously plugged SI angular momentum
    // (J/(Mc), metres) into the dimensionless 1 - a^2 formula,
    // yielding sqrt(negative) = NaN (r_+ = nanm).  Fix: convert to
    // dimensionless a* = (J/(Mc)) / (GM/c^2) before squaring.
    {
        const double M = 10.0 * 1.98892e30;  // 10 M☉ in kg
        const double spin = 0.9;
        const double J = spin * Event4D::G * M * M / Event4D::C;

        SingularityHandler sh(SingularityType::KERR, M, J);
        const double r_plus = sh.getProperties().event_horizon_radius;
        QV_CHECK(std::isfinite(r_plus));
        QV_CHECK_NEAR(r_plus, 21209.0, 50.0);  // matches log line
        std::cout << "[PASS] KerrHorizonMatchesMetric: r_+ = " << r_plus << " m (expected ~21209 m)"
                  << std::endl;
    }

    {
        const double M = 10.0 * 1.98892e30;  // 10 M☉ in kg
        const double spin = 1.5;             // naked, a* > 1
        const double J = spin * Event4D::G * M * M / Event4D::C;

        SingularityHandler sh(SingularityType::KERR, M, J);
        const double r_plus = sh.getProperties().event_horizon_radius;
        QV_CHECK(std::isfinite(r_plus));  // no NaN
        const double r_h = Event4D::G * M / (Event4D::C * Event4D::C);
        QV_CHECK_NEAR(r_plus, r_h, 1.0);  // clamped to extremal
        std::cout << "[PASS] KerrNakedClamped: r_+ = " << r_plus << " m (expected ~" << r_h << " m)"
                  << std::endl;
    }

    std::cout << "=== MercuryPrecessionExperiment Test: ALL PASS ===" << std::endl;
    return 0;
}
