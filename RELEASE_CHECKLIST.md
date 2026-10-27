# QuantumVerse Simulator v7.2 Release Checklist

## Pre-Release Verification

- [x] All core GR validation tests pass (Mercury, light deflection, redshift, frame dragging)
- [x] Code formatted with clang-format
- [x] Static analysis passes (clang-tidy, cppcheck)
- [x] Memory safety tests pass
- [x] Documentation complete (user manual, API docs, deployment guide)
- [x] Release notes drafted (RELEASE_NOTES.md)
- [x] Grid deformation verified (`gridZRange = -6.4..-0.0708`)
- [x] Kerr observables verified (K=6.973e-21, z=0.1175)
- [x] Discovery paper pipeline end-to-end (scan → abstract → PDF)
- [x] Installer built (32.4 MB, unsigned NSIS)
- [x] Non-admin install verified
- [x] Uninstall verified (output directory survives)

## Build Artifacts

### Windows
- [x] Run `cmake -B build -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_TESTS=ON`
- [x] Run `cmake --build build --parallel`
- [x] Run `ctest -C Release --output-on-failure`
- [x] Run `deploy.bat`
- [x] Build installer: `cd installer && iscc quantumverse.iss`
- [x] Verify installer runs on clean Windows machine
- [x] Test installation and first launch

## Git Operations

- [x] Commit all changes (`c8c2d03`: `chore: ignore training_data and *.pyd binaries`)
- [x] Create annotated tag:
  ```bash
  git tag -a v7.2-shipped -m "v7.2-shipped: installer + 15 fixes"
  ```
  *(Note: tag reused as `v7.2-final` due to immutability rules on GitHub)*
- [x] Push to origin:
  ```bash
  git push -u origin main
  git push origin v7.2-shipped
  ```

## GitHub Release

- [x] Create release on GitHub (`v7.2-final`)
- [x] Upload installer asset (`QuantumVerse-7.2-Setup.exe`, 33,968,842 bytes)
- [x] Paste release notes
- [x] Mark as latest release

## Post-Release

- [x] Update version in documentation (README.md, DEVELOPMENT_STATUS.md, WINDOWS_DEPLOYMENT.md)
- [x] Verify download URL resolves: `https://github.com/Kravors/QuantumVerse-Simulator/releases/download/v7.2-final/QuantumVerse-7.2-Setup.exe`

## Quick Commands

```bash
# Build all artifacts
cmake -B build -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_TESTS=ON
cmake --build build --parallel

# Run tests
cd build && ctest -C Release --output-on-failure

# Format code
cmake --build build --target check-format

# Build installer
cd installer && iscc quantumverse.iss
```

## Contact

For issues: quantumverse@example.com
Documentation: docs/user_manual/README.md
Release: https://github.com/Kravors/QuantumVerse-Simulator/releases/tag/v7.2-final