# QuantumVerse Simulator – Unified Workflow File

**Version**: 3.0  
**Last Updated**: 2026-06-01  
**Purpose**: Single source of truth for all development workflows in the QuantumVerse project.  
**Scope**: Physics module scaffolding, neural ODE surrogate training, ImGui panel creation, validation suite execution, spacetime export, performance profiling, AI discovery agent deployment, quantum gravity plugin testing, multi-view synchronisation, release packaging, and continuous integration.

---

## Workflow 1: Create Physics Module

Scaffold a new physics module (header, implementation, unit test, and CMake integration) following C++17, Doxygen, RAII, and smart‑pointer rules.

### Step 1: Gather Module Information

Ask the user for:

1. **Module name** (PascalCase, e.g. `DilatonMetric`, `HawkingCalculator`).  
2. **Submodule directory** – choose from `spacetime`, `physics`, `rendering`, `ui4d`, `discovery`, `quantumgravity`, `data`, `ml`, `math`.  
3. **Brief description** – used for `@brief` Doxygen tag.  
4. **Dependencies** – list of existing headers (e.g. `MetricTensor.h`, `Spacetime.h`).  
5. **Implementation requirement** – header‑only or `.cpp` file needed?  
6. **Target** – existing library/executable to attach source files (e.g. `QuantumVersePhysics`, `UnitTests`).  
7. **Test framework** – default is Google Test (`gtest`).

**Validation checks**:
- Name must match `[A-Z][a-zA-Z0-9_]*`.
- Submodule directory must exist under `src/`.
- Dependencies must correspond to real header files.

### Step 2: Create Header File

Create `src/<sub>/<ModuleName>.h` with the following content, adapted to the module’s specifics:

```cpp
/**
 * @file <ModuleName>.h
 * @brief <brief description>
 * @author QuantumVerse Team
 * @copyright GPL-3.0-or-later
 */

#pragma once

#include <cstddef>
#include <memory>
#include <vector>
#include <string>
#include <optional>

// Forward declarations
namespace QuantumVerse {
class MetricTensor;
}

namespace QuantumVerse {

/**
 * @class <ModuleName>
 * @brief <brief description>
 * @details The class encapsulates the physics computation using the PIMPL idiom,
 * ensuring ABI stability and RAII. All owned resources are managed by std::unique_ptr.
 *
 * Usage:
 * @code
 *   auto obj = <ModuleName>::create( parameters );
 *   auto result = obj->compute( input );
 * @endcode
 *
 * @note This class is move‑only, non‑copyable.
 * @see <list dependent classes>
 */
class <ModuleName> {
public:
    /**
     * @brief Factory method – preferred construction.
     * @param args Optional initialisation parameters.
     * @return Unique pointer to a fully initialised instance.
     */
    static std::unique_ptr<<ModuleName>> create(/* parameters */);

    /** @brief Default constructor (internal use only). */
    <ModuleName>();
    /** @brief Destructor – defined in the implementation file. */
    ~<ModuleName>();

    // Non-copyable
    <ModuleName>(const <ModuleName>&) = delete;
    <ModuleName>& operator=(const <ModuleName>&) = delete;

    // Movable
    <ModuleName>(<ModuleName>&&) noexcept;
    <ModuleName>& operator=(<ModuleName>&&) noexcept;

    /**
     * @brief Primary computation interface.
     * @param input Description.
     * @return Result of the computation, or std::nullopt on failure.
     */
    std::optional<double> compute(const std::vector<double>& input) const;

private:
    struct Impl;                     ///< Forward‑declared implementation details.
    std::unique_ptr<Impl> pImpl;     ///< Opaque pointer to implementation.
};

} // namespace QuantumVerse
```

If the module is **header‑only**, implement all methods inline inside the class and omit the separate `.cpp` file.

### Step 3: Create Implementation File (if needed)

Create `src/<sub>/<ModuleName>.cpp`:

```cpp
#include "<ModuleName>.h"
#include "core/Logging.h"          // for QUANTUMVERSE_LOG macros
#include <stdexcept>

namespace QuantumVerse {

struct <ModuleName>::Impl {
    // Configuration and state
    double scale = 1.0;
    // … additional member variables
};

std::unique_ptr<<ModuleName>> <ModuleName>::create() {
    auto obj = std::make_unique<<ModuleName>>();
    // Perform any validation or initialisation
    return obj;
}

<ModuleName>::<ModuleName>()
    : pImpl(std::make_unique<Impl>()) {
    QUANTUMVERSE_LOG_DEBUG("<ModuleName> constructed");
}

<ModuleName>::~<ModuleName>() = default;

<ModuleName>::<ModuleName>(<ModuleName>&&) noexcept = default;
<ModuleName>& <ModuleName>::operator=(<ModuleName>&&) noexcept = default;

std::optional<double> <ModuleName>::compute(const std::vector<double>& input) const {
    if (input.empty()) {
        QUANTUMVERSE_LOG_ERROR("compute called with empty input");
        return std::nullopt;
    }
    // … computation using pImpl->scale
    return input[0] * pImpl->scale;
}

} // namespace QuantumVerse
```

### Step 4: Create Unit Test

Create `tests/test_<modulename>.cpp`:

```cpp
#include <gtest/gtest.h>
#include "<ModuleName>.h"

using namespace QuantumVerse;

class <ModuleName>Test : public ::testing::Test {
protected:
    void SetUp() override {
        obj = <ModuleName>::create();
        ASSERT_NE(obj, nullptr);
    }
    std::unique_ptr<<ModuleName>> obj;
};

TEST_F(<ModuleName>Test, Construction) {
    // Verify that the object is valid
}

TEST_F(<ModuleName>Test, ComputeWithValidInput) {
    auto result = obj->compute({2.0});
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result.value(), 2.0);
}

TEST_F(<ModuleName>Test, ComputeWithEmptyInput) {
    auto result = obj->compute({});
    EXPECT_FALSE(result.has_value());
}
```

### Step 5: Update CMakeLists.txt

- **Library target** (`src/<sub>/CMakeLists.txt`):
  ```cmake
  target_sources(QuantumVersePhysics PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/<ModuleName>.cpp
  )
  ```
  For header‑only, add only the header to `PUBLIC` headers.

- **Test target** (`tests/CMakeLists.txt`):
  ```cmake
  add_executable(test_<modulename> test_<modulename>.cpp)
  target_link_libraries(test_<modulename> PRIVATE QuantumVersePhysics gtest_main)
  gtest_discover_tests(test_<modulename>)
  ```

### Step 6: Build and Verify

```bash
cd build
cmake --build . --target test_<modulename>
./tests/test_<modulename>
```

Expected: all tests pass.

---

## Workflow 2: Train Neural ODE Surrogate

Manage the Neural ODE surrogate training pipeline for Phase 2 Task 2.1 (Geodesic Neural ODE). Covers dataset generation, training, monitoring, ONNX export, and integration.

### Prerequisites
- Python 3.10+
- CUDA 12.1+ (optional)
- Virtual environment with dependencies from `requirements_task2_1.txt`
- Input metrics: `schwarzschild`, `kerr`, `reissner‑nordstrom`

### Step 1: Verify Python Environment

```bash
cd f:/syyyy/python
python -m venv venv_ode
source venv_ode/bin/activate   # or venv_ode\Scripts\activate on Windows
pip install -r requirements_task2_1.txt
python -c "import torch; print('PyTorch', torch.__version__); print('CUDA:', torch.cuda.is_available())"
```

**Expected output**: `CUDA: True` if GPU is available.

If CUDA is unavailable, training will fall back to CPU (slower). Set environment variable `QUANTUMVERSE_DEVICE=cpu` to enforce CPU.

### Step 2: Generate Training Dataset

```bash
python generate_geodesic_dataset.py \
    --metrics schwarzschild,kerr,reissner-nordstrom \
    --num-samples 1000000 \
    --output-dir data/geodesic_dataset/ \
    --threads 8 \
    --seed 42
```

**Post‑generation checks**:
- `data/geodesic_dataset/train/` and `test/` directories contain `.h5` files.
- Run `python validate_dataset.py --data-dir data/geodesic_dataset/` to confirm sample count, numeric ranges, and absence of NaNs.

### Step 3: Train Neural ODE Model

```bash
python train_neural_ode.py \
    --data-dir data/geodesic_dataset/ \
    --batch-size 1024 \
    --epochs 200 \
    --learning-rate 0.001 \
    --hidden-dim 256 \
    --num-layers 4 \
    --gpu-id 0 \
    --experiment-name geodesic_ode_exp1
```

**Hyperparameter tuning** (optional):
Use `optuna` integration:
```bash
python tune_neural_ode.py --study-name geodesic_ode --n-trials 20
```

Monitor training progress with TensorBoard:
```bash
tensorboard --logdir runs/ --bind_all
```
Key metrics: `train/loss`, `val/loss`, `lr`, `grad_norm`.

**Training logs** are saved in `runs/<experiment_name>/` with checkpoints every 10 epochs. The best model (lowest validation loss) is saved as `best_model.pt`.

### Step 4: Validate Model Accuracy

```bash
python validate_neural_ode.py \
    --model runs/geodesic_ode_exp1/best_model.pt \
    --test-dir data/geodesic_dataset/test/ \
    --max-error 0.01 \
    --output-report validation_report.json
```

**Acceptance criteria**:
- Endpoint MSE < 1×10⁻⁴
- Relative error < 1%
- Maximum single‑step error < 0.01
- Report generated as `validation_report.json` and `validation_plots.png`.

If validation fails, adjust hyperparameters, increase dataset size, or refine model architecture.

### Step 5: Export to ONNX

```bash
python export_to_onnx.py \
    --model runs/geodesic_ode_exp1/best_model.pt \
    --output models/geodesic_ode.onnx \
    --input-dim 10 \
    --opset-version 14
```

**Verify ONNX model**:
```bash
python test_onnx_inference.py --model models/geodesic_ode.onnx
```
Expected: output values match PyTorch model to within 1×10⁻⁵.

### Step 6: Integrate into C++

1. Copy `models/geodesic_ode.onnx` to `data/models/`.
2. Update `GeodesicNeuralODE::loadONNXModel()` to:
   ```cpp
   auto modelPath = std::filesystem::path(DATA_DIR) / "models" / "geodesic_ode.onnx";
   ortSession = std::make_unique<Ort::Session>(env, modelPath.c_str(), sessionOptions);
   ```
3. Run integration test:
   ```bash
   ./tests/test_geodesic_neural_ode --gtest_filter=*onnx_integration*
   ```

---

## Workflow 3: Create ImGui Panel

Add a new interactive panel to the 4D dashboard using Dear ImGui.

### Step 1: Define Panel Class

Create `src/ui_imgui/NewPanel.h`:

```cpp
#pragma once
#include <imgui.h>
#include <functional>
#include <string>
#include <memory>

namespace QuantumVerse {

class UIManager;   // forward

class NewPanel {
public:
    explicit NewPanel(UIManager& manager);
    ~NewPanel() = default;

    void render(ImVec2 availSize);
    void setVisible(bool visible) { isVisible = visible; }

    // Public signals
    std::function<void()> onAction;
    std::function<void(float)> onParameterChanged;

private:
    UIManager& uiManager;
    bool isVisible = true;
    float someValue = 0.0f;
    std::string label = "New Panel";
};

} // namespace QuantumVerse
```

### Step 2: Implement Rendering

Create `src/ui_imgui/NewPanel.cpp`:

```cpp
#include "NewPanel.h"
#include "UIManager.h"
#include <imgui_internal.h>

namespace QuantumVerse {

NewPanel::NewPanel(UIManager& manager) : uiManager(manager) {}

void NewPanel::render(ImVec2 /*availSize*/) {
    if (!isVisible) return;

    ImGui::SetNextWindowSize(ImVec2(350, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin(label.c_str(), &isVisible,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Options")) {
            ImGui::MenuItem("Reset", nullptr, false, false);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    float previousValue = someValue;
    ImGui::SliderFloat("Parameter", &someValue, 0.0f, 1.0f, "%.3f");
    if (someValue != previousValue && onParameterChanged) {
        onParameterChanged(someValue);
    }

    if (ImGui::Button("Execute")) {
        if (onAction) onAction();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Execute the selected action.");

    ImGui::End();
}

} // namespace QuantumVerse
```

### Step 3: Register in UI4D_ImGui

1. In `UI4D_ImGui.h`, add:
   ```cpp
   #include "NewPanel.h"
   ...
   std::unique_ptr<NewPanel> newPanel;
   ```
2. In `UI4D_ImGui.cpp` constructor:
   ```cpp
   newPanel = std::make_unique<NewPanel>(uiManager);
   newPanel->onAction = [this]() {
       // Example: launch geodesic integration
       launchGeodesicIntegration();
   };
   ```
3. In `UI4D_ImGui.cpp` render loop, call:
   ```cpp
   if (newPanel) newPanel->render(availableSize);
   ```

### Step 4: Add to Docking Layout

In the main docking setup (usually `UI4D_ImGui::setupDocking()`), attach the panel to the desired dock node:

```cpp
ImGui::DockBuilderDockWindow("New Panel", rightDockId);
```

The window ID must match the string passed to `ImGui::Begin` ("New Panel").

### Step 5: Unit Tests for Panel

Create `tests/test_newpanel.cpp` that uses the ImGui test engine to simulate user interactions and verify callbacks.

---

## Workflow 4: Run Validation Suite

Execute all GR validation tests and generate a compliance report.

### Step 1: Build with Tests Enabled

```bash
mkdir -p build && cd build
cmake .. -DQUANTUMVERSE_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --parallel $(nproc)
```

### Step 2: Run CTest

```bash
ctest --output-on-failure -C Release --parallel 4
```

**Exit criteria**: ≥ 95% pass rate. Failures are logged to `Testing/Temporary/LastTest.log`.

### Step 3: Run Specific Validation Tests

Each validation test can be run individually with additional verbosity:

```bash
# Mercury precession
./tests/test_mercury_precession --gtest_filter=MercuryPrecession.ExcessRate --gtest_output=xml:mercury_results.xml

# Light deflection
./tests/test_light_deflection --gtest_filter=LightDeflection.EddingtonValue --gtest_output=xml:deflection_results.xml
```

Expected values and tolerances are embedded in the test documentation.

### Step 4: Generate Validation Report

```bash
python scripts/validation_report.py \
    --build-dir build/ \
    --output validation_report.md \
    --reference-data data/validation_reference.h5
```

The script compares measured values with reference data from known exact solutions or literature. The report includes:
- Pass/fail summary
- Measured vs. expected values
- Relative error
- Graphs (if `--plot` flag is used)

### Step 5: Continuous Validation (optional)

Add the validation suite to CI (see Workflow 11) so that every pull request automatically triggers the tests.

---

## Workflow 5: Export Spacetime Model

Export a simulation state (metric, geodesics, curvature, stress‑energy) to a portable format (HDF5, CSV, or JSON).

### Step 1: Prepare Export Parameters

Decide:
- **Format**: `HDF5` (recommended for large datasets), `CSV`, or `JSON`.
- **Data selection**: Metric tensor on grid, geodesic trajectories (one or many), curvature scalars (Kretschmann, Ricci scalar, Weyl components), stress‑energy components.
- **Region of interest**: bounding box or time slice.
- **Compression**: gzip level (0–9) for HDF5.

### Step 2: Call Export API

In C++ code:

```cpp
#include "data/SpacetimeExporter.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Geodesic.h"

SpacetimeExporter exporter;
exporter.setMetric(metricTensor);
exporter.addGeodesic(trajectory1);
exporter.addGeodesic(trajectory2);
exporter.setCurvatureData(kretschmannGrid);

SpacetimeExporter::Options opts;
opts.format = SpacetimeExporter::Format::HDF5;
opts.compressionLevel = 4;
opts.overwrite = true;
auto result = exporter.exportToFile("spacetime_export.h5", opts);
if (!result) {
    QUANTUMVERSE_LOG_ERROR("Export failed: {}", result.error());
}
```

### Step 3: Verify Exported File

**Python**:
```python
import h5py, numpy as np
with h5py.File("spacetime_export.h5", "r") as f:
    print("Groups:", list(f.keys()))
    for key in f.keys():
        print(f"{key}: shape {f[key].shape}, dtype {f[key].dtype}")
    # Check a few values
    metric = f["metric"][:]
    assert np.isfinite(metric).all(), "NaN found in metric"
```

**C++ verification** (optional):
```cpp
#include "data/SpacetimeImporter.h"
SpacetimeImporter importer;
auto state = importer.importFromFile("spacetime_export.h5");
if (state) {
    auto& metric = state->metric;
    // compare with original
}
```

### Step 4: Import Back (Optional)

```cpp
SpacetimeImporter importer;
auto imported = importer.importFromFile("spacetime_export.h5");
simulation.restoreState(*imported);
```

### Step 5: Batch Export Script

For automated exports, use:
```bash
python scripts/batch_export.py --config config/export_config.yaml
```
Configuration defines export intervals, data filters, and output naming.

---

## Workflow 6: Profile Performance

Identify bottlenecks in rendering or physics computation using Tracy and Google Benchmark.

### Step 1: Build with Profiling Symbols

```bash
mkdir build_profile && cd build_profile
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo \
         -DQUANTUMVERSE_ENABLE_PROFILING=ON \
         -DQUANTUMVERSE_ENABLE_TRACY=ON
cmake --build . --config RelWithDebInfo --parallel
```

### Step 2: Run with Tracy Profiler

1. Launch the application:
   ```bash
   ./quantumverse_imgui.exe
   ```
2. Open the Tracy GUI (`tracy-profiler`), connect to the running application (default port 8086).
3. Capture a frame and examine the flame graph.
4. Identify functions with high inclusive time (e.g., `GeodesicIntegrator::step`, `VolumeRenderer::raycast`).

### Step 3: Run Microbenchmarks

```bash
./tests/test_differentiable_benchmark --benchmark_format=console --benchmark_out=benchmarks.json
```

**Key benchmarks**:
- `GeodesicIntegrator::step` (RK45 vs. DOP853)
- `MetricTensor::compute` (CPU vs. GPU)
- Memory allocation hot‑spots

### Step 4: Analyse and Optimise

- Use the Tracy memory view to spot frequent allocations.
- Profile GPU workloads with `nsight-systems` or `nvtx` markers.
- Apply optimisations: SIMD (SSE/AVX), parallel loops with OpenMP or TBB, GPU compute (CUDA/OpenCL), caching of intermediate results.
- Re‑profile after each optimisation cycle.

### Step 5: Performance Regression Testing

Add performance tests to CI that fail if runtime of critical functions exceeds a threshold (e.g., `GeodesicIntegrator::step` > 1.2× baseline). Use Google Benchmark’s `--benchmark_min_time` and store baselines.

---

## Workflow 7: Deploy AI Discovery Agent

Run the reinforcement learning agent to explore theory parameter space and discover novel physics.

### Step 1: Configure Environment

Edit `config/discovery_agent.yaml`:

```yaml
environment:
  parameter_space: ["M_blackhole", "alpha_fR", "lambda_phi"]
  bounds:
    M_blackhole: [1.0, 10.0]   # solar masses
    alpha_fR: [-1.0, 1.0]
    lambda_phi: [0.0, 5.0]
  max_steps: 1000
  reward_weights: {novelty: 0.4, validity: 0.3, simplicity: 0.1, fit: 0.2}
  novelty_threshold: 0.1

agent:
  algorithm: "PPO"
  learning_rate: 0.0003
  batch_size: 64
  hidden_size: 256
  gamma: 0.99
  gae_lambda: 0.95

training:
  total_timesteps: 1000000
  eval_freq: 10000
  n_eval_episodes: 5
  log_dir: runs/discovery_agent/
```

### Step 2: Launch Training

```bash
python scripts/train_rl_discovery.py \
    --config config/discovery_agent.yaml \
    --output runs/discovery_agent/ \
    --seed 123 \
    --tensorboard
```

Monitor with TensorBoard:
```bash
tensorboard --logdir runs/discovery_agent/ --bind_all
```

Training progress is saved as periodic checkpoints (`rl_model_XXXXX_steps.zip`) and a final model.

### Step 3: Evaluate Discoveries

After training:
```bash
python scripts/evaluate_discoveries.py \
    --agent runs/discovery_agent/final_model.zip \
    --num-episodes 100 \
    --output discoveries.json \
    --plot discovery_landscape.png
```

The script outputs a ranked list of candidate anomalies with:
- Parameter values
- Bayesian evidence score (log‑evidence)
- Novelty index (deviation from known theories)
- Physical consistency score

### Step 4: Analyse with Visualisation

```bash
python scripts/visualise_discoveries.py --discoveries discoveries.json
```
Generates interactive scatter plots (plotly) of the parameter space colored by reward.

### Step 5: Generate LaTeX Paper

```bash
python scripts/generate_paper.py \
    --discovery-file discoveries.json \
    --template templates/discovery_paper.tex \
    --output paper/
```

Compile with `pdflatex` → `bibtex` → `pdflatex` → `pdflatex`. The resulting PDF is a draft paper with figures, tables, and a summary of the most promising anomalies.

---

## Workflow 8: Test Quantum Gravity Plugin

Validate a new quantum gravity engine (e.g., Causal Dynamical Triangulations, Spin Foam) against reference results.

### Step 1: Build Plugin

The plugin must inherit from `TheoryPlugin` and implement all required virtual methods:

```cpp
#include "quantumgravity/TheoryPlugin.h"

class MyQGEngine : public TheoryPlugin {
public:
    std::string getName() const override { return "My QG Engine"; }
    std::string getDescription() const override { return "Custom quantum gravity model"; }
    void initialize(const json& params) override;
    double computeSpectralDimension(double diffusionTime) const override;
    bool isConsistent() const override;
    std::vector<FieldConfiguration> sampleConfigurations(int n) const override;
    // ... other overrides
};
```

Register the plugin in `src/quantumgravity/PluginRegistry.cpp`.

### Step 2: Run Unit Tests

```bash
./tests/test_my_qg_engine --gtest_filter=MyQGEngine.*
```

Ensure all tests pass. Typical tests include:
- Construction and initialisation
- Spectral dimension at short and long diffusion times
- Consistency checks (e.g., positive definiteness of metric)
- Sample configuration statistical properties

### Step 3: Visual Inspection

Launch the simulator, select the plugin from the Theory Manager panel. Use the Planck microscope to explore the quantum geometry. Verify that:
- LOD transitions occur at the expected scale.
- Colouring/rendering represents physical quantities (e.g., curvature, action density).
- The overlay of classical geometry in the 3D slice matches expectations.

### Step 4: Compare with Reference Data

```bash
python scripts/compare_qg_engines.py \
    --engine1 CDT \
    --engine2 MyQGEngine \
    --metrics spectral_dimension,volume_profile,curvature_fluctuation \
    --output-dir comparison_results/
```

The script computes relative differences and generates summary plots. A pass requires ≤ 5% difference for all metrics.

### Step 5: Performance and Scaling

Run `scripts/qg_scaling_test.py` to measure simulation time vs. lattice size. Ensure the plugin does not regress performance beyond 10% of a baseline (e.g., CDT).

---

## Workflow 9: Synchronise Views (4D Dashboard)

Connect multiple viewports (4D main, 3D slice, causal graph, discovery probe) so that selection in one highlights in all others.

### Step 1: Implement Selection Signal

In `UI4D_ImGui`, add a signal slot mechanism:

```cpp
// UI4D_ImGui.h
#include <sigslot/signal.hpp>   // or use std::function
sigslot::signal<const Event4D&> eventSelected;
```

### Step 2: Emit on Click

In the main viewport’s picking routine (e.g., `FourDViewport::pickEvent`):

```cpp
if (pickedEvent && pickedEvent != lastSelected) {
    lastSelected = pickedEvent;
    eventSelected(*pickedEvent);
}
```

### Step 3: Connect Slice View

In `SliceViewPanel`:

```cpp
SliceViewPanel::SliceViewPanel(UI4D_ImGui& ui4d) {
    ui4d.eventSelected.connect([this](const Event4D& ev) {
        highlightEvent(ev);
        updateSliceCenter(ev.position);
        requestRender();
    });
}
```

### Step 4: Connect Causal Graph Panel

```cpp
ui4d.eventSelected.connect([this](const Event4D& ev) {
    causalGraph->highlightCone(ev);
});
```

### Step 5: Connect Discovery Probe Panel

The discovery probe panel updates its curvature readings and displays the local Lagrangian density.

### Step 6: Verify Synchronisation

Manual test checklist:
- Click an event in the main viewport → highlighted in all views.
- Select from the event list → consistent highlighting.
- Drag the slice position in 3D view → main viewport shows corresponding 4D slice.
- All highlights clear when clicking empty space.

### Step 7: Automated GUI Tests

Use the integrated ImGui test framework to simulate clicks and verify that the correct callbacks fire (e.g., `test_gui_synchronisation.cpp`).

---

## Workflow 10: Build and Package Release

Create a distributable version of QuantumVerse for Windows, Linux, and macOS.

### Step 1: Clean Build

```bash
rm -rf build_release
mkdir build_release && cd build_release
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DQUANTUMVERSE_BUILD_TESTS=OFF \
         -DQUANTUMVERSE_USE_IMGUI=ON \
         -DQUANTUMVERSE_INSTALL=ON \
         -DCMAKE_INSTALL_PREFIX=../install
cmake --build . --config Release --parallel --target install
```

### Step 2: Bundle Dependencies

- **Windows**:
  ```powershell
  windeployqt install/bin/quantumverse_imgui.exe   # if using Qt
  # copy VC++ redistributables (vcruntime140.dll, etc.)
  python scripts/copy_dll_deps.py install/
  ```
- **Linux**:
  ```bash
  linuxdeploy install/usr/share/quantumverse --appdir install --output appimage
  # or create a shell script that sets LD_LIBRARY_PATH
  ```
- **macOS**:
  ```bash
  macdeployqt install/quantumverse.app -dmg
  # use install_name_tool for any non‑Qt libraries
  ```

### Step 3: Create Archive

```bash
cd install
# Windows
7z a quantumverse_v2.5.0_win64.zip .
# Linux
tar czf quantumverse_v2.5.0_linux_x86_64.tar.gz *
# macOS
hdiutil create quantumverse_v2.5.0_macos.dmg -srcfolder quantumverse.app
```

### Step 4: Test on Clean Machine

- Deploy the archive to a virtual machine or a fresh OS installation without development tools.
- Run the executable and verify all features: GR visualisation, ImGui panels, loading of ML models, export/import.
- Check that no missing DLL/shared library errors appear.

### Step 5: Sign and Notarize (macOS, Windows)

- Windows: sign with a code signing certificate using `signtool`.
- macOS: notarize with Apple using `xcrun notarytool`.

---

## Workflow 11: Continuous Integration & Delivery (CI/CD)

Automate building, testing, and packaging on every push or pull request.

### Step 1: CI Configuration

Use GitHub Actions (or GitLab CI). Example `.github/workflows/ci.yml`:

```yaml
name: CI
on: [push, pull_request]
jobs:
  build-linux:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: sudo apt-get install -y cmake build-essential libgtest-dev libglm-dev libsdl2-dev
      - name: Configure
        run: cmake -B build -DQUANTUMVERSE_BUILD_TESTS=ON
      - name: Build
        run: cmake --build build --parallel
      - name: Test
        run: ctest --test-dir build --output-on-failure
  build-windows:
    runs-on: windows-2022
    # ... similar steps with MSVC
  build-macos:
    runs-on: macos-12
    # ...
  neural-ode-tests:
    runs-on: ubuntu-22.04
    needs: build-linux
    steps:
      - uses: actions/checkout@v3
      - uses: actions/setup-python@v4
        with:
          python-version: '3.10'
      - name: Install Python deps
        run: pip install -r f:/syyyy/python/requirements_task2_1.txt
      - name: Run Neural ODE validation
        run: python f:/syyyy/python/validate_neural_ode.py --model dummy
  release:
    if: startsWith(github.ref, 'refs/tags/v')
    needs: [build-linux, build-windows, build-macos, neural-ode-tests]
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/download-artifact@v3
      - name: Package
        run: python scripts/package_release.py --version ${{ github.ref_name }}
      - name: Upload Release
        uses: softprops/action-gh-release@v1
        with:
          files: release/*.tar.gz,release/*.zip,release/*.dmg
```

### Step 2: Branching Strategy

- `main` – stable releases.
- `develop` – integration branch.
- Feature branches: `feature/<workflow>-<description>`.
- Tags: `vX.Y.Z`.

### Step 3: Static Analysis and Code Quality

Add Clang‑Tidy and Cppcheck to the CI pipeline; fail on warnings.

### Step 4: Automated Documentation

Deploy Doxygen to GitHub Pages after each push to `main`. (See `scripts/generate_docs.sh`)

---

## Workflow 12: Documentation Generation

Generate and deploy the project documentation.

### Step 1: Build Documentation

```bash
cd docs
doxygen Doxyfile
```

The Doxyfile includes settings for C++17, graphviz diagrams, and MathJax.

### Step 2: Preview Locally

```bash
cd html
python -m http.server 8000
```

### Step 3: Deploy to GitHub Pages

Use GitHub Actions (see `docs.yml`) or manual script:
```bash
./scripts/deploy_docs.sh
```

---

**End of Unified Workflow File**