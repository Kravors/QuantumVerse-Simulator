#ifndef QUANTUMVERSE_DIFFERENTIABLE_METRIC_H
#define QUANTUMVERSE_DIFFERENTIABLE_METRIC_H

/**
 * @file DifferentiableMetric.h
 * @brief QuantumVerse Simulator - Differentiable Metric Interface
 *
 * Provides an abstract interface for metrics that support automatic differentiation,
 * enabling end-to-end gradient computation through the geodesic integrator.
 *
 * Based on: Spacetime.md - Differentiable Spacetime
 * Uses: AutoDiff.h for forward-mode automatic differentiation
 */

#include <string>
#include <vector>
#include <memory>
#include <array>
#include <cmath>
#include <stdexcept>
#include <utility>

#include "Event4D.h"
#include "MetricTensor.h"
#include "../math/AutoDiff.h"

namespace quantumverse {

/**
 * @brief Abstract base class for metrics with automatic differentiation support.
 *
 * DifferentiableMetric extends MetricTensor by providing methods that return
 * ADVariable types, enabling gradient propagation through the metric evaluation
 * into the geodesic integrator and loss functions.
 */
class DifferentiableMetric {
public:
    virtual ~DifferentiableMetric() = default;

    /**
     * @brief Get the number of parameters this metric depends on.
     */
    [[nodiscard]] virtual size_t parameterCount() const noexcept = 0;

    /**
     * @brief Get the names of parameters (for debugging and logging).
     */
    [[nodiscard]] virtual std::vector<std::string> parameterNames() const {
        std::vector<std::string> names(parameterCount());
        for (size_t i = 0; i < parameterCount(); i++) {
            names[i] = "param_" + std::to_string(static_cast<long long>(i));
        }
        return names;
    }

    /**
     * @brief Get the current parameter values.
     */
    [[nodiscard]] virtual std::vector<double> getParameters() const = 0;

    /**
     * @brief Set the parameter values.
     */
    virtual void setParameters(const std::vector<double>& params) = 0;

    /**
     * @brief Evaluate the metric tensor at an event using standard doubles.
     */
    virtual MetricTensor evaluate(const Event4D& event) const = 0;

    /**
     * @brief Evaluate the metric tensor at an event with AD variables.
     *
     * @param event Spacetime event.
     * @param paramSeeds Seeds for parameter derivatives as array of size 4.
     *                  Defaults to identity seeds for the first parameter.
     * @return Array of 16 ADVariable<4> components in row-major order.
     */
    virtual std::array<math::ADVariable<4>, 16> evaluateAD(
        const Event4D& event,
        const std::array<double, 4>& paramSeeds = {1.0, 0.0, 0.0, 0.0}
    ) const = 0;

    /**
     * @brief Compute the metric and its Jacobian w.r.t. parameters at an event.
     *
     * @param event Spacetime event.
     * @return Pair of (MetricTensor, Jacobian matrix as flat vector of 16*paramCount elements).
     */
    virtual std::pair<MetricTensor, std::vector<double>> evaluateWithJacobian(
        const Event4D& event
    ) const = 0;

    /**
     * @brief Clone this metric (virtual copy constructor).
     */
    virtual std::unique_ptr<DifferentiableMetric> clone() const = 0;

    /**
     * @brief Get a descriptive name for this metric type.
     */
    [[nodiscard]] virtual std::string name() const = 0;

    /**
     * @brief Check if the metric is valid at the given event.
     */
    virtual bool isValid(const Event4D& event) const {
        MetricTensor g = evaluate(event);
        return g.isLorentzian() && std::abs(g.determinant()) > 1e-15;
    }
};

/**
 * @brief Differentiable Schwarzschild metric.
 *
 * Parameters: [M] (mass in geometric units, G=c=1)
 *
 * ds^2 = -(1 - 2M/r) dt^2 + (1 - 2M/r)^-1 dr^2 + r^2 dOmega^2
 */
class DifferentiableSchwarzschildMetric : public DifferentiableMetric {
public:
    explicit DifferentiableSchwarzschildMetric(double mass = 1.0)
        : M_(mass) {}

    size_t parameterCount() const noexcept override { return 1; }

    std::vector<std::string> parameterNames() const override {
        return {"M"};
    }

    std::vector<double> getParameters() const override {
        return {M_};
    }

    void setParameters(const std::vector<double>& params) override {
        if (params.size() != 1) {
            throw std::invalid_argument("DifferentiableSchwarzschildMetric: expected 1 parameter");
        }
        M_ = params[0];
    }

    MetricTensor evaluate(const Event4D& event) const override {
        double r = event.spatialLength();
        if (r < 1e-10) r = 1e-10;

        double rs = 2.0 * M_;
        double factor = 1.0 - rs / r;

        MetricTensor metric;
        metric.setToZero();
        metric.g[0][0] = -factor;
        metric.g[1][1] = 1.0 / factor;
        metric.g[2][2] = r * r;
        metric.g[3][3] = r * r * std::sin(event.theta()) * std::sin(event.theta());

        return metric;
    }

    std::array<math::ADVariable<4>, 16> evaluateAD(
        const Event4D& event,
        const std::array<double, 4>& paramSeeds = {1.0, 0.0, 0.0, 0.0}
    ) const override {
        math::ADVariable<4> M_ad(M_, {paramSeeds[0], paramSeeds[1], paramSeeds[2], paramSeeds[3]});

        double r = event.spatialLength();
        if (r < 1e-10) r = 1e-10;

        math::ADVariable<4> rs = M_ad * 2.0;
        math::ADVariable<4> factor = math::ADVariable<4>(1.0) - rs / math::ADVariable<4>(r);

        std::array<math::ADVariable<4>, 16> result;
        result[0] = -factor;
        result[5] = math::ADVariable<4>(1.0) / factor;
        result[10] = math::ADVariable<4>(r * r);
        result[15] = math::ADVariable<4>(r * r * std::sin(event.theta()) * std::sin(event.theta()));

        for (int i : {1,2,3,4,6,7,8,9,11,12,13,14}) {
            result[i] = math::ADVariable<4>(0.0);
        }

        return result;
    }

    std::pair<MetricTensor, std::vector<double>> evaluateWithJacobian(
        const Event4D& event
    ) const override {
        MetricTensor metric = evaluate(event);

        double r = event.spatialLength();
        if (r < 1e-10) r = 1e-10;

        double rs = 2.0 * M_;
        double factor = 1.0 - rs / r;

        std::vector<double> jacobian(16, 0.0);
        jacobian[0] = 2.0 / r;
        jacobian[5] = (2.0 / r) / (factor * factor);

        return std::make_pair(metric, jacobian);
    }

    std::unique_ptr<DifferentiableMetric> clone() const override {
        return std::make_unique<DifferentiableSchwarzschildMetric>(*this);
    }

    std::string name() const override { return "DifferentiableSchwarzschild"; }

private:
    double M_;
};

/**
 * @brief Differentiable Kerr metric (rotating black hole).
 *
 * Parameters: [M, a] where a is the dimensionless spin parameter.
 */
class DifferentiableKerrMetric : public DifferentiableMetric {
public:
    DifferentiableKerrMetric(double mass = 1.0, double spin = 0.0)
        : M_(mass), a_(spin) {}

    size_t parameterCount() const noexcept override { return 2; }

    std::vector<std::string> parameterNames() const override {
        return {"M", "a"};
    }

    std::vector<double> getParameters() const override {
        return {M_, a_};
    }

    void setParameters(const std::vector<double>& params) override {
        if (params.size() != 2) {
            throw std::invalid_argument("DifferentiableKerrMetric: expected 2 parameters");
        }
        M_ = params[0];
        a_ = params[1];
    }

    MetricTensor evaluate(const Event4D& event) const override {
        double r = event.spatialLength();
        double theta = event.theta();

        if (r < 1e-10) r = 1e-10;

        double a2 = a_ * a_;
        double sin_theta = std::sin(theta);
        double sin2 = sin_theta * sin_theta;
        double cos2 = std::cos(theta) * std::cos(theta);

        double Sigma = r * r + a2 * cos2;
        double Delta = r * r - 2.0 * M_ * r + a2;

        if (std::abs(Delta) < 1e-10) Delta = 1e-10;
        if (std::abs(Sigma) < 1e-10) Sigma = 1e-10;

        MetricTensor metric;
        metric.setToZero();

        metric.g[0][0] = -(1.0 - 2.0 * M_ * r / Sigma);
        metric.g[0][3] = -2.0 * M_ * r * a_ * sin2 / Sigma;
        metric.g[3][0] = metric.g[0][3];
        metric.g[1][1] = Sigma / Delta;
        metric.g[2][2] = Sigma;
        metric.g[3][3] = (r * r + a2 + 2.0 * M_ * r * a2 * sin2 / Sigma) * sin2;

        return metric;
    }

    std::array<math::ADVariable<4>, 16> evaluateAD(
        const Event4D& event,
        const std::array<double, 4>& paramSeeds = {1.0, 0.0, 0.0, 0.0}
    ) const override {
        math::ADVariable<4> M_ad(M_, {paramSeeds[0], paramSeeds[1], 0.0, 0.0});
        math::ADVariable<4> a_ad(a_, {0.0, 0.0, paramSeeds[2], paramSeeds[3]});

        double r = event.spatialLength();
        double theta = event.theta();
        if (r < 1e-10) r = 1e-10;

        math::ADVariable<4> a2 = a_ad * a_ad;
        math::ADVariable<4> sin_theta = math::sin(math::ADVariable<4>(theta));
        math::ADVariable<4> sin2 = sin_theta * sin_theta;
        math::ADVariable<4> cos_theta = math::cos(math::ADVariable<4>(theta));
        math::ADVariable<4> cos2 = cos_theta * cos_theta;

        math::ADVariable<4> r2(r * r);
        math::ADVariable<4> Sigma = r2 + a2 * cos2;
        math::ADVariable<4> Delta = r2 - M_ad * 2.0 * math::ADVariable<4>(r) + a2;
        math::ADVariable<4> inv_Sigma = math::ADVariable<4>(1.0) / Sigma;

        std::array<math::ADVariable<4>, 16> result;
        result[0] = -(math::ADVariable<4>(1.0) - M_ad * 2.0 * math::ADVariable<4>(r) * inv_Sigma);
        result[5] = Sigma / Delta;
        result[10] = Sigma;
        result[15] = (r2 + a2 + M_ad * 2.0 * math::ADVariable<4>(r) * a2 * sin2 * inv_Sigma) * sin2;

        result[3] = -M_ad * 2.0 * math::ADVariable<4>(r) * a_ad * sin2 * inv_Sigma;
        result[12] = result[3];

        for (int i : {1,2,4,6,7,8,9,11,13,14}) {
            result[i] = math::ADVariable<4>(0.0);
        }

        return result;
    }

    std::pair<MetricTensor, std::vector<double>> evaluateWithJacobian(
        const Event4D& event
    ) const override {
        MetricTensor metric = evaluate(event);

        double r = event.spatialLength();
        double theta = event.theta();
        if (r < 1e-10) r = 1e-10;

        double a2 = a_ * a_;
        double sin_theta = std::sin(theta);
        double sin2 = sin_theta * sin_theta;
        double cos2 = std::cos(theta) * std::cos(theta);
        double Sigma = r * r + a2 * cos2;
        double Delta = r * r - 2.0 * M_ * r + a2;

        if (std::abs(Delta) < 1e-10) Delta = 1e-10;
        if (std::abs(Sigma) < 1e-10) Sigma = 1e-10;

        std::vector<double> jacobian(32, 0.0);

        jacobian[0 * 2 + 0] = 2.0 * r / Sigma;
        jacobian[0 * 2 + 1] = 4.0 * M_ * r * a_ * cos2 / (Sigma * Sigma);

        jacobian[5 * 2 + 0] = 2.0 * r * Sigma / (Delta * Delta);
        jacobian[5 * 2 + 1] = -2.0 * a_ * cos2 / Delta;

        jacobian[10 * 2 + 1] = -2.0 * a_ * cos2;

        jacobian[15 * 2 + 0] = 2.0 * r * a2 * sin2 / Sigma;
        jacobian[15 * 2 + 1] = 4.0 * a_ * r * r * sin2 / Sigma;

        jacobian[3 * 2 + 0] = -2.0 * r * a_ * sin2 / Sigma;
        jacobian[3 * 2 + 1] = -2.0 * M_ * r * sin2 * (Sigma - 2.0 * a2 * cos2) / (Sigma * Sigma);

        jacobian[12 * 2 + 0] = jacobian[3 * 2 + 0];
        jacobian[12 * 2 + 1] = jacobian[3 * 2 + 1];

        return std::make_pair(metric, jacobian);
    }

    std::unique_ptr<DifferentiableMetric> clone() const override {
        return std::make_unique<DifferentiableKerrMetric>(*this);
    }

    std::string name() const override { return "DifferentiableKerr"; }

private:
    double M_;
    double a_;
};

/**
 * @brief Differentiable Reissner-Nordstroem metric (charged black hole).
 *
 * Parameters: [M, Q] where Q is the electric charge.
 */
class DifferentiableRNMetric : public DifferentiableMetric {
public:
    DifferentiableRNMetric(double mass = 1.0, double charge = 0.0)
        : M_(mass), Q_(charge) {}

    size_t parameterCount() const noexcept override { return 2; }

    std::vector<std::string> parameterNames() const override {
        return {"M", "Q"};
    }

    std::vector<double> getParameters() const override {
        return {M_, Q_};
    }

    void setParameters(const std::vector<double>& params) override {
        if (params.size() != 2) {
            throw std::invalid_argument("DifferentiableRNMetric: expected 2 parameters");
        }
        M_ = params[0];
        Q_ = params[1];
    }

    MetricTensor evaluate(const Event4D& event) const override {
        double r = event.spatialLength();
        if (r < 1e-10) r = 1e-10;

        double rs = 2.0 * M_;
        double rq2 = Q_ * Q_;
        double f = 1.0 - rs / r + rq2 / (r * r);

        MetricTensor metric;
        metric.setToZero();
        metric.g[0][0] = -f;
        metric.g[1][1] = 1.0 / f;
        metric.g[2][2] = r * r;
        metric.g[3][3] = r * r * std::sin(event.theta()) * std::sin(event.theta());

        return metric;
    }

    std::array<math::ADVariable<4>, 16> evaluateAD(
        const Event4D& event,
        const std::array<double, 4>& paramSeeds = {1.0, 0.0, 0.0, 0.0}
    ) const override {
        math::ADVariable<4> M_ad(M_, {paramSeeds[0], paramSeeds[1], 0.0, 0.0});
        math::ADVariable<4> Q_ad(Q_, {0.0, 0.0, paramSeeds[2], paramSeeds[3]});

        double r = event.spatialLength();
        if (r < 1e-10) r = 1e-10;

        math::ADVariable<4> rs = M_ad * 2.0;
        math::ADVariable<4> rq2 = Q_ad * Q_ad;
        math::ADVariable<4> r_ad(r);
        math::ADVariable<4> f = math::ADVariable<4>(1.0) - rs / r_ad + rq2 / (r_ad * r_ad);

        std::array<math::ADVariable<4>, 16> result;
        result[0] = -f;
        result[5] = math::ADVariable<4>(1.0) / f;
        result[10] = math::ADVariable<4>(r * r);
        result[15] = math::ADVariable<4>(r * r) *
                     math::sin(math::ADVariable<4>(event.theta())) *
                     math::sin(math::ADVariable<4>(event.theta()));

        for (int i : {1,2,3,4,6,7,8,9,11,12,13,14}) {
            result[i] = math::ADVariable<4>(0.0);
        }

        return result;
    }

    std::pair<MetricTensor, std::vector<double>> evaluateWithJacobian(
        const Event4D& event
    ) const override {
        MetricTensor metric = evaluate(event);

        double r = event.spatialLength();
        if (r < 1e-10) r = 1e-10;

        double rs = 2.0 * M_;
        double rq2 = Q_ * Q_;
        double f = 1.0 - rs / r + rq2 / (r * r);

        std::vector<double> jacobian(32, 0.0);

        jacobian[0 * 2 + 0] = 2.0 / r;
        jacobian[0 * 2 + 1] = -2.0 * Q_ / (r * r);

        jacobian[5 * 2 + 0] = 2.0 / (f * f);
        jacobian[5 * 2 + 1] = 2.0 * Q_ / (r * r * f * f);

        return std::make_pair(metric, jacobian);
    }

    std::unique_ptr<DifferentiableMetric> clone() const override {
        return std::make_unique<DifferentiableRNMetric>(*this);
    }

    std::string name() const override { return "DifferentiableReissnerNordstrom"; }

private:
    double M_;
    double Q_;
};

/**
 * @brief Factory function to create differentiable metrics by name.
 */
inline std::unique_ptr<DifferentiableMetric> createDifferentiableMetric(
    const std::string& type,
    const std::vector<double>& params = {}
) {
    if (type == "schwarzschild") {
        double M = params.empty() ? 1.0 : params[0];
        return std::make_unique<DifferentiableSchwarzschildMetric>(M);
    } else if (type == "kerr") {
        double M = params.empty() ? 1.0 : params[0];
        double a = params.size() > 1 ? params[1] : 0.0;
        return std::make_unique<DifferentiableKerrMetric>(M, a);
    } else if (type == "reissner-nordstrom" || type == "rn") {
        double M = params.empty() ? 1.0 : params[0];
        double Q = params.size() > 1 ? params[1] : 0.0;
        return std::make_unique<DifferentiableRNMetric>(M, Q);
    }
    throw std::invalid_argument("Unknown metric type: " + type);
}

} // namespace quantumverse

#endif // QUANTUMVERSE_DIFFERENTIABLE_METRIC_H