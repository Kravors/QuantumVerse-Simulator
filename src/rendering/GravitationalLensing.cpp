/**
 * @file GravitationalLensing.cpp
 * @brief Implementation of ray-marching gravitational lensing renderer.
 */

#include "GravitationalLensing.h"

#include <cmath>
#include <algorithm>
#include <cstring>
#include <vector>
#include <array>
#include <random>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

// ============================================================================
// Shader Sources
// ============================================================================

static const char* lensingVertexSource = R"(
    #version 450 core
    layout(location = 0) in vec2 aPos;
    out vec2 vTexCoord;
    void main() {
        vTexCoord = aPos * 0.5 + 0.5;
        gl_Position = vec4(aPos, 0.0, 1.0);
    }
)";

// Fragment shader is loaded from lensing_fragment.glsl at runtime
// (MSVC cannot compile one giant embedded raw string literal).
static std::string loadLensingFragmentSource() {
    namespace fs = std::filesystem;
    auto path = fs::current_path() / "src" / "rendering" / "lensing_fragment.glsl";
    std::ifstream file(path);
    if (!file) {
        std::cerr << "[GravitationalLensing] failed to open lensing_fragment.glsl" << std::endl;
        return {};
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static const char* quadVertexSource = R"(
    #version 450 core
    layout(location = 0) in vec2 aPos;
    out vec2 vTexCoord;
    void main() {
        vTexCoord = aPos * 0.5 + 0.5;
        gl_Position = vec4(aPos, 0.0, 1.0);
    }
)";

static const char* quadFragmentSource = R"(
    #version 450 core
    in vec2 vTexCoord;
    uniform sampler2D u_texture;
    out vec4 outColor;
    void main() {
        outColor = texture(u_texture, vTexCoord);
    }
)";

// ============================================================================
// Helper Functions
// ============================================================================

static GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        // In a real implementation, we'd log this properly
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint linkProgram(GLuint vertShader, GLuint fragShader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        glDeleteProgram(program);
        return 0;
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    return program;
}

// ============================================================================
// GravitationalLensing Implementation
// ============================================================================

GravitationalLensing::GravitationalLensing(std::shared_ptr<MetricTensor> metric)
    : m_metric(std::move(metric))
{
    // Initialize camera position based on parameters
    m_params.cameraDistance = 10.0f;
    m_params.cameraTheta = 1.5708f;  // equatorial
    m_params.cameraPhi = 0.0f;
    m_params.raySteps = 256;
    m_params.maxDistance = 50.0f;
    m_params.shadowIntensity = 1.0f;
    m_params.starFieldIntensity = 1.0f;
    m_params.photonRingIntensity = 1.0f;
    m_params.accretionDiskIntensity = 0.8f;
    m_params.enablePhotonRing = true;
    m_params.enableAccretionDisk = true;
    m_params.enableStarField = true;

    m_volumetricDisk.enableVolumetricDisk = false;
    m_volumetricDisk.diskDensity = 1.0f;
    m_volumetricDisk.diskTemperature = 1.0f;
    m_volumetricDisk.diskScaleHeight = 0.1f;
    m_volumetricDisk.diskInnerRadius = 0.0f;
    m_volumetricDisk.diskOuterRadius = 20.0f;
    m_volumetricDisk.diskRaySteps = 64;
    m_volumetricDisk.diskOpacity = 1.0f;
    m_volumetricDisk.diskDopplerBoost = 1.0f;
}

GravitationalLensing::~GravitationalLensing() {
    // Only clean up GL resources if they were actually created.
    // Without initialize() being called, no GL resources exist.
    if (!m_initialized) return;

    if (m_lensingProgram) {
        glDeleteProgram(m_lensingProgram);
        m_lensingProgram = 0;
    }
    if (m_quadProgram) {
        glDeleteProgram(m_quadProgram);
        m_quadProgram = 0;
    }
    if (m_starFieldCubemap) {
        glDeleteTextures(1, &m_starFieldCubemap);
        m_starFieldCubemap = 0;
    }
    if (m_quadVao) {
        glDeleteVertexArrays(1, &m_quadVao);
        m_quadVao = 0;
    }
    if (m_quadVbo) {
        glDeleteBuffers(1, &m_quadVbo);
        m_quadVbo = 0;
    }
}

void GravitationalLensing::initialize(int width, int height) {
    if (m_initialized) {
        resize(width, height);
        return;
    }

    m_width = width;
    m_height = height;

    // Compile shaders
    compileLensingShader();
    compileQuadShader();

    // Create star field
    generateStarField(m_starSeed, m_starCount);

    // Create fullscreen quad
    createFullscreenQuad();

    m_initialized = true;
}

void GravitationalLensing::resize(int width, int height) {
    m_width = width;
    m_height = height;
}

void GravitationalLensing::render(const float* /*viewMatrix*/, const float* /*projectionMatrix*/) {
    if (!m_initialized || !m_enabled || !m_lensingProgram) {
        return;
    }

    // Save GL state
    GLint prevProgram, prevVao;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);

    // Use lensing shader
    glUseProgram(m_lensingProgram);

    // Update uniforms
    updateUniforms();

    // Bind star field cubemap
    if (m_starFieldCubemap) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_starFieldCubemap);
        setUniformInt("u_starField", 0);
    }

    // Draw fullscreen quad
    glBindVertexArray(m_quadVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    // Restore GL state
    glUseProgram(prevProgram);
    glBindVertexArray(prevVao);
}

std::vector<uint8_t> GravitationalLensing::renderOffscreen(int width, int height) {
    if (!m_initialized || !m_lensingProgram) {
        initialize(width, height);
        if (!m_lensingProgram) {
            return std::vector<uint8_t>(width * height * 3, 0);
        }
    }
    resize(width, height);
    m_width = width;
    m_height = height;

    GLuint fbo = 0;
    GLuint colorTex = 0;
    GLuint depthRbo = 0;

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &colorTex);
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);

    glGenRenderbuffers(1, &depthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        if (colorTex) glDeleteTextures(1, &colorTex);
        if (depthRbo) glDeleteRenderbuffers(1, &depthRbo);
        if (fbo) glDeleteFramebuffers(1, &fbo);
        return std::vector<uint8_t>(width * height * 3, 0);
    }

    GLint prevProgram, prevVao, prevViewport[4];
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    glViewport(0, 0, width, height);
    glUseProgram(m_lensingProgram);
    updateUniforms();

    if (m_starFieldCubemap) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_starFieldCubemap);
        glUniform1i(glGetUniformLocation(m_lensingProgram, "u_starField"), 0);
    }

    glBindVertexArray(m_quadVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    std::vector<uint8_t> pixels(width * height * 3);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glUseProgram(prevProgram);
    glBindVertexArray(prevVao);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (colorTex) glDeleteTextures(1, &colorTex);
    if (depthRbo) glDeleteRenderbuffers(1, &depthRbo);
    if (fbo) glDeleteFramebuffers(1, &fbo);

    return pixels;
}

void GravitationalLensing::setParams(const LensingParams& params) {
    m_params = params;
}

void GravitationalLensing::setMetric(std::shared_ptr<MetricTensor> metric) {
    m_metric = std::move(metric);
}

void GravitationalLensing::setVolumetricDiskParams(const VolumetricDiskParams& params) {
    m_volumetricDisk = params;
}

void GravitationalLensing::generateStarField(uint32_t seed, int starCount) {
    m_starSeed = seed;
    m_starCount = starCount;
    if (m_initialized) {
        createStarFieldTexture();
    }
}

void GravitationalLensing::setBackgroundTexture(GLuint textureId) {
    if (m_starFieldCubemap) {
        glDeleteTextures(1, &m_starFieldCubemap);
    }
    m_starFieldCubemap = textureId;
}

double GravitationalLensing::computePhotonSphereRadius() const {
    // For Schwarzschild: r_ph = 3M
    // For Kerr: depends on spin and orbit direction
    if (m_params.spin < 0.001) {
        return 3.0 * m_params.mass;
    }
    // Approximate for Kerr (prograde)
    double a = m_params.spin * m_params.mass;
    double M = m_params.mass;
    return 2.0 * M * (1.0 + cos(2.0 / 3.0 * acos(-a / M)));
}

double GravitationalLensing::computeISCO() const {
    // For Schwarzschild: r_isco = 6M
    // For Kerr: depends on spin
    double M = m_params.mass;
    double a = m_params.spin * M;

    if (std::abs(a) < 0.001) {
        return 6.0 * M;
    }

    // Bardeen et al. 1972 formula
    double aM = a / M;
    double z1 = 1.0 + pow(1.0 - aM * aM, 1.0 / 3.0) * (pow(1.0 + aM, 1.0 / 3.0) + pow(1.0 - aM, 1.0 / 3.0));
    double z2 = sqrt(3.0 * aM * aM + z1 * z1);
    double r_isco = M * (3.0 + z2 - sqrt((3.0 - z1) * (3.0 + z1 + 2.0 * z2)));

    return r_isco;
}

/**
 * @brief Dimensionless ISCO radius r/M for a given dimensionless spin.
 *
 * Pure function -- mirrors computeISCO_GLSL() bit-for-bit so the CPU
 * reference and the GPU agree.  Positive spin = prograde (disk co-rotates),
 * which shrinks the ISCO; negative spin = retrograde, which expands it.
 * Schwarzschild limit (a=0) is exactly 6; extremal prograde (a->1) -> 1.
 *
 * @param spin Dimensionless spin a/M in (-1, 1).
 * @return ISCO radius in units of M.
 */
double GravitationalLensing::computeISCORadius(double spin) {
    double a = std::abs(spin);
    if (a < 0.001) return 6.0;

    double a2 = a * a;
    // cbrt(1-a^2): guarded explicitly because pow(0, 1/3) can return NaN on
    // some platforms at a->1 even though a is bounded below 1.
    double cbrt_1ma2 = (a2 >= 1.0) ? 0.0 : std::pow(1.0 - a2, 1.0 / 3.0);
    double cbrt1pa = std::pow(1.0 + a, 1.0 / 3.0);
    double cbrt1ma = std::pow(1.0 - a, 1.0 / 3.0);

    double z1 = 1.0 + cbrt_1ma2 * (cbrt1pa + cbrt1ma);
    double z2 = std::sqrt(3.0 * a2 + z1 * z1);

    double inner = std::sqrt(std::max((3.0 - z1) * (3.0 + z1 + 2.0 * z2), 0.0));
    double signFactor = (spin >= 0.0) ? 1.0 : -1.0;

    return 3.0 + z2 - signFactor * inner;
}

void GravitationalLensing::compileLensingShader() {
    GLuint vertShader = compileShader(GL_VERTEX_SHADER, lensingVertexSource);
    std::string fragSource = loadLensingFragmentSource();
    if (fragSource.empty()) return;
    GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragSource.c_str());

    if (vertShader && fragShader) {
        m_lensingProgram = linkProgram(vertShader, fragShader);
    }
}

void GravitationalLensing::compileQuadShader() {
    GLuint vertShader = compileShader(GL_VERTEX_SHADER, quadVertexSource);
    GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, quadFragmentSource);

    if (vertShader && fragShader) {
        m_quadProgram = linkProgram(vertShader, fragShader);
    }
}

void GravitationalLensing::createStarFieldTexture() {
    // Create a cube map texture with procedural star field
    const int faceSize = 512;

    if (m_starFieldCubemap) {
        glDeleteTextures(1, &m_starFieldCubemap);
    }

    glGenTextures(1, &m_starFieldCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_starFieldCubemap);

    // Generate star positions
    std::mt19937 rng(m_starSeed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> brightnessDist(0.5f, 1.0f);
    std::uniform_int_distribution<int> colorDist(0, 2);

    // Star data: position (x,y,z) on unit sphere + brightness + color type
    struct Star {
        float x, y, z;
        float brightness;
        int colorType;
    };
    std::vector<Star> stars;
    stars.reserve(m_starCount);

    for (int i = 0; i < m_starCount; ++i) {
        // Random direction on sphere
        float x = dist(rng);
        float y = dist(rng);
        float z = dist(rng);
        float len = sqrt(x * x + y * y + z * z);
        if (len < 0.001f) continue;
        x /= len; y /= len; z /= len;

        stars.push_back({x, y, z, brightnessDist(rng), colorDist(rng)});
    }

    // Generate each face
    std::vector<uint8_t> faceData(faceSize * faceSize * 3, 0);

    for (int face = 0; face < 6; ++face) {
        // Clear face
        std::fill(faceData.begin(), faceData.end(), static_cast<uint8_t>(0));

        // For each pixel on this face, compute direction and check for stars
        for (int py = 0; py < faceSize; ++py) {
            for (int px = 0; px < faceSize; ++px) {
                // Convert pixel to direction
                float u = (px + 0.5f) / faceSize * 2.0f - 1.0f;
                float v = (py + 0.5f) / faceSize * 2.0f - 1.0f;

                float x, y, z;
                switch (face) {
                    case 0: x = 1.0f; y = -v; z = -u; break;  // +X
                    case 1: x = -1.0f; y = -v; z = u; break;  // -X
                    case 2: x = u; y = 1.0f; z = v; break;    // +Y
                    case 3: x = u; y = -1.0f; z = -v; break;  // -Y
                    case 4: x = u; y = -v; z = 1.0f; break;   // +Z
                    case 5: x = -u; y = -v; z = -1.0f; break; // -Z
                    default: x = 0; y = 0; z = 1; break;
                }

                float len = sqrt(x * x + y * y + z * z);
                x /= len; y /= len; z /= len;

                // Check each star
                float r = 0, g = 0, b = 0;
                for (const auto& star : stars) {
                    float dot = x * star.x + y * star.y + z * star.z;
                    float angle = acos(std::clamp(dot, -1.0f, 1.0f));

                    // Star apparent size
                    float starRadius = 0.005f;
                    if (angle < starRadius) {
                        float falloff = 1.0f - angle / starRadius;
                        falloff = falloff * falloff;

                        // Star color
                        switch (star.colorType) {
                            case 0: // Blue-white
                                r += 0.8f * falloff * star.brightness;
                                g += 0.9f * falloff * star.brightness;
                                b += 1.0f * falloff * star.brightness;
                                break;
                            case 1: // White
                                r += 1.0f * falloff * star.brightness;
                                g += 1.0f * falloff * star.brightness;
                                b += 0.95f * falloff * star.brightness;
                                break;
                            case 2: // Yellow-orange
                                r += 1.0f * falloff * star.brightness;
                                g += 0.9f * falloff * star.brightness;
                                b += 0.7f * falloff * star.brightness;
                                break;
                        }
                    }
                }

                // Subtle nebula background
                float nebula = (sinf(x * 3.0f + y * 5.0f) * cosf(z * 7.0f) * 0.5f + 0.5f) * 2.0f;
                r += 0.5f + nebula * 0.5f;
                g += 0.3f + nebula * 0.3f;
                b += 1.0f + nebula * 2.0f;

                // Clamp and write
                int idx = (py * faceSize + px) * 3;
                faceData[idx + 0] = static_cast<uint8_t>(std::min(r * 255.0f, 255.0f));
                faceData[idx + 1] = static_cast<uint8_t>(std::min(g * 255.0f, 255.0f));
                faceData[idx + 2] = static_cast<uint8_t>(std::min(b * 255.0f, 255.0f));
            }
        }

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB8,
                     faceSize, faceSize, 0, GL_RGB, GL_UNSIGNED_BYTE, faceData.data());
    }

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void GravitationalLensing::createFullscreenQuad() {
    // Simple fullscreen quad (two triangles)
    float quadVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f
    };

    glGenVertexArrays(1, &m_quadVao);
    glGenBuffers(1, &m_quadVbo);

    glBindVertexArray(m_quadVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void GravitationalLensing::updateUniforms() {
    if (!m_lensingProgram) return;

    // Black hole parameters
    setUniformFloat("u_mass", m_params.mass);
    setUniformFloat("u_spin", m_params.spin);
    setUniformInt("u_raySteps", m_params.raySteps);
    setUniformFloat("u_maxDistance", m_params.maxDistance);
    setUniformFloat("u_shadowIntensity", m_params.shadowIntensity);
    setUniformFloat("u_starFieldIntensity", m_params.starFieldIntensity);
    setUniformFloat("u_photonRingIntensity", m_params.photonRingIntensity);
    setUniformFloat("u_accretionDiskIntensity", m_params.accretionDiskIntensity);
    glUniform1i(glGetUniformLocation(m_lensingProgram, "u_enablePhotonRing"), m_params.enablePhotonRing ? 1 : 0);
    glUniform1i(glGetUniformLocation(m_lensingProgram, "u_enableAccretionDisk"), m_params.enableAccretionDisk ? 1 : 0);
    glUniform1i(glGetUniformLocation(m_lensingProgram, "u_enableStarField"), m_params.enableStarField ? 1 : 0);

    // Volumetric accretion disk uniforms
    glUniform1i(glGetUniformLocation(m_lensingProgram, "u_volumetricDiskEnable"), m_volumetricDisk.enableVolumetricDisk ? 1 : 0);
    setUniformFloat("u_volumetricDiskDensity", m_volumetricDisk.diskDensity);
    setUniformFloat("u_volumetricDiskTemperature", m_volumetricDisk.diskTemperature);
    setUniformFloat("u_volumetricDiskScaleHeight", m_volumetricDisk.diskScaleHeight);
    setUniformFloat("u_volumetricDiskInner", m_volumetricDisk.diskInnerRadius);
    setUniformFloat("u_volumetricDiskOuter", m_volumetricDisk.diskOuterRadius);
    setUniformInt("u_volumetricDiskSteps", m_volumetricDisk.diskRaySteps);
    setUniformFloat("u_volumetricDiskOpacity", m_volumetricDisk.diskOpacity);
    setUniformFloat("u_volumetricDiskDopplerBoost", m_volumetricDisk.diskDopplerBoost);
    setUniformFloat("u_volumetricDiskInnerEdgeFade", m_volumetricDisk.diskInnerEdgeFade);

    // Camera position from spherical coordinates
    float camX = m_params.cameraDistance * sin(m_params.cameraTheta) * cos(m_params.cameraPhi);
    float camY = m_params.cameraDistance * cos(m_params.cameraTheta);
    float camZ = m_params.cameraDistance * sin(m_params.cameraTheta) * sin(m_params.cameraPhi);

    setUniformVec3("u_cameraPos", camX, camY, camZ);

    // Camera direction (looking at origin)
    float dirLen = sqrt(camX * camX + camY * camY + camZ * camZ);
    if (dirLen > 0.001f) {
        setUniformVec3("u_cameraDir", -camX / dirLen, -camY / dirLen, -camZ / dirLen);
    } else {
        setUniformVec3("u_cameraDir", 0.0f, 0.0f, -1.0f);
    }

    // Camera up vector
    float upX = 0.0f, upY = 1.0f, upZ = 0.0f;
    // Adjust up vector if camera is near poles
    if (abs(camY / dirLen) > 0.99f) {
        upX = 0.0f; upY = 0.0f; upZ = 1.0f;
    }
    setUniformVec3("u_cameraUp", upX, upY, upZ);

    // FOV
    setUniformFloat("u_fov", 45.0f * 3.14159265f / 180.0f);
}

void GravitationalLensing::setUniformFloat(const char* name, float value) {
    glUniform1f(glGetUniformLocation(m_lensingProgram, name), value);
}

void GravitationalLensing::setUniformInt(const char* name, int value) {
    glUniform1i(glGetUniformLocation(m_lensingProgram, name), value);
}

void GravitationalLensing::setUniformVec2(const char* name, float x, float y) {
    glUniform2f(glGetUniformLocation(m_lensingProgram, name), x, y);
}

void GravitationalLensing::setUniformVec3(const char* name, float x, float y, float z) {
    glUniform3f(glGetUniformLocation(m_lensingProgram, name), x, y, z);
}

void GravitationalLensing::setUniformMat4(const char* name, const float* matrix) {
   glUniformMatrix4fv(glGetUniformLocation(m_lensingProgram, name), 1, GL_FALSE, matrix);
}

// ============================================================================
// Volumetric disk CPU reference (headless validation)
// Mirrors volumetricDiskEmission() in the GLSL source: same density, temperature,
// blackbody, optical-depth, and Doppler-beaming model, integrated along a
// straight ray in geometric units (c = 1).
// ============================================================================

GravitationalLensing::MarchResult GravitationalLensing::marchFlatSlab(double tauTotal, double j, int steps) {
    MarchResult result;
    if (steps <= 0 || tauTotal <= 0.0) {
        result.tau = tauTotal;
        return result;
    }

    double ds = tauTotal / (steps * j);  // Path length for optical depth normalization
    if (ds <= 0.0) {
        result.tau = tauTotal;
        return result;
    }

    result.emission = 0.0;
    double tau = 0.0;

    for (int i = 0; i < steps; ++i) {
        double dTau = j * ds;
        double T_before = std::exp(-tau);
        double aStep = 1.0 - std::exp(-dTau);
        double avg = (dTau > 1e-3) ? (aStep / dTau) : (1.0 - 0.5 * dTau);
        result.emission += T_before * j * ds * avg;
        tau += dTau;
    }

    result.tau = tau;
    return result;
}

std::array<float, 3> GravitationalLensing::blackbodyColor(float T) {
    float t = std::max(T, 0.01f);
    std::array<float, 3> color;
    color[0] = 1.0f / (1.0f + std::exp(2.0f * (0.8f - t)));
    color[1] = 1.0f / (1.0f + std::exp(3.0f * (1.0f - t)));
    color[2] = 1.0f / (1.0f + std::exp(4.0f * (1.3f - t)));
    float sum = color[0] + color[1] + color[2];
    if (sum > 0.0f) {
        color[0] /= sum;
        color[1] /= sum;
        color[2] /= sum;
    }
    return color;
}

std::array<float, 3> GravitationalLensing::computeVolumetricDiskEmissivity(
    const std::array<float, 3>& pos,
    const VolumetricDiskParams& params,
    float mass,
    float spin)
{
    float rPlane = std::sqrt(pos[0] * pos[0] + pos[2] * pos[2]);
    float rInner = params.diskInnerRadius;
    if (rInner <= 0.0f) {
        rInner = static_cast<float>(computeISCORadius(spin)) * mass;
    }
    if (rPlane < rInner || rPlane > params.diskOuterRadius) return {0.0f, 0.0f, 0.0f};

    float innerFade = (params.diskInnerEdgeFade > 0.0f)
        ? std::clamp((rPlane - rInner) / params.diskInnerEdgeFade, 0.0f, 1.0f)
        : (rPlane >= rInner ? 1.0f : 0.0f);

    float H = params.diskScaleHeight * rPlane;
    float zNorm = (H > 0.0001f) ? pos[1] / H : 0.0f;
    float rho = params.diskDensity * std::pow(rInner / rPlane, 1.5f) * std::exp(-zNorm * zNorm);
    rho *= innerFade;

    float T = params.diskTemperature * std::pow(rInner / rPlane, 0.75f);
    // Doppler-shifted effective temperature: T_obs = T_emit * delta.
    // The delta^4 intensity boost moves from beaming into j below; beaming
    // carries only the user gain.
    float vPhi = std::sqrt(mass / rPlane);
    float sinPhi = pos[2] / rPlane;
    float velX = -sinPhi * vPhi;
    float vDotN = velX;
    float delta = 1.0f / std::max(1.0f - vDotN, 0.01f);
    float T_eff = T * delta;
    float j = params.diskOpacity * rho * T_eff * T_eff * T_eff * T_eff;
    std::array<float, 3> bbColor = blackbodyColor(T_eff);
    float beaming = params.diskDopplerBoost;

    std::array<float, 3> result;
    result[0] = bbColor[0] * j * beaming;
    result[1] = bbColor[1] * j * beaming;
    result[2] = bbColor[2] * j * beaming;
    return result;
}

float GravitationalLensing::computeDiskLuminosity(const VolumetricDiskParams& params, float mass,
                                                  float spin)
{
    float rInner = params.diskInnerRadius;
    if (rInner <= 0.0f) {
        rInner = static_cast<float>(computeISCORadius(spin)) * mass;
    }
    float rOuter = params.diskOuterRadius;
    if (rOuter <= rInner) return 0.0f;

    const int NR = 64;
    const int NPHI = 64;
    const int NZ = 24;
    float lum = 0.0f;
    float dr = (rOuter - rInner) / float(NR);
    float dphi = 2.0f * 3.14159265358979323846f / float(NPHI);
    float Hmax = params.diskScaleHeight * rOuter;

    for (int ir = 0; ir < NR; ++ir) {
        float r = rInner + (ir + 0.5f) * dr;
        for (int ip = 0; ip < NPHI; ++ip) {
            float phi = ip * dphi;
            float x = r * std::cos(phi);
            float z = r * std::sin(phi);
            for (int iz = 0; iz < NZ; ++iz) {
                float y = -Hmax + (iz + 0.5f) * (2.0f * Hmax) / float(NZ);
                std::array<float, 3> pos = {x, y, z};
                std::array<float, 3> emissivity = computeVolumetricDiskEmissivity(pos, params, mass, spin);
                float dV = dr * dphi * r * (2.0f * Hmax) / float(NZ);
                lum += (emissivity[0] + emissivity[1] + emissivity[2]) * dV;
            }
        }
    }
    return lum;
}

} // namespace quantumverse
