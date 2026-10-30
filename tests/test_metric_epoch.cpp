/**
 * @file test_metric_epoch.cpp
 * @brief MetricTensor epoch bumps on every in-place mass/spin change.
 */

#include <QCoreApplication>
#include <iostream>

#include "spacetime/MetricTensor.h"
#include "spacetime/KerrMetric.h"
#include "test_assert.h"

using namespace quantumverse;

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    std::cout << "=== Metric Epoch Test ===" << std::endl;

    // SchwarzschildMetric
    {
        SchwarzschildMetric m(1.989e30);
        const uint64_t e0 = m.epoch();
        m.setMass(2.0 * 1.989e30);
        QV_CHECK(m.epoch() != e0);
        const uint64_t e1 = m.epoch();
        m.setMass(2.0 * 1.989e30); // same value -> no bump
        QV_CHECK(m.epoch() == e1);
        std::cout << "[PASS] Schwarzschild epoch bumps on mass change" << std::endl;
    }

    // KerrMetric
    {
        KerrMetric m(1.989e30, 0.5);
        const uint64_t e0 = m.epoch();
        m.setSpin(0.9);
        QV_CHECK(m.epoch() != e0);
        const uint64_t e1 = m.epoch();
        m.setMass(3.0 * 1.989e30);
        QV_CHECK(m.epoch() != e1);
        std::cout << "[PASS] Kerr epoch bumps on spin and mass change" << std::endl;
    }

    // Base MetricTensor: default setters no-op, epoch stays 0
    {
        MetricTensor m;
        QV_CHECK(m.epoch() == 0);
        m.setMass(1.0);
        m.setSpin(0.5);
        m.setCharge(1.0);
        QV_CHECK(m.epoch() == 0);
        std::cout << "[PASS] base MetricTensor epoch unchanged by no-op setters" << std::endl;
    }

    std::cout << "=== Metric Epoch Test: ALL PASS ===" << std::endl;
    return 0;
}
