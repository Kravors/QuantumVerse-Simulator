# Plan6 Implementation Mapping

## Overview

This document maps the 10 instruments from Plan6.md to the QuantumVerse Simulator framework. All instruments leverage the existing `TheoryPlugin` architecture and `DiscoveryEngine` to enable rapid implementation of new discovery capabilities.

## Framework Integration

### Base Classes Used
- `TheoryPlugin` - Abstract base for alternative gravity theories
- `DiscoveryEngine` - AI-driven anomaly detection and hypothesis management
- `UI4D` - 4D visualization and interaction
- `MetricTensor` - Spacetime metric calculations
- `Event4D` - Spacetime event representation

### Implementation Pattern

Each Plan6 instrument follows this pattern:

```cpp
class Plan6InstrumentX : public TheoryPlugin {
public:
    // Feature-specific metric computation
    MetricTensor computeMetric(const Event4D& event, 
                                const Parameters& params) override {
        // Implement instrument-specific spacetime metric
    }
    
    // Anomaly detection logic
    bool predictsNewPhenomena(const ObservationData& data) override {
        // Compare simulation with observations
        // Return true if anomaly detected
    }
    
    // Visualization parameters
    VisualizationConfig getVisualizationConfig() override {
        return {color, opacity, scale};
    }
};

// Registration
void registerPlan6Instruments(DiscoveryEngine& engine) {
    engine.registerPlugin(
        std::make_unique<Plan6InstrumentX>(params)
    );
}
```

## Instrument Mappings

### 1. Globular Cluster Color–Magnitude Diagram New-Physics Scanner

**Framework Components:**
- `TheoryPlugin` - Exotic energy loss channels
- `DiscoveryEngine` - Statistical comparison with HST/JWST data
- `UI4D` - CMD comparator overlay

**Key Features:**
- Stellar evolution tracks in 4D curved spacetime
- Modified cooling channels (axions, hidden photons)
- Synthetic CMD generation
- Bayesian comparison with observations

**Implementation File:** `src/discovery/Instruments/GlobularClusterScanner.h`

**Status:** Framework-ready, 2-3 weeks implementation

---

### 2. Cosmic Infrared/Optical Background Fluctuation Anomaly Mapper

**Framework Components:**
- `TheoryPlugin` - Exotic background component models
- `DiscoveryEngine` - Power spectrum analysis
- `UI4D` - 3D volume visualization with exotic sources

**Key Features:**
- Cosmological volume simulation
- Galaxy clustering + exotic components
- EBL fluctuation power spectrum
- Comparison with Euclid/SPHEREx/Roman data

**Implementation File:** `src/discovery/Instruments/EBLFluctuationMapper.h`

**Status:** Framework-ready, 3-4 weeks implementation

---

### 3. Magnetar Burst High-Energy Spectral QED Laboratory

**Framework Components:**
- `TheoryPlugin` - QED processes in ultra-strong B-fields
- `MetricTensor` - Magnetar spacetime metric
- `DiscoveryEngine` - Spectral fitting

**Key Features:**
- Photon propagation in B > B_critical
- QED effects: photon splitting, vacuum birefringence
- Synthetic spectra generation
- Comparison with Fermi-GBM/Swift data

**Implementation File:** `src/discovery/Instruments/MagnetarQEDLab.h`

**Status:** Framework-ready, 3-4 weeks implementation

---

### 4. Solar Axion Helioscope Digital Twin

**Framework Components:**
- `TheoryPlugin` - Axion production and conversion
- `DiscoveryEngine` - Signal rate comparison
- `UI4D` - Axion world-line visualization

**Key Features:**
- Solar axion flux calculation
- Gravitational and thermal broadening
- CAST/IAXO conversion probability
- Spectral shape fitting

**Implementation File:** `src/discovery/Instruments/SolarAxionHelioscope.h`

**Status:** Framework-ready, 2-3 weeks implementation

---

### 5. Lunar Laser Ranging Dark Force Ephemeris Constraint Engine

**Framework Components:**
- `TheoryPlugin` - Fifth-force potential
- `GeodesicIntegrator` - Earth-Moon geodesics
- `DiscoveryEngine` - LLR data fitting

**Key Features:**
- 4D geodesic integration with dark force
- Range residual calculation
- APOLLO/LLR comparison
- Eöt-Wash constraint comparison

**Implementation File:** `src/discovery/Instruments/LLRDarkForceEngine.h`

**Status:** Framework-ready, 2-3 weeks implementation

---

### 6. Gravitational-Wave Phase Shift from Dark-Matter Accretion on Binaries

**Framework Components:**
- `TheoryPlugin` - Dark matter halo models
- `MetricTensor` - Binary + DM spacetime
- `DiscoveryEngine` - Waveform comparison

**Key Features:**
- Binary evolution with DM overdensity
- Gravitational waveform extraction
- LIGO/Virgo/KAGRA template matching
- Bayesian evidence calculation

**Implementation File:** `src/discovery/Instruments/DMBinaryWaveform.h`

**Status:** Framework-ready, 3-4 weeks implementation

---

### 7. Cosmic Dawn 21-cm Tomography with Exotic Radio Backgrounds

**Framework Components:**
- `TheoryPlugin` - Exotic radio background models
- `DiscoveryEngine` - 21-cm signal calculation
- `UI4D` - IGM spin temperature visualization

**Key Features:**
- Recombination history modification
- 21-cm global signal
- EDGES/SARAS/HERA/LOFAR/SKA comparison
- Exotic energy injection scenarios

**Implementation File:** `src/discovery/Instruments/CosmicDawn21cm.h`

**Status:** Framework-ready, 3-4 weeks implementation

---

### 8. Intergalactic Magnetic Helicity Detector via Extragalactic Rotation Measures

**Framework Components:**
- `TheoryPlugin` - Helical magnetic field models
- `MetricTensor` - Cosmic web photon propagation
- `DiscoveryEngine` - RM sky map analysis

**Key Features:**
- Large-scale structure with helical B-fields
- Polarized rotation measures
- LOFAR/MWA/SKA sky map comparison
- Helicity correlation statistics

**Implementation File:** `src/discovery/Instruments/MagneticHelicityDetector.h`

**Status:** Framework-ready, 3-4 weeks implementation

---

### 9. Type Ia Supernova Standardisation Residual Anomaly Finder

**Framework Components:**
- `TheoryPlugin` - Varying constant models
- `DiscoveryEngine` - Hierarchical Bayesian fit
- `UI4D` - Hubble diagram with residuals

**Key Features:**
- Supernova light curve simulation
- Varying G, α effects
- Pantheon+/DES dataset fitting
- Cosmic-time-dependent parameters

**Implementation File:** `src/discovery/Instruments/SNeAnomalyFinder.h`

**Status:** Framework-ready, 2-3 weeks implementation

---

### 10. Binary Pulsar Timing Anomaly Constraint Laboratory

**Framework Components:**
- `TheoryPlugin` - Alternative gravity theories
- `GeodesicIntegrator` - Relativistic binary orbits
- `DiscoveryEngine` - Timing residual analysis

**Key Features:**
- Scalar-tensor, bigravity, extra dimension models
- Orbital decay rate calculation
- Hulse-Taylor/double pulsar/PSR J0348+0432 comparison
- Bayesian model selection

**Implementation File:** `src/discovery/Instruments/PulsarTimingLab.h`

**Status:** Framework-ready, 3-4 weeks implementation

---

## Integration Steps

### For Each Instrument:

1. **Create Plugin Class**
   - Inherit from `TheoryPlugin`
   - Implement `computeMetric()`
   - Implement `predictsNewPhenomena()`

2. **Register with DiscoveryEngine**
   - Add to `registerPlan6Instruments()`
   - Configure parameters

3. **Add UI Integration**
   - Extend `UI4D` for visualization
   - Add control panel
   - Configure color schemes

4. **Connect to Data**
   - Implement data loading
   - Configure comparison pipeline
   - Set statistical thresholds

5. **Testing**
   - Unit tests for metric computation
   - Integration tests with DiscoveryEngine
   - Validation against known results

## Dependencies

### External Data Required:
- HST/JWST globular cluster catalogs
- Euclid/SPHEREx/Roman EBL data
- Fermi-GBM/Swift magnetar spectra
- CAST/IAXO axion search results
- APOLLO/LLR lunar ranging data
- LIGO/Virgo/KAGRA GW catalogs
- EDGES/SARAS/HERA 21-cm data
- LOFAR/MWA/SKA RM surveys
- Pantheon+/DES SNe catalogs
- Binary pulsar timing archives

### Internal Dependencies:
- `TheoryPlugin` base class
- `DiscoveryEngine` analysis pipeline
- `UI4D` visualization system
- `MetricTensor` computation engine
- `GeodesicIntegrator` for orbits

## Estimated Implementation Time

| Instrument | Complexity | Time Estimate |
|------------|-----------|---------------|
| Globular Cluster Scanner | Medium | 2-3 weeks |
| EBL Fluctuation Mapper | High | 3-4 weeks |
| Magnetar QED Lab | High | 3-4 weeks |
| Solar Axion Helioscope | Medium | 2-3 weeks |
| LLR Dark Force Engine | Medium | 2-3 weeks |
| DM Binary Waveform | High | 3-4 weeks |
| Cosmic Dawn 21-cm | High | 3-4 weeks |
| Magnetic Helicity Detector | High | 3-4 weeks |
| SNe Anomaly Finder | Medium | 2-3 weeks |
| Pulsar Timing Lab | High | 3-4 weeks |

**Total**: 20-35 weeks (parallelizable across team)

## Risk Assessment

**Low Risk:**
- Well-defined framework
- Proven architecture
- Modular design
- External data available

**Medium Risk:**
- Complex physics implementations
- Data quality/availability
- Computational requirements

**Mitigation:**
- Incremental implementation
- Thorough testing
- Expert consultation
- Performance optimization

## Conclusion

All 10 Plan6 instruments can be implemented using the existing QuantumVerse framework. The `TheoryPlugin` architecture provides a clean interface for adding new physics models, while the `DiscoveryEngine` handles anomaly detection and hypothesis management. The 4D UI system enables intuitive visualization of complex phenomena.

**Framework Readiness**: ✓ Complete  
**Implementation Path**: Clear  
**Risk Level**: Low-Medium  
**Timeline**: 20-35 weeks (parallelizable)

---

**Document Version**: 1.0  
**Date**: 2026-04-27  
**Status**: Framework Ready ✓