/**
 * @file NBodySimulator.h
 * @brief N-body gravitational simulator (Newtonian physics)
 *
 * Integrates physics from Quinta0/gravity and mrshmelloww/3D-Physics-Engine.
 * Provides N-body gravitational force calculation and Euler integration.
 *
 * @note Part of Phase 2 integration (Weeks 9-14)
 */

#pragma once

#include <vector>
#include <memory>
#include <string>
#include <array>

namespace quantumverse {

/**
 * @brief Celestial body for N-body simulation
 */
struct NBody {
    std::string id;         ///< Unique identifier
    double mass;            ///< Mass in kg
    double position[3];     ///< Position in 3D space
    double velocity[3];     ///< Velocity vector
    double acceleration[3]; ///< Acceleration vector
};

/**
 * @brief N-body gravitational simulator
 *
 * Implements Newtonian gravitational force calculation:
 *   F = G * m1 * m2 / r^2
 *
 * And Euler integration for orbital motion.
 */
class NBodySimulator {
public:
    /**
     * @brief Gravitational constant (m^3 kg^-1 s^-2)
     */
    static constexpr double G = 6.67430e-11;

    /**
     * @brief Construct a new NBodySimulator
     */
    NBodySimulator();

    /**
     * @brief Add a celestial body to the simulation
     * @param id Unique identifier
     * @param mass Mass in kg
     * @param position Initial position
     * @param velocity Initial velocity
     */
    void addBody(const std::string& id, double mass,
                 const double position[3], const double velocity[3]);

    /**
     * @brief Remove a body by ID
     * @param id Body identifier
     * @return true if body was found and removed
     */
    bool removeBody(const std::string& id);

    /**
     * @brief Get body by ID
     * @param id Body identifier
     * @return Pointer to body or nullptr if not found
     */
    NBody* getBody(const std::string& id);

    /**
     * @brief Get number of bodies
     */
    size_t bodyCount() const { return m_bodies.size(); }

    /**
     * @brief Set time step for integration
     */
    void setTimeStep(double dt) { m_dt = dt; }

    /**
     * @brief Get time step
     */
    double getTimeStep() const { return m_dt; }

    /**
     * @brief Update simulation by one time step
     */
    void update();

    /**
     * @brief Reset all accelerations to zero
     */
    void resetAccelerations();

    /**
     * @brief Get simulation time
     */
    double getSimulationTime() const { return m_time; }

    /**
     * @brief Set simulation time
     */
    void setSimulationTime(double t) { m_time = t; }

private:
    /**
     * @brief Compute gravitational forces between all bodies
     */
    void computeForces();

    /**
     * @brief Perform Euler integration step
     */
    void integrate();

    std::vector<NBody> m_bodies;
    double m_dt;
    double m_time;
};

} // namespace quantumverse