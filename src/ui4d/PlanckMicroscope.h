#pragma once

// Qt-dependent PlanckMicroscope class - must be outside quantumverse namespace
// to prevent Qt types from being placed in quantumverse:: namespace
// This is critical for Qt 6.11+ which includes STL headers internally

#ifdef QUANTUMVERSE_USE_QML
#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QOpenGLWidget>
#include <QTimer>
#endif

#include <memory>
#include <vector>
#include <map>
#include <string>

// Forward declarations
namespace quantumverse {
class CurvatureRenderer;
class QuantumGravityPlugin;
class Event4D;
class MetricTensor;
}

#ifdef QUANTUMVERSE_USE_QML

/**
 * @class PlanckMicroscope
 * @brief A specialized 4D viewport for Planck-scale exploration (Qt version)
 *
 * This widget combines:
 * - A logarithmic zoom slider (range: 10^-35 to 10^0 meters)
 * - An OpenGL viewport for rendering quantum geometry
 * - Info panel showing current scale, quantum effects, theory-specific data
 * - LOD (Level of Detail) controller for smooth transitions
 */
class PlanckMicroscope : public QWidget
{
    Q_OBJECT

private:
    // UI Components
    QSlider* zoomSlider;           // Logarithmic zoom: -35 to 0 (log10(meters))
    QLabel* currentScaleLabel;     // Display current scale (e.g., "10^-20 m")
    QLabel* quantumEffectsLabel;   // Description of Planck-scale effects
    QPushButton* resetButton;      // Reset to classical scale (1m)
    QPushButton* planckButton;     // Jump to Planck scale (10^-35 m)

    QGroupBox* infoPanel;          // Side panel with theory info
    QOpenGLWidget* glViewport;     // OpenGL rendering area

    QTimer* updateTimer;           // For smooth LOD transitions

    // State
    double currentLogScale;        // log10(scale in meters), range: [-35, 0]
    double targetLogScale;         // For animated transitions
    bool isAnimating;              // Whether LOD transition in progress

    // Quantum gravity theory currently displayed
    std::shared_ptr<quantumverse::QuantumGravityPlugin> activeTheory;

    // Rendering components
    std::unique_ptr<quantumverse::CurvatureRenderer> renderer;

    // LOD configuration
    struct LODLevel {
        double minScale;      // Minimum scale for this LOD (log10)
        double maxScale;      // Maximum scale for this LOD (log10)
        int detailLevel;      // Geometry detail: 0=low, 1=med, 2=high, 3=ultra
        bool showQuantumFoam; // Whether to show quantum foam structure
        std::string description;
    };
    std::vector<LODLevel> lodLevels;

    // Current LOD index
    int currentLOD;

    // Theory-specific visualization data
    struct TheoryVisualization {
        std::vector<double> vertices;   // 4D vertices (t,x,y,z)
        std::vector<int> edges;         // Edge indices
        std::vector<double> colors;     // RGBA per vertex
        double scale;                   // Characteristic scale
    };
    std::map<std::string, TheoryVisualization> theoryData;

    // Internal methods
    void updateScaleDisplay();
    void updateLOD();
    void generateQuantumFoamGeometry();
    void generateClassicalGeometry();
    void updateInfoPanel();
    void animateToTargetScale();
    double scaleFromLog(double logScale) const { return std::pow(10.0, logScale); }
    double logFromScale(double scale) const { return std::log10(scale); }

public:
    explicit PlanckMicroscope(QWidget* parent = nullptr);
    ~PlanckMicroscope() override;

    // Set the active quantum gravity theory for visualization
    void setActiveTheory(std::shared_ptr<quantumverse::QuantumGravityPlugin> theory);

    // Get current scale in meters
    double getCurrentScale() const { return scaleFromLog(currentLogScale); }

    // Set scale directly (will update slider and regenerate geometry)
    void setScale(double scale_meters);

    // Zoom by factor (multiply current scale)
    void zoom(double factor);

    // Get the OpenGL viewport for rendering integration
    QOpenGLWidget* getViewport() const { return glViewport; }

    // Get the curvature renderer
    quantumverse::CurvatureRenderer* getRenderer() const { return renderer.get(); }

    // Configuration
    void setShowQuantumFoam(bool show) { (void)show; /* TODO */ }
    void setColorByCurvature(bool curvature) { (void)curvature; /* TODO */ }
    void setAnimationSpeed(int fps) { (void)fps; /* TODO */ }

    // Public accessors
    double getTargetScale() const { return scaleFromLog(targetLogScale); }
    int getCurrentLOD() const { return currentLOD; }
    std::string getCurrentTheoryName() const;

private slots:
    void onZoomSliderChanged(int value);
    void onResetClicked();
    void onPlanckClicked();
    void onUpdateTimer();
    void onTheoryChanged(const QString& theoryName);

signals:
    // Emitted when user changes scale significantly
    void scaleChanged(double scale_meters);
    // Emitted when LOD changes
    void lodChanged(int lodLevel);
    // Emitted when quantum foam becomes visible/invisible
    void quantumFoamToggled(bool visible);
};

// Provide quantumverse::PlanckMicroscope as an alias when using QML
namespace quantumverse {
using PlanckMicroscope = ::PlanckMicroscope;
} // namespace quantumverse

#else // QUANTUMVERSE_USE_QML not defined - stub version

/**
 * @class PlanckMicroscope
 * @brief Stub Planck Microscope (Qt unavailable)
 *
 * Minimal stub providing the same public interface when Qt6 is not available.
 * All methods are no-ops; this class exists only to satisfy link-time dependencies.
 */
class PlanckMicroscope
{
public:
    explicit PlanckMicroscope(void* parent = nullptr) { (void)parent; }
    ~PlanckMicroscope() = default;

    void setActiveTheory(std::shared_ptr<quantumverse::QuantumGravityPlugin> theory) { (void)theory; }
    double getCurrentScale() const { return 1.0; }
    void setScale(double /*scale_meters*/) {}
    void zoom(double /*factor*/) {}
    void* getViewport() const { return nullptr; }
    quantumverse::CurvatureRenderer* getRenderer() const { return nullptr; }
    void setShowQuantumFoam(bool) {}
    void setColorByCurvature(bool) {}
    void setAnimationSpeed(int) {}
    double getTargetScale() const { return 1.0; }
    int getCurrentLOD() const { return 0; }
    std::string getCurrentTheoryName() const { return "none"; }

private:
    double currentLogScale = 0.0;
    double targetLogScale = 0.0;
    int currentLOD = 0;
    std::string currentTheoryName = "none";
};

// Provide quantumverse::PlanckMicroscope as an alias when Qt is unavailable
namespace quantumverse {
using PlanckMicroscope = ::PlanckMicroscope;
} // namespace quantumverse

#endif // QUANTUMVERSE_USE_QML