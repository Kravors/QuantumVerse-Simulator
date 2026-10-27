# QuantumVerse Simulator v7.2 - Release Notes

## 🎉 v7.2 — Kerr Observables + Deliverable Installer (2026-10-26)

This release delivers a fully working non-admin installer, verified Kerr black hole observables, a grid deformation fix, and an end-to-end discovery paper pipeline. The installer is published as a GitHub Release asset.

**Tag**: `v7.2-final`  
**Installer**: `QuantumVerse-7.2-Setup.exe` (~32.4 MB, unsigned, non-admin install to `%LOCALAPPDATA%\Programs\QuantumVerse\`)

## ✅ Validation Results

| Metric | Value |
|--------|-------|
| **Test pass rate** | All core GR validation tests pass |
| **Discovery instruments** | 16 |
| **UI** | 100% Qt QML |
| **Build** | Zero warnings under `/W4 /WX` |
| **Installer size** | ~32.4 MB |

## 🚀 New Features & Fixes

### Grid Deformation Fix
Fixed a mass unit slip in the vertex dispatch (`kg→solar-mass`). Grid now deforms correctly (`gridZRange = -6.4..-0.0708`, was `-1..-1`).

### Kerr Observables Verified
Full Kerr Kretschmann scalar (`K = 6.973e-21`) and gravitational redshift (`z = 0.1175`) match full Kerr formulas at r=1e5m, θ=0, a/M=0.9 — verified to 3+ significant figures.

### Discovery Paper Pipeline (End-to-End)
Scan → abstract → PDF in `Documents\QuantumVerse\output\discovery_paper.pdf` (writable Documents path via `QStandardPaths::DocumentsLocation`). LaTeX special-character escaping implemented in `PaperGenerator::escapeLatex()`.

### Non-Admin Installer (32.4 MB)
NSIS-based installer at `%LOCALAPPDATA%\Programs\QuantumVerse\` — no elevation required. Excludes `.h5/.hdf5/.pth/.ckpt/.onnx` training artifacts (852 MB removed from repo). Uninstall removes install dir + shortcuts; `Documents\QuantumVerse\output` survives.

### Metric Label Honesty
ComboBox shows actual metric type ("Kerr"); spin display shows actual spin value (0.9). Fixed `setBlackHoleMassSolar()` passing kg where solar masses were expected.

### Physics Validator Fixes
- Kretschmann mass unit: removed double-conversion (`M = m_massKg`, was `× 1.98847e30`)
- Validator rejects `-1..-1` uninitialized grid sentinel
- All 5 validation checks pass; red banner eliminated

### Probe Location Fix
Probe moved from origin (r=0 curvature pole) to r=1e5m, θ=0 — avoids curvature singularity while remaining in weak-field regime for verification.

### Kerr Metric Fixes
- `curvatureScalars()` now uses full Kerr Kretschmann, not Schwarzschild approximation
- `rs = 2GM/c⁴` → `2GM/c²` (was 1e-14 m for 1 M☉, now ≈2952 m)
- `setBlackHoleMass()` clamped to `[0.1, 10.0] M☉`

### Lensing Shader Camera Convention Fix
Three issues fixed in the Kerr black hole shadow rendering:
1. **Theta convention**: `--cam-el 0` → equator (θ=π/2), not pole
2. **Camera world position**: Uses QML camera's actual world position, not spherical reconstruction around origin
3. **Camera direction/basis**: `u_cameraDir = normalize(bhPos - cameraPos)` (looking at BH), orthonormal basis for `u_cameraUp`

## ⚠ Windows SmartScreen

The installer is unsigned. On first launch, Windows will show "Windows protected your PC". Click **More info** → **Run anyway** to proceed.

## Known Issues (Deferred to v7.3)

- Texture array GL warning 0x501 (cosmetic)
- ONNX RL model not deployed (graceful fail)
- Validator uses Schwarzschild expected-K with 50% tolerance

## 🧪 Running Tests

```bash
cd build
ctest -C Release --output-on-failure
```

## 📄 License

MIT License - see `LICENSE` file for details.

---

# QuantumVerse Simulator v3.9.0 - Release Notes

## 🎉 Major Milestone: VR Multi-User Sessions & CI Hygiene

This release merges real WebSocket-based VR multi-user session management, volumetric disk rendering, Pareto archive performance improvements, and CI workflow cleanup.

**Version alignment:** Project version and CPack package version are now aligned to `3.9.0` across `CMakeLists.txt`, CI workflows, and documentation. Previous CPack remnant `5.5.2` has been removed.

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
