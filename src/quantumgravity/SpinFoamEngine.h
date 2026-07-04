/**
 * @file SpinFoamEngine.h
 * @brief Spin Foam Engine - Loop Quantum Gravity dynamics via EPRL model
 * 
 * Implements the spin foam path integral for 4D quantum gravity.
 * Uses the Engle-Pereira-Rovelli-Livine (EPRL) vertex amplitude.
 * 
 * References:
 * - EPRL: Engle, Pereira, Rovelli, Livine (2007), "LQG vertex with finite Immirzi parameter"
 * - Perez (2013): "The Spin Foam Approach to Quantum Gravity"
 */

#ifndef QUANTUMVERSE_SPINFOAM_ENGINE_H
#define QUANTUMVERSE_SPINFOAM_ENGINE_H

#include "discovery/DiscoveryEngine.h"
#include "SpinNetwork.h"
#include "SpinFoam.h"
#include <vector>
#include <memory>
#include <random>

namespace quantumverse {
namespace quantumgravity {

/**
 * @class SpinFoamEngine
 * @brief Main engine for spin foam quantum gravity simulations
 * 
 * This class implements the QuantumGravityPlugin interface and provides:
 * - Spin foam amplitude computation
 * - Monte Carlo sampling over spin configurations
 * - Effective metric extraction (via coherent state techniques)
 * - Spectral dimension calculation
 * - Visualization data for quantum geometry
 */
class SpinFoamEngine : public QuantumGravityPlugin {
 public:
    double getGamma() const { return gamma; }
    double getLambda() const { return lambda; }
    int getMaxSpin() const { return maxSpin; }
    int getNumSamples() const { return numSamples; }
    void setGamma(double g) { gamma = g; }
    void setLambda(double l) { lambda = l; }
    void setNumSamples(int n) { numSamples = n; }
 private:
    std::unique_ptr<SpinFoam> foam;
    double gamma;           // Immirzi parameter
    double lambda;          // Planck length scale
    int maxSpin;            // Maximum spin to sum over
    int numSamples;         // Monte Carlo samples
    
    // Observables
    double spectralDimension;
    double hausdorffDimension;
    double averageAmplitude;
    std::vector<double> amplitudeHistory;
    
    // Random number generator (mutable for use in const methods)
    mutable std::mt19937 rng;
    std::uniform_int_distribution<Spin> spinDist;
    
    // Internal methods
    SpinNetwork generateRandomBoundary(int numVertices, double avgSpin) const;
    double computeVertexAmplitude(const std::array<Spin, 4>& spins) const;
    double computeFaceAmplitude(Spin spin) const;
    
public:
    SpinFoamEngine(
        double gamma_ = 0.274,
        double lambda_ = 1.616e-35,
        int maxSpin_ = 5,
        int samples_ = 1000
    );
    
    ~SpinFoamEngine() override = default;
    
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
    bool predictsWormholes() const override { return true; }  // LQG allows wormhole solutions
    bool predictsNakedSingularities() const override { return false; }
    bool violatesEnergyConditions() const override { return true; }  // Quantum violations possible
    bool allowsTimeTravel() const override { return false; }
    
    std::map<std::string, std::pair<double, double>> getParameterRanges() const override;
    std::unique_ptr<TheoryPlugin> clone() const override;
    
    // QuantumGravityPlugin interface
    QuantumGeometryType getQuantumGeometryType() const override { return QUANTUM_SPIN_FOAM; }
    
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
    
    // Spin foam specific operations
    void runMonteCarlo(int numSteps);
    void setMaxSpin(int max) { maxSpin = max; spinDist = std::uniform_int_distribution<Spin>(0, max); }
    
    // Statistics
    double getAverageAmplitude() const { return averageAmplitude; }
    const std::vector<double>& getAmplitudeHistory() const { return amplitudeHistory; }
    
    // Compute effective metric from spin foam expectation values
    // Uses the coherent state representation to extract geometry
    MetricTensor computeEffectiveMetric(const Event4D& location) const;
    
    // Compute area operator expectation for a surface
    double computeAreaExpectation(double radius) const;
    
    // Compute volume operator expectation for a region
    double computeVolumeExpectation(double radius) const;
};

/**
 * @class SpinFoamPlugin
 * @brief Concrete plugin for DiscoveryEngine registration
 * 
 * This is the registered plugin class that adds spin foam capabilities
 * to the QuantumVerse discovery system.
 */
class SpinFoamPlugin : public SpinFoamEngine {
 private:
    std::string name;
    std::string description;

 public:
    SpinFoamPlugin(
        double gamma = 0.274,
        double lambda = 1.616e-35,
        int maxSpin = 5,
        int samples = 1000
    ) : SpinFoamEngine(gamma, lambda, maxSpin, samples),
        name("Spin Foam (EPRL)"),
        description("Loop Quantum Gravity via spin foam path integral. Implements the EPRL vertex amplitude with Immirzi parameter gamma = 0.274. Predicts discrete quantum geometry, Big Bounce, and Planck-scale foam structure.") {}

    std::string getName() const override { return name; }
    std::string getDescription() const override { return description; }
    std::unique_ptr<TheoryPlugin> clone() const override {
        return std::make_unique<SpinFoamPlugin>(getGamma(), getLambda(), getMaxSpin(), getNumSamples());
    }
};

} // namespace quantumgravity
} // namespace quantumverse

#endif // QUANTUMVERSE_SPINFOAM_ENGINE_H