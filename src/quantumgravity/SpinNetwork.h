/**
 * @file SpinNetwork.h
 * @brief Spin Network data structure for Loop Quantum Gravity
 * 
 * A spin network is a graph with:
 * - Edges labeled by SU(2) spins (half-integers j = 0, 1/2, 1, ...)
 * - Vertices labeled by intertwiners (gauge-invariant tensors)
 * 
 * Spin networks represent quantum states of 3D geometry in LQG.
 * The spin foam formalism describes dynamics as a superposition of
 * 2-complexes (histories of spin networks).
 * 
 * References:
 * - Perez (2013): "The Spin Foam Approach to Quantum Gravity"
 * - Rovelli (2004): "Quantum Gravity"
 * - EPRL model: Engle, Pereira, Rovelli, Livine (2007)
 */

#ifndef QUANTUMVERSE_SPINNETWORK_H
#define QUANTUMVERSE_SPINNETWORK_H

#include <vector>
#include <array>
#include <memory>
#include <cstdint>
#include <cmath>

// GSL header for Wigner 3j/6j symbols - optional for spin network amplitudes
// If GSL is not available, spin coupling functions fall back to direct computation
#ifdef HAS_GSL
#include <gsl/gsl_sf_coupling.h>
#endif

namespace quantumverse {

/**
 * @brief Spin quantum number (half-integer representation)
 * 
 * Stores spin j as integer = 2*j to avoid floating point.
 * Example: j=0 -> 0, j=1/2 -> 1, j=1 -> 2, j=3/2 -> 3
 */
using Spin = int;

/**
 * @brief Intertwiner index (for multiple intertwiners at a vertex)
 * 
 * In SU(2) theory, an intertwiner is an invariant tensor contracting
 * the representations on incident edges. For a vertex with n edges,
 * the space of intertwiners is equivalent to the space of SU(2)
 * invariant tensors with n indices.
 * 
 * For simplicity, we use an integer index to select a particular
 * intertwiner basis state (e.g., via Clebsch-Gordan decomposition).
 */
using IntertwinerIndex = int;

/**
 * @brief Edge descriptor: connects two vertices with a spin label
 */
struct Edge {
    int vertex1;          // First vertex index
    int vertex2;          // Second vertex index
    Spin spin;            // SU(2) spin label (2*j)
    IntertwinerIndex i1;  // Intertwiner index at vertex1 (for half-edge)
    IntertwinerIndex i2;  // Intertwiner index at vertex2
    
    Edge(int v1, int v2, Spin s, IntertwinerIndex i1_ = 0, IntertwinerIndex i2_ = 0)
        : vertex1(v1), vertex2(v2), spin(s), i1(i1_), i2(i2_) {}
};

/**
 * @brief Vertex with incident edges and intertwiner
 */
struct Vertex {
    std::vector<int> incident_edges;  // Indices into SpinNetwork::edges
    IntertwinerIndex intertwiner;     // Gauge-invariant label
    
    Vertex() : intertwiner(0) {}
};

/**
 * @class SpinNetwork
 * @brief Quantum state of 3D geometry (spin network graph)
 * 
 * Encodes a discrete quantum geometry state according to LQG.
 * Each edge carries a spin j ~ area, each vertex carries an
 * intertwiner i ~ volume.
 * 
 * The network can be:
 * - Closed (no boundary) - represents a closed 3-manifold
 * - Open (with boundary) - represents a region with boundary
 * 
 * The total Hilbert space dimension grows rapidly with spins.
 */
class SpinNetwork {
private:
    std::vector<Vertex> vertices;
    std::vector<Edge> edges;
    
    // Boundary spins (for open networks)
    std::vector<int> boundary_edges;  // Indices of edges with open ends
    
public:
    SpinNetwork() = default;
    
    /**
     * @brief Construct a simple spin network from edge list
     */
    SpinNetwork(const std::vector<Edge>& edges_, const std::vector<Vertex>& vertices_)
        : edges(edges_), vertices(vertices_) {}
    
    // Accessors
    const std::vector<Edge>& getEdges() const { return edges; }
    const std::vector<Vertex>& getVertices() const { return vertices; }
    const std::vector<int>& getBoundaryEdges() const { return boundary_edges; }
    
    int numVertices() const { return static_cast<int>(vertices.size()); }
    int numEdges() const { return static_cast<int>(edges.size()); }
    
    /**
     * @brief Add a vertex with given incident edges
     */
    int addVertex(const std::vector<int>& incident_edges, IntertwinerIndex i = 0) {
        Vertex v;
        v.incident_edges = incident_edges;
        v.intertwiner = i;
        vertices.push_back(v);
        return vertices.size() - 1;
    }
    
    /**
     * @brief Add an edge between two vertices
     */
    int addEdge(int v1, int v2, Spin spin, IntertwinerIndex i1 = 0, IntertwinerIndex i2 = 0) {
        edges.emplace_back(v1, v2, spin, i1, i2);
        vertices[v1].incident_edges.push_back(edges.size() - 1);
        vertices[v2].incident_edges.push_back(edges.size() - 1);
        return edges.size() - 1;
    }
    
    /**
     * @brief Mark an edge as boundary (open end)
     */
    void markBoundary(int edge_index) {
        boundary_edges.push_back(edge_index);
    }
    
    /**
     * @brief Compute total quantum number (sum of spins) - used for normalization
     */
    double totalSpin() const {
        double sum = 0.0;
        for (const auto& e : edges) {
            sum += e.spin / 2.0;  // Convert to actual j
        }
        return sum;
    }
    
    /**
     * @brief Check if network is closed (no boundary)
     */
    bool isClosed() const {
        return boundary_edges.empty();
    }
    
    /**
     * @brief Clone the spin network (deep copy)
     */
    std::unique_ptr<SpinNetwork> clone() const {
        return std::make_unique<SpinNetwork>(edges, vertices);
    }
    
    /**
     * @brief Compute the dimension of the intertwiner space at a vertex
     * 
     * For a vertex with n incident edges of spins j1, ..., jn,
     * the intertwiner space dimension is given by the number of
     * ways to couple the spins to total J=0.
     * 
     * For n=3: dim = 1 if triangle inequality satisfied, else 0
     * For n=4: dim = min(2j1,2j2,2j3,2j4) + 1 (roughly)
     */
    static int intertwinerDimension(const std::vector<Spin>& incident_spins) {
        // Simplified: for 3-valent vertex, dimension is 1 if triangle condition holds
        if (incident_spins.size() == 3) {
            Spin j1 = incident_spins[0];
            Spin j2 = incident_spins[1];
            Spin j3 = incident_spins[2];
            // Triangle inequality: |j1-j2| ≤ j3 ≤ j1+j2, and j1+j2+j3 even
            if (std::abs(j1 - j2) <= j3 && j3 <= j1 + j2 && ((j1 + j2 + j3) % 2 == 0)) {
                return 1;
            }
            return 0;
        }
        // For higher valency, would need to compute CG coefficients
        return 1;  // Placeholder
    }
    
    /**
     * @brief Compute the amplitude of this spin network state
     * 
     * In the kinematical Hilbert space, each spin network state
     * |Γ, j, i⟩ is an orthonormal basis state. Its "amplitude"
     * is just 1 (normalized).
     * 
     * However, in a spin foam (dynamical) context, we assign
     * weights to edges and vertices. This method computes the
     * product of vertex and edge weights for a given state.
     * 
     * @param edge_weight Function: spin -> weight (e.g., (2j+1) for dimension)
     * @param vertex_weight Function: (spins, intertwiner) -> weight
     * @return Total amplitude (real number)
     */
    template<typename EdgeWeight, typename VertexWeight>
    double computeAmplitude(EdgeWeight edge_weight, VertexWeight vertex_weight) const {
        double amp = 1.0;
        
        // Edge contributions
        for (const auto& e : edges) {
            amp *= edge_weight(e.spin);
        }
        
        // Vertex contributions
        for (const auto& v : vertices) {
            std::vector<Spin> incident_spins;
            for (int edge_idx : v.incident_edges) {
                incident_spins.push_back(edges[edge_idx].spin);
            }
            amp *= vertex_weight(incident_spins, v.intertwiner);
        }
        
        return amp;
    }
    
    /**
     * @brief Default amplitude: product of (2j+1) for edges, 1 for vertices
     * (This gives the dimension of the representation space)
     */
    double defaultAmplitude() const {
        return computeAmplitude(
            [](Spin j) { return 2.0 * (j / 2.0) + 1.0; },  // dim = 2j+1
            [](const std::vector<Spin>&, IntertwinerIndex) { return 1.0; }
        );
    }
};

/**
 * @brief Generate a random spin network with given number of vertices
 * 
 * Used for Monte Carlo sampling in spin foam models.
 */
SpinNetwork generateRandomSpinNetwork(int num_vertices, int max_spin, double connectivity);

/**
 * @brief Compute the 15j symbol for a closed spin foam vertex
 * 
 * The 15j symbol is a high-order Wigner symbol that appears in
 * the vertex amplitude of the Lorentzian EPRL spin foam model.
 * It couples 4 tetrahedra (4-valent vertices) in a 4-simplex.
 * 
 * @param spins Array of 10 spins (j_ab for 10 edges of 4-simplex)
 * @return The 15j symbol value (real number)
 */
double compute15jSymbol(const std::array<Spin, 10>& spins);

/**
 * @brief Compute the vertex amplitude for a 4-valent vertex in EPRL model
 * 
 * The EPRL vertex amplitude is:
 * A_v = ∏_e (2j_e + 1) * ∫_{SL(2,C)} dg ∏_e D^{j_e, γ j_e}(g)
 * 
 * where γ is the Immirzi parameter, and the representation is the
 * unitary irreducible representation of SL(2,C) with spin j and
 * "electric" part γj.
 * 
 * In the large-spin limit, this reduces to the Regge action.
 * 
 * @param spins Array of 4 incident edge spins
 * @param gamma Immirzi parameter (typically ~0.274)
 * @return Vertex amplitude (real part)
 */
double computeEPRLVertexAmplitude(const std::array<Spin, 4>& spins, double gamma);

/**
 * @brief Compute the 6j symbol (for simpler SU(2) spin networks)
 * 
 * The 6j symbol appears in the vertex amplitude of the Riemannian
 * spin foam model (e.g., FK model).
 * 
 * @param j1, j2, j3, j4, j5, j6 Six spins coupling in a tetrahedral graph
 * @return The 6j symbol value
 */
double compute6jSymbol(Spin j1, Spin j2, Spin j3, Spin j4, Spin j5, Spin j6);

} // namespace quantumverse

#endif // QUANTUMVERSE_SPINNETWORK_H