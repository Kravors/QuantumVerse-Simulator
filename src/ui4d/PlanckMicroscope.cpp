/**
 * @file PlanckMicroscope.cpp
 * @brief Implementation of Planck Microscope UI Widget
 */

// Qt headers MUST be included before opening namespace quantumverse,
// because Qt uses QT_BEGIN_NAMESPACE / QT_END_NAMESPACE macros internally.
#ifdef QUANTUMVERSE_USE_QML
#include <QString>
#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QOpenGLWidget>
#include <QTimer>
#include <QMouseEvent>
#endif

#include "PlanckMicroscope.h"
#include <string>
#include <cmath>

// Ensure M_PI is available on all platforms (MSVC <cmath> may not define it)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef QUANTUMVERSE_USE_QML
// Full includes for types used in implementation (forward declarations in header are in global namespace)
#include "rendering/CurvatureRenderer.h"
#include "discovery/DiscoveryEngine.h"
#include "quantumgravity/SpinFoamEngine.h"
#include "quantumgravity/CDTEngine.h"
#include "quantumgravity/GFTEngine.h"
#include "quantumgravity/CausalSet.h"

PlanckMicroscope::PlanckMicroscope(QWidget* parent)
    : QWidget(parent),
      currentLogScale(0.0),      // Start at 1m (10^0)
      targetLogScale(0.0),
      isAnimating(false),
      currentLOD(0)
{
    // Initialize LOD levels
    lodLevels = {
        {-35.0, -30.0, 3, true, "Planck foam (quantum gravity regime)"},      // LOD 3: Ultra (Planck scale)
        {-30.0, -20.0, 2, true, "Quantum foam (discrete geometry)"},         // LOD 2: High
        {-20.0, -10.0, 1, false, "Quantum corrections visible"},            // LOD 1: Medium
        {-10.0,  0.0, 0, false, "Classical continuum"}                      // LOD 0: Low
    };

    // Set up UI
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Title
    QLabel* titleLabel = new QLabel("<h2>Planck Microscope</h2>");
    mainLayout->addWidget(titleLabel);

    // Zoom control panel
    QGroupBox* zoomControlBox = new QGroupBox("Zoom Control");
    QVBoxLayout* zoomLayout = new QVBoxLayout(zoomControlBox);

    // Logarithmic slider: -35 to 0 (log10(m))
    zoomSlider = new QSlider(Qt::Horizontal);
    zoomSlider->setRange(-35, 0);
    zoomSlider->setValue(0);
    zoomSlider->setTickInterval(5);
    zoomSlider->setTickPosition(QSlider::TicksBelow);
    connect(zoomSlider, &QSlider::valueChanged, this, &PlanckMicroscope::onZoomSliderChanged);
    zoomLayout->addWidget(zoomSlider);

    // Zoom buttons row
    QHBoxLayout* zoomButtonsLayout = new QHBoxLayout();
    resetButton = new QPushButton("Reset (1m)");
    planckButton = new QPushButton("Planck (10^-35 m)");
    connect(resetButton, &QPushButton::clicked, this, &PlanckMicroscope::onResetClicked);
    connect(planckButton, &QPushButton::clicked, this, &PlanckMicroscope::onPlanckClicked);
    zoomButtonsLayout->addWidget(resetButton);
    zoomButtonsLayout->addWidget(planckButton);
    zoomLayout->addLayout(zoomButtonsLayout);

    // Current scale display
    currentScaleLabel = new QLabel("<b>Scale: 1.0 m</b>");
    zoomLayout->addWidget(currentScaleLabel);

    mainLayout->addWidget(zoomControlBox);

    // Info panel
    infoPanel = new QGroupBox("Quantum Effects");
    QVBoxLayout* infoLayout = new QVBoxLayout(infoPanel);
    quantumEffectsLabel = new QLabel("No active theory");
    quantumEffectsLabel->setWordWrap(true);
    infoLayout->addWidget(quantumEffectsLabel);
    mainLayout->addWidget(infoPanel);

    // OpenGL viewport
    glViewport = new QOpenGLWidget(this);
    glViewport->setMinimumSize(400, 300);
    mainLayout->addWidget(glViewport, 1);  // Stretch factor 1

    // Update timer for smooth LOD transitions (30 FPS)
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &PlanckMicroscope::onUpdateTimer);
    updateTimer->start(33);  // ~30 FPS

    // Initialize renderer
    renderer = std::make_unique<quantumverse::CurvatureRenderer>();
    // Defer OpenGL initialization to when a valid GL context is available.
    // QOpenGLWidget::aboutToCompose fires just before rendering with a live context.
    connect(glViewport, &QOpenGLWidget::aboutToCompose, this, [this]() {
        if (renderer && !renderer->isInitialized()) {
            renderer->initializeGL();
        }
    });

    // Initial display update
    updateScaleDisplay();
    updateInfoPanel();
}

PlanckMicroscope::~PlanckMicroscope() = default;

void PlanckMicroscope::setActiveTheory(std::shared_ptr<quantumverse::QuantumGravityPlugin> theory)
{
    activeTheory = theory;

    // Generate visualization data for this theory
    if (theory) {
        std::string name = theory->getName();
        TheoryVisualization vis;

        // Get geometry from theory
        vis.vertices = theory->getGeometryVertices();
        vis.edges = theory->getGeometryEdges();
        vis.colors = theory->getGeometryColors();

        // Store
        theoryData[name] = vis;

        // Update info panel
        updateInfoPanel();
    }
}

void PlanckMicroscope::setScale(double scale_meters)
{
    if (scale_meters <= 0) return;

    double logScale = logFromScale(scale_meters);
    // Clamp to valid range
    logScale = std::max(-35.0, std::min(0.0, logScale));

    targetLogScale = logScale;
    zoomSlider->setValue(static_cast<int>(logScale));

    // If not animating, update immediately
    if (!isAnimating) {
        currentLogScale = targetLogScale;
        updateScaleDisplay();
        updateLOD();
    }
}

void PlanckMicroscope::zoom(double factor)
{
    double newScale = getCurrentScale() * factor;
    setScale(newScale);
}

void PlanckMicroscope::onZoomSliderChanged(int value)
{
    targetLogScale = static_cast<double>(value);
    isAnimating = true;  // Will animate to target
}

void PlanckMicroscope::onResetClicked()
{
    setScale(1.0);  // 1 meter
}

void PlanckMicroscope::onPlanckClicked()
{
    setScale(1.0e-35);  // Planck length
}

void PlanckMicroscope::onTheoryChanged(const QString& theoryName)
{
    // Called when theory selection changes in QML
    // For now, just update the info panel
    Q_UNUSED(theoryName);
    updateInfoPanel();
}

void PlanckMicroscope::onUpdateTimer()
{
    if (isAnimating) {
        // Smooth interpolation towards target
        double diff = targetLogScale - currentLogScale;
        if (std::abs(diff) < 0.1) {
            currentLogScale = targetLogScale;
            isAnimating = false;
        } else {
            currentLogScale += diff * 0.1;  // 10% per frame
        }
        updateScaleDisplay();
        updateLOD();
    }
}

void PlanckMicroscope::updateScaleDisplay()
{
    double scale = scaleFromLog(currentLogScale);

    // Format scale in scientific notation
    QString text = QString("<b>Scale: %1 m</b>").arg(
        scale >= 1.0 ? QString::number(scale, 'f', 2) :
        QString::number(scale, 'e', 2)
    );
    currentScaleLabel->setText(text);

    // Emit signal
    emit scaleChanged(scale);
}

void PlanckMicroscope::updateLOD()
{
    // Determine LOD from current log scale
    int newLOD = 0;
    for (size_t i = 0; i < lodLevels.size(); ++i) {
        if (currentLogScale >= lodLevels[i].minScale &&
            currentLogScale <= lodLevels[i].maxScale) {
            newLOD = lodLevels[i].detailLevel;
            break;
        }
    }

    if (newLOD != currentLOD) {
        currentLOD = newLOD;
        emit lodChanged(currentLOD);

        // Update quantum foam visibility based on LOD
        bool showFoam = false;
        for (const auto& lod : lodLevels) {
            if (lod.detailLevel == currentLOD && lod.showQuantumFoam) {
                showFoam = true;
                break;
            }
        }
        emit quantumFoamToggled(showFoam);
    }

    // Regenerate geometry for new LOD
    if (activeTheory) {
        std::string name = activeTheory->getName();
        auto it = theoryData.find(name);
        if (it != theoryData.end()) {
            // For now, just use stored geometry
            // In full implementation, would downsample/upsample based on LOD
            // renderer->setGeometry(it->second.vertices, it->second.edges, it->second.colors);
        }
    }
}

void PlanckMicroscope::updateInfoPanel()
{
    if (!activeTheory) {
        quantumEffectsLabel->setText("No active quantum gravity theory selected.");
        return;
    }

    std::string name = activeTheory->getName();
    double scale = getCurrentScale();

    QString info = QString("<b>Theory:</b> %1\n").arg(QString::fromStdString(name));
    info += QString("<b>Current scale:</b> %2 m\n").arg(
        scale >= 1.0 ? QString::number(scale, 'f', 2) :
        QString::number(scale, 'e', 2)
    );

    // Add theory-specific quantum effects
    if (scale < 1.0e-19) {  // Below atomic scale
        info += "<br><b>Quantum effects active:</b><ul>";
        if (name.find("Spin Foam") != std::string::npos ||
            name.find("LQG") != std::string::npos) {
            info += "<li>Discrete spin network geometry</li>";
            info += "<li>Area/volume quantization</li>";
            info += "<li>Planck-scale discreteness (~10^-35 m)</li>";
        }
        if (name.find("CDT") != std::string::npos) {
            info += "<li>Triangulated spacetime</li>";
            info += "<li>Dynamical topology changes</li>";
            info += "<li>Spectral dimension running</li>";
        }
        if (name.find("GFT") != std::string::npos) {
            info += "<li>Group field condensate</li>";
            info += "<li>Emergent spacetime from φ≠0</li>";
        }
        if (name.find("Causal") != std::string::npos) {
            info += "<li>Discrete causal poset</li>";
            info += "<li>Locally finite order relation</li>";
        }
        info += "</ul>";
    } else {
        info += "<br><b>Regime:</b> Classical/quantum crossover";
    }

    // Add LOD info
    info += QString("<br><b>Detail level:</b> %1").arg(currentLOD);

    quantumEffectsLabel->setText(info);
}

void PlanckMicroscope::generateQuantumFoamGeometry()
{
    // Generate a procedural quantum foam texture at Planck scale
    // This is a placeholder; real implementation would use theory-specific data
    std::vector<double> vertices;
    std::vector<int> edges;
    std::vector<double> colors;

    // Simple tetrahedral foam (will be replaced by actual theory data)
    double a = 1.0e-35;  // Planck scale
    std::vector<std::array<double,4>> pos = {
        {0.0,  a,  a,  a},
        {0.0,  a, -a, -a},
        {0.0, -a,  a, -a},
        {0.0, -a, -a,  a}
    };

    for (const auto& p : pos) {
        vertices.push_back(p[0]);
        vertices.push_back(p[1]);
        vertices.push_back(p[2]);
        vertices.push_back(p[3]);
    }

    edges = {0,1, 0,2, 0,3, 1,2, 1,3, 2,3};
    colors = {1.0, 0.0, 0.0, 1.0,   // red
              0.0, 1.0, 0.0, 1.0,   // green
              0.0, 0.0, 1.0, 1.0,   // blue
              1.0, 1.0, 0.0, 1.0};  // yellow

    // Pass to renderer
    // renderer->setGeometry(vertices, edges, colors);
}

void PlanckMicroscope::generateClassicalGeometry()
{
    // Generate smooth classical geometry (e.g., sphere or grid)
    // Placeholder
}

std::string PlanckMicroscope::getCurrentTheoryName() const
{
    return activeTheory ? activeTheory->getName() : std::string{};
}

#endif // QUANTUMVERSE_USE_QML