/**
 * @file test_frame_dragging_experiment.cpp
 * @brief FrameDraggingExperiment: Lense–Thirring Ω_LT regression test.
 *
 * Reference case: M=1 M☉, a*=0.9, r=7e6 m → last_mas_per_year ≈ 2.23422e+10
 * mas/yr (deterministic given Event4D::G and Event4D::C).
 *
 * Note: the 39 mas/yr scalar is a historical GP-B reference label only —
 * GP-B assumes M=Earth and a different angular momentum J, so it is NOT a
 * pass/fail target for these defaults.
 */

#include <QCoreApplication>
#include <QVariantMap>
#include <cmath>
#include <iostream>

#include "experiment/experiments/FrameDraggingExperiment.h"
#include "test_assert.h"

using namespace quantumverse;

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    std::cout << "=== FrameDraggingExperiment Test ===" << std::endl;

    FrameDraggingExperiment e;

    // steps=2 so the last iteration lands at t=1.0 → r = rOrb = 7e6 m.
    QVariantMap p;
    p["mass_Msun"] = 1.0;
    p["spin_a_star"] = 0.9;
    p["orbit_radius_m"] = 7.0e6;
    p["steps"] = 2;
    auto r = e.run(p, [](double) {}, []() { return false; });
    QV_CHECK(r.success);
    QV_CHECK(std::isfinite(r.scalars["last_mas_per_year"].toDouble()));

    // Deterministic regression value (verified by direct run):
    //   Ω_LT = 2 G J / (c² r³), J = a* G M² / c
    //   M=1 M☉, a*=0.9, r=7e6 m → 2.23422e+10 mas/yr
    QV_CHECK_NEAR(r.scalars["last_mas_per_year"].toDouble(), 2.23422e10, 1.0e7);

    QV_CHECK(r.scalars["expected_gpb_mas_per_year"].toDouble() == 39.0);
    QV_CHECK(r.curve.size() == 2);
    std::cout << "[PASS] frame dragging last = " << r.scalars["last_mas_per_year"].toDouble()
              << " mas/yr"
              << " (reference GP-B label 39.0)" << std::endl;

    std::cout << "=== FrameDraggingExperiment Test: ALL PASS ===" << std::endl;
    return 0;
}
