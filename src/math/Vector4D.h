#ifndef QUANTUMVERSE_VECTOR4D_H
#define QUANTUMVERSE_VECTOR4D_H

#include <cmath>
#include <array>
#include <iostream>

namespace quantumverse {

/**
 * Vector4D - 4D vector mathematics for spacetime operations
 * 
 * Provides comprehensive 4D vector operations including:
 * - Lorentzian inner product (with signature (-, +, +, +))
 * - Euclidean operations
 * - Normalization and projection
 * - Linear algebra operations
 */
template<typename T = double>
class Vector4D {
private:
    std::array<T, 4> components;

public:
    // Constructors
    Vector4D() : components{0, 0, 0, 0} {}
    
    Vector4D(T t, T x, T y, T z) : components{t, x, y, z} {}
    
    explicit Vector4D(const std::array<T, 4>& arr) : components(arr) {}
    
    // Copy constructor
    Vector4D(const Vector4D&) = default;
    Vector4D& operator=(const Vector4D&) = default;
    
    // Accessors
    T& operator[](int i) { return components[i]; }
    const T& operator[](int i) const { return components[i]; }
    
    T t() const { return components[0]; }
    T x() const { return components[1]; }
    T y() const { return components[2]; }
    T z() const { return components[3]; }
    
    T& t() { return components[0]; }
    T& x() { return components[1]; }
    T& y() { return components[2]; }
    T& z() { return components[3]; }
    
    const std::array<T, 4>& data() const { return components; }
    
    // Lorentzian inner product: -t1*t2 + x1*x2 + y1*y2 + z1*z2
    T lorentzianDot(const Vector4D& other) const {
        return -components[0] * other.components[0]
               + components[1] * other.components[1]
               + components[2] * other.components[2]
               + components[3] * other.components[3];
    }
    
    // Euclidean dot product
    T euclideanDot(const Vector4D& other) const {
        return components[0] * other.components[0]
               + components[1] * other.components[1]
               + components[2] * other.components[2]
               + components[3] * other.components[3];
    }
    
    // Squared Lorentzian norm
    T normSquaredLorentzian() const {
        return lorentzianDot(*this);
    }
    
    // Squared Euclidean norm
    T normSquaredEuclidean() const {
        return euclideanDot(*this);
    }
    
    // Lorentzian norm (can be imaginary for spacelike vectors)
    double normLorentzian() const {
        T n2 = normSquaredLorentzian();
        if (n2 > 0) return std::sqrt(static_cast<double>(n2));      // Timelike
        if (n2 < 0) return -std::sqrt(static_cast<double>(-n2));   // Spacelike (negative)
        return 0.0;  // Lightlike
    }
    
    // Euclidean norm
    double normEuclidean() const {
        return std::sqrt(static_cast<double>(normSquaredEuclidean()));
    }
    
    // Normalize in Lorentzian sense (for timelike vectors)
    Vector4D normalizeLorentzian() const {
        T n2 = normSquaredLorentzian();
        if (std::abs(n2) < 1e-10) return *this;  // Lightlike - cannot normalize
        T invNorm = 1.0 / std::sqrt(std::abs(static_cast<double>(n2)));
        return Vector4D(components[0] * invNorm,
                       components[1] * invNorm,
                       components[2] * invNorm,
                       components[3] * invNorm);
    }
    
    // Normalize in Euclidean sense
    Vector4D normalizeEuclidean() const {
        T n = normEuclidean();
        if (n < 1e-10) return *this;
        return Vector4D(components[0] / n,
                       components[1] / n,
                       components[2] / n,
                       components[3] / n);
    }
    
    // Arithmetic operators
    Vector4D operator+(const Vector4D& other) const {
        return Vector4D(components[0] + other.components[0],
                       components[1] + other.components[1],
                       components[2] + other.components[2],
                       components[3] + other.components[3]);
    }
    
    Vector4D operator-(const Vector4D& other) const {
        return Vector4D(components[0] - other.components[0],
                       components[1] - other.components[1],
                       components[2] - other.components[2],
                       components[3] - other.components[3]);
    }
    
    Vector4D operator-() const {
        return Vector4D(-components[0], -components[1],
                       -components[2], -components[3]);
    }
    
    Vector4D operator*(T scalar) const {
        return Vector4D(components[0] * scalar,
                       components[1] * scalar,
                       components[2] * scalar,
                       components[3] * scalar);
    }
    
    Vector4D operator/(T scalar) const {
        if (std::abs(scalar) < 1e-10) return *this;
        return Vector4D(components[0] / scalar,
                       components[1] / scalar,
                       components[2] / scalar,
                       components[3] / scalar);
    }
    
    Vector4D& operator+=(const Vector4D& other) {
        for (int i = 0; i < 4; i++) components[i] += other.components[i];
        return *this;
    }
    
    Vector4D& operator-=(const Vector4D& other) {
        for (int i = 0; i < 4; i++) components[i] -= other.components[i];
        return *this;
    }
    
    Vector4D& operator*=(T scalar) {
        for (int i = 0; i < 4; i++) components[i] *= scalar;
        return *this;
    }
    
    Vector4D& operator/=(T scalar) {
        if (std::abs(scalar) < 1e-10) return *this;
        for (int i = 0; i < 4; i++) components[i] /= scalar;
        return *this;
    }
    
    bool operator==(const Vector4D& other) const {
        for (int i = 0; i < 4; i++) {
            if (std::abs(components[i] - other.components[i]) > 1e-10)
                return false;
        }
        return true;
    }
    
    bool operator!=(const Vector4D& other) const {
        return !(*this == other);
    }
    
    // Check vector type based on Lorentzian norm
    bool isTimelike() const {
        return normSquaredLorentzian() < -1e-10;
    }
    
    bool isSpacelike() const {
        return normSquaredLorentzian() > 1e-10;
    }
    
    bool isLightlike() const {
        return std::abs(normSquaredLorentzian()) <= 1e-10;
    }
    
    // Projection operations
    Vector4D projectTimelike() const {
        if (!isTimelike()) return Vector4D();
        return normalizeLorentzian();
    }
    
    Vector4D projectSpacelike() const {
        // Project onto spatial subspace (orthogonal to time direction)
        return Vector4D(0, components[1], components[2], components[3]);
    }
    
    // Static utility functions
    static T lorentzianDot(const Vector4D& a, const Vector4D& b) {
        return a.lorentzianDot(b);
    }
    
    static T euclideanDot(const Vector4D& a, const Vector4D& b) {
        return a.euclideanDot(b);
    }
    
    static Vector4D lerp(const Vector4D& a, const Vector4D& b, T t) {
        return a + (b - a) * t;
    }
    
    // Output stream
    friend std::ostream& operator<<(std::ostream& os, const Vector4D& v) {
        os << "(" << v.components[0] << ", " << v.components[1]
           << ", " << v.components[2] << ", " << v.components[3] << ")";
        return os;
    }
};

// Common type aliases
using Vector4d = Vector4D<double>;
using Vector4f = Vector4D<float>;

} // namespace quantumverse

#endif // QUANTUMVERSE_VECTOR4D_H