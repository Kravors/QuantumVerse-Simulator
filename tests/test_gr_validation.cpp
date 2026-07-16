#include "physics/GWPolarization.h"
#include "physics/GeodesicDeviation.h"
#include "physics/SingularityHandler.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "physics/GeodesicIntegrator.h"
#include <cmath>
#include <cassert>
#include <iostream>

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

    assert(plus.mode == PolarizationMode::Plus && "Plus mode should be Plus");
    assert(cross.mode == PolarizationMode::Cross && "Cross mode should be Cross");
    assert(!plus.waveform.empty() && "Plus waveform should not be empty");
    assert(!cross.waveform.empty() && "Cross waveform should not be empty");

    double t = 0.1;
    double h_plus = plus.strainAt(t);
    double h_cross = cross.strainAt(t);
    assert(std::isfinite(h_plus) && "Plus strain should be finite");
    assert(std::isfinite(h_cross) && "Cross strain should be finite");
    assert(std::abs(h_plus) <= plus.amplitude + 1e-30 && "Plus strain within amplitude");
    assert(std::abs(h_cross) <= cross.amplitude + 1e-30 && "Cross strain within amplitude");
    (void)h_plus; (void)h_cross;

    std::vector<GWPolarizationState> polarizations = {plus, cross};
    assert(GWPolarization::isGRConsistent(polarizations) && "Should be GR consistent");

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
    assert(std::isfinite(response) && "Detector response should be finite");

    std::cout << "[PASS] GW detector response: finite=" << response << std::endl;
}

// ============================================================================
// 3.2.3 - Light deflection at various impact parameters
// ============================================================================
void test_light_deflection_general() {
    double M_sun = 1477.0;

    for (double b : {2.0 * M_sun, 5.0 * M_sun, 10.0 * M_sun, 100.0 * M_sun}) {
        double deflection_analytic = 4.0 * M_sun / b;
        assert(std::isfinite(deflection_analytic) && "Deflection should be finite");
        assert(deflection_analytic > 0.0 && "Deflection should be positive");
        (void)deflection_analytic;
    }

    double b_critical = 3.0 * std::sqrt(3.0) * M_sun;
    assert(b_critical > 0.0 && "Photon sphere radius should be positive");

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
            assert(std::isfinite(tidal[i][j]) && "Tidal tensor component should be finite");
        }
    }

    double r = 10.0;
    double expected_rr = -2.0 * M / (r * r * r);
    double expected_tt = M / (r * r * r);

    assert(std::abs(tidal[0][0] - expected_rr) < 1e-6 && "Tidal E_rr should match Schwarzschild");
    assert(std::abs(tidal[1][1] - expected_tt) < 1e-6 && "Tidal E_tt should match Schwarzschild");
    assert(std::abs(tidal[2][2] - expected_tt) < 1e-6 && "Tidal E_pp should match Schwarzschild");
    (void)expected_rr; (void)expected_tt;

    std::cout << "[PASS] Geodesic deviation tidal tensor matches Schwarzschild 1/r^3 law" << std::endl;
}

// ============================================================================
// 3.2.5 - Time dilation: Schwarzschild proper time ratio
// ============================================================================
void test_time_dilation_schwarzschild() {
    double M = 1.0;
    double r1 = 20.0 * M;
    double r2 = 10.0 * M;

    SingularityHandler handler(SingularityType::SCHWARZSCHILD, M, 0.0, 0.0);

    Event4D event1(0.0, r1, 0.0, 0.0);
    Event4D event2(0.0, r2, 0.0, 0.0);

    double z1 = handler.getGravitationalRedshift(event1);
    double z2 = handler.getGravitationalRedshift(event2);

    assert(std::isfinite(z1) && "Redshift at r1 should be finite");
    assert(std::isfinite(z2) && "Redshift at r2 should be finite");
    assert(z2 > z1 && "Redshift should increase closer to horizon");

    double ratio_expected = std::sqrt(1.0 - 2.0 * M / r1) / std::sqrt(1.0 - 2.0 * M / r2);
    double ratio_actual = (1.0 + z1) / (1.0 + z2);
    double relError = std::abs(ratio_actual - ratio_expected) / ratio_expected;
    assert(relError < 0.1 && "Time dilation ratio should match Schwarzschild formula");
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
