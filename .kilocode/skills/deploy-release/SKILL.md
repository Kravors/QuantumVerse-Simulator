# Upgraded Deploy Release Skill

**File**: `.kilo/skills/deploy-release/SKILL.md`  
**Upgrade Version**: 2.0.0 ("Multi‑Platform & Automated Delivery")  
**Date**: 2026-05-28  

This upgrade transforms a simple release checklist into a **professional, automated release pipeline** supporting multiple platforms (Windows, Linux, macOS), artifact signing, changelog generation, GitHub Releases, and integration with CI/CD.

---

## Name
**deploy-release**

## Description
Prepare and package a QuantumVerse release – validate, bump version, build packages for Windows/Linux/macOS, sign binaries, create Git tag, generate release notes, and publish to GitHub Releases or artifact repository. Use when the user says "release", "deploy", "publish", "ship", "package", or "tag version".

## Metadata
```yaml
project: QuantumVerse
version: 2.0.0
author: DHIAEDDINE0223
dependencies:
  - git >= 2.30
  - cmake >= 3.20
  - (Windows) NSIS or WiX Toolset for installer
  - (Linux) dpkg/rpm tools or AppImage tooling
  - (macOS) pkgbuild or create-dmg
  - GitHub CLI (gh) for release upload (optional)
  - Python >= 3.9 with `jinja2`, `markdown`, `PyYAML`
```

---

## When to Use

Trigger this skill when the user requests:

| Command Pattern | Scope |
|----------------|-------|
| "release v2.1.0", "ship version 2.1.0" | Full release with version bump, packaging, tagging |
| "deploy nightly", "package latest" | Snapshot/nightly build without version bump |
| "create release notes", "generate changelog" | Only generate release notes from Git history |
| "publish to GitHub", "upload release" | Upload existing packages to GitHub Releases |
| "sign binaries", "authenticode" | Sign Windows/macOS binaries (if certificates available) |

---

## Process

### Step 0: Pre‑flight Checks

Before any release action, verify the environment:

```bash
cd f:/syyyy

# 1. Repository clean? (no uncommitted changes, unless explicit)
git status --porcelain

# 2. All tests pass on current platform
ctest --output-on-failure

# 3. Ensure all submodules are up-to-date
git submodule update --init --recursive

# 4. Validate physics (GR benchmarks, etc.) – optional but recommended
validate-physics --quick

# 5. Check if version tag already exists
git tag -l "v*" | grep -i "v$VERSION"
```

If any critical check fails, abort with clear error message.

### Step 1: Determine Release Type & Version

Ask the user (or auto-detect from command):

| Input | Action |
|-------|--------|
| `--version 2.1.0` | Use exact version |
| `--major` | Increment major: X.0.0 |
| `--minor` | Increment minor: 1.X.0 |
| `--patch` | Increment patch: 1.0.X |
| `--nightly` | Use date-based version like `2.1.0-dev-20260528` |
| No flag | Prompt user for version and type |

Update version in:
- `CMakeLists.txt` (project version)
- `src/version.h` (generated header)
- `package.json` (if exists for web UI)
- `pyproject.toml` (if Python bindings exist)

### Step 2: Run Full Validation (Optional but Recommended)

If user wants a validated release (e.g., not nightly), run the full physics validation suite:

```bash
validate-physics --full
```

If any critical test fails, abort release unless user forces with `--force`.

### Step 3: Generate Changelog / Release Notes

Automatically generate release notes using one of these methods:

#### A. From Git History (Recommended)
```bash
# Get commits since last tag
LAST_TAG=$(git describe --tags --abbrev=0)
git log ${LAST_TAG}..HEAD --oneline --format="%s" | sort | uniq > commits.txt

# Categorize commits using conventional commits patterns
python f:/syyyy/scripts/generate_release_notes.py --since ${LAST_TAG} --output RELEASE_NOTES.md
```

#### B. Manually Specified File
If user provides a changelog file, use that.

#### C. Interactive Editor
Open a temporary markdown file for user to write notes, then save.

**Release Notes Template** (`RELEASE_NOTES.md`):
```markdown
# QuantumVerse v{VERSION} Release Notes

**Release Date**: {DATE}  
**Git Tag**: v{VERSION}  
**Build Platform**: {PLATFORM}

## Highlights
- [List major new features or fixes]

## New Features
- [List all features from conventional commits `feat:`]

## Bug Fixes
- [List from `fix:` commits]

## Performance Improvements
- [List from `perf:` commits]

## Breaking Changes
- [List from `BREAKING CHANGE:` footer]

## Dependency Updates
- [List from `deps:` commits]

## Validation Status
- ✅ GR historical tests: {passed}/{total}
- ✅ Quantum gravity tests: {passed}/{total}
- ✅ Performance benchmarks: {passed criteria}

## Known Issues
- [Any open critical bugs not fixed]

## SHA256 Checksums
- Windows: `quantumverse-{VERSION}-win64.exe` (hash)
- Linux: `quantumverse-{VERSION}-linux-x86_64.AppImage` (hash)
- macOS: `quantumverse-{VERSION}-macos.dmg` (hash)
```

### Step 4: Build Platform Packages

Build for each target platform. This step may be cross‑compiled or run on native agents.

#### Windows (NSIS or WiX)
```bash
mkdir build_win64
cd build_win64
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --target package
# Output: quantumverse-${VERSION}-win64.exe (installer)
```

#### Linux (AppImage)
```bash
mkdir build_linux
cd build_linux
cmake .. -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_APPIMAGE=ON
cmake --build . --target package
# Output: quantumverse-${VERSION}-linux-x86_64.AppImage
```

#### macOS (DMG)
```bash
mkdir build_macos
cd build_macos
cmake .. -G Xcode -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --target package
# Output: quantumverse-${VERSION}-macos.dmg
```

#### Source Tarball
```bash
git archive --format=tar.gz --prefix=quantumverse-${VERSION}/ HEAD > quantumverse-${VERSION}-source.tar.gz
```

### Step 5: Sign Binaries (Optional, Certificate Required)

If code‑signing certificates are available (and user provides passphrase/environment):

#### Windows (Authenticode)
```bash
signtool sign /fd SHA256 /a /tr http://timestamp.digicert.com /td SHA256 quantumverse-${VERSION}-win64.exe
```

#### macOS (Apple Developer ID)
```bash
codesign --force --options runtime --sign "Developer ID Application: Your Name" quantumverse-${VERSION}-macos.dmg
```

#### Linux (No standard signing, but GPG)
```bash
gpg --detach-sign --armor quantumverse-${VERSION}-linux-x86_64.AppImage
```

If signing fails, proceed without signing but log warning.

### Step 6: Create Git Tag and Push

```bash
# Create annotated tag with release notes as message
git tag -a v${VERSION} -m "Release v${VERSION}\n\n$(cat RELEASE_NOTES.md | head -50)"

# Push tag to remote
git push origin v${VERSION}
```

### Step 7: Upload to GitHub Release

If GitHub CLI is available and user has permissions:

```bash
# Create release on GitHub
gh release create v${VERSION} \
    --title "QuantumVerse v${VERSION}" \
    --notes-file RELEASE_NOTES.md \
    quantumverse-${VERSION}-win64.exe \
    quantumverse-${VERSION}-linux-x86_64.AppImage \
    quantumverse-${VERSION}-macos.dmg \
    quantumverse-${VERSION}-source.tar.gz
```

If `gh` is not available, provide manual instructions.

### Step 8: Update Homebrew Formula (macOS) – Optional

If the project is on Homebrew, update formula:
```bash
# Get SHA256 of macOS binary
SHASUM=$(shasum -a 256 quantumverse-${VERSION}-macos.dmg | cut -d ' ' -f1)

# Update formula file
sed -i "s/url .*/url \"https:\/\/github.com\/QuantumVerse\/quantumverse\/releases\/download\/v${VERSION}\/quantumverse-${VERSION}-macos.dmg\"/" Formula/quantumverse.rb
sed -i "s/sha256 .*/sha256 \"$SHASUM\"/" Formula/quantumverse.rb
```

### Step 9: Generate Artifact Hashes & Verify

```bash
# Generate SHA256SUMS file
sha256sum quantumverse-${VERSION}* > SHA256SUMS-${VERSION}.txt

# Sign the checksums file (if GPG key available)
gpg --detach-sign --armor SHA256SUMS-${VERSION}.txt
```

### Step 10: Create Local Package Archive

Organize all artifacts into a release directory:

```bash
RELEASE_DIR=quantumverse-${VERSION}-release
mkdir ${RELEASE_DIR}
mv quantumverse-${VERSION}* ${RELEASE_DIR}/
mv RELEASE_NOTES.md SHA256SUMS* ${RELEASE_DIR}/
zip -r ${RELEASE_DIR}.zip ${RELEASE_DIR}
```

### Step 11: Post‑Release Actions

- **Announce** on project channels (if hooks configured)
- **Update `latest` symlink** in artifact repository (S3, etc.)
- **Trigger downstream** (e.g., Docker image build, documentation rebuild)
- **Notify** via email/webhook (optional)

### Step 12: Cleanup

```bash
# Remove temporary build directories (unless --keep-build specified)
rm -rf build_* ${RELEASE_DIR} quantumverse-${VERSION}*
```

---

## Custom Commands (Flags)

| User Command | Action |
|--------------|--------|
| `deploy-release --version 2.0.0` | Use exact version, skip prompt |
| `deploy-release --patch` | Auto‑increment patch version |
| `deploy-release --minor` | Auto‑increment minor version |
| `deploy-release --major` | Auto‑increment major version |
| `deploy-release --nightly` | Create nightly snapshot (no version bump, date stamp) |
| `deploy-release --skip-validate` | Skip physics validation |
| `deploy-release --skip-tests` | Skip CTest |
| `deploy-release --skip-sign` | Skip binary signing |
| `deploy-release --skip-gh` | Skip GitHub release upload |
| `deploy-release --keep-build` | Keep build directories after packaging |
| `deploy-release --only-notes` | Generate release notes only (no build) |
| `deploy-release --dry-run` | Perform all steps except actual upload/tag push |

---

## Environment Variables

| Variable | Required? | Purpose |
|----------|-----------|---------|
| `GITHUB_TOKEN` | For GitHub upload | Personal access token |
| `CODE_SIGN_CERT_P12` | For Windows signing | Path to certificate file |
| `CODE_SIGN_PASSWORD` | For Windows signing | Certificate password |
| `APPLE_DEV_ID` | For macOS signing | Developer ID identity |
| `APPLE_NOTARY_PASSWORD` | For macOS notarization | App‑specific password |
| `HOMEBREW_GITHUB_API_TOKEN` | For Homebrew PR | GitHub token with repo access |

---

## Example Session

```bash
$ deploy-release --minor --validate

[INFO] Pre-flight checks passed.
[INFO] Current version: 2.0.0
[INFO] Bumping minor → 2.1.0
[INFO] Updating CMakeLists.txt, version.h...
[INFO] Running full physics validation...
[INFO] ✅ All GR tests passed (6/6)
[INFO] ✅ Quantum gravity tests passed (5/5)
[INFO] ✅ Performance benchmarks passed
[INFO] Generating changelog from commits since v2.0.0...
[INFO] Found: 12 feat, 8 fix, 3 perf, 2 breaking changes.
[INFO] Building Windows package... done.
[INFO] Building Linux AppImage... done.
[INFO] Building macOS DMG... done.
[INFO] Signing binaries... (certificate found) ✅
[INFO] Creating Git tag v2.1.0... ✅
[INFO] Pushing tag to origin... ✅
[INFO] Uploading to GitHub Release... ✅
[INFO] Generating SHA256SUMS... ✅
[INFO] Release directory packaged: quantumverse-2.1.0-release.zip
[INFO] Post-release: Homebrew PR opened.
[INFO] Cleanup complete.

Release v2.1.0 is ready! 🚀
Download: https://github.com/QuantumVerse/quantumverse/releases/tag/v2.1.0
```

---

## Rollback / Undo Release

If release was tagged but not yet announced, user can delete tag and GitHub release:

```bash
git tag -d v${VERSION}
git push origin --delete v${VERSION}
gh release delete v${VERSION} -y
```

---

## Return Codes

| Code | Meaning |
|------|---------|
| 0 | Release successfully created and published |
| 1 | Pre‑flight check failed (dirty tree, missing tests) |
| 2 | Physics validation failed |
| 3 | Build failed |
| 4 | Signing failed (non‑fatal if `--skip-sign` not used) |
| 5 | Git tag creation/push failed |
| 6 | GitHub upload failed |
| 7 | User interrupted |

---

## Integration with CI/CD

The skill can be run in GitHub Actions, GitLab CI, or Jenkins by setting environment variables and using `--dry-run` for testing. Example workflow:

```yaml
# .github/workflows/release.yml
name: Release
on:
  push:
    tags:
      - 'v*'
jobs:
  release:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Deploy release
        run: |
          export GITHUB_TOKEN=${{ secrets.GITHUB_TOKEN }}
          .kilo/skills/deploy-release/run.sh --version ${GITHUB_REF_NAME#v} --skip-validate
```

---

**Upgrade Summary**:
- Multi‑platform packaging (Windows, Linux, macOS, source)
- Automated version bumping (major/minor/patch/nightly)
- Full physics validation integration
- Changelog generation from conventional commits
- Binary signing (Windows Authenticode, macOS Developer ID)
- GitHub Releases upload via `gh`
- Homebrew formula update
- Checksum generation and verification
- CI/CD friendly with `--dry-run` and environment variables
- Rollback procedure

This turns the simple release script into a **production‑grade deployment pipeline** suitable for open‑source scientific software.