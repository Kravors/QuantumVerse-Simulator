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

/**
 * @brief Curvature invariants at a spacetime event.
 *
 * Returned by MetricTensor::curvatureScalars(). A metric that can supply
 * exact invariants (e.g. Schwarzschild) overrides that method and sets
 * valid=true; the default returns invalid so callers fall back to numerical
 * differentiation.
 */
struct CurvatureScalars {
    double kretschmann = 0.0;   ///< K = R_{ρσμν} R^{ρσμν}
    double ricciScalar = 0.0;   ///< R = g^{μν} R_{μν}
    double weylSquared = 0.0;   ///< C_{abcd} C^{abcd}
    bool valid = false;         ///< false => use numerical fallback
};

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

    /**
     * @brief Creates a Schwarzschild metric for a non-rotating black hole.
     * @param mass Black hole mass in kg.
     * @param r Radial coordinate in meters.
     * @param theta Polar angle in radians.
     * @param phi Azimuthal angle in radians (unused for spherical symmetry).
     * @return Schwarzschild metric tensor.
     *
     * Line element: ds² = -(1-2GM/rc²)c²dt² + (1-2GM/rc²)⁻¹dr² + r²dΩ²
     */
    static MetricTensor schwarzschild(double mass, double r, double theta, double phi) {
        (void)phi;
        MetricTensor metric;

        // Schwarzschild radius: rs = 2GM/c²
        double rs = 2.0 * Event4D::G * mass / (Event4D::C * Event4D::C);
        double factor = 1.0 - rs / r;
        
        if (r <= rs) {
            // Inside event horizon - metric signature flips
            // g[0][0] becomes positive (time becomes space-like)
            // g[1][1] becomes negative (radial becomes time-like)
            // factor = 1 - rs/r < 0 when r < rs
            metric.g[0][0] = -factor;       // -factor > 0 (positive)
            metric.g[1][1] = 1.0 / factor;  // 1/factor < 0 (negative)
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

    /**
     * @brief Evaluates the metric tensor at a spacetime event.
     * @param event The spacetime event at which to evaluate.
     * @return 4x4 metric tensor g_μν.
     *
     * Subclasses override for position-dependent metrics (Schwarzschild, Kerr, etc.).
     */
    virtual std::array<std::array<double, 4>, 4> evaluate(const Event4D& event) const {
        (void)event;
        return g;  // Default: return stored metric (constant spacetime)
    }

    /**
     * @brief Curvature invariants at an event, if analytically known.
     *
     * Base returns invalid; subclasses with closed-form curvature (e.g.
     * SchwarzschildMetric) override and set valid=true. Callers should use
     * this when valid, falling back to CurvatureCalculator otherwise.
     */
    virtual CurvatureScalars curvatureScalars(const Event4D& /*event*/) const {
        return CurvatureScalars{};  // invalid by default
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
        (void)T;
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

/**
 * @brief Position-dependent Schwarzschild metric.
 *
 * Unlike the constant base MetricTensor, this evaluates the Schwarzschild
 * geometry at the actual spacetime event. Routing curvature computations,
 * geodesic integration, and instrument scans through evaluate() makes the
 * renderer surface, the probe readout, and the scanner see real curvature
 * instead of a single flat point metric.
 */
class SchwarzschildMetric : public MetricTensor {
public:
    explicit SchwarzschildMetric(double massKg) : m_mass(massKg) {}

    /// @brief Black hole mass in kilograms.
    double mass() const { return m_mass; }

    std::array<std::array<double, 4>, 4> evaluate(const Event4D& event) const override {
        if (!std::isfinite(event.x) || !std::isfinite(event.y) || !std::isfinite(event.z)) {
            return MetricTensor().g;
        }

        // Return the metric in the SAME (t, x, y, z) Cartesian basis that the
        // curvature/geodesic calculators finite-difference. The Schwarzschild
        // line element ds^2 = -(1-rs/r) dt^2 + (1-rs/r)^{-1} dr^2 + r^2 dΩ^2
        // expands to g_ij = δ_ij + (rs/(r-rs)) x_i x_j / r^2 in Cartesian
        // coordinates. (Returning spherical components g_θθ=r^2 while the
        // calculators perturb x,y,z produced a spurious, basis-mismatched
        // curvature of ~4 even for flat space.)
        double x = event.x, y = event.y, z = event.z;
        double r = std::sqrt(x * x + y * y + z * z);
        const double minR = 1e-9;
        if (r < minR) r = minR;

        const double c2 = Event4D::C * Event4D::C;
        double rs = 2.0 * Event4D::G * m_mass / c2;
        double factor = 1.0 - rs / r;                 // g_tt = -factor

        // Spatial correction coefficient; clamp inside the horizon to avoid
        // the singular blow-up (geometric view is degenerate there anyway).
        double fr = (r > rs) ? rs / (r - rs) : 0.0;
        double inv_r2 = 1.0 / (r * r);

        std::array<std::array<double, 4>, 4> g4{};
        g4[0][0] = -factor;
        double xs[3] = {x, y, z};
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                double delta = (i == j) ? 1.0 : 0.0;
                g4[i + 1][j + 1] = delta + fr * (xs[i] * xs[j] * inv_r2);
            }
        }
        return g4;
    }

    CurvatureScalars curvatureScalars(const Event4D& event) const override {
        // Exact Schwarzschild invariants (vacuum: Ricci = 0, Riemann = Weyl).
        // Using the closed form avoids finite-difference round-off, which
        // otherwise zeroes the curvature at astronomical probe distances.
        double r = std::sqrt(event.x * event.x + event.y * event.y + event.z * event.z);
        const double minR = 1e-9;
        if (r < minR) r = minR;

        const double c4 = Event4D::C * Event4D::C * Event4D::C * Event4D::C;
        double G = Event4D::G;
        double K = 48.0 * G * G * m_mass * m_mass / (c4 * std::pow(r, 6));

        CurvatureScalars s;
        s.kretschmann = K;
        s.ricciScalar = 0.0;    // vacuum
        s.weylSquared = K;      // vacuum: Riemann == Weyl
        s.valid = true;
        return s;
    }

private:
    double m_mass;
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