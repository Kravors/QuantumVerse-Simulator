/**
 * @file PostProcess.h
 * @brief HDR bloom post-processing pipeline using FBO-based multi-pass rendering
 *
 * Implements a separable Gaussian blur bloom effect with ACES tone mapping.
 * Pipeline stages:
 *   1. Scene renders to HDR floating-point FBO (RGBA16F)
 *   2. Bright-pass filter extracts areas above luminance threshold
 *   3. Separable Gaussian blur (horizontal + vertical, ping-pong between two FBOs)
 *   4. Final composite combines original HDR + bloom with tone mapping
 *
 * OpenGL 4.5 Core Profile. Designed for integration with QmlGlRenderer.
 */

#ifndef QUANTUMVERSE_POST_PROCESS_H
#define QUANTUMVERSE_POST_PROCESS_H

#include "glad.h"

namespace quantumverse {

/**
 * @brief FBO-based HDR bloom post-processing pipeline
 *
 * Manages all framebuffer objects, shader programs, and render passes
 * needed to apply bloom and tone mapping to the rendered scene.
 */
class PostProcess
{
public:
    /**
     * @brief Construct a PostProcess pipeline (no GL initialization)
     */
    PostProcess();

    /**
     * @brief Destroy and release all OpenGL resources
     */
    ~PostProcess();

    // Non-copyable, non-movable (owns GL resources)
    PostProcess(const PostProcess&) = delete;
    PostProcess& operator=(const PostProcess&) = delete;
    PostProcess(PostProcess&&) = delete;
    PostProcess& operator=(PostProcess&&) = delete;

    /**
     * @brief Initialize OpenGL resources (must be called with valid GL context)
     * @param width Initial viewport width in pixels
     * @param height Initial viewport height in pixels
     */
    void initialize(int width, int height);

    /**
     * @brief Check if OpenGL resources are initialized
     * @return true if initialize() has been called successfully
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Resize all FBOs to match new viewport dimensions
     * @param width New viewport width
     * @param height New viewport height
     */
    void resize(int width, int height);

    /**
     * @brief Bind the HDR scene FBO for rendering
     *
     * Call this before rendering the scene. All subsequent draw calls
     * will render into the HDR floating-point color buffer.
     */
    void beginScene();

    /**
     * @brief Execute the full post-processing pipeline and present to screen
     *
     * Call this after rendering the scene. Performs bright-pass, blur,
     * composite with tone mapping, and renders to the default framebuffer.
     */
    void endScene();

    /**
     * @brief Enable or disable the bloom effect
     * @param enabled true to enable bloom, false to pass through
     */
    void setBloomEnabled(bool enabled) { m_bloomEnabled = enabled; }

    /**
     * @brief Check if bloom is enabled
     * @return true if bloom is enabled
     */
    bool isBloomEnabled() const { return m_bloomEnabled; }

    /**
     * @brief Set the bloom intensity
     * @param intensity Bloom multiplier (0.0 = no bloom, 1.0 = default, >1.0 = strong)
     */
    void setBloomIntensity(float intensity) { m_bloomIntensity = intensity; }

    /**
     * @brief Get the current bloom intensity
     * @return Current bloom intensity multiplier
     */
    float bloomIntensity() const { return m_bloomIntensity; }

    /**
     * @brief Set the bright-pass threshold
     * @param threshold Luminance threshold for bloom extraction (default 1.0)
     */
    void setBloomThreshold(float threshold) { m_bloomThreshold = threshold; }

    /**
     * @brief Get the current bright-pass threshold
     * @return Current bloom threshold
     */
    float bloomThreshold() const { return m_bloomThreshold; }

    /**
     * @brief Set the tone mapping mode
     * @param mode 0 = ACES, 1 = Reinhard, 2 = Uncharted2
     */
    void setToneMappingMode(int mode) { m_toneMappingMode = mode; }

    /**
     * @brief Get the current tone mapping mode
     * @return Current tone mapping mode (0=ACES, 1=Reinhard, 2=Uncharted2)
     */
    int toneMappingMode() const { return m_toneMappingMode; }

    /**
     * @brief Set the exposure value for tone mapping
     * @param exposure Exposure multiplier (1.0 = neutral)
     */
    void setExposure(float exposure) { m_exposure = exposure; }

    /**
     * @brief Get the current exposure value
     * @return Current exposure multiplier
     */
    float exposure() const { return m_exposure; }

private:
    /**
     * @brief Create or recreate an FBO with the given size and format
     * @param fbo Output FBO handle
     * @param texture Output color texture handle
     * @param width FBO width
     * @param height FBO height
     * @param internalFormat OpenGL internal texture format (e.g. GL_RGBA16F)
     */
    void createFbo(GLuint& fbo, GLuint& texture, int width, int height, GLenum internalFormat);

    /**
     * @brief Delete an FBO and its associated texture
     * @param fbo FBO handle (set to 0)
     * @param texture Texture handle (set to 0)
     */
    void destroyFbo(GLuint& fbo, GLuint& texture);

    /**
     * @brief Compile and link a shader program from source strings
     * @param vertexSource GLSL vertex shader source
     * @param fragmentSource GLSL fragment shader source
     * @return Linked program handle, or 0 on failure
     */
    GLuint compileShaderProgram(const char* vertexSource, const char* fragmentSource);

    /**
     * @brief Render a fullscreen quad using the currently bound program
     */
    void drawFullscreenQuad();

    // State
    bool m_initialized;
    int m_width;
    int m_height;

    // HDR scene FBO
    GLuint m_sceneFbo;
    GLuint m_sceneTexture;

    // Bright-pass FBO (half resolution)
    GLuint m_brightPassFbo;
    GLuint m_brightPassTexture;

    // Ping-pong blur FBOs (half resolution)
    GLuint m_blurFbo[2];
    GLuint m_blurTexture[2];

    // Fullscreen quad geometry
    GLuint m_quadVao;

    // Shader programs
    GLuint m_brightPassProgram;
    GLuint m_blurProgram;
    GLuint m_compositeProgram;

    // Parameters
    bool m_bloomEnabled;
    float m_bloomIntensity;
    float m_bloomThreshold;
    int m_toneMappingMode;
    float m_exposure;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_POST_PROCESS_H
