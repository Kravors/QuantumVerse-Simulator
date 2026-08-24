/**
 * @file SGWBBackgroundAnalyzer.cpp
 * @brief Implementation of the Stochastic Gravitational-Wave Background (SGWB)
 *        analyzer
 */

#include "SGWBBackgroundAnalyzer.h"

#include <cmath>
#include <algorithm>
#include <vector>

namespace quantumverse {

namespace {

constexpr double kPi = 3.14159265358979323846;

} // anonymous namespace

SGWBBackgroundAnalyzer::SGWBBackgroundAnalyzer() {
    setParameter("threshold", 3.0);       // detection significance in sigma
    setParameter("frequency_bins", 64.0); // reported bin count for the spectrum
    setParameter("window_type", 0.0);     // 0 = rectangular, 1 = Hann
}

std::vector<InstrumentFinding> SGWBBackgroundAnalyzer::analyzeStrains(
        const std::vector<double>& strainA, const std::vector<double>& strainB) {
    const size_t N = std::min(strainA.size(), strainB.size());
    if (N < kMinPoints) return {};

    // Keep only finite samples; NaN/Inf must never poison the statistic.
    std::vector<double> A, B;
    A.reserve(N);
    B.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        if (std::isfinite(strainA[i]) && std::isfinite(strainB[i])) {
            A.push_back(strainA[i]);
            B.push_back(strainB[i]);
        }
    }
    const size_t M = A.size();
    if (M < kMinPoints) return {};

    // Remove the mean so the statistic measures correlated deviations.
    double ma = 0.0, mb = 0.0;
    for (double v : A) ma += v;
    for (double v : B) mb += v;
    ma /= static_cast<double>(M);
    mb /= static_cast<double>(M);

    const bool hann = getParameter("window_type") != 0.0;
    auto window = [&](size_t i) -> double {
        if (!hann) return 1.0;
        const double x = static_cast<double>(i) / static_cast<double>(M - 1);
        return 0.5 - 0.5 * std::cos(2.0 * kPi * x);
    };

    double Pa = 0.0, Pb = 0.0, C = 0.0;
    for (size_t i = 0; i < M; ++i) {
        const double w = window(i);
        const double xa = (A[i] - ma) * w;
        const double xb = (B[i] - mb) * w;
        Pa += xa * xa;
        Pb += xb * xb;
        C += xa * xb;
    }
    if (!(Pa > 0.0) || !(Pb > 0.0) || !std::isfinite(Pa) || !std::isfinite(Pb) || !std::isfinite(C)) {
        return {};
    }

    // Normalized zero-lag cross-power (in [-1, 1]); signed value is reported.
    const double r0 = C / std::sqrt(Pa * Pb);
    if (!std::isfinite(r0)) return {};

    // Detection significance uses the magnitude of the correlation: a shared
    // signal is present whether the two streams happen to be in-phase or
    // antiphase (e.g. the single-detector split-half fallback).  Under
    // independent noise the estimator has std ~ 1/sqrt(M).
    const double mag = std::fabs(r0);
    const double sigma = mag * std::sqrt(static_cast<double>(M));
    const double threshold = getParameter("threshold");
    if (sigma < threshold) return {};

    InstrumentFinding finding;
    finding.id = "SGWB_" + std::to_string(getTotalFindings());
    finding.instrumentName = getName();
    const double confidence = std::min(1.0, mag);
    finding.severity = confidenceToSeverity(confidence);
    finding.confidence = confidence;
    finding.isAnomaly = true;
    finding.description = "Stochastic gravitational-wave background detected via "
                          "two-detector cross-correlation: normalized cross-power r0=" +
                          std::to_string(r0) + " at " + std::to_string(sigma) +
                          " sigma, exceeding the " + std::to_string(threshold) +
                          "-sigma threshold. Consistent with an isotropic, unpolarized "
                          "SGWB (LIGO/Virgo/KAGRA style stochastic search).";
    finding.parameters["cross_correlation"] = r0;
    finding.parameters["significance_sigma"] = sigma;
    finding.parameters["threshold"] = threshold;
    finding.parameters["omega_gw"] = mag; // normalized GW energy-density proxy
    finding.parameters["frequency_bins"] = getParameter("frequency_bins");
    finding.parameters["window_type"] = getParameter("window_type");
    finding.parameters["n_samples"] = static_cast<double>(M);
    addFinding(finding);
    return {finding};
}

std::vector<InstrumentFinding> SGWBBackgroundAnalyzer::analyze(
        const MetricTensor& metric, const Event4D& location,
        const std::vector<Event4D>& trajectory) {
    (void)metric;
    std::vector<double> A;
    A.reserve(trajectory.size());
    for (const auto& pt : trajectory) {
        if (std::isfinite(pt.t) && std::isfinite(pt.x)) A.push_back(pt.x);
    }
    if (A.size() < kMinPoints) return {};

    if (!m_referenceStrain.empty()) {
        return analyzeStrains(A, m_referenceStrain);
    }

    // Single-detector fallback: split the trajectory into two halves and
    // cross-correlate.  Detects backgrounds whose correlation persists across
    // the split (e.g. a slowly-varying common signal).  A stationary white
    // background correctly yields ~zero correlation here and is rejected.
    const size_t half = A.size() / 2;
    if (half < kMinPoints) return {};
    std::vector<double> first(A.begin(), A.begin() + half);
    std::vector<double> second(A.begin() + half, A.end());
    (void)location;
    return analyzeStrains(first, second);
}

std::map<std::string, std::pair<double, double>> SGWBBackgroundAnalyzer::getParameterRanges() const {
    return {
        {"threshold", {1.0, 10.0}},
        {"frequency_bins", {8.0, 1024.0}},
        {"window_type", {0.0, 1.0}}
    };
}

} // namespace quantumverse
