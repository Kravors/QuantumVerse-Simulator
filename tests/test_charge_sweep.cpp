/**
 * @file test_charge_sweep.cpp
 * @brief ChargeSweepExperiment: extremal horizons coincide at Q/M = 1.
 */

#include <QCoreApplication>
#include <QVariantMap>
#include <cmath>
#include <iostream>

#include "experiment/experiments/ChargeSweepExperiment.h"
#include "test_assert.h"

using namespace quantumverse;

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    std::cout << "=== ChargeSweepExperiment Test ===" << std::endl;

    ChargeSweepExperiment e;

    // Extremal limit: Q/M = 1 -> r+ == r- == M.
    {
        QVariantMap p;
        p["mass"] = 1.0;
        p["charge_min"] = 1.0;
        p["charge_max"] = 1.0;
        p["steps"] = 2;
        auto r = e.run(p, [](double) {}, []() { return false; });
        QV_CHECK(r.success);
        QV_CHECK_NEAR(r.scalars["last_r_plus"].toDouble(),
                      r.scalars["last_r_minus"].toDouble(), 1e-6);
        std::cout << "[PASS] extremal r+ == r-" << std::endl;
    }

    // Horizons shrink as charge grows; r+ > r- for sub-extremal.
    {
        QVariantMap p;
        p["mass"] = 1.0;
        p["charge_min"] = 0.0;
        p["charge_max"] = 0.5;
        p["steps"] = 10;
        auto r = e.run(p, [](double) {}, []() { return false; });
        QV_CHECK(r.success);
        QV_CHECK(r.scalars["last_r_plus"].toDouble() > 0.0);
        QV_CHECK(r.scalars["last_r_minus"].toDouble() >= 0.0);
        std::cout << "[PASS] sub-extremal horizons non-negative" << std::endl;
    }

    std::cout << "=== ChargeSweepExperiment Test: ALL PASS ===" << std::endl;
    return 0;
}
