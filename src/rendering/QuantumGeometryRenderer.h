/**
 * @file QuantumGeometryRenderer.h
 * @brief Quantum Geometry Visualizer - Render CDT, spin foam, GFT, causal sets
 *
 * Extends CurvatureRenderer to display quantum gravity geometries:
 * - CDT: 4D simplices (tetrahedra) as wireframe or translucent volumes
 * - Spin Foam: Spin network vertices/edges colored by spin value
 * - GFT: Field configuration on group manifold (isosurfaces)
 * - Causal Sets: Hasse diagram in 4D spacetime
 *
 * Features:
 * - Level-of-detail (LOD) based on zoom scale
 * - Color mapping: curvature, spin, age, etc.
 * - 4D->3D projection with slicing
 * - Animation of quantum evolution (Monte Carlo steps)
 *
 * References:
 * - Rovelli (2004): "Quantum Gravity" - spin network diagrams
 * - Ambjørn et al. (2005): CDT geometry visualization
 */

#ifndef QUANTUMVERSE_QUANTUM_GEOMETRY_RENDERER_H
#define QUANTUMVERSE_QUANTUM_GEOMETRY_RENDERER_H

#include "CurvatureRenderer.h"
#include <memory>
#include <vector>
#include <map>
#include <cmath>

// Define M_PI for Visual Studio
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "../../third_party/glad/glad.h"

namespace quantumverse {

// Forward declarations for quantum gravity engines
namespace quantumgravity {
class CDTEngine;
class SpinFoamEngine;
class GFTEngine;
class CausalSetEngine;
} // namespace quantumgravity

// Forward declarations from UI4D.h to avoid circular inclusion
class Camera4D;
class SliceView;

// Forward declarations for quantum gravity engines
namespace quantumgravity {
class CDTEngine;
class SpinFoamEngine;
class GFTEngine;
class CausalSetEngine;
} // namespace quantumgravity

// Import commonly used classes into quantumverse namespace
using quantumgravity::CDTEngine;
using quantumgravity::SpinFoamEngine;
using quantumgravity::GFTEngine;
using quantumgravity::CausalSetEngine;

/**
 * @enum QuantumGeometryType
 * @brief Type of quantum geometry to render
 */
enum class QuantumGeometryType {
    NONE,           // No quantum geometry (classical GR)
    CDT,            // Causal Dynamical Triangulations
    SPIN_FOAM,      // Loop Quantum Gravity spin foam
    GFT,            // Group Field Theory
    CAUSAL_SET      // Causal set theory
};

/**
 * @class QuantumGeometryRenderer
 * @brief OpenGL-based renderer for quantum gravity geometries
 *
 * This class extends CurvatureRenderer with specialized rendering
 * for discrete quantum geometries from various quantum gravity approaches.
 */
class QuantumGeometryRenderer : public CurvatureRenderer
{
private:
    // Active quantum geometry source
    QuantumGeometryType currentType;
    std::shared_ptr<void> quantumSource;  // Typed pointer to active engine

    // Rendering modes
    bool showWireframe;
    bool showVolumes;
    bool showLabels;
    bool animateEvolution;

    // Color schemes
    enum ColorScheme { CURVATURE, SPIN, AGE, TYPE_COUNT };
    ColorScheme currentColorScheme;

    // Vertex buffer objects (VBOs) for different geometry types
    struct GeometryBuffers {
        unsigned int vao = 0;              // Vertex Array Object
        unsigned int vertexVBO = 0;        // Vertex positions (4D)
        unsigned int colorVBO = 0;         // Vertex colors (RGBA)
        unsigned int indexVBO = 0;         // Edge/face indices
        size_t numVertices = 0;
        size_t numIndices = 0;
        bool isValid = false;
    };

    std::map<QuantumGeometryType, GeometryBuffers> geometryCache;

    // Animation state
    double animationTime;
    int currentFrame;                // For time-evolved geometries

    // LOD parameters
    double currentScale;             // Current zoom scale (meters)
    int targetLOD;                   // Target level of detail
    struct LODConfig {
        int maxVertices;             // Maximum vertices to render
        float pointSize;             // Point size for vertices
        float lineWidth;             // Line width for edges
        bool showSimplices;          // Show filled simplices (CDT)
    };
    std::vector<LODConfig> lodLevels;  // Indexed by LOD level (0=low, 3=high)

    // Internal methods
    void generateCDTGeometry(const std::vector<double>& vertices,
                             const std::vector<int>& simplices);
    void generateSpinFoamGeometry(const std::vector<double>& vertices,
                                   const std::vector<int>& edges,
                                   const std::vector<double>& spins);
    void generateGFTGeometry(const std::vector<double>& field,
                              int gridSize);
    void generateCausalSetGeometry(const std::vector<double>& vertices,
                                    const std::vector<int>& edges,
                                    const std::vector<double>& times);

    void updateColorScheme();
    void updateLODFromScale();
    void animateGeometry(double deltaTime);
    void uploadGeometryToGPU(GeometryBuffers& buffers,
                             const std::vector<double>& vertices,
                             const std::vector<int>& indices,
                             const std::vector<double>& colors);

public:
    QuantumGeometryRenderer();
    virtual ~QuantumGeometryRenderer() = default;  // virtual for proper inheritance

    // Set the quantum gravity engine to visualize
    void setCDTEngine(const std::shared_ptr<CDTEngine>& engine);
    void setSpinFoamEngine(const std::shared_ptr<SpinFoamEngine>& engine);
    void setGFTEngine(const std::shared_ptr<GFTEngine>& engine);
    void setCausalSetEngine(const std::shared_ptr<CausalSetEngine>& engine);

    // Clear current geometry
    void clearGeometry();

    // Set current zoom scale (affects LOD)
    void setViewScale(double scale_meters);

    // Set color scheme
    void setColorScheme(ColorScheme scheme) { currentColorScheme = scheme; updateColorScheme(); }

    // Rendering modes
    void setWireframeMode(bool enabled) { showWireframe = enabled; }
    void setVolumeMode(bool enabled) { showVolumes = enabled; }
    void setLabelMode(bool enabled) { showLabels = enabled; }
    void setAnimationEnabled(bool enabled) { animateEvolution = enabled; }

    // Render method with quantum geometry specific parameters
    // Note: This is NOT an override - it has a different signature than CurvatureRenderer::render()
    void render(const Camera4D& camera,
                const std::vector<SliceView>& slices,
                const MetricTensor& metric);

    // Render quantum geometry (called from render())
    void renderQuantumGeometry(const Camera4D& camera);

    // Render a single simplex (tetrahedron in 4D projected to 3D)
    void renderSimplex(const std::array<Event4D, 4>& vertices,
                     const std::array<double, 4>& colors);

    // Render spin network edge
    void renderSpinEdge(const Event4D& v1, const Event4D& v2,
                       double spin, const double color[4]);

    // Get info about current geometry
    struct GeometryInfo {
        size_t vertexCount;
        size_t edgeCount;
        size_t simplexCount;
        QuantumGeometryType type;
        std::string description;
    };
    GeometryInfo getGeometryInfo() const;

    // LOD control
    void setLODLevel(int level);  // 0-3
    int getCurrentLOD() const { return targetLOD; }

    // Get/set current geometry type
    QuantumGeometryType getCurrentType() const { return currentType; }
    void setCurrentType(QuantumGeometryType type) { currentType = type; }

    // Animation
    void stepAnimation();         // Advance one frame
    void resetAnimation();

private:
    // Helper: project 4D point to 3D screen coordinates
    std::array<float, 3> projectToScreen(const Event4D& point,
                                           const Camera4D& camera,
                                           int viewportWidth,
                                           int viewportHeight) const;

    // Helper: compute color from value (curvature, spin, age)
    std::array<float, 4> colorFromValue(double value,
                                       double minVal,
                                       double maxVal,
                                       ColorScheme scheme) const;

    // Private rendering helpers for each quantum geometry type
    void renderCDTGeometry(const std::array<double, 16>& viewMatrix,
                           const std::array<double, 16>& projMatrix);
    void renderSpinFoamGeometry(const std::array<double, 16>& viewMatrix,
                                const std::array<double, 16>& projMatrix);
    void renderGFTGeometry(const std::array<double, 16>& viewMatrix,
                           const std::array<double, 16>& projMatrix);
    void renderCausalSetGeometry(const std::array<double, 16>& viewMatrix,
                                  const std::array<double, 16>& projMatrix);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_QUANTUM_GEOMETRY_RENDERER_H