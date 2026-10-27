// QuantumVerse Physics Validator - Reality Monitor
//
// Validates mathematical and physical invariants of the simulation in real-time:
//   * Lorentzian signature: (-,+,+,+)
//   * Determinant: det(g) < 0
//   * Asymptotic flatness at large r
//   * Energy/angular momentum conservation along geodesics
//   * 4-velocity norm: u·u = -1 (massive) or 0 (photon)
//   * Kretschmann scaling: K ~ 1/r^6
//
// Runs every N frames inside QmlGlViewport::updateSimulation() alongside the
// existing [REALITY] diagnostic dump.

#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "../spacetime/Event4D.h"
#include "../spacetime/MetricTensor.h"

namespace quantumverse {

class CurvatureRenderer;

// Result of a single validation check
struct PhysicsCheck {
    std::string name;
    bool passed;
    double value;      // measured value (for logging)
    double expected;   // expected value (for logging)
    double tolerance;  // allowed deviation
    std::string message; // human-readable detail
};

// Summary of one validation sweep over the current state
struct PhysicsReport {
    int totalChecks = 0;
    int passedChecks = 0;
    int failedChecks = 0;
    std::vector<PhysicsCheck> failures;
};

// Summary of validation across N probes (Layer 2: multi-probe coverage)
struct ValidationResult {
    bool passed = true;
    std::vector<PhysicsReport> perProbe;
    int totalChecks = 0;
    int totalFailures = 0;
};

// Validates metric tensor and geodesic invariants at the current probe
// position. Call once per N frames (aligned with the [REALITY] log).
class PhysicsValidator {
public:
    explicit PhysicsValidator(std::shared_ptr<MetricTensor> metric)
        : m_metric(std::move(metric)) {}

    // Set the metric (called when the probe metric changes)
    void setMetric(std::shared_ptr<MetricTensor> metric) { m_metric = std::move(metric); }

    // Run a full validation sweep at the given probe position.
    // Returns a report with pass/fail status for every check.
    PhysicsReport validate(const Event4D& probePos);

    // Quick boolean: is everything currently valid?
    bool isValid() const { return m_lastReport.failedChecks == 0; }

    // Run all checks at each probe in `probes` (Layer 2: multi-probe coverage).
    // Metric-agnostic: signature, determinant, Kretschmann >= 0, Ricci finite.
    // Header-only so tests can use it without pulling in CurvatureRenderer's GL deps.
    ValidationResult validateAtProbes(const MetricTensor& metric,
                                       const std::vector<Event4D>& probes) const {
        ValidationResult result;

        for (const auto& probe : probes) {
            PhysicsReport report;
            auto g = metric.evaluate(probe);

            // Check 1: Lorentzian signature (-,+,+,+)
            {
                PhysicsCheck c;
                c.name = "lorentzian_signature";
                bool ok = checkLorentzianSignature(g, c);
                report.totalChecks++;
                if (!ok) {
                    report.failedChecks++;
                    report.failures.push_back(c);
                }
            }

            // Check 2: det(g) < 0
            {
                PhysicsCheck c;
                c.name = "metric_determinant";
                double det = determinant4x4(g);
                c.value = det;
                c.expected = -1.0;
                c.tolerance = 1e-6;
                c.passed = det < 0.0 && std::isfinite(det);
                c.message = c.passed ? "det(g) < 0 (Lorentzian)"
                                     : "det(g) >= 0 or NaN/Inf (signature violation)";
                report.totalChecks++;
                if (!c.passed) {
                    report.failedChecks++;
                    report.failures.push_back(c);
                }
            }

            // Check 3: Kretschmann >= 0 and finite
            {
                PhysicsCheck c;
                c.name = "kretschmann_non_negative";
                auto scalars = metric.curvatureScalars(probe);
                c.value = scalars.kretschmann;
                c.passed = scalars.valid && std::isfinite(scalars.kretschmann)
                           && scalars.kretschmann >= 0.0;
                c.expected = 0.0;
                c.tolerance = 0.0;
                c.message = c.passed ? "K >= 0 and finite"
                                     : "K invalid, negative, or non-finite";
                report.totalChecks++;
                if (!c.passed) {
                    report.failedChecks++;
                    report.failures.push_back(c);
                }
            }

            // Check 4: Ricci scalar finite
            {
                PhysicsCheck c;
                c.name = "ricci_scalar_finite";
                auto scalars = metric.curvatureScalars(probe);
                c.value = scalars.ricciScalar;
                c.passed = scalars.valid && std::isfinite(scalars.ricciScalar);
                c.expected = 0.0;
                c.tolerance = 0.0;
                c.message = c.passed ? "R finite" : "R non-finite";
                report.totalChecks++;
                if (!c.passed) {
                    report.failedChecks++;
                    report.failures.push_back(c);
                }
            }

            result.perProbe.push_back(std::move(report));
            result.totalChecks += report.totalChecks;
            result.totalFailures += report.failedChecks;
            if (report.failedChecks > 0) result.passed = false;
        }

        return result;
    }

    // Get the most recent report.
    const PhysicsReport& lastReport() const { return m_lastReport; }

    // Set the curvature renderer (non-owning) for grid-state cross-checks.
    void setCurvatureRenderer(CurvatureRenderer* renderer) { m_renderer = renderer; }

    // Set the expected black hole mass (for Kretschmann scaling check)
    void setMass(double massKg) { m_massKg = massKg; }

    // Set the Schwarzschild radius (for asymptotic flatness check)
    void setSchwarzschildRadius(double rs) { m_rs = rs; }

private:
    std::shared_ptr<MetricTensor> m_metric;
    double m_massKg = 1.0;
    double m_rs = 2.95325008e3; // ~2.95 km for 1 solar mass (geometric units)
    PhysicsReport m_lastReport;

    // Non-owning pointer to the curvature renderer for grid-state validation.
    CurvatureRenderer* m_renderer = nullptr;

    // Helper: determinant of a 4x4 matrix (Laplace expansion along first row).
    static double determinant4x4(const std::array<std::array<double, 4>, 4>& m) {
        double det = 0.0;
        for (int j = 0; j < 4; ++j) {
            std::array<std::array<double, 3>, 3> sub{};
            int si = 0;
            for (int i = 1; i < 4; ++i) {
                int sj = 0;
                for (int jj = 0; jj < 4; ++jj) {
                    if (jj == j) continue;
                    sub[si][sj] = m[i][jj];
                    sj++;
                }
                si++;
            }
            double subDet = sub[0][0] * (sub[1][1] * sub[2][2] - sub[1][2] * sub[2][1])
                          - sub[0][1] * (sub[1][0] * sub[2][2] - sub[1][2] * sub[2][0])
                          + sub[0][2] * (sub[1][0] * sub[2][1] - sub[1][1] * sub[2][0]);
            double cofactor = ((0 + j) % 2 == 0 ? 1.0 : -1.0) * subDet;
            det += m[0][j] * cofactor;
        }
        return det;
    }

    // Helper: check eigenvalue signature of a symmetric matrix
    bool checkLorentzianSignature(const std::array<std::array<double, 4>, 4>& g,
                                   PhysicsCheck& out) const {
        auto eigs = jacobiEigenvalues(g);

        int negCount = 0, posCount = 0, zeroCount = 0;
        for (double e : eigs) {
            if (e < -1e-10) negCount++;
            else if (e > 1e-10) posCount++;
            else zeroCount++;
        }

        out.value = static_cast<double>(negCount);
        out.expected = 1.0;  // exactly one negative eigenvalue
        out.tolerance = 0.0;
        out.passed = (negCount == 1 && posCount == 3 && zeroCount == 0);
        out.message = out.passed
                          ? "signature (-,+,+,+)"
                          : "signature violation: " +
                                std::to_string(negCount) + " neg, " +
                                std::to_string(posCount) + " pos, " +
                                std::to_string(zeroCount) + " zero";
        return out.passed;
    }

    // Helper: compute eigenvalues of a 4x4 symmetric matrix via Jacobi rotation
    static inline std::array<double, 4> jacobiEigenvalues(
        std::array<std::array<double, 4>, 4> a) {
        std::array<double, 4> d = {a[0][0], a[1][1], a[2][2], a[3][3]};
        std::array<std::array<double, 4>, 4> v = {{
            {{1.0, 0.0, 0.0, 0.0}},
            {{0.0, 1.0, 0.0, 0.0}},
            {{0.0, 0.0, 1.0, 0.0}},
            {{0.0, 0.0, 0.0, 1.0}},
        }};

        const int maxRot = 100;
        for (int rot = 0; rot < maxRot; ++rot) {
            // Find largest off-diagonal element
            int p = 0, q = 1;
            double maxOff = 0.0;
            for (int i = 0; i < 4; ++i) {
                for (int j = i + 1; j < 4; ++j) {
                    double aij = std::fabs(a[i][j]);
                    if (aij > maxOff) {
                        maxOff = aij;
                        p = i;
                        q = j;
                    }
                }
            }

            if (maxOff < 1e-14) break;  // Converged

            // Jacobi rotation angle
            double app = a[p][p], aqq = a[q][q], apq = a[p][q];
            double theta = 0.5 * std::atan2(2.0 * apq, aqq - app);
            double c = std::cos(theta);
            double s = std::sin(theta);

            // Apply rotation to rows/cols p and q
            for (int k = 0; k < 4; ++k) {
                double akp = a[k][p], akq = a[k][q];
                a[k][p] = c * akp - s * akq;
                a[k][q] = s * akp + c * akq;
            }
            for (int k = 0; k < 4; ++k) {
                double apk = a[p][k], aqk = a[q][k];
                a[p][k] = c * apk - s * aqk;
                a[q][k] = s * apk + c * aqk;
            }
            a[p][q] = 0.0;
            a[q][p] = 0.0;

            // Update diagonal
            double appNew = a[p][p];
            double aqqNew = a[q][q];
            d[p] = appNew;
            d[q] = aqqNew;

            // Update eigenvectors
            for (int k = 0; k < 4; ++k) {
                double vkp = v[k][p], vkq = v[k][q];
                v[k][p] = c * vkp - s * vkq;
                v[k][q] = s * vkp + c * vkq;
            }
        }

        return d;
    }

    // Helper: check Kretschmann scaling K ~ 1/r^6
    bool checkKretschmannScaling(double r, double kretch, PhysicsCheck& out) const;

    // Helper: check asymptotic flatness at large r
    bool checkAsymptoticFlatness(double r, const std::array<std::array<double, 4>, 4>& g,
                                 PhysicsCheck& out) const;

    // Helper: check renderer grid state matches the metric mass
    bool validateRenderer(PhysicsCheck& out) const;
};

} // namespace quantumverse
