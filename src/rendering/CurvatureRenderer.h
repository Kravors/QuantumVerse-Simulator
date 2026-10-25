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
#include <cstdint>
#include <string>
// NOTE: glad.h is NOT included here to avoid conflicts with Qt's OpenGL
// headers. Include glad.h BEFORE this header in .cpp files only.
#include "../spacetime/Event4D.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/LightCone.h"
#include "../physics/SingularityHandler.h"
#include "../ml/GeodesicNeuralODE.h"
#include "../utils/ThreadPool.h"

class QVector4D;

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
    void setUniform(const char* name, const QVector4D& color) const;
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
    ShaderProgram ergoShader;     ///< Ergosphere shader
    ShaderProgram computeShader;  ///< Compute shader for grid deformation

    // Compute shader SSBO
    unsigned int gridSSBO;        ///< Shader Storage Buffer Object for grid positions
    int gridWidth;                ///< Grid width (vertices per row)
    int gridHeight;               ///< Grid height (vertices per column)
    bool computeBuffersInitialized;
    bool m_computeDirty;          ///< True when compute shader has updated SSBO

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

    // Undeformed Z for each grid vertex, so deformation is applied
    // relative to the base position rather than accumulating each frame.
    std::vector<float> m_baseZ;

    // World position the BH well centers on. Defaults to the origin; the
    // viewport sets it from the same config source the annulus reads, so the
    // grid well and the accretion disk track the same physics BH.
    float m_blackHoleCenter[3] = {0.0f, 0.0f, 0.0f};

    // BH mass in solar masses, used to scale the well's radius. Defaults to
    // the config value; the viewport updates it on every metric rebuild.
    double m_blackHoleMassSolar = 10.0;

    // Scale factor for grid deformation (maps physical curvature to grid units)
    float m_deformationScale;
    
    // Light cone rendering buffers (reused each frame)
    unsigned int lightConeVao;
    unsigned int lightConeVbo;
    unsigned int lightConeEbo;
    bool lightConeBuffersInitialized;
    
    // Fallback geometry for null renderer guard
    unsigned int fallbackVao;
    unsigned int fallbackVbo;

    // Geodesic line rendering buffers
    unsigned int geodesicVao;
    unsigned int geodesicVbo;
    bool geodesicBuffersInitialized;

    // Neural ODE geodesic rendering (Einstein Ring lensing)
    std::unique_ptr<quantumverse::ml::GeodesicNeuralODE> neuralODE;
    std::unique_ptr<utils::ThreadPool> neuralPool;
    unsigned int neuralGeoVao;
    unsigned int neuralGeoVbo;
    unsigned int neuralGeoVertexCount;
    bool neuralGeoBuffersInitialized;
    bool m_showNeuralGeodesics;

    // Ergosphere rendering buffers (Kerr ergosphere: r_E(θ) = M + √(M² - a²cos²θ))
    unsigned int ergoVao;
    unsigned int ergoVbo;
    unsigned int ergoEbo;
    unsigned int ergoVertexCount;
    bool ergoBuffersInitialized;

    // Ergosphere parameters (updated by QML UI via viewport)
    double m_ergoMass = 10.0;     ///< BH mass in solar masses
    double m_ergoSpin = 0.0;      ///< BH spin parameter a (0 to 0.999)
    double m_ergoCharge = 0.0;    ///< BH charge Q
    double m_lastErgoSpin = -1.0; ///< Last rendered spin (for dirty check)
    double m_lastErgoMass = -1.0; ///< Last rendered mass (for dirty check)
    bool m_showErgosphere = true; ///< Toggle ergosphere visibility

    // Global alpha for fade transitions (Planck Microscope)
    float m_globalAlpha = 1.0f;

    // Cached grid Z range from last compute shader dispatch (for diagnostics)
    float m_gridZMin = 0.0f;
    float m_gridZMax = 0.0f;

    // Last observed metric epoch, used to detect mass changes that require
    // grid re-deformation. Kept in sync with metric->epoch().
    uint64_t m_lastEpoch = 0;

    // Spatial partitioning for optimization
    struct GridCell {
        int startIndex;
        int endIndex;
        bool needsUpdate;
    };
    std::vector<GridCell> gridCells;
    int cellsPerDimension;

    // Plane mode: when true, grid is a 2D plane (rubber sheet) instead of 3D cube
    bool m_planeMode;
    int m_planeResolution;  ///< Resolution for plane mode (higher = smoother curvature)

    // Initialize grid vertices
    void initializeGrid();
    
    // Initialize light cone buffers
    void initializeLightConeBuffers();
    
    // Initialize fallback geometry for null renderer guard
    void initializeFallbackGeometry();
    
    // Initialize spatial partitioning for LOD
    void initializeSpatialPartitioning();
    
    // Generate ergosphere mesh (Kerr frame-dragging boundary)
    void generateErgosphereMesh();
    
    // Compute Riemann curvature tensor component
    double computeRiemannComponent(int rho, int sigma, int mu, int nu, const Event4D& event) const;
    
    // Compute Kretschmann scalar: K = R_ρσμν R^ρσμν
    double computeKretschmannScalar(const Event4D& event) const;
    
    // Compute Ricci scalar: R = g^μν R_μν
    double computeRicciScalar(const Event4D& event) const;
    
    // Deform grid based on spacetime curvature
    void deformGrid();
    
    // Deform grid using compute shader (GPU-accelerated)
    void deformGridGPU();

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

    // Grid deformation range for diagnostics
    std::pair<float, float> gridZRange() const;

    /// @brief Force re-deformation of the grid Z displacement from the current
    /// metric. Called when metric->epoch() changes (e.g. mass sweep).
    /// Reads back the displacement SSBO to keep m_gridZMin/m_gridZMax current.
    void recomputeGridZRange();

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

    /// @brief Current metric used for curvature/deformation.
    std::shared_ptr<MetricTensor> metric() const { return currentMetric; }

    // Render curvature visualization
    void render(const float* viewMatrix, const float* projectionMatrix);

    // Render ergosphere (Kerr frame-dragging boundary)
    void renderErgosphere(const float* viewMatrix, const float* projectionMatrix);

    // Render Neural ODE geodesics (Einstein Ring lensing)
    void renderNeuralGeodesics(const float* viewMatrix, const float* projectionMatrix);

    // Toggle Neural ODE geodesics visibility
    void setShowNeuralGeodesics(bool show);

    // Get Neural ODE geodesics visibility
    bool showNeuralGeodesics() const { return m_showNeuralGeodesics; }

    // Load the ONNX model for Neural ODE geodesic tracing
    bool loadNeuralODEModel(const std::string& onnxPath);

    // Render geodesic lines
    void renderGeodesics(const std::vector<std::vector<Event4D>>& geodesics,
                        const float* viewMatrix, const float* projectionMatrix);

    // Update GL buffer data after deformation
    void updateGLBuffers();

    // Update all vertex colors
    void updateAllColors();

    // Set ergosphere spin parameter (a, clamped [0, 0.999])
    void setErgosphereSpin(double spin);

    // Set ergosphere mass (solar masses)
    void setErgosphereMass(double mass);

    // Set ergosphere charge
    void setErgosphereCharge(double charge);

    // Toggle ergosphere visibility
    void setShowErgosphere(bool show);

    // Get ergosphere visibility
    bool showErgosphere() const { return m_showErgosphere; }

    // Set global alpha multiplier for fade transitions (used by Planck Microscope)
    void setGlobalAlpha(float alpha);
    float globalAlpha() const { return m_globalAlpha; }

    // Set plane mode (2D rubber sheet vs 3D cube)
    void setPlaneMode(bool enable);

    // Get plane mode state
    bool isPlaneMode() const { return m_planeMode; }

    // Set plane mode resolution (vertices per side)
    void setPlaneResolution(int resolution);

    // Get plane mode resolution
    int getPlaneResolution() const { return m_planeResolution; }

    // Set deformation scale multiplier for grid Z displacement
    void setDeformationScale(float scale) { m_deformationScale = scale; }

    // Get deformation scale multiplier
    float deformationScale() const { return m_deformationScale; }

    // World position the BH well centers on (viewport world units)
    void setBlackHoleCenter(float x, float y, float z)
    {
        m_blackHoleCenter[0] = x;
        m_blackHoleCenter[1] = y;
        m_blackHoleCenter[2] = z;
        // Flag for re-deformation on next render pass.
        m_metricDirty = true;
    }
    void blackHoleCenter(float& x, float& y, float& z) const
    {
        x = m_blackHoleCenter[0];
        y = m_blackHoleCenter[1];
        z = m_blackHoleCenter[2];
    }

    // BH mass in solar masses, scales the well's radius (log-scaled so the
    // slider's full range stays legible)
    void setBlackHoleMassSolar(double mass) { m_blackHoleMassSolar = mass; }
    double blackHoleMassSolar() const { return m_blackHoleMassSolar; }

    // BH spin parameter for compute shader (clamped [0, 0.999])
    void setBlackHoleSpin(double spin) { m_ergoSpin = std::clamp(spin, 0.0, 0.999); }
    double blackHoleSpin() const { return m_ergoSpin; }

    /// @brief Dirty flag: when true, deformGrid() must be recomputed before
    /// the next render. Set by setMetric()/setBlackHoleCenter()/updateTime().
    /// Cleared by deformGrid() itself after updating vertex data.
    bool m_metricDirty = true;

    // Compile shader from source strings (implementation in .cpp)
    bool compileShader(unsigned int& program,
                       const char* vertexSource,
                       const char* fragmentSource);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_CURVATURE_RENDERER_H