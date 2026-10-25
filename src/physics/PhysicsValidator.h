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

// Validates metric tensor and geodesic invariants at the current probe
// position. Call once per N frames (aligned with the [REALITY] log).
class PhysicsValidator {
public:
    explicit PhysicsValidator(std::shared_ptr<MetricTensor> metric);

    // Set the metric (called when the probe metric changes)
    void setMetric(std::shared_ptr<MetricTensor> metric) { m_metric = std::move(metric); }

    // Run a full validation sweep at the given probe position.
    // Returns a report with pass/fail status for every check.
    PhysicsReport validate(const Event4D& probePos);

    // Quick boolean: is everything currently valid?
    bool isValid() const { return m_lastReport.failedChecks == 0; }

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

    // Helper: check eigenvalue signature of a symmetric matrix
    bool checkLorentzianSignature(const std::array<std::array<double, 4>, 4>& g,
                                  PhysicsCheck& out) const;

    // Helper: check Kretschmann scaling K ~ 1/r^6
    bool checkKretschmannScaling(double r, double kretch, PhysicsCheck& out) const;

    // Helper: check asymptotic flatness at large r
    bool checkAsymptoticFlatness(double r, const std::array<std::array<double, 4>, 4>& g,
                                 PhysicsCheck& out) const;

    // Helper: check renderer grid state matches the metric mass
    bool validateRenderer(PhysicsCheck& out) const;
};

} // namespace quantumverse
