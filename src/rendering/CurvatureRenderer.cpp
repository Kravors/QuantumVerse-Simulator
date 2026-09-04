/**
 * @file CurvatureRenderer.cpp
 * @brief Implementation of CurvatureRenderer and ShaderProgram methods
 *
 * Contains the extracted OpenGL-dependent ShaderProgram inline methods
 * that require glad.h to be included first. This file includes glad.h
 * before any Qt OpenGL headers, avoiding the include-order conflict.
 */

// glad.h MUST be first to provide OpenGL function prototypes
// before any system <GL/gl.h> is pulled in by other headers
#include "glad.h"

#include "CurvatureRenderer.h"
#include "../physics/CurvatureCalculator.h"
#include <QVector4D>
#include <QDebug>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#ifdef _WIN32
#include <windows.h>  // Required for GetEnvironmentVariableA in isTruthyEnvironmentFlag
#endif

namespace quantumverse {

// Forward declaration for environment variable check
static bool isTruthyEnvironmentFlag(const char* name);

// Simple vertex shader for fallback rendering
static const char* fallbackVert = R"(
    #version 450 core
    layout(location = 0) in vec3 aPos;
    uniform mat4 mvp;
    void main() {
        gl_Position = mvp * vec4(aPos, 1.0);
    }
)";

// Simple fragment shader for fallback rendering (magenta)
static const char* fallbackFrag = R"(
    #version 450 core
    out vec4 outColor;
    void main() { outColor = vec4(1.0, 0.0, 1.0, 1.0); }
)";

// Fallback shader program ID
static unsigned int fallbackShaderId = 0;

// ============================================================================
// ShaderProgram Implementation
// ============================================================================

void ShaderProgram::use() const
{
    glUseProgram(id);
}

void ShaderProgram::release() const
{
    glUseProgram(0);
}

void ShaderProgram::setUniform(const char* name, float value) const
{
    GLint loc = glGetUniformLocation(id, name);
    if (loc >= 0) glUniform1f(loc, value);
}

void ShaderProgram::setUniform(const char* name, int value) const
{
    GLint loc = glGetUniformLocation(id, name);
    if (loc >= 0) glUniform1i(loc, value);
}

void ShaderProgram::setUniform(const char* name, const float* matrix) const
{
    GLint loc = glGetUniformLocation(id, name);
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, matrix);
}

void ShaderProgram::setUniform(const char* name, const double* matrix) const
{
    // Convert double array to float array for OpenGL
    static thread_local float floatMatrix[16];
    for (int i = 0; i < 16; i++) {
        floatMatrix[i] = static_cast<float>(matrix[i]);
    }
    GLint loc = glGetUniformLocation(id, name);
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, floatMatrix);
}

void ShaderProgram::setUniform(const char* name, const QVector4D& color) const
{
    GLint loc = glGetUniformLocation(id, name);
    if (loc >= 0) glUniform4f(loc, color.x(), color.y(), color.z(), color.w());
}

// ============================================================================
// CurvatureRenderer Implementation
// ============================================================================

CurvatureRenderer::CurvatureRenderer(
    int resolution,
    float size,
    CurvatureMode initialMode
) : gridResolution(resolution), gridSize(size), mode(initialMode),
     wireframe(false), showGrid(true), showLightCones(false),
     vao(0), vbo(0), ebo(0), time(0.0f), animationSpeed(1.0f),
     m_initialized(false), lodLevel(0), cacheValid(false),
     lightConeVao(0), lightConeVbo(0), lightConeEbo(0),
     lightConeBuffersInitialized(false),
     fallbackVao(0), fallbackVbo(0),
     cellsPerDimension(4),
     m_planeMode(true), m_planeResolution(100)
{
     // Only initialize non-OpenGL data in constructor
     // OpenGL resources (VBOs, VAOs, shaders) are initialized lazily in initializeGL()
     initializeGrid();
     initializeSpatialPartitioning();
}

CurvatureRenderer::~CurvatureRenderer()
{
    // Cleanup is handled externally when GL context is active
}

void CurvatureRenderer::initializeFallbackGeometry()
{
    // Create a simple quad for fallback rendering (magenta warning square)
    float quadVertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f
    };
    
    glGenVertexArrays(1, &fallbackVao);
    glGenBuffers(1, &fallbackVbo);
    glBindVertexArray(fallbackVao);
    glBindBuffer(GL_ARRAY_BUFFER, fallbackVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void CurvatureRenderer::initializeGL()
{
    if (m_initialized) return;

    // Initialize fallback geometry first
    initializeFallbackGeometry();

    // Generate VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Generate VBO
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(CurvatureVertex),
                 vertices.data(),
                 GL_DYNAMIC_DRAW);

    // Generate EBO
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(),
                 GL_STATIC_DRAW);

    // Vertex attributes layout matches CurvatureVertex struct:
    //   location 0: position (3 floats)    offset 0
    //   location 1: normal   (3 floats)    offset 12
    //   location 2: color    (4 floats)    offset 24
    //   location 3: curvature (1 float)    offset 40
    //   location 4: time_dilation (1 float) offset 44
    // Total stride = 48 bytes (12 floats)

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                        sizeof(CurvatureVertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                        sizeof(CurvatureVertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE,
                        sizeof(CurvatureVertex), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE,
                        sizeof(CurvatureVertex), (void*)(10 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE,
                        sizeof(CurvatureVertex), (void*)(11 * sizeof(float)));
    glEnableVertexAttribArray(4);

    glBindVertexArray(0);

    // Compile shaders
    const char* gridVert = R"(
        #version 450 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec4 aColor;
        layout(location = 3) in float aCurvature;
        layout(location = 4) in float aTimeDilation;

        uniform mat4 viewMatrix;
        uniform mat4 projectionMatrix;
        uniform float time;
        uniform float curvatureScale;
        uniform int curvatureMode;

        out vec4 fragColor;
        out float curvatureValue;
        out float timeDilation;

        void main() {
            vec3 displacedPos = aPos;
            if (curvatureMode == 0 || curvatureMode == 3) {
                float displacement = aCurvature * curvatureScale * 0.05;
                displacedPos += aNormal * displacement;
            }
            gl_Position = projectionMatrix * viewMatrix * vec4(displacedPos, 1.0);
            fragColor = aColor;
            curvatureValue = aCurvature;
            timeDilation = aTimeDilation;
        }
    )";

    const char* gridFrag = R"(
        #version 450 core
        in vec4 fragColor;
        in float curvatureValue;
        in float timeDilation;
        uniform int curvatureMode;
        out vec4 outColor;

        void main() {
            vec3 baseColor = fragColor.rgb;
            float alpha = fragColor.a;

            switch (curvatureMode) {
                case 0: {
                    // Color based on curvature magnitude using logarithmic scale
                    // Deep blue (low) -> Cyan -> Green -> Yellow -> Red (high)
                    float K = abs(curvatureValue);
                    float logK = log2(max(K, 1e-10)) * 0.5;
                    float hue = clamp(logK * 0.1 + 0.5, 0.0, 1.0);
                    if (hue < 0.25)
                        baseColor = mix(vec3(0.15,0.35,1.0), vec3(0,1,1), hue * 4.0);
                    else if (hue < 0.5)
                        baseColor = mix(vec3(0,1,1), vec3(0,1,0), (hue - 0.25) * 4.0);
                    else if (hue < 0.75)
                        baseColor = mix(vec3(0,1,0), vec3(1,1,0), (hue - 0.5) * 4.0);
                    else
                        baseColor = mix(vec3(1,1,0), vec3(1,0,0), (hue - 0.75) * 4.0);
                    alpha = 0.7;
                    break;
                }
                case 1: {
                    float K = abs(curvatureValue);
                    float logK = log2(max(K, 1e-10)) * 0.5;
                    float hue = clamp(logK * 0.1 + 0.5, 0.0, 1.0);
                    if (hue < 0.25)
                        baseColor = mix(vec3(0.15,0.35,1.0), vec3(0,1,1), hue * 4.0);
                    else if (hue < 0.5)
                        baseColor = mix(vec3(0,1,1), vec3(0,1,0), (hue - 0.25) * 4.0);
                    else if (hue < 0.75)
                        baseColor = mix(vec3(0,1,0), vec3(1,1,0), (hue - 0.5) * 4.0);
                    else
                        baseColor = mix(vec3(1,1,0), vec3(1,0,0), (hue - 0.75) * 4.0);
                    alpha = 0.7;  // Increased from 0.2 for visibility
                    break;
                }
                case 2: {
                    float intensity = clamp(log2(max(curvatureValue, 1.0)) * 0.05, 0.0, 1.0);
                    baseColor = mix(vec3(0.15, 0.35, 1.0), vec3(1.0, 0.1, 0.1), intensity);
                    alpha = 0.7;  // Increased from 0.2 for visibility
                    break;
                }
                case 3: {
                    float d = clamp(timeDilation, 0.0, 2.0) * 0.5;
                    if (d < 0.5)
                        baseColor = vec3(1.0, d * 2.0, 0.0);
                    else
                        baseColor = vec3((1.0 - d) * 2.0, 1.0, (d - 0.5) * 2.0);
                    alpha = 0.25;
                    break;
                }
                default:
                    baseColor = vec3(0.2, 0.6, 1.0);
                    alpha = 0.15;
                    break;
            }
            outColor = vec4(baseColor, alpha);
        }
    )";

    const char* geodesicVert = R"(
        #version 450 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec4 aColor;
        uniform mat4 viewMatrix;
        uniform mat4 projectionMatrix;
        out vec4 fragColor;
        void main() {
            gl_Position = projectionMatrix * viewMatrix * vec4(aPos, 1.0);
            fragColor = aColor;
        }
    )";

    const char* geodesicFrag = R"(
        #version 450 core
        in vec4 fragColor;
        out vec4 outColor;
        void main() { outColor = fragColor; }
    )";

    const char* overlayVert = R"(
        #version 450 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec4 aColor;
        uniform mat4 projectionMatrix;
        out vec4 fragColor;
        void main() {
            gl_Position = projectionMatrix * vec4(aPos, 0.0, 1.0);
            fragColor = aColor;
        }
    )";

    const char* overlayFrag = R"(
        #version 450 core
        in vec4 fragColor;
        out vec4 outColor;
        void main() { outColor = fragColor; }
    )";

    compileShader(gridShader.id, gridVert, gridFrag);
    compileShader(curvatureShader.id, gridVert, gridFrag);
    compileShader(geodesicShader.id, geodesicVert, geodesicFrag);
    compileShader(overlayShader.id, overlayVert, overlayFrag);

    // Compile fallback shader
    compileShader(fallbackShaderId, fallbackVert, fallbackFrag);

    // Initialize light cone buffers (also requires GL context)
    initializeLightConeBuffers();

    m_initialized = true;
}

bool CurvatureRenderer::compileShader(unsigned int& program,
                                        const char* vertexSource,
                                        const char* fragmentSource)
{
    // Delete prior program if present to avoid resource leak on recompile
    if (program != 0) {
        glDeleteProgram(program);
        program = 0;
    }

    // Log shader source for debugging (only in debug builds)
    if (isTruthyEnvironmentFlag("QUANTUMVERSE_DEBUG_SHADERS")) {
        std::fprintf(stderr, "[CurvatureRenderer] Compiling vertex shader:\n%s\n", vertexSource);
        std::fprintf(stderr, "[CurvatureRenderer] Compiling fragment shader:\n%s\n", fragmentSource);
    }

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);

    int success;
    char infoLog[1024];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, sizeof(infoLog), nullptr, infoLog);
        std::fprintf(stderr, "ERROR: Vertex shader compilation failed:\n%s\n", infoLog);
        // Also print the source for context
        std::fprintf(stderr, "Vertex shader source:\n%s\n", vertexSource);
        glDeleteShader(vertexShader);
        return false;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, sizeof(infoLog), nullptr, infoLog);
        std::fprintf(stderr, "ERROR: Fragment shader compilation failed:\n%s\n", infoLog);
        std::fprintf(stderr, "Fragment shader source:\n%s\n", fragmentSource);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
        std::fprintf(stderr, "ERROR: Shader program linking failed:\n%s\n", infoLog);
        glDeleteProgram(program);
        program = 0;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    if (success != 0) {
        std::fprintf(stderr, "[CurvatureRenderer] Shader program compiled successfully (id=%u)\n", program);
    }
    return success != 0;
}

// Helper for environment variable check (used in debug builds)
static bool isTruthyEnvironmentFlag(const char* name) {
#if defined(_WIN32)
    char value[32]{};
    const DWORD length = GetEnvironmentVariableA(name, value, sizeof(value));
    return length > 0 && length < sizeof(value) &&
           (value[0] == '1' || value[0] == 't' || value[0] == 'T' ||
            value[0] == 'y' || value[0] == 'Y');
#else
    const char* value = std::getenv(name);
    if (!value || value[0] == '\0') return false;
    return (value[0] == '1' || value[0] == 't' || value[0] == 'T' ||
            value[0] == 'y' || value[0] == 'Y');
#endif
}

void CurvatureRenderer::initializeLightConeBuffers()
{
    // Generate VAO
    glGenVertexArrays(1, &lightConeVao);
    glBindVertexArray(lightConeVao);

    // Generate VBO
    glGenBuffers(1, &lightConeVbo);
    glBindBuffer(GL_ARRAY_BUFFER, lightConeVbo);
    // Initial buffer allocation will happen when we have data
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    // Generate EBO
    glGenBuffers(1, &lightConeEbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lightConeEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                        sizeof(LightConeVertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute (overlay shader expects aColor at location 1)
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
                        sizeof(LightConeVertex), (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    lightConeBuffersInitialized = true;
}

void CurvatureRenderer::initializeSpatialPartitioning()
{
    // Create spatial partitioning grid for LOD updates
    cellsPerDimension = 4; // 4x4x4 = 64 cells
    int totalCells = cellsPerDimension * cellsPerDimension * cellsPerDimension;
    gridCells.resize(totalCells);

    int verticesPerCell = (gridResolution / cellsPerDimension) * 
                         (gridResolution / cellsPerDimension) * 
                         (gridResolution / cellsPerDimension);

    for (int i = 0; i < cellsPerDimension; i++) {
        for (int j = 0; j < cellsPerDimension; j++) {
            for (int k = 0; k < cellsPerDimension; k++) {
                int cellIndex = i * cellsPerDimension * cellsPerDimension + 
                               j * cellsPerDimension + k;
                gridCells[cellIndex].startIndex = 
                    (i * (gridResolution / cellsPerDimension) * gridResolution * gridResolution) +
                    (j * (gridResolution / cellsPerDimension) * gridResolution) +
                    (k * (gridResolution / cellsPerDimension));
                gridCells[cellIndex].endIndex = gridCells[cellIndex].startIndex + verticesPerCell - 1;
                gridCells[cellIndex].needsUpdate = true;
            }
        }
    }
}

void CurvatureRenderer::render(const float* viewMatrix, const float* projectionMatrix)
{
    static int s_callCount = 0;
    if (s_callCount++ < 5) {
        std::cout << "[CurvatureRenderer] render() ENTERED, call #" << s_callCount << std::endl;
    }
    // Null renderer guard - check if properly initialized
    if (!m_initialized) {
        static int s_uninitCount = 0;
        if (s_uninitCount++ < 5) {
            std::cout << "[CurvatureRenderer] NOT INITIALIZED, drawing fallback" << std::endl;
        }
        // Draw magenta warning square using modern OpenGL
        glUseProgram(fallbackShaderId);
        glBindVertexArray(fallbackVao);
        // Simple orthographic projection for fallback
        float mvp[16] = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };
        GLint loc = glGetUniformLocation(fallbackShaderId, "mvp");
        if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, mvp);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);
        glUseProgram(0);
        return;
    }
    
    if (!showGrid) {
        static int s_showGridCount = 0;
        if (s_showGridCount++ < 5) {
            std::cout << "[CurvatureRenderer] showGrid is FALSE, skipping render" << std::endl;
        }
        return;
    }
    
    static int s_renderCount = 0;
    if (s_renderCount++ < 5) {
        std::cout << "[CurvatureRenderer] render() called, showGrid=true, mode=" << static_cast<int>(mode)
                  << ", planeMode=" << m_planeMode << ", vertices=" << vertices.size() << std::endl;
    }
    
    // [DIAG] Save the current FBO so we can restore it after drawing.
    // This prevents state leakage that could cause the QML FBO screenshot
    // to capture the wrong buffer (black screen bug).
    GLint oldFBO = -1;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFBO);
    if (isTruthyEnvironmentFlag("QUANTUMVERSE_FBO_CHECK")) {
        std::fprintf(stderr, "[CurvatureRenderer] FBO before draw: %d\n", oldFBO);
    }

    // FBO completeness check (debug mode)
    if (isTruthyEnvironmentFlag("QUANTUMVERSE_FBO_CHECK")) {
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::fprintf(stderr, "FBO incomplete in CurvatureRenderer::render: 0x%X\n", status);
        }
    }

    if (wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    ShaderProgram* shader = &gridShader;
    if (mode == CurvatureMode::RIEMANN_COLOR || mode == CurvatureMode::CURVATURE_SCALAR)
        shader = &curvatureShader;

    shader->use();
    shader->setUniform("viewMatrix", viewMatrix);
    shader->setUniform("projectionMatrix", projectionMatrix);
    shader->setUniform("time", time);
    shader->setUniform("curvatureScale", 1.0f);
    shader->setUniform("curvatureMode", static_cast<int>(mode));

    glBindVertexArray(vao);
    // Enable depth testing for proper grid rendering
    glEnable(GL_DEPTH_TEST);
    // Disable blending for grid to prevent overlapping transparent lines
    // from accumulating and appearing white when camera is inside the grid
    glDisable(GL_BLEND);
    glDrawElements(wireframe ? GL_LINES : GL_TRIANGLES,
                   static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glEnable(GL_BLEND); // Re-enable blending for subsequent renders
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(0);

    // Restore polygon mode to FILL so subsequent renders (celestial bodies, etc.)
    // are not drawn as wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    shader->release();

    // [DIAG] Restore the FBO that was bound before we started drawing.
    // This ensures the QML FBO remains bound for screenshot capture.
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(oldFBO));
    if (isTruthyEnvironmentFlag("QUANTUMVERSE_FBO_CHECK")) {
        std::fprintf(stderr, "[CurvatureRenderer] FBO restored to: %d\n", oldFBO);
    }

    if (showLightCones)
        renderLightCones(viewMatrix, projectionMatrix);
}

void CurvatureRenderer::renderGeodesics(const std::vector<std::vector<Event4D>>& geodesics,
                                        const float* viewMatrix, const float* projectionMatrix)
{
    if (!m_initialized) return;

    geodesicShader.use();
    geodesicShader.setUniform("viewMatrix", viewMatrix);
    geodesicShader.setUniform("projectionMatrix", projectionMatrix);

    for (const auto& geodesic : geodesics) {
        if (geodesic.size() < 2) continue;
        // Dynamic geodesic rendering would use a separate VBO
    }

    geodesicShader.release();
}

void CurvatureRenderer::updateGLBuffers()
{
    if (!m_initialized || vertices.empty()) return;

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // Use glBufferData (not SubData) to reallocate when grid size changes
    // (e.g., switching between 2D plane and 3D cube modes)
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(CurvatureVertex),
                 vertices.data(),
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Also reallocate index buffer if needed
    if (!indices.empty()) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     indices.size() * sizeof(unsigned int),
                     indices.data(),
                     GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
}

void CurvatureRenderer::updateAllColors()
{
    for (auto& vertex : vertices)
        updateVertexColor(vertex);
    updateGLBuffers();
}

void CurvatureRenderer::setMode(CurvatureMode newMode)
{
    mode = newMode;
    updateAllColors();
}

void CurvatureRenderer::setWireframe(bool enable) { wireframe = enable; }
void CurvatureRenderer::toggleGrid(bool show) { showGrid = show; }
void CurvatureRenderer::toggleLightCones(bool show) { showLightCones = show; }

void CurvatureRenderer::setMetric(std::shared_ptr<MetricTensor> metric)
{
     currentMetric = metric;
     deformGrid();
     updateGLBuffers();
}

void CurvatureRenderer::setLightCone(std::shared_ptr<class LightCone> cone)
{
     lightCone = std::move(cone);
}

void CurvatureRenderer::addSingularity(std::shared_ptr<class SingularityHandler> singularity)
{
    singularities.push_back(singularity);
    deformGrid();
    updateGLBuffers();
}

void CurvatureRenderer::removeSingularity(std::shared_ptr<class SingularityHandler> singularity)
{
    auto it = std::find(singularities.begin(), singularities.end(), singularity);
    if (it != singularities.end()) {
        singularities.erase(it);
        deformGrid();
        updateGLBuffers();
    }
}

void CurvatureRenderer::updateTime(float deltaTime)
{
     time += deltaTime * animationSpeed;
     deformGrid();
     updateGLBuffers();
}

void CurvatureRenderer::regenerateGrid()
{
     initializeGrid();
     if (currentMetric) {
         deformGrid();
     }
     if (m_initialized) {
         updateGLBuffers();
     }
}

void CurvatureRenderer::setAnimationSpeed(float speed)
{
     animationSpeed = speed;
}

void CurvatureRenderer::setPlaneMode(bool enable)
{
    std::cout << "[CurvatureRenderer] setPlaneMode(" << enable << ") called, current=" << m_planeMode << std::endl;
    if (m_planeMode == enable) return;
    m_planeMode = enable;
    initializeGrid();
    if (currentMetric) {
        deformGrid();
        updateGLBuffers();
    }
    std::cout << "[CurvatureRenderer] setPlaneMode done, vertices=" << vertices.size() << ", indices=" << indices.size() << std::endl;
}

void CurvatureRenderer::setPlaneResolution(int resolution)
{
    if (resolution < 4) resolution = 4;
    if (resolution > 500) resolution = 500;
    if (m_planeResolution == resolution) return;
    m_planeResolution = resolution;
    if (m_planeMode) {
        initializeGrid();
        if (currentMetric) {
            deformGrid();
            updateGLBuffers();
        }
    }
}

const std::vector<CurvatureVertex>& CurvatureRenderer::getVertices() const { return vertices; }
int CurvatureRenderer::getResolution() const { return gridResolution; }
float CurvatureRenderer::getSize() const { return gridSize; }

void CurvatureRenderer::renderLightCones(const float* viewMatrix, const float* projectionMatrix)
{
     if (!lightCone || !lightCone->isValid()) return;
  
     // Simple wireframe rendering of light cone using GL_LINES
     // Full mesh rendering would use a dedicated shader with lighting
     glUseProgram(overlayShader.id);
     overlayShader.setUniform("projectionMatrix", projectionMatrix);
     overlayShader.setUniform("viewMatrix", viewMatrix);

    const auto& lightConeVertices = lightCone->getVertices();
    const auto& lightConeIndices = lightCone->getIndices();

    if (lightConeVertices.empty() || lightConeIndices.empty()) return;

    // Initialize light cone buffers if not already done
    if (!lightConeBuffersInitialized) {
        initializeLightConeBuffers();
    }

    // Update vertex data
    glBindBuffer(GL_ARRAY_BUFFER, lightConeVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 lightConeVertices.size() * sizeof(LightConeVertex),
                 lightConeVertices.data(), GL_DYNAMIC_DRAW);

    // Update index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lightConeEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 lightConeIndices.size() * sizeof(unsigned int),
                 lightConeIndices.data(), GL_DYNAMIC_DRAW);

    glBindVertexArray(lightConeVao);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                        sizeof(LightConeVertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute (overlay shader expects aColor at location 1)
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
                        sizeof(LightConeVertex), (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);

    // Render as wireframe lines
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(lightConeIndices.size()),
                  GL_UNSIGNED_INT, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindVertexArray(0);
    glUseProgram(0);
}

// ============================================================================
// Private helper implementations
// ============================================================================

void CurvatureRenderer::initializeGrid()
{
    vertices.clear();
    indices.clear();
    metricCache.clear();
    curvatureCache.clear();
    m_baseZ.clear();

    float halfSize = gridSize / 2.0f;

    if (m_planeMode) {
        // 2D plane mode: rubber sheet at Z=0 with higher resolution
        int res = m_planeResolution;
        float step = gridSize / (res - 1);

        for (int i = 0; i < res; i++) {
            for (int j = 0; j < res; j++) {
                CurvatureVertex vertex;
                vertex.position[0] = -halfSize + i * step;
                vertex.position[1] = -halfSize + j * step;
                vertex.position[2] = 0.0f;
                vertex.normal[0] = 0.0f;
                vertex.normal[1] = 0.0f;
                vertex.normal[2] = 1.0f;
                vertex.color[0] = 0.2f;
                vertex.color[1] = 0.2f;
                vertex.color[2] = 0.8f;
                vertex.color[3] = 0.8f;
                vertex.curvature = 0.0f;
                vertex.time_dilation = 1.0f;
                vertices.push_back(vertex);

                metricCache.push_back(0.0);
                curvatureCache.push_back(0.0);
                m_baseZ.push_back(0.0f);
            }
        }

        if (wireframe) {
            // Line indices for wireframe grid (horizontal and vertical lines)
            for (int i = 0; i < res; i++) {
                for (int j = 0; j < res - 1; j++) {
                    // Horizontal line
                    indices.push_back(i * res + j);
                    indices.push_back(i * res + j + 1);
                }
            }
            for (int j = 0; j < res; j++) {
                for (int i = 0; i < res - 1; i++) {
                    // Vertical line
                    indices.push_back(i * res + j);
                    indices.push_back((i + 1) * res + j);
                }
            }
        } else {
            // Triangle indices for solid grid (2 triangles per cell)
            for (int i = 0; i < res - 1; i++) {
                for (int j = 0; j < res - 1; j++) {
                    int base = i * res + j;
                    int baseX = base + 1;
                    int baseY = base + res;
                    int baseXY = base + res + 1;

                    // First triangle
                    indices.push_back(base);
                    indices.push_back(baseX);
                    indices.push_back(baseY);

                    // Second triangle
                    indices.push_back(baseX);
                    indices.push_back(baseXY);
                    indices.push_back(baseY);
                }
            }
        }
    } else {
        // 3D cube mode: original grid
        float step = gridSize / (gridResolution - 1);

        for (int i = 0; i < gridResolution; i++) {
            for (int j = 0; j < gridResolution; j++) {
                for (int k = 0; k < gridResolution; k++) {
                    CurvatureVertex vertex;
                    vertex.position[0] = -halfSize + i * step;
                    vertex.position[1] = -halfSize + j * step;
                    vertex.position[2] = -halfSize + k * step;
                    vertex.normal[0] = 0.0f;
                    vertex.normal[1] = 0.0f;
                    vertex.normal[2] = 1.0f;
                    vertex.color[0] = 0.2f;
                    vertex.color[1] = 0.2f;
                    vertex.color[2] = 0.8f;
                    vertex.color[3] = 0.8f;
                    vertex.curvature = 0.0f;
                    vertex.time_dilation = 1.0f;
                    vertices.push_back(vertex);

                    metricCache.push_back(0.0);
                    curvatureCache.push_back(0.0);
                    m_baseZ.push_back(vertex.position[2]);
                }
            }
        }

        // Create triangle indices for a proper 3D grid mesh
        // Each cell is a cube made of 12 triangles (2 per face)
        for (int i = 0; i < gridResolution - 1; i++) {
            for (int j = 0; j < gridResolution - 1; j++) {
                for (int k = 0; k < gridResolution - 1; k++) {
                    int base = i * gridResolution * gridResolution + j * gridResolution + k;
                int baseX = base + 1;
                int baseY = base + gridResolution;
                int baseZ = base + gridResolution * gridResolution;
                int baseXY = base + gridResolution + 1;
                int baseXZ = base + gridResolution * gridResolution + 1;
                int baseYZ = base + gridResolution * gridResolution + gridResolution;
                int baseXYZ = base + gridResolution * gridResolution + gridResolution + 1;
                
                // XY plane quad (2 triangles)
                indices.push_back(base);
                indices.push_back(baseX);
                indices.push_back(baseY);
                indices.push_back(baseX);
                indices.push_back(baseXY);
                indices.push_back(baseY);
                
                // XZ plane quad (2 triangles)
                indices.push_back(base);
                indices.push_back(baseX);
                indices.push_back(baseZ);
                indices.push_back(baseX);
                indices.push_back(baseXZ);
                indices.push_back(baseZ);
                
                // YZ plane quad (2 triangles)
                indices.push_back(base);
                indices.push_back(baseY);
                indices.push_back(baseZ);
                indices.push_back(baseY);
                indices.push_back(baseYZ);
                indices.push_back(baseZ);
                
                // Back face (2 triangles)
                indices.push_back(baseX);
                indices.push_back(baseXY);
                indices.push_back(baseXZ);
                indices.push_back(baseXY);
                indices.push_back(baseXYZ);
                indices.push_back(baseXZ);
                
                // Top face (2 triangles)
                indices.push_back(baseY);
                indices.push_back(baseXY);
                indices.push_back(baseYZ);
                indices.push_back(baseXY);
                indices.push_back(baseXYZ);
                indices.push_back(baseYZ);
                
                // Right face (2 triangles)
                indices.push_back(baseZ);
                indices.push_back(baseXZ);
                indices.push_back(baseYZ);
                indices.push_back(baseXZ);
                indices.push_back(baseXYZ);
                indices.push_back(baseYZ);
            }
        }
    }
    }

    cacheValid = false;
}

// ============================================================================
// Private helper implementations
// ============================================================================

void CurvatureRenderer::deformGrid()
{
    if (!m_initialized) return;
    
    // Use CurvatureCalculator to compute curvature at each vertex
    CurvatureCalculator calculator(currentMetric);
    
    for (size_t i = 0; i < vertices.size(); i++) {
        auto& vertex = vertices[i];
        
        // Create event at this vertex position
        Event4D event(time, vertex.position[0], vertex.position[1], vertex.position[2]);
        
        // Compute curvature using the calculator
        calculator.computeKretschmann(event);
        double curvature = calculator.getKretschmann();
        vertex.curvature = static_cast<float>(curvature);
        
        // Apply deformation based on curvature.
        // Clamp the displacement to avoid the 1/r^6 Kretschmann blow-up
        // near the singularity producing an enormous spike.
        float rawDisplacement = static_cast<float>(curvature * 0.1);
        float displacement = rawDisplacement > 8.0f ? 8.0f
                          : (rawDisplacement < -8.0f ? -8.0f : rawDisplacement);
        // Apply displacement relative to the undeformed base Z so repeated
        // calls (e.g. every animation frame via updateTime) do not accumulate
        // and drift the grid unboundedly.
        vertex.position[2] = m_baseZ[i] + displacement;
        
        // Update time dilation
        calculator.computeRicciScalar(event);
        vertex.time_dilation = static_cast<float>(calculator.getRicciScalar());
        
        // Update color
        updateVertexColor(vertex);
    }
}

void CurvatureRenderer::updateVertexColor(CurvatureVertex& vertex)
{
    // Color based on curvature mode
    switch (mode) {
        case CurvatureMode::GRID_DEFORMATION: {
            // Color based on displacement magnitude
            float intensity = std::abs(vertex.position[2]) / (gridSize * 0.5f);
            intensity = std::min(intensity, 1.0f);
            vertex.color[0] = intensity;
            vertex.color[1] = 0.2f;
            vertex.color[2] = 1.0f - intensity;
            break;
        }
        case CurvatureMode::RIEMANN_COLOR: {
            // Color based on curvature value
            float K = vertex.curvature;
            float logK = std::log10(std::max(std::abs(K), 1e-10f));
            float hue = (logK + 10.0f) / 20.0f;
            hue = std::max(0.0f, std::min(1.0f, hue));
            
            if (hue < 0.25f) {
                vertex.color[0] = 0.0f;
                vertex.color[1] = hue * 4.0f;
                vertex.color[2] = 1.0f;
            } else if (hue < 0.5f) {
                vertex.color[0] = 0.0f;
                vertex.color[1] = 1.0f;
                vertex.color[2] = (0.5f - hue) * 4.0f;
            } else if (hue < 0.75f) {
                vertex.color[0] = (hue - 0.5f) * 4.0f;
                vertex.color[1] = 1.0f;
                vertex.color[2] = 0.0f;
            } else {
                vertex.color[0] = 1.0f;
                vertex.color[1] = (1.0f - hue) * 4.0f;
                vertex.color[2] = 0.0f;
            }
            break;
        }
        case CurvatureMode::CURVATURE_SCALAR: {
            // Heat map of curvature intensity
            float intensity = std::min(std::abs(vertex.curvature) * 0.1f, 1.0f);
            vertex.color[0] = intensity;
            vertex.color[1] = 1.0f - intensity;
            vertex.color[2] = 0.0f;
            break;
        }
        case CurvatureMode::GEODESIC_FLOW: {
            // Color based on time dilation
            float d = std::min(std::abs(vertex.time_dilation), 1.0f);
            vertex.color[0] = d;
            vertex.color[1] = 0.2f;
            vertex.color[2] = 1.0f - d;
            break;
        }
        case CurvatureMode::TIME_DILATION: {
            // Color based on time dilation
            float d = std::min(vertex.time_dilation, 1.0f);
            if (d < 0.5f) {
                vertex.color[0] = 1.0f;
                vertex.color[1] = d * 2.0f;
                vertex.color[2] = 0.0f;
            } else {
                vertex.color[0] = (1.0f - d) * 2.0f;
                vertex.color[1] = 1.0f;
                vertex.color[2] = (d - 0.5f) * 2.0f;
            }
            break;
        }
    }
    vertex.color[3] = 0.8f;
}

double CurvatureRenderer::computeKretschmannScalar(const Event4D& event) const
{
    if (!currentMetric) return 0.0;
    
    CurvatureCalculator calculator(currentMetric);
    calculator.computeKretschmann(event);
    return calculator.getKretschmann();
}

double CurvatureRenderer::computeRicciScalar(const Event4D& event) const
{
    if (!currentMetric) return 0.0;
    
    CurvatureCalculator calculator(currentMetric);
    calculator.computeRicciScalar(event);
    return calculator.getRicciScalar();
}

double CurvatureRenderer::computeRiemannComponent(int rho, int sigma, int mu, int nu, const Event4D& event) const
{
    if (!currentMetric) return 0.0;
    if (rho < 0 || rho > 3 || sigma < 0 || sigma > 3 || mu < 0 || mu > 3 || nu < 0 || nu > 3) return 0.0;
    
    CurvatureCalculator calculator(currentMetric);
    calculator.computeRiemann(event);
    return calculator.getRiemann()[rho][sigma][mu][nu];
}

void CurvatureRenderer::updateLodLevel()
{
    // LOD is updated based on camera distance
    // This is a placeholder - full implementation would use view matrix
}

void CurvatureRenderer::updateVerticesByLod()
{
    // Update only vertices in visible cells
    for (auto& cell : gridCells) {
        if (cell.needsUpdate) {
            for (int i = cell.startIndex; i <= cell.endIndex; i++) {
                if (i < static_cast<int>(vertices.size())) {
                    updateSingleVertex(i);
                }
            }
            // Mark as updated
            cell.needsUpdate = false;
        }
    }
}

void CurvatureRenderer::updateAllVertices()
{
    for (auto& vertex : vertices) {
        updateVertexColor(vertex);
    }
}

void CurvatureRenderer::updateCellVertices(int cellIndex)
{
    if (cellIndex < 0 || cellIndex >= static_cast<int>(gridCells.size())) return;
    
    const auto& cell = gridCells[cellIndex];
    for (int i = cell.startIndex; i <= cell.endIndex; i++) {
        if (i < static_cast<int>(vertices.size())) {
            updateSingleVertex(i);
        }
    }
}

void CurvatureRenderer::updateSingleVertex(int vertexIndex)
{
    if (vertexIndex < 0 || vertexIndex >= static_cast<int>(vertices.size())) return;
    
    auto& vertex = vertices[vertexIndex];
    Event4D event(time, vertex.position[0], vertex.position[1], vertex.position[2]);
    
    if (currentMetric) {
        vertex.curvature = static_cast<float>(computeKretschmannScalar(event));
        vertex.time_dilation = static_cast<float>(computeRicciScalar(event));
    }
    
    updateVertexColor(vertex);
}

} // namespace quantumverse