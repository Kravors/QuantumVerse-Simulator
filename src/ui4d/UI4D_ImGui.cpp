#include "UI4D_ImGui.h"
#include <cmath>
#include <cstdio>

namespace quantumverse {
namespace ui_imgui {

TimelineBar::TimelineBar() {}

void TimelineBar::render(ImVec2 availSize) {
    ImGui::Begin("Timeline");
    
    if (ImGui::Button(isPlaying_ ? "||" : ">")) {
        isPlaying_ = !isPlaying_;
        if (onPlayPause) onPlayPause();
    }
    ImGui::SameLine();
    
    if (ImGui::Button("<<")) {
        currentTime = minTime_;
        if (onTimeScrub) onTimeScrub(currentTime);
    }
    ImGui::SameLine();
    
    if (ImGui::Button(">>")) {
        currentTime = maxTime_;
        if (onTimeScrub) onTimeScrub(currentTime);
    }
    ImGui::SameLine();
    
    ImGui::SliderFloat("Speed", &playSpeed, 0.1f, 5.0f);
    if (onSpeedChange) onSpeedChange(playSpeed);

    ImGui::SameLine();
    ImGui::PushItemWidth(availSize.x - 200);

    if (ImGui::SliderScalar("Time", ImGuiDataType_Double, &currentTime, &minTime_, &maxTime_)) {
        currentTime = std::max(minTime_, std::min(currentTime, maxTime_));
        if (onTimeScrub) onTimeScrub(currentTime);
    }
    
    ImGui::End();
}

void TimelineBar::setTime(double time) { currentTime = time; }
double TimelineBar::getTime() const { return currentTime; }
void TimelineBar::setTimeRange(double minTime, double maxTime) { minTime_ = minTime; maxTime_ = maxTime; }
void TimelineBar::setPlaying(bool playing) { isPlaying_ = playing; }
void TimelineBar::setSpeed(float speed) { playSpeed = speed; }

ObjectBrowser::ObjectBrowser() {
    objects = {
        {"sun", "Sun", "Star"},
        {"mercury", "Mercury", "Planet"},
        {"venus", "Venus", "Planet"},
        {"earth", "Earth", "Planet"},
        {"mars", "Mars", "Planet"},
        {"jupiter", "Jupiter", "Planet"},
        {"blackhole", "Black Hole", "Singularity"}
    };
}

void ObjectBrowser::render(ImVec2 availSize) {
    ImGui::Begin("Object Browser");
    
    ImGui::InputTextWithHint("##filter", "Filter...", filterText, sizeof(filterText));
    
    for (auto& obj : objects) {
        if (filterText[0] && strstr(obj.name.c_str(), filterText) == nullptr) {
            continue;
        }
        
        bool nodeOpen = ImGui::TreeNode(obj.name.c_str());
        
        if (ImGui::IsItemClicked()) {
            selectedObjectId = obj.id;
            if (onFlyToBody) onFlyToBody(obj.id);
        }
        
        if (ImGui::Button("Fly to")) {
            selectedObjectId = obj.id;
            if (onFlyToBody) onFlyToBody(obj.id);
        }
        
        if (nodeOpen) {
            ImGui::TreePop();
        }
    }
    
    ImGui::End();
}

void ObjectBrowser::refresh() {}
void ObjectBrowser::selectObject(const std::string& id) { selectedObjectId = id; }
void ObjectBrowser::clearSelection() { selectedObjectId.clear(); }

PropertyEditor::PropertyEditor() {}

void PropertyEditor::render(ImVec2 availSize) {
    ImGui::Begin("Property Editor");
    
    if (!objectId.empty()) {
        ImGui::Text("Editing: %s", objectName.c_str());
        ImGui::Separator();
        
        ImGui::InputDouble("Mass (kg)", &mass);
        ImGui::InputDouble("Radius (m)", &radius);
        
        ImGui::Text("Position:");
        ImGui::InputDouble("X", &position[0]);
        ImGui::InputDouble("Y", &position[1]);
        ImGui::InputDouble("Z", &position[2]);
        
        if (ImGui::Button("Apply")) {
            if (onApply) onApply(objectId, mass, radius, position);
        }
    } else {
        ImGui::Text("Select an object to edit");
    }
    
    ImGui::End();
}

void PropertyEditor::setObject(const std::string& id, const std::string& name, double m, double r,
                               const std::array<double, 3>& pos) {
    objectId = id;
    objectName = name;
    mass = m;
    radius = r;
    position = pos;
}

ContextMenu::ContextMenu() {}

void ContextMenu::openAt(const ImVec2& position, const std::string& objName) {
    menuPosition = position;
    objectName = objName;
    isMenuOpen = true;
}

void ContextMenu::render() {
    if (!isMenuOpen) return;
    
    ImGui::OpenPopup("ContextMenuPopup");
    isMenuOpen = false;
    
    if (ImGui::BeginPopup("ContextMenuPopup")) {
        ImGui::Text("Object: %s", objectName.c_str());
        ImGui::Separator();
        
        if (ImGui::MenuItem("Drop Test Particle")) {
            if (onDropTestParticle) onDropTestParticle();
        }
        if (ImGui::MenuItem("Measure Precession")) {
            if (onMeasurePrecession) onMeasurePrecession();
        }
        if (ImGui::MenuItem("Compute Redshift")) {
            if (onComputeRedshift) onComputeRedshift();
        }
        
        ImGui::EndPopup();
    }
}

TheorySelectorPanel::TheorySelectorPanel() : currentTheory(TheoryType::SCHWARZSCHILD), showPanel(true) {}

void TheorySelectorPanel::render(ImVec2 availSize) {
    ImGui::Begin("Theory Selector");
    
    const char* theories[] = {"Schwarzschild", "Kerr", "Wormhole", "Newtonian", "CDT", "Spin Foam", "GFT", "Causal Set"};
    int current = static_cast<int>(currentTheory);
    
    if (ImGui::Combo("Theory", &current, theories, IM_ARRAYSIZE(theories))) {
        currentTheory = static_cast<TheoryType>(current);
        if (onTheoryChange) onTheoryChange(currentTheory);
    }
    
    switch (currentTheory) {
        case TheoryType::SCHWARZSCHILD:
            ImGui::DragFloat("Mass", &schwarzschildMass, 0.1f, 0.1f, 10.0f);
            break;
        case TheoryType::KERR:
            ImGui::DragFloat("Mass", &kerrMass, 0.1f, 0.1f, 10.0f);
            ImGui::DragFloat("Spin", &kerrSpin, 0.01f, 0.0f, 1.0f);
            break;
        case TheoryType::WORMHOLE:
            ImGui::DragFloat("Throat Radius", &wormholeThroatRadius, 0.1f, 0.1f, 10.0f);
            break;
        default:
            break;
    }
    
    ImGui::End();
}

void TheorySelectorPanel::setCurrentTheory(TheoryType theory) { currentTheory = theory; }

SpacetimeFlightController::SpacetimeFlightController() {}

void SpacetimeFlightController::update(double deltaTime) {
    for (int i = 0; i < 3; i++) {
        velocity[i] += acceleration[i] * deltaTime;
        position[i] += velocity[i] * deltaTime;
    }
    
    for (int i = 0; i < 3; i++) {
        if (std::abs(velocity[i]) > 0.1) velocity[i] *= 0.95;
    }
    
    double r = std::sqrt(position[0]*position[0] + position[1]*position[1] + position[2]*position[2]);
    curvatureValue = 1.0 / (r * r + 0.1);
}

FlightTelemetryPanel::FlightTelemetryPanel() {}

void FlightTelemetryPanel::render(ImVec2 availSize) {
    ImGui::Begin("Flight Telemetry");
    
    if (flightController) {
        ImGui::Text("Curvature: %.2e", flightController->getCurvatureValue());
    }
    
    ImGui::End();
}

MinimapPanel::MinimapPanel() {}

void MinimapPanel::render(ImVec2 availSize) {
    ImGui::Begin("Minimap");
    
    const char* planes[] = {"XY", "XZ", "YZ"};
    ImGui::Combo("Plane", &viewPlane, planes, IM_ARRAYSIZE(planes));
    
    ImVec2 canvasSize = ImVec2(150, 150);
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(20, 20, 30, 255));
    
    if (flightController) {
        ImVec2 center = ImVec2(canvasPos.x + canvasSize.x/2, canvasPos.y + canvasSize.y/2);
        draw_list->AddCircleFilled(center, 5.0f, IM_COL32(255, 100, 100, 255));
    }
    
    ImGui::Dummy(canvasSize);
    
    ImGui::End();
}

ToolPalette::ToolPalette() {}

void ToolPalette::render(ImVec2 availSize) {
    ImGui::Begin("Tools");
    
    if (ImGui::Button("Add Solar System")) {
        if (onAddSolarSystem) onAddSolarSystem();
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Black Hole")) {
        if (onAddBlackHole) onAddBlackHole();
    }
    ImGui::SameLine();
    
    if (ImGui::Button("Run GR Test")) {
        if (onRunGRTest) onRunGRTest();
    }
    ImGui::SameLine();
    
    if (ImGui::Button("Reset View")) {
        if (onResetView) onResetView();
    }
    if (ImGui::Button("Wireframe")) {
        if (onToggleWireframe) onToggleWireframe();
    }
    
    ImGui::End();
}

FloatingPanels::FloatingPanels() {}

void FloatingPanels::render() {
    if (showCurvatureOverlay) {
        ImGui::Begin("Curvature Overlay", &showCurvatureOverlay);
        ImGui::ProgressBar(0.5f, ImVec2(-1, 0), "Curvature");
        ImGui::End();
    }
    
    if (showGeodesicTracer) {
        ImGui::Begin("Geodesic Tracer", &showGeodesicTracer);
        ImGui::Text("Geodesics: 0");
        ImGui::End();
    }
    
    if (showAnomalyAlerts && !alerts.empty()) {
        ImGui::Begin("Anomaly Alerts", &showAnomalyAlerts);
        for (const auto& alert : alerts) {
            ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "ALERT: %s (%.2f)", alert.message.c_str(), alert.severity);
        }
        if (ImGui::Button("Clear")) clearAlerts();
        ImGui::End();
    }
}

void FloatingPanels::addAnomaly(const AnomalyAlert& alert) { alerts.push_back(alert); }
void FloatingPanels::clearAlerts() { alerts.clear(); }

UI4D_ImGui::UI4D_ImGui() {}

UI4D_ImGui::~UI4D_ImGui() {}

void UI4D_ImGui::render() {
    ImGui::DockSpaceOverViewport();
    
    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x < 1.0f) avail.x = 640.0f;
    if (avail.y < 1.0f) avail.y = 480.0f;
    
    if (show4DView) render4DView(avail);
    if (showSliceViews) renderSliceViews(avail);
    if (showPlanckMicroscope) renderPlanckMicroscope(avail);
    if (showTimeline) timelineBar.render(avail);
    if (showObjectBrowser) objectBrowser.render(avail);
    if (showPropertyEditor) propertyEditor.render(avail);
    if (showTheorySelector) theorySelectorPanel.render(avail);
    if (showMinimap) minimapPanel.render(avail);
    if (showDiscovery) floatingPanels.render();
}

void UI4D_ImGui::renderPanels() {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x < 0) avail.x = 0;
    if (avail.y < 0) avail.y = 0;
    render4DView(avail);
    renderSliceViews(avail);
    renderPlanckMicroscope(avail);
    timelineBar.render(avail);
    objectBrowser.render(avail);
    propertyEditor.render(avail);
    theorySelectorPanel.render(avail);
    minimapPanel.render(avail);
    floatingPanels.render();
}

void UI4D_ImGui::processInput() {
    ImGuiIO& io = ImGui::GetIO();
    
    if (io.WantCaptureMouse) return;
    
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    bool mouseDown = ImGui::IsMouseDown(0);
    
    if (mouseDown && !mouseDragging) {
        mouseDragging = true;
        lastMousePos = mousePos;
    } else if (!mouseDown) {
        mouseDragging = false;
    }
    
    if (mouseDragging) {
        ImVec2 delta = ImVec2(mousePos.x - lastMousePos.x, mousePos.y - lastMousePos.y);
        cameraYaw += delta.x * 0.01f;
        cameraPitch += delta.y * 0.01f;
        lastMousePos = mousePos;
    }
}

void UI4D_ImGui::render4DView(ImVec2 availSize) {
    ImGui::Begin("4D View");
    
    ImGui::Text("4D Spacetime Viewport");
    ImGui::Text("Position: (%.2f, %.2f, %.2f)", cameraPosition[0], cameraPosition[1], cameraPosition[2]);
    
    ImVec2 contentSize = ImVec2(
        (availSize.x > 0 && availSize.x < 10000) ? availSize.x * 0.8f : 400,
        (availSize.y > 0 && availSize.y < 10000) ? availSize.y * 0.8f : 300
    );
    ImGui::Dummy(contentSize);
    
    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0)) {
        ImVec2 pos = ImGui::GetIO().MousePos;
        contextMenu.openAt(pos, "4D Space");
    }
    
    contextMenu.render();
    
    ImGui::End();
}

void UI4D_ImGui::renderSliceViews(ImVec2 availSize) {
    ImGui::Begin("Slice Views");
    
    ImVec2 smallSize(
        (availSize.x > 0 && availSize.x < 10000) ? availSize.x * 0.48f : 200,
        (availSize.y > 0 && availSize.y < 10000) ? availSize.y * 0.48f : 200
    );
    
    ImGui::Dummy(smallSize);
    ImGui::SameLine();
    ImGui::Dummy(smallSize);
    ImGui::Dummy(smallSize);
    ImGui::SameLine();
    ImGui::Dummy(smallSize);
    
    ImGui::End();
}

void UI4D_ImGui::renderPlanckMicroscope(ImVec2 availSize) {
    ImGui::Begin("Planck Microscope");
    
    static float logScale = 0.0f;
    if (ImGui::SliderFloat("Scale (log10 m)", &logScale, -35.0f, 0.0f)) {
        // Scale changed
    }
    
    const char* lodLabels[] = {"Quantum Foam", "Planck Length", "Particle Scale", "Atomic", "Human", "Planetary", "Stellar"};
    static int lod = 0;
    ImGui::Combo("LOD", &lod, lodLabels, IM_ARRAYSIZE(lodLabels));
    
    ImGui::End();
}

void UI4D_ImGui::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New")) {}
            if (ImGui::MenuItem("Open")) {}
            if (ImGui::MenuItem("Save")) {}
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("4D View", nullptr, &show4DView);
            ImGui::MenuItem("Slice Views", nullptr, &showSliceViews);
            ImGui::MenuItem("Object Browser", nullptr, &showObjectBrowser);
            ImGui::MenuItem("Property Editor", nullptr, &showPropertyEditor);
            ImGui::MenuItem("Timeline", nullptr, &showTimeline);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Tools")) {
            ImGui::MenuItem("Discovery Scan");
            ImGui::MenuItem("Anomaly Detection");
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}

void UI4D_ImGui::renderToolbar() {
    ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);
    
    toolPalette.render(ImGui::GetContentRegionAvail());
    
    ImGui::End();
}

} // namespace ui_imgui
} // namespace quantumverse