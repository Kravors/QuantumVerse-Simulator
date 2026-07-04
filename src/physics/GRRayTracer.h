/**
 * @file GRRayTracer.h
 * @brief General Relativistic ray tracer for black hole and wormhole visualization
 *
 * Integrates GPU-accelerated geodesic ray tracing from 20k/geodesic_raytracing.
 * Supports Schwarzschild, Kerr, and wormhole metrics.
 *
 * @note Part of Phase 3 integration (Weeks 15-20)
 */

#pragma once

#include <vector>
#include <memory>
#include <array>
#include <functional>
#include "../spacetime/MetricTensor.h"

namespace quantumverse {

// Forward declaration
class MetricTensor;

/**
 * @brief Ray tracing result
 */
struct RayTraceResult {
    std::vector<Event4D> path;     ///< Geodesic path points
    std::vector<float> colors;     ///< Color values along path
    bool hitEventHorizon;          ///< Whether ray hit event horizon
    bool hitSingularity;           ///< Whether ray hit singularity
    double finalTime;              ///< Final proper time
};

/**
 * @brief Metric type enumeration
 */
enum class MetricType {
    SCHWARZSCHILD,  ///< Non-rotating black hole
    KERR,           ///< Rotating black hole
    WORMHOLE,       ///< Traversable wormhole (Ellis/Morris-Thorne)
    CUSTOM          ///< User-defined metric
};

/**
 * @brief General Relativistic ray tracer
 *
 * Implements geodesic ray tracing with support for:
 * - Schwarzschild metric (static black hole)
 * - Kerr metric (rotating black hole)
 * - Wormhole metrics (Ellis/Morris-Thorne)
 *
 * Uses 2nd order Verlet integration with adaptive step size.
 */
class GRRayTracer {
public:
    /**
     * @brief Construct a new GRRayTracer
     * @param metricType Type of metric to use
     */
    explicit GRRayTracer(MetricType metricType = MetricType::SCHWARZSCHILD);

    /**
     * @brief Set the metric type
     */
    void setMetricType(MetricType type) { m_metricType = type; }

    /**
     * @brief Set custom metric function
     * @param field Function returning metric at any spacetime point
     */
    void setCustomMetric(std::function<MetricTensor(const Event4D&)> field) {
        m_customMetric = field;
    }

    /**
     * @brief Set metric parameters
     * @param mass Mass parameter (M in geometric units)
     * @param spin Spin parameter (a in geometric units, for Kerr)
     */
    void setParameters(double mass, double spin = 0.0) {
        m_mass = mass;
        m_spin = spin;
    }

    /**
     * @brief Trace a geodesic ray
     * @param startEvent Starting spacetime event
     * @param initialDirection Initial direction (normalized)
     * @param maxDistance Maximum distance to trace
     * @return Ray trace result
     */
    RayTraceResult traceRay(const Event4D& startEvent,
                          const std::array<double, 3>& initialDirection,
                          double maxDistance = 1000.0);

    /**
     * @brief Get the current metric at a point
     */
    MetricTensor getMetric(const Event4D& event) const;

    /**
     * @brief Set integration step size
     */
    void setStepSize(double step) { m_stepSize = step; }

    /**
     * @brief Enable/disable adaptive stepping
     */
    void setAdaptive(bool adaptive) { m_adaptive = adaptive; }

private:
    /**
     * @brief Compute Christoffel symbols for current metric
     */
    void computeChristoffel(const Event4D& position) const;

    /**
     * @brief Get Christoffel symbol value
     */
    double getChristoffel(int lambda, int mu, int nu) const;

    /**
     * @brief Verlet integration step
     */
    std::array<double, 4> verletStep(
        const std::array<double, 4>& position,
        const std::array<double, 4>& momentum,
        double step) const;

    MetricType m_metricType;
    double m_mass;
    double m_spin;
    double m_stepSize;
    bool m_adaptive;

    // Custom metric function
    std::function<MetricTensor(const Event4D&)> m_customMetric;

    // Christoffel cache
    mutable std::array<std::array<std::array<double, 4>, 4>, 4> m_christoffel;
    mutable bool m_cacheValid;
    mutable Event4D m_lastPosition;
};

} // namespace quantumverse