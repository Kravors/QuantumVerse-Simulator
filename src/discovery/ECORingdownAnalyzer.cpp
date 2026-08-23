/**
 * @file ECORingdownAnalyzer.cpp
 * @brief Implementation of the Exotic Compact Object (ECO) ringdown analyzer
 */

#include "ECORingdownAnalyzer.h"

#include <cmath>
#include <algorithm>
#include <functional>
#include <vector>

namespace quantumverse {

namespace {

constexpr double kPi = 3.14159265358979323846;


/**
 * @brief Solve a small dense linear system A x = b via Gaussian elimination
 *        with partial pivoting.  A is n x n (copied), b has length n.
 * @return true on success, false if the system is (near) singular.
 */
bool solveLinear(std::vector<std::vector<double>> A, std::vector<double> b,
                 int n, std::vector<double>& x) {
    x.assign(static_cast<size_t>(n), 0.0);
    for (int col = 0; col < n; ++col) {
        int piv = col;
        double best = std::fabs(A[col][col]);
        for (int r = col + 1; r < n; ++r) {
            double v = std::fabs(A[r][col]);
            if (v > best) { best = v; piv = r; }
        }
        if (best < 1e-12) return false;
        if (piv != col) {
            std::swap(A[col], A[piv]);
            std::swap(b[col], b[piv]);
        }
        for (int r = col + 1; r < n; ++r) {
            double f = A[r][col] / A[col][col];
            for (int c = col; c < n; ++c) A[r][c] -= f * A[col][c];
            b[r] -= f * b[col];
        }
    }
    for (int i = n - 1; i >= 0; --i) {
        double s = b[i];
        for (int c = i + 1; c < n; ++c) s -= A[i][c] * x[c];
        x[i] = s / A[i][i];
    }
    return true;
}

/**
 * @brief Fit @p y with the given basis functions and return the residual
 *        sum of squares.  Coefficients are written to @p coeffs.
 */
double fitResidual(const std::vector<double>& y,
                   const std::vector<std::function<double(size_t)>>& basis,
                   std::vector<double>& coeffs) {
    const int n = static_cast<int>(basis.size());
    const size_t N = y.size();
    std::vector<std::vector<double>> XtX(n, std::vector<double>(n, 0.0));
    std::vector<double> Xty(n, 0.0);
    for (size_t i = 0; i < N; ++i) {
        for (int a = 0; a < n; ++a) {
            double ba = basis[a](i);
            Xty[a] += ba * y[i];
            for (int b = 0; b < n; ++b) XtX[a][b] += ba * basis[b](i);
        }
    }
    if (!solveLinear(XtX, Xty, n, coeffs)) return 0.0;
    double rss = 0.0;
    for (size_t i = 0; i < N; ++i) {
        double pred = 0.0;
        for (int a = 0; a < n; ++a) pred += coeffs[a] * basis[a](i);
        double res = y[i] - pred;
        rss += res * res;
    }
    return rss;
}

/**
 * @brief Estimate the ringdown oscillation period (in samples) from the
 *        zero-crossing of the strain autocorrelation.
 */
double estimatePeriodSamples(const std::vector<double>& h) {
    const size_t N = h.size();
    if (N < 4) return 16.0;
    double mean = 0.0;
    for (double v : h) mean += v;
    mean /= static_cast<double>(N);
    std::vector<double> mh(N);
    for (size_t i = 0; i < N; ++i) mh[i] = h[i] - mean;

    size_t maxLag = N / 2;
    double prev = 0.0;
    bool hasPrev = false;
    for (size_t k = 1; k <= maxLag; ++k) {
        double A = 0.0;
        for (size_t i = 0; i + k < N; ++i) A += mh[i] * mh[i + k];
        if (hasPrev && prev > 0.0 && A <= 0.0) {
            double period = 4.0 * static_cast<double>(k); // first zero of cos(w t) at w t = pi/2
            return std::max(4.0, std::min(period, static_cast<double>(N) / 4.0));
        }
        prev = A;
        hasPrev = true;
    }
    return std::max(4.0, static_cast<double>(N) / 16.0);
}

/**
 * @brief Estimate the ringdown damping time (in samples) from the lag at
 *        which the autocorrelation envelope falls to 1/e of its peak.
 */
double estimateDampingSamples(const std::vector<double>& h, double periodSamples) {
    const size_t N = h.size();
    if (N < 4) return 4.0 * periodSamples;
    double mean = 0.0;
    for (double v : h) mean += v;
    mean /= static_cast<double>(N);
    std::vector<double> mh(N);
    for (size_t i = 0; i < N; ++i) mh[i] = h[i] - mean;

    double A0 = 0.0;
    for (size_t i = 0; i + 1 < N; ++i) A0 += mh[i] * mh[i];
    if (A0 <= 0.0) return 4.0 * periodSamples;
    double target = A0 / std::exp(1.0);
    size_t maxLag = N / 2;
    for (size_t k = 1; k <= maxLag; ++k) {
        double A = 0.0;
        for (size_t i = 0; i + k < N; ++i) A += mh[i] * mh[i + k];
        if (std::fabs(A) <= target) {
            double tau = static_cast<double>(k);
            return std::max(periodSamples, std::min(tau, static_cast<double>(N) / 2.0));
        }
    }
    return 4.0 * periodSamples;
}

} // anonymous namespace

ECORingdownAnalyzer::ECORingdownAnalyzer() {
    setParameter("spin", 0.0);
    setParameter("mass_solar", 30.0);
    setParameter("echo_threshold", 0.10);
}

std::vector<InstrumentFinding> ECORingdownAnalyzer::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory) {
    (void)metric;
    std::vector<InstrumentFinding> findings;

    // Extract a clean, finite strain time series.
    std::vector<double> tvec, hvec;
    tvec.reserve(trajectory.size());
    hvec.reserve(trajectory.size());
    for (const auto& pt : trajectory) {
        if (std::isfinite(pt.t) && std::isfinite(pt.x)) {
            tvec.push_back(pt.t);
            hvec.push_back(pt.x);
        }
    }
    const size_t N = hvec.size();
    if (N < kMinPoints) return findings;

    double dt = (tvec.back() - tvec.front()) / static_cast<double>(N - 1);
    if (!(dt > 0.0) || !std::isfinite(dt)) dt = 1.0;

    // Estimate the ringdown period and damping from the EARLY portion of the
    // waveform only.  Echoes (if present) arrive AFTER the main ringdown, so the
    // early segment is a clean single-mode (Kerr) damped sinusoid that pins down
    // the true frequency and decay before any echo can contaminate them.
    size_t earlyN = std::max(kMinPoints, static_cast<size_t>(0.15 * N));
    std::vector<double> earlyH(hvec.begin(), hvec.begin() + earlyN);
    double periodSamples = estimatePeriodSamples(earlyH);
    double dampingSamples = estimateDampingSamples(earlyH, periodSamples);
    double omega = 2.0 * kPi / (periodSamples * dt);
    double tau = dampingSamples * dt;

    // Residual of a single damped-sinusoid (Kerr BH) fit for a given signal.
    auto bhResidual = [&](const std::vector<double>& y, double w, double t) {
        std::vector<std::function<double(size_t)>> basis = {
            [=](size_t i) { double tt = tvec[i]; return std::exp(-tt / t) * std::cos(w * tt); },
            [=](size_t i) { double tt = tvec[i]; return std::exp(-tt / t) * std::sin(w * tt); }
        };
        std::vector<double> c;
        return fitResidual(y, basis, c);
    };

    // Refine (omega, tau) against the early segment only, so the single-mode
    // baseline stays locked to the main ringdown.  A pure damped sinusoid then
    // fits almost perfectly (near-zero residual) and only a genuine delayed
    // echo can raise the significance above threshold.
    double rssEarly = bhResidual(earlyH, omega, tau);
    for (int iw = 0; iw <= 60; ++iw) {
        double w = omega * (0.9 + 0.2 * static_cast<double>(iw) / 60.0);
        for (int it = 0; it <= 20; ++it) {
            double t = tau * (0.7 + 0.6 * static_cast<double>(it) / 20.0);
            if (!(t > 0.0)) continue;
            double r = bhResidual(earlyH, w, t);
            if (r < rssEarly) { rssEarly = r; omega = w; tau = t; }
        }
    }

    // Full-signal single-mode (Kerr) fit using the main-ringdown (omega, tau).
    double rssBH = bhResidual(hvec, omega, tau);
    if (!(rssBH > 0.0) || !std::isfinite(rssBH)) return findings;

    // Total variance of the strain, used to normalise the echo significance.
    // Normalising by the residual of the BH fit would make a near-pure damped
    // sinusoid (tiny BH residual) score spuriously high, so we measure the
    // echo's contribution against the whole signal instead.
    double rssTotal = 0.0;
    for (double v : hvec) rssTotal += v * v;
    if (!(rssTotal > 0.0) || !std::isfinite(rssTotal)) return findings;

    std::vector<double> coeffs;

    // Scan candidate echo delays and keep the most significant one.
    int delayMin = static_cast<int>(std::ceil(1.5 * periodSamples));
    int delayMax = static_cast<int>(std::min(static_cast<double>(N) / 3.0, 25.0 * periodSamples));
    delayMin = std::max(delayMin, 2);
    if (delayMax <= delayMin) delayMax = delayMin + 1;

    double bestSignificance = 0.0;
    int bestDelay = delayMin;
    for (int d = delayMin; d <= delayMax; ++d) {
        double delayTime = static_cast<double>(d) * dt;
        std::vector<std::function<double(size_t)>> ecoBasis = {
            [=](size_t i) { double t = tvec[i]; return std::exp(-t / tau) * std::cos(omega * t); },
            [=](size_t i) { double t = tvec[i]; return std::exp(-t / tau) * std::sin(omega * t); },
            [=](size_t i) {
                double t = tvec[i];
                return (t >= delayTime) ? std::exp(-(t - delayTime) / tau) * std::cos(omega * (t - delayTime)) : 0.0;
            },
            [=](size_t i) {
                double t = tvec[i];
                return (t >= delayTime) ? std::exp(-(t - delayTime) / tau) * std::sin(omega * (t - delayTime)) : 0.0;
            }
        };
        double rssECO = fitResidual(hvec, ecoBasis, coeffs);
        if (!std::isfinite(rssECO)) continue;
        // Fraction of total ringdown variance explained by the echo term.
        double sig = (rssBH - rssECO) / rssTotal;
        if (sig < 0.0) sig = 0.0;
        if (sig > bestSignificance) {
            bestSignificance = sig;
            bestDelay = d;
        }
    }

    double threshold = getParameter("echo_threshold");
    if (bestSignificance <= threshold) return findings;

    InstrumentFinding finding;
    finding.id = "ECORD_" + std::to_string(getTotalFindings());
    finding.instrumentName = getName();
    double confidence = std::min(1.0, bestSignificance);
    finding.severity = confidenceToSeverity(confidence);
    finding.confidence = confidence;
    finding.isAnomaly = true;
    finding.description = "ECO ringdown echo detected: a delayed, repeated pulse "
                          "explains " + std::to_string(bestSignificance * 100.0) +
                          "% of the ringdown variance beyond a single Kerr "
                          "quasi-normal mode. This is the horizonless-compact-object "
                          "echo signature (boson star / gravastar / fuzzball).";
    finding.location = location;
    finding.timestamp = tvec.back();
    finding.parameters["echo_significance"] = bestSignificance;
    finding.parameters["echo_threshold"] = threshold;
    finding.parameters["echo_delay_samples"] = static_cast<double>(bestDelay);
    finding.parameters["echo_delay_time"] = static_cast<double>(bestDelay) * dt;
    finding.parameters["ringdown_period_samples"] = periodSamples;
    finding.parameters["damping_samples"] = dampingSamples;
    finding.parameters["spin"] = getParameter("spin");
    finding.parameters["mass_solar"] = getParameter("mass_solar");
    addFinding(finding);
    findings.push_back(finding);
    return findings;
}

std::map<std::string, std::pair<double, double>> ECORingdownAnalyzer::getParameterRanges() const {
    return {
        {"spin", {0.0, 1.0}},
        {"mass_solar", {5.0, 100.0}},
        {"echo_threshold", {0.01, 0.5}}
    };
}

} // namespace quantumverse
