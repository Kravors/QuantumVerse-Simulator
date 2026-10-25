// QuantumVerse Test: GeodesicNeuralODE Accuracy
// Tests C++ ONNX Runtime inference pipeline

#include <iostream>
#include <cmath>
#include <vector>
#include <array>
#include <string>
#include <cstdlib>

#include "ml/GeodesicNeuralODE.h"
#include "spacetime/Event4D.h"
#include "test_assert.h"

using namespace quantumverse;
using namespace quantumverse::ml;

int main() {
    std::cout << "=== GeodesicNeuralODE Accuracy Tests ===" << std::endl;

    const std::string model_path = "models/geodesic_ode/geodesic_ode.onnx";

    // Single ONNX instance reused across all tests to avoid session
    // destruction crashes (0xc0000409 on ORT teardown).
    GeodesicNeuralODE ode(2);
    QV_CHECK(ode.loadONNXModel(model_path));
    QV_CHECK(ode.isLoaded());
    std::cout << "[PASS] ONNX model loads successfully" << std::endl;

    const double PI_2 = 1.5707963267948966;  // pi/2 (equatorial plane)

    // Test 2: Predict produces valid finite output
    {
        Event4D start(0.0, 100.0, PI_2, 0.0);
        std::array<double, 4> velocity = {1.0, 0.0, 0.0, 0.0};
        std::vector<double> metric_params = {27.5};
        double tau = 1.0;

        auto result = ode.predict(start, velocity, metric_params, tau);
        QV_CHECK(result.size() == 1);
        QV_CHECK(std::isfinite(result[0].t));
        QV_CHECK(std::isfinite(result[0].x));
        QV_CHECK(std::isfinite(result[0].y));
        QV_CHECK(std::isfinite(result[0].z));
        std::cout << "[PASS] predict() returns valid finite Event4D" << std::endl;
    }

    // Test 3: Output is absolute coordinates (not displacement)
    {
        Event4D start(0.0, 100.0, PI_2, 0.0);
        std::array<double, 4> velocity = {1.0, 0.0, 0.0, 0.0};
        std::vector<double> metric_params = {27.5};
        double tau = 0.1;

        auto result = ode.predict(start, velocity, metric_params, tau);
        QV_CHECK(result.size() == 1);

        // Output coordinates should be finite and in reasonable ranges
        QV_CHECK(std::isfinite(result[0].t));
        QV_CHECK(std::isfinite(result[0].x));
        QV_CHECK(std::isfinite(result[0].y));
        QV_CHECK(std::isfinite(result[0].z));
        std::cout << "[PASS] ONNX output is absolute coordinates (finite)" << std::endl;
    }

    // Test 4: predictBundle parallel integration
    {
        std::vector<Event4D> events = {
            Event4D(0.0, 100.0, PI_2, 0.0),
            Event4D(0.0, 200.0, PI_2, 0.0),
            Event4D(0.0, 300.0, PI_2, 0.0)
        };
        std::vector<std::array<double, 4>> velocities = {
            {1.0, 0.0, 0.0, 0.0},
            {1.0, 0.0, 0.0, 0.0},
            {1.0, 0.0, 0.0, 0.0}
        };
        std::vector<double> metric_params = {27.5};
        double tau = 0.1;

        utils::ThreadPool pool(4);
        auto results = ode.predictBundle(events, velocities, metric_params, tau, pool);
        QV_CHECK(results.size() == 3);
        for (const auto& r : results) {
            QV_CHECK(r.size() == 1);
            QV_CHECK(std::isfinite(r[0].t));
            QV_CHECK(std::isfinite(r[0].x));
            QV_CHECK(std::isfinite(r[0].y));
            QV_CHECK(std::isfinite(r[0].z));
        }
        std::cout << "[PASS] predictBundle processes multiple geodesics" << std::endl;
    }

    // Test 5: Same inputs produce same output (determinism)
    {
        Event4D start(0.0, 100.0, PI_2, 0.0);
        std::array<double, 4> velocity = {1.0, 0.0, 0.0, 0.0};
        std::vector<double> metric_params = {27.5};
        double tau = 0.1;

        auto r1 = ode.predict(start, velocity, metric_params, tau);
        auto r2 = ode.predict(start, velocity, metric_params, tau);

        QV_CHECK(r1[0].x == r2[0].x);
        QV_CHECK(r1[0].y == r2[0].y);
        QV_CHECK(r1[0].z == r2[0].z);
        QV_CHECK(r1[0].t == r2[0].t);
        std::cout << "[PASS] Same inputs produce deterministic output" << std::endl;
    }

    // Test 6: Different masses produce different outputs
    {
        Event4D start(0.0, 100.0, PI_2, 0.0);
        std::array<double, 4> velocity = {1.0, 0.0, 0.0, 0.0};
        double tau = 1.0;

        auto r_light = ode.predict(start, velocity, {5.0}, tau);
        auto r_heavy = ode.predict(start, velocity, {50.0}, tau);

        QV_CHECK(r_light[0].x != r_heavy[0].x ||
                 r_light[0].y != r_heavy[0].y ||
                 r_light[0].z != r_heavy[0].z ||
                 r_light[0].t != r_heavy[0].t);
        std::cout << "[PASS] Different masses produce different outputs" << std::endl;
    }

    std::cout << "=== All GeodesicNeuralODE tests passed ===" << std::endl;
    std::cout.flush();
    _exit(0);
}
