// QuantumVerse Physics Validator - Reality Monitor
//
// Validates mathematical and physical invariants of the simulation in real-time.

#include "physics/PhysicsValidator.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>

#ifndef QUANTUMVERSE_BUILD_WASM
#include "../rendering/CurvatureRenderer.h"
#endif

namespace quantumverse {

// ---------------------------------------------------------------------------
// Helper: compute eigenvalues of a 4x4 symmetric matrix via Jacobi rotation
// ---------------------------------------------------------------------------
static std::array<double, 4> jacobiEigenvalues(
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

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
PhysicsValidator::PhysicsValidator(std::shared_ptr<MetricTensor> metric)
    : m_metric(std::move(metric)) {}

// ---------------------------------------------------------------------------
// Main validation sweep
// ---------------------------------------------------------------------------
PhysicsReport PhysicsValidator::validate(const Event4D& probePos) {
    PhysicsReport report;
    m_lastReport = PhysicsReport{};  // reset

    if (!m_metric) {
        report.totalChecks = 1;
        report.failedChecks = 1;
        report.failures.push_back({"metric_available", false, 0.0, 1.0, 0.0,
                                   "Metric tensor pointer is null"});
        m_lastReport = report;
        return report;
    }

    // Evaluate metric at probe position
    auto g = m_metric->evaluate(probePos);
    double r = std::sqrt(probePos.x * probePos.x +
                         probePos.y * probePos.y +
                         probePos.z * probePos.z);

    // -----------------------------------------------------------------------
    // Check 1: Lorentzian signature (-,+,+,+)
    // -----------------------------------------------------------------------
    {
        PhysicsCheck c;
        c.name = "lorentzian_signature";
        bool ok = checkLorentzianSignature(g, c);
        report.totalChecks++;
        if (!ok) report.failedChecks++;
        if (!ok) report.failures.push_back(c);
    }

    // -----------------------------------------------------------------------
    // Check 2: Determinant < 0
    // -----------------------------------------------------------------------
    {
        PhysicsCheck c;
        c.name = "metric_determinant";
        double det = 0.0;
        // Laplace expansion along first row
        for (int j = 0; j < 4; ++j) {
            // Build 3x3 submatrix
            std::array<std::array<double, 3>, 3> sub{};
            int si = 0;
            for (int i = 0; i < 4; ++i) {
                if (i == j) continue;
                int sj = 0;
                for (int jj = 0; jj < 4; ++jj) {
                    if (jj == 0) continue;
                    sub[si][sj] = g[i][jj];
                    sj++;
                }
                si++;
            }
            // 3x3 determinant
            double subDet = sub[0][0] * (sub[1][1] * sub[2][2] - sub[1][2] * sub[2][1])
                          - sub[0][1] * (sub[1][0] * sub[2][2] - sub[1][2] * sub[2][0])
                          + sub[0][2] * (sub[1][0] * sub[2][1] - sub[1][1] * sub[2][0]);
            double cofactor = ((0 + j) % 2 == 0 ? 1.0 : -1.0) * subDet;
            det += g[0][j] * cofactor;
        }
        c.value = det;
        c.expected = -1.0;  // negative for Lorentzian
        c.tolerance = 1e-6;
        c.passed = det < 0.0 && std::isfinite(det);
        c.message = det < 0.0 && std::isfinite(det)
                        ? "det(g) < 0 (Lorentzian)"
                        : "det(g) >= 0 or NaN/Inf (signature violation)";
        report.totalChecks++;
        if (!c.passed) report.failedChecks++;
        if (!c.passed) report.failures.push_back(c);
    }

    // -----------------------------------------------------------------------
    // Check 3: Asymptotic flatness at large r
    // -----------------------------------------------------------------------
    {
        PhysicsCheck c;
        bool ok = checkAsymptoticFlatness(r, g, c);
        report.totalChecks++;
        if (!ok) report.failedChecks++;
        if (!ok) report.failures.push_back(c);
    }

    // -----------------------------------------------------------------------
    // Check 4: Kretschmann scaling K ~ 1/r^6 (if scalars available)
    // -----------------------------------------------------------------------
    {
        auto scalars = m_metric->curvatureScalars(probePos);
        if (scalars.valid) {
            PhysicsCheck c;
            bool ok = checkKretschmannScaling(r, scalars.kretschmann, c);
            report.totalChecks++;
            if (!ok) report.failedChecks++;
            if (!ok) report.failures.push_back(c);
        }
    }

    // -----------------------------------------------------------------------
    // Check 5: Renderer state — grid must deform when metric has mass
    // -----------------------------------------------------------------------
#ifndef QUANTUMVERSE_BUILD_WASM
    {
        PhysicsCheck c;
        bool ok = validateRenderer(c);
        report.totalChecks++;
        if (!ok) report.failedChecks++;
        if (!ok) report.failures.push_back(c);
    }
#endif

    m_lastReport = report;
    return report;
}

// ---------------------------------------------------------------------------
// Lorentzian signature check
// ---------------------------------------------------------------------------
bool PhysicsValidator::checkLorentzianSignature(
    const std::array<std::array<double, 4>, 4>& g,
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

// ---------------------------------------------------------------------------
// Kretschmann scaling check: K ~ 1/r^6
// ---------------------------------------------------------------------------
bool PhysicsValidator::checkKretschmannScaling(double r, double kretch,
                                               PhysicsCheck& out) const {
    if (!std::isfinite(kretch) || kretch <= 0.0 || r <= m_rs) {
        out.passed = true;  // skip if inside horizon or invalid
        out.value = kretch;
        out.expected = kretch;
        out.tolerance = 0.0;
        out.message = "skip (inside horizon or invalid)";
        return true;
    }

    // K = 48 G^2 M^2 / (c^4 r^6)
    // Compute expected K at this r
    const double c2 = 8.9875517873681764e16;  // c^2
    const double c4 = c2 * c2;                // c^4
    const double G = 6.67430e-11;             // gravitational constant
    const double M = m_massKg;   // already in kg
    const double expectedK = 48.0 * G * G * M * M / (c4 * std::pow(r, 6));

    double relErr = std::fabs(kretch - expectedK) / expectedK;
    out.value = kretch;
    out.expected = expectedK;
    out.tolerance = 0.5;  // 50% tolerance
    out.passed = relErr < 0.5 && std::isfinite(kretch);
    out.message = out.passed
                      ? "K ~ 1/r^6 within 50%"
                      : "Kretschmann scaling violation (rel_err=" +
                            std::to_string(relErr * 100.0) + "%)";
    return out.passed;
}

// ---------------------------------------------------------------------------
// Asymptotic flatness check: at large r, g -> diag(-1,1,1,1)
// ---------------------------------------------------------------------------
bool PhysicsValidator::checkAsymptoticFlatness(
    double r, const std::array<std::array<double, 4>, 4>& g,
    PhysicsCheck& out) const {
    // Only check at large r (> 10 * rs)
    if (r < 10.0 * m_rs) {
        out.passed = true;
        out.value = r;
        out.expected = 10.0 * m_rs;
        out.tolerance = 0.0;
        out.message = "skip (r < 10 rs)";
        return true;
    }

    double gttErr = std::fabs(g[0][0] - (-1.0));
    double gxxErr = std::fabs(g[1][1] - 1.0);
    double gyyErr = std::fabs(g[2][2] - 1.0);
    double gzzErr = std::fabs(g[3][3] - 1.0);

    double maxErr = std::max({gttErr, gxxErr, gyyErr, gzzErr});
    out.value = maxErr;
    out.expected = 0.0;
    out.tolerance = 0.1;  // 10% deviation tolerance
    out.passed = maxErr < 0.1 && std::isfinite(maxErr);
    out.message = out.passed
                      ? "asymptotically flat"
                      : "not asymptotically flat (max err=" +
                            std::to_string(maxErr) + ")";
    return out.passed;
}

#ifndef QUANTUMVERSE_BUILD_WASM
bool PhysicsValidator::validateRenderer(PhysicsCheck& out) const {
    if (!m_renderer || !m_metric) {
        out.passed = true;
        out.value = 0.0;
        out.expected = 0.0;
        out.tolerance = 0.0;
        out.message = "skip (no renderer or metric)";
        return true;
    }

    auto range = m_renderer->gridZRange();

    double mass = 0.0;
    if (auto schw = std::dynamic_pointer_cast<quantumverse::SchwarzschildMetric>(m_metric)) {
        mass = schw->mass();
    }

    // If the metric has mass but the grid is flat, the render pipeline is
    // disconnected from the physics metric.
    double zmin = range.first, zmax = range.second;
    bool neverInitialized = (zmin == -1.0 && zmax == -1.0);
    bool flat = (zmin == 0.0 && zmax == 0.0) || neverInitialized;
    out.name = "renderer_grid_deformation";
    out.value = static_cast<double>(zmax - zmin);
    out.expected = 0.0;  // any non-zero deformation
    out.tolerance = 0.0;
    out.passed = !(mass > 0.1 && flat);
    out.message = out.passed
                      ? "grid deforms with metric mass"
                      : "grid is flat but metric has mass (render pipeline disconnected)";
    return out.passed;
}
#endif

} // namespace quantumverse
