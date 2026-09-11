/**
 * @file GravitationalLensing.h
 * @brief Ray-marching gravitational lensing renderer for black hole visualization
 *
 * Performs real-time ray-marching through curved spacetime to produce
 * the iconic "black hole shadow" effect with gravitational lensing of
 * background star fields. Supports both Schwarzschild and Kerr metrics.
 *
 * Pipeline:
 *   1. Generate procedural star field on a cube map
 *   2. For each pixel, trace a ray backward from the camera
 *   3. At each step, compute geodesic deviation using Christoffel symbols
 *   4. If ray falls into black hole -> black pixel (shadow)
 *   5. If ray escapes -> sample star field at deflected angle
 */

#ifndef QUANTUMVERSE_GRAVITATIONAL_LENSING_H
#define QUANTUMVERSE_GRAVITATIONAL_LENSING_H

#include "glad.h"

#include <memory>
#include <array>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>
#include <cstdint>

#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Parameters controlling the gravitational lensing effect.
 */
struct LensingParams {
    float mass = 1.0f;              ///< Black hole mass in geometric units (M)
    float spin = 0.0f;              ///< Dimensionless spin a* = a/M in [0, 1)
    float cameraDistance = 10.0f;   ///< Camera distance in units of M
    float cameraTheta = 1.5708f;    ///< Camera polar angle (pi/2 = equatorial)
    float cameraPhi = 0.0f;         ///< Camera azimuthal angle
    int raySteps = 256;             ///< Number of ray-marching steps
    float maxDistance = 50.0f;      ///< Maximum ray distance in units of M
    float shadowIntensity = 1.0f;   ///< Shadow darkness (0.0 = gray, 1.0 = black)
    float starFieldIntensity = 1.0f; ///< Brightness of background stars
    float photonRingIntensity = 1.0f; ///< Brightness of photon ring glow
    float accretionDiskIntensity = 0.8f; ///< Accretion disk brightness
    bool enablePhotonRing = true;   ///< Render photon ring
    bool enableAccretionDisk = true; ///< Render accretion disk
    bool enableStarField = true;    ///< Render background star field
};

/**
 * @brief Ray-marching gravitational lensing renderer.
 *
 * Renders the visual distortion of background starlight caused by a
 * black hole's gravity, producing the iconic "black hole shadow" effect.
 * Uses GPU-accelerated ray-marching through curved spacetime.
 */
class GravitationalLensing {
public:
    /// @brief Lensing parameters struct.
    using LensingParams = ::quantumverse::LensingParams;

    /**
     * @brief Construct a gravitational lensing renderer.
     * @param metric Shared pointer to the metric tensor (Kerr or Schwarzschild)
     */
    explicit GravitationalLensing(std::shared_ptr<MetricTensor> metric);

    /**
     * @brief Destroy and release all OpenGL resources.
     */
    ~GravitationalLensing();

    // Non-copyable, non-movable (owns GL resources)
    GravitationalLensing(const GravitationalLensing&) = delete;
    GravitationalLensing& operator=(const GravitationalLensing&) = delete;
    GravitationalLensing(GravitationalLensing&&) = delete;
    GravitationalLensing& operator=(GravitationalLensing&&) = delete;

    /**
     * @brief Initialize OpenGL resources (must be called with valid GL context).
     * @param width Viewport width in pixels
     * @param height Viewport height in pixels
     */
    void initialize(int width, int height);

    /**
     * @brief Check if OpenGL resources are initialized.
     * @return true if initialize() has been called successfully
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Resize the rendering buffers.
     * @param width New viewport width
     * @param height New viewport height
     */
    void resize(int width, int height);

    /**
     * @brief Render the gravitational lensing effect.
     *
     * Performs ray-marching through curved spacetime and renders
     * the lensed background star field with black hole shadow.
     *
     * @param viewMatrix 4x4 view matrix (column-major)
     * @param projectionMatrix 4x4 projection matrix (column-major)
     */
    void render(const float* viewMatrix, const float* projectionMatrix);

    /**
     * @brief Set the lensing parameters.
     * @param params New lensing parameters
     */
    void setParams(const LensingParams& params);

    /**
     * @brief Get the current lensing parameters.
     * @return Current lensing parameters
     */
    const LensingParams& params() const { return m_params; }

    /**
     * @brief Set the metric tensor.
     * @param metric Shared pointer to the metric tensor
     */
    void setMetric(std::shared_ptr<MetricTensor> metric);

    /**
     * @brief Get the current metric tensor.
     * @return Shared pointer to the current metric
     */
    std::shared_ptr<MetricTensor> metric() const { return m_metric; }

    /**
     * @brief Set the ray-marching step count.
     * @param steps Number of steps (higher = more accurate, slower)
     */
    void setRaySteps(int steps) { m_params.raySteps = std::clamp(steps, 32, 1024); }

    /**
     * @brief Get the current ray-marching step count.
     * @return Current step count
     */
    int raySteps() const { return m_params.raySteps; }

    /**
     * @brief Set the shadow intensity.
     * @param intensity Shadow darkness (0.0 = gray, 1.0 = fully black)
     */
    void setShadowIntensity(float intensity) { m_params.shadowIntensity = std::clamp(intensity, 0.0f, 1.0f); }

    /**
     * @brief Get the current shadow intensity.
     * @return Current shadow intensity
     */
    float shadowIntensity() const { return m_params.shadowIntensity; }

    /**
     * @brief Enable or disable the lensing effect.
     * @param enabled true to enable, false to disable
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }

    /**
     * @brief Check if lensing is enabled.
     * @return true if enabled
     */
    bool isEnabled() const { return m_enabled; }

    /**
     * @brief Generate a procedural star field texture.
     *
     * Creates a cube map with procedurally generated stars for the
     * background sky that gets lensed by the black hole.
     *
     * @param seed Random seed for star field generation
     * @param starCount Number of stars to generate
     */
    void generateStarField(uint32_t seed = 42, int starCount = 2000);

    /**
     * @brief Set a custom background texture (cube map).
     * @param textureId OpenGL texture ID of the cube map
     */
    void setBackgroundTexture(GLuint textureId);

    /**
     * @brief Compute the photon sphere radius for the current metric.
     * @return Photon sphere radius in geometric units
     */
    double computePhotonSphereRadius() const;

    /**
     * @brief Compute the innermost stable circular orbit (ISCO) radius.
     * @return ISCO radius in geometric units (length, = r/M * M).
     */
    double computeISCO() const;

    /**
     * @brief Dimensionless ISCO radius r/M for a given dimensionless spin.
     *
     * Pure function mirroring computeISCO_GLSL() so the CPU reference and the
     * GPU agree bit-for-bit.  Positive spin = prograde (shrinks ISCO).
     *
     * @param spin Dimensionless spin a/M in (-1, 1).
     * @return ISCO radius in units of M.
     */
    static double computeISCORadius(double spin);

    /**
     * @brief Parameters controlling the volumetric accretion disk.
     *
     * The disk is ray-marched as a volume (density x blackbody emissivity x
     * Doppler beaming) rather than rendered as a flat billboard.  This models
     * a thin, optically thick Shakura-Sunyaev-style disk in geometric units
     * (c = 1), so velocities are fractions of c and the Doppler factor is
     * delta = 1 / (1 - v . n).
     */
    struct VolumetricDiskParams {
        bool enableVolumetricDisk = false; ///< Ray-march the disk volume
        float diskDensity = 1.0f;           ///< Normalised density at rInner
        float diskTemperature = 1.0f;       ///< Normalised temperature at rInner
        float diskScaleHeight = 0.1f;       ///< H/r at rInner (disk thickness)
        float diskInnerRadius = 0.0f;       ///< Inner edge (0 = use ISCO)
        float diskOuterRadius = 20.0f;      ///< Outer edge in units of M
        int diskRaySteps = 64;              ///< Steps per ray through the volume
        float diskOpacity = 1.0f;          ///< Tau per unit density (optical depth)
        float diskDopplerBoost = 1.0f;      ///< Global Doppler beaming gain
    };

    /**
     * @brief Set the volumetric disk parameters.
     * @param params New volumetric disk parameters
     */
    void setVolumetricDiskParams(const VolumetricDiskParams& params);

    /**
     * @brief Get the current volumetric disk parameters.
     * @return Current volumetric disk parameters
     */
    const VolumetricDiskParams& volumetricDiskParams() const { return m_volumetricDisk; }

    /**
     * @brief Enable or disable the volumetric disk.
     * @param enabled true to enable, false to disable
     */
    void setEnabledVolumetricDisk(bool enabled) { m_volumetricDisk.enableVolumetricDisk = enabled; }

    /**
     * @brief Check if the volumetric disk is enabled.
     * @return true if enabled
     */
    bool isVolumetricDiskEnabled() const { return m_volumetricDisk.enableVolumetricDisk; }

    /**
     * @brief Compute the local disk emissivity at a point (CPU reference).
     *
     * Pure function of position and parameters; mirrors the GLSL
     * volumetricDiskEmission() used at render time so the physics can be
     * validated headlessly.  @return Emissivity in normalised units (>= 0).
     *
     * @param pos Position in geometric units (disk lies in the y = 0 plane)
     * @param params Disk parameters
     * @param mass Black hole mass in geometric units
     * @param spin Dimensionless spin a/M (default 0 = Schwarzschild ISCO)
     */
    static float computeVolumetricDiskEmissivity(
        const std::array<float, 3>& pos,
        const VolumetricDiskParams& params,
        float mass,
        float spin = 0.0f);

    /**
     * @brief Compute the total disk luminosity by integrating the volume.
     *
     * Integrates emissivity over r in [rInner, rOuter], all phi, and
     * z in [-4H, 4H].  @return Integrated luminosity (>= 0).
     *
     * @param params Disk parameters
     * @param mass Black hole mass in geometric units
     * @param spin Dimensionless spin a/M (default 0 = Schwarzschild ISCO)
     */
    static float computeDiskLuminosity(const VolumetricDiskParams& params, float mass,
                                       float spin = 0.0f);

private:
    /**
     * @brief Compile and link the lensing shader program.
     */
    void compileLensingShader();

    /**
     * @brief Compile and link the fullscreen quad shader.
     */
    void compileQuadShader();

    /**
     * @brief Create the star field cube map texture.
     */
    void createStarFieldTexture();

    /**
     * @brief Create the fullscreen quad geometry.
     */
    void createFullscreenQuad();

    /**
     * @brief Update shader uniforms from current parameters.
     */
    void updateUniforms();

    /**
     * @brief Set a uniform value by name.
     */
    void setUniformFloat(const char* name, float value);
    void setUniformInt(const char* name, int value);
    void setUniformVec2(const char* name, float x, float y);
    void setUniformVec3(const char* name, float x, float y, float z);
    void setUniformMat4(const char* name, const float* matrix);

    // State
    bool m_initialized = false;
    bool m_enabled = true;
    int m_width = 0;
    int m_height = 0;

    // Parameters
    LensingParams m_params;
    VolumetricDiskParams m_volumetricDisk;

    // Metric
    std::shared_ptr<MetricTensor> m_metric;

    // OpenGL resources
    GLuint m_lensingProgram = 0;
    GLuint m_quadProgram = 0;
    GLuint m_starFieldCubemap = 0;
    GLuint m_quadVao = 0;
    GLuint m_quadVbo = 0;

    // Star field generation state
    std::vector<std::array<uint8_t, 3>> m_starData;
    uint32_t m_starSeed = 42;
    int m_starCount = 2000;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_GRAVITATIONAL_LENSING_H
