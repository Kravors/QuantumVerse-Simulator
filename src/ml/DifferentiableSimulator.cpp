// DifferentiableSimulator.cpp
// End-to-end differentiable physics simulation wrapper
//
// Implements parameter-to-observable gradient computation via
// central finite differences on a MetricTensor factory.
//
// Author: QuantumVerse Phase 3 Implementation
// Date: 2026-05-13

#define _USE_MATH_DEFINES
#include "DifferentiableSimulator.h"

// Ensure M_PI is available on all platforms (MSVC <cmath> may not define it)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <algorithm>
#include <numeric>
#include <iostream>
#include <iomanip>
#include <cmath>

namespace quantumverse {

// ============================================================================
// DifferentiableSimulator Implementation
// ============================================================================

DifferentiableSimulator::DifferentiableSimulator(
    MetricFactory factory,
    std::vector<std::string> paramNames,
    const SimulatorConfig& config
)
    : metricFactory_(std::move(factory))
    , paramNames_(std::move(paramNames))
    , config_(config)
    , baseMetric_(nullptr)
{
    if (!metricFactory_) {
        throw std::invalid_argument(
            "DifferentiableSimulator: metric factory must not be nullptr"
        );
    }
}

void DifferentiableSimulator::rebuildMetric_() const
{
    if (metricFactory_) {
        baseMetric_ = metricFactory_(currentParams_);
    }
}

Event4D DifferentiableSimulator::simulate(
    const Event4D& initialEvent,
    const std::array<double, 4>& initialVelocity,
    GeodesicType geodesicType,
    double targetProperTime
) const
{
    auto trajectory = simulateFull(
        initialEvent, initialVelocity, geodesicType,
        targetProperTime, false
    );

    if (trajectory.empty()) {
        return initialEvent;
    }
    return trajectory.back().event;
}

std::vector<GeodesicStep> DifferentiableSimulator::simulateFull(
    const Event4D& initialEvent,
    const std::array<double, 4>& initialVelocity,
    GeodesicType geodesicType,
    double targetProperTime,
    bool storeTrajectory
) const
{
    rebuildMetric_();

    if (!baseMetric_) {
        throw std::runtime_error(
            "DifferentiableSimulator: failed to create metric"
        );
    }

    // Build a position-dependent metric field wrapper
    auto metricPtr = baseMetric_;
    GeodesicIntegrator integrator(
        config_.tolerance,
        config_.minStepSize,
        config_.maxStepSize,
        config_.safetyFactor,
        config_.maxIterations
    );

    // Wrap evaluate() which returns std::array<std::array<double,4>,4>
    // into a function returning MetricTensor
    integrator.setMetricField(
        [metricPtr](const Event4D& evt) -> MetricTensor {
            auto g = metricPtr->evaluate(evt);
            MetricTensor mt;
            mt.g = g;
            return mt;
        }
    );

    auto initialMetric = metricPtr->evaluate(initialEvent);
    MetricTensor initialMt;
    initialMt.g = initialMetric;
    integrator.setMetric(
        std::make_shared<MetricTensor>(initialMt)
    );

    auto trajectory = integrator.integrate(
        initialEvent, initialVelocity, geodesicType,
        targetProperTime, true
    );

    if (storeTrajectory) {
        lastTrajectory_ = trajectory;
        trajectoryCached_ = true;
    }

    return trajectory;
}

std::vector<double> DifferentiableSimulator::extractObservables(
    const std::vector<GeodesicStep>& trajectory
) const
{
    if (trajectory.empty()) {
        return {};
    }

    const auto& finalStep = trajectory.back();

    switch (config_.observableMode) {
        case ObservableMode::FINAL_POSITION:
            return {
                finalStep.event.t,
                finalStep.event.x,
                finalStep.event.y,
                finalStep.event.z
            };

        case ObservableMode::FINAL_VELOCITY: {
            if (trajectory.size() < 2) {
                return {0.0, 0.0, 0.0, 0.0};
            }
            const auto& prevStep = trajectory[trajectory.size() - 2];
            double dt = finalStep.properTime - prevStep.properTime;
            if (dt < 1e-15) return {0.0, 0.0, 0.0, 0.0};
            return {
                (finalStep.event.t - prevStep.event.t) / dt,
                (finalStep.event.x - prevStep.event.x) / dt,
                (finalStep.event.y - prevStep.event.y) / dt,
                (finalStep.event.z - prevStep.event.z) / dt
            };
        }

        case ObservableMode::FINAL_STATE: {
            std::vector<double> obs;
            obs.reserve(8);
            obs.push_back(finalStep.event.t);
            obs.push_back(finalStep.event.x);
            obs.push_back(finalStep.event.y);
            obs.push_back(finalStep.event.z);

            if (trajectory.size() >= 2) {
                const auto& prevStep = trajectory[trajectory.size() - 2];
                double dt = finalStep.properTime - prevStep.properTime;
                if (dt > 1e-15) {
                    obs.push_back((finalStep.event.t - prevStep.event.t) / dt);
                    obs.push_back((finalStep.event.x - prevStep.event.x) / dt);
                    obs.push_back((finalStep.event.y - prevStep.event.y) / dt);
                    obs.push_back((finalStep.event.z - prevStep.event.z) / dt);
                } else {
                    obs.insert(obs.end(), 4, 0.0);
                }
            } else {
                obs.insert(obs.end(), 4, 0.0);
            }
            return obs;
        }

        case ObservableMode::PROPER_TIME:
            return {finalStep.properTime};

        case ObservableMode::CUSTOM:
            if (config_.customObservable) {
                return config_.customObservable(trajectory);
            }
            return {};

        default:
            return {};
    }
}

std::vector<std::vector<double>>
DifferentiableSimulator::computeJacobian(
    const Event4D& initialEvent,
    const std::array<double, 4>& initialVelocity,
    GeodesicType geodesicType,
    double targetProperTime
) const
{
    const size_t numParams = getNumParameters();
    if (numParams == 0) {
        return {};
    }

    // Get base trajectory and observables
    auto baseTrajectory = simulateFull(
        initialEvent, initialVelocity, geodesicType,
        targetProperTime, false
    );
    auto baseObservables = extractObservables(baseTrajectory);
    const size_t numObservables = baseObservables.size();

    // Jacobian matrix: numObservables × numParameters
    std::vector<std::vector<double>> jacobian(
        numObservables,
        std::vector<double>(numParams, 0.0)
    );

    const double eps = config_.paramEpsilon;

    // Save original params
    std::vector<double> originalParams = currentParams_;

    // Central finite differences for each parameter
    for (size_t p = 0; p < numParams; ++p) {
        // Perturb +eps
        std::vector<double> paramsPlus = currentParams_;
        paramsPlus[p] += eps;
        currentParams_ = paramsPlus;
        rebuildMetric_();

        GeodesicIntegrator integratorPlus(
            config_.tolerance, config_.minStepSize,
            config_.maxStepSize, config_.safetyFactor,
            config_.maxIterations
        );
        auto metricPtrPlus = baseMetric_;
        integratorPlus.setMetricField(
            [metricPtrPlus](const Event4D& evt) -> MetricTensor {
                auto g = metricPtrPlus->evaluate(evt);
                MetricTensor mt;
                mt.g = g;
                return mt;
            }
        );
        auto initMetricPlus = metricPtrPlus->evaluate(initialEvent);
        MetricTensor initMtPlus;
        initMtPlus.g = initMetricPlus;
        integratorPlus.setMetric(
            std::make_shared<MetricTensor>(initMtPlus)
        );

        auto trajPlus = integratorPlus.integrate(
            initialEvent, initialVelocity, geodesicType,
            targetProperTime, true
        );
        auto obsPlus = extractObservables(trajPlus);

        // Perturb -eps
        std::vector<double> paramsMinus = originalParams;
        paramsMinus[p] -= eps;
        currentParams_ = paramsMinus;
        rebuildMetric_();

        GeodesicIntegrator integratorMinus(
            config_.tolerance, config_.minStepSize,
            config_.maxStepSize, config_.safetyFactor,
            config_.maxIterations
        );
        auto metricPtrMinus = baseMetric_;
        integratorMinus.setMetricField(
            [metricPtrMinus](const Event4D& evt) -> MetricTensor {
                auto g = metricPtrMinus->evaluate(evt);
                MetricTensor mt;
                mt.g = g;
                return mt;
            }
        );
        auto initMetricMinus = metricPtrMinus->evaluate(initialEvent);
        MetricTensor initMtMinus;
        initMtMinus.g = initMetricMinus;
        integratorMinus.setMetric(
            std::make_shared<MetricTensor>(initMtMinus)
        );

        auto trajMinus = integratorMinus.integrate(
            initialEvent, initialVelocity, geodesicType,
            targetProperTime, true
        );
        auto obsMinus = extractObservables(trajMinus);

        // Compute central difference gradient
        for (size_t o = 0; o < numObservables; ++o) {
            double diff = 0.0;
            if (o < obsPlus.size() && o < obsMinus.size()) {
                diff = obsPlus[o] - obsMinus[o];
            }
            jacobian[o][p] = diff / (2.0 * eps);
        }
    }

    // Restore original parameters
    currentParams_ = originalParams;
    rebuildMetric_();

    return jacobian;
}

std::pair<std::vector<double>, std::vector<std::vector<double>>>
DifferentiableSimulator::simulateWithJacobian(
    const Event4D& initialEvent,
    const std::array<double, 4>& initialVelocity,
    GeodesicType geodesicType,
    double targetProperTime
) const
{
    auto baseTrajectory = simulateFull(
        initialEvent, initialVelocity, geodesicType,
        targetProperTime, false
    );
    auto observables = extractObservables(baseTrajectory);
    auto jacobian = computeJacobian(
        initialEvent, initialVelocity, geodesicType, targetProperTime
    );
    return {observables, jacobian};
}

void DifferentiableSimulator::setParameters(const std::vector<double>& params)
{
    currentParams_ = params;
    baseMetric_ = nullptr;
    trajectoryCached_ = false;
}

std::vector<double> DifferentiableSimulator::getParameters() const
{
    return currentParams_;
}

size_t DifferentiableSimulator::getNumParameters() const noexcept
{
    return currentParams_.size();
}

std::vector<std::string> DifferentiableSimulator::getParameterNames() const
{
    return paramNames_;
}

double DifferentiableSimulator::verifyGradient(
    const Event4D& initialEvent,
    const std::array<double, 4>& initialVelocity,
    GeodesicType geodesicType,
    double targetProperTime,
    double epsilon
) const
{
    const size_t numParams = getNumParameters();
    if (numParams == 0) return 0.0;

    // Get Jacobian using the configured paramEpsilon
    auto jacobian = computeJacobian(
        initialEvent, initialVelocity, geodesicType, targetProperTime
    );

    double maxRelError = 0.0;
    std::vector<double> originalParams = currentParams_;

    for (size_t p = 0; p < numParams; ++p) {
        // Perturb +epsilon
        std::vector<double> paramsP = originalParams;
        paramsP[p] += epsilon;
        currentParams_ = paramsP;
        rebuildMetric_();

        GeodesicIntegrator integratorP(
            config_.tolerance, config_.minStepSize,
            config_.maxStepSize, config_.safetyFactor,
            config_.maxIterations
        );
        auto metricPtrP = baseMetric_;
        integratorP.setMetricField(
            [metricPtrP](const Event4D& evt) -> MetricTensor {
                auto g = metricPtrP->evaluate(evt);
                MetricTensor mt;
                mt.g = g;
                return mt;
            }
        );
        auto initMetricP = metricPtrP->evaluate(initialEvent);
        MetricTensor initMtP;
        initMtP.g = initMetricP;
        integratorP.setMetric(
            std::make_shared<MetricTensor>(initMtP)
        );

        auto trajP = integratorP.integrate(
            initialEvent, initialVelocity, geodesicType,
            targetProperTime, true
        );
        auto obsP = extractObservables(trajP);

        // Base observables at nominal parameters
        currentParams_ = originalParams;
        rebuildMetric_();
        auto baseTraj = simulateFull(
            initialEvent, initialVelocity, geodesicType,
            targetProperTime, false
        );
        auto baseObs = extractObservables(baseTraj);

        for (size_t o = 0; o < baseObs.size() && o < obsP.size(); ++o) {
            double numericalGrad = (obsP[o] - baseObs[o]) / epsilon;
            double analyticalGrad = jacobian[o][p];

            double denom = std::max(
                std::abs(analyticalGrad),
                std::abs(numericalGrad)
            );
            if (denom > 1e-15) {
                double relError = std::abs(analyticalGrad - numericalGrad) / denom;
                maxRelError = std::max(maxRelError, relError);
            }
        }
    }

    // Restore original parameters
    currentParams_ = originalParams;
    rebuildMetric_();

    return maxRelError;
}

std::string DifferentiableSimulator::getMetricName() const
{
    // Return the metric type name based on parameter names (even if baseMetric_ is null)
    if (!paramNames_.empty()) {
        if (paramNames_.size() == 1 && paramNames_[0] == "M") {
            return "DifferentiableSchwarzschild";
        } else if (paramNames_.size() == 2 && paramNames_[0] == "M" && paramNames_[1] == "a") {
            return "DifferentiableKerr";
        } else if (paramNames_.size() == 2 && paramNames_[0] == "M" && paramNames_[1] == "Q") {
            return "DifferentiableReissnerNordstrom";
        }
        // Generic name for other parameter combinations
        std::string name = "Differentiable";
        for (const auto& pn : paramNames_) {
            name += pn;
        }
        return name;
    }
    return "custom";
}

bool DifferentiableSimulator::isValid() const noexcept
{
    return metricFactory_ != nullptr;
}

// ============================================================================
// Factory Function
// ============================================================================

std::unique_ptr<DifferentiableSimulator> createDifferentiableSimulator(
    const std::string& metricType,
    const std::vector<double>& params,
    const SimulatorConfig& config
)
{
    MetricFactory factory;
    std::vector<std::string> paramNames;

    if (metricType == "schwarzschild") {
        paramNames = {"M"};
        factory = [](const std::vector<double>& p) -> std::shared_ptr<MetricTensor> {
            double M = p.empty() ? 1.0 : p[0];
            auto metric = std::make_shared<MetricTensor>(
                MetricTensor::schwarzschild(M, 10.0, M_PI / 2.0, 0.0)
            );
            return metric;
        };
    }
    else if (metricType == "kerr") {
        paramNames = {"M", "a"};
        factory = [](const std::vector<double>& p) -> std::shared_ptr<MetricTensor> {
            double M = p.size() > 0 ? p[0] : 1.0;
            double a = p.size() > 1 ? p[1] : 0.0;
            return std::make_shared<MetricTensor>(
                MetricTensor::kerr(M, a, 10.0, M_PI / 2.0)
            );
        };
    }
    else if (metricType == "reissner-nordstrom") {
        paramNames = {"M", "Q"};
        factory = [](const std::vector<double>& p) -> std::shared_ptr<MetricTensor> {
            double M = p.size() > 0 ? p[0] : 1.0;
            double Q = p.size() > 1 ? p[1] : 0.0;
            double rs = 2.0 * M;
            double rq2 = Q * Q;
            double r = 10.0;
            double f = 1.0 - rs / r + rq2 / (r * r);
            std::array<double, 4> diag = {-f, 1.0 / f, r * r, r * r};
            return std::make_shared<MetricTensor>(MetricTensor::diagonal(diag));
        };
    }
    else {
        throw std::invalid_argument(
            "createDifferentiableSimulator: unknown metric type: " + metricType
        );
    }

    auto sim = std::make_unique<DifferentiableSimulator>(
        factory, paramNames, config
    );
    sim->setParameters(params);
    return sim;
}

} // namespace quantumverse