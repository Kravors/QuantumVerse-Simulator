# QuantumVerse Simulator User Manual

**Version 0.1.1**

## Table of Contents

1. [Introduction](#introduction)
2. [Installation](#installation)
3. [Quick Start](#quick-start)
4. [User Interface Overview](#user-interface-overview)
5. [Physics Modules](#physics-modules)
6. [4D Viewport Navigation](#4d-viewport-navigation)
7. [Cosmology Simulation](#cosmology-simulation)
8. [N-Body Simulation](#n-body-simulation)
9. [Quantum Gravity](#quantum-gravity)
10. [Discovery Tools](#discovery-tools)
11. [Post-Processing](#post-processing)
12. [Troubleshooting](#troubleshooting)

---

## Introduction

QuantumVerse Simulator is a cutting-edge 4D spacetime physics engine that allows you to explore:

- **General Relativity**: Simulate black holes, gravitational waves, and curved spacetime
- **Cosmology**: Model the expanding universe, dark matter, and dark energy
- **Quantum Gravity**: Explore causal dynamical triangulations and loop quantum gravity
- **Astrophysics**: Galaxy formation, stellar evolution, and neutron star physics
- **Machine Learning**: Neural ODEs for differentiable physics simulations

### Key Features

- Real-time 4D spacetime visualization
- Interactive discovery tools for anomaly detection
- GPU-accelerated rendering
- Python integration for custom simulations
- Cross-platform support (Windows, Linux, macOS)

---

## Installation

### System Requirements

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| OS | Windows 10, Ubuntu 20.04, macOS 11 | Windows 11, Ubuntu 22.04, macOS 13 |
| CPU | 4 cores | 8+ cores |
| RAM | 8 GB | 16+ GB |
| GPU | OpenGL 3.3 | OpenGL 4.5, Vulkan, or CUDA |
| Storage | 2 GB | 10+ GB |

### Pre-built Binaries

Download the latest release from the [releases page](https://github.com/quantumverse/simulator/releases).

**Windows:**
```bash
# Download QuantumVerse-0.1.1-windows-x64.exe
# Run as Administrator
# Follow the installer prompts
```

**Linux:**
```bash
# Download QuantumVerse-0.1.1-linux-x86_64.AppImage
chmod +x QuantumVerse-0.1.1-linux-x86_64.AppImage
./QuantumVerse-0.1.1-linux-x86_64.AppImage
```

**macOS:**
```bash
# Download QuantumVerse-0.1.1-macos.dmg
# Drag QuantumVerse.app to Applications
```

### Building from Source

```bash
# Clone the repository
git clone https://github.com/quantumverse/simulator.git
cd simulator

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_USE_IMGUI=ON
cmake --build build --parallel

# Run
./build/quantumverse_imgui
```

---

## Quick Start

### Your First Simulation

1. **Launch the Simulator**
   - Run `quantumverse_imgui` from the installation directory
   - The 4D Viewport will appear

2. **Load a Preset**
   - Go to `File > Load Preset`
   - Select "Solar System" or "Schwarzschild Black Hole"

3. **Explore the 4D Viewport**
   - Use `WASD` keys to move
   - Mouse look with right-click drag
   - Scroll to zoom

4. **Adjust Parameters**
   - Use the Property Editor (right panel)
   - Modify mass, spin, and other parameters in real-time

5. **Run the Simulation**
   - Press `Space` to start/pause
   - Press `R` to reset

---

## User Interface Overview

### Main Panels

```
┌─────────────────────────────────────────────────────────────┐
│  Menu Bar                                                   │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌────────────────────────────────────┐ │
│  │ Discovery    │  │                                    │ │
│  │ Panel        │  │         4D Viewport                │ │
│  │              │  │                                    │ │
│  ├──────────────┤  └────────────────────────────────────┘ │
│  │ Cosmology    │                                         │
│  │ Panel        │  ┌──────────────┐  ┌──────────────────┐ │
│  │              │  │ Property     │  │ Performance      │ │
│  │              │  │ Editor       │  │ Monitor          │ │
│  └──────────────┘  └──────────────┘  └──────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `WASD` | Move in 4D space |
| `Shift + WASD` | Fast movement |
| `Mouse Right` | Look around |
| `Space` | Play/Pause simulation |
| `R` | Reset simulation |
| `F` | Focus on selected object |
| `G` | Toggle grid |
| `P` | Take screenshot |
| `1-4` | Switch viewport mode |

---

## Physics Modules

### General Relativity

**Schwarzschild Metric:**
```
ds² = -(1 - 2M/r)dt² + (1 - 2M/r)⁻¹dr² + r²(dθ² + sin²θ dφ²)
```

**Kerr Metric (Rotating Black Hole):**
```
ds² = -(1 - 2Mr/Σ)dt² - (4Mra sin²θ/Σ)dφdt + Σ/Δ dr² + Σ dθ² + ((r² + a²)sin²θ + 2Mra²sin⁴θ/Σ)dφ²
```

Where:
- `M` = Black hole mass
- `a` = Spin parameter
- `Σ = r² + a²cos²θ`
- `Δ = r² - 2Mr + a²`

### Cosmology

**Friedmann Equations:**
```
H² = (ȧ/a)² = (8πGρ + Λ)/3
ä/a = -(4πG/3)(ρ + 3p) + Λ/3
```

### Quantum Gravity

**Causal Dynamical Triangulations (CDT):**
- Discrete spacetime approach
- Emergent 4D geometry
- Phase transition analysis

---

## 4D Viewport Navigation

### Coordinate System

QuantumVerse uses a 4D coordinate system (t, x, y, z) where:
- **t** = Time dimension (visualized as color/width)
- **x, y, z** = Spatial dimensions

### Navigation Modes

1. **Flight Mode** (`WASD + Arrow Keys`)
   - Move forward/backward in time
   - Strafe in spatial dimensions

2. **Orbit Mode** (`Mouse + Keys`)
   - Orbit around selected objects
   - Adjust viewing angle

3. **Slice Mode** (`1-4 Keys`)
   - View 3D slices at different t values
   - Animate through time

---

## Cosmology Simulation

### Parameters

| Parameter | Description | Range |
|-----------|-------------|-------|
| Hubble Constant (H₀) | Current expansion rate | 50-100 km/s/Mpc |
| Dark Energy (Ω_Λ) | Dark energy density | 0-1 |
| Dark Matter (Ω_cdm) | Cold dark matter density | 0-1 |
| Baryon Density (Ω_b) | Ordinary matter density | 0-1 |
| Curvature (Ω_k) | Spatial curvature | -1 to 1 |

### Running a Cosmology Simulation

1. Select the **Cosmology Panel**
2. Adjust parameters using sliders
3. Click **Run Simulation**
4. Observe:
   - Scale factor evolution
   - Matter density history
   - CMB temperature evolution

---

## N-Body Simulation

### Features

- Gravitational N-body integration
- Dark matter halo modeling
- Galaxy collision simulations
- Real-time force visualization

### Setup

1. Go to **N-Body Panel**
2. Add bodies:
   - Sun-like star
   - Earth-like planet
   - Moon-like satellite
3. Set initial conditions:
   - Position (x, y, z)
   - Velocity (vx, vy, vz)
   - Mass and radius

### Validation

The simulator validates against known results:
- **Mercury Precession**: 43.0 arcsec/century ✓
- **Light Deflection**: 1.75 arcsec ✓
- **Gravitational Redshift**: Δλ/λ = GM/(c²r) ✓

---

## Quantum Gravity

### Modules Available

1. **Causal Dynamical Triangulations (CDT)**
   - Discrete spacetime triangulation
   - Spectral dimension analysis

2. **Loop Quantum Gravity (LQG)**
   - Spin network representation
   - Area and volume operators

3. **Group Field Theory (GFT)**
   - Tensorial field theory
   - Quantum cosmology

### Using Quantum Gravity Tools

1. Select **Quantum Gravity Panel**
2. Choose theory (CDT, LQG, or GFT)
3. Set discretization parameters
4. Run simulation
5. Analyze results in the output panel

---

## Discovery Tools

### Anomaly Detection

The Discovery Engine automatically scans for:
- **Metric deviations** from GR predictions
- **Quantum gravity signatures**
- **Exotic matter indicators**
- **Dark matter distributions**

### Discovery Panels

| Panel | Purpose |
|-------|---------|
| Galactic Rotation Curve Scanner | Detect dark matter anomalies |
| Black Hole Jet Anomaly Recogniser | Identify jet irregularities |
| Fine Structure Constant Drift Observatory | Search for varying constants |
| Holographic Duality Lab | Explore AdS/CFT correspondence |

---

## Post-Processing

### Screenshots

```
F12 - Take screenshot
```

Screenshots are saved to:
- Windows: `%USERPROFILE%\Pictures\QuantumVerse\Screenshots`
- Linux: `~/Pictures/QuantumVerse/Screenshots`
- macOS: `~/Pictures/QuantumVerse/Screenshots`

### Video Recording

1. Enable recording in **Settings > Output**
2. Set recording parameters:
   - Resolution
   - FPS
   - Codec
3. Press `Ctrl+R` to start/stop

### Data Export

Export simulation data:
- **CSV**: Time series data
- **JSON**: Configuration and state
- **VTK**: 3D visualization data
- **ONNX**: Neural network models

---

## Troubleshooting

### Common Issues

**Application won't start:**
- Ensure graphics drivers are updated
- Check that OpenGL 3.3+ is supported
- Verify all DLLs are present (Windows)

**Poor performance:**
- Reduce particle count
- Lower rendering quality
- Enable LOD (Level of Detail)

**Physics results incorrect:**
- Check units are consistent
- Verify initial conditions
- Run validation tests

### Getting Help

- **Documentation**: [docs.quantumverse.dev](https://docs.quantumverse.dev)
- **GitHub Issues**: [github.com/quantumverse/simulator/issues](https://github.com/quantumverse/simulator/issues)
- **Discord**: [discord.gg/quantumverse](https://discord.gg/quantumverse)

---

## License

QuantumVerse Simulator is released under the MIT License.

---

*Last updated: June 2026*