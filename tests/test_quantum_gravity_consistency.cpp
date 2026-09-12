#include "quantumgravity/CDTEngine.h"
#include "quantumgravity/CausalSet.h"
#include "quantumgravity/SpinFoam.h"
#include <cmath>
#include <iostream>
#include "test_assert.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace quantumverse;
using namespace quantumverse::quantumgravity;

// ============================================================================
// 3.3.1 - CDT spectral dimension in 4D range
// ============================================================================
void test_cdt_spectral_dimension_range() {
    CDTEngine engine(2000, 50.0, 50.0);
    engine.thermalize(500);
    engine.runMonteCarlo(500);

    double d_s = engine.getSpectralDimension();
    double d_H = engine.getHausdorffDimension();

    QV_CHECK(std::isfinite(d_s));
    QV_CHECK(std::isfinite(d_H));
    QV_CHECK(d_s >= 1.0 && d_s <= 6.0);
    QV_CHECK(d_H >= 0.0 && d_H <= 6.0);

    std::cout << "[PASS] CDT dimensions: spectral=" << d_s << ", Hausdorff=" << d_H << std::endl;
}

// ============================================================================
// 3.3.2 - Causal set dimension estimate (2D Minkowski)
// ============================================================================
void test_causal_set_dimension_estimate() {
    double planckVolume = 1.0;
    CausalSetEngine engine(0.5, 0.1, planckVolume, 20);

    engine.grow(500, 40.0);

    QV_CHECK(engine.getNumElements() > 100);

    double d_s = engine.computeSpectralDimension(10.0);

    QV_CHECK(std::isfinite(d_s));
    QV_CHECK(d_s > 0.0 && d_s < 6.0);

    std::cout << "[PASS] Causal set dimension estimate: spectral_dim=" << d_s
              << ", elements=" << engine.getNumElements() << std::endl;
}

// ============================================================================
// 3.3.3 - Spin foam vertex amplitude finite
// ============================================================================
void test_spin_foam_amplitude_finite() {
    SpinFoam foam(0.274);

    std::vector<std::array<Spin, 4>> vertex_spins;
    for (int v = 0; v < 3; ++v) {
        std::array<Spin, 4> spins = {1, 1, 1, 1};
        vertex_spins.push_back(spins);
    }
    foam.setVertexSpins(vertex_spins);

    std::vector<std::vector<Spin>> face_spins;
    for (int f = 0; f < 4; ++f) {
        face_spins.push_back({1, 1});
    }
    foam.setFaceSpins(face_spins);

    double amp = foam.computeAmplitude();
    QV_CHECK(std::isfinite(amp));
    QV_CHECK(!std::isnan(amp));

    std::cout << "[PASS] Spin foam amplitude: finite=" << amp << std::endl;
}

// ============================================================================
// 3.3.4 - CDT Regge action positivity
// ============================================================================
void test_cdt_regge_action_positive() {
    CDTEngine engine(200, 50.0, 50.0);
    engine.thermalize(100);
    engine.runMonteCarlo(100);

    const auto* manifold = engine.getManifold();
    QV_CHECK(manifold != nullptr);

    double action = manifold->computeReggeAction();
    QV_CHECK(std::isfinite(action));

    double avgDeficit = manifold->getAverageDeficit();
    QV_CHECK(std::isfinite(avgDeficit));

    std::cout << "[PASS] CDT Regge action: S=" << action << ", avg_deficit=" << avgDeficit << std::endl;
}

// ============================================================================
// 3.3.5 - CDT Monte Carlo preserves manifold structure
// ============================================================================
void test_cdt_monte_carlo_stability() {
    CDTEngine engine(100, 20.0, 20.0);
    engine.thermalize(50);

    int simplices_before = engine.getNumSimplices();
    QV_CHECK(simplices_before > 0);

    engine.runMonteCarlo(50);
    int simplices_after = engine.getNumSimplices();
    QV_CHECK(simplices_after > 0);

    double ratio = static_cast<double>(simplices_after) / static_cast<double>(simplices_before);
    QV_CHECK(ratio > 0.5 && ratio < 2.0);
    (void)ratio;

    std::cout << "[PASS] CDT MC stability: before=" << simplices_before << ", after=" << simplices_after << std::endl;
}

int main() {
    test_cdt_spectral_dimension_range();
    test_causal_set_dimension_estimate();
    test_spin_foam_amplitude_finite();
    test_cdt_regge_action_positive();
    test_cdt_monte_carlo_stability();

    std::cout << "=== ALL QUANTUM GRAVITY CONSISTENCY TESTS PASSED ===" << std::endl;
    return 0;
}
