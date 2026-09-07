/**
 * @file MergingBinaryInspiralAnalyzer.cpp
 * @brief Implementation of the merging binary inspiral chirp analyzer
 */

#include "MergingBinaryInspiralAnalyzer.h"
#include <cmath>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

MergingBinaryInspiralAnalyzer::MergingBinaryInspiralAnalyzer()
{
    setParameter("snr_threshold", 5.0);
    setParameter("confidence_threshold", 0.7);
    setParameter("min_samples", static_cast<double>(kMinSamples));
}

std::map<std::string, std::pair<double, double>>
MergingBinaryInspiralAnalyzer::getParameterRanges() const
{
    return {
        {"snr_threshold", {3.0, 10.0}},
        {"confidence_threshold", {0.5, 0.99}},
        {"min_samples", {16.0, 256.0}}
    };
}

double MergingBinaryInspiralAnalyzer::taylorF2Frequency(double chirpMass, double tau)
{
    if (tau <= 0.0 || chirpMass <= 0.0) return 0.0;
    double mc58 = std::pow(chirpMass, -5.0 / 8.0);
    double tau38 = std::pow(tau, -3.0 / 8.0);
    return (1.0 / (8.0 * M_PI)) * mc58 * tau38;
}

std::vector<std::pair<double, double>>
MergingBinaryInspiralAnalyzer::trackInstantaneousFrequency(
    const std::vector<Event4D>& trajectory, size_t windowSize)
{
    std::vector<std::pair<double, double>> track;
    size_t n = trajectory.size();
    if (n < 4) return track;

    double dt = trajectory[1].t - trajectory[0].t;
    if (dt <= 0.0) return track;

    // Interpolated zero-crossing detection: find the exact (sub-sample) time
    // at which the signal crosses zero, using linear interpolation between
    // adjacent samples. This provides sub-sample precision that eliminates
    // the quantization error of integer crossing counts, which is critical
    // for low-frequency chirp signals where only a few crossings fall inside
    // a window.
    std::vector<double> crossingTimes;
    crossingTimes.reserve(n / 2);

    for (size_t i = 1; i < n; ++i) {
        double hPrev = trajectory[i - 1].x;
        double hCurr = trajectory[i].x;
        if (!std::isfinite(hPrev) || !std::isfinite(hCurr)) continue;
        if ((hPrev >= 0.0 && hCurr < 0.0) || (hPrev < 0.0 && hCurr >= 0.0)) {
            double frac = hPrev / (hPrev - hCurr);
            double tCross = trajectory[i - 1].t + frac * dt;
            crossingTimes.push_back(tCross);
        }
    }

    if (crossingTimes.size() < 2) {
        (void)windowSize;
        return track;
    }

    // Estimate instantaneous frequency from consecutive zero-crossing intervals.
    // Each pair of consecutive crossings spans half a period, so
    // f = 1 / (2 * (tCross[k+1] - tCross[k])).
    // Filter out implausibly short intervals (noise-induced sub-sample bounces)
    // using a floor of 2*dt; legitimate signal periods are always much larger.
    double minInterval = 2.0 * dt;
    for (size_t i = 1; i < crossingTimes.size(); ++i) {
        double period = crossingTimes[i] - crossingTimes[i - 1];
        if (period > minInterval && std::isfinite(period)) {
            double instFreq = 1.0 / (2.0 * period);
            if (std::isfinite(instFreq) && instFreq > 0.0) {
                double tMid = 0.5 * (crossingTimes[i] + crossingTimes[i - 1]);
                track.emplace_back(tMid, instFreq);
            }
        }
    }

    return track;
}

MergingBinaryInspiralAnalyzer::ChirpFit
MergingBinaryInspiralAnalyzer::fitTaylorF2(
    const std::vector<std::pair<double, double>>& freqTrack)
{
    ChirpFit result;
    if (freqTrack.size() < 4) return result;

    double tLast = freqTrack.back().first;
    double fLast = freqTrack.back().second;
    if (fLast <= 0.0) return result;

    double bestMc = 0.0;
    double bestTc = 0.0;
    double bestErr = 1e30;

    // Stage 1: Coarse grid search
    for (double mc = 0.01; mc <= 100.0; mc += 0.1) {
        for (double tcOff = 0.0; tcOff <= 10.0; tcOff += 0.1) {
            double tc = tLast + tcOff;
            double err = 0.0;
            size_t count = 0;
            for (const auto& p : freqTrack) {
                double tau = tc - p.first;
                if (tau <= 0.0) continue;
                double fPred = taylorF2Frequency(mc, tau);
                double diff = (p.second - fPred) / p.second;
                err += diff * diff;
                count++;
            }
            if (count > 0) {
                err /= static_cast<double>(count);
                if (err < bestErr) {
                    bestErr = err;
                    bestMc = mc;
                    bestTc = tc;
                }
            }
        }
    }

    // Stage 2: Outlier rejection + fine grid refinement.
    // With small chirp masses the coarse grid step of 0.1 jumps by ~10x
    // (e.g. 0.01 -> 0.11), and only ~10 frequency points are available,
    // so 1-2 noise outliers can dominate the MSE and cap the SNR below
    // the detection threshold.

    // 2a. Outlier rejection: compute residuals with the coarse best fit,
    //     then remove frequency points whose relative error exceeds 3x
    //     the median residual.
    std::vector<double> residuals;
    for (const auto& p : freqTrack) {
        double tau = bestTc - p.first;
        if (tau <= 0.0) continue;
        double fPred = taylorF2Frequency(bestMc, tau);
        if (fPred <= 0.0 || p.second <= 0.0) continue;
        residuals.push_back(std::abs((p.second - fPred) / p.second));
    }
    std::vector<std::pair<double, double>> cleanTrack = freqTrack;
    if (residuals.size() >= 3) {
        std::vector<double> sorted = residuals;
        std::sort(sorted.begin(), sorted.end());
        double medianRes = sorted[sorted.size() / 2];
        double outlierThreshold = 3.0 * medianRes;
        if (outlierThreshold > 0.0) {
            cleanTrack.clear();
            for (const auto& p : freqTrack) {
                double tau = bestTc - p.first;
                if (tau <= 0.0) continue;
                double fPred = taylorF2Frequency(bestMc, tau);
                if (fPred <= 0.0 || p.second <= 0.0) continue;
                double relErr = std::abs((p.second - fPred) / p.second);
                if (relErr <= outlierThreshold) {
                    cleanTrack.push_back(p);
                }
            }
        }
    }
    const auto& fitTrack = (cleanTrack.size() >= 4) ? cleanTrack : freqTrack;

    // 2b. Fine grid search centered on the coarse best, using a step that
    //     always includes bestMc (integer multiple of fineStep from bestMc).
    const double fineStep = 0.005;
    const int fineSteps = 10;  // ±10 * 0.005 = ±0.05 range
    const double coarseTcOff = bestTc - tLast;
    for (int iMc = -fineSteps; iMc <= fineSteps; ++iMc) {
        double mc = bestMc + iMc * fineStep;
        if (mc < 0.001) continue;
        for (int iTc = -fineSteps; iTc <= fineSteps; ++iTc) {
            double tcOff = coarseTcOff + iTc * fineStep;
            if (tcOff < 0.0) continue;
            double tc = tLast + tcOff;
            double err = 0.0;
            size_t count = 0;
            for (const auto& p : fitTrack) {
                double tau = tc - p.first;
                if (tau <= 0.0) continue;
                double fPred = taylorF2Frequency(mc, tau);
                if (fPred <= 0.0) continue;
                double diff = (p.second - fPred) / p.second;
                err += diff * diff;
                count++;
            }
            if (count > 0) {
                err /= static_cast<double>(count);
                if (err < bestErr) {
                    bestErr = err;
                    bestMc = mc;
                    bestTc = tc;
                }
            }
        }
    }

    result.chirpMass = bestMc;
    result.coalescenceTime = bestTc;

    double fitStd = std::sqrt(bestErr);
    result.snr = (fitStd > 0.0) ? (1.0 / fitStd) : 0.0;
    result.confidence = std::max(0.0, 1.0 - fitStd);

    double totalMass = std::pow(bestMc, 5.0 / 3.0);
    double qEst = 1.0;
    double bestQErr = 1e30;
    for (double q = 0.1; q <= 1.0; q += 0.01) {
        double m1 = totalMass * std::pow(q, 1.0 / 3.0) / (1.0 + q);
        double m2 = totalMass / (1.0 + q);
        double mcCheck = std::pow(m1 * m2, 3.0 / 5.0) / std::pow(m1 + m2, 1.0 / 5.0);
        double qErr = std::abs(mcCheck - bestMc) / bestMc;
        if (qErr < bestQErr) {
            bestQErr = qErr;
            qEst = q;
        }
    }
    result.massRatio = qEst;

    return result;
}

std::vector<InstrumentFinding> MergingBinaryInspiralAnalyzer::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    (void)metric;
    std::vector<InstrumentFinding> findings;

    size_t n = trajectory.size();
    size_t minSamples = static_cast<size_t>(getParameter("min_samples"));
    if (n < minSamples) return findings;

    double dt = trajectory[1].t - trajectory[0].t;
    if (dt <= 0.0) return findings;

    for (size_t i = 0; i < n; ++i) {
        if (std::isnan(trajectory[i].x) || std::isinf(trajectory[i].x)) return findings;
    }

    size_t windowSize = n / 4;
    if (windowSize < 8) windowSize = 8;

    auto freqTrack = trackInstantaneousFrequency(trajectory, windowSize);

    if (freqTrack.size() < 4) return findings;

    ChirpFit fit = fitTaylorF2(freqTrack);
    if (fit.chirpMass <= 0.0 || fit.snr <= 0.0) return findings;

    double snrThreshold = getParameter("snr_threshold");
    double confThreshold = getParameter("confidence_threshold");

    if (fit.snr > snrThreshold && fit.confidence > confThreshold) {
        InstrumentFinding finding;
        finding.id = "MBIA_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(fit.confidence);
        finding.confidence = fit.confidence;
        finding.isAnomaly = true;
        finding.description = "Binary inspiral detected: chirp mass " +
            std::to_string(fit.chirpMass) + " M_sun, mass ratio q=" +
            std::to_string(fit.massRatio) + ", SNR=" + std::to_string(fit.snr) + ".";
        finding.location = location;
        finding.timestamp = trajectory.back().t;
        finding.parameters["chirp_mass"] = fit.chirpMass;
        finding.parameters["mass_ratio"] = fit.massRatio;
        finding.parameters["snr"] = fit.snr;
        finding.parameters["confidence"] = fit.confidence;
        finding.parameters["coalescence_time"] = fit.coalescenceTime;
        finding.parameters["freq_track_points"] = static_cast<double>(freqTrack.size());
        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

} // namespace quantumverse
