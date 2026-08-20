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
}

std::vector<InstrumentFinding> GWEchoHunter::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    (void)metric;
    std::vector<InstrumentFinding> findings;
    if (trajectory.size() < kMinTrajectorySize) return findings;

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

    // Extract ringdown envelope crests (local maxima of |h|). At a crest the
    // oscillatory factor |cos| is near unity, so crests trace the true decay
    // envelope free of the cos() bias that corrupts a point-wise log fit.
    std::vector<std::pair<double, double>> crests;
    auto ampAt = [&](size_t i) { return std::abs(samples[i].second); };
    for (size_t i = 0; i < samples.size(); ++i) {
        if (ampAt(i) <= 1e-12) continue;
        bool isMax = true;
        if (i > 0 && ampAt(i) < ampAt(i - 1)) isMax = false;
        if (i + 1 < samples.size() && ampAt(i) < ampAt(i + 1)) isMax = false;
        if (isMax) crests.emplace_back(samples[i].first, ampAt(i));
    }
    if (crests.empty()) return findings;

    // Global ringdown peak is the earliest crest.
    double tPeak = crests[0].first;
    double aRing = crests[0].second;
    if (aRing <= 0.0) return findings;

    double echoStart = tPeak + echoDelay;

    // Estimate the decay constant tau from pre-echo crests (unbiased).
    std::vector<std::pair<double, double>> fitCrests;
    for (const auto& c : crests) {
        if (c.first < echoStart) fitCrests.push_back(c);
    }
    if (fitCrests.size() < 2) fitCrests = crests; // fallback

    double tau = kMaxTau;
    if (fitCrests.size() >= 2) {
        double sumT = 0.0, sumL = 0.0, sumTT = 0.0, sumTL = 0.0;
        size_t cnt = 0;
        for (const auto& c : fitCrests) {
            double t = c.first - tPeak;
            double l = std::log(c.second / aRing);
            sumT += t; sumL += l; sumTT += t * t; sumTL += t * l;
            ++cnt;
        }
        double denom = cnt * sumTT - sumT * sumT;
        if (std::abs(denom) > 1e-12) {
            double slope = (cnt * sumTL - sumT * sumL) / denom; // = -1/tau
            if (slope < 0.0) tau = -1.0 / slope;
        }
    }
    if (!(tau > 0.0) || !std::isfinite(tau) || tau > kMaxTau) tau = kMaxTau;

    // Scan echo-window crests for a localized strain excess above the
    // extrapolated ringdown tail.
    bool found = false;
    double bestRatio = 0.0;
    double tEcho = tPeak;
    double aEcho = 0.0;
    for (const auto& c : crests) {
        if (c.first < echoStart) continue;
        double expected = aRing * std::exp(-(c.first - tPeak) / tau);
        double ratio = c.second / (expected + 1e-12);
        if (ratio > bestRatio) {
            bestRatio = ratio;
            tEcho = c.first;
            aEcho = c.second;
            found = true;
        }
    }
    if (!found) return findings;

    if (bestRatio > ratioThreshold) {
        double excess = bestRatio - ratioThreshold;
        double confidence = std::min(1.0, excess / (2.0 * ratioThreshold));

        InstrumentFinding f;
        f.id = "GWECHO_" + std::to_string(getTotalFindings());
        f.instrumentName = getName();
        f.severity = confidenceToSeverity(confidence);
        f.confidence = confidence;
        f.isAnomaly = true;
        f.description = "Post-ringdown gravitational-wave echo detected at t=" +
            std::to_string(tEcho) + " with strain excess ratio " + std::to_string(bestRatio) +
            "x over the extrapolated ringdown tail. Consistent with quantum "
            "structure near the horizon (echoes) or an exotic compact object.";
        f.location = location;
        f.timestamp = tEcho;
        f.parameters["echo_time"] = tEcho;
        f.parameters["echo_delay"] = echoDelay;
        f.parameters["echo_excess_ratio"] = bestRatio;
        f.parameters["ringdown_amplitude"] = aRing;
        f.parameters["echo_amplitude"] = aEcho;
        f.parameters["expected_tail_amplitude"] = aRing * std::exp(-(tEcho - tPeak) / tau);
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
        {"echo_ratio_threshold", {1.2, 3.0}}
    };
}

} // namespace quantumverse
