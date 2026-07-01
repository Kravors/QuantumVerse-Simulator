# String Cosmology Module

## Overview

The String Cosmology module implements string-theoretic extensions to cosmological models in the QuantumVerse simulator. This module provides tools for exploring effects of extra dimensions, dilaton fields, and brane inflation.

## Components

### DilatonField (`src/stringcosmology/DilatonField.h`)

Evolves the dilaton field in an FRW background using the equation of motion:

```
φ̈ + 3Hφ̇ + dV/dφ = 0
```

**Key Features:**
- Exponential potential: V(φ) = V₀ * exp(-p*φ)
- RK4 integration for field evolution
- Slow-roll parameter computation
- E-folding number calculation

**Configuration Parameters:**
- `initial_phi`: Initial dilaton value
- `initial_dot_phi`: Initial velocity
- `potential_amplitude`: V₀ in potential
- `potential_exponent`: p in potential
- `string_tension`: String tension parameter (α')

### ModuliStabilisation (`src/stringcosmology/ModuliStabilisation.h`)

Computes and finds minima of the moduli potential arising from flux compactifications and anti-D3 brane uplifts.

**Key Features:**
- KKLT-type potential: V(φ) = V_flux + V_antid3 * exp(-φ)
- Gradient descent optimization
- Stabilization detection

**Configuration Parameters:**
- `initial_modulus`: Initial modulus value
- `flux_parameter`: Flux quantum number
- `anti_d3_tension`: Anti-D3 brane tension
- `warp_factor`: Warp factor in throat

### BraneInflationModel (`src/stringcosmology/BraneInflationModel.h`)

Implements DBI inflation with brane separation as the inflaton.

**Key Features:**
- DBI action formulation
- cosh-potential for inflation
- Scalar spectral index and tensor-to-scalar ratio computation

**Configuration Parameters:**
- `initial_separation`: Initial brane separation
- `brane_tension`: Brane tension (T)
- `tension_parameter`: T_p parameter
- `warp_exponent`: Warp factor exponent (p)

### StringCosmologyModel (`src/stringcosmology/StringCosmologyModel.h`)

Orchestrator class that combines all string cosmology components.

**Key Features:**
- Unified interface for all components
- Stability checking
- Equation of state computation

## Usage Example

```cpp
#include "stringcosmology/StringCosmologyModel.h"

using namespace quantumverse::stringcosmology;

// Configure the model
StringCosmologyConfig config;
config.dilaton.initial_phi = 1.0;
config.dilaton.potential_amplitude = 1.0;
config.dilaton.potential_exponent = 2.0;

// Create and initialize
StringCosmologyModel model(config);
model.initialize();

// Evolve the system
model.evolve(1.0);

// Check stability
if (model.isStable()) {
    // System is stabilized
}
```

## Physics References

1. Giddings, S. B. (1994). "Lectures on 2D gravity and 2D quantum field theory"
2. Kachru, S., Kallosh, A., Linde, A., & Trivedi, N. (2003). "De Sitter vacua in string theory"
3. Douglas, M. R. (2003). "The statistics of string/M theory vacua"

## Integration Points

- Connects to `spacetime/DilatonMetric.h` for 2D dilaton gravity
- Uses `physics/GeodesicIntegrator.h` for geodesic evolution
- Integrates with `ui_imgui/CosmologyPanel.cpp` for UI controls