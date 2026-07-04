/**
 * @file CausalSet.h
 * @brief Causal Set Dynamics Engine - Quantum gravity via discrete spacetime posets
 *
 * Implements causal set theory where:
 * - Spacetime is a partially ordered set (poset) of elements
 * - Order relation encodes causality: x < y means x is in the past of y
 * - Locally finite: number of elements in any region is proportional to its volume
 * - Continuum emerges via coarse-graining
 * - Dynamics via growth processes (Alexandrov sets, birth/death)
 *
 * References:
 * - Bombelli, Lee, Meyer, Sorkin (1987): "Space-time as a causal set"
 * - Sorkin (2005): "Causal sets: Discrete gravity"
 * - Rideout, Sorkin (1999): "Classical sequential growth dynamics for causal sets"
 */

#ifndef QUANTUMVERSE_CAUSAL_SET_H
#define QUANTUMVERSE_CAUSAL_SET_H

#include "discovery/DiscoveryEngine.h"
#include <vector>
#include <set>
#include <map>
#include <memory>
#include <random>
#include <algorithm>

namespace quantumverse {
namespace quantumgravity {

/**
 * @struct CausalElement
 * @brief A single element in the causal set (an atom of spacetime)
 */
struct CausalElement {
    int id;                 // Unique identifier
    double birth_time;      // "Time" of birth (Alexandrov coordinate)
    std::set<int> past;     // Set of element IDs that are in the past
    std::set<int> future;   // Set of element IDs that are in the future
    double volume;          // Volume assigned to this element (typically Planck volume)

    CausalElement(int id_, double t_) : id(id_), birth_time(t_), volume(1.0) {}
};

/**
 * @class CausalSet
 * @brief Represents a causal set (partially ordered set of spacetime atoms)
 */
class CausalSet {
private:
    std::vector<CausalElement> elements;
    std::map<int, int> id_to_index;  // Fast lookup
    int next_id;

    // Internal: check if relation x < y is transitive (should be)
    bool isTransitive(int x, int y) const;

public:
    CausalSet();

    // Add a new element born at time t, with specified past set
    int addElement(double birth_time, const std::set<int>& past_ids);

    // Check if element x precedes element y (causal order)
    bool precedes(int x, int y) const;

    // Get all elements in the past of element id
    std::set<int> getPast(int id) const;

    // Get all elements in the future of element id
    std::set<int> getFuture(int id) const;

    // Get the number of elements (cardinality)
    size_t size() const { return elements.size(); }

    // Get element by ID
    const CausalElement& getElement(int id) const;

    // Get all element IDs
    std::vector<int> getAllIDs() const;

    // Compute the "distance" (number of elements in interval) between two elements
    int intervalSize(int x, int y) const;

    // Check if the poset is a tree (no cycles, single parent per child)
    bool isTree() const;

    // Compute sprinkling density (elements per unit volume)
    double density() const;

    // Export to adjacency matrix (for visualization/analysis)
    std::vector<std::vector<int>> adjacencyMatrix() const;
};

/**
 * @class CausalSetDynamics
 * @brief Growth dynamics for causal sets (sequential growth model)
 *
 * Implements the classical sequential growth (CSG) dynamics:
 * - At each step, a new element is born
 * - It chooses a random "parent" set from existing elements
 * - Probability depends on parameters α (inclusion) and β (exclusion)
 *
 * References: Rideout & Sorkin (1999, 2000)
 */
class CausalSetDynamics {
private:
    CausalSet& cs;
    std::mt19937 rng;
    std::uniform_real_distribution<double> uniformDist;

    // Dynamics parameters
    double alpha;   // Tendency to include past elements
    double beta;    // Tendency to exclude (anti-correlation)

    // History
    std::vector<double> volume_history;  // Volume vs. "time"
    std::vector<int> element_count_history;

    // Internal: probability of adding new element with given past set
    double birthProbability(const std::set<int>& past, double current_time) const;

public:
    CausalSetDynamics(CausalSet& cs_, double alpha_ = 0.5, double beta_ = 0.1);

    // Perform one growth step: add a new element
    int growStep(double birth_time);

    // Grow the causal set by n elements
    void grow(int n, double max_time);

    // Get the current causal set
    const CausalSet& getCausalSet() const { return cs; }

    // Get volume history
    const std::vector<double>& getVolumeHistory() const { return volume_history; }
    const std::vector<int>& getElementCountHistory() const { return element_count_history; }

    // Set dynamics parameters
    void setAlpha(double a) { alpha = a; }
    void setBeta(double b) { beta = b; }
    double getAlpha() const { return alpha; }
    double getBeta() const { return beta; }
};

/**
 * @class CausalSetEngine
 * @brief Main engine for causal set quantum gravity simulations
 *
 * This class implements the QuantumGravityPlugin interface and provides:
 * - Causal set generation via sequential growth
 * - Effective metric extraction from order relation
 * - Spectral dimension calculation (via random walks)
 * - Continuum limit estimation
 * - Visualization data (Hasse diagram)
 */
class CausalSetEngine : public QuantumGravityPlugin {
private:
    std::unique_ptr<CausalSet> causal_set;
    std::unique_ptr<CausalSetDynamics> dynamics;

    // Parameters
    double alpha;          // Growth parameter α
    double beta;           // Growth parameter β
    double planck_volume;  // Volume per element (l_P³)
    int num_elements;      // Current number of elements

    // Observables
    double spectralDimension;
    double dimensionality;  // Estimated Hausdorff dimension
    double sprinkling_density;

    // Random number generator
    std::mt19937 rng;

    // Internal methods
    double estimateDimensionality() const;  // via box-counting on poset
    double randomWalkSpectralDimension(int num_walks, int steps) const;

public:
    CausalSetEngine(
        double alpha_ = 0.5,
        double beta_ = 0.1,
        double planck_vol_ = 1.0,
        int initial_elements_ = 100
    );

    ~CausalSetEngine() override = default;

    // TheoryPlugin interface
    std::string getName() const override;
    std::string getDescription() const override;
    std::string getFieldEquation() const override;

    MetricTensor computeMetric(
        const Event4D& location,
        const std::map<std::string, double>& parameters
    ) const override;

    std::array<std::array<double, 4>, 4> computeChristoffel(
        const Event4D& location,
        int rho, int mu, int nu
    ) const override;

    MetricTensor computeRicciTensor(const Event4D& location) const override;
    double computeRicciScalar(const Event4D& location) const override;
    double computeKretschmannScalar(const Event4D& location) const override;

    // Phenomenology
    bool predictsWormholes() const override { return false; }
    bool predictsNakedSingularities() const override { return false; }
    bool violatesEnergyConditions() const override { return false; }
    bool allowsTimeTravel() const override { return false; }

    std::map<std::string, std::pair<double, double>> getParameterRanges() const override;
    std::unique_ptr<TheoryPlugin> clone() const override;

    // QuantumGravityPlugin interface
    QuantumGeometryType getQuantumGeometryType() const override { return QUANTUM_CAUSAL_SET; }

    std::string getPlanckScaleEffects() const override;

    bool isDiscrete() const override { return true; }

    double computeAmplitude(
        const Event4D& from,
        const Event4D& to
    ) const override;

    double computeSpectralDimension(double mu) const override;
    double computeHausdorffDimension() const override;

    // Visualization
    std::vector<double> getGeometryVertices() const override;
    std::vector<int> getGeometryEdges() const override;
    std::vector<double> getGeometryColors() const override;

    // Causal set specific operations
    void grow(int n, double max_time);
    void setParameters(double a, double b) { alpha = a; beta = b; if(dynamics) dynamics->setAlpha(a), dynamics->setBeta(b); }
    int getNumElements() const { return num_elements; }
    double getSprinklingDensity() const { return sprinkling_density; }
    const CausalSet& getCausalSet() const { return *causal_set; }

    // Accessor methods for private parameters (needed by tests and UI)
    double getAlpha() const { return alpha; }
    double getBeta() const { return beta; }
    double getPlanckVolume() const { return planck_volume; }
};

/**
 * @class CausalSetPlugin
 * @brief Concrete plugin for DiscoveryEngine registration
 */
class CausalSetPlugin : public CausalSetEngine {
private:
    std::string name;
    std::string description;

public:
    CausalSetPlugin(
        double alpha = 0.5,
        double beta = 0.1,
        double planck_vol = 1.0,
        int initial = 100
    ) : CausalSetEngine(alpha, beta, planck_vol, initial),
        name("Causal Set Theory"),
        description("Quantum gravity via discrete spacetime posets. Spacetime is a causal set where order encodes causality. Emergent continuum via coarse-graining. Predicts finite spacetime discreteness at Planck scale.") {}

    std::string getName() const override { return name; }
    std::string getDescription() const override { return description; }
    std::unique_ptr<TheoryPlugin> clone() const override {
        return std::make_unique<CausalSetPlugin>(getAlpha(), getBeta(), getPlanckVolume(), getNumElements());
    }
};

} // namespace quantumgravity
} // namespace quantumverse

#endif // QUANTUMVERSE_CAUSAL_SET_H