/**
 * @file SpinFoam.h
 * @brief Spin Foam data structure for Loop Quantum Gravity dynamics
 * 
 * A spin foam is a 2-complex (a collection of faces, edges, vertices)
 * that represents a quantum spacetime history. It is the path integral
 * description of LQG: each foam contributes with an amplitude.
 * 
 * The spin foam model (EPRL/FK) assigns amplitudes to:
 * - Faces (edges in 2-complex): usually (2j+1) factor
 * - Vertices: vertex amplitude from EPRL model (15j symbols or group integrals)
 * 
 * References:
 * - EPRL: Engle, Pereira, Rovelli, Livine (2007)
 * - FK: Freidel, Krasnov (2007)
 * - Perez (2013): "The Spin Foam Approach to Quantum Gravity"
 */

#ifndef QUANTUMVERSE_SPINFOAM_H
#define QUANTUMVERSE_SPINFOAM_H

#include <vector>
#include <memory>
#include <cmath>
#include "SpinNetwork.h"

namespace quantumverse {

/**
 * @class SpinFoam
 * @brief Represents a spin foam (2-complex) with amplitudes
 * 
 * A spin foam consists of:
 * - A set of spin network states at different "times" (boundary networks)
 * - Vertices representing spacetime events (4-valent in 4D)
 * - Faces representing transitions between spin networks
 * 
 * The total amplitude is the product of vertex and face amplitudes.
 */
class SpinFoam {
private:
    std::vector<std::unique_ptr<SpinNetwork>> boundary_slices;  // Initial and final spin networks
    std::vector<std::vector<Spin>> face_spins;  // Spins on faces (edges of 2-complex)
    std::vector<std::array<Spin, 4>> vertex_spins;  // Spins incident to each vertex
    double gamma;  // Immirzi parameter
    
public:
    SpinFoam(double gamma_ = 0.274) : gamma(gamma_) {}
    
    /**
     * @brief Add a boundary spin network slice
     */
    void addBoundarySlice(std::unique_ptr<SpinNetwork> network) {
        boundary_slices.push_back(std::move(network));
    }
    
    /**
     * @brief Set the spins on faces (edges of the 2-complex)
     * Each face connects two vertices or a vertex to a boundary
     */
    void setFaceSpins(const std::vector<std::vector<Spin>>& faces) {
        face_spins = faces;
    }
    
    /**
     * @brief Set vertex spin assignments (4 spins per vertex for 4D)
     */
    void setVertexSpins(const std::vector<std::array<Spin, 4>>& vertices) {
        vertex_spins = vertices;
    }
    
    /**
     * @brief Get the Immirzi parameter
     */
    double getGamma() const { return gamma; }
    
    /**
     * @brief Set the Immirzi parameter
     */
    void setGamma(double g) { gamma = g; }
    
    /**
     * @brief Compute the total amplitude of this spin foam
     * 
     * Z = ∏_f (2j_f + 1) * ∏_v A_v(j_f)
     * 
     * where f runs over faces, v over vertices.
     * The vertex amplitude A_v is from the EPRL model.
     * 
     * @return Total amplitude (real number)
     */
    double computeAmplitude() const {
        double amp = 1.0;
        
        // Face amplitudes: product of (2j+1) for each face spin
        for (const auto& face : face_spins) {
            for (Spin s : face) {
                double j = s / 2.0;
                amp *= (2.0 * j + 1.0);
            }
        }
        
        // Vertex amplitudes (EPRL)
        for (const auto& v_spins : vertex_spins) {
            amp *= computeEPRLVertexAmplitude(v_spins, gamma);
        }
        
        return amp;
    }
    
    /**
     * @brief Compute the amplitude using the 15j symbol formalism
     * 
     * For a closed 4-simplex, the vertex amplitude is proportional
     * to the 15j symbol of the 10 edge spins around the vertex.
     * 
     * @param simplex_spins Array of 10 spins for the 4-simplex edges
     * @return Amplitude contribution
     */
    static double amplitudeFrom15j(const std::array<Spin, 10>& simplex_spins, double gamma) {
        (void)gamma;
        // In EPRL, the vertex amplitude involves a sum over SO(3) intertwiners
        // and ultimately reduces to a 15j symbol in the large spin limit.
        double j15 = compute15jSymbol(simplex_spins);
        
        // There's also a phase factor exp(i * S_Regge) but we take real part
        // For now, return the 15j value directly (simplified)
        return j15;
    }
    
    /**
     * @brief Get number of boundary slices
     */
    int numBoundaries() const { return static_cast<int>(boundary_slices.size()); }
    
    /**
     * @brief Get number of vertices in the foam
     */
    int numVertices() const { return static_cast<int>(vertex_spins.size()); }
    
    /**
     * @brief Get number of faces
     */
    int numFaces() const { return static_cast<int>(face_spins.size()); }
    
    /**
     * @brief Clone this spin foam
     */
    std::unique_ptr<SpinFoam> clone() const {
        auto copy = std::make_unique<SpinFoam>(gamma);
        for (const auto& slice : boundary_slices) {
            copy->addBoundarySlice(slice->clone());
        }
        copy->face_spins = face_spins;
        copy->vertex_spins = vertex_spins;
        return copy;
    }
};

/**
 * @brief Build a simple spin foam from two spin network boundaries
 * 
 * Creates a foam with initial and final spin networks connected
 * by a single layer of vertices (like a single 4-simplex bridging them).
 * 
 * @param initial Initial spin network (boundary)
 * @param final Final spin network (boundary)
 * @param gamma Immirzi parameter
 * @return A SpinFoam object
 */
std::unique_ptr<SpinFoam> buildSimpleFoam(
    const SpinNetwork& initial,
    const SpinNetwork& final,
    double gamma
);

/**
 * @brief Compute the path integral over spin foams with given boundary
 * 
 * Sums over all spin assignments to the foam interior (very expensive).
 * In practice, one uses Monte Carlo or asymptotic methods.
 * 
 * @param boundary The boundary spin network
 * @param max_spin Maximum spin to sum over
 * @param gamma Immirzi parameter
 * @return Approximate path integral (real number)
 */
double pathIntegralOverFoams(const SpinNetwork& boundary, int max_spin, double gamma);

} // namespace quantumverse

#endif // QUANTUMVERSE_SPINFOAM_H