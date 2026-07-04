/**
 * @file NBodySimulator.cpp
 * @brief Implementation of NBodySimulator
 */

#include "NBodySimulator.h"
#include <cmath>
#include <algorithm>

namespace quantumverse {

NBodySimulator::NBodySimulator()
    : m_dt(3600.0)  // 1 hour default time step
    , m_time(0.0)
{
}

void NBodySimulator::addBody(const std::string& id, double mass,
                             const double position[3], const double velocity[3])
{
    NBody body;
    body.id = id;
    body.mass = mass;
    std::copy(position, position + 3, body.position);
    std::copy(velocity, velocity + 3, body.velocity);
    body.acceleration[0] = 0.0;
    body.acceleration[1] = 0.0;
    body.acceleration[2] = 0.0;
    m_bodies.push_back(body);
}

bool NBodySimulator::removeBody(const std::string& id)
{
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(),
        [&id](const NBody& b) { return b.id == id; });
    if (it != m_bodies.end()) {
        m_bodies.erase(it);
        return true;
    }
    return false;
}

NBody* NBodySimulator::getBody(const std::string& id)
{
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(),
        [&id](const NBody& b) { return b.id == id; });
    return (it != m_bodies.end()) ? &(*it) : nullptr;
}

void NBodySimulator::resetAccelerations()
{
    for (auto& body : m_bodies) {
        body.acceleration[0] = 0.0;
        body.acceleration[1] = 0.0;
        body.acceleration[2] = 0.0;
    }
}

void NBodySimulator::computeForces()
{
    resetAccelerations();

    // Compute pairwise gravitational forces
    for (size_t i = 0; i < m_bodies.size(); ++i) {
        for (size_t j = i + 1; j < m_bodies.size(); ++j) {
            // Distance vector from i to j
            double dx = m_bodies[j].position[0] - m_bodies[i].position[0];
            double dy = m_bodies[j].position[1] - m_bodies[i].position[1];
            double dz = m_bodies[j].position[2] - m_bodies[i].position[2];

            // Distance magnitude
            double r2 = dx * dx + dy * dy + dz * dz;
            double r = std::sqrt(r2);

            // Avoid division by zero
            if (r < 1e-10) continue;

            // Gravitational force magnitude: F = G * m1 * m2 / r^2
            double F = G * m_bodies[i].mass * m_bodies[j].mass / r2;

            // Unit vector
            double ux = dx / r;
            double uy = dy / r;
            double uz = dz / r;

            // Acceleration: a = F / m
            double ai = F / m_bodies[i].mass;
            double aj = F / m_bodies[j].mass;

            // Apply accelerations (Newton's third law)
            m_bodies[i].acceleration[0] += aj * ux;
            m_bodies[i].acceleration[1] += aj * uy;
            m_bodies[i].acceleration[2] += aj * uz;

            m_bodies[j].acceleration[0] -= ai * ux;
            m_bodies[j].acceleration[1] -= ai * uy;
            m_bodies[j].acceleration[2] -= ai * uz;
        }
    }
}

void NBodySimulator::integrate()
{
    // Euler integration: v += a * dt, x += v * dt
    for (auto& body : m_bodies) {
        for (int k = 0; k < 3; ++k) {
            body.velocity[k] += body.acceleration[k] * m_dt;
            body.position[k] += body.velocity[k] * m_dt;
        }
    }
}

void NBodySimulator::update()
{
    computeForces();
    integrate();
    m_time += m_dt;
}

} // namespace quantumverse