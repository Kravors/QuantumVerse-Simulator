#ifndef QUANTUMVERSE_DISCOVERY_ENGINE_H
#define QUANTUMVERSE_DISCOVERY_ENGINE_H

#include <vector>
#include <memory>
#include <string>
#include <map>
#include <functional>
#include "spacetime/Event4D.h"
#include "spacetime/MetricTensor.h"
#include "physics/GeodesicIntegrator.h"
#include "ml/CurvatureNormalizingFlow.h"

namespace quantumverse {

// Forward declarations for quantum gravity namespaces
namespace quantumgravity {
    class SimplicialManifold;
    class GFTField;
}

// Forward declarations for LQG types (defined in quantumverse namespace)
class SpinNetwork;
class SpinFoam;
class CausalSet;

/**
 * TheoryPlugin - Interface for alternative gravity theories
 * Base class for all gravity theory plugins (GR, f(R), Brans-Dicke, LQG, etc.)
 */
class TheoryPlugin {
public:
    virtual ~TheoryPlugin() = default;
    
    virtual std::string getName() const = 0;
    virtual std::string getDescription() const = 0;
    virtual std::string getFieldEquation() const = 0;
    
    // Compute metric for given conditions
    virtual MetricTensor computeMetric(
        const Event4D& location,
        const std::map<std::string, double>& parameters
    ) const = 0;
    
    // Compute Christoffel symbols
    virtual std::array<std::array<double, 4>, 4> computeChristoffel(
        const Event4D& location,
        int rho, int mu, int nu
    ) const = 0;
    
    // Compute curvature tensors
    virtual MetricTensor computeRicciTensor(
        const Event4D& location
    ) const = 0;
    
    virtual double computeRicciScalar(
        const Event4D& location
    ) const = 0;
    
    virtual double computeKretschmannScalar(
        const Event4D& location
    ) const = 0;
    
    // Check if theory predicts new phenomena
    virtual bool predictsWormholes() const = 0;
    virtual bool predictsNakedSingularities() const = 0;
    virtual bool violatesEnergyConditions() const = 0;
    virtual bool allowsTimeTravel() const = 0;
    
    // Parameter ranges
    virtual std::map<std::string, std::pair<double, double>> getParameterRanges() const = 0;
    
    // Clone
    virtual std::unique_ptr<TheoryPlugin> clone() const = 0;
};

/**
 * QuantumGravityPlugin - Base class for quantum gravity theories
 * Extends TheoryPlugin with quantum-specific functionality
 */
class QuantumGravityPlugin : public TheoryPlugin {
public:
    enum QuantumGeometryType {
        QUANTUM_CDT,           // Causal Dynamical Triangulations
        QUANTUM_SPIN_FOAM,     // Loop Quantum Gravity (EPRL/FK)
        QUANTUM_GFT,           // Group Field Theory
        QUANTUM_CAUSAL_SET,    // Causal Set Theory
        QUANTUM_REGULAR_BH     // Regular/Quantum-corrected black holes
    };

    virtual ~QuantumGravityPlugin() = default;

    // Quantum geometry type
    virtual QuantumGeometryType getQuantumGeometryType() const = 0;

    // Planck-scale effects description
    virtual std::string getPlanckScaleEffects() const = 0;

    // Discreteness flag
    virtual bool isDiscrete() const = 0;

    // Path integral amplitude (for quantum theories)
    virtual double computeAmplitude(
        const Event4D& from,
        const Event4D& to
    ) const = 0;

    // Spectral dimension at scale μ
    virtual double computeSpectralDimension(double mu) const = 0;

    // Hausdorff dimension
    virtual double computeHausdorffDimension() const = 0;

    // Quantum geometry visualization data
    virtual std::vector<double> getGeometryVertices() const = 0;
    virtual std::vector<int> getGeometryEdges() const = 0;
    virtual std::vector<double> getGeometryColors() const = 0;
};

/**
 * DiscoveryResult - Structure for anomaly/discovery output
 */
struct DiscoveryResult {
    std::string id;
    std::string type;
    std::string description;
    double confidence = 0.0;
    Event4D location;
    std::map<std::string, double> parameters;
    std::string fieldEquation;
    double timestamp = 0.0;
    bool validated = false;
};

/**
 * Hypothesis - Structure for proposed new physics
 */
struct Hypothesis {
    std::string id;
    std::string name;
    std::string description;
    std::string fieldEquation;
    std::map<std::string, double> parameters;
    std::vector<double> predictions;
    std::vector<double> observations;
    double confidence = 0.0;
    std::string status = "proposed";
};

/**
 * DiscoveryEngine - AI-driven autonomous discovery system
 * 
 * Implements symbolic regression, anomaly detection, and hypothesis
 * lifecycle management for discovering new physics beyond GR.
 */
class DiscoveryEngine {
private:
    std::vector<DiscoveryResult> discoveries;
    std::vector<Hypothesis> hypotheses;
    
    // AI/ML model handles
    bool symbolicRegressionEnabled;
    bool anomalyDetectionEnabled;
    bool hypothesisTestingEnabled;
    
    // Validation thresholds
    double minConfidenceThreshold;
    double maxAnomalyScore;
    
    // Callbacks for discovery notifications
    std::vector<std::function<void(const DiscoveryResult&)>> discoveryCallbacks;
    
    // Task 2.5: Curvature Normalizing Flow for anomaly detection
    std::unique_ptr<CurvatureNormalizingFlow> curvatureFlow_;
    std::string curvatureModelPath_;
    double flowAnomalyThreshold_;  // e.g., 50.0 = anomalous
    
    public:
     // Helper: Compute 15 curvature features from metric at location
     std::vector<double> computeCurvatureFeatures(
         const Event4D& location,
         const MetricTensor& metric
     ) const;
    DiscoveryEngine();
    
    // Enable/disable AI subsystems
    void enableSymbolicRegression(bool enable);
    void enableAnomalyDetection(bool enable);
    void enableHypothesisTesting(bool enable);
    
    // Symbolic regression for field equation generation
    std::string generateFieldEquation(
        const std::vector<Event4D>& trajectory,
        const MetricTensor& metric,
        double tolerance = 1e-6
    );
    
    // Anomaly detection in curvature evolution
    DiscoveryResult detectAnomaly(
        const Event4D& location,
        const MetricTensor& metric,
        const std::vector<Event4D>& geodesicTrajectory
    );
    
    // Classify anomaly type
    std::string classifyAnomaly(const DiscoveryResult& result);
    
    // Hypothesis lifecycle management
    std::string proposeHypothesis(
        const std::string& name,
        const std::string& fieldEquation,
        const std::map<std::string, double>& parameters
    );
    
    bool testHypothesis(
        const std::string& hypothesisId,
        const std::vector<Event4D>& testTrajectories
    );
    
    bool validateHypothesis(const std::string& hypothesisId);
    bool refuteHypothesis(const std::string& hypothesisId);
    
    // Discovery queries
    const std::vector<DiscoveryResult>& getDiscoveries() const;
    std::vector<DiscoveryResult> getDiscoveriesByType(const std::string& type) const;
    const std::vector<Hypothesis>& getHypotheses() const;
    std::vector<Hypothesis> getActiveHypotheses() const;
    
    // Validation against known physics
    bool validateAgainstGR(const DiscoveryResult& result);
    bool validateAgainstMercuryPrecession(const MetricTensor& metric);
    bool validateAgainstLightDeflection(const MetricTensor& metric);
    bool validateAgainstFrameDragging(const MetricTensor& metric);
    
    // Statistical analysis
    double calculateAnomalyScore(
        const Event4D& location,
        const MetricTensor& metric
    );
    
    double calculateDeviationFromGR(
        const MetricTensor& metric,
        const Event4D& location
    );
    
    // Bayesian model comparison
    double calculateBayesFactor(
        const Hypothesis& h1,
        const Hypothesis& h2,
        const std::vector<Event4D>& data
    );
    
    // Callback registration
    void registerDiscoveryCallback(
        std::function<void(const DiscoveryResult&)> callback
    );
    
    // Export/import
    std::string exportDiscovery(const DiscoveryResult& result) const;
    std::string exportHypothesis(const Hypothesis& hypothesis) const;
    
    // Auto-generated reports
    std::string generateDiscoveryReport(const DiscoveryResult& result) const;
    std::string generateHypothesisReport(const Hypothesis& hypothesis) const;
    std::string generateLaTeXPaper(const DiscoveryResult& result) const;
    
    // Reproducibility
    void saveState(const std::string& filename) const;
    void loadState(const std::string& filename);
    
    // Clear
    void clearDiscoveries();
    void clearHypotheses();
    
    // Statistics
    size_t getDiscoveryCount() const;
    size_t getHypothesisCount() const;
    double getAverageConfidence() const;
    
    // Task 2.5: Curvature flow integration
    /**
     * Load the curvature normalizing flow model for anomaly detection.
     * Call this once during initialization to enable flow-based scoring.
     * @param modelPath Path to ONNX model file (or metadata JSON for stub)
     * @return true if model loaded successfully, false otherwise
     */
    bool loadCurvatureModel(const std::string& modelPath);
    
    /**
     * Check if curvature flow model is loaded and ready for inference.
     */
    bool isCurvatureModelLoaded() const;
};

/**
 * Built-in theory plugins (inherit from TheoryPlugin)
 */

class FRLGravityPlugin : public TheoryPlugin {
    // f(R) gravity with chameleon screening
private:
    double alpha;  // Coupling constant
    double n;      // Power law index
    
public:
    FRLGravityPlugin(double alpha = 1.0, double n = 1.0);
    
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
    
    bool predictsWormholes() const override;
    bool predictsNakedSingularities() const override;
    bool violatesEnergyConditions() const override;
    bool allowsTimeTravel() const override;
    
    std::map<std::string, std::pair<double, double>> getParameterRanges() const override;
    
    std::unique_ptr<TheoryPlugin> clone() const override;
};

class BransDickePlugin : public TheoryPlugin {
    // Brans-Dicke scalar-tensor theory
private:
    double omega;  // Brans-Dicke parameter
    double phi0;   // Scalar field background value
    
public:
    BransDickePlugin(double omega = 40000.0, double phi0 = 1.0);
    
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
    
    bool predictsWormholes() const override;
    bool predictsNakedSingularities() const override;
    bool violatesEnergyConditions() const override;
    bool allowsTimeTravel() const override;
    
    std::map<std::string, std::pair<double, double>> getParameterRanges() const override;
    
    std::unique_ptr<TheoryPlugin> clone() const override;
};

class LQGPlugin : public TheoryPlugin {
    // Loop Quantum Gravity - effective polymer-corrected metric
private:
    double gamma;  // Immirzi parameter
    double lambda; // Polymer length scale (Planck length)
    
public:
    LQGPlugin(double gamma = 0.2375, double lambda = 1.616e-35);
    
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
    
    bool predictsWormholes() const override;
    bool predictsNakedSingularities() const override;
    bool violatesEnergyConditions() const override;
    bool allowsTimeTravel() const override;
    
    std::map<std::string, std::pair<double, double>> getParameterRanges() const override;
    
    std::unique_ptr<TheoryPlugin> clone() const override;
};

/**
 * TheoryManager - Manages collection of theory plugins
 */
class TheoryManager {
private:
    std::map<std::string, std::unique_ptr<TheoryPlugin>> plugins;
    std::string activeTheory;
    
public:
    TheoryManager();
    
    // Plugin management
    bool registerPlugin(std::unique_ptr<TheoryPlugin> plugin);
    bool unregisterPlugin(const std::string& name);
    TheoryPlugin* getPlugin(const std::string& name);
    const TheoryPlugin* getPlugin(const std::string& name) const;
    
    // Active theory
    bool setActiveTheory(const std::string& name);
    const std::string& getActiveTheory() const;
    
    // Compute with active theory
    MetricTensor computeMetric(
        const Event4D& location,
        const std::map<std::string, double>& parameters
    ) const;
    
    // List available theories
    std::vector<std::string> getAvailableTheories() const;
    std::string getTheoryDescription(const std::string& name) const;
    
    // Compare theories
    std::map<std::string, double> compareTheories(
        const Event4D& location,
        const std::map<std::string, double>& parameters
    ) const;
    
    // Export theory comparison
    std::string exportComparison(
        const std::vector<std::string>& theoryNames,
        const Event4D& location
    ) const;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_DISCOVERY_ENGINE_H
