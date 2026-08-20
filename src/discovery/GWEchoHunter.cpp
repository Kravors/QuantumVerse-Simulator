/**
 * @file GWEchoHunter.cpp
 * @brief Implementation of the post-ringdown gravitational-wave echo hunter
 */

#include "GWEchoHunter.h"
#include <cmath>
#include <algorithm>
#include <utility>

namespace quantumverse {

GWEchoHunter::GWEchoHunter()
{
    setParameter("echo_delay", 0.5);
    setParameter("echo_ratio_threshold", 1.5);
    setParameter("ringdown_fraction", 0.25);
}

std::vector<InstrumentFinding> GWEchoHunter::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    (void)metric;
    std::vector<InstrumentFinding> findings;
    if (trajectory.size() < kMinTrajectorySize) return findings;

    double ringFrac = getParameter("ringdown_fraction");
    if (ringFrac <= 0.0 || ringFrac >= 1.0) ringFrac = 0.25;
    double echoDelay = getParameter("echo_delay");
    double ratioThreshold = getParameter("echo_ratio_threshold");

    // Validate samples, skipping NaN/Inf in the time or strain channels.
    std::vector<std::pair<double, double>> samples;
    samples.reserve(trajectory.size());
    for (const auto& pt : trajectory) {
        if (std::isnan(pt.t) || std::isnan(pt.x) || std::isinf(pt.t) || std::isinf(pt.x)) {
            continue;
        }
        samples.emplace_back(pt.t, pt.x);
    }
    if (samples.size() < kMinTrajectorySize) return findings;

    std::sort(samples.begin(), samples.end());

    // Locate the ringdown peak within the early window.
    size_t earlyN = static_cast<size_t>(ringFrac * samples.size());
    if (earlyN < 1) earlyN = 1;
    if (earlyN > samples.size()) earlyN = samples.size();

    double tPeak = 0.0;
    double aRing = 0.0;
    for (size_t i = 0; i < earlyN; ++i) {
        double a = std::abs(samples[i].second);
        if (a > aRing) {
            aRing = a;
            tPeak = samples[i].first;
        }
    }
    if (aRing <= 0.0) return findings;

    // Estimate the ringdown decay constant tau from a normalized log-envelope
    // linear fit over the early window: ln(a/aRing) = -(t - tPeak)/tau.
    double sumT = 0.0, sumL = 0.0, sumTT = 0.0, sumTL = 0.0;
    size_t cnt = 0;
    for (size_t i = 0; i < earlyN; ++i) {
        double t = samples[i].first - tPeak;
        double a = std::abs(samples[i].second);
        if (a <= 0.0) continue;
        double l = std::log(a / aRing);
        sumT += t; sumL += l; sumTT += t * t; sumTL += t * l;
        ++cnt;
    }

    double tau = kMaxTau;
    if (cnt >= 2) {
        double denom = cnt * sumTT - sumT * sumT;
        if (std::abs(denom) > 1e-12) {
            double slope = (cnt * sumTL - sumT * sumL) / denom; // = -1/tau
            if (slope < 0.0) tau = -1.0 / slope;
        }
    }
    if (!(tau > 0.0) || !std::isfinite(tau) || tau > kMaxTau) tau = kMaxTau;

    // Scan the delayed echo window for a localized strain excess.
    double echoStart = tPeak + echoDelay;
    double aEcho = 0.0;
    double tEcho = echoStart;
    bool found = false;
    for (const auto& s : samples) {
        if (s.first < echoStart) continue;
        double a = std::abs(s.second);
        if (a > aEcho) {
            aEcho = a;
            tEcho = s.first;
            found = true;
        }
    }
    if (!found) return findings;

    double expected = aRing * std::exp(-(tEcho - tPeak) / tau);
    double ratio = aEcho / (expected + 1e-12);

    if (ratio > ratioThreshold) {
        double excess = ratio - ratioThreshold;
        double confidence = std::min(1.0, excess / (2.0 * ratioThreshold));

        InstrumentFinding f;
        f.id = "GWECHO_" + std::to_string(getTotalFindings());
        f.instrumentName = getName();
        f.severity = confidenceToSeverity(confidence);
        f.confidence = confidence;
        f.isAnomaly = true;
        f.description = "Post-ringdown gravitational-wave echo detected at t=" +
            std::to_string(tEcho) + " with strain excess ratio " + std::to_string(ratio) +
            "x over the extrapolated ringdown tail. Consistent with quantum "
            "structure near the horizon (echoes) or an exotic compact object.";
        f.location = location;
        f.timestamp = tEcho;
        f.parameters["echo_time"] = tEcho;
        f.parameters["echo_delay"] = echoDelay;
        f.parameters["echo_excess_ratio"] = ratio;
        f.parameters["ringdown_amplitude"] = aRing;
        f.parameters["expected_tail_amplitude"] = expected;
        f.parameters["ratio_threshold"] = ratioThreshold;
        f.parameters["decay_tau"] = tau;
        addFinding(f);
        findings.push_back(f);
    }

    return findings;
}

std::map<std::string, std::pair<double, double>> GWEchoHunter::getParameterRanges() const
{
    return {
        {"echo_delay", {0.1, 2.0}},
        {"echo_ratio_threshold", {1.2, 3.0}},
        {"ringdown_fraction", {0.1, 0.5}}
    };
}

} // namespace quantumverse
