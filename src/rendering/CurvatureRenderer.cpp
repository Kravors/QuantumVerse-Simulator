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
#include <cstdlib>
#include <QVector4D>
#include <QDebug>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#ifdef _WIN32
#include <windows.h>  // Required for GetEnvironmentVariableA in isTruthyEnvironmentFlag
#endif

namespace quantumverse {

// Forward declaration for environment variable check
static bool isTruthyEnvironmentFlag(const char* name);

// Load shader source from file
static std::string loadShaderFromFile(const std::string& path);

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
     m_planeMode(true), m_planeResolution(500),
     m_deformationScale(1.0f),
     gridSSBO(0), gridWidth(500), gridHeight(500),
     computeBuffersInitialized(false), m_computeDirty(false),
     m_gridZMin(0.0f), m_gridZMax(0.0f),
     m_lastEpoch(0),
     neuralODE(nullptr), neuralPool(nullptr),
     neuralGeoVao(0), neuralGeoVbo(0), neuralGeoVertexCount(0),
     neuralGeoBuffersInitialized(false), m_showNeuralGeodesics(false)
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
        layout(location = 0) in vec4 aPosSSBO;   // x, y, z, displacement (from compute shader SSBO)
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec4 aColor;
        layout(location = 3) in float aCurvature;
        layout(location = 4) in float aTimeDilation;

        uniform mat4 viewMatrix;
        uniform mat4 projectionMatrix;
        uniform float time;
        uniform float curvatureScale;
        uniform int curvatureMode;
        uniform int useSSBO;  // 1 = read position from SSBO (zero-copy path)

        out vec4 fragColor;
        out float curvatureValue;
        out float timeDilation;

        void main() {
            vec3 displacedPos;
            if (useSSBO == 1) {
                // Zero-copy path: position comes from compute shader SSBO
                // w component holds the displacement value from compute shader
                float dz = aPosSSBO.w * curvatureScale;
                displacedPos = vec3(aPosSSBO.x, aPosSSBO.y, aPosSSBO.z + dz);
            } else {
                displacedPos = aPos;
            }

            if (curvatureMode == 0) {
                gl_Position = projectionMatrix * viewMatrix * vec4(displacedPos, 1.0);
            } else if (curvatureMode == 3) {
                float r = length(displacedPos.xy);
                float depression = 15.0 / (1.0 + r * 0.03);
                displacedPos.z -= depression;
                gl_Position = projectionMatrix * viewMatrix * vec4(displacedPos, 1.0);
            } else {
                gl_Position = projectionMatrix * viewMatrix * vec4(displacedPos, 1.0);
            }

            // Compute color from displacement magnitude for SSBO path
            if (useSSBO == 1) {
                float dispMag = abs(aPosSSBO.w);
                float intensity = clamp(dispMag * 2.0, 0.0, 1.0);
                fragColor = vec4(intensity, 0.2, 1.0 - intensity, 0.8);
            } else {
                fragColor = aColor;
            }
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
        uniform float u_global_alpha;
        out vec4 outColor;

        void main() {
            vec3 baseColor = fragColor.rgb;
            float alpha = fragColor.a * u_global_alpha;

            switch (curvatureMode) {
                case 0: {
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
                    alpha = 0.7;
                    break;
                }
                case 2: {
                    float intensity = clamp(log2(max(curvatureValue, 1.0)) * 0.05, 0.0, 1.0);
                    baseColor = mix(vec3(0.15, 0.35, 1.0), vec3(1.0, 0.1, 0.1), intensity);
                    alpha = 0.7;
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

    // Ergosphere shader: semi-transparent glowing oblate spheroid
    const char* ergoVert = R"(
        #version 450 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;

        uniform mat4 viewMatrix;
        uniform mat4 projectionMatrix;
        uniform float ergoMass;
        uniform float ergoSpin;
        uniform float ergoCharge;

        out vec3 vNormal;
        out float vLatitude;

        void main() {
            vec3 n = normalize(aNormal);
            float theta = acos(clamp(n.y, -1.0, 1.0)); // latitude from +Y pole
            float cosTheta = cos(theta);

            // Ergosphere radius: r_E(theta) = M + sqrt(M^2 - a^2 cos^2(theta))
            float a2 = ergoSpin * ergoSpin;
            float m2 = ergoMass * ergoMass;
            float disc = m2 - a2 * cosTheta * cosTheta;
            float rE = ergoMass + sqrt(max(0.0, disc));

            // Scale the unit sphere position by rE / M
            float scale = rE / ergoMass;
            vec3 displacedPos = aPos * scale;

            gl_Position = projectionMatrix * viewMatrix * vec4(displacedPos, 1.0);
            vNormal = n;
            vLatitude = theta;
        }
    )";

    const char* ergoFrag = R"(
        #version 450 core
        in vec3 vNormal;
        in float vLatitude;
        uniform float ergoMass;
        uniform float ergoSpin;
        out vec4 outColor;

        void main() {
            // Glowing cyan/purple ergosphere
            float lat = vLatitude / 3.141592653589793; // 0 to 1
            vec3 baseColor = mix(vec3(0.2, 0.6, 1.0), vec3(0.6, 0.2, 1.0), lat);
            float glow = 0.6 + 0.3 * sin(vLatitude * 3.0);
            float alpha = 0.4 + 0.2 * abs(dot(vNormal, vec3(0.0, 1.0, 0.0)));
            outColor = vec4(baseColor * glow, alpha);
        }
    )";

    compileShader(gridShader.id, gridVert, gridFrag);
    compileShader(curvatureShader.id, gridVert, gridFrag);
    compileShader(geodesicShader.id, geodesicVert, geodesicFrag);
    compileShader(overlayShader.id, overlayVert, overlayFrag);
    compileShader(ergoShader.id, ergoVert, ergoFrag);

    // Compile fallback shader
    compileShader(fallbackShaderId, fallbackVert, fallbackFrag);

    // Initialize light cone buffers (also requires GL context)
    initializeLightConeBuffers();

    // Initialize geodesic line buffers
    glGenVertexArrays(1, &geodesicVao);
    glGenBuffers(1, &geodesicVbo);
    geodesicBuffersInitialized = true;

    // Initialize ergosphere buffers
    glGenVertexArrays(1, &ergoVao);
    glGenBuffers(1, &ergoVbo);
    glGenBuffers(1, &ergoEbo);
    ergoBuffersInitialized = false;
    ergoVertexCount = 0;

    // Initialize compute shader for grid deformation
    std::string computeSrc = loadShaderFromFile("shaders/curvature_compute.comp");
    if (!computeSrc.empty()) {
        const char* computeSource = computeSrc.c_str();
        unsigned int computeShaderId = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(computeShaderId, 1, &computeSource, nullptr);
        glCompileShader(computeShaderId);

        int success;
        glGetShaderiv(computeShaderId, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(computeShaderId, 512, nullptr, infoLog);
            std::cerr << "[CurvatureRenderer] Compute shader compilation failed:\n" << infoLog << std::endl;
        } else {
            computeShader.id = glCreateProgram();
            glAttachShader(computeShader.id, computeShaderId);
            glLinkProgram(computeShader.id);
            glGetProgramiv(computeShader.id, GL_LINK_STATUS, &success);
            if (!success) {
                char infoLog[512];
                glGetProgramInfoLog(computeShader.id, 512, nullptr, infoLog);
                std::cerr << "[CurvatureRenderer] Compute program linking failed:\n" << infoLog << std::endl;
            }
            glDeleteShader(computeShaderId);
            std::cout << "[CurvatureRenderer] Compute shader compiled successfully (id=" << computeShader.id << ")" << std::endl;
        }
    } else {
        std::cerr << "[CurvatureRenderer] WARNING: Could not load curvature_compute.comp" << std::endl;
    }

    // Initialize grid SSBO for compute shader
    glGenBuffers(1, &gridSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gridSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, vertices.size() * sizeof(float) * 4, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    computeBuffersInitialized = true;

    // Initialize Neural ODE for geodesic tracing
    neuralODE = std::make_unique<quantumverse::ml::GeodesicNeuralODE>(1);
    neuralPool = std::make_unique<utils::ThreadPool>(4);
    if (neuralODE->loadONNXModel("rl_discovery_agent.onnx")) {
        std::cout << "[CurvatureRenderer] Neural ODE model loaded for geodesic tracing" << std::endl;
    } else {
        std::cerr << "[CurvatureRenderer] WARNING: Could not load Neural ODE model for geodesics" << std::endl;
    }

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

// Load shader source from file
static std::string loadShaderFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[CurvatureRenderer] Failed to open shader file: " << path << std::endl;
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
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
    shader->setUniform("useSSBO", 0);
    shader->setUniform("u_global_alpha", m_globalAlpha);

    if (currentMetric) {
        // Re-deform grid when the metric's epoch changes (e.g. mass sweep).
        // This keeps grid Z deformation in sync with in-place metric mutation
        // without relying solely on m_metricDirty.
        if (currentMetric->epoch() != m_lastEpoch) {
            recomputeGridZRange();
            m_lastEpoch = currentMetric->epoch();
        }

        if (computeShader.id != 0 && computeBuffersInitialized) {
            deformGridGPU();
        } else {
            // Fall back to CPU deformation using the actual metric when the
            // compute shader is unavailable (e.g. shader file not found).
            deformGrid();
        }
        updateGLBuffers();
    }

    // Zero-copy rendering: bind SSBO directly as VBO when compute shader updated it
    if (m_computeDirty && computeBuffersInitialized && computeShader.id != 0) {
        // Bind VAO first (required for vertex attribute state)
        glBindVertexArray(vao);
        
        // Bind SSBO as the vertex buffer source for position attribute
        glBindBuffer(GL_ARRAY_BUFFER, gridSSBO);
        
        // Position attribute: read vec4(x, y, z, displacement) from SSBO
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Disable other attributes (color computed in fragment shader from displacement)
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(3);
        glDisableVertexAttribArray(4);

    shader->use();
    shader->setUniform("viewMatrix", viewMatrix);
    shader->setUniform("projectionMatrix", projectionMatrix);
    shader->setUniform("time", time);
    shader->setUniform("curvatureScale", m_deformationScale);
    shader->setUniform("curvatureMode", static_cast<int>(mode));
    shader->setUniform("useSSBO", 1);
    shader->setUniform("u_global_alpha", m_globalAlpha);

        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDrawElements(wireframe ? GL_LINES : GL_TRIANGLES,
                       static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
        glDisable(GL_DEPTH_TEST);
        
        if (wireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        shader->release();
        m_computeDirty = false;
    } else {
        glBindVertexArray(vao);
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDrawElements(wireframe ? GL_LINES : GL_TRIANGLES,
                       static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
        glDisable(GL_DEPTH_TEST);
        glBindVertexArray(0);
        if (wireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    shader->release();

    // [DIAG] Restore the FBO that was bound before we started drawing.
    // This ensures the QML FBO remains bound for screenshot capture.
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(oldFBO));
    if (isTruthyEnvironmentFlag("QUANTUMVERSE_FBO_CHECK")) {
        std::fprintf(stderr, "[CurvatureRenderer] FBO restored to: %d\n", oldFBO);
    }

    if (showLightCones)
        renderLightCones(viewMatrix, projectionMatrix);

    if (m_showErgosphere)
        renderErgosphere(viewMatrix, projectionMatrix);

    if (m_showNeuralGeodesics)
        renderNeuralGeodesics(viewMatrix, projectionMatrix);
}

void CurvatureRenderer::renderErgosphere(const float* viewMatrix, const float* projectionMatrix)
{
    if (!m_initialized || !m_showErgosphere) return;

    // (Re)generate ergosphere mesh if spin/mass changed
    if (!ergoBuffersInitialized || m_ergoSpin != m_lastErgoSpin || m_ergoMass != m_lastErgoMass) {
        generateErgosphereMesh();
        m_lastErgoSpin = m_ergoSpin;
        m_lastErgoMass = m_ergoMass;
    }

    if (ergoVertexCount == 0) return;

    ergoShader.use();
    ergoShader.setUniform("viewMatrix", viewMatrix);
    ergoShader.setUniform("projectionMatrix", projectionMatrix);
    ergoShader.setUniform("ergoMass", static_cast<float>(m_ergoMass));
    ergoShader.setUniform("ergoSpin", static_cast<float>(m_ergoSpin));
    ergoShader.setUniform("ergoCharge", static_cast<float>(m_ergoCharge));

    glBindVertexArray(ergoVao);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // Don't write depth so horizon shows through

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(ergoVertexCount), GL_UNSIGNED_INT, 0);

    glDepthMask(GL_TRUE);
    glBindVertexArray(0);
    ergoShader.release();
}

void CurvatureRenderer::generateErgosphereMesh()
{
    // Clean up old buffers
    if (ergoBuffersInitialized) {
        glDeleteBuffers(1, &ergoVbo);
        glDeleteBuffers(1, &ergoEbo);
    }

    const int latRes = 32;
    const int lonRes = 64;
    const double M = m_ergoMass;
    const double a = m_ergoSpin;

    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= latRes; ++i) {
        double theta = 3.141592653589793 * i / latRes; // 0 to pi
        double cosT = cos(theta);
        double sinT = sin(theta);

        // Ergosphere radius: r_E(theta) = M + sqrt(M^2 - a^2 cos^2(theta))
        double disc = M * M - a * a * cosT * cosT;
        double rE = M + sqrt((disc > 0.0) ? disc : 0.0);

        for (int j = 0; j <= lonRes; ++j) {
            double phi = 2.0 * 3.141592653589793 * j / lonRes; // 0 to 2pi

            // Spherical coordinates: x = r sinθ cosφ, y = r cosθ, z = r sinθ sinφ
            // Note: Y is up in our coordinate system
            double x = rE * sinT * cos(phi);
            double y = rE * cosT;
            double z = rE * sinT * sin(phi);

            positions.push_back(static_cast<float>(x));
            positions.push_back(static_cast<float>(y));
            positions.push_back(static_cast<float>(z));

            // Normal is radial from origin (approximate for ergosphere)
            double len = sqrt(x*x + y*y + z*z);
            normals.push_back(static_cast<float>(x / len));
            normals.push_back(static_cast<float>(y / len));
            normals.push_back(static_cast<float>(z / len));
        }
    }

    // Build triangle strip indices
    for (int i = 0; i < latRes; ++i) {
        for (int j = 0; j <= lonRes; ++j) {
            indices.push_back(i * (lonRes + 1) + j);
            indices.push_back((i + 1) * (lonRes + 1) + j);
        }
        // Degenerate triangle to restart strip
        if (i < latRes - 1) {
            indices.push_back((i + 1) * (lonRes + 1) + lonRes);
            indices.push_back((i + 1) * (lonRes + 1));
        }
    }

    ergoVertexCount = static_cast<unsigned int>(indices.size());

    glGenVertexArrays(1, &ergoVao);
    glBindVertexArray(ergoVao);

    glGenBuffers(1, &ergoVbo);
    glBindBuffer(GL_ARRAY_BUFFER, ergoVbo);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), positions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &ergoEbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ergoEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
    ergoBuffersInitialized = true;

    std::cout << "[CurvatureRenderer] Ergosphere mesh generated: "
              << ergoVertexCount << " indices, M=" << M << ", a=" << a << std::endl;
}

void CurvatureRenderer::setShowNeuralGeodesics(bool show)
{
    m_showNeuralGeodesics = show;
}

bool CurvatureRenderer::loadNeuralODEModel(const std::string& onnxPath)
{
    if (!neuralODE) return false;
    return neuralODE->loadONNXModel(onnxPath);
}

void CurvatureRenderer::renderNeuralGeodesics(const float* viewMatrix, const float* projectionMatrix)
{
    if (!m_initialized || !m_showNeuralGeodesics || !neuralODE || !neuralPool) return;

    // Generate photon bundle: grid of starting positions far from black hole
    const int gridSize = 16; // 16x16 = 256 rays
    const float startZ = -60.0f;
    const float span = 20.0f;

    static std::vector<Event4D> initialEvents;
    static std::vector<std::array<double,4>> initialVelocities;
    static bool eventsGenerated = false;

    if (!eventsGenerated) {
        for (int i = 0; i < gridSize; i++) {
            for (int j = 0; j < gridSize; j++) {
                float x = -span/2.0f + (float)i * span / (gridSize - 1);
                float y = -span/2.0f + (float)j * span / (gridSize - 1);
                initialEvents.emplace_back(0.0, x, y, startZ);
                // Null-like velocity pointing toward +Z (and slightly toward center)
                initialVelocities.push_back({1.0, 0.0, 0.0, 1.0});
            }
        }
        eventsGenerated = true;
    }

    // Trace trajectories by stepping the Neural ODE
    const int numSteps = 8;
    const double tau = 0.5;
    const double rs = 2.95; // Schwarzschild radius for visualization scale
    const double mMass = m_ergoMass;

    // Current states (start from initial events)
    static std::vector<Event4D> currentStates = initialEvents;
    static bool firstStep = true;

    if (firstStep) {
        currentStates = initialEvents;
        firstStep = false;
    }

    // Store all trajectory points for rendering
    static std::vector<std::vector<Event4D>> trajectories(initialEvents.size());
    if (firstStep) {
        for (size_t i = 0; i < initialEvents.size(); i++) {
            trajectories[i].clear();
            trajectories[i].push_back(initialEvents[i]);
        }
        firstStep = false;
    }

    // Step through the Neural ODE
    std::vector<double> metricParams = {mMass, 0.0}; // mass, spin
    for (int step = 0; step < numSteps; step++) {
        auto nextStates = neuralODE->predictBundle(currentStates, initialVelocities, metricParams, tau, *neuralPool);
        
        bool allEscaped = true;
        for (size_t i = 0; i < nextStates.size(); i++) {
            if (nextStates[i].empty()) continue;
            const auto& finalEvent = nextStates[i].back();
            
            // Check if photon fell into black hole
            double r = sqrt(finalEvent.x*finalEvent.x + finalEvent.y*finalEvent.y + finalEvent.z*finalEvent.z);
            if (r > rs * 1.01 && r < 100.0) {
                currentStates[i] = finalEvent;
                trajectories[i].push_back(finalEvent);
                allEscaped = false;
            }
            // If r <= rs*1.01, photon hit horizon - stop tracing it
            // If r > 100, photon escaped to infinity - stop tracing it
        }

        if (allEscaped) break;
    }

    // Upload trajectories to GPU and render
    // Build a single VBO with all line strips
    std::vector<float> allVertexData;
    std::vector<unsigned int> allIndices;
    unsigned int currentOffset = 0;

    for (const auto& traj : trajectories) {
        if (traj.size() < 2) continue;

        for (const auto& pt : traj) {
            allVertexData.push_back(static_cast<float>(pt.x));
            allVertexData.push_back(static_cast<float>(pt.y));
            allVertexData.push_back(static_cast<float>(pt.z));
            allVertexData.push_back(0.0f);   // R (cyan)
            allVertexData.push_back(0.9f);   // G
            allVertexData.push_back(1.0f);   // B
            allVertexData.push_back(0.6f);   // A
        }

        // Indices for this line strip
        for (size_t k = 0; k < traj.size(); k++) {
            allIndices.push_back(currentOffset + k);
        }
        currentOffset += static_cast<unsigned int>(traj.size());
    }

    if (allVertexData.empty()) return;

    // Initialize neural geodesic buffers if needed
    if (!neuralGeoBuffersInitialized) {
        glGenVertexArrays(1, &neuralGeoVao);
        glGenBuffers(1, &neuralGeoVbo);
        neuralGeoBuffersInitialized = true;
    }

    glBindVertexArray(neuralGeoVao);
    glBindBuffer(GL_ARRAY_BUFFER, neuralGeoVbo);
    glBufferData(GL_ARRAY_BUFFER, allVertexData.size() * sizeof(float),
                 allVertexData.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    neuralGeoVertexCount = static_cast<unsigned int>(allIndices.size());

    // Render with additive blending for glowing effect
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(allVertexData.size() / 7));

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

void CurvatureRenderer::renderGeodesics(const std::vector<std::vector<Event4D>>& geodesics,
                                        const float* viewMatrix, const float* projectionMatrix)
{
    if (!m_initialized || !geodesicBuffersInitialized) return;

    geodesicShader.use();
    geodesicShader.setUniform("viewMatrix", viewMatrix);
    geodesicShader.setUniform("projectionMatrix", projectionMatrix);

    glBindVertexArray(geodesicVao);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (const auto& geodesic : geodesics) {
        if (geodesic.size() < 2) continue;

        std::vector<float> vertexData;
        vertexData.reserve(geodesic.size() * 7);
        for (const auto& pt : geodesic) {
            vertexData.push_back(static_cast<float>(pt.x));
            vertexData.push_back(static_cast<float>(pt.y));
            vertexData.push_back(static_cast<float>(pt.z));
            vertexData.push_back(0.0f);   // R
            vertexData.push_back(0.9f);   // G
            vertexData.push_back(0.2f);   // B
            vertexData.push_back(1.0f);   // A
        }

        glBindBuffer(GL_ARRAY_BUFFER, geodesicVbo);
        glBufferData(GL_ARRAY_BUFFER,
                     vertexData.size() * sizeof(float),
                     vertexData.data(),
                     GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(geodesic.size()));
    }

    glDisable(GL_BLEND);
    glBindVertexArray(0);
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
     m_metricDirty = true;
}

void CurvatureRenderer::setLightCone(std::shared_ptr<class LightCone> cone)
{
     lightCone = std::move(cone);
}

void CurvatureRenderer::addSingularity(std::shared_ptr<class SingularityHandler> singularity)
{
    singularities.push_back(singularity);
    m_metricDirty = true;
}

void CurvatureRenderer::removeSingularity(std::shared_ptr<class SingularityHandler> singularity)
{
    auto it = std::find(singularities.begin(), singularities.end(), singularity);
    if (it != singularities.end()) {
        singularities.erase(it);
        m_metricDirty = true;
    }
}

void CurvatureRenderer::setErgosphereSpin(double spin)
{
    m_ergoSpin = std::clamp(spin, 0.0, 0.999);
}

void CurvatureRenderer::setErgosphereMass(double mass)
{
    m_ergoMass = std::max(mass, 0.1);
}

void CurvatureRenderer::setErgosphereCharge(double charge)
{
    m_ergoCharge = std::max(charge, 0.0);
}

void CurvatureRenderer::setShowErgosphere(bool show)
{
    m_showErgosphere = show;
}

void CurvatureRenderer::setGlobalAlpha(float alpha)
{
    m_globalAlpha = std::clamp(alpha, 0.0f, 1.0f);
}

void CurvatureRenderer::updateTime(float deltaTime)
{
     time += deltaTime * animationSpeed;
     // Time does not affect the static Schwarzschild spatial metric, so no
     // re-deformation is needed here. The render() loop handles dirty flags.
}

void CurvatureRenderer::regenerateGrid()
{
     initializeGrid();
     m_metricDirty = true;
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
    m_metricDirty = true;
    std::cout << "[CurvatureRenderer] setPlaneMode done, vertices=" << vertices.size() << ", indices=" << indices.size() << std::endl;
}

void CurvatureRenderer::setPlaneResolution(int resolution)
{
    if (resolution < 4) resolution = 4;
    if (resolution > 500) resolution = 500;
    if (m_planeResolution == resolution) return;
    m_planeResolution = resolution;
    gridWidth = resolution;
    gridHeight = resolution;
    if (m_planeMode) {
        initializeGrid();
        m_metricDirty = true;
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
    if (vertices.empty()) return;

    float cx = m_blackHoleCenter[0];
    float cy = m_blackHoleCenter[1];

    bool haveMetric = (currentMetric != nullptr);

    float rawZMin = 0.0f;
    float rawZMax = 0.0f;
    bool first = true;

    for (size_t i = 0; i < vertices.size(); i++) {
        auto& vertex = vertices[i];

        float dx = vertex.position[0] - cx;
        float dy = vertex.position[1] - cy;
        float r = std::sqrt(dx * dx + dy * dy);

        float rawDisplacement = 0.0f;

        if (haveMetric && r > 1e-6f) {
            // Use the actual metric to compute gravitational time dilation.
            // Schwarzschild g_tt = -(1 - rs/r); time dilation = sqrt(-g_tt).
            // Well depth = 1 - timeDilation: 0 at infinity, approaches 1 at horizon.
            Event4D event(0.0, dx, dy, 0.0);
            auto g = currentMetric->evaluate(event);
            double gtt = g[0][0];
            double timeDilation = 1.0;
            if (gtt < 0.0 && std::isfinite(gtt)) {
                timeDilation = std::sqrt(std::max(0.0, -gtt));
                rawDisplacement = static_cast<float>(-(1.0 - timeDilation));
            } else {
                rawDisplacement = -1.0f;
            }

            // Capture exact curvature scalars for vertex coloring.
            auto scalars = currentMetric->curvatureScalars(event);
            if (scalars.valid) {
                vertex.curvature = static_cast<float>(scalars.kretschmann);
                vertex.time_dilation = static_cast<float>(timeDilation);
            }
        } else {
            // Fallback: Newtonian 1/r potential well when no metric is bound.
            const float rs = 2.95f;
            rawDisplacement = -rs / (r + 0.1f);
        }

        vertex.position[2] = rawDisplacement * m_deformationScale;

        if (first) {
            rawZMin = rawZMax = rawDisplacement;
            first = false;
        } else {
            rawZMin = std::min(rawZMin, rawDisplacement);
            rawZMax = std::max(rawZMax, rawDisplacement);
        }

        updateVertexColor(vertex);
    }
    if (std::getenv("GRID_DEBUG")) {
        std::cout << "[GRID] rawZ=" << rawZMin << ".." << rawZMax
                  << " scaledZ=" << (rawZMin * m_deformationScale) << ".." << (rawZMax * m_deformationScale)
                  << " unitRange=" << gridSize
                  << " wellCenter=" << m_blackHoleCenter[0] << "," << m_blackHoleCenter[1]
                  << "," << m_blackHoleCenter[2]
                  << " haveMetric=" << (haveMetric ? 1 : 0)
                  << std::endl;
    }
    m_gridZMin = rawZMin * m_deformationScale;
    m_gridZMax = rawZMax * m_deformationScale;
    m_metricDirty = false;
}

void CurvatureRenderer::deformGridGPU()
{
    if (!computeBuffersInitialized || computeShader.id == 0 || vertices.empty()) return;

    // Upload current vertex positions to SSBO
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gridSSBO);
    
    // Build position array: vec4(x, y, z, displacement)
    std::vector<float> posData;
    posData.reserve(vertices.size() * 4);
    for (const auto& v : vertices) {
        posData.push_back(v.position[0]);
        posData.push_back(v.position[1]);
        posData.push_back(v.position[2]);
        posData.push_back(0.0f); // displacement (will be overwritten by compute shader)
    }
    
    glBufferData(GL_SHADER_STORAGE_BUFFER, posData.size() * sizeof(float),
                 posData.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gridSSBO);

    // Set uniforms
    computeShader.use();

    // Use the metric's mass when available so the GPU well matches the physics metric.
    double massSolar = m_blackHoleMassSolar;
    if (auto schw = std::dynamic_pointer_cast<quantumverse::SchwarzschildMetric>(currentMetric)) {
        massSolar = schw->mass() / 1.98847e30;  // kg -> solar masses
    }
    computeShader.setUniform("u_mass", static_cast<float>(massSolar));

    // Schwarzschild radius: rs = 2*G*M/c^2 (visual scaling for grid units)
    float rs = static_cast<float>(2.0 * massSolar * 0.01f);
    computeShader.setUniform("u_rs", rs);
    computeShader.setUniform("u_width", gridWidth);
    computeShader.setUniform("u_height", gridHeight);

    // Dispatch compute shader
    int groupsX = (gridWidth + 15) / 16;
    int groupsY = (gridHeight + 15) / 16;
    glDispatchCompute(groupsX, groupsY, 1);
    glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    computeShader.release();

    // Read back displacement values from SSBO for gridZRange reporting
    // and to keep CPU-side vertices in sync
    float zMin = 0.0f, zMax = 0.0f;
    bool first = true;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gridSSBO);
    float* mapped = static_cast<float*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
        posData.size() * sizeof(float), GL_MAP_READ_BIT));
    
    if (mapped) {
        for (size_t i = 0; i < vertices.size(); i++) {
            float displacement = mapped[i * 4 + 3]; // w component
            vertices[i].position[2] = displacement * m_deformationScale;
            if (first) {
                zMin = zMax = vertices[i].position[2];
                first = false;
            } else {
                zMin = std::min(zMin, vertices[i].position[2]);
                zMax = std::max(zMax, vertices[i].position[2]);
            }
        }
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        
        // Cache the range for gridZRange()
        m_gridZMin = zMin;
        m_gridZMax = zMax;
    }
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    m_metricDirty = false;
    m_computeDirty = true; // Signal render() to use SSBO as VBO source
}

std::pair<float, float> CurvatureRenderer::gridZRange() const
{
    if (vertices.empty()) return {0.0f, 0.0f};
    return {m_gridZMin, m_gridZMax};
}

void CurvatureRenderer::recomputeGridZRange()
{
    if (vertices.empty()) return;

    if (computeShader.id != 0 && computeBuffersInitialized) {
        deformGridGPU();
    } else {
        deformGrid();
    }

    m_lastEpoch = currentMetric ? currentMetric->epoch() : 0;
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