// QuantumVerse Test: PBHMicrolensingScanner
// TDD validation of primordial black-hole microlensing detection.
// Verifies a flat light curve (no event), an injected Paczynski magnification
// bump (flagged), a symmetric transit dip (not a brightening lens -> not
// flagged), empty/small input, NaN/Inf robustness, and parameter ranges.

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <limits>
#include <algorithm>

#include "discovery/PBHMicrolensingScanner.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

namespace {
constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kInf = std::numeric_limits<double>::infinity();

double paczynski(double u) {
    if (u <= 0.0) return 1.0;
    double u2 = u * u;
    return (u2 + 2.0) / (u * std::sqrt(u2 + 4.0));
}

std::vector<Event4D> makeFlat() {
    std::vector<Event4D> d;
    d.reserve(60);
    for (int i = 0; i < 60; ++i) {
        double t = i * 0.05;
        double f = 1.0 + 0.01 * std::sin(7.0 * t);
        d.emplace_back(t, f, 0.0, 0.0);
    }
    return d;
}

std::vector<Event4D> makeMicrolensing() {
    std::vector<Event4D> d;
    d.reserve(60);
    double F0 = 1.0, u0 = 0.3, tE = 0.5, t0 = 1.5;
    for (int i = 0; i < 60; ++i) {
        double t = i * 0.05;
        double u = std::sqrt(u0 * u0 + ((t - t0) / tE) * ((t - t0) / tE));
        double f = F0 * paczynski(u);
        d.emplace_back(t, f, 0.0, 0.0);
    }
    return d;
}

// Symmetric transit-like DIP (darkening, not a brightening lens).
std::vector<Event4D> makeDip() {
    std::vector<Event4D> d;
    d.reserve(60);
    double F0 = 1.0, depth = 0.5, t0 = 1.5, w = 0.15;
    for (int i = 0; i < 60; ++i) {
        double t = i * 0.05;
        double g = std::exp(-((t - t0) * (t - t0)) / (2.0 * w * w));
        double f = F0 * (1.0 - depth * g);
        d.emplace_back(t, f, 0.0, 0.0);
    }
    return d;
}
} // namespace

int main() {
    std::cout << "=== PBHMicrolensingScanner Test ===" << std::endl;

    MetricTensor metric;
    metric.g[0][0] = -1.0; metric.g[1][1] = 1.0;
    metric.g[2][2] = 1.0; metric.g[3][3] = 1.0;
    Event4D location(0.0, 0.0, 0.0, 0.0);

    // --- 1. Flat light curve: no event expected -------------------------
    {
        PBHMicrolensingScanner scanner;
        auto findings = scanner.analyze(metric, location, makeFlat());
        assert(findings.empty() && "Flat light curve must not trigger microlensing");
        std::cout << "  Flat light curve: no event (findings=" << findings.size() << ")" << std::endl;
    }

    // --- 2. Paczynski magnification bump: should be flagged --------------
    {
        PBHMicrolensingScanner scanner;
        auto findings = scanner.analyze(metric, location, makeMicrolensing());
        assert(!findings.empty() && "Microlensing event should produce a finding");
        assert(findings.size() == 1u);
        assert(findings[0].isAnomaly);
        assert(findings[0].confidence > 0.0 && findings[0].confidence <= 1.0);
        std::cout << "  Microlensing event: confidence=" << findings[0].confidence
                  << " severity=" << static_cast<int>(findings[0].severity)
                  << " A0=" << findings[0].parameters["peak_magnification"] << std::endl;

        // PBH mass/velocity estimate should be populated and positive.
        assert(findings[0].parameters.count("pbh_mass_solar") > 0);
        assert(findings[0].parameters["pbh_mass_solar"] > 0.0);
        assert(findings[0].parameters["einstein_radius_au"] > 0.0);
        assert(findings[0].parameters["pbh_velocity_kms"] > 0.0);
        std::cout << "  PBH estimate: M=" << findings[0].parameters["pbh_mass_solar"]
                  << " M_sun, R_E=" << findings[0].parameters["einstein_radius_au"]
                  << " AU" << std::endl;
    }

    // --- 3. Symmetric transit dip: darkening, not a brightening lens -----
    {
        PBHMicrolensingScanner scanner;
        auto findings = scanner.analyze(metric, location, makeDip());
        assert(findings.empty() && "Transit dip must not be flagged as microlensing");
        std::cout << "  Transit dip: correctly not flagged (findings=" << findings.size() << ")" << std::endl;
    }

    // --- 4. Empty trajectory: must not crash, returns empty -------------
    {
        PBHMicrolensingScanner scanner;
        std::vector<Event4D> empty;
        auto findings = scanner.analyze(metric, location, empty);
        assert(findings.empty());
        std::cout << "  Empty trajectory handled safely." << std::endl;
    }

    // --- 5. Too-few samples: must not crash, returns empty --------------
    {
        PBHMicrolensingScanner scanner;
        std::vector<Event4D> small;
        for (int i = 0; i < 5; ++i) small.emplace_back(i * 0.05, 1.0, 0.0, 0.0);
        auto findings = scanner.analyze(metric, location, small);
        assert(findings.empty());
        std::cout << "  Small trajectory handled safely." << std::endl;
    }

    // --- 6. NaN/Inf observations: must not crash ------------------------
    {
        PBHMicrolensingScanner scanner;
        auto traj = makeFlat();
        traj[10] = Event4D(kNaN, traj[10].x, 0.0, 0.0);
        auto f1 = scanner.analyze(metric, location, traj);
        assert(f1.empty());
        std::cout << "  NaN observation handled safely." << std::endl;

        auto traj2 = makeFlat();
        traj2[20] = Event4D(traj2[20].t, kInf, 0.0, 0.0);
        auto f2 = scanner.analyze(metric, location, traj2);
        assert(f2.empty());
        std::cout << "  Inf observation handled safely." << std::endl;
    }

    // --- 7. Parameter ranges are sensible -------------------------------
    {
        PBHMicrolensingScanner scanner;
        auto ranges = scanner.getParameterRanges();
        assert(ranges.count("min_magnification") > 0);
        assert(ranges.count("fit_ratio_threshold") > 0);
        assert(ranges.count("lens_distance_kpc") > 0);
        assert(ranges.count("source_distance_kpc") > 0);
        assert(ranges.count("lens_velocity_kms") > 0);
        assert(ranges.count("seconds_per_time_unit") > 0);
        assert(ranges["min_magnification"].first < ranges["min_magnification"].second);
        assert(ranges["fit_ratio_threshold"].first < ranges["fit_ratio_threshold"].second);
        std::cout << "  Parameter ranges are valid." << std::endl;
    }

    // --- 8. PBH mass/velocity mapping inverts the microlensing relations --
    {
        // Forward model: given M, D_l, D_s, v_t, compute the implied tE (seconds),
        // then verify estimatePBH recovers M (exact inverse up to round-off).
        const double G = 6.67430e-11, c = 299792458.0, Msun = 1.98847e30, kpc = 3.085677581e19;
        auto tEfromMass = [&](double M_solar, double Dl, double Ds, double v_kms) {
            double M = M_solar * Msun;
            double RE = std::sqrt((4.0 * G * M / (c * c)) *
                (Dl * kpc) * ((Ds - Dl) * kpc) / (Ds * kpc));
            return RE / (v_kms * 1000.0); // seconds
        };

        double tE = tEfromMass(1.0, 4.0, 8.0, 200.0);
        auto est = PBHMicrolensingScanner::estimatePBH(tE, 0.3, 4.0, 8.0, 200.0, 0.9);
        assert(std::abs(est.mass_solar - 1.0) < 1e-6);
        assert(std::abs(est.velocity_kms - 200.0) < 1e-9);
        assert(est.einstein_radius_au > 0.0);
        std::cout << "  Mass mapping round-trip: M=" << est.mass_solar
                  << " M_sun (expected 1.0)" << std::endl;

        // A heavier PBH should map to a larger mass.
        double tE2 = tEfromMass(10.0, 4.0, 8.0, 200.0);
        auto est2 = PBHMicrolensingScanner::estimatePBH(tE2, 0.3, 4.0, 8.0, 200.0, 0.9);
        assert(est2.mass_solar > est.mass_solar);
        std::cout << "  Heavier PBH maps to larger mass: M=" << est2.mass_solar
                  << " M_sun" << std::endl;
    }

    // --- 9. Degenerate/ill-posed geometry returns a zero estimate -------
    {
        auto bad = PBHMicrolensingScanner::estimatePBH(0.0, 0.3, 4.0);
        assert(bad.mass_solar == 0.0);
        (void)bad;
        auto bad2 = PBHMicrolensingScanner::estimatePBH(100.0, 0.3, 8.0, 4.0); // lens behind source
        assert(bad2.mass_solar == 0.0);
        (void)bad2;
        std::cout << "  Degenerate geometry handled safely (M=0)." << std::endl;
    }

    std::cout << "All PBHMicrolensingScannerTest checks passed." << std::endl;
    return 0;
}
