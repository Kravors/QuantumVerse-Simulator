#ifndef QUANTUMVERSE_METRIC_TENSOR_H
#define QUANTUMVERSE_METRIC_TENSOR_H

/**
 * QuantumVerse Simulator - Metric Tensor
 * 
 * Represents the metric tensor g_μν for spacetime geometry
 * Supports Minkowski (flat) and curved spacetime metrics
 * 
 * Based on: Spacetime.md, Curved_spacetime.md
 * Signature: (-, +, +, +) - East Coast convention
 */

#include <array>
#include <cmath>
#include <memory>
#include "Event4D.h"

namespace quantumverse {

// Forward declaration
class StressEnergyTensor;

class MetricTensor {
public:
    // 4x4 metric tensor components
    // Using row-major storage: g[μ][ν] where μ,ν ∈ {0,1,2,3}
    // Index 0 = time, 1 = x, 2 = y, 3 = z
    std::array<std::array<double, 4>, 4> g;

    // Constructor - creates Minkowski metric by default
    MetricTensor() {
        // Minkowski metric: diag(-1, 1, 1, 1) in units where c=1
        g[0][0] = -1.0;  g[0][1] = 0.0;   g[0][2] = 0.0;   g[0][3] = 0.0;
        g[1][0] = 0.0;   g[1][1] = 1.0;   g[1][2] = 0.0;   g[1][3] = 0.0;
        g[2][0] = 0.0;   g[2][1] = 0.0;   g[2][2] = 1.0;   g[2][3] = 0.0;
        g[3][0] = 0.0;   g[3][1] = 0.0;   g[3][2] = 0.0;   g[3][3] = 1.0;
    }

    // Create Schwarzschild metric (non-rotating black hole)
    // ds² = -(1-2GM/rc²)c²dt² + (1-2GM/rc²)⁻¹dr² + r²dΩ²
    static MetricTensor schwarzschild(double mass, double r, double theta, double phi) {
        MetricTensor metric;

        // Schwarzschild radius: rs = 2GM/c²
        double rs = 2.0 * Event4D::G * mass / (Event4D::C * Event4D::C);
        double factor = 1.0 - rs / r;
        
        if (r <= rs) {
            // Inside event horizon - metric signature flips
            metric.g[0][0] = 1.0 / factor;   // Now positive (timelike becomes spacelike)
            metric.g[1][1] = -factor;        // Now negative (spacelike becomes timelike)
        } else {
            metric.g[0][0] = -factor;
            metric.g[1][1] = 1.0 / factor;
        }
        
        metric.g[2][2] = r * r;
        metric.g[3][3] = r * r * std::sin(theta) * std::sin(theta);
        
        // Off-diagonal terms for Kerr metric would go here
        
        return metric;
    }

    // Create Kerr metric (rotating black hole)
    // Includes frame-dragging effects
    static MetricTensor kerr(double mass, double angular_momentum, double r, double theta) {
        MetricTensor metric;

        // Schwarzschild radius: rs = 2GM/c²
        double rs = 2.0 * Event4D::G * mass / (Event4D::C * Event4D::C);
        double a = angular_momentum / (mass * Event4D::C);  // Dimensionless spin parameter
        double sigma = r * r + a * a * std::cos(theta) * std::cos(theta);
        double delta = r * r - rs * r + a * a;
        
        // Kerr metric components
        metric.g[0][0] = -(1.0 - rs * r / sigma);
        metric.g[0][3] = -rs * r * a * std::sin(theta) * std::sin(theta) / sigma;  // g_tφ
        metric.g[3][0] = metric.g[0][3];  // Symmetric
        metric.g[1][1] = sigma / delta;
        metric.g[2][2] = sigma;
        metric.g[3][3] = (r * r + a * a + rs * r * a * a * std::sin(theta) * std::sin(theta) / sigma) * 
                         std::sin(theta) * std::sin(theta);
        
        return metric;
    }

    // Virtual method to evaluate metric at a spacetime event
    // Subclasses override for position-dependent metrics (Schwarzschild, Kerr, etc.)
    virtual std::array<std::array<double, 4>, 4> evaluate(const Event4D& event) const {
        return g;  // Default: return stored metric (constant spacetime)
    }

    // Virtual destructor for proper polymorphic cleanup
    virtual ~MetricTensor() = default;

    // Create a diagonal metric from a 4-element array
    // Useful for constructing metrics programmatically
    static MetricTensor diagonal(const std::array<double, 4>& diag) {
        MetricTensor metric;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                metric.g[i][j] = (i == j) ? diag[i] : 0.0;
            }
        }
        return metric;
    }

    // Create metric from stress-energy tensor (Einstein field equations)
    // G_μν = (8πG/c⁴) T_μν
    static MetricTensor fromStressEnergy(const StressEnergyTensor& T) {
        // Simplified: linearized gravity approximation
        // g_μν = η_μν + h_μν where h_μν is small perturbation
        MetricTensor metric;
        
        // In weak field limit: h_00 ≈ -2Φ/c² where Φ is gravitational potential
        // For now, return Minkowski metric (full implementation requires solving EFE)
        return metric;
    }

    // Set entire metric to Minkowski (flat spacetime)
    void setToMinkowski() {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                g[i][j] = (i == j) ? (i == 0 ? -1.0 : 1.0) : 0.0;
            }
        }
    }

    // Set entire metric to zero
    void setToZero() {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                g[i][j] = 0.0;
            }
        }
    }

    // Set individual metric component g[μ][ν]
    void set(int mu, int nu, double val) {
        g[mu][nu] = val;
    }

    // Compute spacetime interval: ds² = g_μν dx^μ dx^ν
    double interval(const Event4D& event1, const Event4D& event2) const {
        double dt = event2.t - event1.t;
        double dx = event2.x - event1.x;
        double dy = event2.y - event1.y;
        double dz = event2.z - event1.z;
        
        double coords[4] = {dt, dx, dy, dz};
        
        double ds2 = 0.0;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                ds2 += g[i][j] * coords[i] * coords[j];
            }
        }
        
        return ds2;
    }

    // Compute proper time along worldline segment
    // Note: Metric is in natural units (c=1), so proper time is √(-ds²)
    double properTime(const Event4D& event1, const Event4D& event2) const {
        double ds2 = interval(event1, event2);
        if (ds2 < 0) {  // Timelike separation
            return std::sqrt(-ds2);
        }
        return 0.0;
    }

    // Check if metric is Lorentzian (correct signature) using Sylvester's criterion
    // For signature (-,+,+,+), the sequence (1, D1, D2, D3, D4) of leading
    // principal minors must have exactly 1 sign change.
    // Also requires the matrix to be symmetric.
    bool isLorentzian() const {
        // First check symmetry: g[mu][nu] == g[nu][mu]
        for (int i = 0; i < 4; i++) {
            for (int j = i + 1; j < 4; j++) {
                if (std::abs(g[i][j] - g[j][i]) > 1e-10) {
                    return false;  // Not symmetric
                }
            }
        }

        // 1x1 leading principal minor
        double D1 = g[0][0];

        // 2x2 leading principal minor
        double D2 = g[0][0] * g[1][1] - g[0][1] * g[1][0];

        // 3x3 leading principal minor (Laplace expansion along first row)
        double D3 = g[0][0] * (g[1][1] * g[2][2] - g[1][2] * g[2][1])
                - g[0][1] * (g[1][0] * g[2][2] - g[1][2] * g[2][0])
                + g[0][2] * (g[1][0] * g[2][1] - g[1][1] * g[2][0]);

        // 4x4 full determinant
        double D4 = determinant();

        // Count sign changes in sequence: 1, D1, D2, D3, D4
        double seq[5] = {1.0, D1, D2, D3, D4};
        int signChanges = 0;
        for (int i = 0; i < 4; i++) {
            if (seq[i] * seq[i + 1] < 0.0) signChanges++;
        }

        return (signChanges == 1);
    }

    // Compute metric determinant
    double determinant() const {
        // 4x4 determinant calculation
        double det = 0.0;
        
        // Laplace expansion along first row
        for (int j = 0; j < 4; j++) {
            det += g[0][j] * cofactor(0, j);
        }
        
        return det;
    }

    // Compute inverse metric g^μν
    virtual MetricTensor inverse() const {
        MetricTensor inv;
        double det = determinant();
        
        if (std::abs(det) < 1e-10) {
            // Singular metric - return Minkowski as fallback
            return MetricTensor();
        }
        
        // Compute inverse using adjugate matrix
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                inv.g[i][j] = cofactor(j, i) / det;  // Transpose of cofactor matrix
            }
        }
        
        return inv;
    }

    // Lower index: v_μ = g_μν v^ν
    std::array<double, 4> lowerIndex(const std::array<double, 4>& vector) const {
        std::array<double, 4> result = {0, 0, 0, 0};
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result[i] += g[i][j] * vector[j];
            }
        }
        return result;
    }

    // Raise index: v^μ = g^μν v_ν
    std::array<double, 4> raiseIndex(const std::array<double, 4>& covector) const {
        MetricTensor inv = inverse();
        std::array<double, 4> result = {0, 0, 0, 0};
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result[i] += inv.g[i][j] * covector[j];
            }
        }
        return result;
    }

private:
    // Compute cofactor for determinant
    double cofactor(int row, int col) const {
        // Create 3x3 submatrix
        std::array<std::array<double, 3>, 3> sub;
        int sub_i = 0, sub_j;
        
        for (int i = 0; i < 4; i++) {
            if (i == row) continue;
            sub_j = 0;
            for (int j = 0; j < 4; j++) {
                if (j == col) continue;
                sub[sub_i][sub_j] = g[i][j];
                sub_j++;
            }
            sub_i++;
        }
        
        // Compute 3x3 determinant
        double det = sub[0][0] * (sub[1][1] * sub[2][2] - sub[1][2] * sub[2][1])
                   - sub[0][1] * (sub[1][0] * sub[2][2] - sub[1][2] * sub[2][0])
                   + sub[0][2] * (sub[1][0] * sub[2][1] - sub[1][1] * sub[2][0]);
        
        // Apply sign: (-1)^(row+col)
        return ((row + col) % 2 == 0 ? 1 : -1) * det;
    }
};

// Stress-energy tensor (source of spacetime curvature)
class StressEnergyTensor {
public:
    std::array<std::array<double, 4>, 4> T;

    StressEnergyTensor() {
        // Initialize to zero
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                T[i][j] = 0.0;
            }
        }
    }

    // Create from perfect fluid: T_μν = (ρ + p/c²)u_μ u_ν + p g_μν
    static StressEnergyTensor perfectFluid(double density, double pressure, 
                                           const std::array<double, 4>& four_velocity,
                                           const MetricTensor& metric) {
        StressEnergyTensor tensor;
        
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                tensor.T[i][j] = (density + pressure / Event4D::C2) * four_velocity[i] * four_velocity[j]
                               + pressure * metric.g[i][j];
            }
        }
        
        return tensor;
    }

    // Trace: T = g^μν T_μν
    double trace(const MetricTensor& metric) const {
        MetricTensor inv = metric.inverse();
        double trace = 0.0;
        
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                trace += inv.g[i][j] * T[i][j];
            }
        }
        
        return trace;
    }
};

} // namespace quantumverse

#endif // QUANTUMVERSE_METRIC_TENSOR_H