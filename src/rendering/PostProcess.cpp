/**
 * @file PostProcess.cpp
 * @brief HDR bloom post-processing pipeline implementation
 *
 * Multi-pass FBO-based pipeline:
 *   Scene -> HDR FBO -> Bright-pass -> Gaussian blur (ping-pong) -> Composite to screen
 */

#include "PostProcess.h"

#include <QDebug>
#include <cmath>
#include <cstring>

namespace quantumverse {

namespace {

// Fullscreen quad vertex shader — generates a clip-space quad from gl_VertexID
const char* fullscreenVert = R"(
    #version 450 core
    out vec2 vTexCoord;

    void main() {
        // Generate fullscreen triangle from vertex ID
        vec2 pos = vec2(
            float((gl_VertexID & 1) << 2) - 1.0,
            float((gl_VertexID & 2) << 1) - 1.0
        );
        vTexCoord = pos * 0.5 + 0.5;
        gl_Position = vec4(pos, 0.0, 1.0);
    }
)";

// Bright-pass fragment shader — extract areas above luminance threshold
const char* brightPassFrag = R"(
    #version 450 core
    in vec2 vTexCoord;
    out vec4 fragColor;

    uniform sampler2D uSceneTexture;
    uniform float uThreshold;
    uniform float uSoftKnee;

    void main() {
        vec3 color = texture(uSceneTexture, vTexCoord).rgb;

        // Luminance (Rec. 709)
        float lum = dot(color, vec3(0.2126, 0.7152, 0.0722));

        // Soft knee: smooth transition around threshold
        float soft = lum - uThreshold + uSoftKnee;
        soft = clamp(soft, 0.0, 2.0 * uSoftKnee);
        soft = soft * soft / (4.0 * uSoftKnee + 0.00001);

        float contribution = max(soft, lum - uThreshold);
        contribution /= max(lum, 0.00001);

        fragColor = vec4(color * contribution, 1.0);
    }
)";

// Separable Gaussian blur fragment shader — horizontal or vertical pass
const char* blurFrag = R"(
    #version 450 core
    in vec2 vTexCoord;
    out vec4 fragColor;

    uniform sampler2D uTexture;
    uniform vec2 uDirection;  // (1/width, 0) for horizontal, (0, 1/height) for vertical

    void main() {
        // 9-tap Gaussian weights
        float weights[5];
        weights[0] = 0.227027;
        weights[1] = 0.1945946;
        weights[2] = 0.1216216;
        weights[3] = 0.054054;
        weights[4] = 0.016216;

        vec3 result = texture(uTexture, vTexCoord).rgb * weights[0];

        for (int i = 1; i < 5; ++i) {
            vec2 offset = uDirection * float(i);
            result += texture(uTexture, vTexCoord + offset).rgb * weights[i];
            result += texture(uTexture, vTexCoord - offset).rgb * weights[i];
        }

        fragColor = vec4(result, 1.0);
    }
)";

// Composite fragment shader — tone mapping + bloom composite
const char* compositeFrag = R"(
    #version 450 core
    in vec2 vTexCoord;
    out vec4 fragColor;

    uniform sampler2D uSceneTexture;
    uniform sampler2D uBloomTexture;
    uniform float uBloomIntensity;
    uniform float uExposure;
    uniform int uToneMappingMode;

    // ACES filmic tone mapping
    vec3 acesToneMap(vec3 x) {
        const float a = 2.51;
        const float b = 0.03;
        const float c = 2.43;
        const float d = 0.59;
        const float e = 0.14;
        return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
    }

    // Reinhard tone mapping
    vec3 reinhardToneMap(vec3 x) {
        return x / (1.0 + x);
    }

    // Uncharted 2 tone mapping
    vec3 uncharted2ToneMap(vec3 x) {
        const float A = 0.15;
        const float B = 0.50;
        const float C = 0.10;
        const float D = 0.20;
        const float E = 0.02;
        const float F = 0.30;
        return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
    }

    void main() {
        // Sample HDR scene and bloom
        vec3 scene = texture(uSceneTexture, vTexCoord).rgb;
        vec3 bloom = texture(uBloomTexture, vTexCoord).rgb;

        // Add bloom contribution
        vec3 color = scene + bloom * uBloomIntensity;

        // Apply exposure
        color *= uExposure;

        // Tone mapping
        vec3 mapped;
        if (uToneMappingMode == 1) {
            mapped = reinhardToneMap(color);
        } else if (uToneMappingMode == 2) {
            mapped = uncharted2ToneMap(color) / uncharted2ToneMap(vec3(11.2));
        } else {
            mapped = acesToneMap(color);
        }

        // Gamma correction
        mapped = pow(mapped, vec3(1.0 / 2.2));

        fragColor = vec4(mapped, 1.0);
    }
)";

} // anonymous namespace

PostProcess::PostProcess()
    : m_initialized(false)
    , m_width(0)
    , m_height(0)
    , m_sceneFbo(0)
    , m_sceneTexture(0)
    , m_brightPassFbo(0)
    , m_brightPassTexture(0)
    , m_blurFbo{0, 0}
    , m_blurTexture{0, 0}
    , m_quadVao(0)
    , m_brightPassProgram(0)
    , m_blurProgram(0)
    , m_compositeProgram(0)
    , m_bloomEnabled(true)
    , m_bloomIntensity(0.8f)
    , m_bloomThreshold(1.0f)
    , m_toneMappingMode(0)
    , m_exposure(1.0f)
{
}

PostProcess::~PostProcess()
{
    if (!m_initialized) return;

    // Destroy FBOs
    destroyFbo(m_sceneFbo, m_sceneTexture);
    destroyFbo(m_brightPassFbo, m_brightPassTexture);
    destroyFbo(m_blurFbo[0], m_blurTexture[0]);
    destroyFbo(m_blurFbo[1], m_blurTexture[1]);

    // Destroy quad geometry
    if (m_quadVao) {
        glDeleteVertexArrays(1, &m_quadVao);
        m_quadVao = 0;
    }

    // Destroy shader programs
    if (m_brightPassProgram) {
        glDeleteProgram(m_brightPassProgram);
        m_brightPassProgram = 0;
    }
    if (m_blurProgram) {
        glDeleteProgram(m_blurProgram);
        m_blurProgram = 0;
    }
    if (m_compositeProgram) {
        glDeleteProgram(m_compositeProgram);
        m_compositeProgram = 0;
    }

    m_initialized = false;
}

void PostProcess::initialize(int width, int height)
{
    if (m_initialized) {
        resize(width, height);
        return;
    }

    m_width = width;
    m_height = height;

    // Create fullscreen quad VAO (VBO not needed — vertices generated in shader)
    glGenVertexArrays(1, &m_quadVao);
    glBindVertexArray(m_quadVao);
    glBindVertexArray(0);

    // Compile shader programs
    m_brightPassProgram = compileShaderProgram(fullscreenVert, brightPassFrag);
    m_blurProgram = compileShaderProgram(fullscreenVert, blurFrag);
    m_compositeProgram = compileShaderProgram(fullscreenVert, compositeFrag);

    if (!m_brightPassProgram || !m_blurProgram || !m_compositeProgram) {
        qWarning() << "PostProcess: Failed to compile shader programs";
        return;
    }

    // Create FBOs
    int halfW = std::max(1, width / 2);
    int halfH = std::max(1, height / 2);

    createFbo(m_sceneFbo, m_sceneTexture, width, height, GL_RGBA16F);
    createFbo(m_brightPassFbo, m_brightPassTexture, halfW, halfH, GL_RGBA16F);
    createFbo(m_blurFbo[0], m_blurTexture[0], halfW, halfH, GL_RGBA16F);
    createFbo(m_blurFbo[1], m_blurTexture[1], halfW, halfH, GL_RGBA16F);

    if (!m_sceneFbo || !m_brightPassFbo || !m_blurFbo[0] || !m_blurFbo[1]) {
        qWarning() << "PostProcess: Failed to create framebuffer objects";
        return;
    }

    m_initialized = true;
    qDebug() << "PostProcess: Initialized bloom pipeline" << width << "x" << height;
}

void PostProcess::resize(int width, int height)
{
    if (!m_initialized || (width == m_width && height == m_height)) {
        m_width = width;
        m_height = height;
        return;
    }

    m_width = width;
    m_height = height;

    int halfW = std::max(1, width / 2);
    int halfH = std::max(1, height / 2);

    // Recreate all FBOs at new size
    destroyFbo(m_sceneFbo, m_sceneTexture);
    destroyFbo(m_brightPassFbo, m_brightPassTexture);
    destroyFbo(m_blurFbo[0], m_blurTexture[0]);
    destroyFbo(m_blurFbo[1], m_blurTexture[1]);

    createFbo(m_sceneFbo, m_sceneTexture, width, height, GL_RGBA16F);
    createFbo(m_brightPassFbo, m_brightPassTexture, halfW, halfH, GL_RGBA16F);
    createFbo(m_blurFbo[0], m_blurTexture[0], halfW, halfH, GL_RGBA16F);
    createFbo(m_blurFbo[1], m_blurTexture[1], halfW, halfH, GL_RGBA16F);
}

void PostProcess::beginScene()
{
    if (!m_initialized) return;

    // Bind HDR scene FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFbo);
    glViewport(0, 0, m_width, m_height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PostProcess::endScene()
{
    if (!m_initialized) return;

    // Disable depth testing for post-processing passes
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    int halfW = std::max(1, m_width / 2);
    int halfH = std::max(1, m_height / 2);

    // Pass 1: Bright-pass filter
    glBindFramebuffer(GL_FRAMEBUFFER, m_brightPassFbo);
    glViewport(0, 0, halfW, halfH);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_brightPassProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_sceneTexture);
    glUniform1i(glGetUniformLocation(m_brightPassProgram, "uSceneTexture"), 0);
    glUniform1f(glGetUniformLocation(m_brightPassProgram, "uThreshold"), m_bloomThreshold);
    glUniform1f(glGetUniformLocation(m_brightPassProgram, "uSoftKnee"), m_bloomThreshold * 0.5f);

    drawFullscreenQuad();

    // Pass 2: Separable Gaussian blur (ping-pong, multiple iterations for wide blur)
    GLuint inputTexture = m_brightPassTexture;

    glUseProgram(m_blurProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    glUniform1i(glGetUniformLocation(m_blurProgram, "uTexture"), 0);

    int blurIterations = 4;
    for (int i = 0; i < blurIterations; ++i) {
        // Horizontal pass
        glBindFramebuffer(GL_FRAMEBUFFER, m_blurFbo[i % 2]);
        glViewport(0, 0, halfW, halfH);
        glClear(GL_COLOR_BUFFER_BIT);

        glUniform2f(glGetUniformLocation(m_blurProgram, "uDirection"),
                     1.0f / halfW, 0.0f);

        glBindTexture(GL_TEXTURE_2D, (i == 0) ? m_brightPassTexture : m_blurTexture[(i + 1) % 2]);
        drawFullscreenQuad();

        // Vertical pass
        glBindFramebuffer(GL_FRAMEBUFFER, m_blurFbo[(i + 1) % 2]);
        glViewport(0, 0, halfW, halfH);
        glClear(GL_COLOR_BUFFER_BIT);

        glUniform2f(glGetUniformLocation(m_blurProgram, "uDirection"),
                     0.0f, 1.0f / halfH);

        glBindTexture(GL_TEXTURE_2D, m_blurTexture[i % 2]);
        drawFullscreenQuad();
    }

    // Final blur result is in m_blurFbo[(blurIterations) % 2]
    GLuint finalBlurTexture = m_blurTexture[blurIterations % 2];

    // Pass 3: Composite to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_compositeProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_sceneTexture);
    glUniform1i(glGetUniformLocation(m_compositeProgram, "uSceneTexture"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, finalBlurTexture);
    glUniform1i(glGetUniformLocation(m_compositeProgram, "uBloomTexture"), 1);

    glUniform1f(glGetUniformLocation(m_compositeProgram, "uBloomIntensity"),
                m_bloomEnabled ? m_bloomIntensity : 0.0f);
    glUniform1f(glGetUniformLocation(m_compositeProgram, "uExposure"), m_exposure);
    glUniform1i(glGetUniformLocation(m_compositeProgram, "uToneMappingMode"), m_toneMappingMode);

    drawFullscreenQuad();

    glUseProgram(0);
    glBindVertexArray(0);

    // Restore state for next frame
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void PostProcess::createFbo(GLuint& fbo, GLuint& texture, int width, int height, GLenum internalFormat)
{
    // Delete existing resources
    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (texture) glDeleteTextures(1, &texture);

    // Create texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create FBO
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        qWarning() << "PostProcess: FBO incomplete:" << status;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcess::destroyFbo(GLuint& fbo, GLuint& texture)
{
    if (fbo) {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }
    if (texture) {
        glDeleteTextures(1, &texture);
        texture = 0;
    }
}

GLuint PostProcess::compileShaderProgram(const char* vertexSource, const char* fragmentSource)
{
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &vertexSource, nullptr);
    glCompileShader(vertShader);

    GLint success;
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertShader, 512, nullptr, infoLog);
        qWarning() << "PostProcess: Vertex shader compilation failed:" << infoLog;
        glDeleteShader(vertShader);
        return 0;
    }

    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragShader);

    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragShader, 512, nullptr, infoLog);
        qWarning() << "PostProcess: Fragment shader compilation failed:" << infoLog;
        glDeleteShader(fragShader);
        glDeleteShader(vertShader);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        qWarning() << "PostProcess: Shader linking failed:" << infoLog;
        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    return program;
}

void PostProcess::drawFullscreenQuad()
{
    glBindVertexArray(m_quadVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

} // namespace quantumverse
