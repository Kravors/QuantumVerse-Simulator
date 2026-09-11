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

    // Volumetric accretion disk parameters
    uniform bool u_volumetricDiskEnable;
    uniform float u_volumetricDiskDensity;
    uniform float u_volumetricDiskTemperature;
    uniform float u_volumetricDiskScaleHeight;
    uniform float u_volumetricDiskInner;
    uniform float u_volumetricDiskOuter;
    uniform int u_volumetricDiskSteps;
    uniform float u_volumetricDiskOpacity;
    uniform float u_volumetricDiskIntensity;
    uniform float u_volumetricDiskDopplerBoost;

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

    // Bardeen-Press-Teukolsky (1972) innermost stable circular orbit, in units of M.
    // spin is the dimensionless a/M; positive => prograde (disk co-rotates).
    // Schwarzschild limit (a=0) is exactly 6M; extremal prograde (a->1) -> M.
    float computeISCO(float spin) {
        float a = clamp(abs(spin), 0.0f, 0.9999f);
        float a2 = a * a;

        // cbrt(1-a^2): guarded explicitly because some drivers return NaN for
        // pow(0, 1/3) at a->1 even though a is clamped below 1.
        float cbrt_1ma2 = (a2 >= 1.0f) ? 0.0f : pow(1.0f - a2, 1.0f / 3.0f);
        float cbrt1pa   = pow(1.0f + a, 1.0f / 3.0f);
        float cbrt1ma   = pow(1.0f - a, 1.0f / 3.0f);

        float Z1 = 1.0f + cbrt_1ma2 * (cbrt1pa + cbrt1ma);
        float Z2 = sqrt(3.0f * a2 + Z1 * Z1);

        float inner = sqrt(max((3.0f - Z1) * (3.0f + Z1 + 2.0f * Z2), 0.0f));
        float signFactor = (spin >= 0.0f) ? 1.0f : -1.0f;

        return 3.0f + Z2 - signFactor * inner;
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

    // Volumetric accretion disk: ray-march the disk volume instead of testing a
    // single plane.  Density ~ r^(-3/2) with a Gaussian scale height, temperature
    // ~ r^(-3/4) (Keplerian), blackbody emissivity ~ T^4, optical depth
    // integration, and relativistic Doppler beaming from the azimuthal flow.
    //
    // Returns the SELF-ATTENUATED emission integral: each step's emissivity is
    // multiplied by exp(-tau_so_far), i.e. the accumulated integral of
    // j(s) * exp(-tau(s)) ds.  tau_out carries the total optical depth along the
    // ray so the caller can composite the disk against the background with the
    // correct radiative-transfer law:
    //   color = color_bg * exp(-tau) + emission
    // (additive emission alone is physically wrong: it makes an optically thick
    // disk vanish and an optically thin disk stay bright.)
    vec3 volumetricDiskEmission(vec3 rayOrigin, vec3 rayDir, float rs, float isco,
                               out float tauOut) {
        if (!u_volumetricDiskEnable) { tauOut = 0.0; return vec3(0.0); }

        float rInner = u_volumetricDiskInner;
        if (rInner <= 0.0) rInner = isco;
        float rOuter = u_volumetricDiskOuter;
        float steps = float(u_volumetricDiskSteps);
        float stepSize = u_maxDistance / steps;
        float opacity = u_volumetricDiskOpacity;
        float density = u_volumetricDiskDensity;
        float temp0 = u_volumetricDiskTemperature;
        float H0 = u_volumetricDiskScaleHeight;

        vec3 pos = rayOrigin;
        vec3 dir = normalize(rayDir);
        vec3 emissivity = vec3(0.0);
        float tau = 0.0;

        for (int i = 0; i < 256; i++) {
            if (i >= int(steps)) break;

            float r = length(pos);
            if (r < 0.001 || r > u_maxDistance) break;

            // Disk plane is y = 0; spin axis is +y
            float z = pos.y;
            float rPlane = length(pos.xz);

            if (rPlane >= rInner && rPlane <= rOuter) {
                // Scale height H/r = H0 (constant), H = H0 * r
                float H = H0 * rPlane;
                float zNorm = (H > 0.0001) ? z / H : 0.0;
                float rho = density * pow(rInner / rPlane, 1.5) * exp(-zNorm * zNorm);

                // Keplerian temperature profile T ~ r^(-3/4)
                float T = temp0 * pow(rInner / rPlane, 0.75);
                // Blackbody emissivity ~ T^4 (Stefan-Boltzmann)
                float j = rho * T * T * T * T;

                // Optical depth over this step
                float dTau = rho * opacity * stepSize;
                tau += dTau;

                // Doppler beaming: azimuthal orbital velocity v_phi = sqrt(M/r)
                // (geometric units, c = 1). Prograde for Kerr.
                float vPhi = sqrt(u_mass / rPlane);
                float cosPhi = (rPlane > 0.0001) ? pos.x / rPlane : 0.0;
                float sinPhi = (rPlane > 0.0001) ? pos.z / rPlane : 0.0;
                // Velocity vector in the disk plane (tangential, +phi direction)
                vec3 vel = vec3(-sinPhi, 0.0, cosPhi) * vPhi;
                float vDotN = dot(vel, dir);
                // Doppler factor delta = 1 / (1 - v . n); blueshift when v.n < 0
                float delta = 1.0 / max(1.0 - vDotN, 0.01);
                float beaming = delta * delta * delta * delta * u_volumetricDiskDopplerBoost;

                // Self-attenuated contribution: j * exp(-tau_before) * ds.
                // This is the integral of j(s) * exp(-tau(s)) ds, so the returned
                // value is already the disk's own emitted flux at the observer.
                emissivity += j * stepSize * exp(-tau) * beaming * u_volumetricDiskIntensity;
            }

            // Adaptive step near the black hole
            float adaptiveStep = stepSize * max(r / (3.0 * rs), 0.1);
            dir = normalize(dir + computeDeflection(pos, u_mass, u_spin) * adaptiveStep);
            pos += dir * adaptiveStep;
        }

        tauOut = tau;
        return emissivity;
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

        // Volumetric accretion disk: integrate emissivity along the lensed ray.
        // ISCO from the exact Bardeen-Press-Teukolsky formula (matches the CPU
        // computeISCO()); the previous mix() approximation diverged badly for
        // high spin, which is exactly the visually dominant regime.
        float rs = 2.0 * u_mass;
        float isco = computeISCO(u_spin) * u_mass;
        float diskTau = 0.0;
        vec3 diskEmission = volumetricDiskEmission(u_cameraPos, rayDir, rs, isco, diskTau);

        // Radiative-transfer composite.  volumetricDiskEmission returns the
        // self-attenuated emission integral  ∫ j(s) exp(-tau(s)) ds, and diskTau
        // is the total optical depth to the observer.  The background (lensed
        // star field) is attenuated by exp(-tau) and the disk emission is added
        // on top -- this is the correct form in both limits:
        //   optically thin  (tau -> 0): background passes through, disk faint
        //   optically thick (tau -> infinity): background is occulted, disk
        //                                    saturates to its source function
        // The old code multiplied exp(-tau) INTO the emission, which made a thick
        // disk vanish and a thin disk stay bright -- both backwards.
        float transmission = exp(-diskTau);
        color = color * transmission + diskEmission;

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

float GravitationalLensing::computeVolumetricDiskEmissivity(
    const std::array<float, 3>& pos,
    const VolumetricDiskParams& params,
    float mass,
    float spin)
{
    float rPlane = std::sqrt(pos[0] * pos[0] + pos[2] * pos[2]);
    float rInner = params.diskInnerRadius;
    if (rInner <= 0.0f) {
        // Exact Kerr ISCO (matches computeISCO_GLSL in the shader); the old
        // hardcoded 6M only ever agreed at spin=0.
        rInner = static_cast<float>(computeISCORadius(spin)) * mass;
    }
    if (rPlane < rInner || rPlane > params.diskOuterRadius) return 0.0f;

    float H = params.diskScaleHeight * rPlane;
    float zNorm = (H > 0.0001f) ? pos[1] / H : 0.0f;
    float rho = params.diskDensity * std::pow(rInner / rPlane, 1.5f) * std::exp(-zNorm * zNorm);

    float T = params.diskTemperature * std::pow(rInner / rPlane, 0.75f);
    float j = rho * T * T * T * T;  // blackbody emissivity ~ T^4

    // Doppler beaming from azimuthal flow: v_phi = sqrt(M/r), delta = 1/(1 - v.n)
    // Reference ray direction is +x (n = (1,0,0)), so only the x-component of the
    // tangential velocity contributes.  Prograde (+phi) flow: vel = (-sin, 0, cos).
    float vPhi = std::sqrt(mass / rPlane);
    float sinPhi = pos[2] / rPlane;
    float velX = -sinPhi * vPhi;
    float vDotN = velX;
    float delta = 1.0f / std::max(1.0f - vDotN, 0.01f);
    float beaming = delta * delta * delta * delta * params.diskDopplerBoost;

    return j * beaming;
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
                lum += computeVolumetricDiskEmissivity(pos, params, mass, spin) * dr * dphi * r * (2.0f * Hmax) / float(NZ);
            }
        }
    }
    return lum;
}

} // namespace quantumverse
