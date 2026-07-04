#ifndef QUANTUMVERSE_MATRIX4X4_H
#define QUANTUMVERSE_MATRIX4X4_H

#include <array>
#include <cmath>
#include <iostream>
#include <iomanip>

namespace quantumverse {

/**
 * Matrix4x4 - 4x4 matrix for spacetime transformations
 * 
 * Supports Lorentz transformations, metric tensors,
 * and general coordinate transformations in 4D spacetime.
 */
template<typename T = double>
class Matrix4x4 {
private:
    std::array<std::array<T, 4>, 4> data_;

public:
    // Constructors
    Matrix4x4() {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                data_[i][j] = (i == j) ? 1 : 0;
    }
    
    explicit Matrix4x4(T value) {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                data_[i][j] = value;
    }
    
    // Identity matrix
    static Matrix4x4 identity() {
        return Matrix4x4();
    }
    
    // Zero matrix
    static Matrix4x4 zero() {
        return Matrix4x4(T(0));
    }
    
    // Minkowski metric (flat spacetime)
    static Matrix4x4 minkowski() {
        Matrix4x4 m;
        m(0, 0) = -1;  // Time component
        m(1, 1) = 1;   // Spatial x
        m(2, 2) = 1;   // Spatial y
        m(3, 3) = 1;   // Spatial z
        return m;
    }
    
    // Access operators
    T& operator()(int i, int j) { return data_[i][j]; }
    const T& operator()(int i, int j) const { return data_[i][j]; }
    
    // Row access
    std::array<T, 4>& operator[](int i) { return data_[i]; }
    const std::array<T, 4>& operator[](int i) const { return data_[i]; }
    
    // Matrix operations
    Matrix4x4 operator+(const Matrix4x4& other) const {
        Matrix4x4 result;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                result(i, j) = data_[i][j] + other(i, j);
        return result;
    }
    
    Matrix4x4 operator-(const Matrix4x4& other) const {
        Matrix4x4 result;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                result(i, j) = data_[i][j] - other(i, j);
        return result;
    }
    
    Matrix4x4 operator*(T scalar) const {
        Matrix4x4 result;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                result(i, j) = data_[i][j] * scalar;
        return result;
    }
    
    Matrix4x4 operator/(T scalar) const {
        if (std::abs(scalar) < 1e-10) return *this;
        Matrix4x4 result;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                result(i, j) = data_[i][j] / scalar;
        return result;
    }
    
    // Matrix multiplication
    Matrix4x4 operator*(const Matrix4x4& other) const {
        Matrix4x4 result(0);
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                T sum = 0;
                for (int k = 0; k < 4; k++) {
                    sum += data_[i][k] * other(k, j);
                }
                result(i, j) = sum;
            }
        }
        return result;
    }
    
    // Matrix-vector multiplication
    template<typename U>
    std::array<U, 4> multiply(const std::array<U, 4>& vec) const {
        std::array<U, 4> result = {0, 0, 0, 0};
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result[i] += static_cast<U>(data_[i][j]) * vec[j];
            }
        }
        return result;
    }
    
    // Transpose
    Matrix4x4 transpose() const {
        Matrix4x4 result;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                result(i, j) = data_[j][i];
        return result;
    }
    
    // Determinant (4x4)
    T determinant() const {
        T det = 0;
        for (int i = 0; i < 4; i++) {
            det += data_[0][i] * cofactor(0, i);
        }
        return det;
    }
    
    // Cofactor
    T cofactor(int row, int col) const {
        return minor(row, col) * (((row + col) % 2 == 0) ? 1 : -1);
    }
    
    // Minor (3x3 determinant)
    T minor(int row, int col) const {
        T m[3][3];
        int mi = 0, mj = 0;
        
        for (int i = 0; i < 4; i++) {
            if (i == row) continue;
            mj = 0;
            for (int j = 0; j < 4; j++) {
                if (j == col) continue;
                m[mi][mj] = data_[i][j];
                mj++;
            }
            mi++;
        }
        
        return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
             - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
             + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    }
    
    // Inverse
    Matrix4x4 inverse() const {
        T det = determinant();
        if (std::abs(det) < 1e-10) return identity();
        
        Matrix4x4 result;
        T invDet = 1.0 / det;
        
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result(j, i) = cofactor(i, j) * invDet;
            }
        }
        
        return result;
    }
    
    // Trace
    T trace() const {
        return data_[0][0] + data_[1][1] + data_[2][2] + data_[3][3];
    }
    
    // Generate Lorentz boost matrix
    static Matrix4x4 lorentzBoost(T vx, T vy, T vz) {
        T v2 = vx*vx + vy*vy + vz*vz;
        if (v2 >= 1.0) return identity();  // Invalid
        
        T gamma = 1.0 / std::sqrt(1.0 - v2);
        
        Matrix4x4 result;
        result(0, 0) = gamma;
        result(0, 1) = -gamma * vx;
        result(0, 2) = -gamma * vy;
        result(0, 3) = -gamma * vz;
        
        result(1, 0) = -gamma * vx;
        result(1, 1) = 1.0 + (gamma - 1.0) * vx*vx / v2;
        result(1, 2) = (gamma - 1.0) * vx*vy / v2;
        result(1, 3) = (gamma - 1.0) * vx*vz / v2;
        
        result(2, 0) = -gamma * vy;
        result(2, 1) = (gamma - 1.0) * vy*vx / v2;
        result(2, 2) = 1.0 + (gamma - 1.0) * vy*vy / v2;
        result(2, 3) = (gamma - 1.0) * vy*vz / v2;
        
        result(3, 0) = -gamma * vz;
        result(3, 1) = (gamma - 1.0) * vz*vx / v2;
        result(3, 2) = (gamma - 1.0) * vz*vy / v2;
        result(3, 3) = 1.0 + (gamma - 1.0) * vz*vz / v2;
        
        return result;
    }
    
    // Generate rotation matrix in 4D (in plane ij by angle)
    static Matrix4x4 rotation(int i, int j, T angle) {
        Matrix4x4 result = identity();
        T cosA = std::cos(angle);
        T sinA = std::sin(angle);
        
        result(i, i) = cosA;
        result(i, j) = -sinA;
        result(j, i) = sinA;
        result(j, j) = cosA;
        
        return result;
    }
    
    // Output
    friend std::ostream& operator<<(std::ostream& os, const Matrix4x4& m) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                os << std::setw(12) << m(i, j) << " ";
            }
            os << "\n";
        }
        return os;
    }
};

} // namespace quantumverse

#endif // QUANTUMVERSE_MATRIX4X4_H