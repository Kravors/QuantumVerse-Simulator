/**
 * @file KerrNoHairViolationAnalyzer.cpp
 * @brief Implementation of the Kerr no-hair theorem violation analyzer
 */

#include "KerrNoHairViolationAnalyzer.h"
#include <cmath>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

KerrNoHairViolationAnalyzer::KerrNoHairViolationAnalyzer()
{
    setParameter("sigma_threshold", 2.0);
    setParameter("mass_solar", 10.0);
    setParameter("spin", 0.0);
    setParameter("min_samples", static_cast<double>(kMinSamples));
}

std::map<std::string, std::pair<double, double>>
KerrNoHairViolationAnalyzer::getParameterRanges() const
{
    return {
        {"sigma_threshold", {1.0, 5.0}},
        {"mass_solar", {1.0, 100.0}},
        {"spin", {0.0, 0.99}},
        {"min_samples", {8.0, 256.0}}
    };
}

double KerrNoHairViolationAnalyzer::kerrQNM220Real(double spin)
{
    double a = spin;
    double a2 = a * a;
    return 1.0 - 0.63 * (1.0 - a) + 0.3 * a2;
}

double KerrNoHairViolationAnalyzer::kerrQNM220Imag(double spin)
{
    double a = spin;
    return (1.0 - 0.63 * (1.0 - a - 0.6 * a * a));
}

double KerrNoHairViolationAnalyzer::kerrQNM221Real(double spin)
{
    double a = spin;
    double a2 = a * a;
    return 1.0 - 0.63 * (1.0 - a) + 0.2 * a2 - 0.5 * a2 * a;
}

double KerrNoHairViolationAnalyzer::kerrQNM221Imag(double spin)
{
    double a = spin;
    return (1.0 - 0.63 * (1.0 - a - 0.4 * a * a));
}

KerrNoHairViolationAnalyzer::QNMResult
KerrNoHairViolationAnalyzer::extractDominantMode(const std::vector<Event4D>& trajectory,
                                                  double dt)
{
    QNMResult result;
    size_t n = trajectory.size();
    if (n < 2 || dt <= 0.0) return result;

    std::vector<double> h(n);
    for (size_t i = 0; i < n; ++i) h[i] = trajectory[i].x;

    double bestMag = 0.0;
    size_t bestK = 1;
    size_t maxK = n / 2;

    for (size_t k = 1; k <= maxK; ++k) {
        double re = 0.0, im = 0.0;
        for (size_t i = 0; i < n; ++i) {
            double phase = 2.0 * M_PI * static_cast<double>(k) * static_cast<double>(i) /
                           static_cast<double>(n);
            re += h[i] * std::cos(phase);
            im -= h[i] * std::sin(phase);
        }
        double mag = std::sqrt(re * re + im * im);
        if (mag > bestMag) {
            bestMag = mag;
            bestK = k;
        }
    }

    double df = 1.0 / (static_cast<double>(n) * dt);
    result.freqReal = static_cast<double>(bestK) * df;
    result.amplitude = bestMag / static_cast<double>(n);

    if (bestK > 0 && bestK < maxK) {
        double magPrev = 0.0, magNext = 0.0;
        double rePrev = 0.0, imPrev = 0.0;
        double reNext = 0.0, imNext = 0.0;
        for (size_t i = 0; i < n; ++i) {
            double phasePrev = 2.0 * M_PI * static_cast<double>(bestK - 1) *
                               static_cast<double>(i) / static_cast<double>(n);
            rePrev += h[i] * std::cos(phasePrev);
            imPrev -= h[i] * std::sin(phasePrev);
            double phaseNext = 2.0 * M_PI * static_cast<double>(bestK + 1) *
                               static_cast<double>(i) / static_cast<double>(n);
            reNext += h[i] * std::cos(phaseNext);
            imNext -= h[i] * std::sin(phaseNext);
        }
        magPrev = std::sqrt(rePrev * rePrev + imPrev * imPrev);
        magNext = std::sqrt(reNext * reNext + imNext * imNext);

        double denom = magPrev - 2.0 * bestMag + magNext;
        if (std::abs(denom) > 1e-30) {
            double delta = 0.5 * (magPrev - magNext) / denom;
            delta = std::max(-0.5, std::min(0.5, delta));
            result.freqReal = (static_cast<double>(bestK) + delta) * df;
        }
    }

    std::vector<double> env;
    for (size_t i = 1; i + 1 < n; ++i) {
        if (std::abs(h[i]) >= std::abs(h[i - 1]) && std::abs(h[i]) >= std::abs(h[i + 1])) {
            env.push_back(std::abs(h[i]));
        }
    }
    if (env.size() >= 2) {
        double logRatio = std::log(env.back() / env.front()) /
                          (trajectory.back().t - trajectory.front().t);
        result.freqImag = -logRatio / (2.0 * M_PI);
    }

    return result;
}

double KerrNoHairViolationAnalyzer::inferSpinFrom220(double mOmegaReal)
{
    double bestSpin = 0.0;
    double bestErr = 1e30;
    for (double a = 0.0; a <= 0.99; a += 0.01) {
        double predicted = kerrQNM220Real(a);
        double err = std::abs(predicted - mOmegaReal);
        if (err < bestErr) {
            bestErr = err;
            bestSpin = a;
        }
    }
    return bestSpin;
}

double KerrNoHairViolationAnalyzer::inferMassFromQNM(double mOmegaReal, double spin,
                                                     double (*qnmRealFunc)(double))
{
    double mOmegaPred = qnmRealFunc(spin);
    if (mOmegaPred <= 0.0) return 0.0;
    return mOmegaPred / (mOmegaReal > 0.0 ? mOmegaReal : 1e-30);
}

std::vector<InstrumentFinding> KerrNoHairViolationAnalyzer::analyze(
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

    size_t half = n / 2;
    std::vector<Event4D> firstHalf(trajectory.begin(), trajectory.begin() + static_cast<ptrdiff_t>(half));
    std::vector<Event4D> secondHalf(trajectory.begin() + static_cast<ptrdiff_t>(half), trajectory.end());

    QNMResult mode220 = extractDominantMode(firstHalf, dt);
    QNMResult mode221 = extractDominantMode(secondHalf, dt);

    std::cerr << "KERR: mode220.freqReal=" << mode220.freqReal
              << " mode220.amplitude=" << mode220.amplitude
              << " mode221.freqReal=" << mode221.freqReal
              << " mode221.amplitude=" << mode221.amplitude << std::endl;

    if (mode220.amplitude <= 0.0 || mode221.amplitude <= 0.0) return findings;
    if (mode220.freqReal <= 0.0 || mode221.freqReal <= 0.0) return findings;

    double spin220 = inferSpinFrom220(mode220.freqReal);
    double spin221 = inferSpinFrom220(mode221.freqReal);

    double spinDiff = std::abs(spin220 - spin221);
    double spinUncertainty = 0.05;
    double deviationSigma = spinDiff / (2.0 * spinUncertainty);

    double threshold = getParameter("sigma_threshold");

    if (deviationSigma > threshold) {
        double confidence = std::min(1.0, deviationSigma / (2.0 * threshold));

        InstrumentFinding finding;
        finding.id = "KNH_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(confidence);
        finding.confidence = confidence;
        finding.isAnomaly = true;
        finding.description = "Kerr no-hair violation: QNM modes imply inconsistent "
            "spin values (a*_220=" + std::to_string(spin220) +
            ", a*_221=" + std::to_string(spin221) +
            "), deviation=" + std::to_string(deviationSigma) + " sigma.";
        finding.location = location;
        finding.timestamp = trajectory.back().t;
        finding.parameters["spin_220"] = spin220;
        finding.parameters["spin_221"] = spin221;
        finding.parameters["spin_diff"] = spinDiff;
        finding.parameters["deviation_sigma"] = deviationSigma;
        finding.parameters["threshold_sigma"] = threshold;
        finding.parameters["freq_220"] = mode220.freqReal;
        finding.parameters["freq_221"] = mode221.freqReal;
        finding.parameters["amplitude_220"] = mode220.amplitude;
        finding.parameters["amplitude_221"] = mode221.amplitude;
        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

} // namespace quantumverse
