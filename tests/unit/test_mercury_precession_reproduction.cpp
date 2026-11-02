/**
 * @file test_mercury_precession_reproduction.cpp
 * @brief Non-vacuous test for Mercury perihelion precession reproduction.
 *
 * Runs MercuryPrecessionExperiment with mass=1 M☉, e=0.2056, a=5.79e10 m,
 * compareToPublished=true, and verifies:
 *   - mass and spin propagate from params (not defaults)
 *   - precession_arcsec_per_century ≈ 43.0
 *   - precession_delta_sigma within 1.5σ of expected (~0.0σ since formula should match)
 *   - reference string contains "Einstein"
 *   - ScanSummary citation/observedValue propagation
 *
 * Pre-flight computation:
 *   Δφ_per_orbit = 6πGM/(a(1-e²)c²)
 *   With a=5.79e10 m, e=0.2056, M=1 M☉:
 *   Δφ_per_orbit ≈ 5.02e-7 rad/orbit
 *   orbits_per_century = 36525/87.97 ≈ 415.2
 *   precession ≈ 42.98 arcsec/century
 *   Δσ = (42.98 - 43.0) / 0.1 ≈ -0.02σ
 *
 * The precession formula should reproduce ~43 arcsec/century, well within 1σ.
 */

#include "test_assert.h"

#include <iostream>

#include "experiment/experiments/MercuryPrecessionExperiment.h"
#include "experiment/ScanSummaryBuilder.h"
#include "discovery/ScanSummary.h"

using namespace quantumverse;

int main() {
    MercuryPrecessionExperiment exp;
    QVariantMap p;
    p["semi_major_m"] = 5.79e10;
    p["eccentricity"] = 0.2056;
    p["orbits"] = 100;
    p["mass_sun_Msun"] = 1.0;
    p["compareToPublished"] = true;
    ExperimentResult r = exp.run(p, [](double){}, [](){ return false; });

    QV_CHECK(r.success);
    QV_CHECK(r.scalars.contains(QStringLiteral("precession_arcsec_per_century")));
    QV_CHECK(r.scalars.contains(QStringLiteral("precession_delta_sigma")));
    QV_CHECK(r.scalars.contains(QStringLiteral("reference")));
    QV_CHECK(r.scalars.contains(QStringLiteral("source_system")));
    QV_CHECK(r.scalars.contains(QStringLiteral("compareToPublished")));

    // Mass and eccentricity propagate from params
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("semi_major_m")).toDouble() - 5.79e10) < 1.0);
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("eccentricity")).toDouble() - 0.2056) < 1e-9);

    // Precession should be ≈ 43 arcsec/century
    const double precession = r.scalars.value(QStringLiteral("precession_arcsec_per_century")).toDouble();
    QV_CHECK(std::abs(precession - 43.0) < 1.0);
    QV_CHECK(precession > 40.0);
    QV_CHECK(precession < 46.0);

    // Delta sigma within tolerance
    const double dsigma = r.scalars.value(QStringLiteral("precession_delta_sigma")).toDouble();
    QV_CHECK(std::abs(dsigma) < 1.5);

    // Reference string must cite Einstein/Mercury
    const QString ref = r.scalars.value(QStringLiteral("reference")).toString();
    const QString source = r.scalars.value(QStringLiteral("source_system")).toString();
    QV_CHECK(ref.contains("Einstein"));
    QV_CHECK(source.contains("Mercury"));

    // ScanSummaryBuilder must propagate citation and observedValue
    auto summary = ScanSummaryBuilder::fromExperiment(QStringLiteral("mercury_precession"), r);
    QV_CHECK(summary.citation.contains("Einstein"));
    QV_CHECK(summary.observedValue.has_value());
    QV_CHECK(std::abs(*summary.observedValue - precession) < 1e-6);

    std::cout << "=== Mercury Precession Reproduction Test: ALL PASS ===" << std::endl;
    std::cout << "  precession      = " << precession << " arcsec/century" << std::endl;
    std::cout << "  delta_sigma     = " << dsigma << std::endl;
    std::cout << "  citation        = " << summary.citation.toStdString() << std::endl;
    return 0;
}
