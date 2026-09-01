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

static const char* lensingFragmentSource = R"(
    #version 450 core

    in vec2 vTexCoord;
    out vec4 outColor;

    // Black hole parameters
    uniform float u_mass;
    uniform float u_spin;
    uniform int u_raySteps;
    uniform float u_maxDistance;
    uniform float u_shadowIntensity;
    uniform float u_starFieldIntensity;
    uniform float u_photonRingIntensity;
    uniform float u_accretionDiskIntensity;
    uniform bool u_enablePhotonRing;
    uniform bool u_enableAccretionDisk;
    uniform bool u_enableStarField;

    // Camera parameters
    uniform vec3 u_cameraPos;
    uniform vec3 u_cameraDir;
    uniform vec3 u_cameraUp;
    uniform float u_fov;

    // Star field
    uniform samplerCube u_starField;

    const float PI = 3.14159265358979323846;
    const float TWO_PI = 6.28318530717958647692;

    // Compute Schwarzschild radius
    float schwarzschildRadius(float mass) {
        return 2.0 * mass;
    }

    // Kerr metric: compute horizon radius
    float kerrHorizon(float mass, float spin) {
        float M = mass;
        float a = spin * mass;
        return M + sqrt(max(M * M - a * a, 0.0));
    }

    // Kerr metric: compute ergosphere radius at given theta
    float kerrErgosphere(float mass, float spin, float theta) {
        float M = mass;
        float a = spin * mass;
        return M + sqrt(max(M * M - a * a * cos(theta) * cos(theta), 0.0));
    }

    // Procedural star field (fallback when texture not available)
    vec3 proceduralStarField(vec3 dir) {
        // Hash function for star placement
        vec3 absDir = abs(dir);
        float scale = 50.0;
        vec3 grid = floor(dir * scale);
        vec3 frac = fract(dir * scale);

        // Simple hash
        float h = sin(dot(grid, vec3(127.1, 311.7, 74.7))) * 43758.5453;
        float star = fract(h);

        // Stars appear rarely
        vec3 color = vec3(0.0);
        if (star > 0.97) {
            float brightness = (star - 0.97) / 0.03;
            // Vary star colors (blue-white to red)
            float colorType = fract(h * 3.7);
            vec3 starColor;
            if (colorType < 0.3) {
                starColor = vec3(0.8, 0.9, 1.0);  // Blue-white
            } else if (colorType < 0.7) {
                starColor = vec3(1.0, 1.0, 0.95);  // White
            } else {
                starColor = vec3(1.0, 0.9, 0.7);  // Yellow-orange
            }
            color = starColor * brightness * u_starFieldIntensity;
        }

        // Subtle nebula background
        float nebula = sin(dir.x * 3.0 + dir.y * 5.0) * cos(dir.z * 7.0) * 0.5 + 0.5;
        color += vec3(0.01, 0.005, 0.02) * nebula * u_starFieldIntensity;

        return color;
    }

    // Sample star field (texture or procedural)
    vec3 sampleStarField(vec3 dir) {
        if (u_enableStarField) {
            vec3 texColor = texture(u_starField, dir).rgb;
            if (texColor.r > 0.0 || texColor.g > 0.0 || texColor.b > 0.0) {
                return texColor * u_starFieldIntensity;
            }
            return proceduralStarField(dir) * u_starFieldIntensity;
        }
        return proceduralStarField(dir);
    }

    // Compute deflection due to gravity (weak field approximation for ray marching)
    vec3 computeDeflection(vec3 pos, float mass, float spin) {
        float r = length(pos);
        if (r < 0.001) return vec3(0.0);

        float rs = 2.0 * mass;
        vec3 rHat = pos / r;

        // Newtonian deflection (dominant term)
        float deflectionStrength = rs / (r * r);
        vec3 deflection = -rHat * deflectionStrength;

        // Kerr frame-dragging correction
        if (spin > 0.001) {
            vec3 spinAxis = vec3(0.0, 1.0, 0.0);  // Spin along y-axis
            vec3 perpComponent = cross(spinAxis, pos);
            float twistStrength = spin * rs * rs / (r * r * r);
            deflection += perpComponent * twistStrength;
        }

        return deflection;
    }

    // Ray march through curved spacetime
    vec3 rayMarch(vec3 rayOrigin, vec3 rayDir) {
        float stepSize = u_maxDistance / float(u_raySteps);
        vec3 pos = rayOrigin;
        vec3 dir = normalize(rayDir);

        float rs = schwarzschildRadius(u_mass);
        float horizon = kerrHorizon(u_mass, u_spin);
        float photonSphere = 1.5 * rs;  // Approximate for Schwarzschild

        // Adjust photon sphere for Kerr (prograde orbit is smaller)
        if (u_spin > 0.001) {
            photonSphere = mix(photonSphere, rs, u_spin * 0.5);
        }

        for (int i = 0; i < 512; i++) {
            if (i >= u_raySteps) break;

            float r = length(pos);

            // Check if ray fell into black hole
            if (r < horizon) {
                return vec3(0.0);  // Shadow
            }

            // Check if ray escaped to infinity
            if (r > u_maxDistance) {
                return sampleStarField(dir);
            }

            // Compute deflection
            vec3 deflection = computeDeflection(pos, u_mass, u_spin);

            // Adaptive step size (smaller near black hole)
            float adaptiveStep = stepSize * max(r / (3.0 * rs), 0.1);

            // Apply deflection to ray direction
            dir = normalize(dir + deflection * adaptiveStep);
            pos += dir * adaptiveStep;
        }

        // Ray didn't escape or get captured - sample at current direction
        return sampleStarField(dir) * 0.5;
    }

    // Render accretion disk
    vec3 renderAccretionDisk(vec3 rayOrigin, vec3 rayDir, vec3 hitPos) {
        if (!u_enableAccretionDisk) return vec3(0.0);

        float r = length(hitPos);
        float rs = 2.0 * u_mass;

        // Disk extends from ISCO (~6M for Schwarzschild) to ~20M
        float diskInner = 3.0 * rs;
        float diskOuter = 15.0 * u_mass;

        // Kerr ISCO is smaller for prograde orbits
        if (u_spin > 0.001) {
            diskInner = mix(diskInner, rs, u_spin * 0.7);
        }

        // Check if ray hits disk plane (y = 0)
        if (abs(rayDir.y) < 0.001) return vec3(0.0);

        float t = -rayOrigin.y / rayDir.y;
        if (t < 0.0) return vec3(0.0);

        vec3 diskHit = rayOrigin + rayDir * t;
        float diskR = length(diskHit.xz);

        if (diskR < diskInner || diskR > diskOuter) return vec3(0.0);

        // Temperature profile (hotter near center)
        float temp = pow(diskInner / diskR, 0.75);

        // Doppler beaming asymmetry for Kerr
        float asymmetry = 1.0;
        if (u_spin > 0.001) {
            float angle = atan(diskHit.z, diskHit.x);
            asymmetry = 1.0 + u_spin * 0.5 * cos(angle);
        }

        // Blackbody-ish color
        vec3 hotColor = vec3(1.0, 0.95, 0.8);
        vec3 midColor = vec3(1.0, 0.6, 0.2);
        vec3 coolColor = vec3(0.8, 0.2, 0.05);

        vec3 color;
        if (temp > 0.7) {
            color = mix(midColor, hotColor, (temp - 0.7) / 0.3);
        } else {
            color = mix(coolColor, midColor, temp / 0.7);
        }

        // Radial falloff
        float falloff = 1.0;
        if (diskR < diskInner * 1.5) {
            falloff = (diskR - diskInner) / (diskInner * 0.5);
        } else if (diskR > diskOuter * 0.8) {
            falloff = (diskOuter - diskR) / (diskOuter * 0.2);
        }

        return color * falloff * asymmetry * u_accretionDiskIntensity;
    }

    // Render photon ring glow
    vec3 renderPhotonRing(float dist) {
        if (!u_enablePhotonRing) return vec3(0.0);

        float rs = 2.0 * u_mass;
        float photonR = 1.5 * rs * sqrt(3.0);  // Apparent photon ring radius

        float ringWidth = rs * 0.3;
        float ringDist = abs(dist - photonR);

        if (ringDist > ringWidth) return vec3(0.0);

        float intensity = (1.0 - ringDist / ringWidth);
        intensity = pow(intensity, 2.0);

        return vec3(0.6, 0.7, 1.0) * intensity * u_photonRingIntensity;
    }

    void main() {
        // Reconstruct ray direction from camera
        vec2 ndc = vTexCoord * 2.0 - 1.0;
        float aspect = float(textureSize(u_starField, 0).x) / float(textureSize(u_starField, 0).y);
        float tanHalfFov = tan(u_fov * 0.5);

        vec3 rayDir = normalize(
            u_cameraDir +
            u_cameraUp * ndc.y * tanHalfFov +
            normalize(cross(u_cameraDir, u_cameraUp)) * ndc.x * tanHalfFov * aspect
        );

        // Ray march through curved spacetime
        vec3 color = rayMarch(u_cameraPos, rayDir);

        // Add photon ring glow
        float r = length(u_cameraPos);
        vec2 ndcPos = vTexCoord * 2.0 - 1.0;
        float distFromCenter = length(ndcPos) * r * 0.3;
        vec3 ringGlow = renderPhotonRing(distFromCenter);
        color += ringGlow;

        // Tone mapping
        color = color / (color + vec3(1.0));
        color = pow(color, vec3(1.0 / 2.2));

        outColor = vec4(color, 1.0);
    }
)";

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

void GravitationalLensing::setParams(const LensingParams& params) {
    m_params = params;
}

void GravitationalLensing::setMetric(std::shared_ptr<MetricTensor> metric) {
    m_metric = std::move(metric);
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

void GravitationalLensing::compileLensingShader() {
    GLuint vertShader = compileShader(GL_VERTEX_SHADER, lensingVertexSource);
    GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, lensingFragmentSource);

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
        std::fill(faceData.begin(), faceData.end(), 0);

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

} // namespace quantumverse
