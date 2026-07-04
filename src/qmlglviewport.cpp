/**
 * @file qmlglviewport.cpp
 * @brief Implementation of QML OpenGL viewport renderer
 *
 * Implements the QQuickFramebufferObject::Renderer interface to provide
 * hardware-accelerated OpenGL rendering within QML layouts.
 */

// glad.h MUST be included before Qt OpenGL headers to provide
// OpenGL function prototypes and prevent system <GL/gl.h> conflicts.
#define GLFW_INCLUDE_NONE
#include "../third_party/glad/glad.h"

// Qt headers MUST be included outside any namespace block, since Qt uses
// QT_BEGIN_NAMESPACE / QT_END_NAMESPACE macros internally.
#include <QDateTime>
#include <QDebug>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions>

#include "qmlglviewport.h"
#include "rendering/CurvatureRenderer.h"
#include "rendering/QuantumGeometryRenderer.h"
#include "rendering/CelestialBodyRenderer.h"
#include "rendering/GLDebug.h"
#include "ui4d/UI4D.h"
#include "ui4d/Camera4DAdapter.h"
#include "spacetime/Event4D.h"
#include "spacetime/MetricTensor.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <iostream>
#include <vector>
#include <windows.h>  // for MessageBoxA

// Ensure M_PI is available on all platforms (MSVC <cmath> may not define it)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// All quantumverse types are used with explicit namespace qualifiers
// to avoid namespace pollution issues with Qt headers

// QmlGlRenderer implementation (Qt-dependent, in quantumverse namespace)
// Note: Classes are defined in quantumverse namespace in the header,
// so we open the namespace block here.
namespace quantumverse {

QmlGlRenderer::QmlGlRenderer(int viewportWidth, int viewportHeight)
: m_viewportWidth(viewportWidth)
, m_viewportHeight(viewportHeight)
, m_showGrid(true)
, m_showLightCones(false)
, m_showGeodesics(true)
, m_showQuantumGeometry(false)
, m_curvatureMode(0)
, m_time(0.0f)
, m_frameTime(0.0f)
, m_frameCount(0)
, m_cameraDistance(50.0f)
, m_cameraAngleX(0.0f)
, m_cameraAngleY(0.0f)
, m_cameraPanX(0.0f)
, m_cameraPanY(0.0f)
, m_fbo(nullptr)
{
    // Initialize view matrix
    m_viewMatrix.setToIdentity();
    m_viewMatrix.translate(0.0f, 0.0f, -m_cameraDistance);

    // Initialize projection matrix
    const float fov = 45.0f;
    const float aspect = static_cast<float>(viewportWidth) / viewportHeight;
    const float nearPlane = 0.1f;
    const float farPlane = 1000.0f;
    m_projectionMatrix.perspective(fov, aspect, nearPlane, farPlane);

    // Note: CelestialBodyRenderer is initialized lazily in render() when GL context is available
    // to avoid crashes when constructor is called before GL context is ready
}

QmlGlRenderer::~QmlGlRenderer()
{
    // Release OpenGL resources in reverse allocation order
    if (m_profilingVAO) {
        glDeleteVertexArrays(1, &m_profilingVAO);
        m_profilingVAO = 0;
    }
    if (m_profilingVBO) {
        glDeleteBuffers(1, &m_profilingVBO);
        m_profilingVBO = 0;
    }
    if (m_overlayVAO) {
        glDeleteVertexArrays(1, &m_overlayVAO);
        m_overlayVAO = 0;
    }
    if (m_overlayVBO) {
        glDeleteBuffers(1, &m_overlayVBO);
        m_overlayVBO = 0;
    }
    if (m_axisVao) {
        glDeleteVertexArrays(1, &m_axisVao);
        m_axisVao = 0;
    }
    if (m_axisVbo) {
        glDeleteBuffers(1, &m_axisVbo);
        m_axisVbo = 0;
    }
    if (m_gridVao) {
        glDeleteVertexArrays(1, &m_gridVao);
        m_gridVao = 0;
    }
    if (m_gridVbo) {
        glDeleteBuffers(1, &m_gridVbo);
        m_gridVbo = 0;
    }
    if (m_gridEbo) {
        glDeleteBuffers(1, &m_gridEbo);
        m_gridEbo = 0;
    }
}

void QmlGlRenderer::render()
{
    // Record frame time
    static int64_t lastTime = 0;
    int64_t currentTime = QDateTime::currentMSecsSinceEpoch();
    if (lastTime > 0) {
        m_frameTime = static_cast<float>(currentTime - lastTime) / 1000.0f;
    }
    lastTime = currentTime;
    m_frameCount++;

    // Record frame in profiler (microsecond precision)
    static int64_t frameStartUs = 0;
    int64_t frameEndUs = currentTime * 1000;
    if (frameStartUs > 0) {
        m_frameProfiler.recordFrame(frameStartUs, frameEndUs);
    }
    frameStartUs = frameEndUs;

    // Get the current OpenGL context
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if (!ctx) {
        qWarning("QmlGlRenderer: No OpenGL context available in render()");
        return;
    }

    // Initialize OpenGL state on first render
    static bool initialized = false;
    if (!initialized) {
        qDebug() << "QmlGlRenderer: Initializing OpenGL on first render";
        initializeGL();
        initialized = true;
    }

    // Initialize celestial body renderer on first render (GL context is now confirmed current)
    // This is done here instead of in synchronize() because synchronize() may be called
    // before the GL context is available on the render thread
    if (m_celestialBodyRenderer && !m_celestialBodyRenderer->isInitialized()) {
        qDebug() << "QmlGlRenderer: Initializing celestial body renderer";
        try {
            m_celestialBodyRenderer->initializeGL();
        } catch (const std::exception& e) {
            qWarning() << "QmlGlRenderer: Failed to initialize celestial body renderer:" << e.what();
        } catch (...) {
            qWarning() << "QmlGlRenderer: Failed to initialize celestial body renderer (unknown error)";
        }
    }

    // Update view matrix from Camera4DAdapter if available
    if (m_camera4DAdapter) {
        m_viewMatrix = m_camera4DAdapter->viewMatrix();
    }

    // Clear the framebuffer
    glClearColor(0.02f, 0.02f, 0.06f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);

    // Render scene components
    if (m_showGrid) {
        renderGrid();
    }

    renderAxisGizmo();

    if (m_showGeodesics && m_curvatureRenderer) {
        renderGeodesics();
    }

    // Render curvature visualization (delegates to CurvatureRenderer)
    if (m_curvatureRenderer) {
        m_curvatureRenderer->render(m_viewMatrix.constData(),
                                     m_projectionMatrix.constData());
    }

    // Render celestial bodies (only if initialized)
    if (m_celestialBodyRenderer) {
        try {
            if (m_celestialBodyRenderer->isInitialized()) {
                m_celestialBodyRenderer->render(m_viewMatrix.constData(),
                                               m_projectionMatrix.constData());
            }
        } catch (const std::exception& e) {
            qWarning() << "QmlGlRenderer: Error rendering celestial bodies:" << e.what();
        } catch (...) {
            qWarning() << "QmlGlRenderer: Unknown error rendering celestial bodies";
        }
    }

    if (m_showQuantumGeometry && m_quantumRenderer) {
        renderQuantumGeometry();
    }

    renderOverlay();
    renderProfilingOverlay();

    // Update time for animation
    m_time += m_frameTime;
}

QOpenGLFramebufferObject* QmlGlRenderer::createFramebufferObject(const QSize& size)
{
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4); // MSAA for smooth rendering
    m_viewportWidth = size.width();
    m_viewportHeight = size.height();

    // Update projection matrix for new aspect ratio
    const float fov = 45.0f;
    const float aspect = static_cast<float>(size.width()) / size.height();
    m_projectionMatrix.setToIdentity();
    m_projectionMatrix.perspective(fov, aspect, 0.1f, 1000.0f);

    return new QOpenGLFramebufferObject(size, format);
}

void QmlGlRenderer::synchronize(::QQuickFramebufferObject* item)
{
    try {
        Q_UNUSED(item);
        
        // Null renderer guards - prevent crashes when renderers are not set
        if (!m_curvatureRenderer) {
            qWarning() << "QmlGlRenderer: curvatureRenderer is null in synchronize()";
            return;
        }
        if (!m_quantumRenderer) {
            qWarning() << "QmlGlRenderer: quantumRenderer is null in synchronize()";
            // Continue anyway - quantum geometry is optional
        }
        
        // Initialize the curvature renderer's GL resources if it has been set
        // but not yet initialized (requires a valid GL context which we now have).
        // Note: CelestialBodyRenderer initialization is deferred to render() to avoid
        // potential crashes when synchronize() is called before GL context is ready.
        try {
            if (!m_curvatureRenderer->isInitialized()) {
                m_curvatureRenderer->initializeGL();
            }
        } catch (const std::exception& e) {
            qWarning() << "QmlGlRenderer: Failed to initialize curvature renderer:" << e.what();
        } catch (...) {
            qWarning() << "QmlGlRenderer: Unknown error initializing curvature renderer";
        }
        
        if (m_quantumRenderer) {
            try {
                if (!m_quantumRenderer->isInitialized()) {
                    m_quantumRenderer->initializeGL();
                }
            } catch (const std::exception& e) {
                qWarning() << "QmlGlRenderer: Failed to initialize quantum renderer:" << e.what();
            } catch (...) {
                qWarning() << "QmlGlRenderer: Unknown error initializing quantum renderer";
            }
        }
        // Note: m_celestialBodyRenderer initialization is done in render() on first frame
        // to ensure GL context is current on the render thread
    } catch (const std::exception& e) {
        std::cerr << "EXCEPTION in synchronize: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "UNKNOWN EXCEPTION in synchronize" << std::endl;
    }
}

void QmlGlRenderer::setCurvatureRenderer(std::shared_ptr<CurvatureRenderer> renderer)
{
    m_curvatureRenderer = std::move(renderer);
}

void QmlGlRenderer::setQuantumRenderer(std::shared_ptr<QuantumGeometryRenderer> renderer)
{
    m_quantumRenderer = std::move(renderer);
}

void QmlGlRenderer::setUI4D(std::shared_ptr<UI4D> ui4d)
{
    m_ui4d = std::move(ui4d);
}

void QmlGlRenderer::setCelestialBodyRenderer(std::shared_ptr<CelestialBodyRenderer> renderer)
{
    m_celestialBodyRenderer = std::move(renderer);
}

void QmlGlRenderer::setCamera4DAdapter(std::shared_ptr<Camera4DAdapter> adapter)
{
    m_camera4DAdapter = std::move(adapter);
}

void QmlGlRenderer::setViewMatrix(const QMatrix4x4& view)
{
    m_viewMatrix = view;
}

void QmlGlRenderer::setProjectionMatrix(const QMatrix4x4& proj)
{
    m_projectionMatrix = proj;
}

QMatrix4x4 QmlGlRenderer::getViewMatrix() const
{
    return m_viewMatrix;
}

QMatrix4x4 QmlGlRenderer::getProjectionMatrix() const
{
    return m_projectionMatrix;
}

void QmlGlRenderer::zoom(float factor)
{
    m_cameraDistance *= factor;
    m_cameraDistance = std::max(0.1f, std::min(m_cameraDistance, 10000.0f));
    m_viewMatrix.setToIdentity();
    m_viewMatrix.translate(0.0f, 0.0f, -m_cameraDistance);
    m_viewMatrix.rotate(m_cameraAngleX, 1.0f, 0.0f, 0.0f);
    m_viewMatrix.rotate(m_cameraAngleY, 0.0f, 1.0f, 0.0f);
    m_viewMatrix.translate(m_cameraPanX, m_cameraPanY, 0.0f);
}

void QmlGlRenderer::rotate(float angleX, float angleY)
{
    m_cameraAngleX += angleX;
    m_cameraAngleY += angleY;

    // Clamp X rotation to avoid gimbal lock
    m_cameraAngleX = std::max(-89.0f * static_cast<float>(M_PI) / 180.0f,
                                std::min(89.0f * static_cast<float>(M_PI) / 180.0f, m_cameraAngleX));

    m_viewMatrix.setToIdentity();
    m_viewMatrix.translate(0.0f, 0.0f, -m_cameraDistance);
    m_viewMatrix.rotate(m_cameraAngleX, 1.0f, 0.0f, 0.0f);
    m_viewMatrix.rotate(m_cameraAngleY, 0.0f, 1.0f, 0.0f);
    m_viewMatrix.translate(m_cameraPanX, m_cameraPanY, 0.0f);
}

void QmlGlRenderer::pan(float dx, float dy)
{
    float scale = m_cameraDistance * 0.001f;
    m_cameraPanX += dx * scale;
    m_cameraPanY += dy * scale;

    m_viewMatrix.setToIdentity();
    m_viewMatrix.translate(0.0f, 0.0f, -m_cameraDistance);
    m_viewMatrix.rotate(m_cameraAngleX, 1.0f, 0.0f, 0.0f);
    m_viewMatrix.rotate(m_cameraAngleY, 0.0f, 1.0f, 0.0f);
    m_viewMatrix.translate(m_cameraPanX, m_cameraPanY, 0.0f);
}

void QmlGlRenderer::setSliceOffset(int viewIndex, double offset)
{
    if (m_ui4d) {
        m_ui4d->setSliceOffset(viewIndex, offset);
    }
}

void QmlGlRenderer::setShowGrid(bool show) { m_showGrid = show; }
void QmlGlRenderer::setShowLightCones(bool show) { m_showLightCones = show; }
void QmlGlRenderer::setShowGeodesics(bool show) { m_showGeodesics = show; }
void QmlGlRenderer::setShowQuantumGeometry(bool show) { m_showQuantumGeometry = show; }
void QmlGlRenderer::setCurvatureMode(int mode) { m_curvatureMode = mode; }

void QmlGlRenderer::updateTime(float deltaTime)
{
    m_time += deltaTime;
    if (m_curvatureRenderer) {
        m_curvatureRenderer->updateTime(deltaTime);
    }
}

void QmlGlRenderer::resetTime()
{
    m_time = 0.0f;
}

// ============================================================================
// OpenGL Initialization
// ============================================================================

void QmlGlRenderer::initializeGL()
{
    // Load OpenGL function pointers via glad
    if (!gladLoadGL()) {
        qWarning("Failed to initialize GLAD OpenGL loader");
        return;
    }

    // Initialize Qt's OpenGL function wrappers
    initializeOpenGLFunctions();

    // Install GL debug callback for error detection
    if (!quantumverse::GLDebug::instance().initialize()) {
        qWarning("Failed to initialize GL debug callback");
    }

    // Setup shaders
    setupShaders();

    // Setup geometry
    setupGridGeometry();
    setupAxisGizmo();
    setupOverlayGeometry();

    // OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
}

bool QmlGlRenderer::compileShader(QOpenGLShaderProgram& program,
                                   const char* vertexSource,
                                   const char* fragmentSource)
{
    bool success = program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexSource);
    if (!success) {
        qWarning("Vertex shader compilation failed: %s", program.log().toStdString().c_str());
        return false;
    }

    success = program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentSource);
    if (!success) {
        qWarning("Fragment shader compilation failed: %s", program.log().toStdString().c_str());
        return false;
    }

    success = program.link();
    if (!success) {
        qWarning("Shader linking failed: %s", program.log().toStdString().c_str());
        return false;
    }

    return true;
}

void QmlGlRenderer::setupShaders()
{
    // Grid shader - vertex displacement based on curvature
    const char* gridVert = R"(
        #version 450 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec4 aColor;
        layout(location = 3) in float aCurvature;

        uniform mat4 viewMatrix;
        uniform mat4 projectionMatrix;
        uniform float time;
        uniform float curvatureScale;

        out vec4 fragColor;
        out float curvatureValue;

        void main() {
            // Displace vertices based on curvature for grid deformation
            float displacement = aCurvature * curvatureScale * 0.1;
            vec3 displacedPos = aPos + aNormal * displacement;

            gl_Position = projectionMatrix * viewMatrix * vec4(displacedPos, 1.0);
            fragColor = aColor;
            curvatureValue = aCurvature;
        }
    )";

    const char* gridFrag = R"(
        #version 450 core
        in vec4 fragColor;
        in float curvatureValue;

        out vec4 outColor;

        void main() {
            // Color based on curvature magnitude
            float intensity = clamp(log2(abs(curvatureValue) + 1.0) * 0.3, 0.0, 1.0);
            vec3 blueShift = mix(vec3(0.1, 0.2, 0.6), vec3(0.8, 0.1, 0.1), intensity);
            outColor = vec4(blueShift, fragColor.a * 0.7);
        }
    )";

    // Geodesic shader - renders worldlines as glowing tubes
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

        void main() {
            outColor = fragColor;
        }
    )";

    // Overlay shader - HUD elements, light cones
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

        void main() {
            outColor = fragColor;
        }
    )";

    compileShader(m_gridShader, gridVert, gridFrag);
    compileShader(m_geodesicShader, geodesicVert, geodesicFrag);
    compileShader(m_overlayShader, overlayVert, overlayFrag);
}

void QmlGlRenderer::setupGridGeometry()
{
    // Generate a 3D grid for spacetime visualization
    const int gridRes = 20;
    const float gridSize = 100.0f;
    const float halfSize = gridSize * 0.5f;
    const float step = gridSize / gridRes;

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Create grid vertices along XZ plane at Y=0
    for (int i = 0; i <= gridRes; ++i) {
        float x = -halfSize + i * step;
        // Z-axis lines
        vertices.insert(vertices.end(), {
            x, 0.0f, -halfSize,  // position
            0.0f, 1.0f, 0.0f,    // normal
            0.2f, 0.3f, 0.6f, 0.5f, // color
            0.0f                  // curvature
        });
        vertices.insert(vertices.end(), {
            x, 0.0f, halfSize,
            0.0f, 1.0f, 0.0f,
            0.2f, 0.3f, 0.6f, 0.5f,
            0.0f
        });

        // X-axis lines
        float z = -halfSize + i * step;
        vertices.insert(vertices.end(), {
            -halfSize, 0.0f, z,
            0.0f, 1.0f, 0.0f,
            0.2f, 0.3f, 0.6f, 0.5f,
            0.0f
        });
        vertices.insert(vertices.end(), {
            halfSize, 0.0f, z,
            0.0f, 1.0f, 0.0f,
            0.2f, 0.3f, 0.6f, 0.5f,
            0.0f
        });
    }

    // Generate indices for line drawing
    for (int i = 0; i <= gridRes; ++i) {
        // Z-lines
        indices.push_back(i * 2);
        indices.push_back(i * 2 + 1);
        // X-lines
        indices.push_back((gridRes + 1) * 2 + i * 2);
        indices.push_back((gridRes + 1) * 2 + i * 2 + 1);
    }

    // Create OpenGL buffers
    glGenVertexArrays(1, &m_gridVao);
    glGenBuffers(1, &m_gridVbo);
    glGenBuffers(1, &m_gridEbo);

    glBindVertexArray(m_gridVao);

    glBindBuffer(GL_ARRAY_BUFFER, m_gridVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(float),
                 vertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_gridEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(),
                 GL_STATIC_DRAW);

    // Position (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal (3 floats)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Color (4 floats)
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    // Curvature (1 float)
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(10 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
}

void QmlGlRenderer::setupAxisGizmo()
{
    // Simple axis gizmo: 3 colored lines for X, Y, Z
    const float axisLength = 5.0f;
    std::vector<float> vertices = {
        // X axis (red)
        0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f,  0.0f,
        axisLength, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f,  0.0f,
        // Y axis (green)
        0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f,  0.0f,
        0.0f, axisLength, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f,  0.0f,
        // Z axis (blue)
        0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f, 1.0f,  0.0f,
        0.0f, 0.0f, axisLength,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f, 1.0f,  0.0f,
    };

    std::vector<unsigned int> indices = {0, 1, 2, 3, 4, 5};

    glGenVertexArrays(1, &m_axisVao);
    glGenBuffers(1, &m_axisVbo);

    glBindVertexArray(m_axisVao);

    glBindBuffer(GL_ARRAY_BUFFER, m_axisVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(float),
                 vertices.data(),
                 GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal (reused as color hint)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Color
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

// ============================================================================
// Overlay Geometry Setup
// ============================================================================

void QmlGlRenderer::setupOverlayGeometry()
{
    // Axis indicator VAO/VBO (screen-space, 2D position + 4D color = 6 floats per vertex)
    glGenVertexArrays(1, &m_overlayVAO);
    glGenBuffers(1, &m_overlayVBO);

    glBindVertexArray(m_overlayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_overlayVBO);
    // Dynamic draw since we update axis vertices each frame
    glBufferData(GL_ARRAY_BUFFER, 6 * 6 * sizeof(GLfloat), nullptr, GL_DYNAMIC_DRAW);

    // layout 0: vec2 aPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    // layout 1: vec4 aColor
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Profiling bars VAO/VBO (dynamic, grows with frame count)
    glGenVertexArrays(1, &m_profilingVAO);
    glGenBuffers(1, &m_profilingVBO);

    glBindVertexArray(m_profilingVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_profilingVBO);
    // Allocate a reasonable maximum (e.g., 120 frames worth of quads = 120 * 4 * 6 floats)
    glBufferData(GL_ARRAY_BUFFER, 120 * 4 * 6 * sizeof(GLfloat), nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// ============================================================================
// Rendering Methods
// ============================================================================

void QmlGlRenderer::renderGrid()
{
    if (!m_gridShader.bind()) return;

    m_gridShader.setUniformValue("viewMatrix", m_viewMatrix);
    m_gridShader.setUniformValue("projectionMatrix", m_projectionMatrix);
    m_gridShader.setUniformValue("time", m_time);
    m_gridShader.setUniformValue("curvatureScale", 1.0f);

    glBindVertexArray(m_gridVao);
    glLineWidth(1.0f);
    glDrawElements(GL_LINES, 2 * (21 + 21), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    m_gridShader.release();
}

void QmlGlRenderer::renderAxisGizmo()
{
    if (!m_overlayShader.bind()) return;

    // Use orthographic projection for screen-space axis indicator
    QMatrix4x4 ortho;
    ortho.ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
    m_overlayShader.setUniformValue("projectionMatrix", ortho);

    glBindVertexArray(m_axisVao);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, 6);
    glBindVertexArray(0);

    m_overlayShader.release();
}

void QmlGlRenderer::renderGeodesics()
 {
     if (!m_geodesicShader.bind()) return;

     m_geodesicShader.setUniformValue("viewMatrix", m_viewMatrix);
     m_geodesicShader.setUniformValue("projectionMatrix", m_projectionMatrix);

     // Collect orbit worldlines from UI4D solar system data
     if (!m_ui4d) {
         m_geodesicShader.release();
         return;
     }
     
     const auto& solarData = m_ui4d->getSolarSystemData();
     std::vector<std::vector<Event4D>> geodesics;

     for (const auto& bodyPair : solarData.bodies) {
         const auto& body = bodyPair.second;
         if (!body.showOrbit || body.orbitPoints.empty()) continue;

         // Convert orbit points from solar system scale to viewport scale
         std::vector<Event4D> scaledOrbit;
         scaledOrbit.reserve(body.orbitPoints.size());
         for (const auto& pt : body.orbitPoints) {
             scaledOrbit.emplace_back(
                 pt.t,
                 pt.x * solarData.scaleFactor,
                 pt.y * solarData.scaleFactor,
                 pt.z * solarData.scaleFactor
             );
         }
         geodesics.push_back(std::move(scaledOrbit));
     }

     // Delegate to CurvatureRenderer for actual GPU rendering
     if (m_curvatureRenderer && !geodesics.empty()) {
         try {
             m_curvatureRenderer->renderGeodesics(
                 geodesics,
                 m_viewMatrix.constData(),
                 m_projectionMatrix.constData()
             );
         } catch (const std::exception& e) {
             qWarning() << "QmlGlRenderer: Error rendering geodesics:" << e.what();
         } catch (...) {
             qWarning() << "QmlGlRenderer: Unknown error rendering geodesics";
         }
     }

     // Populate celestial body renderer with solar system bodies (once per frame)
     // Only if the renderer is initialized (GL context is available)
     if (m_celestialBodyRenderer) {
         try {
             if (m_celestialBodyRenderer->isInitialized()) {
                 // Clear previous frame's bodies to prevent unbounded accumulation
                 m_celestialBodyRenderer->clearBodies();

                 // Assign texture layers based on body name for texture array support
                 auto assignTextureLayer = [](const std::string& name, bool isStar) -> int {
                     if (isStar) return 7; // Sun / star layer
                     if (name.find("Earth") != std::string::npos) return 0;
                     if (name.find("Mars") != std::string::npos) return 1;
                     if (name.find("Jupiter") != std::string::npos) return 2;
                     if (name.find("Saturn") != std::string::npos) return 3;
                     if (name.find("Venus") != std::string::npos) return 4;
                     if (name.find("Mercury") != std::string::npos) return 5;
                     if (name.find("Moon") != std::string::npos) return 6;
                     return -1; // No texture
                 };

                 for (const auto& bodyPair : solarData.bodies) {
                     const auto& body = bodyPair.second;
                     if (!body.showOrbit || body.orbitPoints.empty()) continue;

                     CelestialBodyInstance cbi;
                     cbi.objectId = bodyPair.first;
                     cbi.name = body.name;
                     cbi.mass = static_cast<float>(body.mass);
                     cbi.radius = static_cast<float>(body.radius) * solarData.scaleFactor * 0.5f;
                     cbi.position[0] = static_cast<float>(body.orbitPoints.back().x * solarData.scaleFactor);
                     cbi.position[1] = static_cast<float>(body.orbitPoints.back().y * solarData.scaleFactor);
                     cbi.position[2] = static_cast<float>(body.orbitPoints.back().z * solarData.scaleFactor);
                     cbi.isStar = body.isStar;
                     cbi.hasAtmosphere = !body.isStar;
                     cbi.atmosphereRadius = 1.15f;
                     cbi.textureLayer = assignTextureLayer(body.name, body.isStar);

                     // Color based on body type
                     if (body.isStar) {
                         cbi.color[0] = 1.0f; cbi.color[1] = 0.95f; cbi.color[2] = 0.8f;
                         cbi.emissive[0] = 1.0f; cbi.emissive[1] = 0.9f; cbi.emissive[2] = 0.7f;
                     } else if (body.name.find("Earth") != std::string::npos) {
                         cbi.color[0] = 0.2f; cbi.color[1] = 0.5f; cbi.color[2] = 0.9f;
                         cbi.emissive[0] = 0.0f; cbi.emissive[1] = 0.0f; cbi.emissive[2] = 0.0f;
                     } else if (body.name.find("Mars") != std::string::npos) {
                         cbi.color[0] = 0.9f; cbi.color[1] = 0.4f; cbi.color[2] = 0.2f;
                         cbi.emissive[0] = 0.0f; cbi.emissive[1] = 0.0f; cbi.emissive[2] = 0.0f;
                     } else {
                         cbi.color[0] = 0.6f; cbi.color[1] = 0.6f; cbi.color[2] = 0.6f;
                         cbi.emissive[0] = 0.0f; cbi.emissive[1] = 0.0f; cbi.emissive[2] = 0.0f;
                     }

                     // Add body (respects maxBodies cap internally)
                     m_celestialBodyRenderer->addBody(cbi);
                 }
             }
         } catch (const std::exception& e) {
             qWarning() << "QmlGlRenderer: Error in renderGeodesics celestial body rendering:" << e.what();
         } catch (...) {
             qWarning() << "QmlGlRenderer: Unknown error in renderGeodesics celestial body rendering";
         }
     }
     
     m_geodesicShader.release();
 }

void QmlGlRenderer::renderQuantumGeometry()
{
    // Check for null quantum renderer to avoid crash
    if (!m_quantumRenderer) return;
    if (!m_ui4d) return;

    // Use Camera4DAdapter for camera state if available
    Event4D camPos, lookAt;
    double fov = M_PI / 4.0;

    if (m_camera4DAdapter) {
        camPos = m_camera4DAdapter->cameraPosition4D();
        lookAt = m_camera4DAdapter->cameraTarget4D();
        // FOV from distance for perspective
        fov = 2.0 * std::atan(1.0 / m_camera4DAdapter->distance());
    } else {
        // Fallback: Build Camera4D from viewport camera state
        const float distance = m_cameraDistance;
        const float elevRad = m_cameraAngleX;
        const float azimRad = m_cameraAngleY;

        const float camX = distance * std::cos(elevRad) * std::sin(azimRad);
        const float camY = distance * std::sin(elevRad);
        const float camZ = distance * std::cos(elevRad) * std::cos(azimRad);

        camPos = Event4D(0.0, camX + m_cameraPanX, camY + m_cameraPanY, camZ);
        lookAt = Event4D(0.0, m_cameraPanX, m_cameraPanY, 0.0);
    }

    // Construct Camera4D
    Camera4D camera4d(camPos, lookAt, fov);

    // Get current slices from UI4D
    const auto& slices = m_ui4d->getSliceViews();

    // Get metric from UI4D
    auto metric = m_ui4d->getCurrentMetric();
    if (!metric) return;

    // Delegate to QuantumGeometryRenderer
    try {
        m_quantumRenderer->render(camera4d, slices, *metric);
    } catch (const std::exception& e) {
        qWarning() << "QmlGlRenderer: Error in renderQuantumGeometry:" << e.what();
    } catch (...) {
        qWarning() << "QmlGlRenderer: Unknown error in renderQuantumGeometry";
    }
}

void QmlGlRenderer::renderOverlay()
{
    // Delegate to Camera4DAdapter for view matrix if available
    if (m_camera4DAdapter) {
        m_viewMatrix = m_camera4DAdapter->viewMatrix();
    }

    // Render HUD overlay elements:
    // - Coordinate grid labels
    // - Frame rate counter
    // - Camera position info

    if (!m_overlayShader.bind()) return;

    QMatrix4x4 ortho;
    ortho.ortho(0.0f, static_cast<float>(m_viewportWidth),
                0.0f, static_cast<float>(m_viewportHeight),
                -1.0f, 1.0f);
    m_overlayShader.setUniformValue("projectionMatrix", ortho);

    // Render axis indicator lines (simple colored lines for X, Y, Z)
    float labelOffset = 10.0f;

    // X axis (red) - horizontal
    GLfloat xAxisVerts[] = {
        labelOffset, labelOffset,         1.0f, 0.0f, 0.0f, 0.9f,
        labelOffset + 50.0f, labelOffset, 1.0f, 0.0f, 0.0f, 0.9f,
    };

    // Y axis (green) - vertical
    GLfloat yAxisVerts[] = {
        labelOffset, labelOffset,              0.0f, 1.0f, 0.0f, 0.9f,
        labelOffset, labelOffset + 50.0f,      0.0f, 1.0f, 0.0f, 0.9f,
    };

    // Z axis (blue) - diagonal
    GLfloat zAxisVerts[] = {
        labelOffset, labelOffset,                    0.0f, 0.0f, 1.0f, 0.9f,
        labelOffset + 35.0f, labelOffset + 35.0f,    0.0f, 0.0f, 1.0f, 0.9f,
    };

    // Draw axis lines using VAO/VBO
    glBindVertexArray(m_overlayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_overlayVBO);

    // Upload and draw X axis
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(xAxisVerts), xAxisVerts);
    glDrawArrays(GL_LINES, 0, 2);

    // Upload and draw Y axis
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(yAxisVerts), yAxisVerts);
    glDrawArrays(GL_LINES, 0, 2);

    // Upload and draw Z axis
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(zAxisVerts), zAxisVerts);
    glDrawArrays(GL_LINES, 0, 2);

    // Draw axis endpoint dots
    float dotSize = 4.0f;

    // X endpoint (red)
    GLfloat xDot[] = {
        labelOffset + 50.0f - dotSize, labelOffset - dotSize, 1.0f, 0.0f, 0.0f, 0.9f,
        labelOffset + 50.0f + dotSize, labelOffset - dotSize, 1.0f, 0.0f, 0.0f, 0.9f,
        labelOffset + 50.0f + dotSize, labelOffset + dotSize, 1.0f, 0.0f, 0.0f, 0.9f,
        labelOffset + 50.0f - dotSize, labelOffset + dotSize, 1.0f, 0.0f, 0.0f, 0.9f,
    };
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(xDot), xDot);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // Y endpoint (green)
    GLfloat yDot[] = {
        labelOffset - dotSize, labelOffset + 50.0f - dotSize, 0.0f, 1.0f, 0.0f, 0.9f,
        labelOffset + dotSize, labelOffset + 50.0f - dotSize, 0.0f, 1.0f, 0.0f, 0.9f,
        labelOffset + dotSize, labelOffset + 50.0f + dotSize, 0.0f, 1.0f, 0.0f, 0.9f,
        labelOffset - dotSize, labelOffset + 50.0f + dotSize, 0.0f, 1.0f, 0.0f, 0.9f,
    };
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(yDot), yDot);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // Origin dot (white)
    GLfloat originDot[] = {
        labelOffset - dotSize, labelOffset - dotSize, 1.0f, 1.0f, 1.0f, 0.9f,
        labelOffset + dotSize, labelOffset - dotSize, 1.0f, 1.0f, 1.0f, 0.9f,
        labelOffset + dotSize, labelOffset + dotSize, 1.0f, 1.0f, 1.0f, 0.9f,
        labelOffset - dotSize, labelOffset + dotSize, 1.0f, 1.0f, 1.0f, 0.9f,
    };
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(originDot), originDot);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_overlayShader.release();
}

void QmlGlRenderer::renderProfilingOverlay()
{
    // Draw profiling stats using OpenGL primitives
    if (!m_overlayShader.bind()) return;

    QMatrix4x4 ortho;
    ortho.ortho(0.0f, static_cast<float>(m_viewportWidth),
                0.0f, static_cast<float>(m_viewportHeight),
                -1.0f, 1.0f);
    m_overlayShader.setUniformValue("projectionMatrix", ortho);

    // Render profiling bars: top-right corner HUD
    const auto& frames = m_frameProfiler.getRecentFrames();
    if (!frames.empty()) {
        glBindVertexArray(m_profilingVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_profilingVBO);

        float barWidth = 2.0f;
        float maxBarHeight = 100.0f;
        float originX = static_cast<float>(m_viewportWidth) - 10.0f - barWidth * static_cast<float>(frames.size());
        float originY = 50.0f;

        float maxTime = m_frameProfiler.getMaxFrameTime();
        if (maxTime < 1.0f) maxTime = 1.0f;

        glLineWidth(1.0f);

        // Build all bar vertices into a single buffer
        size_t maxBars = std::min(frames.size(), static_cast<size_t>(120));
        std::vector<GLfloat> barData(maxBars * 4 * 6); // 4 verts * 6 floats per bar

        for (size_t i = 0; i < maxBars; ++i) {
            float height = (frames[i].frameTimeMs / maxTime) * maxBarHeight;
            float x = originX + static_cast<float>(i) * barWidth;

            float r = frames[i].frameTimeMs < 16.0f ? 0.0f : (frames[i].frameTimeMs < 33.0f ? 1.0f : 1.0f);
            float g = frames[i].frameTimeMs < 16.0f ? 1.0f : (frames[i].frameTimeMs < 33.0f ? 1.0f : 0.0f);
            float b = 0.0f;

            float* base = barData.data() + i * 4 * 6;
            // Triangle fan: BL, TL, TR, BR
            base[0]  = x;              base[1]  = originY;              base[2] = r; base[3] = g; base[4] = b; base[5] = 0.7f;
            base[6]  = x;              base[7]  = originY + height;     base[8] = r; base[9] = g; base[10] = b; base[11] = 0.7f;
            base[12] = x + barWidth;   base[13] = originY + height;     base[14] = r; base[15] = g; base[16] = b; base[17] = 0.7f;
            base[18] = x + barWidth;   base[19] = originY;              base[20] = r; base[21] = g; base[22] = b; base[23] = 0.7f;
        }

        glBufferSubData(GL_ARRAY_BUFFER, 0, maxBars * 4 * 6 * sizeof(GLfloat), barData.data());

        // Draw all bars as triangle fans (4 verts each)
        for (size_t i = 0; i < maxBars; ++i) {
            glDrawArrays(GL_TRIANGLE_FAN, static_cast<GLint>(i * 4), 4);
        }

        // Render 16ms threshold line
        float thresholdY = originY + (16.0f / maxTime) * maxBarHeight;
        GLfloat thresholdLine[] = {
            originX - 2.0f, thresholdY,  1.0f, 1.0f, 1.0f, 0.5f,
            originX + barWidth * static_cast<float>(maxBars) + 2.0f, thresholdY,  1.0f, 1.0f, 1.0f, 0.5f,
        };
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(thresholdLine), thresholdLine);
        glDrawArrays(GL_LINES, 0, 2);

        // Render FPS legend dots (correct 6-float stride: vec2 pos + vec4 color)
        float legendX = originX - 12.0f;
        float legendY = originY + 5.0f;
        float dotSize = 4.0f;

        // Green dot (< 16ms)
        GLfloat greenDot[] = {
            legendX, legendY,                    0.0f, 1.0f, 0.0f, 0.9f,
            legendX + dotSize, legendY,          0.0f, 1.0f, 0.0f, 0.9f,
            legendX + dotSize, legendY + dotSize, 0.0f, 1.0f, 0.0f, 0.9f,
            legendX, legendY + dotSize,          0.0f, 1.0f, 0.0f, 0.9f,
        };
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(greenDot), greenDot);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // Yellow dot (16-33ms)
        GLfloat yellowDot[] = {
            legendX, legendY + dotSize + 4.0f,       1.0f, 1.0f, 0.0f, 0.9f,
            legendX + dotSize, legendY + dotSize + 4.0f, 1.0f, 1.0f, 0.0f, 0.9f,
            legendX + dotSize, legendY + 2.0f * (dotSize + 4.0f), 1.0f, 1.0f, 0.0f, 0.9f,
            legendX, legendY + 2.0f * (dotSize + 4.0f), 1.0f, 1.0f, 0.0f, 0.9f,
        };
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(yellowDot), yellowDot);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // Red dot (> 33ms)
        GLfloat redDot[] = {
            legendX, legendY + 2.0f * (dotSize + 4.0f) + 4.0f, 1.0f, 0.0f, 0.0f, 0.9f,
            legendX + dotSize, legendY + 2.0f * (dotSize + 4.0f) + 4.0f, 1.0f, 0.0f, 0.0f, 0.9f,
            legendX + dotSize, legendY + 3.0f * (dotSize + 4.0f), 1.0f, 0.0f, 0.0f, 0.9f,
            legendX, legendY + 3.0f * (dotSize + 4.0f), 1.0f, 0.0f, 0.0f, 0.9f,
        };
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(redDot), redDot);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    m_overlayShader.release();
}

// ============================================================================
// QmlGlViewport
// ============================================================================

QmlGlViewport::QmlGlViewport(QQuickItem* parent)
    : QQuickFramebufferObject(parent)
    , m_renderer(nullptr)
    , m_showGrid(true)
    , m_showLightCones(false)
    , m_showGeodesics(true)
    , m_showQuantumGeometry(false)
    , m_curvatureMode(0)
    , m_cameraDistance(50.0f)
    , m_cameraAngleX(0.0f)
    , m_cameraAngleY(0.0f)
    , m_simulationTime(0.0f)
    , m_frameRate(0.0f)
    , m_frameCount(0)
    , m_lastFrameTime(0)
{
    setObjectName("viewport");  // Required for findChild in main_qml.cpp
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemHasContents, true);
}

QmlGlViewport::~QmlGlViewport()
{
    delete m_renderer;
}

QQuickFramebufferObject::Renderer* QmlGlViewport::createRenderer() const
{
    m_renderer = new QmlGlRenderer(width() > 0 ? static_cast<int>(width()) : 1280,
                                    height() > 0 ? static_cast<int>(height()) : 720);
    return m_renderer;
}

float QmlGlViewport::frameRate() const
{
    if (m_frameRate > 0.0f) {
        return m_frameRate;
    }
    return 0.0f;
}

void QmlGlViewport::setShowGrid(bool show)
{
    if (m_showGrid != show) {
        m_showGrid = show;
        if (m_renderer) m_renderer->setShowGrid(show);
        emit showGridChanged();
        update();
    }
}

void QmlGlViewport::setShowLightCones(bool show)
{
    if (m_showLightCones != show) {
        m_showLightCones = show;
        if (m_renderer) m_renderer->setShowLightCones(show);
        emit showLightConesChanged();
        update();
    }
}

void QmlGlViewport::setShowGeodesics(bool show)
{
    if (m_showGeodesics != show) {
        m_showGeodesics = show;
        if (m_renderer) m_renderer->setShowGeodesics(show);
        emit showGeodesicsChanged();
        update();
    }
}

void QmlGlViewport::setShowQuantumGeometry(bool show)
{
    if (m_showQuantumGeometry != show) {
        m_showQuantumGeometry = show;
        if (m_renderer) m_renderer->setShowQuantumGeometry(show);
        emit showQuantumGeometryChanged();
        update();
    }
}

void QmlGlViewport::setCurvatureMode(int mode)
{
    if (m_curvatureMode != mode) {
        m_curvatureMode = mode;
        if (m_renderer) m_renderer->setCurvatureMode(mode);
        emit curvatureModeChanged();
        update();
    }
}

void QmlGlViewport::setCameraDistance(float dist)
{
    if (std::abs(m_cameraDistance - dist) > 0.01f) {
        m_cameraDistance = dist;
        if (m_renderer) m_renderer->zoom(dist / 50.0f);
        emit cameraDistanceChanged();
        update();
    }
}

void QmlGlViewport::setCameraAngleX(float angle)
{
    if (std::abs(m_cameraAngleX - angle) > 0.001f) {
        float delta = angle - m_cameraAngleX;
        m_cameraAngleX = angle;
        if (m_renderer) m_renderer->rotate(delta, 0.0f);
        emit cameraAngleXChanged();
        update();
    }
}

void QmlGlViewport::setCameraAngleY(float angle)
{
    if (std::abs(m_cameraAngleY - angle) > 0.001f) {
        float delta = angle - m_cameraAngleY;
        m_cameraAngleY = angle;
        if (m_renderer) m_renderer->rotate(0.0f, delta);
        emit cameraAngleYChanged();
        update();
    }
}

void QmlGlViewport::zoomIn()
{
    setCameraDistance(m_cameraDistance * 0.9f);
}

void QmlGlViewport::zoomOut()
{
    setCameraDistance(m_cameraDistance * 1.1f);
}

void QmlGlViewport::resetView()
{
    setCameraDistance(50.0f);
    setCameraAngleX(0.0f);
    setCameraAngleY(0.0f);
}

void QmlGlViewport::setCurvatureRenderer(QObject* renderer)
{
    // QML-accessible slot - CurvatureRenderer is not a QObject,
    // so this cannot be invoked from QML directly.
    // Use setCurvatureRendererDirect() from C++ instead.
    Q_UNUSED(renderer);
}

void QmlGlViewport::setQuantumRenderer(QObject* renderer)
{
    // QML-accessible slot - QuantumGeometryRenderer is not a QObject,
    // so this cannot be invoked from QML directly.
    // Use setQuantumRendererDirect() from C++ instead.
    Q_UNUSED(renderer);
}

void QmlGlViewport::setCurvatureRendererDirect(std::shared_ptr<CurvatureRenderer> renderer)
{
    m_curvatureRenderer = std::move(renderer);
    if (m_renderer) {
        m_renderer->setCurvatureRenderer(m_curvatureRenderer);
    }
    emit curvatureRendererChanged();
    update();
}

void QmlGlViewport::setQuantumRendererDirect(std::shared_ptr<QuantumGeometryRenderer> renderer)
{
    m_quantumRenderer = std::move(renderer);
    if (m_renderer) {
        m_renderer->setQuantumRenderer(m_quantumRenderer);
    }
    emit quantumRendererChanged();
    update();
}

void QmlGlViewport::setCamera4DAdapterDirect(std::shared_ptr<Camera4DAdapter> adapter)
{
    m_camera4DAdapter = std::move(adapter);
    if (m_renderer) {
        m_renderer->setCamera4DAdapter(m_camera4DAdapter);
    }
    emit camera4DAdapterChanged();
}

void QmlGlViewport::setUI4D(std::shared_ptr<UI4D> ui4d)
{
    m_ui4d = std::move(ui4d);
    if (m_renderer) {
        m_renderer->setUI4D(m_ui4d);
    }
}

void QmlGlViewport::setCelestialBodyRendererDirect(std::shared_ptr<CelestialBodyRenderer> renderer)
{
    m_celestialBodyRenderer = std::move(renderer);
    if (m_renderer) {
        m_renderer->setCelestialBodyRenderer(m_celestialBodyRenderer);
    }
}

QObject* QmlGlViewport::camera4DAdapterObj() const
{
    return m_camera4DAdapter.get();
}

void QmlGlViewport::setCamera4DAdapterObj(QObject* adapter)
{
    if (adapter) {
        auto typedAdapter = qobject_cast<Camera4DAdapter*>(adapter);
        if (typedAdapter) {
            setCamera4DAdapterDirect(std::shared_ptr<Camera4DAdapter>(typedAdapter, [](Camera4DAdapter*){}));
        }
    }
}

void QmlGlViewport::updateSimulation(double deltaTime)
{
    m_simulationTime += deltaTime;
    if (m_renderer) {
        m_renderer->updateTime(static_cast<float>(deltaTime));
    }
    emit simulationTimeChanged();

    // Update frame rate
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_lastFrameTime > 0) {
        qint64 elapsed = now - m_lastFrameTime;
        if (elapsed > 0) {
            m_frameRate = 1000.0f / elapsed;
            emit frameRateChanged();
        }
    }
    m_lastFrameTime = now;

    update();
}

void QmlGlViewport::handleMousePress(float x, float y, int button)
{
    Q_UNUSED(x); Q_UNUSED(y); Q_UNUSED(button);
    // Mouse interaction handled via QML MouseArea
}

void QmlGlViewport::handleMouseMove(float x, float y, int buttons)
{
    Q_UNUSED(x); Q_UNUSED(y); Q_UNUSED(buttons);
    // Mouse interaction handled via QML MouseArea
}

void QmlGlViewport::handleMouseReleased(float x, float y, int button)
{
    Q_UNUSED(x); Q_UNUSED(y); Q_UNUSED(button);
    // Mouse interaction handled via QML MouseArea
}

void QmlGlViewport::setSliceOffset(int viewIndex, double offset)
{
    if (m_renderer) {
        m_renderer->setSliceOffset(viewIndex, offset);
    }
    if (m_ui4d) {
        m_ui4d->setSliceOffset(viewIndex, offset);
    }
}

void QmlGlViewport::handleWheel(float delta)
{
    if (delta > 0) {
        zoomIn();
    } else {
        zoomOut();
    }
}

} // namespace quantumverse

