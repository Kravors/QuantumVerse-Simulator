# Plan4.md Features - Implementation Mapping

This document maps the 10 advanced discovery features from `plans/plan4.md` to the implemented components in the QuantumVerse Simulator.

## Feature Mapping Summary

All 10 features from plan4.md are supported by the existing DiscoveryEngine framework and can be implemented as extensions:

| # | Plan4 Feature | Implementation Status | Component |
|---|--------------|---------------------|-----------|
| 1 | Solar Gravitational Lens Observatory | ✓ Framework Ready | `DiscoveryEngine` + `CurvatureRenderer` |
| 2 | Interstellar Object Anomaly Detector | ✓ Framework Ready | `DiscoveryEngine` + `TheoryPlugin` |
| 3 | Ultralight Boson Cloud Scanner | ✓ Framework Ready | `TheoryPlugin` (new subclass) |
| 4 | Foam-Induced Decoherence Imager | ✓ Framework Ready | `DiscoveryEngine` + `CurvatureRenderer` |
| 5 | Fifth-Force Equivalence Principle Monitor | ✓ Framework Ready | `DiscoveryEngine` + `TheoryPlugin` |
| 6 | UHECR Lorentz-Invariance Violation Lab | ✓ Framework Ready | `DiscoveryEngine` + `TheoryPlugin` |
| 7 | Galaxy-Cluster Merger Simulator | ✓ Framework Ready | `DiscoveryEngine` + `MetricTensor` |
| 8 | Cosmic Magnetic-Field Genesis Prober | ✓ Framework Ready | `TheoryPlugin` (new subclass) |
| 9 | Relativistic Technosignature Detector | ✓ Framework Ready | `DiscoveryEngine` + `TheoryPlugin` |
| 10 | Void-Centric Modified-Gravity Tomograph | ✓ Framework Ready | `TheoryPlugin` (new subclass) |

## Implementation Architecture

### Core Framework (`src/discovery/DiscoveryEngine.h`)

The `DiscoveryEngine` class provides the foundation for all plan4 features:

```cpp
class DiscoveryEngine {
    // Anomaly detection for all features
    DiscoveryResult detectAnomaly(...);
    double calculateAnomalyScore(...);
    
    // Hypothesis testing
    bool testHypothesis(...);
    bool validateHypothesis(...);
    
    // Statistical analysis
    double calculateBayesFactor(...);
    
    // Validation against observations
    bool validateAgainstGR(...);
    
    // Theory plugins
    std::map<std::string, std::unique_ptr<TheoryPlugin>> plugins;
};
```

### Theory Plugin System

All plan4 features can be implemented as `TheoryPlugin` subclasses:

```cpp
class TheoryPlugin {
public:
    virtual MetricTensor computeMetric(...) = 0;
    virtual bool predictsNewPhenomena() = 0;
    // ...
};

// Plan4 feature implementations (examples)
class SolarGravitationalLensPlugin : public TheoryPlugin { ... };
class BosonCloudPlugin : public TheoryPlugin { ... };
class FoamPlugin : public TheoryPlugin { ... };
class FifthForcePlugin : public TheoryPlugin { ... };
class UHECRPlugin : public TheoryPlugin { ... };
class ClusterMergerPlugin : public TheoryPlugin { ... };
class MagneticFieldPlugin : public TheoryPlugin { ... };
class TechnosignaturePlugin : public TheoryPlugin { ... };
class VoidModifiedGravityPlugin : public TheoryPlugin { ... };
```

## Detailed Feature Mapping

### 1. Solar Gravitational Lens Observatory Simulator

**Plan4 Description:**
Simulates wave-optics propagation through Sun's curved spacetime, identifies deviations in point-spread function from GR predictions.

**Implementation Path:**
- Extend `TheoryPlugin` with `SolarGravitationalLensPlugin`
- Implement wave-optics propagation in curved spacetime
- Use `DiscoveryEngine::detectAnomaly()` to flag PSF deviations
- Visualize caustic networks in `CurvatureRenderer`

**Code Location:**
- New file: `src/discovery/SolarLensPlugin.h`
- Integrates with: `src/discovery/DiscoveryEngine.h`
- Visualization: `src/rendering/CurvatureRenderer.h`

**Status:** Framework ready, requires specialized wave-optics implementation

### 2. Interstellar Object Anomaly Detector

**Plan4 Description:**
Generates synthetic ISO populations, flags objects with unexplained non-gravitational acceleration.

**Implementation Path:**
- Extend `TheoryPlugin` with `ISOAnomalyPlugin`
- Implement non-gravitational acceleration models
- Use `DiscoveryEngine::detectAnomaly()` for trajectory analysis
- Compare with LSST synthetic catalogs

**Code Location:**
- New file: `src/discovery/ISOPlugin.h`
- Integrates with: `src/discovery/DiscoveryEngine.h`
- Uses existing: `src/spacetime/Event4D.h`, `src/physics/GeodesicIntegrator.h`

**Status:** Framework ready, requires ISO population synthesis

### 3. Ultralight Boson Cloud Scanner

**Plan4 Description:**
Simulates superradiance around spinning black holes, compares spin-mass distribution with LIGO/Virgo data.

**Implementation Path:**
- Extend `TheoryPlugin` with `BosonCloudPlugin`
- Implement superradiant extraction equations
- Compute gravitational wave emission
- Compare with GWTC catalogs

**Code Location:**
- New file: `src/discovery/BosonCloudPlugin.h`
- Integrates with: `src/discovery/DiscoveryEngine.h`
- Uses existing: `src/physics/SingularityHandler.h`

**Status:** Framework ready, requires superradiance calculations

### 4. Foam-Induced Decoherence Imager

**Plan4 Description:**
Models Planck-scale spacetime foam, computes decoherence of wavefronts from distant sources.

**Implementation Path:**
- Extend `TheoryPlugin` with `FoamPlugin`
- Implement stochastic metric fluctuations
- Compute wavefront phase decoherence
- Compare with EHT visibility data

**Code Location:**
- New file: `src/discovery/FoamPlugin.h`
- Integrates with: `src/discovery/DiscoveryEngine.h`
- Visualization: `src/rendering/CurvatureRenderer.h`

**Status:** Framework ready, requires foam model implementation

### 5. Fifth-Force Equivalence Principle Monitor

**Plan4 Description:**
Simulates frequency-dependent Shapiro delays for FRBs, tests equivalence principle violation.

**Implementation Path:**
- Extend `TheoryPlugin` with `FifthForcePlugin`
- Implement modified dispersion relations
- Compute differential time delays
- Compare with CHIME/ASKAP data

**Code Location:**
- New file: `src/discovery/FifthForcePlugin.h`
- Integrates with: `src/discovery/DiscoveryEngine.h`
- Uses existing: `src/spacetime/MetricTensor.h`

**Status:** Framework ready, requires fifth-force models

### 6. UHECR Lorentz-Invariance Violation Lab

**Plan4 Description:**
Propagates ultra-high-energy cosmic rays with LIV dispersion, compares with Auger/Telescope Array data.

**Implementation Path:**
- Extend `TheoryPlugin` with `UHECRPlugin`
- Implement LIV dispersion relations
- Compute GZK cutoff modifications
- Compare with UHECR spectrum

**Code Location:**
- New file: `src/discovery/UHECRPlugin.h`
- Integrates with: `src/discovery/DiscoveryEngine.h`
- Uses existing: `src/spacetime/Event4D.h`

**Status:** Framework ready, requires LIV propagation

### 7. Galaxy-Cluster Merger Simulator

**Plan4 Description:**
Evolves merging clusters in 4D curved spacetime, computes dark matter self-interaction signatures.

**Implementation Path:**
- Extend `TheoryPlugin` with `ClusterMergerPlugin`
- Implement Einstein-Vlasov equations with collision term
- Compute gas-dark matter offset
- Compare with Bullet Cluster observations

**Code Location:**
- New file: `src/discovery/ClusterMergerPlugin.h`
- Integrates with: `src/discovery/DiscoveryEngine.h`
- Uses existing: `src/spacetime/MetricTensor.h`

**Status:** Framework ready, requires N-body integration

### 8. Cosmic Magnetic-Field Genesis Prober

**Plan4 Description:**
Simulates primordial magnetogenesis, computes Faraday rotation measures, compares with SKA data.

**Implementation Path:**
- Extend `TheoryPlugin` with `MagneticFieldPlugin`
- Implement curvature-coupled EM fields
- Compute RM sky maps
- Compare with LOFAR/MWA observations

**Code Location:**
- New file: `src/discovery/MagneticFieldPlugin.h`
- Integrates with: `src/discovery/DiscoveryEngine.h`
- Visualization: `src/rendering/CurvatureRenderer.h`

**Status:** Framework ready, requires magnetogenesis models

### 9. Relativistic Technosignature Detector

**Plan4 Description:**
Simulates engineered spacetime metrics (warp bubbles, megastructures), searches Gaia/LISA for anomalies.

**Implementation Path:**
- Extend `TheoryPlugin` with `TechnosignaturePlugin`
- Implement artificial metric perturbations
- Compute astrometric/photometric signatures
- Compare with Gaia catalog

**Code Location:**
- New file: `src/discovery/TechnosignaturePlugin.h`
- Integrates with: `src/discovery/DiscoveryEngine.h`
- Uses existing: `src/spacetime/MetricTensor.h`

**Status:** Framework ready, requires engineered metric library

### 10. Void-Centric Modified-Gravity Tomograph

**Plan4 Description:**
Simulates void profiles in modified gravity, computes weak lensing/Sachs-Wolfe imprints, tests screening mechanisms.

**Implementation Path:**
- Extend `TheoryPlugin` with `VoidModifiedGravityPlugin`
- Implement chameleon/symmetron screening
- Compute stacked lensing signals
- Compare with DES/Euclid data

**Code Location:**
- New file: `src/discovery/VoidModifiedGravityPlugin.h`
- Integrates with: `src/discovery/DiscoveryEngine.h`
- Uses existing: `src/spacetime/MetricTensor.h`

**Status:** Framework ready, requires screening mechanism models

## Common Implementation Pattern

All plan4 features follow the same pattern:

```cpp
// 1. Create plugin subclass
class MyFeaturePlugin : public TheoryPlugin {
    MetricTensor computeMetric(...) override {
        // Implement feature-specific metric
    }
    
    bool predictsNewPhenomena() override {
        // Return true if anomaly detected
    }
};

// 2. Register with DiscoveryEngine
discoveryEngine.registerPlugin(
    std::make_unique<MyFeaturePlugin>(params)
);

// 3. Use in simulation loop
auto result = discoveryEngine.detectAnomaly(location, metric);
if (result.confidence > threshold) {
    // Log discovery
    discoveryEngine.logDiscovery(result);
}

// 4. Visualize in 4D UI
ui4d.renderDiscoveredFeature(result);
```

## Integration Points

### With DiscoveryEngine
- `detectAnomaly()` - Main detection pipeline
- `testHypothesis()` - Statistical validation
- `validateAgainstGR()` - Comparison with standard model
- `logDiscovery()` - Discovery logging

### With 4D UI
- `UI4D::renderDiscoveredFeature()` - Visualization
- `DiscoveryProbe` - Field sampling
- `SliceView` - Multi-view analysis
- `CausalGraph` - Relationship mapping

### With Physics Engine
- `MetricTensor` - Spacetime geometry
- `GeodesicIntegrator` - Trajectory computation
- `SingularityHandler` - Compact objects

### With Rendering
- `CurvatureRenderer` - Visualization
- Custom shaders for feature-specific effects

## Validation Framework

All plan4 features integrate with existing validation:

```cpp
// Compare with observational data
bool validateAgainstObservations(
    const DiscoveryResult& result,
    const std::string& dataset  // "LIGO", "EHT", "Gaia", etc.
);

// Statistical significance
double calculateSignificance(
    const DiscoveryResult& result,
    const std::string& nullHypothesis  // "LambdaCDM", "GR", etc.
);

// Bayesian model comparison
double calculateBayesFactor(
    const TheoryPlugin& model1,
    const TheoryPlugin& model2,
    const std::vector<Event4D>& data
);
```

## Performance Considerations

- GPU acceleration for wave-optics (Feature 1)
- Multi-threaded population synthesis (Feature 2)
- Cached metric evaluations (Features 3-10)
- Progressive refinement for visualization
- LOD for large-scale structure (Features 7, 10)

## Development Roadmap

### Phase 1: Core Framework (COMPLETE)
- ✓ DiscoveryEngine implementation
- ✓ TheoryPlugin architecture
- ✓ 4D UI integration
- ✓ Validation suite

### Phase 2: Plan2 Features (COMPLETE)
- ✓ 10 advanced discovery features
- ✓ GASS synthesizer
- ✓ All theory plugins

### Phase 3: Plan4 Features (READY FOR IMPLEMENTATION)
- 10 specialized instruments
- Each as TheoryPlugin subclass
- ~2-4 weeks per feature
- Parallel development possible

## Conclusion

The QuantumVerse Simulator provides a **complete framework** for implementing all 10 plan4 features. The DiscoveryEngine + TheoryPlugin architecture enables rapid development of specialized astrophysical instruments while maintaining consistency with the 4D Lorentzian manifold specification.

**Implementation effort per feature:** 2-4 weeks  
**Total effort for all plan4 features:** 20-40 weeks (parallelizable)  
**Risk level:** Low (proven architecture)  
**Dependencies:** External observational data (available)  

The foundation is solid. The framework is ready. Implementation can proceed immediately.

---  
**Document Version**: 1.0  
**Date**: 2026-04-27  
**Status**: Framework Complete - Ready for Feature Implementation