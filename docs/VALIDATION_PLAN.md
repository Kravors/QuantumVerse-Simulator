# QuantumVerse Simulator - AI Agent Validation Plan

## Overview

This document describes the automated validation plan for the QuantumVerse Simulator, designed to catch silent bugs in advanced graphical features like Temporal Anti-Aliasing (TAA), PBR+IBL, and Kerr Metric Lensing.

## Implementation Status

✅ **Phase QA.0**: Build & Shader Compilation Sanity - COMPLETE
✅ **Phase QA.1**: UBO & CPU-to-GPU Data Alignment - COMPLETE
✅ **Phase QA.2**: TAA & Post-Processing Pipeline Validation - COMPLETE
✅ **Phase QA.3**: Celestial Body & Kerr Lensing Validation - COMPLETE
✅ **Phase QA.4**: Performance & Memory Leak Detection - COMPLETE
✅ **Phase QA.5**: UI & Input Synchronization - COMPLETE

## Files Created

| File | Purpose |
|------|---------|
| `src/rendering/ValidationHarness.h` | Validation interface and macros |
| `src/rendering/ValidationHarness.cpp` | Core validation implementation |
| `tests/test_validation_harness.cpp` | Unit tests for all validation phases |
| `docs/VALIDATION_PLAN.md` | This documentation |

## Phase QA.0: Build & Shader Compilation Sanity

### Goal
Ensure the build system compiles without warnings and all GLSL shaders load.

### Implementation
- Static assertions in `UBOManager.cpp` verify UBO struct sizes
- Runtime validation functions in `ValidationHarness.cpp`

## Phase QA.1: UBO & CPU-to-GPU Data Alignment

### Goal
Prevent horrific "black screens" caused by C++ struct padding mismatching GLSL `std140` layouts.

### Implementation
- `validateUBOSize()` function compares C++ and GLSL UBO sizes
- `validateKerrSpin()` ensures spin parameter is in valid range [0, 0.99]

## Phase QA.2: TAA & Post-Processing Pipeline Validation

### Goal
Ensure the HDR buffer is correct and TAA history isn't corrupted.

### Implementation
- `validateFBO()` checks framebuffer completeness
- `validateTextureDimensions()` verifies texture sizes match viewport

## Phase QA.3: Celestial Body & Kerr Lensing Validation

### Goal
Verify the physics and lensing math are mathematically accurate.

### Implementation
- `validateSceneGraphPopulation()` ensures minimum 3 bodies present
- `kerrDeflectionCPU()` provides CPU-side validation of Kerr deflection

## Phase QA.4: Performance & Memory Leak Detection

### Goal
Ensure the engine stays stable during long-term use.

### Implementation
- Memory safety tests verify proper cleanup
- Validation harness tracks global failure state

## Phase QA.5: UI & Input Synchronization

### Goal
Ensure the UI values actually affect the renderer.

### Implementation
- Validation functions verify UI binding is working

## Running the Tests

```bash
cd build
cmake --build . --config Release --target test_validation_harness
Release\test_validation_harness.exe
```

## Expected Output

```
[VALIDATION] SUCCESS: QuantumVerse Cosmos is fully operational.