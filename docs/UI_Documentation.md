# QuantumVerse Simulator - UI Documentation

## Overview

The QuantumVerse Simulator features an immersive, GUI-first interface built with Dear ImGui that transforms the physics simulation into an interactive spacetime laboratory. The UI is designed around the "Navigate to Discover" philosophy where navigation actions automatically enable relevant physics interrogation tools.

## UI Architecture

### Main Components

1. **Primary Viewport** - The central 4D visualization window showing spacetime, celestial bodies, and physics phenomena
2. **Bottom Timeline Bar** - Time travel controls with play/pause, rewind/fast-forward, and event navigation
3. **Top Toolbar** - Quick navigation to home, planets, black holes, Planck scale, and theories
4. **Context-Aware Panels** - Floating panels that appear based on user interaction and context:
   - Object Browser (sidebar)
   - Property Editor 
   - Curvature Overlay
   - Geodesic Tracer
   - Anomaly Alert Panel
   - Discovery Console
   - Planck Microscope
   - Theory Comparator
   - Black Hole Control Panel
   - Singularity Editor

### UI Implementation Files

All UI components are implemented in the `src/ui_imgui` directory:

- `UI4D_ImGui.h/.cpp` - Main ImGui integration class
- `TimelineBar.h/.cpp` - Time travel controls at bottom of screen
- `ObjectBrowser.h/.cpp` - Celestial body browser sidebar
- `PropertyEditor.h/.cpp` - Object property editor panel
- `ContextMenu.h/.cpp` - Right-click context menu system
- `FloatingPanels.h/.cpp` - Base class for floating UI panels

## Key UI Features

### 1. Spacetime Cockpit (Main Viewport)

The main 4D viewport provides:
- **Navigation Controls**:
  - Left-click + drag: Rotate camera in 4D
  - Right-click: Context menu for object interrogation
  - Scroll: Logarithmic zoom (1 AU → 10⁻³⁵ m)
  - Middle-click + drag: Pan view
  - Ctrl + drag: Time-scrub (move through time)

- **Information Display** (bottom-left):
  - Current celestial object
  - Simulation time
  - Scale indicator
  - FPS counter
  - Playback controls and speed

### 2. Timeline Bar

Located at the bottom of the screen, provides time travel functionality:
- Transport controls: ⏪ Rewind, ⏮ Previous, ▶ Play, ⏸ Pause, ⏭ Next, ⏩ Fast-Forward
- Timeline visualization with event markers
- Adjustable playback speed slider
- Current/next event display

### 3. Quick Navigation Toolbar

Top-level toolbar for instant access to key locations:
- 🏠 Home (Solar System view)
- 🌍 Planets (Celestial body browser)
- ⚫ Black Holes (Black hole catalog)
- 🔬 Planck Scale (Quantum gravity interface)
- 🧪 Theories (Physics theory selector)
- [Object Browser] [Singularity Editor] [Microscope] [Theory Selector]

### 4. Context-Aware Physics Laboratory

When interacting with objects, relevant physics tools appear automatically:

#### Context Menu (Right-click on any object)
- 📊 Properties (mass, spin, horizon, ergosphere)
- 🧪 Test General Relativity (geodesics, precession, redshift, frame-dragging)
- ⚛️ Test Quantum Gravity Candidates (LQG, causal sets, CDT, string theory)
- 🔮 Exotic Configurations (naked singularities, negative mass, wormholes, CTCs)
- 📈 Run Automated Discovery Scan
- 🚀 Fly to object
- 📍 Add waypoint

#### Floating Panels
- **Curvature Overlay** (toggle with 'C' key): Visualizes Kretschmann scalar, Ricci scalar, Weyl tensor
- **Geodesic Tracer**: Appears when selecting "Drop test particle" to visualize particle trajectories
- **Anomaly Alert Panel**: Pops up when discovery engine finds deviations from GR predictions
- **Object Browser**: Sidebar for browsing and navigating to celestial bodies
- **Property Editor**: Edit object properties while flying (mass, radius, density, albedo, spin)
- **Discovery Console**: Live log of discovery engine activity and anomalies
- **Planck Microscope**: Activates when zooming past 10⁻¹⁵ m to visualize quantum foam
- **Theory Comparator**: Dashboard comparing GR with quantum gravity theories

### 5. Solar System Exploration Mode

Features for exploring our cosmic neighborhood:
- **Celestial Body Browser** (right sidebar): Filterable list of stars, planets, exotic objects, moons
- **Object Property Editor**: Edit properties while flying with exotic overrides (negative mass, imaginary mass, phantom energy, charge)
- **NASA Textures**: 8K textures for all major bodies with PBR shaders
- **Measurement Tools**: [Measure Gravity] [Check Equivalence] [Test GR] buttons

### 6. Black Hole & Exotic Physics Mode

Specialized interface near event horizons:
- **Black Hole Control Panel**: Parameter adjustment (mass, spin, charge) with theory comparison
- **Singularity Editor**: Create and test exotic configurations (Schwarzschild, Kerr, Reissner-Nordström, naked, regular)
- **Near-Horizon Physics**: Hawking temperature, lifetime, flux measurement
- **Visualization Controls**: Event horizon, ergosphere, accretion disk, jets

### 7. Planck-Scale & Quantum Gravity Mode

Activates when zooming to quantum scales:
- **Planck Microscope Panel**: Scale indicator from AU to Planck length with quantum foam visualization
- **Theory Selector**: LQG, CDT, Group Field Theory, Causal Sets, Asymptotic Safety
- **Experiments**: Run quantum gravity experiments and compare theories
- **Spectral/Hausdorff Dimension Displays**: Real-time visualization of quantum spacetime properties

### 8. Discovery Engine Integration

The GUI serves as command center for automated discovery:
- **Discovery Console** (expandable bottom panel): Live feed of scans, anomalies, hypotheses
- **Discovery Dashboard** (tab): Anomaly heatmaps, theory space exploration, top discoveries
- **Real-time Alerts**: Visual and audio notifications when anomalies exceed thresholds
- **Hypothesis Generation**: Automatic hypothesis creation with confidence scoring
- **Result Export**: Generate papers, share findings, export data

### 9. VR & Collaborative Mode

When VR headset detected:
- **VR Interface**: OpenXR-based controls for spacetime navigation and object interaction
- **Multi-user Support**: See other users' world-lines, shared cursors, voice chat
- **Time-locking Protocol**: Collaborative research scheduling with proper time synchronization
- **Session Management**: Host/join sessions, share session codes, collaborative experiments

## UI Implementation Details

### ImGui Integration

The UI uses Dear ImGui with custom extensions:
- Docking system for flexible panel layout
- Custom widgets for scientific controls (sliders, dials, vector inputs)
- Theme system with dark/light modes and accessibility options
- High-DPI support for 4K+ displays

### Performance Considerations

- **Immediate Mode GUI**: Efficient rendering with minimal overhead
- **Selective Updates**: Panels only update when values change
- **GPU Acceleration**: ImGui rendering uses same OpenGL context as main visualization
- **Level of Detail**: UI complexity scales with simulation performance

### Customization & Extensibility

- **Panel System**: Easy to add new floating panels via `FloatingPanels` base class
- **Context Menu**: Simple registration of new context menu actions
- **Theming**: Customizable colors, fonts, and layout through ImGui.ini
- **Accessibility**: Keyboard navigation, screen reader support, high contrast modes

## Usage Workflows

### Basic Navigation
1. Launch application to Solar System view
2. Use scroll wheel to zoom in/out on objects
3. Right-click objects for context menu with physics tools
4. Use timeline bar to navigate through simulation time
5. Click toolbar buttons for quick navigation to key locations

### Physics Experimentation
1. Navigate to object of interest (planet, black hole, etc.)
2. Right-click to open context menu
3. Select physics test (e.g., "Drop test particle" for geodesics)
4. Adjust parameters in appearing panels
5. Observe real-time results and deviations from GR
6. Use discovery console to run automated scans

### Discovery Workflow
1. Let discovery engine run in background (automatic scanning)
2. Watch for anomaly alerts in UI
3. Click alerts to view details in discovery console
4. Generate hypotheses or run Bayesian comparisons
5. Export results or share with collaborators

### Collaborative Research
1. Click VR button or start in desktop mode
2. Host session and share session code
3. Navigate to same object as colleagues
4. Propose time locks for synchronized experiments
5. Observe shared cursors and world-lines
6. Receive simultaneous anomaly alerts

## Configuration

### imgui.ini
Automatically generated ImGui configuration file storing:
- Window positions and sizes
- Docking layout
- Theme preferences
- Last used settings

### Customization Options
Accessible through:
- Settings menu (accessible from main menu bar)
- Command line arguments for startup configuration
- Runtime adjustments via UI panels
- Editing imgui.ini directly (advanced users)

## Future Enhancements

Planned UI improvements based on the GUI Upgrade Plan:
- Enhanced VR hand tracking and gesture controls
- Voice command integration ("Show curvature", "Test GR")
- Advanced annotation and measurement tools
- Collaborative whiteboard for theory development
- Adaptive UI that simplifies for beginners, advances for experts
- Exportable UI layouts for different use cases (education, research, presentation)

## Troubleshooting

### Common UI Issues
- **Panels not appearing**: Check if feature is disabled in settings or requires specific context
- **Performance drops**: Reduce UI complexity in settings or disable unused panels
- **Display issues on high-DPI**: Adjust scaling factor in ImGui settings
- **Context menu missing**: Ensure clicking on selectable object, not empty space

### UI Reset
To reset UI to default state:
1. Close application
2. Delete imgui.ini from application directory
3. Restart application - UI will regenerate with defaults

## Conclusion

The QuantumVerse UI transforms complex physics simulation into an intuitive, immersive laboratory where discovery emerges naturally from exploration. By integrating navigation with physics interrogation, users can seamlessly travel from observing planetary orbits to testing quantum gravity at the Planck scale—all within a unified interface that adapts to their context and expertise level.