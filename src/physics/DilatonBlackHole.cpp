#include "physics/DilatonBlackHole.h"

namespace quantumverse {

DilatonBlackHole::DilatonBlackHole(double mass, double sigma) noexcept
    : metric(sigma, mass) {}

bool DilatonBlackHole::isHorizon(double x_plus, double x_minus, double tol) const noexcept {
    return metric.isApparentHorizon(x_plus, x_minus, tol);
}

bool DilatonBlackHole::isSingularity(double x_plus, double x_minus, double tol) const noexcept {
    return metric.isSingularity(x_plus, x_minus, tol);
}

const DilatonMetric& DilatonBlackHole::getMetric() const noexcept {
    return metric;
}

double DilatonBlackHole::getMass() const noexcept {
    return metric.getMass();
}

double DilatonBlackHole::getSigma() const noexcept {
    return metric.getSigma();
}

double DilatonBlackHole::hawkingTemperature() const noexcept {
    double m = getMass();
    if (m == 0.0) return 0.0;
    return 1.0 / (4.0 * PI * m);
}

double DilatonBlackHole::entropy() const noexcept {
    return 2.0 * PI * getMass();
}

} // namespace quantumverse
