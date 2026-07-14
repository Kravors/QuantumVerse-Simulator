/**
 * @file CelestialBodyRenderer.cpp
 * @brief Implementation of celestial body sphere renderer
 */

// glad.h MUST be first to provide OpenGL function prototypes
// before any system <GL/gl.h> is pulled in by other headers
#include "../../third_party/glad/glad.h"

#include "CelestialBodyRenderer.h"
#include "../spacetime/Event4D.h"
#include "Texture.h"
#include "ProceduralTextures.h"

// Ensure M_PI is defined on all platforms
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <cmath>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <QDebug>

namespace quantumverse {

// ============================================================================
// CelestialBodyRenderer
// ============================================================================

CelestialBodyRenderer::CelestialBodyRenderer(QualityLevel quality, size_t maxBodies)
    : m_quality(quality)
    , m_initialized(false)
    , m_showAtmospheres(true)
    , m_showOrbitRings(false)
    , m_wireframe(false)
    , m_maxBodies(maxBodies)
    , m_buffersDirty(false)
    , m_sphereVAO(0)
    , m_sphereVBO(0)
    , m_sphereEBO(0)
    , m_instanceVBO(0)
    , m_shaderProgram(0)
    , m_atmosphereShaderProgram(0)
    , m_indexCount(0)
    , m_stackCount(0)
    , m_sliceCount(0)
    , m_useTextureArray(false)
{
    m_lightPosition[0] = 0.0f;
    m_lightPosition[1] = 0.0f;
    m_lightPosition[2] = 0.0f;
    m_lightColor[0] = 1.0f;
    m_lightColor[1] = 1.0f;
    m_lightColor[2] = 1.0f;
    m_lightIntensity = 1.5f;
    m_ambientColor[0] = 0.08f;
    m_ambientColor[1] = 0.08f;
    m_ambientColor[2] = 0.12f;

    setQuality(quality);
}

CelestialBodyRenderer::~CelestialBodyRenderer()
{
    // Clean up OpenGL resources
    if (m_sphereVAO) glDeleteVertexArrays(1, &m_sphereVAO);
    if (m_sphereVBO) glDeleteBuffers(1, &m_sphereVBO);
    if (m_sphereEBO) glDeleteBuffers(1, &m_sphereEBO);
    if (m_instanceVBO) glDeleteBuffers(1, &m_instanceVBO);
    if (m_shaderProgram) glDeleteProgram(m_shaderProgram);
    if (m_atmosphereShaderProgram) glDeleteProgram(m_atmosphereShaderProgram);
}

void CelestialBodyRenderer::setQuality(QualityLevel quality)
{
    m_quality = quality;
    switch (quality) {
    case QualityLevel::LOW:
        m_stackCount = 16;
        m_sliceCount = 32;
        break;
    case QualityLevel::MEDIUM:
        m_stackCount = 32;
        m_sliceCount = 64;
        break;
    case QualityLevel::HIGH:
        m_stackCount = 64;
        m_sliceCount = 128;
        break;
    case QualityLevel::ULTRA:
        m_stackCount = 128;
        m_sliceCount = 256;
        break;
    }
    generateSphereMesh();
    // generateSphereVAO() is called in initializeGL() when GL context is available
}

// ---------------------------------------------------------------------------
// Sphere mesh generation
// ---------------------------------------------------------------------------

void CelestialBodyRenderer::generateSphereMesh()
{
    m_vertices.clear();
    m_indices.clear();

    // UV sphere: stacks (latitude) x slices (longitude)
    for (int i = 0; i <= m_stackCount; ++i) {
        double phi = M_PI * i / m_stackCount;  // 0 to PI
        double sinPhi = std::sin(phi);
        double cosPhi = std::cos(phi);

        for (int j = 0; j <= m_sliceCount; ++j) {
            double theta = 2.0 * M_PI * j / m_sliceCount;  // 0 to 2PI
            double sinTheta = std::sin(theta);
            double cosTheta = std::cos(theta);

            SphereVertex v;
            v.position[0] = static_cast<float>(sinPhi * cosTheta);
            v.position[1] = static_cast<float>(cosPhi);
            v.position[2] = static_cast<float>(sinPhi * sinTheta);

            // Normals = position on unit sphere
            v.normal[0] = v.position[0];
            v.normal[1] = v.position[1];
            v.normal[2] = v.position[2];

            v.texCoord[0] = static_cast<float>(j) / m_sliceCount;
            v.texCoord[1] = static_cast<float>(i) / m_stackCount;

            // Tangent (for future normal mapping)
            v.tangent[0] = static_cast<float>(-sinTheta);
            v.tangent[1] = 0.0f;
            v.tangent[2] = static_cast<float>(cosTheta);

            m_vertices.push_back(v);
        }
    }

    // Indices
    for (int i = 0; i < m_stackCount; ++i) {
        for (int j = 0; j < m_sliceCount; ++j) {
            int first = i * (m_sliceCount + 1) + j;
            int second = first + m_sliceCount + 1;

            m_indices.push_back(first);
            m_indices.push_back(second);
            m_indices.push_back(first + 1);

            m_indices.push_back(second);
            m_indices.push_back(second + 1);
            m_indices.push_back(first + 1);
        }
    }

    m_indexCount = static_cast<unsigned int>(m_indices.size());
}

void CelestialBodyRenderer::generateSphereVAO()
{
    // Clean up old resources
    if (m_sphereVAO) glDeleteVertexArrays(1, &m_sphereVAO);
    if (m_sphereVBO) glDeleteBuffers(1, &m_sphereVBO);
    if (m_sphereEBO) glDeleteBuffers(1, &m_sphereEBO);
    if (m_instanceVBO) glDeleteBuffers(1, &m_instanceVBO);

    glGenVertexArrays(1, &m_sphereVAO);
    glGenBuffers(1, &m_sphereVBO);
    glGenBuffers(1, &m_sphereEBO);
    glGenBuffers(1, &m_instanceVBO);

    glBindVertexArray(m_sphereVAO);

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, m_sphereVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 m_vertices.size() * sizeof(SphereVertex),
                 m_vertices.data(),
                 GL_STATIC_DRAW);

    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 m_indices.size() * sizeof(unsigned int),
                 m_indices.data(),
                 GL_STATIC_DRAW);

    // Per-vertex attributes
    // Position (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SphereVertex), (void*)offsetof(SphereVertex, position));
    glEnableVertexAttribArray(0);

    // Normal (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SphereVertex), (void*)offsetof(SphereVertex, normal));
    glEnableVertexAttribArray(1);

    // TexCoord (location 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(SphereVertex), (void*)offsetof(SphereVertex, texCoord));
    glEnableVertexAttribArray(2);

    // Tangent (location 3)
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(SphereVertex), (void*)offsetof(SphereVertex, tangent));
    glEnableVertexAttribArray(3);

    // Instance attributes
    // Instance data layout per instance (33 floats = 132 bytes):
    //   [0..15]   modelMatrix (mat4, column-major)
    //   [16..24]  modelMatrixIT (mat3, column-major)
    //   [25..27]  color (vec3)
    //   [28..30]  emissive (vec3)
    //   [31]      radius (float)
    //   [32]      textureLayer (float, cast to int in shader)
    const GLsizeiptr kInstanceStride = 33 * sizeof(float);

    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);

    // modelMatrix columns (locations 4-7), each column is 4 floats
    for (int col = 0; col < 4; ++col) {
        glEnableVertexAttribArray(4 + col);
        glVertexAttribPointer(4 + col, 4, GL_FLOAT, GL_FALSE,
                              kInstanceStride,
                              reinterpret_cast<void*>(static_cast<uintptr_t>(col * 4 * sizeof(float))));
        glVertexAttribDivisor(4 + col, 1);
    }

    // modelMatrixIT columns (locations 8-10), each column is 3 floats
    for (int col = 0; col < 3; ++col) {
        glEnableVertexAttribArray(8 + col);
        glVertexAttribPointer(8 + col, 3, GL_FLOAT, GL_FALSE,
                              kInstanceStride,
                              reinterpret_cast<void*>(static_cast<uintptr_t>(16 * sizeof(float) + col * 3 * sizeof(float))));
        glVertexAttribDivisor(8 + col, 1);
    }

    // Color (location 11)
    glEnableVertexAttribArray(11);
    glVertexAttribPointer(11, 3, GL_FLOAT, GL_FALSE,
                          kInstanceStride,
                          reinterpret_cast<void*>(static_cast<uintptr_t>(25 * sizeof(float))));
    glVertexAttribDivisor(11, 1);

    // Emissive (location 12)
    glEnableVertexAttribArray(12);
    glVertexAttribPointer(12, 3, GL_FLOAT, GL_FALSE,
                          kInstanceStride,
                          reinterpret_cast<void*>(static_cast<uintptr_t>(28 * sizeof(float))));
    glVertexAttribDivisor(12, 1);

    // Radius (location 13)
    glEnableVertexAttribArray(13);
    glVertexAttribPointer(13, 1, GL_FLOAT, GL_FALSE,
                          kInstanceStride,
                          reinterpret_cast<void*>(static_cast<uintptr_t>(31 * sizeof(float))));
    glVertexAttribDivisor(13, 1);

    // Texture layer index (location 14) - float attribute, cast to int in shader
    glEnableVertexAttribArray(14);
    glVertexAttribPointer(14, 1, GL_FLOAT, GL_FALSE,
                          kInstanceStride,
                          reinterpret_cast<void*>(static_cast<uintptr_t>(32 * sizeof(float))));
    glVertexAttribDivisor(14, 1);

    glBindVertexArray(0);
}

// ---------------------------------------------------------------------------
// Shader compilation
// ---------------------------------------------------------------------------

namespace {

const char* celestialVertSrc = R"(
#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;

// Instance attributes
// Layout: model(16) + modelIT(9) + color(3) + emissive(3) + radius(1) + texLayer(1) = 33 floats
layout(location = 4) in mat4 aModelMatrix;
layout(location = 8) in mat3 aModelMatrixIT;
layout(location = 11) in vec3 aColor;
layout(location = 12) in vec3 aEmissive;
layout(location = 13) in float aRadius;
layout(location = 14) in float aTexLayer;

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform float scaleFactor;  // Converts meters to scene units
uniform int textureArrayEnabled;

out VS_OUT {
    vec3 worldPos;
    vec3 normal;
    vec2 texCoord;
    vec3 color;
    vec3 emissive;
    vec3 viewDir;
    flat float texLayer;
} vs_out;

void main() {
    // Transform position to world space
    vec4 worldPos4 = aModelMatrix * vec4(aPos * aRadius, 1.0);
    vs_out.worldPos = worldPos4.xyz;

    // Normal in world space (use inverse-transpose for non-uniform scale)
    vs_out.normal = normalize(aModelMatrixIT * aNormal);

    vs_out.texCoord = aTexCoord;
    vs_out.color = aColor;
    vs_out.emissive = aEmissive;
    vs_out.texLayer = aTexLayer;

    // View direction
    vec4 viewPos = viewMatrix * worldPos4;
    vs_out.viewDir = -viewPos.xyz;

    gl_Position = projectionMatrix * viewPos;
}
)";

const char* celestialFragSrc = R"(
#version 450 core

in VS_OUT {
    vec3 worldPos;
    vec3 normal;
    vec2 texCoord;
    vec3 color;
    vec3 emissive;
    vec3 viewDir;
    flat float texLayer;
} fs_in;

uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform float lightIntensity;
uniform vec3 ambientColor;
uniform float time;
uniform int textureArrayEnabled;
uniform sampler2DArray textureArray;

out vec4 outColor;

void main() {
    vec3 N = normalize(fs_in.normal);
    vec3 L = normalize(lightPosition - fs_in.worldPos);
    vec3 V = normalize(fs_in.viewDir);
    vec3 H = normalize(L + V);

    // Sample texture if enabled and layer is valid
    vec4 texColor = vec4(1.0);
    if (textureArrayEnabled > 0 && fs_in.texLayer >= 0.0) {
        texColor = texture(textureArray, vec3(fs_in.texCoord, fs_in.texLayer));
    }

    // Ambient
    vec3 ambient = ambientColor * fs_in.color * texColor.rgb;

    // Diffuse (Lambertian)
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * fs_in.color * texColor.rgb * lightColor * lightIntensity;

    // Specular (Blinn-Phong)
    float spec = pow(max(dot(N, H), 0.0), 64.0);
    vec3 specular = spec * vec3(0.3) * lightColor * lightIntensity;

    // Atmospheric glow at edges
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    vec3 atmosphere = fresnel * vec3(0.2, 0.4, 0.8) * 0.5;

    // Emissive glow
    vec3 emission = fs_in.emissive * lightColor * 0.5;

    vec3 result = ambient + diffuse + specular + atmosphere + emission;

    // Gamma correction
    result = pow(result, vec3(1.0 / 2.2));

    outColor = vec4(result, texColor.a);
}
)";

const char* atmosphereVertSrc = R"(
#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

layout(location = 4) in mat4 aModelMatrix;
layout(location = 13) in float aRadius;

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform float atmosphereScale;  // e.g., 1.1 for 10% larger

void main() {
    vec4 worldPos4 = aModelMatrix * vec4(aPos * aRadius * atmosphereScale, 1.0);
    gl_Position = projectionMatrix * viewMatrix * worldPos4;
}
)";

const char* atmosphereFragSrc = R"(
#version 450 core
out vec4 outColor;

void main() {
    outColor = vec4(0.3, 0.5, 1.0, 0.15);  // Subtle blue atmosphere
}
)";

bool compileShader(GLuint& program, const char* vertSrc, const char* fragSrc)
{
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &vertSrc, nullptr);
    glCompileShader(vertShader);

    GLint success;
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertShader, 512, nullptr, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
        glDeleteShader(vertShader);
        return false;
    }

    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragSrc, nullptr);
    glCompileShader(fragShader);

    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragShader, 512, nullptr, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
        glDeleteShader(fragShader);
        glDeleteShader(vertShader);
        return false;
    }

    program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader linking failed: " << infoLog << std::endl;
        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    return program != 0;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// OpenGL initialization
// ---------------------------------------------------------------------------

void CelestialBodyRenderer::initializeGL()
{
    if (!gladLoadGL()) {
        std::cerr << "CelestialBodyRenderer: Failed to load GLAD" << std::endl;
        return;
    }

    generateSphereVAO();

    // Compile shaders
    if (!compileShader(m_shaderProgram, celestialVertSrc, celestialFragSrc)) {
        std::cerr << "CelestialBodyRenderer: Failed to compile main shader" << std::endl;
        return;
    }

    if (!compileShader(m_atmosphereShaderProgram, atmosphereVertSrc, atmosphereFragSrc)) {
        std::cerr << "CelestialBodyRenderer: Failed to compile atmosphere shader" << std::endl;
        return;
    }

    m_initialized = true;
    m_buffersDirty = true;

    std::cout << "CelestialBodyRenderer initialized (stacks=" << m_stackCount
              << ", slices=" << m_sliceCount << ")" << std::endl;
}

// ---------------------------------------------------------------------------
// Body management
// ---------------------------------------------------------------------------

int CelestialBodyRenderer::addBody(const CelestialBodyInstance& body)
{
    if (m_bodies.size() >= m_maxBodies) {
        std::cerr << "CelestialBodyRenderer: Maximum body count reached ("
                  << m_maxBodies << ")" << std::endl;
        return -1;
    }

    BodyData data;
    data.instance = body;

    // The vertex shader already applies aRadius to aPos, so the model matrix
    // must be translation-only (unit scale). Baking radius into the scale here
    // too would apply it twice (radius^2), shrinking bodies to invisibility.
    float* m = data.modelMatrix;
    std::memset(m, 0, 16 * sizeof(float));
    m[0] = 1.0f;  m[5] = 1.0f;  m[10] = 1.0f;  m[15] = 1.0f;
    m[12] = body.position[0];
    m[13] = body.position[1];
    m[14] = body.position[2];

    // Inverse transpose (uniform scale = 1 -> identity)
    std::memset(data.modelMatrixIT, 0, 9 * sizeof(float));
    data.modelMatrixIT[0] = 1.0f;
    data.modelMatrixIT[4] = 1.0f;
    data.modelMatrixIT[8] = 1.0f;

    m_bodyIndexMap[body.objectId] = m_bodies.size();
    m_bodies.push_back(data);
    m_buffersDirty = true;

    return static_cast<int>(m_bodies.size() - 1);
}

bool CelestialBodyRenderer::removeBody(const std::string& objectId)
{
    auto it = m_bodyIndexMap.find(objectId);
    if (it == m_bodyIndexMap.end()) return false;

    size_t idx = it->second;
    m_bodies.erase(m_bodies.begin() + idx);
    m_bodyIndexMap.erase(it);

    // Rebuild index map
    m_bodyIndexMap.clear();
    for (size_t i = 0; i < m_bodies.size(); ++i) {
        m_bodyIndexMap[m_bodies[i].instance.objectId] = i;
    }

    m_buffersDirty = true;
    return true;
}

bool CelestialBodyRenderer::updateBody(const std::string& objectId,
                                        const CelestialBodyInstance& body)
{
    auto it = m_bodyIndexMap.find(objectId);
    if (it == m_bodyIndexMap.end()) return false;

    size_t idx = it->second;
    m_bodies[idx].instance = body;

    // Update model matrix (translation only; shader applies aRadius)
    float* m = m_bodies[idx].modelMatrix;
    std::memset(m, 0, 16 * sizeof(float));
    m[0] = 1.0f; m[5] = 1.0f; m[10] = 1.0f; m[15] = 1.0f;
    m[12] = body.position[0];
    m[13] = body.position[1];
    m[14] = body.position[2];

    std::memset(m_bodies[idx].modelMatrixIT, 0, 9 * sizeof(float));
    m_bodies[idx].modelMatrixIT[0] = 1.0f;
    m_bodies[idx].modelMatrixIT[4] = 1.0f;
    m_bodies[idx].modelMatrixIT[8] = 1.0f;

    m_buffersDirty = true;
    return true;
}

void CelestialBodyRenderer::clearBodies()
{
    m_bodies.clear();
    m_bodyIndexMap.clear();
    m_buffersDirty = true;
}

void CelestialBodyRenderer::setLightPosition(const float position[3])
{
    std::memcpy(m_lightPosition, position, 3 * sizeof(float));
}

void CelestialBodyRenderer::setLightProperties(const float color[3], float intensity)
{
    std::memcpy(m_lightColor, color, 3 * sizeof(float));
    m_lightIntensity = intensity;
}

void CelestialBodyRenderer::setAmbientColor(const float color[3])
{
    std::memcpy(m_ambientColor, color, 3 * sizeof(float));
}

/**
 * @brief Generate a procedural planet texture and add it to the array
 * @param layerIndex Which layer in the array to fill
 * @param config Texture configuration parameters
 * @return true on success
 *
 * Generates a noise-based procedural texture using the ProceduralTextureGenerator.
 */
bool CelestialBodyRenderer::generateProceduralTexture(int layerIndex, const PlanetTextureConfig& config)
{
    if (!m_textureArray.isValid()) {
        std::cerr << "CelestialBodyRenderer: Texture array not initialized" << std::endl;
        return false;
    }

    if (layerIndex < 0 || layerIndex >= m_textureArray.getMaxLayers()) {
        std::cerr << "CelestialBodyRenderer: Invalid texture layer index " << layerIndex << std::endl;
        return false;
    }

    // Generate texture using the procedural texture generator
    std::vector<uint8_t> pixels = m_textureGenerator.generatePlanetTexture(config);

    glBindTexture(GL_TEXTURE_2D_ARRAY, m_textureArray.getId());
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                    0, 0, layerIndex,
                    config.width, config.height, 1,
                    GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    return true;
}

bool CelestialBodyRenderer::loadTextureArray(const std::vector<std::string>& texturePaths,
                                               int width, int height)
{
    // Calculate number of layers: use provided paths, or default to 1 for procedural textures
    int numLayers = static_cast<int>(texturePaths.size());
    if (numLayers == 0) {
        numLayers = 1; // Default to 1 layer for procedural texture generation
    }

    if (!m_textureArray.initialize(numLayers, width, height)) {
        std::cerr << "CelestialBodyRenderer: Failed to initialize texture array" << std::endl;
        return false;
    }

    for (size_t i = 0; i < texturePaths.size(); ++i) {
        int layer = m_textureArray.addLayerFromFile(texturePaths[i]);
        if (layer < 0) {
            std::cerr << "CelestialBodyRenderer: Failed to load texture layer " << i
                      << " from '" << texturePaths[i] << "'" << std::endl;
            // Continue loading other layers
        }
    }

    m_useTextureArray = true;
    std::cout << "CelestialBodyRenderer: Initialized texture array with " << m_textureArray.getMaxLayers()
              << " layers (" << width << "x" << height << ")" << std::endl;
    return true;
}

// ---------------------------------------------------------------------------
// Instance buffer update
// ---------------------------------------------------------------------------

void CelestialBodyRenderer::updateBodyBuffers()
{
    if (!m_buffersDirty || m_bodies.empty()) return;

    // Build instance data: model(16) + modelIT(9) + color(3) + emissive(3) + radius(1) + texLayer(1) = 33 floats
    std::vector<float> instanceData;
    instanceData.reserve(m_bodies.size() * 33);

    for (const auto& body : m_bodies) {
        // Model matrix (16 floats)
        for (int i = 0; i < 16; ++i)
            instanceData.push_back(body.modelMatrix[i]);

        // Inverse-transpose (9 floats)
        for (int i = 0; i < 9; ++i)
            instanceData.push_back(body.modelMatrixIT[i]);

        // Color (3 floats)
        instanceData.push_back(body.instance.color[0]);
        instanceData.push_back(body.instance.color[1]);
        instanceData.push_back(body.instance.color[2]);

        // Emissive (3 floats)
        instanceData.push_back(body.instance.emissive[0]);
        instanceData.push_back(body.instance.emissive[1]);
        instanceData.push_back(body.instance.emissive[2]);

        // Radius (1 float)
        instanceData.push_back(static_cast<float>(body.instance.radius));

        // Texture layer index (1 float, cast to int in shader)
        instanceData.push_back(static_cast<float>(body.instance.textureLayer));
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 instanceData.size() * sizeof(float),
                 instanceData.data(),
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_buffersDirty = false;
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void CelestialBodyRenderer::render(const float* viewMatrix, const float* projectionMatrix)
{
    qWarning("[DIAG-CelestialRender] render() called, body count = %zu", m_bodies.size());
    if (!m_initialized || m_bodies.empty()) return;

    updateBodyBuffers();

    // Render atmospheres first (semi-transparent, back-to-front would be ideal)
    if (m_showAtmospheres) {
        glUseProgram(m_atmosphereShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(m_atmosphereShaderProgram, "viewMatrix"),
                           1, GL_FALSE, viewMatrix);
        glUniformMatrix4fv(glGetUniformLocation(m_atmosphereShaderProgram, "projectionMatrix"),
                           1, GL_FALSE, projectionMatrix);
        glUniform1f(glGetUniformLocation(m_atmosphereShaderProgram, "atmosphereScale"), 1.1f);

        glBindVertexArray(m_sphereVAO);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDrawElementsInstanced(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0,
                                static_cast<GLsizei>(m_bodies.size()));
        glDisable(GL_BLEND);
    }

    // Render solid bodies
    glUseProgram(m_shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "viewMatrix"),
                       1, GL_FALSE, viewMatrix);
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projectionMatrix"),
                       1, GL_FALSE, projectionMatrix);
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "lightPosition"), 1, m_lightPosition);
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "lightColor"), 1, m_lightColor);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "lightIntensity"), m_lightIntensity);
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "ambientColor"), 1, m_ambientColor);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "scaleFactor"), 1.0f);

    // Texture array binding
    if (m_useTextureArray && m_textureArray.isValid()) {
        glUniform1i(glGetUniformLocation(m_shaderProgram, "textureArrayEnabled"), 1);
        m_textureArray.bind(0);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "textureArray"), 0);
    } else {
        glUniform1i(glGetUniformLocation(m_shaderProgram, "textureArrayEnabled"), 0);
    }

    glBindVertexArray(m_sphereVAO);

    if (m_wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    glDrawElementsInstanced(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0,
                            static_cast<GLsizei>(m_bodies.size()));

    if (m_wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // Unbind texture array
    if (m_useTextureArray && m_textureArray.isValid()) {
        TextureArray::unbind(0);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

void CelestialBodyRenderer::renderBody(const CelestialBodyInstance& body,
                                         const float* viewMatrix,
                                         const float* projectionMatrix)
{
    if (!m_initialized) return;

    glUseProgram(m_shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "viewMatrix"),
                       1, GL_FALSE, viewMatrix);
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projectionMatrix"),
                       1, GL_FALSE, projectionMatrix);
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "lightPosition"), 1, m_lightPosition);
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "lightColor"), 1, m_lightColor);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "lightIntensity"), m_lightIntensity);
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "ambientColor"), 1, m_ambientColor);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "scaleFactor"), 1.0f);

    // Texture array binding for single body render
    if (m_useTextureArray && m_textureArray.isValid() && body.textureLayer >= 0) {
        glUniform1i(glGetUniformLocation(m_shaderProgram, "textureArrayEnabled"), 1);
        m_textureArray.bind(0);
        glUniform1i(glGetUniformLocation(m_shaderProgram, "textureArray"), 0);
    } else {
        glUniform1i(glGetUniformLocation(m_shaderProgram, "textureArrayEnabled"), 0);
    }

    // Build instance data for this single body
    // Layout: model(16) + modelIT(9) + color(3) + emissive(3) + radius(1) + texLayer(1) = 33 floats
    float instanceData[33];
    std::memset(instanceData, 0, 33 * sizeof(float));

    float scale = static_cast<float>(body.radius);  // radius consumed by shader (aRadius)
    // Model matrix: translation only (shader applies aRadius), so unit scale
    instanceData[0] = 1.0f;
    instanceData[5] = 1.0f;
    instanceData[10] = 1.0f;
    instanceData[15] = 1.0f;
    instanceData[12] = body.position[0];
    instanceData[13] = body.position[1];
    instanceData[14] = body.position[2];

    // Inverse-transpose (uniform scale = 1 -> identity)
    instanceData[16] = 1.0f;
    instanceData[20] = 1.0f;
    instanceData[24] = 1.0f;

    // Color
    instanceData[25] = body.color[0];
    instanceData[26] = body.color[1];
    instanceData[27] = body.color[2];

    // Emissive
    instanceData[28] = body.emissive[0];
    instanceData[29] = body.emissive[1];
    instanceData[30] = body.emissive[2];

    // Radius
    instanceData[31] = scale;

    // Texture layer
    instanceData[32] = static_cast<float>(body.textureLayer);

    // Upload instance data to the instance VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, 33 * sizeof(float), instanceData, GL_DYNAMIC_DRAW);

    glBindVertexArray(m_sphereVAO);
    glDrawElementsInstanced(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0, 1);
    glBindVertexArray(0);

    // Unbind texture array
    if (m_useTextureArray && m_textureArray.isValid() && body.textureLayer >= 0) {
        TextureArray::unbind(0);
    }

    glUseProgram(0);
}

} // namespace quantumverse