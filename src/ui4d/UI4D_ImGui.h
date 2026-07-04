#pragma once

#include <imgui.h>
#include <implot.h>
#include <array>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace quantumverse {
namespace spacetime {
    struct Event4D;
    class MetricTensor;
}
namespace ui4d {
    class CurvatureRenderer;
    class CelestialBodyRenderer;
    class SceneGraphManager;
}
}

namespace quantumverse {
namespace ui_imgui {

struct UI4DConfig {
    bool showLightCones = true;
    bool showCausalStructure = true;
    bool showCurvatureGrid = true;
    bool showWorldLines = true;
    bool showTimeGeography = true;
    bool showTopologyIndicators = true;
    float curvatureScale = 1.0f;
    float lightConeTransparency = 0.3f;
    float worldLineThickness = 2.0f;
    int numSlices = 4;
    float sliceSpacing = 10.0f;
};

enum class TheoryType {
    SCHWARZSCHILD,
    KERR,
    WORMHOLE,
    NEWTONIAN,
    CDT,
    SPIN_FOAM,
    GFT,
    CAUSAL_SET
};

struct AnomalyAlert {
    std::string message;
    double severity;
    std::string type;
    double timestamp;
};

class TimelineBar {
public:
    TimelineBar();
    ~TimelineBar() = default;

    void render(ImVec2 availSize);
    void setTime(double time);
    double getTime() const;
    void setTimeRange(double minTime, double maxTime);
    void setPlaying(bool playing);
    bool isPlaying() const { return isPlaying_; }
    void setSpeed(float speed);
    float getSpeed() const { return playSpeed; }

    std::function<void()> onRewind;
    std::function<void()> onPlayPause;
    std::function<void()> onFastForward;
    std::function<void(float)> onSpeedChange;
    std::function<void(double)> onTimeScrub;

private:
    double currentTime = 0.0;
    double minTime_ = 0.0;
    double maxTime_ = 100.0;
    bool isPlaying_ = false;
    float playSpeed = 1.0f;
};

class ObjectBrowser {
public:
    ObjectBrowser();
    ~ObjectBrowser() = default;

    void render(ImVec2 availSize);
    void refresh();
    void selectObject(const std::string& id);
    void clearSelection();

    std::function<void(const std::string&)> onFlyToBody;

private:
    struct ObjectItem {
        std::string id;
        std::string name;
        std::string type;
        bool selected = false;
    };

    std::vector<ObjectItem> objects;
    std::string selectedObjectId;
    char filterText[256] = "";
};

class PropertyEditor {
public:
    PropertyEditor();
    ~PropertyEditor() = default;

    void render(ImVec2 availSize);
    void setObject(const std::string& id, const std::string& name, double mass, double radius,
                   const std::array<double, 3>& position);

    std::function<void(const std::string&, double, double, const std::array<double, 3>&)> onApply;

private:
    std::string objectId;
    std::string objectName;
    double mass = 0.0;
    double radius = 0.0;
    std::array<double, 3> position = {0.0, 0.0, 0.0};
};

class ContextMenu {
public:
    ContextMenu();
    ~ContextMenu() = default;

    void openAt(const ImVec2& position, const std::string& objectName);
    void render();
    bool isOpen() const { return isMenuOpen; }

    std::function<void()> onDropTestParticle;
    std::function<void()> onMeasurePrecession;
    std::function<void()> onComputeRedshift;

private:
    bool isMenuOpen = false;
    ImVec2 menuPosition;
    std::string objectName;
};

class TheorySelectorPanel {
public:
    TheorySelectorPanel();
    ~TheorySelectorPanel() = default;

    void render(ImVec2 availSize);
    void setCurrentTheory(TheoryType theory);
    TheoryType getCurrentTheory() const { return currentTheory; }
    void setVisible(bool visible) { showPanel = visible; }
    bool isVisible() const { return showPanel; }

    std::function<void(TheoryType)> onTheoryChange;

private:
    TheoryType currentTheory;
    bool showPanel;

    float schwarzschildMass = 1.0f;
    float kerrMass = 1.0f;
    float kerrSpin = 0.0f;
    float wormholeThroatRadius = 1.0f;
};

class SpacetimeFlightController {
public:
    SpacetimeFlightController();
    ~SpacetimeFlightController() = default;

    void update(double deltaTime);
    void setAcceleration(const std::array<double, 3>& accel) { acceleration = accel; }
    void setRotation(const std::array<double, 3>& rot) { rotation = rot; }

    double getCurvatureValue() const { return curvatureValue; }

private:
    std::array<double, 3> position = {0.0, 0.0, 0.0};
    std::array<double, 3> velocity = {0.0, 0.0, 0.0};
    std::array<double, 3> acceleration = {0.0, 0.0, 0.0};
    std::array<double, 3> rotation = {0.0, 0.0, 0.0};
    double curvatureValue = 0.0;
};

class FlightTelemetryPanel {
public:
    FlightTelemetryPanel();
    ~FlightTelemetryPanel() = default;

    void render(ImVec2 availSize);
    void setFlightController(SpacetimeFlightController& controller) { flightController = &controller; }

private:
    SpacetimeFlightController* flightController = nullptr;
};

class MinimapPanel {
public:
    MinimapPanel();
    ~MinimapPanel() = default;

    void render(ImVec2 availSize);
    void setFlightController(SpacetimeFlightController& controller) { flightController = &controller; }

private:
    SpacetimeFlightController* flightController = nullptr;
    int viewPlane = 0;
};

class ToolPalette {
public:
    ToolPalette();
    ~ToolPalette() = default;

    void render(ImVec2 availSize);

    std::function<void()> onAddSolarSystem;
    std::function<void()> onAddBlackHole;
    std::function<void()> onRunGRTest;
    std::function<void()> onResetView;
    std::function<void()> onToggleWireframe;

private:
    int activeTool = 0;
};

class FloatingPanels {
public:
    FloatingPanels();
    ~FloatingPanels() = default;

    void render();
    void addAnomaly(const AnomalyAlert& alert);
    void clearAlerts();

    bool showCurvatureOverlay = true;
    bool showGeodesicTracer = true;
    bool showAnomalyAlerts = true;

private:
    std::vector<AnomalyAlert> alerts;
};

class UI4D_ImGui {
public:
    UI4D_ImGui();
    ~UI4D_ImGui();

    void render();
    void renderPanels();
    void processInput();

    UI4DConfig& getConfig() { return config; }

    void setShow4DView(bool show) { show4DView = show; }
    void setShowSliceViews(bool show) { showSliceViews = show; }
    void setShowObjectBrowser(bool show) { showObjectBrowser = show; }
    void setShowPropertyEditor(bool show) { showPropertyEditor = show; }
    void setShowTimeline(bool show) { showTimeline = show; }
    void setShowTheorySelector(bool show) { showTheorySelector = show; }

private:
    UI4DConfig config;

    unsigned int viewTexture = 0;
    unsigned int sliceTexture = 0;

    bool show4DView = true;
    bool showSliceViews = true;
    bool showObjectBrowser = true;
    bool showPropertyEditor = true;
    bool showTimeline = true;
    bool showTheorySelector = true;
    bool showPlanckMicroscope = false;
    bool showDiscovery = true;
    bool showMinimap = true;

    TimelineBar timelineBar;
    ObjectBrowser objectBrowser;
    PropertyEditor propertyEditor;
    ContextMenu contextMenu;
    TheorySelectorPanel theorySelectorPanel;
    SpacetimeFlightController flightController;
    FlightTelemetryPanel flightTelemetryPanel;
    MinimapPanel minimapPanel;
    ToolPalette toolPalette;
    FloatingPanels floatingPanels;

    double simulationTime = 0.0;

    std::array<double, 3> cameraPosition = {0.0, 0.0, 0.0};
    std::array<double, 3> cameraTarget = {0.0, 0.0, -1.0};
    float cameraYaw = 0.0f;
    float cameraPitch = 0.0f;

    ImVec2 lastMousePos = {0, 0};
    bool mouseDragging = false;

    void render4DView(ImVec2 availSize);
    void renderSliceViews(ImVec2 availSize);
    void renderPlanckMicroscope(ImVec2 availSize);
    void renderMenuBar();
    void renderToolbar();
};

} // namespace ui_imgui
} // namespace quantumverse