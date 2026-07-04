/**
 * @file qmlglviewport.h
 * @brief QQuickFramebufferObject-based OpenGL viewport for QML integration
 *
 * Provides a hardware-accelerated OpenGL rendering surface that can be
 * embedded in QML layouts. Wraps the existing CurvatureRenderer and
 * QuantumGeometryRenderer into a QML-compatible component.
 *
 * Based on: plan9.md Phase 0, Qt Quick/QOpenGLFramebufferObject patterns
 */

#ifndef QMLGLVIEWPORT_H
#define QMLGLVIEWPORT_H

// Prevent GLFW (if transitively included by Qt) from pulling in
// system <GL/gl.h> before glad can provide the prototypes.
#define GLFW_INCLUDE_NONE
#include "../third_party/glad/glad.h"

// Qt headers MUST be included outside any namespace, because Qt uses
// QT_BEGIN_NAMESPACE / QT_END_NAMESPACE macros that break when nested
// inside namespace quantumverse {}.
#include <QQuickFramebufferObject>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QDateTime>
#include <QOpenGLContext>
#include <QDebug>
#include <memory>
#include <vector>
#include <deque>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <array>

// Forward declarations to avoid Qt header conflicts with renderer includes
// These are in the quantumverse namespace
namespace quantumverse {
class CurvatureRenderer;
class QuantumGeometryRenderer;
class UI4D;
class Camera4D;
class Camera4DAdapter;
class CelestialBodyRenderer;
}

// Ensure M_PI is available on all platforms (MSVC <cmath> may not define it)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

/**
 * @brief Timing data for a single rendered frame
 */
struct FrameTiming {
    int64_t frameStart;   ///< Timestamp at frame start (ms)
    int64_t frameEnd;     ///< Timestamp at frame end (ms)
    float frameTimeMs;    ///< Duration in milliseconds
    float fps;            ///< Instantaneous FPS
};

/**
 * @brief Rolling frame statistics for benchmark/profiling overlay
 *
 * Maintains a sliding window of frame timings to compute
 * average/min/max/percentile frame times and FPS.
 */
class FrameProfiler {
public:
    /**
     * @brief Construct a new FrameProfiler
     * @param windowSize Number of recent frames to keep in the rolling window
     */
    explicit FrameProfiler(size_t windowSize = 60)
        : m_windowSize(windowSize), m_totalFrameTime(0) {}

    /**
     * @brief Record a completed frame's timing
     * @param startUs Frame start timestamp (microseconds)
     * @param endUs Frame end timestamp (microseconds)
     */
    void recordFrame(int64_t startUs, int64_t endUs) {
        FrameTiming timing;
        timing.frameStart = startUs / 1000;
        timing.frameEnd = endUs / 1000;
        timing.frameTimeMs = static_cast<float>(endUs - startUs) / 1000.0f;
        timing.fps = timing.frameTimeMs > 0.0f ? 1000.0f / timing.frameTimeMs : 0.0f;

        m_frameHistory.push_back(timing);
        m_totalFrameTime += static_cast<int64_t>(timing.frameTimeMs);

        // Keep only the most recent windowSize frames
        while (m_frameHistory.size() > m_windowSize) {
            m_totalFrameTime -= static_cast<int64_t>(m_frameHistory.front().frameTimeMs);
            m_frameHistory.pop_front();
        }
    }

    /**
     * @brief Get average frame time over the rolling window
     * @return Average frame time in milliseconds
     */
    float getAverageFrameTime() const {
        if (m_frameHistory.empty()) return 0.0f;
        return static_cast<float>(m_totalFrameTime) / static_cast<float>(m_frameHistory.size());
    }

    /**
     * @brief Get average FPS over the rolling window
     * @return Average frames per second
     */
    float getAverageFPS() const {
        float avgTime = getAverageFrameTime();
        return avgTime > 0.0f ? 1000.0f / avgTime : 0.0f;
    }

    /**
     * @brief Get minimum frame time in the rolling window
     * @return Minimum frame time in milliseconds
     */
    float getMinFrameTime() const {
        if (m_frameHistory.empty()) return 0.0f;
        float minTime = m_frameHistory[0].frameTimeMs;
        for (const auto& t : m_frameHistory) {
            if (t.frameTimeMs < minTime) minTime = t.frameTimeMs;
        }
        return minTime;
    }

    /**
     * @brief Get maximum frame time in the rolling window
     * @return Maximum frame time in milliseconds
     */
    float getMaxFrameTime() const {
        if (m_frameHistory.empty()) return 0.0f;
        float maxTime = m_frameHistory[0].frameTimeMs;
        for (const auto& t : m_frameHistory) {
            if (t.frameTimeMs > maxTime) maxTime = t.frameTimeMs;
        }
        return maxTime;
    }

    /**
     * @brief Get the Nth percentile frame time (e.g., 99th)
     * @param percentile Percentile value in [0, 100]
     * @return Frame time at the given percentile in milliseconds
     */
    float getPercentileFrameTime(float percentile = 99.0f) const {
        if (m_frameHistory.empty()) return 0.0f;
        std::deque<FrameTiming> sorted = m_frameHistory;
        std::sort(sorted.begin(), sorted.end(),
                  [](const FrameTiming& a, const FrameTiming& b) {
                      return a.frameTimeMs < b.frameTimeMs;
                  });
        size_t idx = static_cast<size_t>(
            std::min(percentile / 100.0f * static_cast<float>(sorted.size() - 1),
                     static_cast<float>(sorted.size() - 1)));
        return sorted[idx].frameTimeMs;
    }

    /**
     * @brief Get total number of recorded frames
     */
    size_t getFrameCount() const { return m_frameHistory.size(); }

    /**
     * @brief Get reference to recent frame history
     */
    const std::deque<FrameTiming>& getRecentFrames() const { return m_frameHistory; }

private:
    std::deque<FrameTiming> m_frameHistory;  ///< Rolling window of frame timings
    size_t m_windowSize;                     ///< Maximum frames to retain
    int64_t m_totalFrameTime;                ///< Sum of frame times for fast average
};

/**
  * @brief Renderer callback interface for the QML OpenGL viewport
  *
  * Implements the QQuickFramebufferObject::Renderer interface to provide
  * OpenGL rendering into a framebuffer object that QML can display.
  * Inherits QOpenGLFunctions to access OpenGL function pointers.
  */
 class QmlGlRenderer : public ::QQuickFramebufferObject::Renderer, public ::QOpenGLFunctions
 {
 public:
     /**
      * @brief Construct a new QML GL Renderer
      * @param viewportWidth Initial viewport width in pixels
      * @param viewportHeight Initial viewport height in pixels
      */
     explicit QmlGlRenderer(int viewportWidth = 1280, int viewportHeight = 720);
     ~QmlGlRenderer() override;

    // Disable copy, enable move
    QmlGlRenderer(const QmlGlRenderer&) = delete;
    QmlGlRenderer& operator=(const QmlGlRenderer&) = delete;
    QmlGlRenderer(QmlGlRenderer&&) = default;
    QmlGlRenderer& operator=(QmlGlRenderer&&) = default;

    /**
     * @brief Render the current frame
     *
     * Called by Qt's render thread. Performs OpenGL rendering of the
     * curvature grid, geodesics, and any active quantum geometry overlay.
     */
    void render() override;

    /**
     * @brief Create the framebuffer object for rendering
     * @return QOpenGLFramebufferObject* The created FBO
     */
    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override;

    /**
     * @brief Handle viewport size changes
     * @param size New viewport dimensions
     */
    void synchronize(QQuickFramebufferObject* item) override;

    /**
     * @brief Set the curvature renderer
     * @param renderer Shared pointer to the curvature renderer
     */
    void setCurvatureRenderer(std::shared_ptr<CurvatureRenderer> renderer);

    /**
     * @brief Set the quantum geometry renderer
     * @param renderer Shared pointer to the quantum geometry renderer
     */
    void setQuantumRenderer(std::shared_ptr<QuantumGeometryRenderer> renderer);

    /**
     * @brief Set the UI4D instance for scene data
     * @param ui4d Shared pointer to the UI4D instance
     */
    void setUI4D(std::shared_ptr<UI4D> ui4d);

    // Camera control
    void setViewMatrix(const QMatrix4x4& view);
    void setProjectionMatrix(const QMatrix4x4& proj);
    QMatrix4x4 getViewMatrix() const;
    QMatrix4x4 getProjectionMatrix() const;

    // Interaction
    void zoom(float factor);
    void rotate(float angleX, float angleY);
    void pan(float dx, float dy);
    void setSliceOffset(int viewIndex, double offset);

    // Rendering state
    void setShowGrid(bool show);
    void setShowLightCones(bool show);
    void setShowGeodesics(bool show);
    void setShowQuantumGeometry(bool show);
    void setCurvatureMode(int mode);

    // Time control
    void updateTime(float deltaTime);
    void resetTime();

    // Metrics
    float getFrameTime() const { return m_frameTime; }
    int getFrameCount() const { return m_frameCount; }

    // Frame profiler access for overlay rendering
    const FrameProfiler& getFrameProfiler() const { return m_frameProfiler; }

// Celestial body & camera adapters
    void setCelestialBodyRenderer(std::shared_ptr<CelestialBodyRenderer> renderer);
    void setCamera4DAdapter(std::shared_ptr<Camera4DAdapter> adapter);

    private:
    // OpenGL initialization helpers
    void initializeGL();
    void setupShaders();
    void setupGridGeometry();
    void setupAxisGizmo();
    void setupOverlayGeometry();

    // Rendering helpers
    void renderGrid();
    void renderAxisGizmo();
    void renderGeodesics();
    void renderQuantumGeometry();
    void renderOverlay();
    void renderProfilingOverlay();

    // Shader compilation
    bool compileShader(QOpenGLShaderProgram& program,
                       const char* vertexSource,
                       const char* fragmentSource);

    // Viewport dimensions
    int m_viewportWidth;
    int m_viewportHeight;

    // OpenGL resources
    QOpenGLShaderProgram m_gridShader;
    QOpenGLShaderProgram m_geodesicShader;
    QOpenGLShaderProgram m_overlayShader;
    GLuint m_gridVao;
    GLuint m_gridVbo;
    GLuint m_gridEbo;
    GLuint m_axisVao;
    GLuint m_axisVbo;

    // Overlay rendering resources (screen-space HUD)
    GLuint m_overlayVAO;
    GLuint m_overlayVBO;

    // Profiling overlay resources (dynamic frame-time bars)
    GLuint m_profilingVAO;
    GLuint m_profilingVBO;

    // Rendering state
    bool m_showGrid;
    bool m_showLightCones;
    bool m_showGeodesics;
    bool m_showQuantumGeometry;
    int m_curvatureMode;
    float m_time;
    float m_frameTime;
    int m_frameCount;

    // Matrices
    QMatrix4x4 m_viewMatrix;
    QMatrix4x4 m_projectionMatrix;

    // Camera state
    float m_cameraDistance;
    float m_cameraAngleX;
    float m_cameraAngleY;
    float m_cameraPanX;
    float m_cameraPanY;

    // Pointers to core renderers (non-owning)
    std::shared_ptr<CurvatureRenderer> m_curvatureRenderer;
    std::shared_ptr<QuantumGeometryRenderer> m_quantumRenderer;

    // Celestial body renderer (non-owning)
    std::shared_ptr<CelestialBodyRenderer> m_celestialBodyRenderer;

    // Camera4D adapter for 4D navigation (non-owning)
    std::shared_ptr<Camera4DAdapter> m_camera4DAdapter;

    // Frame profiler for benchmark/profiling overlay
    FrameProfiler m_frameProfiler;

    // Mutable renderer storage for createRenderer() const
    mutable std::shared_ptr<CurvatureRenderer> m_pendingCurvatureRenderer;
    mutable std::shared_ptr<QuantumGeometryRenderer> m_pendingQuantumRenderer;
    std::shared_ptr<UI4D> m_ui4d;

    // Framebuffer object
    QOpenGLFramebufferObject* m_fbo;
};

/**
 * @brief QML item that provides an OpenGL viewport
 *
 * Usage in QML:
 * \qml
 * import QuantumVerse 1.0
 * QmlGlViewport {
 *     id: viewport
 *     width: 800
 *     height: 600
 *     showGrid: true
 *     showGeodesics: true
 * }
 * \endqml
 */
class QmlGlViewport : public ::QQuickFramebufferObject
{
    Q_OBJECT
    Q_PROPERTY(bool showGrid READ showGrid WRITE setShowGrid NOTIFY showGridChanged)
    Q_PROPERTY(bool showLightCones READ showLightCones WRITE setShowLightCones NOTIFY showLightConesChanged)
    Q_PROPERTY(bool showGeodesics READ showGeodesics WRITE setShowGeodesics NOTIFY showGeodesicsChanged)
    Q_PROPERTY(bool showQuantumGeometry READ showQuantumGeometry WRITE setShowQuantumGeometry NOTIFY showQuantumGeometryChanged)
    Q_PROPERTY(int curvatureMode READ curvatureMode WRITE setCurvatureMode NOTIFY curvatureModeChanged)
    Q_PROPERTY(float cameraDistance READ cameraDistance WRITE setCameraDistance NOTIFY cameraDistanceChanged)
    Q_PROPERTY(float cameraAngleX READ cameraAngleX WRITE setCameraAngleX NOTIFY cameraAngleXChanged)
    Q_PROPERTY(float cameraAngleY READ cameraAngleY WRITE setCameraAngleY NOTIFY cameraAngleYChanged)
    Q_PROPERTY(float simulationTime READ simulationTime NOTIFY simulationTimeChanged)
    Q_PROPERTY(float frameRate READ frameRate NOTIFY frameRateChanged)

public:
    explicit QmlGlViewport(QQuickItem* parent = nullptr);
    ~QmlGlViewport() override;

    // QQuickFramebufferObject interface
    Renderer* createRenderer() const override;

    // Property accessors
    bool showGrid() const { return m_showGrid; }
    bool showLightCones() const { return m_showLightCones; }
    bool showGeodesics() const { return m_showGeodesics; }
    bool showQuantumGeometry() const { return m_showQuantumGeometry; }
    int curvatureMode() const { return m_curvatureMode; }
    float cameraDistance() const { return m_cameraDistance; }
    float cameraAngleX() const { return m_cameraAngleX; }
    float cameraAngleY() const { return m_cameraAngleY; }
    float simulationTime() const { return m_simulationTime; }
    float frameRate() const;

    // Exposed methods for QML
    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();
    Q_INVOKABLE void resetView();
    Q_INVOKABLE void setCurvatureRenderer(QObject* renderer);
    Q_INVOKABLE void setQuantumRenderer(QObject* renderer);
    Q_INVOKABLE void updateSimulation(double deltaTime);

    // Camera4D adapter access for QML toolbar
    Q_PROPERTY(QObject* camera4DAdapter READ camera4DAdapterObj WRITE setCamera4DAdapterObj NOTIFY camera4DAdapterChanged)

    // Direct C++ setters for shared_ptr renderers (bypasses QML type system)
    void setCurvatureRendererDirect(std::shared_ptr<CurvatureRenderer> renderer);
    void setQuantumRendererDirect(std::shared_ptr<QuantumGeometryRenderer> renderer);

    // Set UI4D instance for scene data access
    void setUI4D(std::shared_ptr<UI4D> ui4d);

    // Camera4D adapter access for QML toolbar
    QObject* camera4DAdapterObj() const;
    void setCamera4DAdapterObj(QObject* adapter);

    // Direct setter for CelestialBodyRenderer (bypasses QML type system)
    void setCelestialBodyRendererDirect(std::shared_ptr<CelestialBodyRenderer> renderer);

    // Direct setter for Camera4DAdapter (bypasses QML type system)
    void setCamera4DAdapterDirect(std::shared_ptr<Camera4DAdapter> adapter);

    // Set slice offset (called from QML)
    Q_INVOKABLE void setSliceOffset(int viewIndex, double offset);

signals:
    void showGridChanged();
    void showLightConesChanged();
    void showGeodesicsChanged();
    void showQuantumGeometryChanged();
    void curvatureModeChanged();
    void cameraDistanceChanged();
    void cameraAngleXChanged();
    void cameraAngleYChanged();
    void simulationTimeChanged();
    void frameRateChanged();
    void viewportInitialized();
    void curvatureRendererChanged();
    void quantumRendererChanged();
    void camera4DAdapterChanged();

public slots:
    void setShowGrid(bool show);
    void setShowLightCones(bool show);
    void setShowGeodesics(bool show);
    void setShowQuantumGeometry(bool show);
    void setCurvatureMode(int mode);
    void setCameraDistance(float dist);
    void setCameraAngleX(float angle);
    void setCameraAngleY(float angle);
    void handleMousePress(float x, float y, int button);
    void handleMouseMove(float x, float y, int buttons);
    void handleMouseReleased(float x, float y, int button);
    void handleWheel(float delta);

private:
    mutable QmlGlRenderer* m_renderer;
    bool m_showGrid;
    bool m_showLightCones;
    bool m_showGeodesics;
    bool m_showQuantumGeometry;
    int m_curvatureMode;
    float m_cameraDistance;
    float m_cameraAngleX;
    float m_cameraAngleY;
    float m_simulationTime;
    float m_frameRate;
    int m_frameCount;
    qint64 m_lastFrameTime;

    // Pointers to core renderers (non-owning)
    std::shared_ptr<CurvatureRenderer> m_curvatureRenderer;
    std::shared_ptr<QuantumGeometryRenderer> m_quantumRenderer;

    // Celestial body renderer (non-owning)
    std::shared_ptr<CelestialBodyRenderer> m_celestialBodyRenderer;

    // UI4D instance for scene data (non-owning)
    std::shared_ptr<UI4D> m_ui4d;

    // Camera4D adapter for 4D navigation (non-owning)
    std::shared_ptr<Camera4DAdapter> m_camera4DAdapter;
};

} // namespace quantumverse

#endif // QMLGLVIEWPORT_H