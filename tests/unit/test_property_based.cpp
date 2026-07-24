#include "math/Vector4D.h"
#include "math/Matrix4x4.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "quantumgravity/CDTEngine.h"
#include "physics/CurvatureCalculator.h"
#include <cmath>
#include <cassert>
#include <cstdlib>
#include <ctime>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace quantumverse;

double randDouble(double lo, double hi) {
    return lo + (hi - lo) * (std::rand() / (RAND_MAX + 1.0));
}

Event4D randomEvent(unsigned int* /*seed*/) {
    double t = randDouble(-1e3, 1e3);
    double x = randDouble(1.0, 1e6);
    double y = randDouble(-1e5, 1e5);
    double z = randDouble(-1e5, 1e5);
    return Event4D(t, x, y, z);
}

void test_lorentz_interval_invariance_random() {
    std::srand(42u);
    MetricTensor eta;
    for (int iter = 0; iter < 200; ++iter) {
        Event4D e1 = randomEvent(nullptr);
        Event4D e2 = randomEvent(nullptr);
        double ds2_original = eta.interval(e1, e2);
        (void)ds2_original;

        double beta = randDouble(-0.9, 0.9);
        Matrix4x4<double> boost = Matrix4x4<double>::lorentzBoost(beta, 0.0, 0.0);
        auto bd = boost;
        (void)bd;
        Event4D e1p(
            bd[0][0] * e1.t + bd[0][1] * e1.x,
            bd[1][0] * e1.t + bd[1][1] * e1.x,
            e1.y,
            e1.z
        );
        Event4D e2p(
            bd[0][0] * e2.t + bd[0][1] * e2.x,
            bd[1][0] * e2.t + bd[1][1] * e2.x,
            e2.y,
            e2.z
        );
        double ds2_boosted = eta.interval(e1p, e2p);
        (void)ds2_boosted;
        assert(std::abs(ds2_original - ds2_boosted) < 1e-9 * std::max(std::abs(ds2_original), 1.0));
    }
}

void test_metric_tensor_symmetry_random() {
    std::srand(123u);
    for (int iter = 0; iter < 100; ++iter) {
        double mass = randDouble(1e30, 1e35);
        double r = randDouble(1e4, 1e15);
        double theta = randDouble(0.1, M_PI - 0.1);
        Event4D ev(0.0, r, 0.0, 0.0);

        auto sch = MetricTensor::schwarzschild(mass, r, theta, 0.0);
        for (int i = 0; i < 4; i++) {
            for (int j = i + 1; j < 4; j++) {
                assert(std::abs(sch.g[i][j] - sch.g[j][i]) < 1e-12);
            }
        }

        double a = randDouble(0.0, 0.99) * mass * Event4D::C;
        auto kerr = MetricTensor::kerr(mass, a, r, theta);
        for (int i = 0; i < 4; i++) {
            for (int j = i + 1; j < 4; j++) {
                assert(std::abs(kerr.g[i][j] - kerr.g[j][i]) < 1e-12);
            }
        }
    }
}

void test_kretschmann_non_negativity_random() {
    std::srand(456u);
    for (int iter = 0; iter < 100; ++iter) {
        double mass = randDouble(1e30, 1e35);
        SchwarzschildMetric sch(mass);
        double r = randDouble(1e4, 1e15);
        Event4D ev(0.0, r, 0.0, 0.0);
        auto scalars = sch.curvatureScalars(ev);
        assert(scalars.valid);
        assert(scalars.kretschmann >= 0.0);
    }
}

void test_metric_tensor_symmetry() {
    auto minkowski = MetricTensor();
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            assert(std::abs(minkowski.g[i][j] - minkowski.g[j][i]) < 1e-12);
        }
    }

    SchwarzschildMetric sch(1.989e30);
    auto g = sch.evaluate(Event4D(0, 1e10, 0, 0));
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            assert(std::abs(g[i][j] - g[j][i]) < 1e-12);
        }
    }
    (void)g;

    auto kerrMetric = MetricTensor::kerr(1.989e30, 0.5 * 1.989e30 * 299792458.0, 1e10, M_PI / 2);
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            assert(std::abs(kerrMetric.g[i][j] - kerrMetric.g[j][i]) < 1e-12);
        }
    }
}

void test_lorentz_interval_invariance() {
    Event4D e1(0.0, 0.0, 0.0, 0.0);
    Event4D e2(1.0, 3.0, 0.0, 0.0);

    MetricTensor eta;
    double ds2_original = eta.interval(e1, e2);
    (void)ds2_original;

    Matrix4x4<double> boost = Matrix4x4<double>::lorentzBoost(0.6, 0.0, 0.0);
    auto bdata = boost;
    (void)bdata;

    Event4D e1p(
        bdata[0][0] * e1.t + bdata[0][1] * e1.x,
        bdata[1][0] * e1.t + bdata[1][1] * e1.x,
        e1.y,
        e1.z
    );
    Event4D e2p(
        bdata[0][0] * e2.t + bdata[0][1] * e2.x,
        bdata[1][0] * e2.t + bdata[1][1] * e2.x,
        e2.y,
        e2.z
    );

    double ds2_boosted = eta.interval(e1p, e2p);
    (void)ds2_boosted;
    assert(std::abs(ds2_original - ds2_boosted) < 1e-9);
}

void test_cdt_regge_action_idempotency() {
    quantumverse::quantumgravity::CDTEngine engine(100, 20.0, 20.0);
    engine.thermalize(50);
    double action1 = engine.getManifold()->computeReggeAction();
    double action2 = engine.getManifold()->computeReggeAction();
    (void)action1;
    (void)action2;
    assert(action1 == action2);
}

void test_kretschmann_non_negativity() {
    SchwarzschildMetric sch(10.0 * 1.989e30);
    double radii[] = {1e3, 1e6, 1e9, 1e12, 1e15};
    for (double r : radii) {
        Event4D ev(0.0, r, 0.0, 0.0);
        auto scalars = sch.curvatureScalars(ev);
        assert(scalars.valid);
        assert(scalars.kretschmann >= 0.0);
        (void)scalars;
    }
}

void test_vector4d_lorentzian_symmetry() {
    Vector4d a(1.0, 2.0, 3.0, 4.0);
    Vector4d b(2.0, 3.0, 4.0, 5.0);
    
    double dot_ab = a.lorentzianDot(b);
    double dot_ba = b.lorentzianDot(a);
    (void)dot_ab;
    (void)dot_ba;
    
    assert(dot_ab == dot_ba);
}

void test_vector4d_euclidean_symmetry() {
    Vector4d a(1.0, 2.0, 3.0, 4.0);
    Vector4d b(2.0, 3.0, 4.0, 5.0);
    
    double dot_ab = a.euclideanDot(b);
    double dot_ba = b.euclideanDot(a);
    (void)dot_ab;
    (void)dot_ba;
    
    assert(dot_ab == dot_ba);
}

void test_vector4d_norm_consistency() {
    Vector4d v(1.0, 2.0, 3.0, 4.0);
    
    double normSq = v.normSquaredLorentzian();
    double dot = v.lorentzianDot(v);
    (void)normSq;
    (void)dot;
    
    assert(normSq == dot);
}

void test_matrix4x4_transpose_double() {
    Matrix4x4<double> m;
    m(0, 1) = 1.5;
    m(1, 0) = 2.5;
    m(2, 3) = 3.5;
    m(3, 2) = 4.5;
    
    auto tt = m.transpose().transpose();
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            assert(tt(i, j) == m(i, j));
        }
    }
    (void)tt;
}

void test_matrix4x4_minkowski_signature() {
    auto eta = Matrix4x4<double>::minkowski();
    
    assert(eta(0, 0) == -1.0);
    assert(eta(1, 1) == 1.0);
    assert(eta(2, 2) == 1.0);
    assert(eta(3, 3) == 1.0);
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (i != j) {
                assert(eta(i, j) == 0.0);
            }
        }
    }
    (void)eta;
}

void test_vector4d_timelike_classification() {
    Vector4d timelike(10.0, 1.0, 0.0, 0.0);
    Vector4d spacelike(1.0, 10.0, 0.0, 0.0);
    Vector4d lightlike(1.0, 1.0, 0.0, 0.0);
    
    assert(timelike.isTimelike());
    assert(spacelike.isSpacelike());
    assert(lightlike.isLightlike());
}

void test_matrix4x4_identity_properties() {
    auto I = Matrix4x4<double>::identity();
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (i == j) {
                assert(I(i, j) == 1.0);
            } else {
                assert(I(i, j) == 0.0);
            }
        }
    }
    (void)I;
}

int main() {
    test_vector4d_lorentzian_symmetry();
    test_vector4d_euclidean_symmetry();
    test_vector4d_norm_consistency();
    test_matrix4x4_transpose_double();
    test_matrix4x4_minkowski_signature();
    test_vector4d_timelike_classification();
    test_matrix4x4_identity_properties();
    test_metric_tensor_symmetry();
    test_lorentz_interval_invariance();
    test_lorentz_interval_invariance_random();
    test_metric_tensor_symmetry_random();
    test_kretschmann_non_negativity();
    test_kretschmann_non_negativity_random();
    test_cdt_regge_action_idempotency();

    return 0;
}