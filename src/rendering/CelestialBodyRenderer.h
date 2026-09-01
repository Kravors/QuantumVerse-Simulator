/**
 * @file CelestialBodyRenderer.h
 * @brief Celestial body sphere renderer with PBR (Cook-Torrance) lighting
 *
 * Renders celestial bodies as lit spheres using UV sphere tessellation
 * with physically-based rendering (metallic/roughness workflow),
 * atmospheric glow, and level-of-detail control.
 *
 * References:
 * - Solar System Scope rendering pipeline (solarsystemscope.com)
 * - Cook-Torrance BRDF (SIGGRAPH 1982)
 * - "Real Shading in Unreal Engine 4" (Karis 2013)
 * - glTF 2.0 specification (Khronos Group)
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
 * @brief PBR material properties for a celestial body
 */
struct PBRMaterial {
    float metallic = 0.0f;      ///< Metallic factor (0=dielectric, 1=metal)
    float roughness = 0.5f;     ///< Surface roughness (0=mirror, 1=diffuse)
    float ao = 1.0f;            ///< Ambient occlusion (0=occluded, 1=unoccluded)
    int metallicTexLayer = -1;  ///< Texture array layer for metallic map (-1=none)
    int roughnessTexLayer = -1; ///< Texture array layer for roughness map (-1=none)
    int normalTexLayer = -1;    ///< Texture array layer for normal map (-1=none)
    int aoTexLayer = -1;        ///< Texture array layer for AO map (-1=none)
};

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
    PBRMaterial pbrMaterial;    ///< PBR material properties (planets only)
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
     * @brief Set coordinate scale for unit conversion
     * @param scale Meters per viewport unit (e.g., 1.496e10 for 1 AU = 10 units)
     */
    void setCoordinateScale(double scale) { m_coordinateScale = scale; }
    double coordinateScale() const { return m_coordinateScale; }

    /**
     * @brief Texture array management
     */
    bool loadTextureArray(const std::vector<std::string>& texturePaths,
                        int width = 1024, int height = 512);
    bool generateProceduralTexture(int layerIndex, const PlanetTextureConfig& config);
    void setTextureArrayEnabled(bool enabled) { m_useTextureArray = enabled; }
    bool isTextureArrayEnabled() const { return m_useTextureArray; }
    bool initializeTextureArray(int numLayers, int width, int height);
    bool isTextureArrayInitialized() const { return m_textureArray.isValid(); }

    /**
     * @brief PBR texture generation
     * @param layerIndex Base layer index for PBR maps (uses layers base..base+3)
     * @param config Texture configuration
     * @return true on success
     *
     * Generates metallic, roughness, normal, and AO maps procedurally
     * using noise-based algorithms that are consistent with the albedo texture.
     */
    bool generatePBRMaps(int layerIndex, const PlanetTextureConfig& config);

    /**
     * @brief Enable/disable PBR rendering (fallback to legacy Phong)
     */
    void setPBREnabled(bool enabled) { m_usePBR = enabled; }
    bool isPBREnabled() const { return m_usePBR; }

    /**
     * @brief Set environment color for IBL approximation
     * @param color Environment irradiance (RGB)
     */
    void setEnvironmentColor(const float color[3]);

    /**
     * @brief Load texture layer with fallback: file first, then procedural
     * @param layerIndex Layer index in texture array
     * @param filepath Path to texture file (empty to force procedural)
     * @param config Procedural config (used if file missing or forceProcedural=true)
     * @param forceProcedural Skip file loading, use procedural
     * @return true on success
     */
    bool loadTextureLayer(int layerIndex, const std::string& filepath,
                          const PlanetTextureConfig& config,
                          bool forceProcedural = false);
    
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
    unsigned int m_pbrShaderProgram;
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

    // Coordinate scale: meters per viewport unit
    double m_coordinateScale = 1.496e10; // 1 AU = 10 units

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

    // PBR state
    bool m_usePBR;
    float m_envColor[3];
};

} // namespace quantumverse

#endif // QUANTUMVERSE_CELESTIAL_BODY_RENDERER_H