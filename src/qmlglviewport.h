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

#include "glad.h"

// Qt headers MUST be included outside any namespace, because Qt uses
// QT_BEGIN_NAMESPACE / QT_END_NAMESPACE macros that break when nested
// inside namespace quantumverse {}.
#include <QQuickFramebufferObject>
#include <QQuickItem>
#include <QSGSimpleTextureNode>
#include <QSGTexture>
#include <QOpenGLFramebufferObject>

#include <QMatrix4x4>
#include <QDateTime>
#include <QOpenGLContext>
#include <QDebug>
#include <QVariant>
#include <memory>
#include <vector>
#include <deque>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <array>
#include <atomic>

#include "ml/SurrogateIntegration.h"
#ifdef QUANTUMVERSE_USE_VR
#include "vr/OpenXRBackend.h"
#endif

// Forward declarations to avoid Qt header conflicts with renderer includes
// These are in the quantumverse namespace
namespace quantumverse {
class CurvatureRenderer;
class QuantumGeometryRenderer;
class UI4D;
class Camera4D;
class Camera4DAdapter;
class CelestialBodyRenderer;
class MetricTensor;
class PostProcess;
class GravitationalLensing;
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
     * Uses GLAD for OpenGL function loading (no QOpenGLFunctions inheritance
     * to avoid shadowing global GLAD functions).
     */
    class QmlGlRenderer : public ::QQuickFramebufferObject::Renderer
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
    void setViewportSize(int width, int height);

    // VR head tracking
#ifdef QUANTUMVERSE_USE_VR
    void setHeadPose(const quantumverse::vr::HeadPose& pose);
    void applyHeadPose();
    bool hasHeadPose() const { return m_hasHeadPose; }

    // Stereo VR rendering
    void setVRActive(bool active);
    bool isVRActive() const { return m_vrActive; }
    void setVRConfig(const quantumverse::vr::VRConfig& config) { m_vrConfig = config; }
    const quantumverse::vr::VRConfig& vrConfig() const { return m_vrConfig; }
    QOpenGLFramebufferObject* vrFbo(quantumverse::vr::StereoEye eye) const;
    void renderVRStereoPass(quantumverse::vr::StereoEye eye,
                            const QMatrix4x4& viewMatrix,
                            const QMatrix4x4& projMatrix);
    void cleanupVRResources();
#endif

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
    void setShowHUD(bool show);
    void setCurvatureMode(int mode);

    // Texture source
    enum class TextureSource { Procedural, FileFirst };
    void setTextureSource(TextureSource source) { m_textureSource = source; }
    TextureSource textureSource() const { return m_textureSource; }

    // Time control
    void updateTime(float deltaTime);
    void resetTime();
    void setSimulationTime(float time);

    // Metrics
    float getFrameTime() const { return m_frameTime; }
    int getFrameCount() const { return m_frameCount; }

    // Frame profiler access for overlay rendering and tests
    const FrameProfiler& getFrameProfiler() const { return m_frameProfiler; }

    // Headless performance baseline support
    void setHeadlessFrameTarget(int frames);
    bool headlessTargetReached() const { return m_headlessStatsLogged; }
    int getHeadlessFrameCount() const { return m_frameCount; }

    // Headless screenshot support
    void requestScreenshot(const QString &path);
    bool screenshotRequested() const { return m_screenshotRequested; }
    QString screenshotPath() const { return m_screenshotPath; }
    void clearScreenshotRequest() { m_screenshotRequested = false; }

    // Headless frame-time dump support (--dump-frame-times)
    void setFrameTimesPath(const QString &path);
    QString frameTimesPath() const { return m_frameTimesPath; }

     // Celestial body & camera adapters
     void setCelestialBodyRenderer(std::shared_ptr<CelestialBodyRenderer> renderer);
     void setCamera4DAdapter(std::shared_ptr<Camera4DAdapter> adapter);
     void setLensingRenderer(std::shared_ptr<GravitationalLensing> lensing);
     void renderHUD();

    private:
    // OpenGL initialization helpers
    void initializeGL();
    void setupShaders();
    void setupGridGeometry();
    void setupAxisGizmo();
    void setupOverlayGeometry();
    void setupHUDGeometry();

     // Rendering helpers
     void renderGrid();
     void renderAxisGizmo();
     void renderGeodesics();
     void renderQuantumGeometry();
     void renderOverlay();
     void renderProfilingOverlay();
     void renderLensing();

    // Shader compilation (raw OpenGL to avoid threading issues)
    static bool compileShader(GLuint& program, const char* vertexSource, const char* fragmentSource);

    // Viewport dimensions
    int m_viewportWidth;
    int m_viewportHeight;

    // OpenGL resources (raw GL handles to avoid threading issues)
    GLuint m_gridShaderProgram = 0;
    GLuint m_geodesicShaderProgram = 0;
    GLuint m_overlayShaderProgram = 0;
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

    // Persistent HUD quad VAOs/VBOs (created once in initializeGL, reused per frame)
    GLuint m_hudBgVao = 0;
    GLuint m_hudBgVbo = 0;
    GLuint m_hudFpsVao = 0;
    GLuint m_hudFpsVbo = 0;
    GLuint m_hudErrVao = 0;
    GLuint m_hudErrVbo = 0;
    bool m_hudInitialized = false;

    // Rendering state
    bool m_showGrid;
    bool m_showLightCones;
    bool m_showGeodesics;
    bool m_showQuantumGeometry;
    int m_curvatureMode;
    TextureSource m_textureSource = TextureSource::Procedural;
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

    // VR head tracking state
#ifdef QUANTUMVERSE_USE_VR
     // Multi-user remote participants (ghost cameras)
     struct RemoteParticipant {
         std::string id;
         std::string name;
         std::array<float, 16> cameraMatrix;
         std::array<float, 3> position;
         bool vrActive = false;
         double lastUpdateTime = 0.0;
     };
     std::vector<RemoteParticipant> m_remoteParticipants;
     bool m_showGhostCameras = true;

     Q_PROPERTY(bool showGhostCameras READ showGhostCameras WRITE setShowGhostCameras NOTIFY showGhostCamerasChanged)
     Q_INVOKABLE void updateRemoteParticipant(const QString& id, const QString& name, const QVariantList& cameraMatrix, const QVariantList& position, bool vrActive);
     Q_INVOKABLE void removeRemoteParticipant(const QString& id);
#endif

    // Pointers to core renderers (non-owning)
    std::shared_ptr<CurvatureRenderer> m_curvatureRenderer;
    std::shared_ptr<QuantumGeometryRenderer> m_quantumRenderer;

    // Celestial body renderer (non-owning)
    std::shared_ptr<CelestialBodyRenderer> m_celestialBodyRenderer;
    bool m_celestialTexturesInitialized = false;

    // Camera4D adapter for 4D navigation (non-owning)
    std::shared_ptr<Camera4DAdapter> m_camera4DAdapter;

    // Surrogate integration for real-time geodesic prediction (deferred:
    // SurrogateIntegration depends on geometry/BVH.h which is not yet present)
#if 0
    std::unique_ptr<quantumverse::ml::SurrogateIntegration> m_surrogateIntegration;
#endif

    // Frame profiler for benchmark/profiling overlay
    FrameProfiler m_frameProfiler;

    // Headless baseline target
    int m_headlessTargetFrames;
    bool m_headlessStatsLogged;

    // Headless screenshot state
    bool m_screenshotRequested;
    QString m_screenshotPath;
    QString m_frameTimesPath;

    // Mutable renderer storage for createRenderer() const
    mutable std::shared_ptr<CurvatureRenderer> m_pendingCurvatureRenderer;
    mutable std::shared_ptr<QuantumGeometryRenderer> m_pendingQuantumRenderer;
    std::shared_ptr<UI4D> m_ui4d;

    // Framebuffer object
    QOpenGLFramebufferObject* m_fbo;

    // Post-processing pipeline (HDR bloom + tone mapping)
     std::shared_ptr<PostProcess> m_postProcess;

     // Gravitational lensing renderer
     std::shared_ptr<GravitationalLensing> m_lensing;

     // One-time GL initialization guard
     bool m_glInitialized;

    // H5: cached scaled-orbit worldlines for renderGeodesics(). Rebuilt only
    // when the solar system data signature changes, avoiding per-frame
    // allocation churn of vector<vector<Event4D>>.
    std::vector<std::vector<Event4D>> m_cachedGeodesics;
    size_t m_geodesicsCacheKey = 0;
    const UI4D* m_cachedGeodesicsUi4d = nullptr;
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
    Q_PROPERTY(bool showHUD READ showHUD WRITE setShowHUD NOTIFY showHUDChanged)
    Q_PROPERTY(int curvatureMode READ curvatureMode WRITE setCurvatureMode NOTIFY curvatureModeChanged)

    // Gravitational lensing properties
    Q_PROPERTY(bool lensingEnabled READ lensingEnabled WRITE setLensingEnabled NOTIFY lensingEnabledChanged)
    Q_PROPERTY(int lensingSteps READ lensingSteps WRITE setLensingSteps NOTIFY lensingStepsChanged)
    Q_PROPERTY(float shadowIntensity READ shadowIntensity WRITE setShadowIntensity NOTIFY shadowIntensityChanged)
    Q_PROPERTY(float lensingMass READ lensingMass WRITE setLensingMass NOTIFY lensingMassChanged)
    Q_PROPERTY(float lensingSpin READ lensingSpin WRITE setLensingSpin NOTIFY lensingSpinChanged)
    Q_PROPERTY(float lensingDistance READ lensingDistance WRITE setLensingDistance NOTIFY lensingDistanceChanged)
    Q_PROPERTY(bool accretionDiskEnabled READ accretionDiskEnabled WRITE setAccretionDiskEnabled NOTIFY accretionDiskEnabledChanged)
    Q_PROPERTY(bool photonRingEnabled READ photonRingEnabled WRITE setPhotonRingEnabled NOTIFY photonRingEnabledChanged)
    Q_PROPERTY(float accretionDiskIntensity READ accretionDiskIntensity WRITE setAccretionDiskIntensity NOTIFY accretionDiskIntensityChanged)
    Q_PROPERTY(float cameraDistance READ cameraDistance WRITE setCameraDistance NOTIFY cameraDistanceChanged)
    Q_PROPERTY(float cameraAngleX READ cameraAngleX WRITE setCameraAngleX NOTIFY cameraAngleXChanged)
    Q_PROPERTY(float cameraAngleY READ cameraAngleY WRITE setCameraAngleY NOTIFY cameraAngleYChanged)
    Q_PROPERTY(float simulationTime READ simulationTime NOTIFY simulationTimeChanged)
    Q_PROPERTY(float frameRate READ frameRate NOTIFY frameRateChanged)
#ifdef QUANTUMVERSE_USE_VR
    Q_PROPERTY(bool vrEnabled READ vrEnabled WRITE setVrEnabled NOTIFY vrEnabledChanged)
    Q_PROPERTY(bool vrActive READ vrActive NOTIFY vrActiveChanged)
    Q_PROPERTY(float vrIpd READ vrIpd WRITE setVrIpd NOTIFY vrIpdChanged)
#endif

public:
    explicit QmlGlViewport(QQuickItem* parent = nullptr);
    ~QmlGlViewport() override;

    // QQuickFramebufferObject scene graph interface
    QQuickFramebufferObject::Renderer* createRenderer() const override;

    // Deterministic simulation time (bypasses wall-clock delta accumulation)
    void setSimulationTime(float time);
    float getSimulationTime() const { return m_simulationTime; }

    // Property accessors
    bool showGrid() const { return m_showGrid; }
    bool showLightCones() const { return m_showLightCones; }
    bool showGeodesics() const { return m_showGeodesics; }
    bool showQuantumGeometry() const { return m_showQuantumGeometry; }
    bool showHUD() const { return m_showHUD; }
    int curvatureMode() const { return m_curvatureMode; }
    float cameraDistance() const { return m_cameraDistance; }
    float cameraAngleX() const { return m_cameraAngleX; }
    float cameraAngleY() const { return m_cameraAngleY; }
    float simulationTime() const { return m_simulationTime; }
    float frameRate() const;
#ifdef QUANTUMVERSE_USE_VR
     bool vrEnabled() const { return m_vrEnabled; }
     bool vrActive() const { return m_vrActive; }
     float vrIpd() const { return m_vrIpd; }
     void setVrEnabled(bool enabled);
     void setVrActive(bool active);
     void setVrIpd(float ipd);
     bool showGhostCameras() const { return m_showGhostCameras; }
     void setShowGhostCameras(bool show);
#endif

    // --- Curvature probe readout (wired to the live metric) -------------
    Q_PROPERTY(QString kretschmann READ kretschmann NOTIFY probeChanged)
    Q_PROPERTY(QString ricciScalar READ ricciScalar NOTIFY probeChanged)
    Q_PROPERTY(QString weylSquared READ weylSquared NOTIFY probeChanged)
    Q_PROPERTY(QString redshift READ redshift NOTIFY probeChanged)
     Q_PROPERTY(bool probeValid READ probeValid NOTIFY probeChanged)
    Q_PROPERTY(bool planeMode READ isPlaneMode WRITE togglePlaneMode NOTIFY planeModeChanged)
    Q_PROPERTY(int planeResolution READ getPlaneResolution WRITE setPlaneResolution NOTIFY planeResolutionChanged)
    Q_PROPERTY(int textureSource READ textureSource WRITE setTextureSource NOTIFY textureSourceChanged)
    Q_PROPERTY(double blackHoleMass READ blackHoleMass WRITE setBlackHoleMass NOTIFY blackHoleMassChanged)
    Q_PROPERTY(double liveKineticEnergy READ liveKineticEnergy NOTIFY telemetryUpdated)
    Q_PROPERTY(double livePotentialEnergy READ livePotentialEnergy NOTIFY telemetryUpdated)
    Q_PROPERTY(double liveTotalEnergy READ liveTotalEnergy NOTIFY telemetryUpdated)
    Q_PROPERTY(double liveEarthSpeed READ liveEarthSpeed NOTIFY telemetryUpdated)
    Q_PROPERTY(double physicsG READ physicsG WRITE setPhysicsG NOTIFY physicsConstantsChanged)
    Q_PROPERTY(double physicsC READ physicsC WRITE setPhysicsC NOTIFY physicsConstantsChanged)
    Q_PROPERTY(double physicsGLog READ physicsGLog WRITE setPhysicsGLog NOTIFY physicsConstantsChanged)
     Q_PROPERTY(double gravitationalWaveStrain READ gravitationalWaveStrain NOTIFY telemetryUpdated)
     Q_PROPERTY(double gravitationalWaveFreq READ gravitationalWaveFreq NOTIFY telemetryUpdated)

     // Scenario system properties
     Q_PROPERTY(QString scenarioName READ scenarioName NOTIFY scenarioChanged)
     Q_PROPERTY(double scenarioTime READ scenarioTime NOTIFY scenarioChanged)
     Q_PROPERTY(bool scenarioPlaying READ scenarioPlaying NOTIFY scenarioChanged)
     Q_PROPERTY(QStringList scenarioList READ scenarioList NOTIFY scenarioListChanged)

     // Telemetry recording properties
     Q_PROPERTY(bool recording READ isRecording NOTIFY recordingChanged)
     Q_PROPERTY(bool playing READ isPlaying NOTIFY playbackChanged)
     Q_PROPERTY(int currentFrame READ currentFrame NOTIFY playbackChanged)
     Q_PROPERTY(int totalFrames READ totalFrames NOTIFY recordingChanged)
     Q_PROPERTY(double recordingDuration READ recordingDuration NOTIFY recordingChanged)

    // Post-processing / bloom controls
    Q_PROPERTY(bool bloomEnabled READ bloomEnabled WRITE setBloomEnabled NOTIFY bloomEnabledChanged)
    Q_PROPERTY(float bloomIntensity READ bloomIntensity WRITE setBloomIntensity NOTIFY bloomIntensityChanged)
    Q_PROPERTY(float bloomThreshold READ bloomThreshold WRITE setBloomThreshold NOTIFY bloomThresholdChanged)
    Q_PROPERTY(int toneMappingMode READ toneMappingMode WRITE setToneMappingMode NOTIFY toneMappingModeChanged)
    Q_PROPERTY(float exposure READ exposure WRITE setExposure NOTIFY exposureChanged)

    // Undo/Redo
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY undoRedoChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY undoRedoChanged)
    Q_PROPERTY(QString undoDescription READ undoDescription NOTIFY undoRedoChanged)
    Q_PROPERTY(QString redoDescription READ redoDescription NOTIFY undoRedoChanged)

    /// @brief Compute curvature scalars at a world position and update the readout.
    Q_INVOKABLE void probeAt(double x, double y, double z);
    /// @brief Reset the readout to its placeholder state.
    Q_INVOKABLE void clearProbe();
    /// @brief Provide the metric used for probe computations.
    void setProbeMetric(std::shared_ptr<MetricTensor> metric);

    QString kretschmann() const { return m_kretschmann; }
    QString ricciScalar() const { return m_ricciScalar; }
    QString weylSquared() const { return m_weylSquared; }
    QString redshift() const { return m_redshift; }
    bool probeValid() const { return m_probeValid; }

    // Exposed methods for QML
    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();
    Q_INVOKABLE void resetView();
    Q_INVOKABLE void setCurvatureRenderer(QObject* renderer);
    Q_INVOKABLE void setQuantumRenderer(QObject* renderer);
    Q_INVOKABLE void updateSimulation(double deltaTime);
    Q_INVOKABLE void togglePlaneMode(bool enable);
    Q_INVOKABLE bool isPlaneMode() const;
    Q_INVOKABLE void setPlaneResolution(int resolution);
    Q_INVOKABLE int getPlaneResolution() const;
    Q_INVOKABLE void reloadConfig();
    Q_INVOKABLE QString configStatus() const;
    Q_INVOKABLE void setTextureSource(int source);
    Q_INVOKABLE int textureSource() const;
    Q_INVOKABLE void setBlackHoleMass(double mass);
    Q_INVOKABLE double blackHoleMass() const;
    Q_INVOKABLE void setBlackHoleType(const QString& type);
    void setBlackHoleMassDirect(double mass); // For undo/redo (no command push)
    Q_INVOKABLE double liveKineticEnergy() const;
    Q_INVOKABLE double livePotentialEnergy() const;
    Q_INVOKABLE double liveTotalEnergy() const;
    Q_INVOKABLE double liveEarthSpeed() const;
    Q_INVOKABLE double physicsG() const;
    Q_INVOKABLE void setPhysicsG(double G);
    Q_INVOKABLE double physicsC() const;
    Q_INVOKABLE void setPhysicsC(double c);
    Q_INVOKABLE double physicsGLog() const;
    Q_INVOKABLE void setPhysicsGLog(double log10_G);
    Q_INVOKABLE double gravitationalWaveStrain() const;
    Q_INVOKABLE double gravitationalWaveFreq() const;
    Q_INVOKABLE void setAudioEnabled(bool enabled);
    Q_INVOKABLE bool audioEnabled() const;
    Q_INVOKABLE void setAudioVolume(double volume);
    Q_INVOKABLE double audioVolume() const;
    Q_INVOKABLE void setAudioSourcePosition(double x, double y, double z);
    Q_INVOKABLE void setAudioListenerPosition(double x, double y, double z);
    Q_INVOKABLE void setAudioListenerForward(double fx, double fy, double fz);
    Q_INVOKABLE void setAudioPan(double pan);
    Q_INVOKABLE void setWaveformMode(int mode);
    Q_INVOKABLE void injectAsteroid(double x, double y, double z, double vx, double vy, double vz, double mass);
     Q_INVOKABLE void clearAsteroids();
     Q_INVOKABLE int asteroidCount() const;

     // Scenario system methods
     Q_INVOKABLE void loadScenario(const QString& name);
     Q_INVOKABLE void playScenario();
     Q_INVOKABLE void pauseScenario();
     Q_INVOKABLE void stopScenario();
     Q_INVOKABLE void reloadScenarios();
     Q_INVOKABLE QStringList scenarioList();

     QString scenarioName() const;
     double scenarioTime() const;
     bool scenarioPlaying() const;

     // Telemetry recording & replay
     Q_INVOKABLE void startRecording(int maxFrames = 6000, int frameInterval = 1);
     Q_INVOKABLE void stopRecording();
     Q_INVOKABLE void playRecording();
     Q_INVOKABLE void pausePlayback();
     Q_INVOKABLE void stopPlayback();
     Q_INVOKABLE void scrubToFrame(int frameIndex);
     Q_INVOKABLE void scrubToTime(double time);
     Q_INVOKABLE void exportRecording(const QString& filepath);
    Q_INVOKABLE void clearRecording();
    bool isRecording() const;
    bool isPlaying() const;
    int currentFrame() const;
    int totalFrames() const;
    double recordingDuration() const;

    // Undo/Redo
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void clearUndoRedo();
    bool canUndo() const;
    bool canRedo() const;
    QString undoDescription() const;
    QString redoDescription() const;

    // Gravitational lensing accessors
    bool lensingEnabled() const { return m_lensingEnabled; }
    int lensingSteps() const { return m_lensingSteps; }
    float shadowIntensity() const { return m_shadowIntensity; }
    float lensingMass() const { return m_lensingMass; }
    float lensingSpin() const { return m_lensingSpin; }
    float lensingDistance() const { return m_lensingDistance; }
    bool accretionDiskEnabled() const { return m_accretionDiskEnabled; }
    bool photonRingEnabled() const { return m_photonRingEnabled; }
    float accretionDiskIntensity() const { return m_accretionDiskIntensity; }

    // Gravitational lensing QML methods
    Q_INVOKABLE void setLensingEnabled(bool enabled);
    Q_INVOKABLE void setLensingSteps(int steps);
    Q_INVOKABLE void setShadowIntensity(float intensity);
    Q_INVOKABLE void setLensingMass(float mass);
    Q_INVOKABLE void setLensingSpin(float spin);
    Q_INVOKABLE void setLensingDistance(float distance);
    Q_INVOKABLE void setAccretionDiskEnabled(bool enabled);
    Q_INVOKABLE void setPhotonRingEnabled(bool enabled);
    Q_INVOKABLE void setAccretionDiskIntensity(float intensity);
    Q_INVOKABLE void setLensingMetric(const QString& metricType);

     Q_INVOKABLE void setBloomEnabled(bool enabled);
     Q_INVOKABLE bool bloomEnabled() const;
     Q_INVOKABLE float bloomIntensity() const;
    Q_INVOKABLE void setBloomIntensity(float intensity);
    Q_INVOKABLE float bloomThreshold() const;
    Q_INVOKABLE void setBloomThreshold(float threshold);
    Q_INVOKABLE int toneMappingMode() const;
    Q_INVOKABLE void setToneMappingMode(int mode);
    Q_INVOKABLE float exposure() const;
    Q_INVOKABLE void setExposure(float exposure);

    // Camera4D adapter access for QML toolbar
    Q_PROPERTY(QObject* camera4DAdapter READ camera4DAdapterObj WRITE setCamera4DAdapterObj NOTIFY camera4DAdapterChanged)

    // Direct C++ setters for shared_ptr renderers (bypasses QML type system)
    void setCurvatureRendererDirect(std::shared_ptr<CurvatureRenderer> renderer);
    std::shared_ptr<CurvatureRenderer> curvatureRenderer() const { return m_curvatureRenderer; }

    void setQuantumRendererDirect(std::shared_ptr<QuantumGeometryRenderer> renderer);
    std::shared_ptr<QuantumGeometryRenderer> quantumGeometryRenderer() const { return m_quantumRenderer; }

    // Set UI4D instance for scene data access
    void setUI4D(std::shared_ptr<UI4D> ui4d);
    std::shared_ptr<UI4D> ui4d() const { return m_ui4d; }

    // Camera4D adapter access for QML toolbar
    QObject* camera4DAdapterObj() const;
    void setCamera4DAdapterObj(QObject* adapter);

    // Direct setter for CelestialBodyRenderer (bypasses QML type system)
    void setCelestialBodyRendererDirect(std::shared_ptr<CelestialBodyRenderer> renderer);
    std::shared_ptr<CelestialBodyRenderer> celestialBodyRenderer() const { return m_celestialBodyRenderer; }

    // Direct setter for Camera4DAdapter (bypasses QML type system)
    void setCamera4DAdapterDirect(std::shared_ptr<Camera4DAdapter> adapter);
    std::shared_ptr<Camera4DAdapter> camera4DAdapter() const { return m_camera4DAdapter; }

    // Post-processing pipeline access (for QmlGlRenderer synchronization)
     std::shared_ptr<PostProcess> postProcess() const { return m_postProcess; }

     // Gravitational lensing access
     void setLensingRenderer(std::shared_ptr<GravitationalLensing> lensing);
     std::shared_ptr<GravitationalLensing> lensingRenderer() const { return m_lensing; }

    void setHeadlessFrameTarget(int frames) {
        m_headlessTargetFrames = frames;
        if (m_renderer) m_renderer->setHeadlessFrameTarget(frames);
    }
    void setFrameTimesPath(const QString &path) {
        if (m_renderer) m_renderer->setFrameTimesPath(path);
    }
    bool headlessTargetReached() const { return m_renderer ? m_renderer->headlessTargetReached() : false; }

    void requestScreenshot(const QString &path) {
        m_pendingScreenshotRequested = true;
        m_pendingScreenshotPath = path;
        qDebug() << "[DEBUG] requestScreenshot() called, path=" << path << "flag=" << m_pendingScreenshotRequested.load();
    }
    bool screenshotRequested() const { return m_renderer ? m_renderer->screenshotRequested() : false; }

    // Set slice offset (called from QML)
    Q_INVOKABLE void setSliceOffset(int viewIndex, double offset);
#ifdef QUANTUMVERSE_USE_VR
      Q_INVOKABLE void toggleVR();
      Q_INVOKABLE void updateVRControllerInput();
      Q_INVOKABLE void updateRemoteParticipant(const QString& id, const QString& name, const QVariantList& cameraMatrix, const QVariantList& position, bool vrActive);
      Q_INVOKABLE void removeRemoteParticipant(const QString& id);
#endif

signals:
    void showGridChanged();
    void showLightConesChanged();
    void showGeodesicsChanged();
    void showQuantumGeometryChanged();
    void showHUDChanged();
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
    void probeChanged();
#ifdef QUANTUMVERSE_USE_VR
     void vrEnabledChanged();
     void vrActiveChanged();
     void vrIpdChanged();
     void showGhostCamerasChanged();
#endif
    void planeModeChanged();
    void planeResolutionChanged();
    void textureSourceChanged();
    void blackHoleMassChanged();
    void telemetryUpdated();
    void physicsConstantsChanged();
      void bloomEnabledChanged();
      void bloomIntensityChanged();
      void bloomThresholdChanged();
      void toneMappingModeChanged();
      void exposureChanged();

      // Undo/Redo signals
      void undoRedoChanged();

      // Gravitational lensing signals
      void lensingEnabledChanged();
      void lensingStepsChanged();
      void shadowIntensityChanged();
      void lensingMassChanged();
      void lensingSpinChanged();
      void lensingDistanceChanged();
      void accretionDiskEnabledChanged();
      void photonRingEnabledChanged();
      void accretionDiskIntensityChanged();

      void scenarioChanged();
     void scenarioListChanged();

     // Telemetry recording signals
     void recordingChanged();
     void playbackChanged();

public slots:
    void setShowGrid(bool show);
    void setShowLightCones(bool show);
    void setShowGeodesics(bool show);
    void setShowQuantumGeometry(bool show);
    void setShowHUD(bool show);
    void setCurvatureMode(int mode);
    void setCameraDistance(float dist);
    void setCameraAngleX(float angle);
    void setCameraAngleY(float angle);
    void updateFrameRate(float fps);
    void handleMousePress(float x, float y, int button);
    void handleMouseMove(float x, float y, int buttons);
    void handleMouseReleased(float x, float y, int button);
    void handleWheel(float delta);

    // Gravitational lensing slots
    void syncLensingParams();

private:
    QmlGlRenderer* m_renderer;
    bool m_showGrid;
    bool m_showLightCones;
    bool m_showGeodesics;
    bool m_showQuantumGeometry;
    bool m_showHUD;
    int m_curvatureMode;
    float m_cameraDistance;
    float m_cameraAngleX;
    float m_cameraAngleY;
    float m_simulationTime;
    float m_frameRate;
    double m_blackHoleMass = 10.0;
    int m_frameCount;
    int m_headlessTargetFrames = 0;  // mirrors the renderer's target; gates the FBO->screen blit in headless/benchmark mode
    qint64 m_lastFrameTime;

    // FPS averaging state (measured from the actual render rate in renderGL)
    int m_fpsFrameCount = 0;
    qint64 m_fpsWindowStart = 0;

    // Pointers to core renderers (non-owning)
    std::shared_ptr<CurvatureRenderer> m_curvatureRenderer;
    std::shared_ptr<QuantumGeometryRenderer> m_quantumRenderer;

    // Celestial body renderer (non-owning)
    std::shared_ptr<CelestialBodyRenderer> m_celestialBodyRenderer;

    // UI4D instance for scene data (non-owning)
    std::shared_ptr<UI4D> m_ui4d;

    // Gravitational wave audio synthesizer
    class GravitationalWaveAudio* m_gwAudio = nullptr;
    bool m_audioEnabled = false;
    double m_audioVolume = 0.5;

    // Camera4D adapter for 4D navigation (non-owning)
    std::shared_ptr<Camera4DAdapter> m_camera4DAdapter;

    // Post-processing pipeline (HDR bloom + tone mapping)
    std::shared_ptr<PostProcess> m_postProcess;

    // Bloom state (mirrored to renderer)
    bool m_bloomEnabled = true;
    float m_bloomIntensity = 0.8f;
    float m_bloomThreshold = 1.0f;
    int m_toneMappingMode = 0;
    float m_exposure = 1.0f;

    // Probe readout state (driven by probeAt)
    std::shared_ptr<MetricTensor> m_probeMetric;
    QString m_kretschmann = "—";
    QString m_ricciScalar = "—";
    QString m_weylSquared = "—";
    QString m_redshift = "—";
    bool m_probeValid = false;

#ifdef QUANTUMVERSE_USE_VR
     // VR state
     bool m_vrEnabled = false;
     bool m_vrActive = false;
     float m_vrIpd = 0.063f;

     // Multi-user remote participants (ghost cameras)
     struct RemoteParticipant {
         std::string id;
         std::string name;
         std::array<float, 16> cameraMatrix;
         std::array<float, 3> position;
         bool vrActive = false;
         double lastUpdateTime = 0.0;
     };
     std::vector<RemoteParticipant> m_remoteParticipants;
     bool m_showGhostCameras = true;
#endif

    // Pending screenshot request, transferred to the render thread in
    // synchronize() (QQuickFramebufferObject owns the FBO, not this item).
    std::atomic<bool> m_pendingScreenshotRequested{false};
    QString m_pendingScreenshotPath;

     // The QQuickFramebufferObject renderer needs full access to the item's
     // renderer/camera/flag state to mirror it onto the render thread.
     class ViewportRenderer;
     friend class ViewportRenderer;

     // Scenario system state
     QStringList m_scenarioList;
     QString m_scenarioName;
     double m_scenarioTime = 0.0;
     bool m_scenarioPlaying = false;

     // Telemetry recording state
     bool m_recording = false;
     bool m_playing = false;
     int m_currentFrame = 0;
     int m_totalFrames = 0;
    double m_recordingDuration = 0.0;

    // Undo/Redo system
    struct Command {
        QString description;
        std::function<void()> undo;
        std::function<void()> redo;
    };
    std::vector<Command> m_undoStack;
    std::vector<Command> m_redoStack;
    int m_maxUndoSteps = 50;
    bool m_isUndoRedoAction = false; // Prevents recording undo/redo actions

    void pushCommand(const QString& description, std::function<void()> undo, std::function<void()> redo);
    void truncateUndoStack();
    void truncateRedoStack();

    // Gravitational lensing state
     bool m_lensingEnabled = false;
     int m_lensingSteps = 256;
     float m_shadowIntensity = 1.0f;
     float m_lensingMass = 1.0f;
     float m_lensingSpin = 0.6f;
     float m_lensingDistance = 10.0f;
     bool m_accretionDiskEnabled = true;
     bool m_photonRingEnabled = true;
     float m_accretionDiskIntensity = 0.8f;
     std::shared_ptr<GravitationalLensing> m_lensing;
     std::shared_ptr<MetricTensor> m_lensingMetric;

 private slots:
    // (renderGL / onWindowChanged removed: rendering now happens in the
    // QQuickFramebufferObject::Renderer created by createRenderer().)
};

} // namespace quantumverse

#endif // QMLGLVIEWPORT_H