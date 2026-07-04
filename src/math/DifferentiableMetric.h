#ifndef QUANTUMVERSE_DIFFERENTIABLE_METRIC_H
#define QUANTUMVERSE_DIFFERENTIABLE_METRIC_H

#include "../spacetime/MetricTensor.h"
#include "AutoDiff.h"
#include <functional>

namespace quantumverse {
namespace math {

/**
 * DifferentiableMetric - Wrapper making MetricTensor differentiable
 *
 * Allows automatic differentiation of metric components with respect
 * to parameters (mass, spin, charge, quantum corrections).
 *
 * Usage:
 *   DifferentiableMetric dm([&](double M) { return SchwarzschildMetric(M, r, θ, φ); });
 *   auto grad = dm.gradient(M_value);
 */
class DifferentiableMetric {
private:
    std::function<MetricTensor(double)> metricFunction;
    double baseParams[10];  // Parameter values
    int numParams;

public:
    DifferentiableMetric() : numParams(0) {}

    DifferentiableMetric(
        std::function<MetricTensor(double)> f,
        double initialParam = 0.0
    ) : metricFunction(f), numParams(1) {
        baseParams[0] = initialParam;
    }

    // Set parameter vector
    void setParameters(const std::vector<double>& params) {
        numParams = std::min((int)params.size(), 10);
        for (int i = 0; i < numParams; ++i) {
            baseParams[i] = params[i];
        }
    }

    // Evaluate metric at given parameter values
    MetricTensor evaluate(const std::vector<double>& params) const {
        if (params.size() == 1 && metricFunction) {
            return metricFunction(params[0]);
        }
        // For multi-parameter, would need multivariate AD
        // Placeholder: return Minkowski
        MetricTensor m;
        m.setToMinkowski();
        return m;
    }

    // Compute gradient of metric component (i,j) w.r.t. parameter k
    std::array<double, 10> gradientComponent(
        int i, int j,
        const std::vector<double>& paramValues,
        double h = 1e-5
    ) const {
        std::array<double, 10> grad;
        grad.fill(0.0);

        for (int k = 0; k < numParams; ++k) {
            // Forward difference
            std::vector<double> paramsPlus = paramValues;
            paramsPlus[k] += h;

            MetricTensor m_plus = evaluate(paramsPlus);
            MetricTensor m_base = evaluate(paramValues);

            grad[k] = (m_plus.g[i][j] - m_base.g[i][j]) / h;
        }

        return grad;
    }

    // Compute gradient of scalar curvature invariant (e.g., Kretschmann)
    double gradientCurvature(
        const std::function<double(const MetricTensor&)>& curvatureFunc,
        const std::vector<double>& paramValues,
        double h = 1e-5
    ) const {
        std::vector<double> grad(paramValues.size(), 0.0);

        double baseCurv = curvatureFunc(evaluate(paramValues));

        for (size_t k = 0; k < paramValues.size(); ++k) {
            std::vector<double> paramsPlus = paramValues;
            paramsPlus[k] += h;
            double plusCurv = curvatureFunc(evaluate(paramsPlus));
            grad[k] = (plusCurv - baseCurv) / h;
        }

        // Return magnitude of gradient
        double mag2 = 0.0;
        for (double g : grad) mag2 += g*g;
        return std::sqrt(mag2);
    }

    // Use ADVariable for exact derivatives (single parameter)
    double adGradientComponent(
        int i, int j,
        double param,
        const std::function<ADVariable<1>(ADVariable<1>)>& f
    ) const {
        // Create AD variable
        ADVariable<1> x(param, 1.0);  // value=param, derivative=1
        ADVariable<1> result = f(x);
        return result.getDerivs()[0];
    }

    // Convenience: gradient of Kretschmann scalar w.r.t. mass for Schwarzschild
    static double gradientKretschmannSchwarzschild(double M, double r) {
        // K = 48 G^2 M^2 / r^6  (in geometric units G=c=1)
        // dK/dM = 96 M / r^6
        return 96.0 * M / (r * r * r * r * r * r);
    }

    // Convenience: gradient of light deflection angle w.r.t. mass
    static double gradientDeflectionSchwarzschild(double M, double impactParam) {
        // α = 4M / b  (radians, geometric units)
        // dα/dM = 4 / b
        return 4.0 / impactParam;
    }
};

} // namespace math
} // namespace quantumverse

#endif // QUANTUMVERSE_DIFFERENTIABLE_METRIC_H
