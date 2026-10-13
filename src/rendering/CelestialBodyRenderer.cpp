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
    if (m_annulusVAO) glDeleteVertexArrays(1, &m_annulusVAO);
    if (m_annulusVBO) glDeleteBuffers(1, &m_annulusVBO);
    if (m_annulusEBO) glDeleteBuffers(1, &m_annulusEBO);
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
    // Layout per instance (36 floats = 144 bytes):
    //   [0..15]   modelMatrix (mat4, column-major)
    //   [16..24]  modelMatrixIT (mat3, column-major)
    //   [25..27]  color (vec3)
    //   [28..31]  emissiveAO (vec4: xyz=emissive, w=ao)
    //   [32..35]  packedRadiusMR (vec4: x=radius, y=metallic, z=roughness, w=texLayer)
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

    // EmissiveAO (location 12): xyz=emissive, w=ao
    glEnableVertexAttribArray(12);
    glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE,
                          kInstanceStride,
                          reinterpret_cast<void*>(static_cast<uintptr_t>(28 * sizeof(float))));
    glVertexAttribDivisor(12, 1);

    // Packed radius/metallic/roughness/texLayer (location 13)
    glEnableVertexAttribArray(13);
    glVertexAttribPointer(13, 4, GL_FLOAT, GL_FALSE,
                          kInstanceStride,
                          reinterpret_cast<void*>(static_cast<uintptr_t>(32 * sizeof(float))));
    glVertexAttribDivisor(13, 1);
    // Locations 14+ are no longer used

    glBindVertexArray(0);
}

void CelestialBodyRenderer::generateAnnulusMesh(float innerR, float outerR)
{
    m_annulusVertices.clear();
    m_annulusIndices.clear();

    const int radialSegments = 8;
    const int angularSegments = 32;

    // Vertices lie in the XZ plane (the disk's equatorial plane).
    for (int i = 0; i <= radialSegments; ++i) {
        const float r = innerR + (outerR - innerR) * static_cast<float>(i) /
                        static_cast<float>(radialSegments);
        for (int j = 0; j <= angularSegments; ++j) {
            const float theta = 2.0f * static_cast<float>(M_PI) *
                                static_cast<float>(j) /
                                static_cast<float>(angularSegments);

            SphereVertex v;
            v.position[0] = r * std::cos(theta);
            v.position[1] = 0.0f;
            v.position[2] = r * std::sin(theta);
            // Normal points up (+Y): the disk is lit from above.
            v.normal[0] = 0.0f;
            v.normal[1] = 1.0f;
            v.normal[2] = 0.0f;
            // u wraps around the disk, v runs from inner to outer radius.
            v.texCoord[0] = static_cast<float>(j) / angularSegments;
            v.texCoord[1] = (r - innerR) / (outerR - innerR);
            // Tangent follows increasing theta.
            v.tangent[0] = -std::sin(theta);
            v.tangent[1] = 0.0f;
            v.tangent[2] = std::cos(theta);

            m_annulusVertices.push_back(v);
        }
    }

    // Two triangles per quad of the (radialSegments x angularSegments) grid.
    //
    // Winding is chosen so the geometric normal (right-hand rule on the
    // emitted index order) is +Y, matching the declared vertex normal. The
    // original order (first, second, first+1) produced a -Y geometric
    // normal against a +Y vertex normal. With face culling disabled that
    // mismatch is invisible - the disc renders from both sides - but any
    // glEnable(GL_CULL_FACE) would then cull the disc when viewed from
    // above, which is exactly the Doppler capture pose (el=+30).
    const int ring = angularSegments + 1;
    for (int i = 0; i < radialSegments; ++i) {
        for (int j = 0; j < angularSegments; ++j) {
            const int first = i * ring + j;
            const int second = first + ring;

            m_annulusIndices.push_back(first);
            m_annulusIndices.push_back(first + 1);
            m_annulusIndices.push_back(second);

            m_annulusIndices.push_back(second);
            m_annulusIndices.push_back(first + 1);
            m_annulusIndices.push_back(second + 1);
        }
    }

    m_annulusIndexCount = static_cast<unsigned int>(m_annulusIndices.size());
}

void CelestialBodyRenderer::generateAnnulusVAO()
{
    // Clean up old resources
    if (m_annulusVAO) glDeleteVertexArrays(1, &m_annulusVAO);
    if (m_annulusVBO) glDeleteBuffers(1, &m_annulusVBO);
    if (m_annulusEBO) glDeleteBuffers(1, &m_annulusEBO);

    glGenVertexArrays(1, &m_annulusVAO);
    glGenBuffers(1, &m_annulusVBO);
    glGenBuffers(1, &m_annulusEBO);

    glBindVertexArray(m_annulusVAO);

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, m_annulusVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 m_annulusVertices.size() * sizeof(SphereVertex),
                 m_annulusVertices.data(),
                 GL_STATIC_DRAW);

    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_annulusEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 m_annulusIndices.size() * sizeof(unsigned int),
                 m_annulusIndices.data(),
                 GL_STATIC_DRAW);

    // Per-vertex attributes (identical layout to the sphere VAO)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SphereVertex),
                          (void*)offsetof(SphereVertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SphereVertex),
                          (void*)offsetof(SphereVertex, normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(SphereVertex),
                          (void*)offsetof(SphereVertex, texCoord));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(SphereVertex),
                          (void*)offsetof(SphereVertex, tangent));
    glEnableVertexAttribArray(3);

    // Instance attributes share the sphere's instance VBO layout (36 floats).
    const GLsizeiptr kInstanceStride = 36 * sizeof(float);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);

    // modelMatrix columns (locations 4-7), one vec4 per column
    for (int col = 0; col < 4; ++col) {
        glEnableVertexAttribArray(4 + col);
        glVertexAttribPointer(4 + col, 4, GL_FLOAT, GL_FALSE,
                              kInstanceStride,
                              reinterpret_cast<void*>(
                                  static_cast<uintptr_t>(col * 4 * sizeof(float))));
        glVertexAttribDivisor(4 + col, 1);
    }

    // modelMatrixIT columns (locations 8-10), one vec3 per column
    for (int col = 0; col < 3; ++col) {
        glEnableVertexAttribArray(8 + col);
        glVertexAttribPointer(8 + col, 3, GL_FLOAT, GL_FALSE,
                              kInstanceStride,
                              reinterpret_cast<void*>(
                                  static_cast<uintptr_t>(16 * sizeof(float) +
                                                         col * 3 * sizeof(float))));
        glVertexAttribDivisor(8 + col, 1);
    }

    // Color (location 11)
    glEnableVertexAttribArray(11);
    glVertexAttribPointer(11, 3, GL_FLOAT, GL_FALSE,
                          kInstanceStride,
                          reinterpret_cast<void*>(
                              static_cast<uintptr_t>(25 * sizeof(float))));
    glVertexAttribDivisor(11, 1);

    // EmissiveAO (location 12): xyz = emissive, w = ao
    glEnableVertexAttribArray(12);
    glVertexAttribPointer(12, 4, GL_FLOAT, GL_FALSE,
                          kInstanceStride,
                          reinterpret_cast<void*>(
                              static_cast<uintptr_t>(28 * sizeof(float))));
    glVertexAttribDivisor(12, 1);

    // Packed radius/metallic/roughness/texLayer (location 13)
    glEnableVertexAttribArray(13);
    glVertexAttribPointer(13, 4, GL_FLOAT, GL_FALSE,
                          kInstanceStride,
                          reinterpret_cast<void*>(
                              static_cast<uintptr_t>(32 * sizeof(float))));
    glVertexAttribDivisor(13, 1);

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
layout(location = 12) in vec4 aEmissiveAO;      // xyz = emissive, w = ao
layout(location = 13) in vec4 aPackedRadiusMR;   // x = radius, y = metallic, z = roughness, w = texLayer

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform float scaleFactor;
uniform int textureArrayEnabled;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vTexCoord;
out vec3 vColor;
out vec3 vEmissive;
out vec3 vViewDir;
flat out float vTexLayer;
flat out float vMetallic;
flat out float vRoughness;
flat out float vAo;

void main() {
    vec4 worldPos4 = aModelMatrix * vec4(aPos * aPackedRadiusMR.x, 1.0);
    mat3 modelIT = mat3(aModelMatrixIT_col0, aModelMatrixIT_col1, aModelMatrixIT_col2);
    vWorldPos = worldPos4.xyz;
    vNormal = normalize(modelIT * aNormal);
    vTexCoord = aTexCoord;
    vColor = aColor;
    vEmissive = aEmissiveAO.xyz;
    vMetallic = aPackedRadiusMR.y;
    vRoughness = aPackedRadiusMR.z;
    vAo = aEmissiveAO.w;
    vTexLayer = aPackedRadiusMR.w;

    vec4 viewPos = viewMatrix * worldPos4;
    vViewDir = -viewPos.xyz;

    gl_Position = projectionMatrix * viewPos;
}
)";

const char* celestialFragSrc = R"(
#version 450 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec3 vColor;
in vec3 vEmissive;
in vec3 vViewDir;
flat in float vTexLayer;
flat in float vMetallic;
flat in float vRoughness;
flat in float vAo;

uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform float lightIntensity;
uniform vec3 ambientColor;
uniform float time;
uniform int textureArrayEnabled;
uniform sampler2DArray textureArray;
uniform int debugMode;
// Disk rotation axis, pinned at file scope so the fragment stage has a single
// point of definition for the Doppler sign convention. The accretion disk lies
// in the XZ plane (normal +Y); los = dot(v_dir, view_dir) is defined relative
// to this axis, so which limb is "approaching" is only meaningful once this is
// fixed. v_dir at a fragment is normalize(cross(OMEGA_AXIS, radial)).
const vec3 OMEGA_AXIS = vec3(0.0, 1.0, 0.0);

// Set to 1 only while drawing the black hole annulus: makes the disk render
// as flat magenta so its on-screen extent can be verified by pixel count.
uniform int debugAnnulus;
// Second annulus debug view: encodes the fragment's world position relative to
// the black hole in RGB. Scale is (+-2)/4 world units per channel, i.e. 0.016
// units per count, which resolves a 0.05-unit separation - fine enough to
// identify a specific (r, phi) disk fragment across two captures. The radius
// encoding that preceded it (r/4.0 in red, the [0.72, 1.80] geometry probe)
// was removed because this view carries the same information: length(rel.xz)
// recovers the radius exactly, so a separate radius mode had no remaining
// caller after the one-time geometry check at commit 15ce33f. Alpha is
// exactly 1.0 so nothing blends with the skybox behind the disk.
uniform int debugAnnulusPosition;
// Radial/positional origin for the encodings. Must match the annulus instance
// translation (m_bhCenterWorld), not the world origin, so a non-zero config
// position still decodes relative to the black hole.
uniform vec3 bhCenterWorld;
// Fourth annulus debug view: encodes the sign of the line-of-sight term
// los = dot(v_dir, view_dir) at each disk fragment. v_dir is the disk rotation
// direction, view_dir points from the fragment to the camera, so the sign is
// purely a function of the camera's world position - which is why
// cameraPosWorld must be the live camera eye, not a fixed axis. Red channel
// carries max(los, 0) (approaching limb), blue carries max(-los, 0)
// (receding limb), green is 0, alpha exactly 1.0 so nothing blends with the
// skybox. Los magnitude is deliberately NOT encoded: this view answers only
// the sign question that gates the step-4 beaming ratio.
uniform int debugAnnulusDoppler;
// Camera eye in world space (extracted from the view matrix per frame). The
// view_dir below must point at the actual camera; using a fixed axis mirrors
// the beaming between az=0 and az=180 and hides the sign flip.
uniform vec3 cameraPosWorld;
// Set to 1 only while drawing the black hole annulus in NORMAL debug mode:
// gates the Doppler beaming term so bodies keep their unshifted shading.
uniform int dopplerBeaming;

// Visual mass of the black hole in annulus radius units. The annulus spans
// [0.72, 1.80] world units, and 0.72 = 3 * r_s (ISCO) fixes r_s = 0.24, so
// M = r_s / 2 = 0.12. Keplerian speed beta = sqrt(M / r) then evaluates to
// sqrt(1/6) at ISCO and to 0.301 at the r = 1.326 fragments the Doppler
// verification samples.
const float M_VISUAL = 0.12;

// Display gain applied with the beaming term below. A pure scale multiplying
// both limbs equally, so the measured limb ratio is gain-independent.
//
// Shipped default is 1.0, i.e. no compensation: the beamed approaching limb
// saturates to (255,255,255) wherever base * D^4 exceeds the display range.
// That saturation is the intended shipped look, decided and recorded here so
// it does not read as an accident. A Doppler-boosted limb has an emission
// peak wider than the display range, and clipping it is standard practice
// for relativistic disk renders (cf. M87*/Sgr A* images, whose bright limbs
// saturate the detector). If the shipped disk ever needs to communicate the
// beaming gradient across the whole limb instead of saturating, the fix is a
// tone-mapping curve on the beamed colour - NOT a smaller linear gain, which
// would scale the already-dark receding limb too and silently shrink the
// shipped headroom. A tone map is nonlinear, so it changes the limb ratios
// and needs its own verification with its own expected values; the linear
// path below is what the smoke ratio test measures.
//
// The shipped base does NOT clip on its own: a 52-pixel profile of the whole
// ring at gain 0.3 (all four debug views at az=90/el=30/dist=150) fits one
// shared base of 246.5/255 median with a 239-253 spread (the per-fragment
// shading variation) - see the profile check in this commit's history. It is
// base * D^4 that evaluates to ~810 (247 * 3.28 at the r = 0.770 inner
// fragment) and clips the approaching side. The 0.3 measurement value keeps
// red unclipped everywhere on the ring (max 243); blue does clip at the
// innermost fragment, which is why the smoke test samples red, not blue.
// Set via --beaming-gain: a measurement enabler, not a correction for an
// over-bright disk.
uniform float beamingDisplayGain;

// Line-of-sight term los = dot(v_dir, view_dir) for a disk fragment.
// v_dir = normalize(cross(OMEGA_AXIS, radial)) is the Keplerian rotation
// direction, radial lies in the disk plane (XZ) relative to the black hole,
// and view_dir points at the live camera eye. The DOPPLER debug view and the
// normal-path beaming term both call this so the sign convention has a
// single point of definition. Returns 0.0 at the exact center, where no
// rotation direction exists.
float diskLos(vec3 fragWorldPos)
{
    vec3 radial = vec3(fragWorldPos.x - bhCenterWorld.x, 0.0,
                       fragWorldPos.z - bhCenterWorld.z);
    if (dot(radial, radial) < 1e-12) {
        return 0.0;
    }
    vec3 vDir = normalize(cross(OMEGA_AXIS, radial));
    vec3 viewDir = normalize(cameraPosWorld - fragWorldPos);
    return dot(vDir, viewDir);
}

out vec4 outColor;

void main() {
    if (debugAnnulus == 1) { outColor = vec4(1.0, 0.0, 1.0, 1.0); return; }
    if (debugAnnulusPosition == 1) {
        vec3 rel = vWorldPos - bhCenterWorld;
        vec3 enc = (rel + vec3(2.0, 2.0, 2.0)) / vec3(4.0, 4.0, 4.0);
        outColor = vec4(enc, 1.0);
        return;
    }
    if (debugAnnulusDoppler == 1) {
        // Sign of the line-of-sight term, split into colour channels. Shares
        // diskLos() with the normal-path beaming term below, so the debug
        // view and the rendered disk can never disagree about which limb is
        // approaching. Alpha exactly 1.0, matching every other annulus debug
        // view.
        float los = diskLos(vWorldPos);
        outColor = vec4(max(los, 0.0), 0.0, max(-los, 0.0), 1.0);
        return;
    }

    vec3 N = normalize(vNormal);
    vec3 L = normalize(lightPosition - vWorldPos);
    vec3 V = normalize(vViewDir);
    vec3 H = normalize(L + V);

    vec4 texColor = vec4(1.0);
    if (textureArrayEnabled > 0 && vTexLayer >= 0.0) {
        texColor = texture(textureArray, vec3(vTexCoord, vTexLayer));
    }

    if (debugMode == 1) { outColor = vec4(1.0, 0.0, 0.0, 1.0); return; }
    if (debugMode == 2) { outColor = vec4(texColor.a); return; }
    if (debugMode == 3) { outColor = texColor; return; }

    vec3 ambient = ambientColor * vColor * texColor.rgb;
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * vColor * texColor.rgb * lightColor * lightIntensity;
    float spec = pow(max(dot(N, H), 0.0), 64.0);
    vec3 specular = spec * vec3(0.3) * lightColor * lightIntensity;
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    vec3 atmosphere = fresnel * vec3(0.2, 0.4, 0.8) * 0.5;
    vec3 emission = vEmissive * lightColor * 0.5;
    vec3 result = ambient + diffuse + specular + atmosphere + emission;
    result = pow(result, vec3(1.0 / 2.2));
    outColor = vec4(result, texColor.a);

    if (dopplerBeaming == 1) {
        // Doppler beaming for the accretion disk. beta = sqrt(M / r) is the
        // Keplerian orbital speed in geometrized visual units and D =
        // 1 / (gamma * (1 - beta * los)) is the Doppler factor: D > 1 on the
        // approaching limb (los > 0), D < 1 on the receding limb. The chosen
        // exponent is 4 - bolometric beaming via Stefan-Boltzmann
        // (T_obs = T * delta, so intensity scales as delta^4), the same
        // convention the volumetric disk uses (GravitationalLensing.cpp,
        // asserted by test_lensing.cpp "delta^4, not delta^8"). Applied to
        // the display-encoded colour so a captured 8-bit limb ratio equals
        // the bolometric ratio the verification predicts (4.75x was the D^3
        // alternative; the measured ratio below identifies the exponent).
        float rDisk = length(vWorldPos.xz - bhCenterWorld.xz);
        float beta = sqrt(M_VISUAL / max(rDisk, 4.0 * M_VISUAL));
        float gamma = inversesqrt(1.0 - beta * beta);
        float D = 1.0 / (gamma * (1.0 - beta * diskLos(vWorldPos)));
        outColor.rgb *= pow(D, 4.0) * beamingDisplayGain;
    }
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

    // Generate the black hole annulus (accretion disk) debug geometry
    if (!m_annulusVAO) {
        generateAnnulusMesh(m_annInnerR, m_annOuterR);
        generateAnnulusVAO();
        qDebug() << "[ANNULUS] generated: inner=" << m_annInnerR
                 << "outer=" << m_annOuterR
                 << "indices=" << m_annulusIndexCount;
        // The annulus instance matrix translates to m_bhCenterWorld, so this is
        // the world position the disk is actually drawn at. It matches the
        // configured black hole only because main_qml.cpp feeds it
        // config black_hole.position via setBlackHoleCenter(); if they ever
        // diverge, the disk is displaced while the magenta pixel count stays
        // unchanged (the count never inspects world coordinates).
        qDebug() << "[ANNULUS] bhCenterWorld =" << m_bhCenterWorld[0]
                 << m_bhCenterWorld[1] << m_bhCenterWorld[2]
                 << "(from config black_hole.position)";
    }

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

    // Cache uniform locations queried once at init time
    m_debugAnnulusLoc = glGetUniformLocation(m_shaderProgram, "debugAnnulus");
    m_debugAnnulusPositionLoc = glGetUniformLocation(m_shaderProgram, "debugAnnulusPosition");
    m_debugAnnulusDopplerLoc = glGetUniformLocation(m_shaderProgram, "debugAnnulusDoppler");
    m_dopplerBeamingLoc = glGetUniformLocation(m_shaderProgram, "dopplerBeaming");
    m_beamingDisplayGainLoc = glGetUniformLocation(m_shaderProgram, "beamingDisplayGain");
    m_bhCenterWorldLoc = glGetUniformLocation(m_shaderProgram, "bhCenterWorld");
    m_cameraPosWorldLoc = glGetUniformLocation(m_shaderProgram, "cameraPosWorld");

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
    if (!m_buffersDirty || (m_bodies.empty() && !m_annulusVAO)) return;

    // Build instance data: model(16) + modelIT(9) + color(3) + emissiveAO(4) + packedRadiusMR(4) = 36 floats
    std::vector<float> instanceData;
    instanceData.reserve((m_bodies.size() + 1) * 36);

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

        // EmissiveAO (4 floats): xyz=emissive, w=ao
        instanceData.push_back(body.instance.emissive[0]);
        instanceData.push_back(body.instance.emissive[1]);
        instanceData.push_back(body.instance.emissive[2]);
        instanceData.push_back(body.instance.pbrMaterial.ao);

        // Packed radius/metallic/roughness/texLayer (4 floats)
        instanceData.push_back(static_cast<float>(body.instance.radius));
        instanceData.push_back(body.instance.pbrMaterial.metallic);
        instanceData.push_back(body.instance.pbrMaterial.roughness);
        instanceData.push_back(static_cast<float>(body.instance.textureLayer));
    }

    // Annulus instance: one extra instance appended after the bodies, so it
    // can be drawn with glDrawElementsInstancedBaseInstance. The model matrix
    // is translation-only to the black hole center; texLayer = -1 keeps the
    // texture array from sampling a layer for the disk.
    if (m_annulusVAO) {
        const float modelMatrix[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            m_bhCenterWorld[0], m_bhCenterWorld[1], m_bhCenterWorld[2], 1.0f
        };
        // Translation-only model -> inverse transpose is identity.
        const float modelMatrixIT[9] = {
            1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 1.0f
        };
        const float color[3] = {1.0f, 1.0f, 1.0f};
        const float emissiveAO[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        const float packedRadiusMR[4] = {1.0f, 0.0f, 0.5f, -1.0f};

        for (int i = 0; i < 16; ++i) instanceData.push_back(modelMatrix[i]);
        for (int i = 0; i < 9; ++i) instanceData.push_back(modelMatrixIT[i]);
        for (int i = 0; i < 3; ++i) instanceData.push_back(color[i]);
        for (int i = 0; i < 4; ++i) instanceData.push_back(emissiveAO[i]);
        for (int i = 0; i < 4; ++i) instanceData.push_back(packedRadiusMR[i]);

        m_annulusInstanceBase = static_cast<unsigned int>(m_bodies.size());
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
    if (!m_initialized || (m_bodies.empty() && !m_annulusVAO)) return;

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

    // Bodies must never be tinted by the annulus debug overlay. The debug
    // modes are mutually exclusive by construction: exactly one of them is 1
    // for the frame, so the shader can never be asked to render both and the
    // early-return ordering is unambiguous.
    const int annulusMagenta = (m_annulusDebugMode == AnnulusDebugMode::MAGENTA) ? 1 : 0;
    const int annulusPosition = (m_annulusDebugMode == AnnulusDebugMode::POSITION) ? 1 : 0;
    const int annulusDoppler = (m_annulusDebugMode == AnnulusDebugMode::DOPPLER) ? 1 : 0;
    // The beaming term only reaches the shaded disk path, which only NORMAL
    // mode falls through to; the three debug modes early-return before it.
    const int annulusBeaming = (m_annulusDebugMode == AnnulusDebugMode::NORMAL) ? 1 : 0;
    glUniform1i(m_debugAnnulusLoc, 0);
    glUniform1i(m_debugAnnulusPositionLoc, 0);
    glUniform1i(m_debugAnnulusDopplerLoc, 0);
    glUniform1i(m_dopplerBeamingLoc, 0);
    // Beaming gain uploaded once per frame, before any body draw: the CLI
    // --beaming-gain override is a measurement enabler for the annulus ratio
    // test and must not leak into the rest of the scene.
    glUniform1f(m_beamingDisplayGainLoc, m_beamingDisplayGain);
    // Camera eye in world space, recovered from the column-major view matrix
    // (eye = -R^T * t). Camera4DAdapter::viewMatrix() is a lookAt matrix, so
    // its 3x3 part is a pure rotation and this inversion is exact.
    const float cameraPosWorld[3] = {
        -(viewMatrix[0] * viewMatrix[12] + viewMatrix[1] * viewMatrix[13] +
          viewMatrix[2] * viewMatrix[14]),
        -(viewMatrix[4] * viewMatrix[12] + viewMatrix[5] * viewMatrix[13] +
          viewMatrix[6] * viewMatrix[14]),
        -(viewMatrix[8] * viewMatrix[12] + viewMatrix[9] * viewMatrix[13] +
          viewMatrix[10] * viewMatrix[14])
    };
    glUniform3fv(m_bhCenterWorldLoc, 1, m_bhCenterWorld);
    glUniform3fv(m_cameraPosWorldLoc, 1, cameraPosWorld);
    glDrawElementsInstanced(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0,
                            static_cast<GLsizei>(m_bodies.size()));
    glBindVertexArray(0);

    // Draw the annulus (accretion disk) as a single instanced draw using the
    // base instance offset assigned in updateBodyBuffers().
    //
    // MAGENTA mode (debug guard, requested explicitly by the smoke suite):
    // flat magenta so the disk's on-screen extent can be verified by pixel
    // count.
    // POSITION mode: RGB encode the world position relative to the black
    // hole, so the same pixels can be sampled to confirm the geometry is at
    // the design radius range [0.72, 1.80] (length(rel.xz) recovers the
    // radius exactly; this replaced the removed r/4.0-in-red encoding).
    // DOPPLER mode: red/blue encode the sign of dot(v_dir, view_dir), so the
    // approaching limb can be told from the receding limb before any beaming
    // ratio is measured. NORMAL mode: the shaded disk renders with the
    // Doppler beaming term applied.
    if (m_annulusVAO) {
        glUniform1i(m_debugAnnulusLoc, annulusMagenta);
        glUniform1i(m_debugAnnulusPositionLoc, annulusPosition);
        glUniform1i(m_debugAnnulusDopplerLoc, annulusDoppler);
        glUniform1i(m_dopplerBeamingLoc, annulusBeaming);
        // Bodies above already uploaded m_beamingDisplayGain; bodies drawn
        // later get the same value re-uploaded when the uniforms reset.
        glBindVertexArray(m_annulusVAO);
        glDrawElementsInstancedBaseInstance(GL_TRIANGLES,
                                            static_cast<GLsizei>(m_annulusIndexCount),
                                            GL_UNSIGNED_INT,
                                            0,
                                            1,
                                            m_annulusInstanceBase);
        glBindVertexArray(0);
        glUniform1i(m_debugAnnulusLoc, 0);
        glUniform1i(m_debugAnnulusPositionLoc, 0);
        glUniform1i(m_debugAnnulusDopplerLoc, 0);
        glUniform1i(m_dopplerBeamingLoc, 0);
    }

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
    // Layout: model(16) + modelIT(9) + color(3) + emissiveAO(4) + packedRadiusMR(4) = 36 floats
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

    // EmissiveAO: [28..31] xyz=emissive, w=ao
    instanceData[28] = body.emissive[0];
    instanceData[29] = body.emissive[1];
    instanceData[30] = body.emissive[2];
    instanceData[31] = body.pbrMaterial.ao;

    // Packed radius/metallic/roughness/texLayer: [32..35]
    instanceData[32] = scale;
    instanceData[33] = body.pbrMaterial.metallic;
    instanceData[34] = body.pbrMaterial.roughness;
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
