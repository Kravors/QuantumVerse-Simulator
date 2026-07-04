#include "physics/HawkingCalculator.h"
#include <cmath>

namespace quantumverse {

HawkingCalculator::HawkingCalculator(std::shared_ptr<DilatonMetric> blackHole) noexcept
    : blackHole(std::move(blackHole)) {}

double HawkingCalculator::computeTMinusMinus(double x_plus, double x_minus) const noexcept {
    if (blackHole->getMass() == 0.0) {
        return 0.0;
    }
    if (x_minus < 0.0) {
        return 1.0 / 48.0;
    }
    return 0.0;
}

double HawkingCalculator::asymptoticFlux() const noexcept {
    return 1.0 / 48.0;
}

double HawkingCalculator::bogoliubovBetaSquared(double omega) const noexcept {
    double T = blackHole->hawkingTemperature();
    if (T <= 0.0) return 0.0;
    return 1.0 / (std::exp(omega / T) - 1.0);
}

double HawkingCalculator::temperature() const noexcept {
    return blackHole->hawkingTemperature();
}

} // namespace quantumverse
