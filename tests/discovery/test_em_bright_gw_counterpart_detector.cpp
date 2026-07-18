// Phase 6.10: EM-Bright GW Counterpart Detector Test
// Validates EMBrightGWCounterpartDetector against synthetic GW-EM
// coincidences, non-coincident pairs, and degenerate inputs.

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

#include "discovery/EMBrightGWCounterpartDetector.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

namespace {
constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kInf = std::numeric_limits<double>::infinity();

MetricTensor makeFlatMetric() {
    MetricTensor m;
    m.g[0][0] = -1.0;
    m.g[1][1] = 1.0;
    m.g[2][2] = 1.0;
    m.g[3][3] = 1.0;
    return m;
}

std::vector<Event4D> makeCoincidentTrajectory() {
    std::vector<Event4D> traj;
    traj.reserve(4);
    traj.emplace_back(0.0, 10.0, 20.0, 0.0);   // GW event at (RA=10, Dec=20), t=0
    traj.emplace_back(1.0, 10.5, 20.5, 0.0);   // EM counterpart ~1s later, nearby
    traj.emplace_back(100.0, 0.0, 0.0, 0.0);   // unrelated event far away in time
    traj.emplace_back(101.0, 0.0, 0.0, 0.0);   // another unrelated event
    return traj;
}

std::vector<Event4D> makeNonCoincidentTrajectory() {
    std::vector<Event4D> traj;
    traj.reserve(3);
    traj.emplace_back(0.0, 0.0, 0.0, 0.0);
    traj.emplace_back(1000.0, 180.0, -90.0, 0.0);
    traj.emplace_back(2000.0, 180.0, -90.0, 0.0);
    return traj;
}
}

int main() {
    std::cout << "=== EMBrightGWCounterpartDetectorTest ===" << std::endl;

    MetricTensor metric = makeFlatMetric();
    Event4D location(0.0, 0.0, 0.0, 0.0);
    std::vector<Event4D> emptyTrajectory;

    // --- 1. Empty trajectory: must not crash, returns empty ------------------------
    {
        EMBrightGWCounterpartDetector detector;
        auto findings = detector.analyze(metric, location, emptyTrajectory);
        assert(findings.empty());
        std::cout << "  Empty trajectory handled safely." << std::endl;
    }

    // --- 2. Non-coincident trajectory: no counterparts expected --------------------
    {
        EMBrightGWCounterpartDetector detector;
        auto traj = makeNonCoincidentTrajectory();
        auto findings = detector.analyze(metric, location, traj);
        assert(findings.empty() && "Non-coincident trajectory should not trigger");
        std::cout << "  Non-coincident trajectory: no findings." << std::endl;
    }

    // --- 3. Coincident GW-EM pair: should flag a counterpart -----------------------
    {
        EMBrightGWCounterpartDetector detector;
        auto traj = makeCoincidentTrajectory();
        auto findings = detector.analyze(metric, location, traj);
        assert(!findings.empty() && "Coincident GW-EM pair should produce a finding");
        assert(findings.size() == 1u);
        assert(findings[0].confidence > 0.0 && findings[0].confidence <= 1.0);
        assert(findings[0].severity >= AlertSeverity::MEDIUM);
        std::cout << "  Coincident GW-EM pair: finding confidence=" << findings[0].confidence
                  << " severity=" << static_cast<int>(findings[0].severity) << std::endl;
    }

    // --- 4. NaN/Inf coordinates: must not crash ------------------------------------
    {
        EMBrightGWCounterpartDetector detector;
        std::vector<Event4D> nanTraj;
        nanTraj.emplace_back(kNaN, 0.0, 0.0, 0.0);
        nanTraj.emplace_back(0.0, kNaN, 0.0, 0.0);
        auto findings = detector.analyze(metric, location, nanTraj);
        assert(findings.empty());
        std::cout << "  NaN coordinates handled safely." << std::endl;

        std::vector<Event4D> infTraj;
        infTraj.emplace_back(kInf, 0.0, 0.0, 0.0);
        infTraj.emplace_back(0.0, kInf, 0.0, 0.0);
        auto findings2 = detector.analyze(metric, location, infTraj);
        assert(findings2.empty());
        std::cout << "  Inf coordinates handled safely." << std::endl;
    }

    // --- 5. Parameter tuning: tighten angular threshold to suppress ----------------
    {
        EMBrightGWCounterpartDetector detector;
        detector.setParameter("angular_threshold_deg", 0.01);
        auto traj = makeCoincidentTrajectory();
        auto findings = detector.analyze(metric, location, traj);
        assert(findings.empty() && "Tight angular threshold should suppress the counterpart");
        std::cout << "  Tight angular threshold suppressed counterpart correctly." << std::endl;
    }

    // --- 6. Parameter tuning: widen time window to recover -------------------------
    {
        EMBrightGWCounterpartDetector detector;
        detector.setParameter("angular_threshold_deg", 5.0);
        detector.setParameter("time_window_sec", 2.0);
        auto traj = makeCoincidentTrajectory();
        auto findings = detector.analyze(metric, location, traj);
        assert(!findings.empty() && "Wider time window should recover the counterpart");
        std::cout << "  Wider time window recovered counterpart." << std::endl;
    }

    std::cout << "All EMBrightGWCounterpartDetectorTest checks passed." << std::endl;
    return 0;
}
