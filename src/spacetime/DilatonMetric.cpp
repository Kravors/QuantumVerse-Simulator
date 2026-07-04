#include "spacetime/DilatonMetric.h"
#include <cmath>

namespace quantumverse {

// Local constant for π
static constexpr double PI = 3.14159265358979323846;

DilatonMetric::DilatonMetric(double sigma, double mass) noexcept
    : sigma(sigma), mass(mass < 0.0 ? 0.0 : mass) {}

double DilatonMetric::conformalFactor(double x_plus, double x_minus) const noexcept {
    if (mass == 0.0) {
        return 1.0;
    }
    return 1.0 + mass * std::exp(2.0 * sigma) * x_plus * x_minus;
}

double DilatonMetric::dilaton(double x_plus, double x_minus) const noexcept {
    if (mass == 0.0) {
        return -sigma;
    }
    double A = conformalFactor(x_plus, x_minus);
    return -sigma + 0.5 * std::log(A);
}

double DilatonMetric::metricComponent(int mu, int nu, double x_plus, double x_minus) const noexcept {
    if (mu != nu) {
        return -0.5 * conformalFactor(x_plus, x_minus);
    }
    return 0.0;
}

double DilatonMetric::ricciScalar(double x_plus, double x_minus) const noexcept {
    if (mass == 0.0) {
        return 0.0;
    }
    double A = conformalFactor(x_plus, x_minus);
    return 4.0 * mass * std::exp(2.0 * sigma) / (A * A * A);
}

bool DilatonMetric::isApparentHorizon(double x_plus, double x_minus, double tol) const noexcept {
    if (mass == 0.0) return false;
    return (std::abs(x_minus) < tol) || (std::abs(x_plus) < tol);
}

bool DilatonMetric::isSingularity(double x_plus, double x_minus, double tol) const noexcept {
    if (mass == 0.0) return false;
    double A = conformalFactor(x_plus, x_minus);
    return std::abs(A) < tol;
}

double DilatonMetric::hawkingTemperature() const noexcept {
    if (mass == 0.0) return 0.0;
    return 1.0 / (4.0 * PI * mass);
}

} // namespace quantumverse
