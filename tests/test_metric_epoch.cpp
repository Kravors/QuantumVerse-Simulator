// QuantumVerse Test: Metric Epoch
// Validates that in-place mass mutation bumps MetricTensor::epoch(),
// epoch only bumps on real change, and is strictly monotonic across a sweep.

#include <iostream>
#include <cmath>
#include <memory>
#include "spacetime/MetricTensor.h"
#include "test_assert.h"

using namespace quantumverse;

int main() {
    std::cout << "=== Metric Epoch Tests ===" << std::endl;

    // Test 1: epoch bumps on mass change, not on no-op
    {
        auto metric = std::make_shared<SchwarzschildMetric>(10.0 * 1.989e30);
        uint64_t e0 = metric->epoch();

        metric->setMass(5.0 * 1.989e30);
        QV_CHECK(metric->epoch() == e0 + 1);

        uint64_t e1 = metric->epoch();
        metric->setMass(5.0 * 1.989e30);  // no-op: same mass
        QV_CHECK(metric->epoch() == e1);   // must NOT bump

        std::cout << "[PASS] epoch bumps on mass change, not on no-op" << std::endl;
    }

    // Test 2: epoch monotonicity
    {
        auto metric = std::make_shared<SchwarzschildMetric>(1.0 * 1.989e30);
        uint64_t prev = metric->epoch();

        for (double m = 0.1; m <= 10.0; m += 0.1) {
            metric->setMass(m * 1.989e30);
            QV_CHECK(metric->epoch() > prev);
            prev = metric->epoch();
        }

        std::cout << "[PASS] epoch is strictly monotonic across mass sweep" << std::endl;
    }

    // Test 3: pointer identity preserved across mass change (no swap)
    {
        auto metric = std::make_shared<SchwarzschildMetric>(10.0 * 1.989e30);
        void* addr = metric.get();

        metric->setMass(5.0 * 1.989e30);
        QV_CHECK(metric.get() == addr);  // same object, mutated in place

        std::cout << "[PASS] pointer identity preserved across mass change" << std::endl;
    }

    // Test 4: base MetricTensor epoch() returns 0 (no-op default)
    {
        MetricTensor base;
        QV_CHECK(base.epoch() == 0);
        base.setMass(5.0);  // base no-op, should not throw
        QV_CHECK(base.epoch() == 0);

        std::cout << "[PASS] base MetricTensor epoch() is 0 (no-op default)" << std::endl;
    }

    std::cout << "=== All Metric Epoch tests passed ===" << std::endl;
    std::cout.flush();
    return 0;
}
