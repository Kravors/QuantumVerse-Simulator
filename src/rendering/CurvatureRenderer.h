/**
 * @file CurvatureRenderer.h
 * @brief QuantumVerse Simulator - Curvature Renderer
 *
 * Shader-based curvature visualization using vertex displacement
 * and fragment-based Riemann tensor coloring
 *
 * Based on: Curved_spacetime.md, Spacetime.md
 * OpenGL 4.5 compatible
 */

#ifndef QUANTUMVERSE_CURVATURE_RENDERER_H
#define QUANTUMVERSE_CURVATURE_RENDERER_H

#include <vector>
#include <array>
#include <memory>
#include <cmath>
// NOTE: glad.h is NOT included here to avoid conflicts with Qt's OpenGL
// headers. Include glad.h BEFORE this header in .cpp files only.
#include "../spacetime/Event4D.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/LightCone.h"
#include "../physics/SingularityHandler.h"

namespace quantumverse {

// Curvature visualization modes
enum class CurvatureMode {
    GRID_DEFORMATION,      ///< Spacetime grid displacement
    RIEMANN_COLOR,         ///< Color-coded Riemann tensor
    CURVATURE_SCALAR,      ///< Kretschmann scalar visualization
    GEODESIC_FLOW,         ///< Geodesic lines on curved surface
    TIME_DILATION          ///< Gravitational time dilation heatmap
};

// Vertex structure for curvature grid
struct CurvatureVertex {
    float position[3];      ///< x, y, z coordinates
    float normal[3];        ///< Normal vector
    float color[4];         ///< RGBA color
    float curvature;        ///< Scalar curvature value
    float time_dilation;    ///< Gravitational time dilation factor
};

// Shader program handle
struct ShaderProgram {
    unsigned int id;

    ShaderProgram() : id(0) {}

    void use() const;
    void release() const;
    void setUniform(const char* name, float value) const;
    void setUniform(const char* name, int value) const;
    void setUniform(const char* name, const float* matrix) const;
    void setUniform(const char* name, const double* matrix) const;
};

class CurvatureRenderer {
protected:
    // Shader programs (protected for QuantumGeometryRenderer access)
    ShaderProgram& getGridShader() { return gridShader; }
    ShaderProgram& getCurvatureShader() { return curvatureShader; }

private:
    // Grid dimensions
    int gridResolution;       ///< Number of vertices per dimension
    float gridSize;           ///< Physical size of grid (meters)
    int lodLevel;             ///< Level of detail (0 = highest, 3 = lowest)
    
    // Rendering state
    CurvatureMode mode;
    bool wireframe;
    bool showGrid;
    bool showLightCones;
    
    // Vertex data
    std::vector<CurvatureVertex> vertices;
    std::vector<unsigned int> indices;
    
    // OpenGL buffers
    unsigned int vao;         ///< Vertex Array Object
    unsigned int vbo;         ///< Vertex Buffer Object
    unsigned int ebo;         ///< Element Buffer Object
    
    // Shader programs
    ShaderProgram gridShader;     ///< Grid deformation shader
    ShaderProgram curvatureShader; ///< Curvature visualization shader
    ShaderProgram geodesicShader; ///< Geodesic line shader
    ShaderProgram overlayShader;  ///< HUD overlay shader
    
    // Current metric
     std::shared_ptr<MetricTensor> currentMetric;
    
    // Light cone visualization
    std::shared_ptr<LightCone> lightCone;
    
    // Singularity influences
    std::vector<std::shared_ptr<class SingularityHandler>> singularities;
    
    // Time parameter for animation
     float time;
     float animationSpeed;  ///< Multiplier for time-dependent deformation
    
    // OpenGL initialization flag
    bool m_initialized;
    
    // Performance optimization caches
    std::vector<double> metricCache;      ///< Cached metric evaluations
    std::vector<double> curvatureCache;   ///< Cached curvature values
    bool cacheValid;                      ///< Whether caches are valid
    
    // Light cone rendering buffers (reused each frame)
    unsigned int lightConeVao;
    unsigned int lightConeVbo;
    unsigned int lightConeEbo;
    bool lightConeBuffersInitialized;
    
    // Fallback geometry for null renderer guard
    unsigned int fallbackVao;
    unsigned int fallbackVbo;
    
    // Spatial partitioning for optimization
    struct GridCell {
        int startIndex;
        int endIndex;
        bool needsUpdate;
    };
    std::vector<GridCell> gridCells;
    int cellsPerDimension;

    // Initialize grid vertices
    void initializeGrid();
    
    // Initialize light cone buffers
    void initializeLightConeBuffers();
    
    // Initialize fallback geometry for null renderer guard
    void initializeFallbackGeometry();
    
    // Initialize spatial partitioning for LOD
    void initializeSpatialPartitioning();
    
    // Compute Riemann curvature tensor component
    double computeRiemannComponent(int rho, int sigma, int mu, int nu, const Event4D& event) const;
    
    // Compute Kretschmann scalar: K = R_ρσμν R^ρσμν
    double computeKretschmannScalar(const Event4D& event) const;
    
    // Compute Ricci scalar: R = g^μν R_μν
    double computeRicciScalar(const Event4D& event) const;
    
    // Deform grid based on spacetime curvature
    void deformGrid();
    
    // Update vertex color based on visualization mode
    void updateVertexColor(CurvatureVertex& vertex);
    
    // Render light cones at grid points
    void renderLightCones(const float* viewMatrix, const float* projectionMatrix);
    
    // Level of detail updates
    void updateLodLevel();
    void updateVerticesByLod();
    void updateAllVertices();
    void updateCellVertices(int cellIndex);
    void updateSingleVertex(int vertexIndex);

public:
    /**
     * @brief Construct a new CurvatureRenderer
     * @param resolution Grid resolution (vertices per dimension)
     * @param size Physical size of the grid in meters
     * @param initialMode Initial visualization mode
     */
    CurvatureRenderer(
        int resolution = 50,
        float size = 100.0f,
        CurvatureMode initialMode = CurvatureMode::GRID_DEFORMATION
    );

    virtual ~CurvatureRenderer();

    // Check if OpenGL resources are initialized
    bool isInitialized() const { return m_initialized; }

    // Initialize OpenGL resources (must be called with valid GL context)
    void initializeGL();

    // Set visualization mode
    void setMode(CurvatureMode newMode);

    // Set wireframe mode
    void setWireframe(bool enable);

    // Get wireframe state
    bool isWireframe() const { return wireframe; }

    // Get current mode
    CurvatureMode getMode() const { return mode; }

    // Toggle grid visibility
    void toggleGrid(bool show);

    // Toggle light cone visualization
    void toggleLightCones(bool show);

    // Set current metric
     void setMetric(std::shared_ptr<MetricTensor> metric);

     // Set light cone for visualization
     void setLightCone(std::shared_ptr<LightCone> cone);

    // Add singularity
    void addSingularity(std::shared_ptr<class SingularityHandler> singularity);

    // Remove singularity
    void removeSingularity(std::shared_ptr<class SingularityHandler> singularity);

    // Update time parameter (advances animation clock)
     void updateTime(float deltaTime);

     // Force immediate grid regeneration from current metric + time
     void regenerateGrid();

     // Set animation speed multiplier
     void setAnimationSpeed(float speed);

    // Get grid vertices
    const std::vector<CurvatureVertex>& getVertices() const;

    // Get grid resolution
    int getResolution() const;

    // Get grid size
    float getSize() const;

    // Render curvature visualization
    void render(const float* viewMatrix, const float* projectionMatrix);

    // Render geodesic lines
    void renderGeodesics(const std::vector<std::vector<Event4D>>& geodesics,
                        const float* viewMatrix, const float* projectionMatrix);

    // Update GL buffer data after deformation
    void updateGLBuffers();

    // Update all vertex colors
    void updateAllColors();

    // Compile shader from source strings (implementation in .cpp)
    bool compileShader(unsigned int& program,
                       const char* vertexSource,
                       const char* fragmentSource);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_CURVATURE_RENDERER_H