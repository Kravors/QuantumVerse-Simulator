#ifndef QUANTUMVERSE_SO4_ROTATION_H
#define QUANTUMVERSE_SO4_ROTATION_H

/**
 * @file SO4Rotation.h
 * @brief QuantumVerse Simulator - 4D Rotation Operations (SO(4))
 *
 * Provides 4D rotation matrices in the 6 independent rotation planes.
 * In 4D, there are 6 planes of rotation: xy, xz, xw, yz, yw, zw
 * where w represents the 4th spatial dimension (or time in some contexts).
 *
 * Based on: 4D geometry for spacetime visualization
 */

#include "Matrix4x4.h"
#include <cmath>
#include <vector>

namespace quantumverse {

/**
 * @brief SO(4) rotation operations for 4D space.
 *
 * In 4D Euclidean space, there are 6 independent rotation planes:
 * - Plane 0: xy (rotation in xy-plane)
 * - Plane 1: xz (rotation in xz-plane)
 * - Plane 2: xw (rotation in xw-plane)
 * - Plane 3: yz (rotation in yz-plane)
 * - Plane 4: yw (rotation in yw-plane)
 * - Plane 5: zw (rotation in zw-plane)
 *
 * Each rotation is specified by a plane index (0-5) and an angle.
 */
class SO4Rotation {
public:
    /**
     * @brief Number of independent rotation planes in 4D.
     */
    static constexpr int NUM_PLANES = 6;

    /**
     * @brief Get the two coordinate indices for a rotation plane.
     * @param plane Plane index (0-5).
     * @return Pair of indices (i, j) for the rotation plane.
     */
    static std::pair<int, int> planeIndices(int plane) {
        switch (plane) {
            case 0: return {0, 1};  // xy
            case 1: return {0, 2};  // xz
            case 2: return {0, 3};  // xw
            case 3: return {1, 2};  // yz
            case 4: return {1, 3};  // yw
            case 5: return {2, 3};  // zw
            default: return {0, 1};
        }
    }

    /**
     * @brief Create a rotation matrix for a specific plane.
     * @param plane Plane index (0-5).
     * @param angle Rotation angle in radians.
     * @return 4x4 rotation matrix.
     */
    static Matrix4x4<double> rotation(int plane, double angle) {
        auto indices = planeIndices(plane);
        int i = indices.first;
        int j = indices.second;
        return Matrix4x4<double>::rotation(i, j, angle);
    }

    /**
     * @brief Rotate a 4D vector in a specific plane.
     * @param v Input 4D vector.
     * @param plane Plane index (0-5).
     * @param angle Rotation angle in radians.
     * @return Rotated 4D vector.
     */
    static std::array<double, 4> rotateVector(
        const std::array<double, 4>& v,
        int plane,
        double angle
    ) {
        auto indices = planeIndices(plane);
        int i = indices.first;
        int j = indices.second;

        double cosA = std::cos(angle);
        double sinA = std::sin(angle);

        std::array<double, 4> result = v;
        double vi = v[i];
        double vj = v[j];

        result[i] = cosA * vi - sinA * vj;
        result[j] = sinA * vi + cosA * vj;

        return result;
    }

    /**
     * @brief Create a compound rotation from multiple plane rotations.
     * @param planes Array of plane indices.
     * @param angles Array of rotation angles (same length as planes).
     * @return Combined 4x4 rotation matrix.
     */
    static Matrix4x4<double> compoundRotation(
        const std::vector<int>& planes,
        const std::vector<double>& angles
    ) {
        Matrix4x4<double> result = Matrix4x4<double>::identity();

        for (size_t k = 0; k < planes.size() && k < angles.size(); ++k) {
            result = result * rotation(planes[k], angles[k]);
        }

        return result;
    }

    /**
     * @brief Rotate a 4D vector by multiple plane rotations.
     * @param v Input 4D vector.
     * @param planes Array of plane indices.
     * @param angles Array of rotation angles.
     * @return Rotated 4D vector.
     */
    static std::array<double, 4> rotateVector(
        const std::array<double, 4>& v,
        const std::vector<int>& planes,
        const std::vector<double>& angles
    ) {
        std::array<double, 4> result = v;

        for (size_t k = 0; k < planes.size() && k < angles.size(); ++k) {
            result = rotateVector(result, planes[k], angles[k]);
        }

        return result;
    }

    /**
     * @brief Identity rotation (no change).
     * @return Identity matrix.
     */
    static Matrix4x4<double> identity() {
        return Matrix4x4<double>::identity();
    }

    /**
     * @brief Check if a matrix is a valid SO(4) rotation.
     * @param m Matrix to check.
     * @return True if matrix is orthogonal with determinant 1.
     */
    static bool isRotation(const Matrix4x4<double>& m) {
        // Check orthogonality: M^T * M = I
        auto mt = m.transpose();
        auto product = mt * m;

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                double expected = (i == j) ? 1.0 : 0.0;
                if (std::abs(product(i, j) - expected) > 1e-10) {
                    return false;
                }
            }
        }

        // Check determinant = 1
        return std::abs(m.determinant() - 1.0) < 1e-10;
    }
};

} // namespace quantumverse

#endif // QUANTUMVERSE_SO4_ROTATION_H