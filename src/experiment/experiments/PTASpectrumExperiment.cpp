/**
 * @file PTASpectrumExperiment.cpp
 * @brief PTA characteristic strain spectrum h_c(f) for the SMBHB background.
 *
 * h_c(f) = A * (f / f_yr)^alpha
 *
 * where f_yr = 1/year (Hz) and alpha = -2/3 for the standard SMBHB model.
 * Output curve is (f_nHz, log10(h_c)) so the power law appears as a straight
 * line in log-log space. First experiment with no cosmology-provider dependency.
 */

#include "PTASpectrumExperiment.h"

#include <algorithm>
#include <cmath>

namespace quantumverse {

QVector<ExperimentParameter> PTASpectrumExperiment::parameters() const {
    QVector<ExperimentParameter> p;
    p.append({QStringLiteral("A_amplitude"), QStringLiteral("Strain amplitude A"),
              QStringLiteral("double"), QVariant(2.4e-15), QVariant(1e-16), QVariant(1e-14),
              QStringList(), QStringLiteral(""), QStringLiteral("Characteristic strain at f_yr.")});
    p.append({QStringLiteral("alpha"), QStringLiteral("Spectral index alpha"),
              QStringLiteral("double"), QVariant(-2.0 / 3.0), QVariant(-3.0), QVariant(0.0),
              QStringList(), QStringLiteral(""),
              QStringLiteral("h_c(f) ~ f^alpha; -2/3 for SMBHB.")});
    p.append({QStringLiteral("f_min_nHz"), QStringLiteral("Min frequency (nHz)"),
              QStringLiteral("double"), QVariant(1.0), QVariant(0.01), QVariant(100.0),
              QStringList(), QStringLiteral("nHz"), QStringLiteral("Low end of frequency sweep.")});
    p.append({QStringLiteral("f_max_nHz"), QStringLiteral("Max frequency (nHz)"),
              QStringLiteral("double"), QVariant(100.0), QVariant(1.0), QVariant(1000.0),
              QStringList(), QStringLiteral("nHz"),
              QStringLiteral("High end of frequency sweep.")});
    p.append({QStringLiteral("n_steps"), QStringLiteral("Frequency samples"), QStringLiteral("int"),
              QVariant(40), QVariant(10), QVariant(200), QStringList(), QStringLiteral(""),
              QStringLiteral("Number of f points on the curve.")});
    return p;
}

ExperimentResult PTASpectrumExperiment::run(const QVariantMap& params,
                                            std::function<void(double)> progress,
                                            std::function<bool()> shouldAbort) {
    ExperimentResult out;

    const double A = params.value(QStringLiteral("A_amplitude"), 2.4e-15).toDouble();
    const double alpha = params.value(QStringLiteral("alpha"), -2.0 / 3.0).toDouble();
    const double f_min_nHz = params.value(QStringLiteral("f_min_nHz"), 1.0).toDouble();
    const double f_max_nHz = params.value(QStringLiteral("f_max_nHz"), 100.0).toDouble();
    const int n_steps = params.value(QStringLiteral("n_steps"), 40).toInt();

    if (!(A > 0.0 && f_min_nHz > 0.0 && f_max_nHz > f_min_nHz && n_steps >= 2)) {
        out.success = false;
        out.logLines << "Invalid parameters: need A > 0, f_max > f_min > 0, n_steps >= 2";
        return out;
    }

    constexpr double SECONDS_PER_YEAR = 31557600.0;  // 365.25 * 86400
    const double f_yr_Hz = 1.0 / SECONDS_PER_YEAR;   // Hz
    const double f_yr_nHz = f_yr_Hz * 1e9;           // nHz

    out.scalars[QStringLiteral("A_amplitude")] = A;
    out.scalars[QStringLiteral("alpha")] = alpha;
    out.scalars[QStringLiteral("f_yr_nHz")] = f_yr_nHz;
    out.scalars[QStringLiteral("f_min_nHz")] = f_min_nHz;
    out.scalars[QStringLiteral("f_max_nHz")] = f_max_nHz;
    out.scalars[QStringLiteral("n_steps")] = double(n_steps);

    const int n = std::max(1, n_steps);
    double sum_log_f = 0.0, sum_log_hc = 0.0, sum_log_f_log_hc = 0.0, sum_log_f_sq = 0.0;

    for (int i = 0; i < n; ++i) {
        if (shouldAbort()) {
            out.success = false;
            return out;
        }

        const double f_nHz = f_min_nHz + (f_max_nHz - f_min_nHz) * static_cast<double>(i) /
                                             static_cast<double>(n > 1 ? n - 1 : 1);
        const double f_Hz = f_nHz * 1e-9;

        const double ratio = f_Hz / f_yr_Hz;  // f / f_yr
        const double h_c = A * std::pow(ratio, alpha);

        out.curve.append(QPointF(f_nHz, std::log10(h_c)));
        out.logLines << QString("f=%1 nHz  log10(h_c)=%2  h_c=%3")
                            .arg(f_nHz, 0, 'f', 3)
                            .arg(std::log10(h_c), 0, 'f', 4)
                            .arg(h_c, 0, 'e', 3);

        // Accumulate for least-squares slope in log-log space
        const double log_f = std::log10(f_nHz);
        const double log_hc = std::log10(h_c);
        sum_log_f += log_f;
        sum_log_hc += log_hc;
        sum_log_f_log_hc += log_f * log_hc;
        sum_log_f_sq += log_f * log_f;

        if (progress)
            progress(static_cast<double>(i + 1) / static_cast<double>(n));
    }

    // Reference pins at 1, 10, 100 nHz
    auto h_c_at = [&](double f_nHz) -> double {
        const double f_Hz = f_nHz * 1e-9;
        return A * std::pow(f_Hz / f_yr_Hz, alpha);
    };
    out.scalars[QStringLiteral("h_c_at_1nHz")] = h_c_at(1.0);
    out.scalars[QStringLiteral("h_c_at_10nHz")] = h_c_at(10.0);
    out.scalars[QStringLiteral("h_c_at_100nHz")] = h_c_at(100.0);

    // Least-squares slope in log-log space: log10(h_c) = slope * log10(f) + intercept
    const double denom = n * sum_log_f_sq - sum_log_f * sum_log_f;
    const double slope =
        (std::abs(denom) > 1e-12) ? (n * sum_log_f_log_hc - sum_log_f * sum_log_hc) / denom : 0.0;
    out.scalars[QStringLiteral("slope_loglog")] = slope;

    out.success = true;
    return out;
}

}  // namespace quantumverse
