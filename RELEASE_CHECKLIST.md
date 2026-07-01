# QuantumVerse Simulator v2.0.0 Release Checklist

## Pre-Release Verification

- [x] All 68 tests pass (100% pass rate)
- [x] Code formatted with clang-format
- [x] Static analysis passes (clang-tidy, cppcheck)
- [x] Memory safety tests pass
- [x] GR validation tests pass (Mercury, light deflection, redshift, frame dragging)
- [x] Documentation complete (user manual, API docs)
- [x] Release notes drafted

## Build Artifacts

### Windows
- [ ] Run `scripts/build-windows-installer.bat 2.0.0`
- [ ] Verify installer runs on clean Windows machine
- [ ] Test installation and first launch

### Linux
- [ ] Run `./scripts/build-appimage.sh 2.0.0`
- [ ] Verify AppImage runs on Ubuntu 22.04
- [ ] Test on multiple Linux distributions

### macOS
- [ ] Build DMG package
- [ ] Verify code signing (if applicable)
- [ ] Test on macOS 13+

### Docker
- [ ] Build `docker build -t quantumverse:2.0.0 .`
- [ ] Test container runs correctly
- [ ] Push to Docker Hub (optional)

## Git Operations

- [ ] Commit all changes
- [ ] Create annotated tag:
  ```bash
  git add .
  git commit -m "Release v2.0.0: QuantumVerse Genesis"
  git tag -a v2.0.0 -m "QuantumVerse Simulator v2.0.0 (Genesis) - Production Release"
  ```
- [ ] Push to origin:
  ```bash
  git push origin main --tags
  ```

## GitHub Release

- [ ] Create release on GitHub
- [ ] Upload all artifacts
- [ ] Paste release notes
- [ ] Mark as pre-release (if applicable)

## Post-Release

- [ ] Update version in documentation
- [ ] Announce on social media
- [ ] Notify early users
- [ ] Monitor for issues

## Quick Commands

```bash
# Build all artifacts
cmake -B build -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_USE_IMGUI=ON
cmake --build build --parallel

# Run tests
cd build && ctest -C Release --output-on-failure

# Format code
cmake --build build --target check-format
```

## Contact

For issues: quantumverse@example.com
Documentation: docs/user_manual/README.md