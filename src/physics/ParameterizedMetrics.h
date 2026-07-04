#ifndef QUANTUMVERSE_PARAMETERIZED_METRICS_H
#define QUANTUMVERSE_PARAMETERIZED_METRICS_H

#include "../spacetime/MetricTensor.h"
#include "../math/AutoDiff.h"
#include <array>

namespace quantumverse {

using math::ADVariable;

/**
 * @brief Parameterized Schwarzschild metric with AD-compatible mass parameter.
 * 
 * This metric wraps a Schwarzschild metric where the mass M is represented
 * as an ADVariable, enabling gradient computation through the metric itself.
 * 
 * Usage:
 *   auto M = ADVariable<1>(5.0, {1.0});  // M = 5.0, ∂M/∂θ₀ = 1.0
 *   ParameterizedSchwarzschildMetric metric(M);
 *   auto g = metric.compute(Event4D{0,0,0,0});
 */
class ParameterizedSchwarzschildMetric {
private:
    ADVariable<1> M_;  // Mass as AD variable (single parameter)
    
public:
    explicit ParameterizedSchwarzschildMetric(const ADVariable<1>& M) : M_(M) {}
    
    /**
     * @brief Compute metric tensor at given event
     */
    MetricTensor compute(const Event4D& event) const {
        double r = event.spatialLength();
        double t = event.t;
        
        // Schwarzschild radius: rs = 2GM/c² (G=c=1)
        double rs = 2.0 * M_.getValue();
        
        // Avoid singularity
        if (r <= rs) r = rs + 1e-10;
        
        // Metric components (diagonal)
        double g_tt = -(1.0 - rs/r);
        double g_rr = 1.0 / (1.0 - rs/r);
        double g_theta_theta = r * r;
        double g_phi_phi = r * r * sin(event.theta()) * sin(event.theta());

        // Build diagonal metric
        std::array<double, 4> diag = {g_tt, g_rr, g_theta_theta, g_phi_phi};
        return MetricTensor::diagonal(diag);
    }

    /**
     * @brief Get mass value (for non-AD contexts)
     */
    double getMass() const { return M_.getValue(); }
    
    /**
     * @brief Get AD mass variable
     */
    const ADVariable<1>& getMassAD() const { return M_; }
};

/**
 * @brief Parameterized Kerr metric (mass M, spin a)
 * 
 * Supports forward-mode AD for both parameters simultaneously.
 * For N-parameter gradients, use ADVariable<N> with appropriate seed vectors.
 */
class ParameterizedKerrMetric {
private:
    ADVariable<2> M_;   // Mass (parameter index 0)
    ADVariable<2> a_;   // Spin parameter (parameter index 1)
    
public:
    ParameterizedKerrMetric(const ADVariable<2>& M, const ADVariable<2>& a)
        : M_(M), a_(a) {}
    
    MetricTensor compute(const Event4D& event) const {
        double r = event.spatialLength();
        double theta = event.theta();

        double M_val = M_.getValue();
        double a_val = a_.getValue();
        
        double rs = 2.0 * M_val;
        double a2 = a_val * a_val;
        
        // Avoid singularities
        if (r <= rs) r = rs + 1e-10;
        
        double sin_theta = sin(theta);
        double cos_theta = cos(theta);
        double sin2 = sin_theta * sin_theta;
        
        // Kerr metric components (Boyer-Lindquist coordinates)
        double Sigma = r*r + a2 * cos_theta * cos_theta;
        double Delta = r*r - rs*r + a2;
        
        if (fabs(Delta) < 1e-10) Delta = 1e-10;
        
        double g_tt = -(1.0 - rs*r/Sigma);
        double g_t_phi = -rs*r*a_val*sin2/Sigma;
        double g_rr = Sigma/Delta;
        double g_theta_theta = Sigma;
        double g_phi_phi = (r*r + a2 + rs*r*a2*sin2/Sigma) * sin2;
        
        // Build metric (non-diagonal due to g_t_phi)
        std::array<double, 4> diag = {g_tt, g_rr, g_theta_theta, g_phi_phi};
        MetricTensor metric = MetricTensor::diagonal(diag);
        metric.set(0, 3, g_t_phi);  // g_tφ
        metric.set(3, 0, g_t_phi);  // g_φt (symmetric)
        
        return metric;
    }
    
    const ADVariable<2>& getMassAD() const { return M_; }
    const ADVariable<2>& getSpinAD() const { return a_; }
};

/**
 * @brief Parameterized Reissner-Nordström metric (M, Q)
 */
class ParameterizedRNMetric {
private:
    ADVariable<2> M_;  // Mass (parameter 0)
    ADVariable<2> Q_;  // Charge (parameter 1)
    
public:
    ParameterizedRNMetric(const ADVariable<2>& M, const ADVariable<2>& Q)
        : M_(M), Q_(Q) {}
    
    MetricTensor compute(const Event4D& event) const {
        double r = event.spatialLength();
        
        double M_val = M_.getValue();
        double Q_val = Q_.getValue();
        
        double rs = 2.0 * M_val;
        double rq2 = Q_val * Q_val;
        
        if (r <= rs) r = rs + 1e-10;
        
        double f = 1.0 - rs/r + rq2/(r*r);
        double g_tt = -f;
        double g_rr = 1.0 / f;
        double g_theta_theta = r * r;
        double g_phi_phi = r * r * sin(event.theta()) * sin(event.theta());

        std::array<double, 4> diag = {g_tt, g_rr, g_theta_theta, g_phi_phi};
        return MetricTensor::diagonal(diag);
    }

    const ADVariable<2>& getMassAD() const { return M_; }
    const ADVariable<2>& getChargeAD() const { return Q_; }
};

} // namespace quantumverse

#endif // QUANTUMVERSE_PARAMETERIZED_METRICS_H