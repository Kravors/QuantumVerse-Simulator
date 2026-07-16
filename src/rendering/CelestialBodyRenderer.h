/**
 * @file CelestialBodyRenderer.h
 * @brief Celestial body sphere renderer with Phong lighting
 *
 * Renders celestial bodies as lit spheres using UV sphere tessellation
 * with Phong shading, atmospheric glow, and level-of-detail control.
 *
 * References:
 * - Solar System Scope rendering pipeline (solarsystemscope.com)
 * - Blinn-Phong shading model (Jim Blinn, 1977)
 */

#ifndef QUANTUMVERSE_CELESTIAL_BODY_RENDERER_H
#define QUANTUMVERSE_CELESTIAL_BODY_RENDERER_H

#include <vector>
#include <memory>
#include <array>
#include <map>
#include <string>
#include <cmath>
#include "Texture.h"
#include "ProceduralTextures.h"

// Forward declarations to avoid Qt/GL header conflicts in header.
// glad.h is included only in the .cpp implementation files.
// See CurvatureRenderer.h for the same pattern used throughout this project.

namespace quantumverse {
class Event4D;
struct ObjectProperties;
}

namespace quantumverse {

/**
 * @brief Vertex data for a single celestial body instance
 */
struct CelestialBodyInstance {
    std::string objectId;
    std::string name;
    double mass = 0.0;
    double radius = 0.0;
    float color[3] = {0.0f, 0.0f, 0.0f};
    float emissive[3] = {0.0f, 0.0f, 0.0f};
    float position[3] = {0.0f, 0.0f, 0.0f};
    bool isStar = false;
    bool hasAtmosphere = false;
    float atmosphereRadius = 1.0f;
    int textureLayer = -1;
};

/**
 * @brief GPU vertex for celestial body sphere rendering
 */
struct SphereVertex {
    float position[3];     // x, y, z in model space
    float normal[3];       // Normal vector
    float texCoord[2];     // u, v texture coordinates
    float tangent[3];      // Tangent for normal mapping (future use)
};

/**
 * @brief Renderer for celestial bodies (planets, stars, moons)
 *
 * Generates UV sphere geometry and renders with Phong lighting.
 * Supports instanced rendering for multiple bodies with shared geometry.
 */
class CelestialBodyRenderer
{
public:
    /**
     * @brief Quality level for sphere tessellation
     */
    enum class QualityLevel {
        LOW,     ///< 16 stacks x 32 slices (~512 faces)
        MEDIUM,  ///< 32 stacks x 64 slices (~2048 faces)
        HIGH,    ///< 64 stacks x 128 slices (~8192 faces)
        ULTRA    ///< 128 stacks x 256 slices (~32768 faces)
    };

    /**
     * @brief Construct a new Celestial Body Renderer
     * @param quality Tessellation quality level
     * @param maxBodies Maximum number of bodies to render
     */
    explicit CelestialBodyRenderer(QualityLevel quality = QualityLevel::MEDIUM,
                                    size_t maxBodies = 64);
    ~CelestialBodyRenderer();

    // Non-copyable
    CelestialBodyRenderer(const CelestialBodyRenderer&) = delete;
    CelestialBodyRenderer& operator=(const CelestialBodyRenderer&) = delete;

    /**
     * @brief Initialize OpenGL resources (must be called with valid GL context)
     */
    void initializeGL();

    /**
     * @brief Check if OpenGL resources are initialized
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Set the tessellation quality
     * @param quality New quality level (regenerates sphere mesh)
     */
    void setQuality(QualityLevel quality);

    /**
     * @brief Add a celestial body to render
     * @param body Body instance data
     * @return Index of the added body, or -1 on failure
     */
    int addBody(const CelestialBodyInstance& body);

    /**
     * @brief Remove a celestial body by object ID
     * @param objectId ID of the body to remove
     * @return true if body was found and removed
     */
    bool removeBody(const std::string& objectId);

    /**
     * @brief Update body transform and visual properties
     * @param objectId ID of the body to update
     * @param body Updated body data
     * @return true if body was found and updated
     */
    bool updateBody(const std::string& objectId, const CelestialBodyInstance& body);

    /**
     * @brief Clear all bodies
     */
    void clearBodies();

    /**
     * @brief Get number of currently rendered bodies
     */
    size_t bodyCount() const { return m_bodies.size(); }

    /**
     * @brief Set the light source position (for Phong lighting)
     * @param position Light position in world space
     */
    void setLightPosition(const float position[3]);

    /**
     * @brief Set light color and intensity
     * @param color Light color (RGB)
     * @param intensity Light intensity
     */
    void setLightProperties(const float color[3], float intensity);

    /**
     * @brief Set ambient lighting color
     * @param color Ambient color (RGB)
     */
    void setAmbientColor(const float color[3]);

    /**
     * @brief Render all celestial bodies
     * @param viewMatrix 4x4 view matrix (column-major)
     * @param projectionMatrix 4x4 projection matrix (column-major)
     */
    void render(const float* viewMatrix, const float* projectionMatrix);

    /**
     * @brief Render a single body (for custom rendering passes)
     * @param body Body to render
     * @param viewMatrix 4x4 view matrix
     * @param projectionMatrix 4x4 projection matrix
     */
    void renderBody(const CelestialBodyInstance& body,
                    const float* viewMatrix,
                    const float* projectionMatrix);

    /**
     * @brief Set rendering flags
     */
    void setShowAtmospheres(bool show) { m_showAtmospheres = show; }
    void setShowOrbitRings(bool show) { m_showOrbitRings = show; }
    void setWireframe(bool show) { m_wireframe = show; }

    /**
     * @brief Texture array management
     */
    bool loadTextureArray(const std::vector<std::string>& texturePaths,
                        int width = 1024, int height = 512);
    bool generateProceduralTexture(int layerIndex, const PlanetTextureConfig& config);
    void setTextureArrayEnabled(bool enabled) { m_useTextureArray = enabled; }
    bool isTextureArrayEnabled() const { return m_useTextureArray; }
    
    /**
     * @brief Get the procedural texture generator
     */
    ProceduralTextureGenerator& getTextureGenerator() { return m_textureGenerator; }

    /**
     * @brief Get the sphere VAO (for external rendering)
     */
    unsigned int sphereVAO() const { return m_sphereVAO; }
    unsigned int sphereIndexCount() const { return m_indexCount; }

private:
    // Sphere generation
    void generateSphereMesh();
    void generateSphereVAO();

    // Rendering helpers
    void setupShaders();
    void updateBodyBuffers();
    void renderSingleBody(const CelestialBodyInstance& body,
                          unsigned int instanceIndex);

    // OpenGL resources
    unsigned int m_sphereVAO;
    unsigned int m_sphereVBO;
    unsigned int m_sphereEBO;
    unsigned int m_instanceVBO;

    unsigned int m_shaderProgram;
    unsigned int m_atmosphereShaderProgram;

    // Sphere geometry
    std::vector<SphereVertex> m_vertices;
    std::vector<unsigned int> m_indices;
    unsigned int m_indexCount;
    int m_stackCount;
    int m_sliceCount;

    // Body data
    struct BodyData {
        CelestialBodyInstance instance;
        float modelMatrix[16] = {0.0f};
        float modelMatrixIT[9] = {0.0f};
    };
    std::vector<BodyData> m_bodies;
    std::map<std::string, size_t> m_bodyIndexMap;

    // Light properties
    float m_lightPosition[3];
    float m_lightColor[3];
    float m_lightIntensity;
    float m_ambientColor[3];

    // Rendering state
    QualityLevel m_quality;
    bool m_initialized;
    bool m_showAtmospheres;
    bool m_showOrbitRings;
    bool m_wireframe;
    size_t m_maxBodies;
    bool m_buffersDirty;

    // Texture array
    TextureArray m_textureArray;
    bool m_useTextureArray;
    
    // Procedural texture generator
    ProceduralTextureGenerator m_textureGenerator;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_CELESTIAL_BODY_RENDERER_H