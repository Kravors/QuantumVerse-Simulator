# QuantumVerse Simulator - Complete API Reference

## Overview

This document provides comprehensive API documentation for all public functions, classes, and methods in the QuantumVerse Simulator.

---

## Table of Contents

1. [Spacetime Module](#spacetime-module)
2. [Physics Module](#physics-module)
3. [UI4D Module](#ui4d-module)
4. [Discovery Module](#discovery-module)
5. [Quantum Gravity Module](#quantum-gravity-module)
6. [Math Utilities Module](#math-utilities-module)
7. [Rendering Module](#rendering-module)
8. [VR Module](#vr-module)
9. [ML Module](#ml-module)

---

## Spacetime Module

### Event4D

**File:** `src/spacetime/Event4D.h`

Represents an event in 4-dimensional Lorentzian manifold (spacetime).

#### Public Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `Event4D()` | `Event4D()` | Default constructor - creates origin of spacetime (0,0,0,0) |
| `Event4D(double, double, double, double)` | `Event4D(double t_, double x_, double y_, double z_)` | Parameterized constructor |
| `intervalSquared` | `double intervalSquared(const Event4D& other) const` | Spacetime interval squared (Minkowski metric) |
| `properTime` | `double properTime(const Event4D& other) const` | Proper time between events (for timelike separation) |
| `spatialDistance` | `double spatialDistance(const Event4D& other) const` | Euclidean spatial distance |
| `spatialLength` | `double spatialLength() const` | Spatial length from origin: r = sqrt(x² + y² + z²) |
| `theta` | `double theta() const` | Polar angle θ (from z-axis), range [0, π] |
| `phi` | `double phi() const` | Azimuthal angle φ (from x-axis in xy-plane), range [0, 2π) |
| `isTimelike` | `bool isTimelike(const Event4D& other) const` | Check if events are timelike separated |
| `isLightlike` | `bool isLightlike(const Event4D& other) const` | Check if events are lightlike (null) separated |
| `isSpacelike` | `bool isSpacelike(const Event4D& other) const` | Check if events are spacelike separated |
| `lorentzTransform` | `Event4D lorentzTransform(double boostVelocity) const` | Lorentz transformation to another inertial frame |
| `operator+` | `Event4D operator+(const Event4D& other) const` | Vector addition |
| `operator-` | `Event4D operator-(const Event4D& other) const` | Vector subtraction |
| `operator*` | `Event4D operator*(double scalar) const` | Scale by scalar |
| `operator==` | `bool operator==(const Event4D& other) const` | Equality comparison |
| `operator!=` | `bool operator!=(const Event4D& other) const` | Inequality comparison |
| `operator[]` | `double& operator[](int i)` | Array access (0=t, 1=x, 2=y, 3=z) |

#### Static Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `C` | `299792458.0` | Speed of light in vacuum (m/s) |
| `C2` | `C * C` | Speed of light squared |
| `G` | `6.67430e-11` | Gravitational constant (m³ kg⁻¹ s⁻²) |

---

### MetricTensor

**File:** `src/spacetime/MetricTensor.h`

Represents the metric tensor g_μν for spacetime geometry.

#### Public Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `MetricTensor()` | `MetricTensor()` | Default constructor - creates Minkowski metric |
| `schwarzschild` | `static MetricTensor schwarzschild(double mass, double r, double theta, double phi)` | Create Schwarzschild metric |
| `kerr` | `static MetricTensor kerr(double mass, double angular_momentum, double r, double theta)` | Create Kerr metric (rotating black hole) |
| `evaluate` | `virtual std::array<std::array<double, 4>, 4> evaluate(const Event4D& event) const` | Evaluate metric at spacetime event |
| `setToMinkowski` | `void setToMinkowski()` | Set metric to Minkowski (flat spacetime) |
| `setToZero` | `void setToZero()` | Set metric to zero |
| `set` | `void set(int mu, int nu, double val)` | Set individual metric component |
| `interval` | `double interval(const Event4D& event1, const Event4D& event2) const` | Compute spacetime interval |
| `properTime` | `double properTime(const Event4D& event1, const Event4D& event2) const` | Compute proper time along worldline |
| `isLorentzian` | `bool isLorentzian() const` | Check if metric has correct signature |
| `determinant` | `double determinant() const` | Compute metric determinant |
| `inverse` | `virtual MetricTensor inverse() const` | Compute inverse metric g^μν |
| `lowerIndex` | `std::array<double, 4> lowerIndex(const std::array<double, 4>& vector) const` | Lower index: v_μ = g_μν v^ν |
| `raiseIndex` | `std::array<double, 4> raiseIndex(const std::array<double, 4>& covector) const` | Raise index: v^μ = g^μν v_ν |

---

### LightCone

**File:** `src/spacetime/LightCone.h`

Light cone calculations for causal structure analysis.

#### Public Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `computeFuture` | `void computeFuture(const Event4D& event, double radius)` | Compute future light cone |
| `computePast` | `void computePast(const Event4D& event, double radius)` | Compute past light cone |
| `isCausal` | `bool isCausal(const Event4D& from, const Event4D& to) const` | Check if two events are causally connected |

---

## Physics Module

### GeodesicIntegrator

**File:** `src/physics/GeodesicIntegrator.h`

Adaptive step-size Runge-Kutta integration for geodesic equations.

#### Public Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `GeodesicIntegrator()` | `GeodesicIntegrator(double tol, double minStep, double maxStep, double safety, int maxIter)` | Constructor with integration parameters |
| `setMetric` | `void setMetric(std::shared_ptr<MetricTensor> metric)` | Set metric for integration |
| `getMetric` | `std::shared_ptr<MetricTensor> getMetric() const` | Get current metric |
| `setMetricField` | `void setMetricField(std::function<MetricTensor(const Event4D&)> field)` | Set custom metric field function |
| `computeChristoffelSymbols` | `void computeChristoffelSymbols(const Event4D& position) const` | Compute Christoffel symbols at position |
| `getChristoffel` | `double getChristoffel(int lambda, int mu, int nu) const` | Get cached Christoffel symbol |
| `getChristoffelSymbols` | `const std::array<std::array<std::array<double, 4>, 4>, 4>& getChristoffelSymbols() const` | Get full Christoffel array |
| `integrate` | `std::vector<GeodesicStep> integrate(const Event4D& startEvent, const std::array<double, 4>& initialVelocity, GeodesicType type, double targetProperTime, bool adaptive)` | Integrate geodesic trajectory |
| `integrateSimple` | `std::vector<Event4D> integrateSimple(const Event4D& start, const std::array<double, 4>& velocity, int steps, double deltaTau)` | Simplified non-adaptive integration |

#### GeodesicType Enum

```cpp
enum class GeodesicType {
    TIMELIKE,   // Massive particle trajectory
    LIGHTLIKE,  // Light ray (photon)
    SPACELIKE   // Not physically realizable
};
```

---

### DifferentiableGeodesicIntegrator

**File:** `src/physics/DifferentiableGeodesicIntegrator.h`

Forward-mode automatic differentiation for geodesic integration.

#### Public Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `DifferentiableGeodesicIntegrator()` | `DifferentiableGeodesicIntegrator()` | Constructor |
| `integrateWithGradients` | `std::vector<GeodesicStep> integrateWithGradients(const Event4D& startEvent, const std::array<double, 4>& initialVelocity, double targetProperTime, std::array<double, 4>& gradients)` | Integrate with gradient computation |
| `setParameter` | `void setParameter(const std::string& name, double value)` | Set metric parameter |
| `getParameter` | `double getParameter(const std::string& name) const` | Get metric parameter |

---

## UI4D Module

### UI4D_ImGui

**File:** `src/ui_imgui/UI4D_ImGui.h`

Dear ImGui-based 4D User Interface.

#### Public Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `UI4D_ImGui()` | `UI4D_ImGui(std::shared_ptr<MetricTensor> metric, std::shared_ptr<CurvatureRenderer> curvatureRenderer, ...)` | Constructor |
| `~UI4D_ImGui()` | `~UI4D_ImGui()` | Destructor |
| `render` | `void render()` | Main render function |
| `setupDockingAndMenu` | `void setupDockingAndMenu()` | Create dockspace + main menu bar |
| `renderPanels` | `void renderPanels()` | Render panels independently |
| `processInput` | `void processInput()` | Handle GLFW input |
| `renderFloatingWindowsFallback` | `void renderFloatingWindowsFallback()` | Fallback without docking |
| `getConfig` | `UI4DConfig& getConfig()` | Get configuration |
| `setShow4DView` | `void setShow4DView(bool show)` | Set 4D view visibility |
| `setShowSliceViews` | `void setShowSliceViews(bool show)` | Set slice views visibility |
| `setShowCausalGraph` | `void setShowCausalGraph(bool show)` | Set causal graph visibility |
| `setShowDiscoveryPanel` | `void setShowDiscoveryPanel(bool show)` | Set discovery panel visibility |
| `setShowPlanckMicroscope` | `void setShowPlanckMicroscope(bool show)` | Set Planck microscope visibility |
| `setShowTheoryManager` | `void setShowTheoryManager(bool show)` | Set theory manager visibility |
| `setShowObjectBrowser` | `void setShowObjectBrowser(bool show)` | Set object browser visibility |
| `setShowPropertyEditor` | `void setShowPropertyEditor(bool show)` | Set property editor visibility |
| `setShowFlightTelemetry` | `void setShowFlightTelemetry(bool show)` | Set flight telemetry visibility |
| `setShowMinimap` | `void setShowMinimap(bool show)` | Set minimap visibility |
| `translateCamera` | `void translateCamera(const std::array<double, 4>& delta)` | Translate camera |
| `rotateCamera` | `void rotateCamera(int plane1, int plane2, double angle)` | Rotate in 4D plane |
| `zoomCamera` | `void zoomCamera(double factor)` | Zoom camera |
| `setSliceOffset` | `void setSliceOffset(int viewIndex, double offset)` | Set slice offset |
| `resetCamera` | `void resetCamera()` | Reset camera to default |
| `setActiveTheory` | `void setActiveTheory(const std::string& theoryName)` | Set active theory |
| `getActiveTheory` | `const std::string& getActiveTheory() const` | Get active theory name |
| `setMetric` | `void setMetric(std::shared_ptr<MetricTensor> m)` | Set metric |
| `setCurvatureRenderer` | `void setCurvatureRenderer(std::shared_ptr<CurvatureRenderer> cr)` | Set curvature renderer |
| `addEvent` | `int addEvent(const Event4D& event)` | Add event to causal graph |
| `moveProbeTo` | `void moveProbeTo(const Event4D& position)` | Move discovery probe |
| `setSceneGraphManager` | `void setSceneGraphManager(std::shared_ptr<SceneGraphManager> manager)` | Set scene graph manager |
| `setupInputCallbacks` | `void setupInputCallbacks(GLFWwindow* window)` | Setup GLFW callbacks |

---

### FloatingPanels

**File:** `src/ui_imgui/FloatingPanels.h`

Manager for floating overlay panels.

#### Public Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `FloatingPanels()` | `FloatingPanels()` | Constructor |
| `~FloatingPanels()` | `~FloatingPanels()` | Destructor |
| `render` | `void render()` | Render all floating panels |
| `toggleCurvatureOverlay` | `void toggleCurvatureOverlay()` | Toggle curvature overlay |
| `toggleGeodesicTracer` | `void toggleGeodesicTracer()` | Toggle geodesic tracer |
| `addAnomaly` | `void addAnomaly(const AnomalyAlert& alert)` | Add anomaly alert |
| `clearAlerts` | `void clearAlerts()` | Clear all alerts |
| `setCurvatureIntensity` | `void setCurvatureIntensity(float intensity)` | Set curvature intensity |

#### Public Members

| Member | Type | Description |
|--------|------|-------------|
| `showCurvatureOverlay` | `bool` | Curvature overlay visibility |
| `showGeodesicTracer` | `bool` | Geodesic tracer visibility |
| `showAnomalyAlerts` | `bool` | Anomaly alerts visibility |
| `curvatureScale` | `float` | Curvature scale factor |
| `gridResolution` | `int` | Grid resolution |
| `curvatureColor` | `ImVec4` | Curvature display color |
| `numGeodesics` | `int` | Number of geodesics to trace |
| `geodesicWidth` | `float` | Geodesic line width |
| `geodesicColor` | `ImVec4` | Geodesic color |

---

## Discovery Module

### DiscoveryEngine

**File:** `src/discovery/DiscoveryEngine.h`

Main AI-driven discovery system.

#### Public Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `DiscoveryEngine()` | `DiscoveryEngine()` | Constructor |
| `enableSymbolicRegression` | `void enableSymbolicRegression(bool enable)` | Enable symbolic regression |
| `enableAnomalyDetection` | `void enableAnomalyDetection(bool enable)` | Enable anomaly detection |
| `enableHypothesisTesting` | `void enableHypothesisTesting(bool enable)` | Enable hypothesis testing |
| `generateFieldEquation` | `std::string generateFieldEquation(const std::vector<Event4D>& trajectory, const MetricTensor& metric, double tolerance)` | Generate field equation |
| `detectAnomaly` | `DiscoveryResult detectAnomaly(const Event4D& location, const MetricTensor& metric, const std::vector<Event4D>& geodesicTrajectory)` | Detect anomalies |
| `classifyAnomaly` | `std::string classifyAnomaly(const DiscoveryResult& result)` | Classify anomaly type |
| `proposeHypothesis` | `std::string proposeHypothesis(const std::string& name, const std::string& fieldEquation, const std::map<std::string, double>& parameters)` | Propose hypothesis |
| `testHypothesis` | `bool testHypothesis(const std::string& hypothesisId, const std::vector<Event4D>& testTrajectories)` | Test hypothesis |
| `validateHypothesis` | `bool validateHypothesis(const std::string& hypothesisId)` | Validate hypothesis |
| `refuteHypothesis` | `bool refuteHypothesis(const std::string& hypothesisId)` | Refute hypothesis |
| `getDiscoveries` | `std::vector<DiscoveryResult> getDiscoveries() const` | Get all discoveries |
| `getHypotheses` | `std::vector<Hypothesis> getHypotheses() const` | Get all hypotheses |
| `loadCurvatureModel` | `bool loadCurvatureModel(const std::string& modelPath)` | Load curvature flow model |
| `isCurvatureModelLoaded` | `bool isCurvatureModelLoaded() const` | Check if model loaded |

---

### Discovery Instruments

#### ExoplanetaryTTVFifthForceHunter

**File:** `src/discovery/ExoplanetaryTTVFifthForceHunter.h`

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `std::string getName() const override` | Returns "ExoplanetaryTTVFifthForceHunter" |
| `getDescription` | `std::string getDescription() const override` | Returns description |
| `getCategory` | `std::string getCategory() const override` | Returns "Exoplanetary" |
| `getDefaultSeverity` | `AlertSeverity getDefaultSeverity() const override` | Returns HIGH |
| `analyze` | `std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location, const std::vector<Event4D>& trajectory = {}) override` | Analyze TTV data |

#### BlackHoleJetAnomalyRecogniser

**File:** `src/discovery/BlackHoleJetAnomalyRecogniser.h`

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `std::string getName() const override` | Returns "BlackHoleJetAnomalyRecogniser" |
| `getDescription` | `std::string getDescription() const override` | Returns description |
| `getCategory` | `std::string getCategory() const override` | Returns "Black Holes" |
| `getDefaultSeverity` | `AlertSeverity getDefaultSeverity() const override` | Returns HIGH |
| `analyze` | `std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location, const std::vector<Event4D>& trajectory = {}) override` | Analyze jet anomalies |

#### BosonStarCollisionPredictor

**File:** `src/discovery/BosonStarCollisionPredictor.h`

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `std::string getName() const override` | Returns "BosonStarCollisionPredictor" |
| `getDescription` | `std::string getDescription() const override` | Returns description |
| `getCategory` | `std::string getCategory() const override` | Returns "Compact Objects" |
| `getDefaultSeverity` | `AlertSeverity getDefaultSeverity() const override` | Returns HIGH |
| `analyze` | `std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location, const std::vector<Event4D>& trajectory = {}) override` | Analyze boson star signatures |

#### NeutronStarGlitchPhaseDetector

**File:** `src/discovery/NeutronStarGlitchPhaseDetector.h`

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `std::string getName() const override` | Returns "NeutronStarGlitchPhaseDetector" |
| `getDescription` | `std::string getDescription() const override` | Returns description |
| `getCategory` | `std::string getCategory() const override` | Returns "Neutron Stars" |
| `getDefaultSeverity` | `AlertSeverity getDefaultSeverity() const override` | Returns MEDIUM |
| `analyze` | `std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location, const std::vector<Event4D>& trajectory = {}) override` | Analyze glitch signatures |

#### GalacticRotationCurveScanner

**File:** `src/discovery/GalacticRotationCurveScanner.h`

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `std::string getName() const override` | Returns "GalacticRotationCurveScanner" |
| `getDescription` | `std::string getDescription() const override` | Returns description |
| `getCategory` | `std::string getCategory() const override` | Returns "Galactic" |
| `getDefaultSeverity` | `AlertSeverity getDefaultSeverity() const override` | Returns MEDIUM |
| `analyze` | `std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location, const std::vector<Event4D>& trajectory = {}) override` | Analyze rotation curves |

#### FineStructureConstantDriftObservatory

**File:** `src/discovery/FineStructureConstantDriftObservatory.h`

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `std::string getName() const override` | Returns "FineStructureConstantDriftObservatory" |
| `getDescription` | `std::string getDescription() const override` | Returns description |
| `getCategory` | `std::string getCategory() const override` | Returns "Fundamental Constants" |
| `getDefaultSeverity` | `AlertSeverity getDefaultSeverity() const override` | Returns CRITICAL |
| `analyze` | `std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location, const std::vector<Event4D>& trajectory = {}) override` | Analyze alpha variations |

#### UltralightDMWaveInterferometer

**File:** `src/discovery/UltralightDMWaveInterferometer.h`

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `std::string getName() const override` | Returns "UltralightDMWaveInterferometer" |
| `getDescription` | `std::string getDescription() const override` | Returns description |
| `getCategory` | `std::string getCategory() const override` | Returns "Dark Matter" |
| `getDefaultSeverity` | `AlertSeverity getDefaultSeverity() const override` | Returns HIGH |
| `analyze` | `std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location, const std::vector<Event4D>& trajectory = {}) override` | Analyze DM wave signatures |

#### PrimordialLithiumCrisisSolver

**File:** `src/discovery/PrimordialLithiumCrisisSolver.h`

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `std::string getName() const override` | Returns "PrimordialLithiumCrisisSolver" |
| `getDescription` | `std::string getDescription() const override` | Returns description |
| `getCategory` | `std::string getCategory() const override` | Returns "Cosmology" |
| `getDefaultSeverity` | `AlertSeverity getDefaultSeverity() const override` | Returns MEDIUM |
| `analyze` | `std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location, const std::vector<Event4D>& trajectory = {}) override` | Analyze BBN lithium problem |

#### GalacticTidalStreamCartographer

**File:** `src/discovery/GalacticTidalStreamCartographer.h`

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `std::string getName() const override` | Returns "GalacticTidalStreamCartographer" |
| `getDescription` | `std::string getDescription() const override` | Returns description |
| `getCategory` | `std::string getCategory() const override` | Returns "Galactic" |
| `getDefaultSeverity` | `AlertSeverity getDefaultSeverity() const override` | Returns LOW |
| `analyze` | `std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location, const std::vector<Event4D>& trajectory = {}) override` | Analyze tidal streams |

#### RecombinationConstantVariationImager

**File:** `src/discovery/RecombinationConstantVariationImager.h`

| Method | Signature | Description |
|--------|-----------|-------------|
| `getName` | `std::string getName() const override` | Returns "RecombinationConstantVariationImager" |
| `getDescription` | `std::string getDescription() const override` | Returns description |
| `getCategory` | `std::string getCategory() const override` | Returns "CMB Physics" |
| `getDefaultSeverity` | `AlertSeverity getDefaultSeverity() const override` | Returns CRITICAL |
| `analyze` | `std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location, const std::vector<Event4D>& trajectory = {}) override` | Analyze recombination variations |

---

## Quantum Gravity Module

### CausalSetEngine

**File:** `src/quantumgravity/CausalSet.h`

| Method | Signature | Description |
|--------|-----------|-------------|
| `CausalSetEngine()` | `CausalSetEngine(double alpha, double beta, double planck_vol, int initial_elements)` | Constructor |
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

---

## Math Utilities Module

### Vector4D

**File:** `src/math/Vector4D.h`

| Method | Signature | Description |
|--------|-----------|-------------|
| `dot` | `double dot(const Vector4D& other) const` | Dot product |
| `cross` | `Vector4D cross(const Vector4D& other) const` | 4D cross product (returns bivector) |
| `magnitude` | `double magnitude() const` | Vector magnitude |
| `normalize` | `Vector4D normalize() const` | Normalize vector |
| `add` | `Vector4D add(const Vector4D& other) const` | Vector addition |
| `subtract` | `Vector4D subtract(const Vector4D& other) const` | Vector subtraction |
| `scale` | `Vector4D scale(double scalar) const` | Scale by scalar |

---

### Matrix4x4

**File:** `src/math/Matrix4x4.h`

| Method | Signature | Description |
|--------|-----------|-------------|
| `multiply` | `Matrix4x4 multiply(const Matrix4x4& other) const` | Matrix multiplication |
| `determinant` | `double determinant() const` | Compute determinant |
| `inverse` | `Matrix4x4 inverse() const` | Compute inverse |
| `transpose` | `Matrix4x4 transpose() const` | Transpose matrix |
| `transform` | `Vector4D transform(const Vector4D& vector) const` | Transform a vector |

---

### SO4Rotation

**File:** `src/math/SO4Rotation.h`

| Method | Signature | Description |
|--------|-----------|-------------|
| `rotateXY` | `Matrix4x4 rotateXY(double angle)` | Rotate in XY plane |
| `rotateYZ` | `Matrix4x4 rotateYZ(double angle)` | Rotate in YZ plane |
| `rotateXZ` | `Matrix4x4 rotateXZ(double angle)` | Rotate in XZ plane |
| `rotateTX` | `Matrix4x4 rotateTX(double angle)` | Rotate in time-X plane |
| `rotateTY` | `Matrix4x4 rotateTY(double angle)` | Rotate in time-Y plane |
| `rotateTZ` | `Matrix4x4 rotateTZ(double angle)` | Rotate in time-Z plane |

---

## Rendering Module

### CurvatureRenderer

**File:** `src/rendering/CurvatureRenderer.h`

| Method | Signature | Description |
|--------|-----------|-------------|
| `CurvatureRenderer()` | `CurvatureRenderer()` | Constructor |
| `initialize` | `bool initialize()` | Initialize renderer |
| `render` | `void render(const MetricTensor& metric, const Event4D& cameraPos, const std::array<double, 4>& cameraRot)` | Render curvature |
| `setGridResolution` | `void setGridResolution(int res)` | Set grid resolution |
| `setCurvatureScale` | `void setCurvatureScale(float scale)` | Set curvature scale |

---

## VR Module

### MultiUserServer

**File:** `src/vr/MultiUserServer.cpp`

| Method | Signature | Description |
|--------|-----------|-------------|
| `MultiUserServer()` | `MultiUserServer()` | Constructor |
| `start` | `bool start(int port)` | Start server |
| `stop` | `void stop()` | Stop server |
| `broadcastState` | `void broadcastState(const SpacetimeState& state)` | Broadcast state to clients |
| `handleClientMessage` | `void handleClientMessage(const std::string& userId, const std::string& message)` | Handle client message |

---

## ML Module

### GeodesicNeuralODE

**File:** `src/ml/GeodesicNeuralODE.h`

| Method | Signature | Description |
|--------|-----------|-------------|
| `GeodesicNeuralODE()` | `GeodesicNeuralODE()` | Constructor |
| `loadModel` | `bool loadModel(const std::string& path)` | Load ONNX model |
| `predict` | `std::vector<Event4D> predict(const Event4D& start, const std::array<double, 4>& velocity, double tau, int steps)` | Predict geodesic |
| `isLoaded` | `bool isLoaded() const` | Check if model loaded |

---

### CurvatureNormalizingFlow

**File:** `src/ml/CurvatureNormalizingFlow.h`

| Method | Signature | Description |
|--------|-----------|-------------|
| `CurvatureNormalizingFlow()` | `CurvatureNormalizingFlow()` | Constructor |
| `loadModel` | `bool loadModel(const std::string& path)` | Load ONNX model |
| `detectAnomaly` | `double detectAnomaly(const std::vector<double>& features)` | Detect anomaly |
| `isLoaded` | `bool isLoaded() const` | Check if model loaded |

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

---

## See Also

- [API_Documentation.md](API_Documentation.md) - Detailed API with examples
- [UI_Documentation.md](UI_Documentation.md) - User interface documentation
- [IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md) - Technical implementation details
- [VERIFICATION_CHECKLIST.md](VERIFICATION_CHECKLIST.md) - Validation checklist