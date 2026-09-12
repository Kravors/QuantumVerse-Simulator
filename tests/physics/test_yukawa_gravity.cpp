/**
 * @file test_yukawa_gravity.cpp
 * @brief Unit tests for the Yukawa fifth-force modified-gravity plugin
 *
 * Verifies:
 *   1. The Yukawa metric satisfies the Lorentz-signature / symmetry /
 *      negative-determinant invariants.
 *   2. alpha = 0 recovers the standard Schwarzschild (GR) metric.
 *   3. A non-zero Yukawa coupling modifies the metric (deviation grows
 *      with coupling strength).
 *   4. The metric is integrable: a timelike geodesic under the Yukawa
 *      metric diverges from the GR geodesic.
 */

#include <cmath>
#include <iostream>
#include <map>
#include <memory>

#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "physics/GeodesicIntegrator.h"
#include "discovery/DiscoveryEngine.h"
#include "test_assert.h"

using namespace quantumverse;

namespace {

void test_metric_invariants() {
    std::cout << "  Testing Yukawa metric invariants..." << std::endl;
    YukawaFifthForcePlugin plugin(0.5, 15.0, 1.0e26);
    std::map<std::string, double> params{{"alpha", 0.5}, {"lambda", 15.0}, {"M", 1.0e26}};
    Event4D loc(0.0, 20.0, 0.0, 0.0);
    MetricTensor m = plugin.computeMetric(loc, params);

    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            QV_CHECK_NEAR(m.g[i][j] - m.g[j][i], 0.0, 1e-12);

    QV_CHECK(m.g[0][0] < 0.0);
    QV_CHECK(m.g[1][1] > 0.0);
    QV_CHECK(m.g[2][2] > 0.0);
    QV_CHECK(m.g[3][3] > 0.0);

    double det = m.determinant();
    QV_CHECK(det < 0.0);
    (void)det;
}

void test_gr_recovery() {
    std::cout << "  Testing GR recovery (alpha = 0)..." << std::endl;
    YukawaFifthForcePlugin yukawa(0.0, 15.0, 1.0);
    FRLGravityPlugin gr(1.0, 1.0); // FRL metric is pure Schwarzschild with M=1.0
    std::map<std::string, double> yp{{"alpha", 0.0}, {"lambda", 15.0}, {"M", 1.0}};
    Event4D loc(0.0, 20.0, 0.0, 0.0);
    MetricTensor my = yukawa.computeMetric(loc, yp);
    MetricTensor mg = gr.computeMetric(loc, {});
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            QV_CHECK_NEAR(my.g[i][j] - mg.g[i][j], 0.0, 1e-12);
}

void test_yukawa_deviation() {
    std::cout << "  Testing Yukawa correction is applied..." << std::endl;
    YukawaFifthForcePlugin plugin(0.5, 15.0, 1.0e26);
    std::map<std::string, double> p0{{"alpha", 0.0}, {"lambda", 15.0}, {"M", 1.0e26}};
    std::map<std::string, double> p1{{"alpha", 0.5}, {"lambda", 15.0}, {"M", 1.0e26}};
    std::map<std::string, double> p2{{"alpha", 1.0}, {"lambda", 15.0}, {"M", 1.0e26}};
    Event4D loc(0.0, 20.0, 0.0, 0.0);
    MetricTensor m0 = plugin.computeMetric(loc, p0);
    MetricTensor m1 = plugin.computeMetric(loc, p1);
    MetricTensor m2 = plugin.computeMetric(loc, p2);
    QV_CHECK(std::fabs(m1.g[0][0] - m0.g[0][0]) > 1e-6);
    QV_CHECK(std::fabs(m2.g[0][0] - m0.g[0][0]) > std::fabs(m1.g[0][0] - m0.g[0][0]));
}

void test_geodesic_integration() {
    std::cout << "  Testing geodesic integration under Yukawa metric..." << std::endl;
    auto makeField = [](double alpha) {
        auto p = std::make_shared<YukawaFifthForcePlugin>(alpha, 15.0, 1.0e26);
        return [p, alpha](const Event4D& evt) -> MetricTensor {
            std::map<std::string, double> params{{"alpha", alpha}, {"lambda", 15.0}, {"M", 1.0e26}};
            return p->computeMetric(evt, params);
        };
    };

    GeodesicIntegrator integ(1e-8, 1e-10, 1.0, 0.9, 1000000);
    Event4D start(0.0, 12.0, 0.0, 0.0);
    std::array<double, 4> vel{1.0, -0.1, 0.0, 0.0};

    integ.setMetricField(makeField(0.0));
    auto grTraj = integ.integrate(start, vel, GeodesicType::TIMELIKE, 100.0);
    integ.setMetricField(makeField(0.9));
    auto yTraj = integ.integrate(start, vel, GeodesicType::TIMELIKE, 100.0);

    QV_CHECK(!grTraj.empty() && !yTraj.empty());
    const auto& grEnd = grTraj.back().event;
    const auto& yEnd = yTraj.back().event;
    QV_CHECK(std::isfinite(grEnd.x) && std::isfinite(yEnd.x));
    QV_CHECK(std::fabs(yEnd.x - grEnd.x) > 1e-3);
    (void)grEnd;
    (void)yEnd;
}

} // namespace

int main() {
    std::cout << "=== YukawaGravityTest ===" << std::endl;
    test_metric_invariants();
    test_gr_recovery();
    test_yukawa_deviation();
    test_geodesic_integration();
    std::cout << "All Yukawa gravity tests passed." << std::endl;
    return 0;
}
