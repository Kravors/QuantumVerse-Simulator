QuantumVerse v7.3.1 — UI patch

Patch release addressing six UI defects discovered post-v7.3.0. No functional
changes to physics, experiments, or the paper pipeline.

### Fixed
- RL Anomaly Detection score now normalized to [0, 1] via running-max
  normalization (was displaying raw detector MSE output — typically 1e8+,
  looking like a broken instrument). Running-max adapts to the actual data
  distribution; sigmoid was rejected because it saturates to exactly 1.0
  for MSE-scale inputs.
- Parameter Sweep "Export" button no longer clipped — now on its own row
  below "Run Sweep", gated on results existing (instrumentCount > 0).
- Orphaned unlabeled SpinBox removed from Parameter Sweep row.
- "Natural Geodesics" checkbox no longer truncated — Reset and Neural
  Geodesics now share their own row with explicit widths.
- Right panel minimum width increased 340 → 380px for breathing room.
- Dead vertical gap in right panel removed — outer ColumnLayout now uses
  Layout.fillHeight with a stretching Item at bottom.

### Upgrade note
If you have v7.3.0 installed, uninstall it first (Windows UAC may silently
cancel an in-place upgrade, leaving v7.3.0 on disk while reporting success).
Then run the v7.3.1 installer.

### What's unchanged from v7.3.0
All functionality is identical: 23 experiments, 5 paper shapes, 2 named-case
reproductions (GW150914, Mercury), theory discrimination matrix. See the
v7.3.0 release notes for the full feature list.
