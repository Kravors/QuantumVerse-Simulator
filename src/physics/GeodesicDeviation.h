// GeodesicDeviation.h
// Non-linear Geodesic Deviation Equation
//
// This module implements the geodesic deviation equation for studying
// tidal forces and geodesic focusing in curved spacetime:
// - Jacobi fields along geodesics
// - Tidal tensor (Riemann tensor projection)
// - Geodesic focusing and conjugate points
// - Raychaudhuri equation
//
// Author: QuantumVerse Team
// Date: 2026-06-07

#pragma once

#include <vector>
#include <array>
#include "../spacetime/Event4D.h"
#include "../spacetime/MetricTensor.h"

namespace quantumverse {

/**
 * @brief Jacobi field (geodesic deviation vector).
 */
class JacobiField {
public:
    Event4D xi;                           ///< Deviation vector
    Event4D dxi_dtau;                     ///< Derivative with respect to proper time
    
    /**
     * @brief Construct a Jacobi field.
     */
    JacobiField() = default;
    
    /**
     * @brief Construct with given values.
     */
    JacobiField(const Event4D& deviation, const Event4D& derivative)
        : xi(deviation), dxi_dtau(derivative) {}
    
    /**
     * @brief Normalize the deviation vector.
     */
    void normalize();
    
    /**
     * @brief Compute magnitude squared.
     */
    double magnitudeSquared() const;
};

/**
 * @brief Geodesic deviation equation solver.
 *
 * The geodesic deviation equation:
 * D²ξ^μ/dτ² = -R^μ_νρσ u^ν u^ρ ξ^σ
 *
 * where u is the tangent vector to the reference geodesic.
 */
class GeodesicDeviation {
public:
    /**
     * @brief Solve geodesic deviation along a geodesic.
     * @param metric The spacetime metric
     * @param initial_position Initial position of reference geodesic
     * @param initial_velocity Initial 4-velocity of reference geodesic
     * @param initial_deviation Initial deviation vector
     * @param initial_derivative Initial derivative of deviation
     * @param tau_max Maximum proper time
     * @param num_steps Number of integration steps
     * @return Vector of Jacobi field values along the geodesic
     */
    static std::vector<JacobiField> solve(
        const MetricTensor& metric,
        const Event4D& initial_position,
        const Event4D& initial_velocity,
        const Event4D& initial_deviation,
        const Event4D& initial_derivative,
        double tau_max,
        int num_steps = 100);
    
    /**
     * @brief Compute tidal tensor (electric part of Weyl tensor).
     * @param metric The spacetime metric
     * @param position Position where to compute tidal tensor
     * @param velocity 4-velocity direction
     * @return 3x3 tidal tensor
     */
    static std::array<std::array<double, 3>, 3> tidalTensor(
        const MetricTensor& metric,
        const Event4D& position,
        const Event4D& velocity);
    
    /**
     * @brief Compute geodesic focusing using Raychaudhuri equation.
     * @param metric The spacetime metric
     * @param initial_separation Initial separation
     * @param initial_expansion Initial expansion scalar
     * @param tau_max Maximum proper time
     * @return Expansion scalar along geodesic
     */
    static double raychaudhuriExpansion(
        const MetricTensor& metric,
        double initial_separation,
        double initial_expansion,
        double tau_max);
    
    /**
     * @brief Find conjugate points along geodesic.
     * @param jacobi_fields Jacobi field values
     * @return Proper time values where conjugate points occur
     */
    static std::vector<double> findConjugatePoints(
        const std::vector<JacobiField>& jacobi_fields);
};

} // namespace quantumverse