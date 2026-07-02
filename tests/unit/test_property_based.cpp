#include "math/Vector4D.h"
#include "math/Matrix4x4.h"
#include <cmath>
#include <cassert>

using namespace quantumverse;

void test_vector4d_lorentzian_symmetry() {
    Vector4d a(1.0, 2.0, 3.0, 4.0);
    Vector4d b(2.0, 3.0, 4.0, 5.0);
    
    double dot_ab = a.lorentzianDot(b);
    double dot_ba = b.lorentzianDot(a);
    
    assert(dot_ab == dot_ba);
}

void test_vector4d_euclidean_symmetry() {
    Vector4d a(1.0, 2.0, 3.0, 4.0);
    Vector4d b(2.0, 3.0, 4.0, 5.0);
    
    double dot_ab = a.euclideanDot(b);
    double dot_ba = b.euclideanDot(a);
    
    assert(dot_ab == dot_ba);
}

void test_vector4d_norm_consistency() {
    Vector4d v(1.0, 2.0, 3.0, 4.0);
    
    double normSq = v.normSquaredLorentzian();
    double dot = v.lorentzianDot(v);
    
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
}

int main() {
    test_vector4d_lorentzian_symmetry();
    test_vector4d_euclidean_symmetry();
    test_vector4d_norm_consistency();
    test_matrix4x4_transpose_double();
    test_matrix4x4_minkowski_signature();
    test_vector4d_timelike_classification();
    test_matrix4x4_identity_properties();
    
    return 0;
}