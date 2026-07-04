/**
 * @file GRRayTracer.cpp
 * @brief Implementation of GRRayTracer
 */

// Ensure M_PI is defined on all platforms
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "GRRayTracer.h"
#include <cmath>
#include <algorithm>

namespace quantumverse {

GRRayTracer::GRRayTracer(MetricType metricType)
    : m_metricType(metricType)
    , m_mass(1.0)
    , m_spin(0.0)
    , m_stepSize(0.01)
    , m_adaptive(true)
    , m_cacheValid(false)
{
}

MetricTensor GRRayTracer::getMetric(const Event4D& event) const
{
    if (m_metricType == MetricType::CUSTOM && m_customMetric) {
        return m_customMetric(event);
    }

    // Schwarzschild metric in standard coordinates
    // ds^2 = -(1 - 2M/r) dt^2 + (1 - 2M/r)^-1 dr^2 + r^2 (dtheta^2 + sin^2 theta dphi^2)
    if (m_metricType == MetricType::SCHWARZSCHILD) {
        double r = std::sqrt(event.x * event.x + event.y * event.y + event.z * event.z);
        double M = m_mass;

        MetricTensor metric;
        // Lorentz signature (-, +, +, +)
        metric.g[0][0] = -(1.0 - 2.0 * M / r);
        metric.g[1][1] = 1.0 / (1.0 - 2.0 * M / r);
        metric.g[2][2] = r * r;
        metric.g[3][3] = r * r;  // Simplified: assume theta = pi/2

        return metric;
    }

    // Kerr metric (rotating black hole)
    if (m_metricType == MetricType::KERR) {
        double r = std::sqrt(event.x * event.x + event.y * event.y + event.z * event.z);
        double M = m_mass;
        double a = m_spin;

        // Simplified Kerr in equatorial plane
        MetricTensor metric;
        double delta = r * r - 2.0 * M * r + a * a;
        double sigma = r * r + a * a;

        metric.g[0][0] = -(1.0 - 2.0 * M * r / sigma);
        metric.g[1][1] = sigma / delta;
        metric.g[2][2] = sigma;
        metric.g[3][3] = (r * r + a * a + 2.0 * M * r * (r * r + a * a) / sigma) * (M_PI * M_PI / 4.0);

        return metric;
    }

    // Morris-Thorne wormhole metric
    if (m_metricType == MetricType::WORMHOLE) {
        double r = std::sqrt(event.x * event.x + event.y * event.y + event.z * event.z);
        double M = m_mass;

        // Wormhole shape function: b(r) = r0^2 / r
        double r0 = M;  // Throat radius
        double b = r0 * r0 / r;

        MetricTensor metric;
        metric.g[0][0] = -1.0;
        metric.g[1][1] = 1.0;
        metric.g[2][2] = r * r;
        metric.g[3][3] = (r * r - b * r) / (r * r);  // Red shift function

        return metric;
    }

    // Default: flat spacetime
    MetricTensor metric;
    metric.g[0][0] = -1.0;
    metric.g[1][1] = 1.0;
    metric.g[2][2] = 1.0;
    metric.g[3][3] = 1.0;

    return metric;
}

void GRRayTracer::computeChristoffel(const Event4D& position) const
{
    if (m_cacheValid && m_lastPosition.t == position.t &&
        m_lastPosition.x == position.x &&
        m_lastPosition.y == position.y &&
        m_lastPosition.z == position.z) {
        return;
    }

    MetricTensor metric = getMetric(position);
    MetricTensor inv = metric.inverse();

    // Reset cache
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 4; k++)
                m_christoffel[i][j][k] = 0.0;

    // Compute Christoffel symbols numerically
    const double h = 1e-6;
    for (int mu = 0; mu < 4; mu++) {
        Event4D pos_plus = position;
        Event4D pos_minus = position;

        switch (mu) {
            case 0: pos_plus.t += h; pos_minus.t -= h; break;
            case 1: pos_plus.x += h; pos_minus.x -= h; break;
            case 2: pos_plus.y += h; pos_minus.y -= h; break;
            case 3: pos_plus.z += h; pos_minus.z -= h; break;
        }

        MetricTensor g_plus = getMetric(pos_plus);
        MetricTensor g_minus = getMetric(pos_minus);

        for (int lambda = 0; lambda < 4; lambda++) {
            for (int nu = 0; nu < 4; nu++) {
                double sum = 0.0;
                for (int sigma = 0; sigma < 4; sigma++) {
                    double dg_mu = (g_plus.g[sigma][nu] - g_minus.g[sigma][nu]) / (2.0 * h);
                    double dg_nu = (g_plus.g[sigma][mu] - g_minus.g[sigma][mu]) / (2.0 * h);
                    double dg_sigma = (g_plus.g[mu][nu] - g_minus.g[mu][nu]) / (2.0 * h);
                    sum += inv.g[lambda][sigma] * (dg_mu + dg_nu - dg_sigma);
                }
                m_christoffel[lambda][mu][nu] = 0.5 * sum;
            }
        }
    }

    m_cacheValid = true;
    m_lastPosition = position;
}

double GRRayTracer::getChristoffel(int lambda, int mu, int nu) const
{
    if (!m_cacheValid) return 0.0;
    return m_christoffel[lambda][mu][nu];
}

std::array<double, 4> GRRayTracer::verletStep(
    const std::array<double, 4>& position,
    const std::array<double, 4>& momentum,
    double step) const
{
    // Simplified geodesic equation: d^2x/dtau^2 = -Gamma * dx/dtau * dx/dtau
    Event4D pos(position[0], position[1], position[2], position[3]);
    computeChristoffel(pos);

    std::array<double, 4> newPosition = position;
    std::array<double, 4> newMomentum = momentum;

    for (int lambda = 0; lambda < 4; lambda++) {
        double acceleration = 0.0;
        for (int mu = 0; mu < 4; mu++) {
            for (int nu = 0; nu < 4; nu++) {
                acceleration -= m_christoffel[lambda][mu][nu] * momentum[mu] * momentum[nu];
            }
        }
        newMomentum[lambda] += acceleration * step;
        newPosition[lambda] += newMomentum[lambda] * step;
    }

    return newPosition;
}

RayTraceResult GRRayTracer::traceRay(const Event4D& startEvent,
                                     const std::array<double, 3>& initialDirection,
                                     double maxDistance)
{
    RayTraceResult result;
    result.hitEventHorizon = false;
    result.hitSingularity = false;
    result.finalTime = 0.0;

    // Initial conditions
    std::array<double, 4> position = {startEvent.t, startEvent.x, startEvent.y, startEvent.z};
    std::array<double, 4> momentum = {1.0, initialDirection[0], initialDirection[1], initialDirection[2]};

    // Normalize momentum
    double norm = 0.0;
    for (int i = 0; i < 4; i++) {
        norm += momentum[i] * momentum[i];
    }
    norm = std::sqrt(std::abs(norm));
    for (int i = 0; i < 4; i++) {
        momentum[i] /= norm;
    }

    double distance = 0.0;
    double time = 0.0;

    while (distance < maxDistance && !result.hitSingularity) {
        Event4D currentPos(position[1], position[2], position[3], position[0]);
        result.path.push_back(currentPos);

        // Check for event horizon (Schwarzschild: r = 2M)
        double r = std::sqrt(currentPos.x * currentPos.x +
                            currentPos.y * currentPos.y +
                            currentPos.z * currentPos.z);
        if (m_metricType == MetricType::SCHWARZSCHILD && r < 2.0 * m_mass) {
            result.hitEventHorizon = true;
            break;
        }

        // Check for singularity
        if (r < 0.1 * m_mass) {
            result.hitSingularity = true;
            break;
        }

        // Integrate
        std::array<double, 4> newPosition = verletStep(position, momentum, m_stepSize);

        // Update distance
        double dr = std::sqrt(
            (newPosition[1] - position[1]) * (newPosition[1] - position[1]) +
            (newPosition[2] - position[2]) * (newPosition[2] - position[2]) +
            (newPosition[3] - position[3]) * (newPosition[3] - position[3])
        );
        distance += dr;
        time += m_stepSize;

        position = newPosition;
    }

    result.finalTime = time;
    return result;
}

} // namespace quantumverse