# QuantumVerse Simulator v3.9.0 - Release Notes

## 🎉 Major Milestone: VR Multi-User Sessions & CI Hygiene

This release merges real WebSocket-based VR multi-user session management, volumetric disk rendering, Pareto archive performance improvements, and CI workflow cleanup.

## ✅ Validation Results

| Metric | Value |
|--------|-------|
| **Test pass rate** | **125/125 (100%)** |
| **Discovery instruments** | **16** |
| **VR backend** | Real multi-user WebSocket sessions + OpenXR |
| **UI** | 100% Qt QML |
| **Build** | Zero warnings under `/W4 /WX` |

## 🚀 New Features

### VR Multi-User Sessions (#73)
- Real `MultiUserServer` with Qt WebSocket session management
- Session create/join/leave flow
- Head pose and controller state broadcasting
- Stale participant cleanup (60s timeout)
- Unit tests: `MultiUserServerTest`

### Volumetric Disk Color (#68)
- Color-mapped accretion-disk rendering
- Improved disk_renderer integration

### Pareto Archive Performance (#69)
- Batch updates to the Pareto archive
- Reduced per-evaluation overhead in `optimizeWithGradient()`

### CI Hygiene
- Extracted canary exclusions to `.github/workflows/_canary-exclusions.txt` (#71)
- Fixed static-analysis `run-clang-tidy -export-fixes` PyYAML issue (#70)
- Fixed coverage job `QT_QPA_PLATFORM=offscreen` on Ubuntu (#70)
- Removed redundant CodeQL Advanced workflow (disk-space failures tracked in #72)
- Removed unused Runtime Invariant Monitoring job (tracked in #74)

## 🧪 Running Tests

```bash
cd build
ctest -C Release --output-on-failure
```

## 📄 License

MIT License - see `LICENSE` file for details.

---

# QuantumVerse Simulator v2.0.0 (Genesis) - Release Notes

## 🎉 Production Release

The QuantumVerse Simulator is now ready for production use. This release includes the complete 4D spacetime physics engine with quantum gravity, cosmology, and discovery tools.

## ✅ Validation Results

| Test Suite | Status |
|------------|--------|
| All 68 Unit Tests | ✅ Passed |
| GR Validation (Mercury, Light Deflection, Redshift) | ✅ Passed |
| Quantum Gravity (CDT, LQG, GFT) | ✅ Passed |
| Cosmology (FRW, Galaxy Formation) | ✅ Passed |
| Memory Safety | ✅ Passed |

## 📦 Release Artifacts

| Platform | Artifact |
|----------|----------|
| Windows | `QuantumVerse-2.0.0-windows-x64.exe` |
| Linux | `QuantumVerse-2.0.0-linux-x86_64.AppImage` |
| macOS | `QuantumVerse-2.0.0-macos.dmg` |
| Docker | `quantumverse:2.0.0` |

## 🚀 New Features

### Core Physics
- **4D Spacetime Visualization**: Real-time rendering of 4D manifolds
- **General Relativity**: Schwarzschild, Kerr, and FRW metrics
- **Quantum Gravity**: CDT, LQG, GFT, and Causal Sets
- **Cosmology**: Dark matter, dark energy, and galaxy formation
- **Astrophysics**: Stellar structure, supernova, neutron stars

### Discovery Tools
- **Anomaly Detection Engine**: Automatic physics anomaly detection
- **Holographic Duality Lab**: AdS/CFT exploration
- **Symbolic Regression**: AI-powered equation discovery
- **RL Discovery Agent**: Reinforcement learning for physics discovery

### UI/UX
- **4D Viewport**: Interactive 4D navigation with WASD+Mouse
- **Property Editor**: Real-time parameter adjustment
- **Discovery Panel**: Anomaly timeline and visualization
- **Performance Monitor**: Real-time FPS and memory usage

## 🔧 Build Instructions

### Windows
```cmd
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### Linux
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_USE_IMGUI=ON
cmake --build build --parallel
```

### macOS
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### Docker
```bash
docker build -t quantumverse:2.0.0 .
docker run --rm -it quantumverse:2.0.0
```

## 📚 Documentation

- **User Manual**: `docs/user_manual/README.md`
- **API Documentation**: `docs/API_Documentation.md`
- **Physics Validation**: `docs/VALIDATION_PLAN.md`

## 🧪 Running Tests

```bash
cd build
ctest -C Release --output-on-failure
```

## 🤝 Contributing

Please read `CONTRIBUTING.md` for development guidelines.

## 📄 License

MIT License - see `LICENSE` file for details.

---

**Thank you for using QuantumVerse Simulator!**

For support: quantumverse@example.com
For issues: https://github.com/quantumverse/simulator/issues