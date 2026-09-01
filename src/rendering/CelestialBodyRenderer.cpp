/**
 * @file CelestialBodyRenderer.cpp
 * @brief Implementation of celestial body sphere renderer
 */

// glad.h MUST be first to provide OpenGL function prototypes
// before any system <GL/gl.h> is pulled in by other headers
#include "glad.h"

#include "CelestialBodyRenderer.h"
#include "../spacetime/Event4D.h"
#include "Texture.h"
#include "ProceduralTextures.h"
#include <fstream>

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
    , m_pbrShaderProgram(0)
    , m_usePBR(true)
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
    m_envColor[0] = 0.02f;
    m_envColor[1] = 0.02f;
    m_envColor[2] = 0.03f;

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
    if (m_pbrShaderProgram) glDeleteProgram(m_pbrShaderProgram);
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
    // Instance data layout per instance (36 floats = 144 bytes):
    //   [0..15]   modelMatrix (mat4, column-major)
    //   [16..24]  modelMatrixIT (mat3, column-major)
    //   [25..27]  color (vec3)
    //   [28..30]  emissive (vec3)
    //   [31]      radius (float)
    //   [32..35]  packedMRAB (vec4: metallic, roughness, ao, texLayer)
    const GLsizeiptr kInstanceStride = 36 * sizeof(float);

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

    // Packed metallic/roughness/ao/texLayer (location 14) as vec4
    // Layout: [0..15] modelMatrix(16) + [16..24] modelMatrixIT(9) + [25..27] color(3) +
    //         [28..30] emissive(3) + [31] radius(1) + [32..35] packedMRAB(4) = 36 floats
    glEnableVertexAttribArray(14);
    glVertexAttribPointer(14, 4, GL_FLOAT, GL_FALSE,
                          kInstanceStride,
                          reinterpret_cast<void*>(static_cast<uintptr_t>(32 * sizeof(float))));
    glVertexAttribDivisor(14, 1);
    // Locations 15, 16, 17 are no longer used (packed into location 14)

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

// Instance attributes
// Layout: model(16) + modelIT(9) + color(3) + emissive(3) + radius(1) + packedMRAB(4) = 36 floats
// PackedMRAB = vec4(metallic, roughness, ao, texLayer) - saves one attribute location
layout(location = 4) in mat4 aModelMatrix;
layout(location = 8) in vec3 aModelMatrixIT_col0;
layout(location = 9) in vec3 aModelMatrixIT_col1;
layout(location = 10) in vec3 aModelMatrixIT_col2;
layout(location = 11) in vec3 aColor;
layout(location = 12) in vec3 aEmissive;
layout(location = 13) in float aRadius;
layout(location = 14) in vec4 aPackedMRAB; // x=metallic, y=roughness, z=ao, w=texLayer

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform float scaleFactor;
uniform int textureArrayEnabled;

out VS_OUT {
    vec3 worldPos;
    vec3 normal;
    vec2 texCoord;
    vec3 color;
    vec3 emissive;
    vec3 viewDir;
    flat float texLayer;
    flat float metallic;
    flat float roughness;
    flat float ao;
} vs_out;

void main() {
    vec4 worldPos4 = aModelMatrix * vec4(aPos * aRadius, 1.0);
    // Reconstruct mat3 from 3 vec3 attributes
    mat3 modelIT = mat3(aModelMatrixIT_col0, aModelMatrixIT_col1, aModelMatrixIT_col2);
    vs_out.worldPos = worldPos4.xyz;
    vs_out.normal = normalize(modelIT * aNormal);
    vs_out.texCoord = aTexCoord;
    vs_out.color = aColor;
    vs_out.emissive = aEmissive;
    vs_out.metallic = aPackedMRAB.x;
    vs_out.roughness = aPackedMRAB.y;
    vs_out.ao = aPackedMRAB.z;
    vs_out.texLayer = aPackedMRAB.w;

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
    flat float metallic;
    flat float roughness;
    flat float ao;
} fs_in;

uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform float lightIntensity;
uniform vec3 ambientColor;
uniform float time;
uniform int textureArrayEnabled;
uniform sampler2DArray textureArray;
uniform int debugMode;

out vec4 outColor;

void main() {
    vec3 N = normalize(fs_in.normal);
    vec3 L = normalize(lightPosition - fs_in.worldPos);
    vec3 V = normalize(fs_in.viewDir);
    vec3 H = normalize(L + V);

    vec4 texColor = vec4(1.0);
    if (textureArrayEnabled > 0 && fs_in.texLayer >= 0.0) {
        texColor = texture(textureArray, vec3(fs_in.texCoord, fs_in.texLayer));
    }

    if (debugMode == 1) { outColor = vec4(1.0, 0.0, 0.0, 1.0); return; }
    if (debugMode == 2) { outColor = vec4(texColor.a); return; }
    if (debugMode == 3) { outColor = texColor; return; }

    vec3 ambient = ambientColor * fs_in.color * texColor.rgb;
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * fs_in.color * texColor.rgb * lightColor * lightIntensity;
    float spec = pow(max(dot(N, H), 0.0), 64.0);
    vec3 specular = spec * vec3(0.3) * lightColor * lightIntensity;
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    vec3 atmosphere = fresnel * vec3(0.2, 0.4, 0.8) * 0.5;
    vec3 emission = fs_in.emissive * lightColor * 0.5;
    vec3 result = ambient + diffuse + specular + atmosphere + emission;
    result = pow(result, vec3(1.0 / 2.2));
    outColor = vec4(result, texColor.a);
}
)";

// PBR Cook-Torrance fragment shader (disabled - uses 18 vertex attributes,
// but OpenGL guarantees only 16). TODO: refactor to use uniform buffers.
#if 0
const char* pbrFragSrc = R"(
#version 450 core

in VS_OUT {
    vec3 worldPos;
    vec3 normal;
    vec2 texCoord;
    vec3 color;
    vec3 emissive;
    vec3 viewDir;
    flat float texLayer;
    flat float metallic;
    flat float roughness;
    flat float ao;
} fs_in;

uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform float lightIntensity;
uniform vec3 ambientColor;
uniform vec3 envColor;        // Environment irradiance for IBL approximation
uniform int textureArrayEnabled;
uniform sampler2DArray textureArray;
uniform int pbrMetallicTexLayer;
uniform int pbrRoughnessTexLayer;
uniform int pbrNormalTexLayer;
uniform int pbrAOTexLayer;
uniform int debugMode;

out vec4 outColor;

const float PI = 3.14159265359;

// GGX/Trowbridge-Reitz normal distribution function
float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / max(denom, 0.0001);
}

// Smith geometry function (Schlick-GGX)
float geometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

// Smith's method combining both view and light geometry terms
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

// Fresnel-Schlick approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 N = normalize(fs_in.normal);
    vec3 V = normalize(fs_in.viewDir);

    // Sample PBR texture maps
    float metallic = fs_in.metallic;
    float roughness = fs_in.roughness;
    float ao = fs_in.ao;

    if (textureArrayEnabled > 0) {
        if (fs_in.texLayer >= 0.0) {
            vec4 albedoSample = texture(textureArray, vec3(fs_in.texCoord, fs_in.texLayer));
            // albedo is used as base color; PBR maps modulate it
        }
        if (pbrMetallicTexLayer >= 0) {
            metallic *= texture(textureArray, vec3(fs_in.texCoord, float(pbrMetallicTexLayer))).r;
        }
        if (pbrRoughnessTexLayer >= 0) {
            roughness *= texture(textureArray, vec3(fs_in.texCoord, float(pbrRoughnessTexLayer))).r;
        }
        if (pbrAOTexLayer >= 0) {
            ao *= texture(textureArray, vec3(fs_in.texCoord, float(pbrAOTexLayer))).r;
        }
    }

    // Clamp PBR parameters
    metallic = clamp(metallic, 0.0, 1.0);
    roughness = clamp(roughness, 0.04, 1.0);

    // Sample albedo texture
    vec3 albedo = fs_in.color;
    if (textureArrayEnabled > 0 && fs_in.texLayer >= 0.0) {
        vec4 texColor = texture(textureArray, vec3(fs_in.texCoord, fs_in.texLayer));
        albedo *= texColor.rgb;
    }

    // Debug modes
    if (debugMode == 1) { outColor = vec4(1.0, 0.0, 0.0, 1.0); return; }
    if (debugMode == 2) { outColor = vec4(vec3(metallic), 1.0); return; }
    if (debugMode == 3) { outColor = vec4(vec3(roughness), 1.0); return; }

    // F0: reflectance at normal incidence (dielectric=0.04, metal=albedo)
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Light contribution
    vec3 L = normalize(lightPosition - fs_in.worldPos);
    vec3 H = normalize(V + L);
    float distance = length(lightPosition - fs_in.worldPos);
    float attenuation = 1.0 / (1.0 + 0.0001 * distance * distance);
    vec3 radiance = lightColor * lightIntensity * attenuation;

    // Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    // Energy conservation: diffuse fraction
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);

    // Direct lighting
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

    // Ambient (IBL approximation using environment color)
    vec3 ambient = (envColor * albedo * ao) * (1.0 - metallic);

    // Atmospheric fresnel rim
    float fresnelRim = pow(1.0 - max(dot(N, V), 0.0), 5.0);
    vec3 atmosphere = fresnelRim * vec3(0.15, 0.3, 0.7) * ao;

    // Emission
    vec3 emission = fs_in.emissive * lightColor * 0.5;

    vec3 result = ambient + Lo + atmosphere + emission;

    // Tone mapping (ACES approximation)
    result = result / (result + vec3(1.0));
    // Gamma correction
    result = pow(result, vec3(1.0 / 2.2));

    float alpha = 1.0;
    if (textureArrayEnabled > 0 && fs_in.texLayer >= 0.0) {
        alpha = texture(textureArray, vec3(fs_in.texCoord, fs_in.texLayer)).a;
    }

    outColor = vec4(result, alpha);
}
)";
#endif

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
    generateSphereVAO();

    // Compile shaders
    if (!compileShader(m_shaderProgram, celestialVertSrc, celestialFragSrc)) {
        std::cerr << "CelestialBodyRenderer: Failed to compile legacy shader" << std::endl;
        return;
    }

    // PBR shader disabled: vertex shader uses 18 attribute locations (0-17),
    // but OpenGL guarantees only 16 (GL_MAX_VERTEX_ATTRIBS). Need to refactor
    // to use uniform buffers or reduce attribute count before re-enabling.
    // if (!compileShader(m_pbrShaderProgram, celestialVertSrc, pbrFragSrc)) {
    //     std::cerr << "CelestialBodyRenderer: Failed to compile PBR shader" << std::endl;
    //     return;
    // }
    m_pbrShaderProgram = m_shaderProgram; // Use legacy shader as fallback

    if (!compileShader(m_atmosphereShaderProgram, atmosphereVertSrc, atmosphereFragSrc)) {
        std::cerr << "CelestialBodyRenderer: Failed to compile atmosphere shader" << std::endl;
        return;
    }

    m_initialized = true;
    m_buffersDirty = true;

    std::cout << "CelestialBodyRenderer initialized (stacks=" << m_stackCount
              << ", slices=" << m_sliceCount << ", PBR enabled)" << std::endl;
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
    if (pixels.empty()) {
        std::cerr << "CelestialBodyRenderer: Generated empty pixels for layer " << layerIndex << std::endl;
        return false;
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, m_textureArray.getId());
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                    0, 0, layerIndex,
                    config.width, config.height, 1,
                    GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    static int s_texLog = 0;
    if (s_texLog++ < 10) {
        std::cout << "[Texture] Generated layer " << layerIndex
                  << " (" << config.width << "x" << config.height << ")"
                  << " pixels: " << pixels.size() << std::endl;
    }

    return true;
}

bool CelestialBodyRenderer::initializeTextureArray(int numLayers, int width, int height)
{
    if (!m_textureArray.initialize(numLayers, width, height)) {
        std::cerr << "CelestialBodyRenderer: Failed to initialize texture array" << std::endl;
        return false;
    }
    m_useTextureArray = true;
    std::cout << "CelestialBodyRenderer: Initialized texture array with " << numLayers
              << " layers (" << width << "x" << height << ")" << std::endl;
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

bool CelestialBodyRenderer::loadTextureLayer(int layerIndex, const std::string& filepath,
                                              const PlanetTextureConfig& config,
                                              bool forceProcedural)
{
    // Try loading from file first (unless forceProcedural is set)
    if (!forceProcedural && !filepath.empty()) {
        if (std::ifstream(filepath).good()) {
            int layer = m_textureArray.addLayerFromFile(filepath);
            if (layer >= 0) {
                std::cout << "CelestialBodyRenderer: Layer " << layerIndex
                          << " loaded from file: " << filepath << std::endl;
                return true;
            }
            std::cerr << "CelestialBodyRenderer: Failed to load " << filepath
                      << ", falling back to procedural" << std::endl;
        }
    }

    // Fallback to procedural generation
    if (!generateProceduralTexture(layerIndex, config)) {
        std::cerr << "CelestialBodyRenderer: Failed to generate procedural texture for layer "
                  << layerIndex << std::endl;
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// PBR texture generation
// ---------------------------------------------------------------------------

namespace {

/**
 * @brief Generate a procedural metallic map from a planet config
 *
 * Metallic maps are mostly black (0) for dielectric surfaces with some
 * variation for mineral deposits, metal-rich regions, etc.
 */
std::vector<uint8_t> generateMetallicMap(const PlanetTextureConfig& config)
{
    int w = config.width;
    int h = config.height;
    std::vector<uint8_t> pixels(w * h);

    // Use a different seed offset for variation
    uint32_t seed = config.seed + 1000;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float u = static_cast<float>(x) / w;
            float v = static_cast<float>(y) / h;

            // Low-frequency noise for metallic regions
            float noise1 = 0.0f;
            float amplitude = 0.5f;
            float frequency = 2.0f;
            for (int oct = 0; oct < 3; ++oct) {
                float nx = u * frequency + seed * 0.1f;
                float ny = v * frequency + seed * 0.1f;
                // Simple hash-based noise
                float val = std::fmod(std::sin(nx * 12.9898f + ny * 78.233f + seed) * 43758.5453f, 1.0f);
                if (val < 0.0f) val += 1.0f;
                noise1 += val * amplitude;
                amplitude *= 0.5f;
                frequency *= 2.0f;
            }

            // Metallic is mostly low (dielectric) with sparse high-metallic spots
            float metallic = 0.0f;
            if (noise1 > 0.6f) {
                metallic = (noise1 - 0.6f) * 2.0f; // 0.0 to 0.8
            }

            // Gas giants and stars have higher metallic content
            if (config.type == PlanetTextureConfig::PlanetType::GAS_GIANT) {
                metallic = std::max(metallic, 0.15f);
            } else if (config.type == PlanetTextureConfig::PlanetType::LAVA_WORLD) {
                metallic = std::max(metallic, 0.3f);
            }

            pixels[y * w + x] = static_cast<uint8_t>(std::min(metallic, 1.0f) * 255.0f);
        }
    }

    return pixels;
}

/**
 * @brief Generate a procedural roughness map from a planet config
 *
 * Roughness maps control microsurface detail. Oceans are smooth (low roughness),
 * mountains are rough (high roughness), deserts are medium.
 */
std::vector<uint8_t> generateRoughnessMap(const PlanetTextureConfig& config)
{
    int w = config.width;
    int h = config.height;
    std::vector<uint8_t> pixels(w * h);

    uint32_t seed = config.seed + 2000;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float u = static_cast<float>(x) / w;
            float v = static_cast<float>(y) / h;

            // Multi-octave noise for roughness variation
            float noise = 0.0f;
            float amplitude = 0.5f;
            float frequency = 3.0f;
            for (int oct = 0; oct < 4; ++oct) {
                float nx = u * frequency + seed * 0.1f;
                float ny = v * frequency + seed * 0.1f;
                float val = std::fmod(std::sin(nx * 12.9898f + ny * 78.233f + seed) * 43758.5453f, 1.0f);
                if (val < 0.0f) val += 1.0f;
                noise += val * amplitude;
                amplitude *= 0.5f;
                frequency *= 2.0f;
            }

            // Base roughness: planets are generally rough (0.4-0.9)
            float roughness = 0.4f + noise * 0.5f;

            // Ice worlds are smoother
            if (config.type == PlanetTextureConfig::PlanetType::ICE_WORLD) {
                roughness *= 0.6f;
            }
            // Lava worlds have varying roughness
            if (config.type == PlanetTextureConfig::PlanetType::LAVA_WORLD) {
                roughness = 0.3f + noise * 0.4f;
            }

            pixels[y * w + x] = static_cast<uint8_t>(std::clamp(roughness, 0.0f, 1.0f) * 255.0f);
        }
    }

    return pixels;
}

/**
 * @brief Generate a procedural ambient occlusion map
 *
 * AO maps darken crevices and valleys. Uses noise to simulate terrain occlusion.
 */
std::vector<uint8_t> generateAOMap(const PlanetTextureConfig& config)
{
    int w = config.width;
    int h = config.height;
    std::vector<uint8_t> pixels(w * h);

    uint32_t seed = config.seed + 3000;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float u = static_cast<float>(x) / w;
            float v = static_cast<float>(y) / h;

            // Low-frequency noise for AO variation
            float noise = 0.0f;
            float amplitude = 0.5f;
            float frequency = 1.5f;
            for (int oct = 0; oct < 3; ++oct) {
                float nx = u * frequency + seed * 0.1f;
                float ny = v * frequency + seed * 0.1f;
                float val = std::fmod(std::sin(nx * 12.9898f + ny * 78.233f + seed) * 43758.5453f, 1.0f);
                if (val < 0.0f) val += 1.0f;
                noise += val * amplitude;
                amplitude *= 0.5f;
                frequency *= 2.0f;
            }

            // AO: mostly bright (1.0) with darker regions in "valleys"
            float ao = 0.7f + noise * 0.3f;

            pixels[y * w + x] = static_cast<uint8_t>(std::clamp(ao, 0.0f, 1.0f) * 255.0f);
        }
    }

    return pixels;
}

} // anonymous namespace

bool CelestialBodyRenderer::generatePBRMaps(int layerIndex, const PlanetTextureConfig& config)
{
    if (!m_textureArray.isValid()) {
        std::cerr << "CelestialBodyRenderer: Texture array not initialized for PBR maps" << std::endl;
        return false;
    }

    if (layerIndex < 0 || layerIndex + 3 >= m_textureArray.getMaxLayers()) {
        std::cerr << "CelestialBodyRenderer: Not enough layers for PBR maps at index " << layerIndex << std::endl;
        return false;
    }

    // Generate metallic map
    {
        std::vector<uint8_t> pixels = generateMetallicMap(config);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_textureArray.getId());
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                        0, 0, layerIndex,
                        config.width, config.height, 1,
                        GL_RED, GL_UNSIGNED_BYTE, pixels.data());
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    }

    // Generate roughness map
    {
        std::vector<uint8_t> pixels = generateRoughnessMap(config);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_textureArray.getId());
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                        0, 0, layerIndex + 1,
                        config.width, config.height, 1,
                        GL_RED, GL_UNSIGNED_BYTE, pixels.data());
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    }

    // Generate AO map
    {
        std::vector<uint8_t> pixels = generateAOMap(config);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_textureArray.getId());
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                        0, 0, layerIndex + 2,
                        config.width, config.height, 1,
                        GL_RED, GL_UNSIGNED_BYTE, pixels.data());
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    }

    std::cout << "[PBR] Generated metallic/roughness/AO maps at layers "
              << layerIndex << "-" << (layerIndex + 2)
              << " (" << config.width << "x" << config.height << ")" << std::endl;

    return true;
}

void CelestialBodyRenderer::setEnvironmentColor(const float color[3])
{
    std::memcpy(m_envColor, color, 3 * sizeof(float));
}

// ---------------------------------------------------------------------------
// Instance buffer update
// ---------------------------------------------------------------------------

void CelestialBodyRenderer::updateBodyBuffers()
{
    if (!m_buffersDirty || m_bodies.empty()) return;

    // Build instance data: model(16) + modelIT(9) + color(3) + emissive(3) + radius(1) + packedMRAB(4) = 36 floats
    std::vector<float> instanceData;
    instanceData.reserve(m_bodies.size() * 36);

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

        // Packed MRAB (4 floats): metallic, roughness, ao, texLayer
        instanceData.push_back(body.instance.pbrMaterial.metallic);
        instanceData.push_back(body.instance.pbrMaterial.roughness);
        instanceData.push_back(body.instance.pbrMaterial.ao);
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
    GLuint activeProgram = m_usePBR ? m_pbrShaderProgram : m_shaderProgram;
    glUseProgram(activeProgram);

    glUniformMatrix4fv(glGetUniformLocation(activeProgram, "viewMatrix"),
                       1, GL_FALSE, viewMatrix);
    glUniformMatrix4fv(glGetUniformLocation(activeProgram, "projectionMatrix"),
                       1, GL_FALSE, projectionMatrix);
    glUniform3fv(glGetUniformLocation(activeProgram, "lightPosition"), 1, m_lightPosition);
    glUniform3fv(glGetUniformLocation(activeProgram, "lightColor"), 1, m_lightColor);
    glUniform1f(glGetUniformLocation(activeProgram, "lightIntensity"), m_lightIntensity);
    glUniform3fv(glGetUniformLocation(activeProgram, "ambientColor"), 1, m_ambientColor);
    glUniform1f(glGetUniformLocation(activeProgram, "scaleFactor"), 1.0f);

    // PBR-specific uniforms
    if (m_usePBR) {
        glUniform3fv(glGetUniformLocation(activeProgram, "envColor"), 1, m_envColor);
        glUniform1i(glGetUniformLocation(activeProgram, "pbrMetallicTexLayer"), -1);
        glUniform1i(glGetUniformLocation(activeProgram, "pbrRoughnessTexLayer"), -1);
        glUniform1i(glGetUniformLocation(activeProgram, "pbrNormalTexLayer"), -1);
        glUniform1i(glGetUniformLocation(activeProgram, "pbrAOTexLayer"), -1);
    }

    // Texture array binding
    if (m_useTextureArray && m_textureArray.isValid()) {
        GLint locEnabled = glGetUniformLocation(activeProgram, "textureArrayEnabled");
        GLint locSampler = glGetUniformLocation(activeProgram, "textureArray");
        glUniform1i(locEnabled, 1);
        m_textureArray.bind(0);
        glUniform1i(locSampler, 0);
    } else {
        glUniform1i(glGetUniformLocation(activeProgram, "textureArrayEnabled"), 0);
    }

    glBindVertexArray(m_sphereVAO);
    glDisable(GL_BLEND);

    glDrawElementsInstanced(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0,
                            static_cast<GLsizei>(m_bodies.size()));
    glBindVertexArray(0);
    glUseProgram(0);
}

void CelestialBodyRenderer::renderBody(const CelestialBodyInstance& body,
                                         const float* viewMatrix,
                                         const float* projectionMatrix)
{
    if (!m_initialized) return;

    GLuint activeProgram = m_usePBR ? m_pbrShaderProgram : m_shaderProgram;
    glUseProgram(activeProgram);

    glUniformMatrix4fv(glGetUniformLocation(activeProgram, "viewMatrix"),
                       1, GL_FALSE, viewMatrix);
    glUniformMatrix4fv(glGetUniformLocation(activeProgram, "projectionMatrix"),
                       1, GL_FALSE, projectionMatrix);
    glUniform3fv(glGetUniformLocation(activeProgram, "lightPosition"), 1, m_lightPosition);
    glUniform3fv(glGetUniformLocation(activeProgram, "lightColor"), 1, m_lightColor);
    glUniform1f(glGetUniformLocation(activeProgram, "lightIntensity"), m_lightIntensity);
    glUniform3fv(glGetUniformLocation(activeProgram, "ambientColor"), 1, m_ambientColor);
    glUniform1f(glGetUniformLocation(activeProgram, "scaleFactor"), 1.0f);

    if (m_usePBR) {
        glUniform3fv(glGetUniformLocation(activeProgram, "envColor"), 1, m_envColor);
        glUniform1i(glGetUniformLocation(activeProgram, "pbrMetallicTexLayer"), -1);
        glUniform1i(glGetUniformLocation(activeProgram, "pbrRoughnessTexLayer"), -1);
        glUniform1i(glGetUniformLocation(activeProgram, "pbrNormalTexLayer"), -1);
        glUniform1i(glGetUniformLocation(activeProgram, "pbrAOTexLayer"), -1);
    }

    // Texture array binding for single body render
    if (m_useTextureArray && m_textureArray.isValid() && body.textureLayer >= 0) {
        glUniform1i(glGetUniformLocation(activeProgram, "textureArrayEnabled"), 1);
        m_textureArray.bind(0);
        glUniform1i(glGetUniformLocation(activeProgram, "textureArray"), 0);
    } else {
        glUniform1i(glGetUniformLocation(activeProgram, "textureArrayEnabled"), 0);
    }

    // Build instance data for this single body
    // Layout: model(16) + modelIT(9) + color(3) + emissive(3) + radius(1) + packedMRAB(4) = 36 floats
    float instanceData[36];
    std::memset(instanceData, 0, 36 * sizeof(float));

    // Apply coordinate scale: convert meters to viewport units
    double invScale = 1.0 / m_coordinateScale;
    float posX = static_cast<float>(body.position[0] * invScale);
    float posY = static_cast<float>(body.position[1] * invScale);
    float posZ = static_cast<float>(body.position[2] * invScale);
    
    float scale = static_cast<float>(body.radius * invScale);
    // Model matrix: translation only (shader applies aRadius), so unit scale
    instanceData[0] = 1.0f;
    instanceData[5] = 1.0f;
    instanceData[10] = 1.0f;
    instanceData[15] = 1.0f;
    instanceData[12] = posX;
    instanceData[13] = posY;
    instanceData[14] = posZ;

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

    // Packed MRAB: [32]=metallic, [33]=roughness, [34]=ao, [35]=texLayer
    instanceData[32] = body.pbrMaterial.metallic;
    instanceData[33] = body.pbrMaterial.roughness;
    instanceData[34] = body.pbrMaterial.ao;
    instanceData[35] = static_cast<float>(body.textureLayer);

    // Upload instance data to the instance VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, 36 * sizeof(float), instanceData, GL_DYNAMIC_DRAW);

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