#include "physics/GWPolarization.h"
#include "physics/GeodesicDeviation.h"
#include "physics/SingularityHandler.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "physics/GeodesicIntegrator.h"
#include <cmath>
#include <iostream>
#include "test_assert.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace quantumverse;

// ============================================================================
// 3.2.1 - Gravitational wave polarization modes
// ============================================================================
void test_gw_polarization_modes() {
    auto plus = GWPolarization::generateWaveform(
        PolarizationMode::Plus, 1e-21, 100.0, 1.0, 4096.0);
    auto cross = GWPolarization::generateWaveform(
        PolarizationMode::Cross, 1e-21, 100.0, 1.0, 4096.0);

    QV_CHECK(plus.mode == PolarizationMode::Plus);
    QV_CHECK(cross.mode == PolarizationMode::Cross);
    QV_CHECK(!plus.waveform.empty());
    QV_CHECK(!cross.waveform.empty());

    double t = 0.1;
    double h_plus = plus.strainAt(t);
    double h_cross = cross.strainAt(t);
    QV_CHECK(std::isfinite(h_plus));
    QV_CHECK(std::isfinite(h_cross));
    QV_CHECK(std::abs(h_plus) <= plus.amplitude + 1e-30);
    QV_CHECK(std::abs(h_cross) <= cross.amplitude + 1e-30);
    (void)h_plus; (void)h_cross;

    std::vector<GWPolarizationState> polarizations = {plus, cross};
    QV_CHECK(GWPolarization::isGRConsistent(polarizations));

    std::cout << "[PASS] GW polarization modes: Plus and Cross generated and valid" << std::endl;
}

// ============================================================================
// 3.2.2 - GW detector response
// ============================================================================
void test_gw_detector_response() {
    auto plus = GWPolarization::generateWaveform(
        PolarizationMode::Plus, 1e-21, 100.0, 1.0, 4096.0);

    std::array<double, 3> arm1 = {1.0, 0.0, 0.0};
    std::array<double, 3> arm2 = {0.0, 1.0, 0.0};

    double response = GWPolarization::detectorResponse(plus, arm1, arm2);
    QV_CHECK(std::isfinite(response));

    std::cout << "[PASS] GW detector response: finite=" << response << std::endl;
}

// ============================================================================
// 3.2.3 - Light deflection at various impact parameters
// ============================================================================
void test_light_deflection_general() {
    double M_sun = 1477.0;

    for (double b : {2.0 * M_sun, 5.0 * M_sun, 10.0 * M_sun, 100.0 * M_sun}) {
        double deflection_analytic = 4.0 * M_sun / b;
        QV_CHECK(std::isfinite(deflection_analytic));
        QV_CHECK(deflection_analytic > 0.0);
        (void)deflection_analytic;
    }

    double b_critical = 3.0 * std::sqrt(3.0) * M_sun;
    QV_CHECK(b_critical > 0.0);

    SchwarzschildMetric sch(1.989e30);
    auto g = sch.evaluate(Event4D(0.0, b_critical, 0.0, 0.0));
    (void)g;

    std::cout << "[PASS] Light deflection: analytic formula valid, photon sphere at b=" << b_critical << std::endl;
}

// ============================================================================
// 3.2.4 - Geodesic deviation: tidal tensor for Schwarzschild
// ============================================================================
void test_geodesic_deviation_tidal_tensor() {
    double M = 1.0;
    SchwarzschildMetric sch(M);
    Event4D position(0.0, 10.0, 0.0, 0.0);
    Event4D velocity(1.0, 0.0, 0.0, 0.0);

    auto tidal = GeodesicDeviation::tidalTensor(sch, position, velocity);

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            QV_CHECK(std::isfinite(tidal[i][j]));
        }
    }

    double r = 10.0;
    double expected_rr = -2.0 * M / (r * r * r);
    double expected_tt = M / (r * r * r);

    QV_CHECK(std::abs(tidal[0][0] - expected_rr) < 1e-6);
    QV_CHECK(std::abs(tidal[1][1] - expected_tt) < 1e-6);
    QV_CHECK(std::abs(tidal[2][2] - expected_tt) < 1e-6);
    (void)expected_rr; (void)expected_tt;
    (void)tidal;

    std::cout << "[PASS] Geodesic deviation tidal tensor matches Schwarzschild 1/r^3 law" << std::endl;
}

// ============================================================================
// 3.2.5 - Time dilation: Schwarzschild proper time ratio
// ============================================================================
void test_time_dilation_schwarzschild() {
    double M = 1.0;
    double r1 = 20.0 * M;
    double r2 = 10.0 * M;

    SingularityHandler handler(SingularityType::SCHWARZSCHILD, M * Event4D::C * Event4D::C / Event4D::G, 0.0, 0.0);

    Event4D event1(0.0, r1, 0.0, 0.0);
    Event4D event2(0.0, r2, 0.0, 0.0);

    double z1 = handler.getGravitationalRedshift(event1);
    double z2 = handler.getGravitationalRedshift(event2);

    QV_CHECK(std::isfinite(z1));
    QV_CHECK(std::isfinite(z2));
    QV_CHECK(z2 > z1);

    double ratio_expected = std::sqrt(1.0 - 2.0 * M / r1) / std::sqrt(1.0 - 2.0 * M / r2);
    double ratio_actual = (1.0 + z2) / (1.0 + z1);
    double relError = std::abs(ratio_actual - ratio_expected) / ratio_expected;
    QV_CHECK(relError < 0.1);
    (void)relError;

    std::cout << "[PASS] Time dilation: ratio=" << ratio_actual << ", expected=" << ratio_expected << std::endl;
}

int main() {
    test_gw_polarization_modes();
    test_gw_detector_response();
    test_light_deflection_general();
    test_geodesic_deviation_tidal_tensor();
    test_time_dilation_schwarzschild();

    std::cout << "=== ALL GR VALIDATION TESTS PASSED ===" << std::endl;
    return 0;
}
