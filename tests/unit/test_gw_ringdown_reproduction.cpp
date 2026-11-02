/**
 * @file test_gw_ringdown_reproduction.cpp
 * @brief Non-vacuous test for GW150914 reproduction paper.
 *
 * Runs GWRingdownExperiment with mass=62.1 M_sun, spin_a=0.68,
 * compareToPublished=true, and verifies:
 *   - mass and spin propagate from params (not defaults)
 *   - kretschmann_at_r_plus is finite and positive
 *   - f_R_delta_sigma within 1.5 sigma of expected 0.95
 *   - tau_delta_sigma within 1.5 sigma of expected -0.65
 *   - reference string contains "GW150914"
 *   - citation propagated through ScanSummaryBuilder
 *
 * These bounds match the pre-flight computation:
 *   f_R = 255.7 Hz (published 251 ± 5 Hz) → Δσ = +0.95
 *   tau = 3.61 ms  (published 4.0 ± 0.6 ms) → Δσ = -0.65
 */

#include "test_assert.h"

#include <iostream>
#include <QVariantMap>

#include "experiment/experiments/GWRingdownExperiment.h"
#include "experiment/ScanSummaryBuilder.h"
#include "discovery/ScanSummary.h"

using namespace quantumverse;

int main() {
    // Run gw_ringdown with GW150914 parameters and comparison enabled
    GWRingdownExperiment exp;
    QVariantMap p;
    p["mass_Msun"]        = 62.1;
    p["spin_a"]           = 0.68;
    p["compareToPublished"] = true;
    p["n_steps"]          = 50;
    ExperimentResult r = exp.run(p, [](double){}, [](){ return false; });

    QV_CHECK(r.success);
    QV_CHECK(r.scalars.contains(QStringLiteral("f_R_Hz")));
    QV_CHECK(r.scalars.contains(QStringLiteral("tau_ms")));
    QV_CHECK(r.scalars.contains(QStringLiteral("f_R_delta_sigma")));
    QV_CHECK(r.scalars.contains(QStringLiteral("tau_delta_sigma")));
    QV_CHECK(r.scalars.contains(QStringLiteral("reference")));
    QV_CHECK(r.scalars.contains(QStringLiteral("compareToPublished")));

    // Mass and spin must propagate from params, not defaults
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("mass_Msun")).toDouble() - 62.1) < 1e-9);
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("spin_a")).toDouble() - 0.68) < 1e-9);

    // Kretschmann must be finite and positive
    QV_CHECK(std::isfinite(r.scalars.value(QStringLiteral("kretschmann_at_r_plus")).toDouble()));
    QV_CHECK(r.scalars.value(QStringLiteral("kretschmann_at_r_plus")).toDouble() > 0.0);

    // f_R delta sigma within tolerance (expected ~0.95)
    const double fR_dsigma = r.scalars.value(QStringLiteral("f_R_delta_sigma")).toDouble();
    QV_CHECK(std::abs(fR_dsigma - 0.95) < 1.5);

    // tau delta sigma within tolerance (expected ~-0.65)
    const double tau_dsigma = r.scalars.value(QStringLiteral("tau_delta_sigma")).toDouble();
    QV_CHECK(std::abs(tau_dsigma - (-0.65)) < 1.5);

    // Reference string must cite GW150914
    const QString ref = r.scalars.value(QStringLiteral("reference")).toString();
    const QString source = r.scalars.value(QStringLiteral("source_system")).toString();
    QV_CHECK(ref.contains("Abbott"));
    QV_CHECK(source.contains("GW150914"));

    // ScanSummaryBuilder must propagate citation and observedValue
    auto summary = ScanSummaryBuilder::fromExperiment(QStringLiteral("gw_ringdown"), r);
    QV_CHECK(summary.citation.contains("Abbott"));
    QV_CHECK(summary.observedValue.has_value());
    const double f_R_hz = r.scalars.value(QStringLiteral("f_R_Hz")).toDouble();
    QV_CHECK(std::abs(*summary.observedValue - f_R_hz) < 1e-6);

    std::cout << "=== GW150914 Reproduction Test: ALL PASS ===" << std::endl;
    std::cout << "  f_R_delta_sigma = " << fR_dsigma << std::endl;
    std::cout << "  tau_delta_sigma = " << tau_dsigma << std::endl;
    std::cout << "  citation        = " << summary.citation.toStdString() << std::endl;
    return 0;
}
