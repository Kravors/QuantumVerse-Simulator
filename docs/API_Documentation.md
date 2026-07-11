# QuantumVerse Simulator - API Documentation

## Overview

This document provides detailed API documentation for the QuantumVerse Simulator's core modules, including the discovery engine, quantum gravity engines, and physics components.

**Version**: 0.1.1  
**Last Updated**: 2026-06-27  
**Status**: Production Ready

---

## Table of Contents

1. [Discovery Engine API](#discovery-engine-api)
2. [Discovery Instruments API](#discovery-instruments-api)
3. [Quantum Gravity Engines API](#quantum-gravity-engines-api)
4. [Physics Components API](#physics-components-api)
5. [Math Utilities API](#math-utilities-api)
6. [Build Configuration](#build-configuration)
7. [Migration Notes](#migration-notes)

---

## Discovery Engine API

### TheoryPlugin

Base class interface for all gravity theory plugins.

```cpp
#include "discovery/DiscoveryEngine.h"
```

#### Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `virtual std::string getName() const = 0` | Returns the theory name |
| `getDescription` | `virtual std::string getDescription() const = 0` | Returns human-readable description |
| `getFieldEquation` | `virtual std::string getFieldEquation() const = 0` | Returns symbolic field equation |
| `computeMetric` | `virtual MetricTensor computeMetric(const Event4D& location, const std::map<std::string, double>& parameters) const = 0` | Computes metric tensor at location |
| `computeChristoffel` | `virtual std::array<std::array<double, 4>, 4> computeChristoffel(const Event4D& location, int rho, int mu, int nu) const = 0` | Computes Christoffel symbols |
| `computeRicciTensor` | `virtual MetricTensor computeRicciTensor(const Event4D& location) const = 0` | Computes Ricci tensor |
| `computeRicciScalar` | `virtual double computeRicciScalar(const Event4D& location) const = 0` | Computes Ricci scalar |
| `computeKretschmannScalar` | `virtual double computeKretschmannScalar(const Event4D& location) const = 0` | Computes Kretschmann scalar |
| `predictsWormholes` | `virtual bool predictsWormholes() const = 0` | Whether theory predicts wormholes |
| `predictsNakedSingularities` | `virtual bool predictsNakedSingularities() const = 0` | Whether theory predicts naked singularities |
| `violatesEnergyConditions` | `virtual bool violatesEnergyConditions() const = 0` | Whether theory violates energy conditions |
| `allowsTimeTravel` | `virtual bool allowsTimeTravel() const = 0` | Whether theory allows CTCs |
| `getParameterRanges` | `virtual std::map<std::string, std::pair<double, double>> getParameterRanges() const = 0` | Returns valid parameter ranges |
| `clone` | `virtual std::unique_ptr<TheoryPlugin> clone() const = 0` | Creates a copy of the plugin |

### QuantumGravityPlugin

Extends `TheoryPlugin` with quantum-specific functionality.

```cpp
class QuantumGravityPlugin : public TheoryPlugin
```

#### Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `getQuantumGeometryType` | `virtual QuantumGeometryType getQuantumGeometryType() const = 0` | Returns quantum geometry type |
| `getPlanckScaleEffects` | `virtual std::string getPlanckScaleEffects() const = 0` | Describes Planck-scale effects |
| `isDiscrete` | `virtual bool isDiscrete() const = 0` | Whether spacetime is discrete |
| `computeAmplitude` | `virtual double computeAmplitude(const Event4D& from, const Event4D& to) const = 0` | Path integral amplitude |
| `computeSpectralDimension` | `virtual double computeSpectralDimension(double mu) const = 0` | Spectral dimension at scale μ |
| `computeHausdorffDimension` | `virtual double computeHausdorffDimension() const = 0` | Hausdorff dimension |
| `getGeometryVertices` | `virtual std::vector<double> getGeometryVertices() const = 0` | Visualization vertices |
| `getGeometryEdges` | `virtual std::vector<int> getGeometryEdges() const = 0` | Visualization edges |
| `getGeometryColors` | `virtual std::vector<double> getGeometryColors() const = 0` | Visualization colors |

#### QuantumGeometryType Enum

```cpp
enum QuantumGeometryType {
    QUANTUM_CDT,           // Causal Dynamical Triangulations
    QUANTUM_SPIN_FOAM,     // Loop Quantum Gravity (EPRL/FK)
    QUANTUM_GFT,           // Group Field Theory
    QUANTUM_CAUSAL_SET,    // Causal Set Theory
    QUANTUM_REGULAR_BH     // Regular/Quantum-corrected black holes
};
```

### DiscoveryEngine

Main AI-driven discovery system.

```cpp
#include "discovery/DiscoveryEngine.h"
```

#### Constructor

```cpp
DiscoveryEngine()
```

#### Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `enableSymbolicRegression` | `void enableSymbolicRegression(bool enable)` | Enable/disable symbolic regression |
| `enableAnomalyDetection` | `void enableAnomalyDetection(bool enable)` | Enable/disable anomaly detection |
| `enableHypothesisTesting` | `void enableHypothesisTesting(bool enable)` | Enable/disable hypothesis testing |
| `generateFieldEquation` | `std::string generateFieldEquation(const std::vector<Event4D>& trajectory, const MetricTensor& metric, double tolerance = 1e-6)` | Generate field equation from data |
| `detectAnomaly` | `DiscoveryResult detectAnomaly(const Event4D& location, const MetricTensor& metric, const std::vector<Event4D>& geodesicTrajectory)` | Detect anomalies in curvature |
| `classifyAnomaly` | `std::string classifyAnomaly(const DiscoveryResult& result)` | Classify anomaly type |
| `proposeHypothesis` | `std::string proposeHypothesis(const std::string& name, const std::string& fieldEquation, const std::map<std::string, double>& parameters)` | Propose new hypothesis |
| `testHypothesis` | `bool testHypothesis(const std::string& hypothesisId, const std::vector<Event4D>& testTrajectories)` | Test hypothesis against data |
| `validateHypothesis` | `bool validateHypothesis(const std::string& hypothesisId)` | Validate hypothesis |
| `refuteHypothesis` | `bool refuteHypothesis(const std::string& hypothesisId)` | Refute hypothesis |
| `getDiscoveries` | `std::vector<DiscoveryResult> getDiscoveries() const` | Get all discoveries |
| `getDiscoveriesByType` | `std::vector<DiscoveryResult> getDiscoveriesByType(const std::string& type) const` | Get discoveries by type |
| `getHypotheses` | `std::vector<Hypothesis> getHypotheses() const` | Get all hypotheses |
| `getActiveHypotheses` | `std::vector<Hypothesis> getActiveHypotheses() const` | Get active hypotheses |
| `validateAgainstGR` | `bool validateAgainstGR(const DiscoveryResult& result)` | Validate against GR |
| `validateAgainstMercuryPrecession` | `bool validateAgainstMercuryPrecession(const MetricTensor& metric)` | Validate Mercury precession |
| `validateAgainstLightDeflection` | `bool validateAgainstLightDeflection(const MetricTensor& metric)` | Validate light deflection |
| `validateAgainstFrameDragging` | `bool validateAgainstFrameDragging(const MetricTensor& metric)` | Validate frame dragging |
| `calculateAnomalyScore` | `double calculateAnomalyScore(const Event4D& location, const MetricTensor& metric)` | Calculate anomaly score |
| `calculateDeviationFromGR` | `double calculateDeviationFromGR(const MetricTensor& metric, const Event4D& location)` | Calculate GR deviation |
| `calculateBayesFactor` | `double calculateBayesFactor(const Hypothesis& h1, const Hypothesis& h2, const std::vector<Event4D>& data)` | Bayesian model comparison |
| `registerDiscoveryCallback` | `void registerDiscoveryCallback(std::function<void(const DiscoveryResult&)> callback)` | Register callback |
| `exportDiscovery` | `std::string exportDiscovery(const DiscoveryResult& result) const` | Export discovery to JSON |
| `exportHypothesis` | `std::string exportHypothesis(const Hypothesis& hypothesis) const` | Export hypothesis to JSON |
| `generateDiscoveryReport` | `std::string generateDiscoveryReport(const DiscoveryResult& result) const` | Generate report |
| `generateHypothesisReport` | `std::string generateHypothesisReport(const Hypothesis& hypothesis) const` | Generate hypothesis report |
| `generateLaTeXPaper` | `std::string generateLaTeXPaper(const DiscoveryResult& result) const` | Generate LaTeX paper |
| `saveState` | `void saveState(const std::string& filename) const` | Save state to file |
| `loadState` | `void loadState(const std::string& filename)` | Load state from file |
| `clearDiscoveries` | `void clearDiscoveries()` | Clear all discoveries |
| `clearHypotheses` | `void clearHypotheses()` | Clear all hypotheses |
| `getDiscoveryCount` | `size_t getDiscoveryCount() const` | Get discovery count |
| `getHypothesisCount` | `size_t getHypothesisCount() const` | Get hypothesis count |
| `getAverageConfidence` | `double getAverageConfidence() const` | Get average confidence |
| `loadCurvatureModel` | `bool loadCurvatureModel(const std::string& modelPath)` | Load curvature flow model (Task 2.5) |
| `isCurvatureModelLoaded` | `bool isCurvatureModelLoaded() const` | Check if model loaded |

### TheoryManager

Manages collection of theory plugins.

```cpp
#include "discovery/DiscoveryEngine.h"
```

#### Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `registerPlugin` | `bool registerPlugin(std::unique_ptr<TheoryPlugin> plugin)` | Register a theory plugin |
| `unregisterPlugin` | `bool unregisterPlugin(const std::string& name)` | Unregister a plugin |
| `getPlugin` | `TheoryPlugin* getPlugin(const std::string& name)` | Get plugin by name |
| `setActiveTheory` | `bool setActiveTheory(const std::string& name)` | Set active theory |
| `getActiveTheory` | `std::string getActiveTheory() const` | Get active theory name |
| `computeMetric` | `MetricTensor computeMetric(const Event4D& location, const std::map<std::string, double>& parameters) const` | Compute with active theory |
| `getAvailableTheories` | `std::vector<std::string> getAvailableTheories() const` | List available theories |
| `getTheoryDescription` | `std::string getTheoryDescription(const std::string& name) const` | Get theory description |
| `compareTheories` | `std::map<std::string, double> compareTheories(const Event4D& location, const std::map<std::string, double>& parameters) const` | Compare theories |
| `exportComparison` | `std::string exportComparison(const std::vector<std::string>& theoryNames, const Event4D& location) const` | Export comparison |

---

## Discovery Instruments API

### DiscoveryInstrument

Base class for all discovery instruments.

```cpp
#include "discovery/DiscoveryInstrument.h"
```

#### AlertSeverity Enum

```cpp
enum class AlertSeverity {
    INFO,       // Informational finding
    LOW,        // Low-significance anomaly
    MEDIUM,     // Moderate anomaly (3-5 sigma)
    HIGH,       // High-significance anomaly (5+ sigma)
    CRITICAL    // Potential discovery
};
```

#### InstrumentFinding Struct

```cpp
struct InstrumentFinding {
    std::string id;
    std::string instrumentName;
    AlertSeverity severity;
    double confidence;           // 0.0 to 1.0
    std::string description;
    std::map<std::string, double> parameters;
    Event4D location;
    double timestamp;
};
```

#### Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `virtual std::string getName() const = 0` | Get instrument name |
| `getDescription` | `virtual std::string getDescription() const = 0` | Get description |
| `getCategory` | `virtual std::string getCategory() const = 0` | Get category |
| `analyze` | `virtual std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location, const std::vector<Event4D>& trajectory = {}) = 0` | Run analysis |
| `getParameterRanges` | `virtual std::map<std::string, std::pair<double, double>> getParameterRanges() const = 0` | Get parameter ranges |
| `setParameter` | `virtual void setParameter(const std::string& name, double value) = 0` | Set parameter |
| `getParameters` | `virtual std::map<std::string, double> getParameters() const = 0` | Get all parameters |
| `getParameter` | `virtual double getParameter(const std::string& name) const` | Get specific parameter |
| `getLastFindings` | `const std::vector<InstrumentFinding>& getLastFindings() const` | Get last findings |
| `getTotalFindings` | `size_t getTotalFindings() const` | Get total findings count |
| `isEnabled` | `bool isEnabled() const` | Check if enabled |
| `setEnabled` | `void setEnabled(bool enabled)` | Enable/disable instrument |
| `getDefaultSeverity` | `virtual AlertSeverity getDefaultSeverity() const` | Get default severity |
| `getAlertThreshold` | `AlertSeverity getAlertThreshold() const` | Get alert threshold |
| `setAlertThreshold` | `void setAlertThreshold(AlertSeverity threshold)` | Set alert threshold |

### ExoplanetaryTTVFifthForceHunter

Detects anomalous Transit Timing Variations indicating a fifth force.

```cpp
#include "discovery/ExoplanetaryTTVFifthForceHunter.h"
```

#### Constructor

```cpp
ExoplanetaryTTVFifthForceHunter()
```

#### Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `std::string getName() const override` | Returns "ExoplanetaryTTTVFifthForceHunter" |
| `getDescription` | `std::string getDescription() const override` | Returns description |
| `getCategory` | `std::string getCategory() const override` | Returns "Exoplanetary" |
| `getDefaultSeverity` | `AlertSeverity getDefaultSeverity() const override` | Returns HIGH |
| `analyze` | `std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location, const std::vector<Event4D>& trajectory = {}) override` | Analyze TTV data |

#### Private Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `computeTTVResidual` | `double computeTTVResidual(double observedTransitTime, double predictedTransitTime, double orbitalPeriod, double semiMajorAxis)` | Compute TTV residual |
| `isAnomalousPattern` | `bool isAnomalousPattern(const std::vector<double>& residuals, double threshold)` | Check for anomalous pattern |
| `estimateFifthForceStrength` | `double estimateFifthForceStrength(const std::vector<double>& residuals, const std::vector<double>& distances)` | Estimate fifth force strength |

### BlackHoleJetAnomalyRecogniser

Detects non-Kerr jet launching anomalies in black hole systems.

```cpp
#include "discovery/BlackHoleJetAnomalyRecogniser.h"
```

#### Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `std::string getName() const override` | Returns "BlackHoleJetAnomalyRecogniser" |
| `getDescription` | `std::string getDescription() const override` | Returns description |
| `getCategory` | `std::string getCategory() const override` | Returns "Black Holes" |
| `getDefaultSeverity` | `AlertSeverity getDefaultSeverity() const override` | Returns HIGH |
| `analyze` | `std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location, const std::vector<Event4D>& trajectory = {}) override` | Analyze jet anomalies |

#### Private Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `computeBlandfordZnajekPower` | `double computeBlandfordZnajekPower(double spin, double magneticField, double mass)` | Compute jet power |
| `computeJetCollimationAngle` | `double computeJetCollimationAngle(double spinParameter)` | Compute collimation angle |
| `detectJetPrecession` | `bool detectJetPrecession(const std::vector<double>& jetAngles)` | Detect jet precession |
| `estimateSpinFromJet` | `double estimateSpinFromJet(double jetPower, double mass)` | Estimate spin from jet |

### BosonStarCollisionPredictor

Detects boson star collision signatures in multi-messenger data.

```cpp
#include "discovery/BosonStarCollisionPredictor.h"
```

#### Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `std::string getName() const override` | Returns "BosonStarCollisionPredictor" |
| `getDescription` | `std::string getDescription() const override` | Returns description |
| `getCategory` | `std::string getCategory() const override` | Returns "Compact Objects" |
| `getDefaultSeverity` | `AlertSeverity getDefaultSeverity() const override` | Returns HIGH |
| `analyze` | `std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location, const std::vector<Event4D>& trajectory = {}) override` | Analyze boson star signatures |

#### Private Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `computeTidalDeformability` | `double computeTidalDeformability(double compactness, double bosonMass)` | Compute tidal deformability |
| `estimateBosonMassFromRingdown` | `double estimateBosonMassFromRingdown(double fRing, double fDamp)` | Estimate boson mass |
| `isMassGapObject` | `bool isMassGapObject(double mass1, double mass2)` | Check mass gap |

### NeutronStarGlitchPhaseDetector

Detects neutron star glitch phase transitions via quantum-vortex statistics.

```cpp
#include "discovery/NeutronStarGlitchPhaseDetector.h"
```

#### Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `std::string getName() const override` | Returns "NeutronStarGlitchPhaseDetector" |
| `getDescription` | `std::string getDescription() const override` | Returns description |
| `getCategory` | `std::string getCategory() const override` | Returns "Neutron Stars" |
| `getDefaultSeverity` | `AlertSeverity getDefaultSeverity() const override` | Returns MEDIUM |
| `analyze` | `std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location, const std::vector<Event4D>& trajectory = {}) override` | Analyze glitch signatures |

#### Private Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `computeVortexUnpinningProbability` | `double computeVortexUnpinningProbability(double spinDownRate, double glitchSize)` | Compute unpinning probability |
| `estimateSuperfluidGap` | `double estimateSuperfluidGap(double glitchRecurrenceTime)` | Estimate superfluid gap |
| `detectPhaseTransition` | `bool detectPhaseTransition(const std::vector<double>& glitchSizes)` | Detect phase transition |

### GalacticRotationCurveScanner

Scans for universal residual patterns indicating modified gravity.

```cpp
#include "discovery/GalacticRotationCurveScanner.h"
```

#### Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `std::string getName() const override` | Returns "GalacticRotationCurveScanner" |
| `getDescription` | `std::string getDescription() const override` | Returns description |
| `getCategory` | `std::string getCategory() const override` | Returns "Galactic" |
| `getDefaultSeverity` | `AlertSeverity getDefaultSeverity() const override` | Returns MEDIUM |
| `analyze` | `std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location, const std::vector<Event4D>& trajectory = {}) override` | Analyze rotation curves |

#### Private Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `computeNFWPrediction` | `double computeNFWPrediction(double radius, double virialMass, double concentration)` | Compute NFW dark matter prediction |
| `computeMONDAcceleration` | `double computeMONDAcceleration(double newtonianAccel, double a0)` | Compute MOND acceleration |
| `detectSystematicResidual` | `bool detectSystematicResidual(const std::vector<double>& residuals)` | Detect systematic residuals |

### FineStructureConstantDriftObservatory

Quasar absorption spectrography for alpha-drift detection.

```cpp
#include "discovery/FineStructureConstantDriftObservatory.h"
```

#### Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `std::string getName() const override` | Returns "FineStructureConstantDriftObservatory" |
| `getDescription` | `std::string getDescription() const override` | Returns description |
| `getCategory` | `std::string getCategory() const override` | Returns "Fundamental Constants" |
| `getDefaultSeverity` | `AlertSeverity getDefaultSeverity() const override` | Returns CRITICAL |
| `analyze` | `std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location, const std::vector<Event4D>& trajectory = {}) override` | Analyze alpha variations |

#### Private Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `computeAlphaVariation` | `double computeAlphaVariation(double redshift, double deltaAlphaOverAlpha)` | Compute alpha variation |
| `isDriftSignificant` | `bool isDriftSignificant(const std::vector<double>& alphaMeasurements, const std::vector<double>& redshifts)` | Check if drift is significant |
| `constrainScalarFieldMass` | `double constrainScalarFieldMass(const std::vector<double>& residuals)` | Constrain scalar field mass |

### UltralightDMWaveInterferometer

Dark matter wave detection in the Solar System.

```cpp
#include "discovery/UltralightDMWaveInterferometer.h"
```

#### Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `std::string getName() const override` | Returns "UltralightDMWaveInterferometer" |
| `getDescription` | `std::string getDescription() const override` | Returns description |
| `getCategory` | `std::string getCategory() const override` | Returns "Dark Matter" |
| `getDefaultSeverity` | `AlertSeverity getDefaultSeverity() const override` | Returns HIGH |
| `analyze` | `std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location, const std::vector<Event4D>& trajectory = {}) override` | Analyze DM wave signatures |

#### Private Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `computeDMSignalAmplitude` | `double computeDMSignalAmplitude(double dmMass, double localDensity)` | Compute DM signal amplitude |
| `computeOscillationFrequency` | `double computeOscillationFrequency(double dmMass)` | Compute oscillation frequency |
| `detectCoherentOscillation` | `bool detectCoherentOscillation(const std::vector<double>& residuals, double frequency)` | Detect coherent oscillation |

### PrimordialLithiumCrisisSolver

Exotic BBN nucleosynthesis explorer.

```cpp
#include "discovery/PrimordialLithiumCrisisSolver.h"
```

#### Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `std::string getName() const override` | Returns "PrimordialLithiumCrisisSolver" |
| `getDescription` | `std::string getDescription() const override` | Returns description |
| `getCategory` | `std::string getCategory() const override` | Returns "Cosmology" |
| `getDefaultSeverity` | `AlertSeverity getDefaultSeverity() const override` | Returns MEDIUM |
| `analyze` | `std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location, const std::vector<Event4D>& trajectory = {}) override` | Analyze BBN lithium problem |

#### Private Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `computeLithiumAbundance` | `double computeLithiumAbundance(double baryonToPhoton, double neutronLifetime, double dmCrossSection)` | Compute lithium abundance |
| `computeBBNChiSquared` | `double computeBBNChiSquared(const std::vector<double>& observedAbundances, const std::vector<double>& predictedAbundances)` | Compute BBN chi-squared |
| `isLithiumCrisisResolved` | `bool isLithiumCrisisResolved(double li7OverH, double be7OverH)` | Check if crisis resolved |

### GalacticTidalStreamCartographer

Dark matter subhalo finder via stellar streams.

```cpp
#include "discovery/GalacticTidalStreamCartographer.h"
```

#### Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `std::string getName() const override` | Returns "GalacticTidalStreamCartographer" |
| `getDescription` | `std::string getDescription() const override` | Returns description |
| `getCategory` | `std::string getCategory() const override` | Returns "Galactic" |
| `getDefaultSeverity` | `AlertSeverity getDefaultSeverity() const override` | Returns LOW |
| `analyze` | `std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location, const std::vector<Event4D>& trajectory = {}) override` | Analyze tidal streams |

#### Private Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `computeStreamWidth` | `double computeStreamWidth(double distance, double velocityDispersion)` | Compute stream width |
| `estimateSubhaloMass` | `double estimateSubhaloMass(double streamDeflection, double distance)` | Estimate subhalo mass |
| `detectGapStructure` | `bool detectGapStructure(const std::vector<double>& streamDensities)` | Detect gap structure |

### RecombinationConstantVariationImager

CMB power spectrum archaeologist for recombination constant variation.

```cpp
#include "discovery/RecombinationConstantVariationImager.h"
```

#### Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `std::string getName() const override` | Returns "RecombinationConstantVariationImager" |
| `getDescription` | `std::string getDescription() const override` | Returns description |
| `getCategory` | `std::string getCategory() const override` | Returns "CMB Physics" |
| `getDefaultSeverity` | `AlertSeverity getDefaultSeverity() const override` | Returns CRITICAL |
| `analyze` | `std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location, const std::vector<Event4D>& trajectory = {}) override` | Analyze recombination variations |

#### Private Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `computeRecombinationRedshift` | `double computeRecombinationRedshift(double alphaOverAlpha0, double meffOverMe0)` | Compute recombination redshift |
| `computeDMultipoleShift` | `double computeDMultipoleShift(double deltaZrec, double lPeak)` | Compute multipole shift |
| `detectSpectralDistortion` | `bool detectSpectralDistortion(const std::vector<double>& powerSpectrum, const std::vector<double>& multipoles)` | Detect spectral distortion |
| `constrainConstantVariation` | `double constrainConstantVariation(double chi2, int dof)` | Constrain constant variation |

---

## Quantum Gravity Engines API

### CausalSetEngine

Quantum gravity via discrete spacetime posets.

```cpp
#include "quantumgravity/CausalSet.h"
```

#### Constructor

```cpp
CausalSetEngine(double alpha = 0.5, double beta = 0.1, double planck_vol = 1.0, int initial_elements = 100)
```

#### Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `std::string getName() const override` | Returns "Causal Set Theory" |
| `getDescription` | `std::string getDescription() const override` | Returns description |
| `getFieldEquation` | `std::string getFieldEquation() const override` | Returns field equation |
| `computeMetric` | `MetricTensor computeMetric(const Event4D& location, const std::map<std::string, double>& parameters) const override` | Compute effective metric |
| `computeChristoffel` | `std::array<std::array<double, 4>, 4> computeChristoffel(const Event4D& location, int rho, int mu, int nu) const override` | Compute Christoffel symbols |
| `computeRicciTensor` | `MetricTensor computeRicciTensor(const Event4D& location) const override` | Compute Ricci tensor |
| `computeRicciScalar` | `double computeRicciScalar(const Event4D& location) const override` | Compute Ricci scalar |
| `computeKretschmannScalar` | `double computeKretschmannScalar(const Event4D& location) const override` | Compute Kretschmann scalar |
| `predictsWormholes` | `bool predictsWormholes() const override` | Returns false |
| `predictsNakedSingularities` | `bool predictsNakedSingularities() const override` | Returns false |
| `violatesEnergyConditions` | `bool violatesEnergyConditions() const override` | Returns false |
| `allowsTimeTravel` | `bool allowsTimeTravel() const override` | Returns false |
| `getParameterRanges` | `std::map<std::string, std::pair<double, double>> getParameterRanges() const override` | Get parameter ranges |
| `clone` | `std::unique_ptr<TheoryPlugin> clone() const override` | Clone plugin |
| `getQuantumGeometryType` | `QuantumGeometryType getQuantumGeometryType() const override` | Returns QUANTUM_CAUSAL_SET |
| `getPlanckScaleEffects` | `std::string getPlanckScaleEffects() const override` | Describe Planck effects |
| `isDiscrete` | `bool isDiscrete() const override` | Returns true |
| `computeAmplitude` | `double computeAmplitude(const Event4D& from, const Event4D& to) const override` | Path integral amplitude |
| `computeSpectralDimension` | `double computeSpectralDimension(double mu) const override` | Spectral dimension |
| `computeHausdorffDimension` | `double computeHausdorffDimension() const override` | Hausdorff dimension |
| `getGeometryVertices` | `std::vector<double> getGeometryVertices() const override` | Visualization vertices |
| `getGeometryEdges` | `std::vector<int> getGeometryEdges() const override` | Visualization edges |
| `getGeometryColors` | `std::vector<double> getGeometryColors() const override` | Visualization colors |
| `grow` | `void grow(int n, double max_time)` | Grow causal set |
| `setParameters` | `void setParameters(double a, double b)` | Set growth parameters |
| `getNumElements` | `int getNumElements() const` | Get element count |
| `getSprinklingDensity` | `double getSprinklingDensity() const` | Get density |
| `getCausalSet` | `const CausalSet& getCausalSet() const` | Get causal set |
| `getAlpha` | `double getAlpha() const` | Get alpha parameter |
| `getBeta` | `double getBeta() const` | Get beta parameter |
| `getPlanckVolume` | `double getPlanckVolume() const` | Get Planck volume |

#### CausalSet Class

```cpp
class CausalSet
```

| Method | Signature | Description |
|--------|-----------|-------------|
| `addElement` | `int addElement(double birth_time, const std::set<int>& past_ids)` | Add element to causal set |
| `precedes` | `bool precedes(int x, int y) const` | Check causal order |
| `getPast` | `std::set<int> getPast(int id) const` | Get past elements |
| `getFuture` | `std::set<int> getFuture(int id) const` | Get future elements |
| `size` | `size_t size() const` | Get element count |
| `getElement` | `const CausalElement& getElement(int id) const` | Get element by ID |
| `getAllIDs` | `std::vector<int> getAllIDs() const` | Get all element IDs |
| `intervalSize` | `int intervalSize(int x, int y) const` | Get interval size |
| `isTree` | `bool isTree() const` | Check if tree structure |
| `density` | `double density() const` | Get sprinkling density |
| `adjacencyMatrix` | `std::vector<std::vector<int>> adjacencyMatrix() const` | Get adjacency matrix |

#### CausalSetDynamics Class

```cpp
class CausalSetDynamics
```

| Method | Signature | Description |
|--------|-----------|-------------|
| `growStep` | `int growStep(double birth_time)` | Perform one growth step |
| `grow` | `void grow(int n, double max_time)` | Grow causal set |
| `getCausalSet` | `const CausalSet& getCausalSet() const` | Get causal set |
| `getVolumeHistory` | `const std::vector<double>& getVolumeHistory() const` | Get volume history |
| `getElementCountHistory` | `const std::vector<int>& getElementCountHistory() const` | Get element count history |
| `setAlpha` | `void setAlpha(double a)` | Set alpha parameter |
| `setBeta` | `void setBeta(double b)` | Set beta parameter |
| `getAlpha` | `double getAlpha() const` | Get alpha parameter |
| `getBeta` | `double getBeta() const` | Get beta parameter |

### CDTEngine

Causal Dynamical Triangulations quantum gravity.

```cpp
#include "quantumgravity/CDTEngine.h"
```

#### Methods

| Method | Description |
|--------|-------------|
| `getName` | Returns "Causal Dynamical Triangulations" |
| `getDescription` | Returns CDT description |
| `getQuantumGeometryType` | Returns QUANTUM_CDT |
| `isDiscrete` | Returns true |
| `computeSpectralDimension` | Returns scale-dependent spectral dimension |
| `getGeometryVertices` | Returns triangulation vertices |
| `getGeometryEdges` | Returns triangulation edges |

### SpinFoamEngine

Loop Quantum Gravity spin foam model.

```cpp
#include "quantumgravity/SpinFoamEngine.h"
```

#### Methods

| Method | Description |
|--------|-------------|
| `getName` | Returns "Spin Foam (LQG)" |
| `getDescription` | Returns LQG description |
| `getQuantumGeometryType` | Returns QUANTUM_SPIN_FOAM |
| `isDiscrete` | Returns true |
| `computeSpectralDimension` | Returns spectral dimension |
| `getGeometryVertices` | Returns spin network vertices |
| `getGeometryEdges` | Returns spin network edges |

### GFTEngine

Group Field Theory quantum gravity.

```cpp
#include "quantumgravity/GFTEngine.h"
```

#### Methods

| Method | Description |
|--------|-------------|
| `getName` | Returns "Group Field Theory" |
| `getDescription` | Returns GFT description |
| `getQuantumGeometryType` | Returns QUANTUM_GFT |
| `isDiscrete` | Returns true |
| `computeSpectralDimension` | Returns spectral dimension |
| `getGeometryVertices` | Returns GFT field vertices |
| `getGeometryEdges` | Returns GFT field edges |

---

## Physics Components API

### GeodesicIntegrator

Adaptive RK4 geodesic solver.

```cpp
#include "physics/GeodesicIntegrator.h"
```

#### Methods

| Method | Description |
|--------|-------------|
| `integrate` | Integrate geodesic trajectory |
| `setStepSize` | Set integration step size |
| `getStepSize` | Get current step size |
| `setTolerance` | Set integration tolerance |
| `getTrajectory` | Get computed trajectory |

### MetricTensor

Metric tensor with Christoffel/Riemann/Ricci/Kretschmann calculations.

```cpp
#include "spacetime/MetricTensor.h"
```

#### Methods

| Method | Description |
|--------|-------------|
| `computeChristoffel` | Compute Christoffel symbols |
| `computeRicciTensor` | Compute Ricci tensor |
| `computeRicciScalar` | Compute Ricci scalar |
| `computeKretschmannScalar` | Compute Kretschmann scalar |

### Event4D

4D spacetime event representation.

```cpp
#include "spacetime/Event4D.h"
```

#### Methods

| Method | Description |
|--------|-------------|
| `getTime` | Get time coordinate |
| `getSpatial` | Get spatial coordinates |
| `setTime` | Set time coordinate |
| `setSpatial` | Set spatial coordinates |
| `distanceTo` | Compute distance to another event |

### LightCone

Light cone calculations.

```cpp
#include "spacetime/LightCone.h"
```

#### Methods

| Method | Description |
|--------|-------------|
| `computeFuture` | Compute future light cone |
| `computePast` | Compute past light cone |
| `isCausal` | Check if two events are causally connected |

---

## Math Utilities API

### Vector4D

4D vector operations.

```cpp
#include "math/Vector4D.h"
```

#### Methods

| Method | Description |
|--------|-------------|
| `dot` | Dot product |
| `cross` | 4D cross product (returns bivector) |
| `magnitude` | Vector magnitude |
| `normalize` | Normalize vector |
| `add` | Vector addition |
| `subtract` | Vector subtraction |
| `scale` | Scale by scalar |

### Matrix4x4

4x4 transformation matrices.

```cpp
#include "math/Matrix4x4.h"
```

#### Methods

| Method | Description |
|--------|-------------|
| `multiply` | Matrix multiplication |
| `determinant` | Compute determinant |
| `inverse` | Compute inverse |
| `transpose` | Transpose matrix |
| `transform` | Transform a vector |

### SO4Rotation

SO(4) rotation utilities for 4D rotations.

```cpp
#include "math/SO4Rotation.h"
```

#### Methods

| Method | Description |
|--------|-------------|
| `rotateXY` | Rotate in XY plane |
| `rotateYZ` | Rotate in YZ plane |
| `rotateXZ` | Rotate in XZ plane |
| `rotateTX` | Rotate in time-X plane |
| `rotateTY` | Rotate in time-Y plane |
| `rotateTZ` | Rotate in time-Z plane |

### AutoDiff

Forward-mode automatic differentiation.

```cpp
#include "math/AutoDiff.h"
```

#### Methods

| Method | Description |
|--------|-------------|
| `derivative` | Compute derivative of function |
| `gradient` | Compute gradient |
| `jacobian` | Compute Jacobian matrix |

---

## Usage Examples

### Basic Discovery Engine Usage

```cpp
#include "discovery/DiscoveryEngine.h"
#include "quantumgravity/CausalSet.h"

using namespace quantumverse;

// Create discovery engine
DiscoveryEngine engine;

// Enable AI subsystems
engine.enableSymbolicRegression(true);
engine.enableAnomalyDetection(true);
engine.enableHypothesisTesting(true);

// Load curvature model (Task 2.5)
engine.loadCurvatureModel("models/curvature_flow.onnx");

// Create and register a theory plugin
auto causalSetPlugin = std::make_unique<CausalSetPlugin>(0.5, 0.1, 1.0, 1000);
TheoryManager manager;
manager.registerPlugin(std::move(causalSetPlugin));
manager.setActiveTheory("Causal Set Theory");

// Analyze a location
Event4D location(0, 1.0, 0, 0, 0);  // t, x, y, z, w
MetricTensor metric = manager.computeMetric(location, {});

// Detect anomalies
DiscoveryResult result = engine.detectAnomaly(location, metric, {});
```

### Using Discovery Instruments

```cpp
#include "discovery/ExoplanetaryTTVFifthForceHunter.h"

// Create instrument
ExoplanetaryTTVFifthForceHunter hunter;

// Run analysis
std::vector<InstrumentFinding> findings = hunter.analyze(metric, location);

// Check findings
for (const auto& finding : findings) {
    if (finding.severity >= AlertSeverity::HIGH) {
        // Potential discovery!
        std::cout << "Anomaly: " << finding.description << std::endl;
    }
}
```

### Quantum Gravity Analysis

```cpp
#include "quantumgravity/CausalSet.h"

// Create causal set engine
quantumgravity::CausalSetEngine engine(0.5, 0.1, 1.0, 1000);

// Grow the causal set
engine.grow(10000, 100.0);

// Get quantum properties
double spectralDim = engine.computeSpectralDimension(1.0);
double hausdorffDim = engine.computeHausdorffDimension();

// Get visualization data
auto vertices = engine.getGeometryVertices();
auto edges = engine.getGeometryEdges();
```

---

## Build Configuration

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `QUANTUMVERSE_USE_QT` | ON | Enable Qt GUI |
| `QUANTUMVERSE_ENABLE_QUANTUM_GRAVITY` | ON | Enable quantum gravity engines |
| `QUANTUMVERSE_BUILD_TESTS` | ON | Build test suite |
| `QUANTUMVERSE_BUILD_EXAMPLES` | ON | Build examples |
| `QUANTUMVERSE_USE_CUDA` | OFF | Enable CUDA acceleration |

### Example Build

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DQUANTUMVERSE_USE_QT=ON \
         -DQUANTUMVERSE_ENABLE_QUANTUM_GRAVITY=ON \
         -DQUANTUMVERSE_BUILD_TESTS=ON
cmake --build . --config Release --parallel 8
```

---

## Build Configuration

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `QUANTUMVERSE_USE_IMGUI` | ON | Enable ImGui GUI (recommended) |
| `QUANTUMVERSE_USE_QT` | OFF | Enable Qt GUI (deprecated) |
| `QUANTUMVERSE_ENABLE_QUANTUM_GRAVITY` | ON | Enable quantum gravity engines |
| `QUANTUMVERSE_BUILD_TESTS` | ON | Build test suite |
| `QUANTUMVERSE_BUILD_EXAMPLES` | ON | Build examples |
| `QUANTUMVERSE_USE_CUDA` | OFF | Enable CUDA acceleration |
| `QUANTUMVERSE_USE_ONNX` | OFF | Enable ONNX Runtime for ML models |

### Example Build

```bash
# Recommended ImGui build
cmake -B build -DCMAKE_BUILD_TYPE=Release \
         -DQUANTUMVERSE_USE_IMGUI=ON \
         -DQUANTUMVERSE_BUILD_TESTS=ON
cmake --build build --config Release --parallel 8
```

---

## Migration Notes

### ImGui Migration (2026-05-29)

The project has been successfully migrated from Qt to Dear ImGui + GLFW. Key changes:

- **Single executable**: No DLL dependencies (static linking)
- **Zero external dependencies**: All libraries statically linked
- **Cross-platform**: Works on Windows, Linux, and macOS
- **VR support**: OpenXR integration via stub mode

### Breaking Changes from Qt Version

| Qt Feature | ImGui Equivalent |
|------------|------------------|
| `MainWindow` | `UI4D_ImGui` |
| QML Viewport | `UI4D_ImGui::render4DView()` |
| Qt Signals/Slots | Direct function calls |
| QML Properties | Direct member access |

### File Structure Changes

```
src/ui_imgui/          # All UI code (replaces src/qml/)
├── UI4D_ImGui.cpp/h    # Main UI implementation
├── TimelineBar.cpp/h   # Timeline control
├── ObjectBrowser.cpp/h # Scene object browser
├── PropertyEditor.cpp/h # Object property editor
└── FloatingPanels.cpp/h # Overlay panels
```

---

## See Also

- [README.md](README.md) - Project overview and quick start
- [UI_Documentation.md](UI_Documentation.md) - User interface documentation
- [user_manual/README.md](user_manual/README.md) - Complete user manual
- [VERIFICATION_CHECKLIST.md](VERIFICATION_CHECKLIST.md) - Validation checklist
- [docs/4D_VIEWPORT_GUIDE.md](docs/4D_VIEWPORT_GUIDE.md) - 4D viewport guide
- [docs/UI_LAYOUT.md](docs/UI_LAYOUT.md) - UI layout documentation