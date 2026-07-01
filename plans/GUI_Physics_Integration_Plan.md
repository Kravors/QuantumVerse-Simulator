# GUI-Physics Integration Plan

## Overview
This document outlines how the new GUI components integrate with existing QuantumVerse physics modules.

## Component Integration Matrix

### 1. TimelineBar ↔ Simulation Engine
**Connection:** `simulationTime` variable in `UI4D_ImGui`
- TimelineBar controls simulation time via `setCurrentTime()` and `setPlaybackSpeed()`
- `simulationTime` is used by physics integrators for time-dependent calculations
- Integration point: `UI4D_ImGui::renderTimelineBar()` updates `simulationTime`

**Required Physics Hooks:**
- `MetricTensor::evolve(double time)` - for time-dependent metrics
- `GeodesicIntegrator::setTime(double time)` - for trajectory evolution

### 2. ObjectBrowser ↔ Spacetime Objects
**Connection:** `ObjectBrowser` maintains list of celestial bodies
- Each object has position, mass, radius, and type
- Integration with `MetricTensor` for object-based metrics

**Required Physics Hooks:**
- `MetricTensor::addPointMass(double mass, std::array<double, 4> position)`
- `MetricTensor::removeObject(size_t index)`
- `MetricTensor::getObjects()` - returns list of objects for display

### 3. PropertyEditor ↔ Object Properties
**Connection:** `PropertyEditor::setObject()` and callbacks
- Edits mass, radius, position of selected objects
- Triggers physics recalculation on apply

**Required Physics Hooks:**
- `MetricTensor::updateObject(size_t index, const ObjectProperties& props)`
- `CurvatureRenderer::recalculate()` - after property changes

### 4. FloatingPanels ↔ Curvature/Geodesic Systems
**Connection:** `FloatingPanels` displays real-time physics data
- Curvature overlay shows grid deformation from `CurvatureRenderer`
- Geodesic tracer shows particle paths from `GeodesicIntegrator`
- Anomaly alerts from `DiscoveryProbe`

**Required Physics Hooks:**
- `CurvatureRenderer::getCurvatureAt(double x, double y, double z)`
- `GeodesicIntegrator::getGeodesics()` - returns current geodesic paths
- `DiscoveryProbe::checkAnomalies()` - returns list of detected anomalies

### 5. ContextMenu ↔ Physics Tools
**Connection:** `ContextMenu` triggers physics operations
- "Test GR" - runs GR validation tests
- "Measure Gravity" - computes local gravitational field
- "Check Equivalence" - tests equivalence principle

**Required Physics Hooks:**
- `MetricTensor::computeChristoffel()` - for gravity measurement
- `GeodesicIntegrator::testEquivalencePrinciple()`
- `ValidationSuite::runGRTests()`

## Data Flow Architecture

```
[GUI Components] → [UI4D_ImGui] → [Physics Modules] → [Renderers] → [Display]
                             ↘
                              → [Data Storage]
```

### Key Integration Points:

1. **Object Selection Flow:**
   - ObjectBrowser → PropertyEditor (on selection)
   - PropertyEditor → MetricTensor (on apply)
   - MetricTensor → CurvatureRenderer (on change)

2. **Time Evolution Flow:**
   - TimelineBar → UI4D_ImGui::simulationTime
   - simulationTime → MetricTensor::evolve()
   - MetricTensor → CurvatureRenderer::update()

3. **Anomaly Detection Flow:**
   - DiscoveryProbe → FloatingPanels (anomaly alerts)
   - FloatingPanels → UI display

## Implementation Checklist

- [ ] Add `MetricTensor::addPointMass()` method
- [ ] Add `MetricTensor::updateObject()` method
- [ ] Add `MetricTensor::evolve()` for time-dependent metrics
- [ ] Add `GeodesicIntegrator::getGeodesics()` for path visualization
- [ ] Add `DiscoveryProbe::checkAnomalies()` for alert system
- [ ] Connect ObjectBrowser selection to PropertyEditor
- [ ] Connect PropertyEditor apply to MetricTensor update
- [ ] Connect TimelineBar time to simulation time propagation

## Testing Requirements

1. **Object Property Changes:**
   - Modify object mass → verify curvature changes
   - Move object position → verify geodesic deflection

2. **Time Evolution:**
   - Play/pause timeline → verify metric evolution
   - Speed control → verify correct time scaling

3. **Anomaly Detection:**
   - Trigger known anomaly → verify alert appears
   - Clear anomaly → verify alert disappears