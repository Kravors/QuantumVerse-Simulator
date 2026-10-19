/**
 * @file main.qml
 * @brief Solar System Scope-inspired QML UI for QuantumVerse 4D Simulator
 *
 * Layout follows the Solar System Scope paradigm:
 * - Central 3D/4D OpenGL viewport
 * - Left sidebar with object browser
 * - Bottom timeline with playback controls
 * - Top toolbar with quick actions
 * - Right info panels (collapsible)
 */

import QtQuick
import QtQuick.Window
import QtQuick.Controls.Basic 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts
import QuantumVerse 1.0

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 1280
    height: 720
    minimumWidth: 1200
    minimumHeight: 700
    title: "QuantumVerse Simulator - 4D Spacetime Explorer"
    color: "#0a0a1a"

    property bool noViewport: Qt.application.arguments.indexOf("--noviewport") !== -1 ||
                              Qt.application.arguments.indexOf("--software-rendering") !== -1
    property Item viewportItem: noViewport ? null : viewportLoader.item
    property bool toolbarOverflow: toolBar.rootLayout.implicitWidth > mainToolbar.width

    onToolbarOverflowChanged: {
        if (viewportItem) viewportItem.setToolbarOverflow(toolbarOverflow)
    }

    Connections {
        target: camera4DAdapter
        onDistanceChanged: {
            if (viewportItem && Math.abs(viewportItem.cameraDistance - camera4DAdapter.distance) > 0.01) {
                viewportItem.cameraDistance = camera4DAdapter.distance
            }
            if (camController && Math.abs(camController.distance - camera4DAdapter.distance) > 0.01) {
                camController.distance = camera4DAdapter.distance
            }
        }
    }

    Component.onCompleted: {
        console.log("sceneGraphModel count:", sceneGraphModel ? sceneGraphModel.count : "model not available")
        console.log("discoveryInstruments count:", discoveryPanelManager ? discoveryPanelManager.instrumentCount : "not available")
        console.log("noViewport=" + noViewport + " args=" + Qt.application.arguments)
        // Runtime: camController defaults to d=500 (C++ constructor). Override at
        // startup so the status bar and orbit navigation use a usable distance;
        // the 10-unit solar system would otherwise be an invisible speck.
        //
        // GATE: skip when the camera came from --cam-* CLI flags (cliCameraSet).
        // Without this gate, the defaults below overwrite the CLI values a few
        // frames after startup and the flags appear to work then silently
        // revert. main_qml.cpp sets this context property (and the adapter
        // values) BEFORE engine.load().
        if (!cliCameraSet) {
            camController.setDistance(150.0)
            camController.setAzimuth(0.7854)  // 45 degrees
            camController.setElevation(0.5236)  // 30 degrees
            // Sync viewportItem.cameraDistance with camera4DAdapter if available
            if (viewportItem && camera4DAdapter) {
                viewportItem.cameraDistance = camera4DAdapter.distance
                camera4DAdapter.setDistance(150.0)
                camera4DAdapter.setAzimuth(0.7854)
                camera4DAdapter.setElevation(0.5236)
            }
        }
        // Force off debug overlays
        if (viewportItem) {
            viewportItem.showGeodesics = false
            viewportItem.showLightCones = false
            viewportItem.showQuantumGeometry = false
        }
    }

    // Early placeholders to avoid ReferenceError before first use
    Item {
        id: aboutDialog
        function open() { console.log("About dialog unavailable: QtQuick.Dialogs 1.3 missing in Qt 6.5.3") }
    }

    // Toast notification for live alerts
    ToastNotification {
        id: toastNotification
    }

    // Educational tour overlay: shown whenever tourController is Running or Paused
    EducationalTourOverlay {
        id: tourOverlay
    }

    Connections {
        target: discoveryPanelManager
        function onLiveAlertProcessed(findingId) {
            var list = discoveryPanelManager.findingsList || []
            var last = list.length > 0 ? list[list.length - 1] : null
            toastNotification.alertType = last ? (last.severity || "INFO") : "INFO"
            toastNotification.origin = last ? (last.instrumentName || findingId) : findingId
        }
    }

    // Global state
    property real simulationTime: 0.0
    property bool simulationPaused: false
    property real timeScale: 1.0

    // Parameter sweep state
    property real sweepProgress: 0.0
    property bool sweepRunning: false

    // Font
    font.family: "Segoe UI"
    font.pixelSize: 13

    // Object browser computed display properties
    property string selectedBodyMass: "—"
    property string selectedBodyPosition: "—"
    property string selectedBodyVelocity: "—"

    function formatMass(kg) {
        if (kg === undefined || kg === null || kg <= 0 || !isFinite(kg)) return "N/A";
        var absKg = Math.abs(kg);
        if (absKg >= 1e30) return (kg / 1e30).toFixed(3) + " × 10³⁰ kg"
        if (absKg >= 1e27) return (kg / 1e27).toFixed(3) + " × 10²⁷ kg"
        if (absKg >= 1e24) return (kg / 1e24).toFixed(3) + " × 10²⁴ kg"
        if (absKg >= 1e21) return (kg / 1e21).toFixed(3) + " × 10²¹ kg"
        if (absKg >= 1e18) return (kg / 1e18).toFixed(3) + " × 10¹⁸ kg"
        if (absKg >= 1e15) return (kg / 1e15).toFixed(3) + " × 10¹⁵ kg"
        if (absKg >= 1e12) return (kg / 1e12).toFixed(3) + " × 10¹² kg"
        if (absKg >= 1e9) return (kg / 1e9).toFixed(3) + " × 10⁹ kg"
        if (absKg >= 1e6) return (kg / 1e6).toFixed(3) + " × 10⁶ kg"
        if (absKg >= 1e3) return (kg / 1e3).toFixed(3) + " × 10³ kg"
        return kg.toExponential(3) + " kg"
    }

    // =========================================================================
    // MENU BAR
    // =========================================================================
    menuBar: MenuBar {
        Menu {
            title: "&File"
            // Commented out for Qt 6.5.3: QtQuick.Dialogs 1.3 module not available
            // MenuItem { text: "&Open Spacetime Configuration..."; onTriggered: fileDialog.open() }
            // MenuItem { text: "&Save Snapshot..."; onTriggered: saveDialog.open() }
            MenuItem {
                text: "&Reload Config"
                onTriggered: {
                    if (viewportItem && typeof viewportItem.reloadConfig === "function") {
                        viewportItem.reloadConfig()
                        console.log("Config reloaded")
                    } else {
                        console.log("Reload Config: viewport not available")
                    }
                }
                ToolTip.text: "Reload config/simulator.json at runtime"
            }
            MenuSeparator {}
            MenuItem { text: "E&xit"; onTriggered: Qt.quit() }
        }
        Menu {
            title: "&Simulation"
            MenuItem {
                text: simulationPaused ? "&Resume" : "Pa&use"
                onTriggered: simulationPaused = !simulationPaused
            }
            MenuItem { text: "&Step (0.1s)"; onTriggered: stepSimulation() }
            MenuItem { text: "&Reset"; onTriggered: resetSimulation() }
        }
        Menu {
            title: "&View"
            MenuItem {
                text: "Show &Grid"
                checkable: true; checked: true
                onToggled: {
                    console.log("Toggling showGrid to: " + checked)
                    if (viewportItem) viewportItem.showGrid = checked
                }
            }
            MenuItem {
                text: "Plane &Mode"
                checkable: true; checked: false
                onToggled: {
                    console.log("Toggling Plane Mode to: " + checked)
                    if (viewportItem) viewportItem.planeMode = checked
                }
            }
            MenuItem {
                text: "Procedural Textures"
                checkable: true; checked: true
                onToggled: {
                    if (viewportItem) viewportItem.textureSource = checked ? 0 : 1
                }
            }
            MenuItem {
                text: "Show &Geodesics"
                checkable: true; checked: true
                onToggled: {
                    if (viewportItem) viewportItem.showGeodesics = checked
                }
            }
            MenuItem {
                text: "Show &Quantum Geometry"
                checkable: true; checked: false
                onToggled: {
                    if (viewportItem) viewportItem.showQuantumGeometry = checked
                }
            }
            MenuSeparator {}
            MenuItem {
                text: "Brightness Boost"
                checkable: true; checked: true
                onToggled: {
                    if (viewportItem) {
                        viewportItem.exposure = checked ? 1.5 : 1.0
                        viewportItem.bloomThreshold = checked ? 0.3 : 1.0
                        viewportItem.bloomIntensity = checked ? 1.5 : 0.8
                    }
                }
            }
            MenuItem {
                text: "Gravitational Lensing"
                checkable: true; checked: false
                onToggled: {
                    if (viewportItem) viewportItem.lensingEnabled = checked
                }
            }
        }
        Menu {
            title: "&Help"
            MenuItem { text: "&About"; onTriggered: aboutDialog.open() }
        }
    }

    // =========================================================================
    // TOOLBAR (Top)
    // =========================================================================
    header: ToolBar {
        id: mainToolbar
        ToolBarRow {
            id: toolBar
            Layout.fillWidth: true
            Layout.preferredHeight: 56

            targetViewport: viewportItem
            targetCamController: camController
            simulationPaused: simulationPaused

            onResetViewClicked: if (viewportItem) viewportItem.resetView()
            onFocusSunClicked: camController.reset()
            onPauseToggled: simulationPaused = paused
            onPlaneAxisChanged: if (viewportItem && viewportItem.camera4DAdapter) viewportItem.camera4DAdapter.activePlane = axis
            onTimeScaleChanged: timeScale = Math.pow(10, value - 1)
            onCurvatureModeChanged: if (viewportItem) viewportItem.curvatureMode = mode
            onPlaneResolutionChanged: if (viewportItem) viewportItem.planeResolution = resolution
            onBlackHoleMassChanged: if (viewportItem) viewportItem.blackHoleMass = mass
            onZoomChanged: {
                if (viewportItem) {
                    viewportItem.cameraDistance = distance
                    if (camController) camController.distance = distance
                }
            }
            onBlackHoleTypeChanged: if (viewportItem) viewportItem.setBlackHoleType(type)
            onPhysicsGLogChanged: if (viewportItem) viewportItem.physicsGLog = logG
            onGravitationalWaveAudioToggled: if (viewportItem) viewportItem.audioEnabled = enabled
        }
    }

    // =========================================================================
    // MAIN CONTENT AREA
    // =========================================================================
    SplitView {
        id: mainContent
        anchors.fill: parent
        orientation: Qt.Horizontal

        ActionRail {
            id: actionRail
            SplitView.preferredWidth: 56
            SplitView.minimumWidth: 56
            SplitView.maximumWidth: 56

            targetViewport: viewportItem
            tourController: tourController
            tourManager: tourManager
            signalingClient: signalingClient
            discoveryPanel: discoveryPanel

            onInjectAsteroid: {
                if (viewportItem) {
                    var angle = Math.random() * 2 * Math.PI;
                    var dist = 1.5e11; // ~1 AU
                    var x = Math.cos(angle) * dist;
                    var z = Math.sin(angle) * dist;
                    var v = 30000; // 30 km/s
                    var vx = -Math.sin(angle) * v * 0.8;
                    var vz = Math.cos(angle) * v * 0.8;
                    viewportItem.injectAsteroid(x, 0, z, vx, 0, vz, 1e15);
                }
            }
            onClearAsteroids: if (viewportItem) viewportItem.clearAsteroids()
            onDiscoveryToggled: discoveryPanel.visible = visible
            onTourToggled: {
                if (!tourManager || !tourController) return
                if (tourController.state === TourController.Stopped) {
                    var ids = tourManager.listTourIds()
                    if (ids.length === 0) return
                    if (tourController.loadTourFromFile(
                            tourManager.tourFilePath(ids[0]))) {
                        tourController.start()
                    }
                } else {
                    tourController.stop()
                }
            }
            onVrToggled: {
                if (signalingClient != null && signalingClient.isConnected) {
                    if (viewportItem) viewportItem.toggleVR()
                } else {
                    console.warn("VR: signaling server not connected")
                }
            }
        }

        Component.onCompleted: {
            console.log("SplitView contentWidth:", contentWidth, "height:", height)
        }

        onWidthChanged: {
            console.log("SplitView width:", width, "contentWidth:", contentWidth)
        }

        // --- LEFT SIDEBAR: Object Browser ---
        Pane {
            id: leftSidebar
            SplitView.preferredWidth: 160
            SplitView.minimumWidth: 80
            SplitView.maximumWidth: 220
            z: 1
            enabled: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                Label {
                    text: "Object Browser"
                    font.bold: true
                    font.pixelSize: 15
                    Layout.fillWidth: true
                }

                TextField {
                    id: searchField
                    placeholderText: "Search objects..."
                    Layout.fillWidth: true
                }

                // Object list
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    ListView {
                        id: objectList
                        model: sceneGraphModel
                        delegate: ItemDelegate {
                            width: parent ? parent.width : 0
                            highlighted: model ? model.isSelected : false
                            onClicked: {
                                if (sceneGraphModel) {
                                    sceneGraphModel.selectObject(model.objectId)
                                }
                                // Focus the camera on the clicked object.
                                // Positions are raw meters; convert to viewport
                                // units with the same factor the renderer uses
                                // so the camera stays on-grid instead of flying
                                // out to ~1e11 (which blanked the viewport).
                                var pos = model && model.position ? model.position
                                    : (sceneGraphModel ? sceneGraphModel.selectedObjectPosition() : null)
                                var s = (typeof viewportScale !== "undefined") ? viewportScale : 1
                                if (pos && pos.x !== undefined &&
                                    viewportItem && viewportItem.camera4DAdapter) {
                                    viewportItem.camera4DAdapter.focusOn(
                                        Qt.vector3d(pos.x * s, pos.y * s, pos.z * s))
                                } else {
                                    console.warn("focusOn skipped: camera4DAdapter or position missing")
                                }
                            }

                            RowLayout {
                                anchors.fill: parent
                                spacing: 6

                                Rectangle {
                                    width: 12; height: 12; radius: 6
                                    color: model ? model.color : "#888"
                                    border.color: "#555"
                                    border.width: 1
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1

                                    Label {
                                        text: model ? model.name : "Unknown"
                                        font.bold: model ? model.isSelected : false
                                        color: "#eee"
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }

                                    Label {
                                        text: model ? model.type : ""
                                        font.pixelSize: 10
                                        color: "#888"
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }

                                Label {
                                    text: model && model.mass > 0 ? formatMass(model.mass) : ""
                                    font.pixelSize: 10
                                    color: "#aaa"
                                    horizontalAlignment: Text.AlignRight
                                }
                            }
                        }
                    }
                }

                // Selected object details
                Pane {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 160
                    visible: objectList.currentItem !== null

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 4

                        Label {
                            text: "Properties"
                            font.bold: true
                            font.pixelSize: 13
                        }

                        GridLayout {
                            columns: 2
                            Layout.fillWidth: true

                            Label { text: "Mass:"; color: "#888" }
                            Label { text: selectedBodyMass; color: "#fff" }

                            Label { text: "Position:"; color: "#888" }
                            Label { text: selectedBodyPosition; color: "#fff" }

                            Label { text: "Velocity:"; color: "#888" }
                            Label { text: selectedBodyVelocity; color: "#fff" }
                        }
                    }
                }
            }

            // Computed display properties
            property string selectedBodyMass: "—"
            property string selectedBodyPosition: "—"
            property string selectedBodyVelocity: "—"

            function formatMass(kg) {
                if (kg === undefined || kg === null || kg <= 0 || !isFinite(kg)) return "N/A";
                var absKg = Math.abs(kg);
                if (absKg >= 1e30) return (kg / 1e30).toFixed(3) + " × 10³⁰ kg"
                if (absKg >= 1e27) return (kg / 1e27).toFixed(3) + " × 10²⁷ kg"
                if (absKg >= 1e24) return (kg / 1e24).toFixed(3) + " × 10²⁴ kg"
                if (absKg >= 1e21) return (kg / 1e21).toFixed(3) + " × 10²¹ kg"
                if (absKg >= 1e18) return (kg / 1e18).toFixed(3) + " × 10¹⁸ kg"
                if (absKg >= 1e15) return (kg / 1e15).toFixed(3) + " × 10¹⁵ kg"
                if (absKg >= 1e12) return (kg / 1e12).toFixed(3) + " × 10¹² kg"
                if (absKg >= 1e9) return (kg / 1e9).toFixed(3) + " × 10⁹ kg"
                if (absKg >= 1e6) return (kg / 1e6).toFixed(3) + " × 10⁶ kg"
                if (absKg >= 1e3) return (kg / 1e3).toFixed(3) + " × 10³ kg"
                return kg.toExponential(3) + " kg"
            }
        }

        // --- CENTRAL VIEWPORT ---
        Pane {
            id: viewportContainer
            SplitView.fillWidth: true
            SplitView.minimumWidth: 200
            // The GL scene is composited onto the window back buffer in
            // QmlGlViewport::renderGL() (beforeRendering); this Pane must not
            // paint an opaque background over it, or the viewport stays black.
            background: null

            Loader {
                id: viewportLoader
                anchors.fill: parent
                sourceComponent: noViewport ? placeholderComponent : viewportComponent
            }

             Component {
                 id: viewportComponent
                QmlGlViewport {
                    id: viewport
                    anchors.fill: parent
                    showGrid: true
                    showGeodesics: false
                    showQuantumGeometry: false
                    bloomEnabled: true
                    bloomIntensity: 1.5
                    bloomThreshold: 0.3
                    exposure: 1.5
                    Component.onCompleted: console.log("Viewport created, size:", width, "x", height)

                        Timer {
                            // GATE: one-shot startup overwrite of the camera defaults
                            // (distance 150, azimuth 45, elevation 30) at t+100ms.
                            // Fires AFTER the first frames have rendered, so an
                            // ungated --cam-* CLI camera works briefly and then
                            // silently reverts. Skipped entirely when the camera
                            // came from the CLI (cliCameraSet).
                            running: !cliCameraSet
                            interval: 100
                            repeat: false
                            onTriggered: {
                                if (!cliCameraSet && viewport.camera4DAdapter) {
                                    viewport.camera4DAdapter.setDistance(150.0)
                                    viewport.camera4DAdapter.setAzimuth(0.7854)  // 45 degrees
                                    viewport.camera4DAdapter.setElevation(0.5236)  // 30 degrees
                                    console.log("Runtime: set camera4DAdapter distance to 150.0, azimuth=45, elevation=30")
                                }
                            }
                        }

                       MouseArea {
                         id: viewportMouse
                         anchors.fill: parent
                         acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                         property point lastPos
                         property bool isPanning: false
                         property int dragButton: 0
                         property bool mousePressed: false

                        onPressed: function(mouse) {
                            lastPos = Qt.point(mouse.x, mouse.y)
                            dragButton = mouse.button
                            isPanning = (mouse.button === Qt.RightButton)
                            mousePressed = true
                            if (viewport && viewport.camera4DAdapter) {
                                viewport.camera4DAdapter.onMousePressed(
                                    mouse.x, mouse.y, mouse.button,
                                    mouse.modifiers !== undefined ? mouse.modifiers : 0)
                            }
                            viewport.handleMousePress(mouse.x, mouse.y, mouse.button)
                        }

                        onPositionChanged: function(mouse) {
                            if (!mousePressed) return
                            var dx = mouse.x - lastPos.x
                            var dy = mouse.y - lastPos.y
                            lastPos = Qt.point(mouse.x, mouse.y)

                            if (viewport && viewport.camera4DAdapter) {
                                viewport.camera4DAdapter.onMouseMoved(
                                    mouse.x, mouse.y,
                                    mouse.buttons !== undefined ? mouse.buttons : dragButton,
                                    mouse.modifiers !== undefined ? mouse.modifiers : 0)
                            }

                            if (dragButton === Qt.LeftButton && !isPanning) {
                                camController.azimuth += dx * 0.005
                                camController.elevation = Math.max(-1.4, Math.min(1.4,
                                    camController.elevation + dy * 0.005))
                            } else if (isPanning) {
                                camController.onMouseMoved(mouse.x, mouse.y, dragButton)
                            }
                        }

                        onReleased: function(mouse) {
                            mousePressed = false
                            isPanning = false
                            if (viewport && viewport.camera4DAdapter) {
                                viewport.camera4DAdapter.onMouseReleased(
                                    mouse.x, mouse.y, mouse.button)
                            }
                            viewport.handleMouseReleased(mouse.x, mouse.y, mouse.button)
                        }

                        onWheel: function(wheel) {
                            if (viewport && viewport.camera4DAdapter) {
                                viewport.camera4DAdapter.onWheelDelta(
                                    wheel.angleDelta.y,
                                    wheel.modifiers !== undefined ? wheel.modifiers : 0)
                            }
                        }

                        onDoubleClicked: function(mouse) {
                            viewport.zoomIn()
                        }
                    }

                    focus: true
                    Keys.onPressed: function(event) {
                        switch (event.key) {
                        case Qt.Key_W: camController.dolly(-1.0); break
                        case Qt.Key_S: camController.dolly(1.0); break
                        case Qt.Key_R: camController.reset(); break
                        case Qt.Key_Space: simulationPaused = !simulationPaused; break
                        case Qt.Key_F3: if (viewport) viewport.showHUD = !viewport.showHUD; break
                        case Qt.Key_1: if (viewport && viewport.camera4DAdapter) viewport.camera4DAdapter.activePlane = 0; break
                        case Qt.Key_2: if (viewport && viewport.camera4DAdapter) viewport.camera4DAdapter.activePlane = 1; break
                        case Qt.Key_3: if (viewport && viewport.camera4DAdapter) viewport.camera4DAdapter.activePlane = 2; break
                        case Qt.Key_4: if (viewport && viewport.camera4DAdapter) viewport.camera4DAdapter.activePlane = 3; break
                        case Qt.Key_5: if (viewport && viewport.camera4DAdapter) viewport.camera4DAdapter.activePlane = 4; break
                        case Qt.Key_6: if (viewport && viewport.camera4DAdapter) viewport.camera4DAdapter.activePlane = 5; break
                        case Qt.Key_Q: if (viewport && viewport.camera4DAdapter) viewport.camera4DAdapter.cyclePlane(); break
                        }
                    }
                }
            }

            Component {
                id: placeholderComponent
                Rectangle {
                    color: "#2b2b2b"
                    anchors.fill: parent
                    Text {
                        anchors.centerIn: parent
                        text: "3D Viewport disabled\n(--noviewport)"
                        color: "white"
                        font.pixelSize: 24
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: console.log("Placeholder clicked")
                    }
                }
            }

            // Overlay: coordinate info
            Label {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.margins: 8
                text: "Time: " + simulationTime.toFixed(2) + "s  |  Scale: " + timeScale.toFixed(1) + "x  |  " +
                      "Camera: az=" + (camController.azimuth * 180 / Math.PI).toFixed(1) + " el=" + (camController.elevation * 180 / Math.PI).toFixed(1) + " d=" + (viewportItem ? viewportItem.cameraDistance.toFixed(1) : camController.distance.toFixed(1)) +
                      " | Zoom: " + (viewportItem ? (500.0 / viewportItem.cameraDistance * 100).toFixed(0) : "100") + "%" +
                      (viewportItem && viewportItem.camera4DAdapter ? "  |  Plane: " + ["XY","XZ","YZ","TX","TY","TZ"][viewportItem.camera4DAdapter.activePlane] : "")
                color: "#888"
                font.pixelSize: 11

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -4
                    color: "#000000aa"
                    radius: 4
                    z: -1
                }

                padding: 4
            }
        }

        // --- RIGHT PANEL: Discovery & Info ---
        Pane {
            id: discoveryPanel
            SplitView.preferredWidth: 360
            SplitView.minimumWidth: 340
            SplitView.maximumWidth: 450
            visible: true
            z: 1
            enabled: true

            Component.onCompleted: {
                console.log("Right panel width:", width, "height:", height)
            }

            onWidthChanged: {
                console.log("Right panel width changed:", width)
            }

            ScrollView {
                anchors.fill: parent
                clip: true

                ColumnLayout {
                    width: discoveryPanel.width - 16
                    spacing: 6

                    Label {
                        text: "🔬 Discovery Console"
                        font.bold: true
                        font.pixelSize: 15
                        Layout.fillWidth: true
                    }

                // Instrument selector
                Pane {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 4

                        Label { text: "Active Instrument"; font.pixelSize: 10; color: "#888" }

                        ComboBox {
                            id: instrumentCombo
                            model: discoveryPanelManager ? discoveryPanelManager.instrumentNames : []
                            currentIndex: discoveryPanelManager ? discoveryPanelManager.activeInstrumentIndex : 0
                            onCurrentIndexChanged: if (discoveryPanelManager) discoveryPanelManager.activeInstrumentIndex = currentIndex
                            Layout.fillWidth: true
                            font.pixelSize: 11

                            delegate: ItemDelegate {
                                width: parent ? parent.width : 0
                                Text {
                                    text: modelData
                                    font.pixelSize: 11
                                    color: "#eee"
                                    elide: Text.ElideRight
                                }
                                highlighted: index === instrumentCombo.currentIndex
                            }
                        }
                    }
                }

                // Scan controls
                Pane {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 6

                        Button {
                            text: discoveryPanelManager && discoveryPanelManager.scanRunning ? "⏹ Stop" : "▶ Scan"
                            Layout.fillWidth: true
                            onClicked: {
                                console.log("Scan button clicked!")
                                if (discoveryPanelManager) {
                                    if (discoveryPanelManager.scanRunning)
                                        discoveryPanelManager.stopScan()
                                    else
                                        discoveryPanelManager.startScan()
                                }
                            }
                        }

                        Button {
                            text: "Export"
                            Layout.preferredWidth: 70
                            onClicked: {
                                console.log("Export button clicked!")
                                if (discoveryPanelManager) discoveryPanelManager.exportCurrentFindings()
                            }
                        }
                    }
                }

                // Progress bar
                ProgressBar {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 8
                    visible: discoveryPanelManager && discoveryPanelManager.scanRunning
                    value: discoveryPanelManager ? discoveryPanelManager.scanProgress : 0
                }

                // ML Anomaly Detection controls
                Pane {
                    Layout.fillWidth: true
                    visible: anomalyMonitor !== undefined && anomalyMonitor !== null
                    background: Rectangle {
                        color: "#1a1a2e"
                        radius: 6
                        border.color: anomalyMonitor && anomalyMonitor.lastScore > (anomalyMonitor.thresholdMultiplier * 0.05) ? "#f44" : "#2a2a4a"
                        border.width: anomalyMonitor && anomalyMonitor.lastScore > (anomalyMonitor.thresholdMultiplier * 0.05) ? 2 : 1
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 4

                        Label { text: "🤖 ML Anomaly Detection"; font.bold: true; font.pixelSize: 12 }

                        RowLayout {
                            spacing: 6

                            CheckBox {
                                id: anomalyEnableBox
                                checked: anomalyMonitor ? anomalyMonitor.enabled : false
                                text: "Enable"
                                onCheckedChanged: {
                                    if (anomalyMonitor) anomalyMonitor.enabled = checked
                                }
                            }

                            Label {
                                text: "Score: " + (anomalyMonitor ? anomalyMonitor.lastScore.toFixed(4) : "—")
                                font.pixelSize: 10
                                color: anomalyMonitor && anomalyMonitor.lastScore > (anomalyMonitor.thresholdMultiplier * 0.05) ? "#f66" : "#aaa"
                            }
                        }

                        RowLayout {
                            spacing: 6

                            Label { text: "Sensitivity"; font.pixelSize: 10; color: "#888" }

                            Slider {
                                Layout.fillWidth: true
                                from: 0.1
                                to: 5.0
                                stepSize: 0.1
                                value: anomalyMonitor ? anomalyMonitor.thresholdMultiplier : 1.0
                                onValueChanged: {
                                    if (anomalyMonitor) anomalyMonitor.thresholdMultiplier = value
                                }
                            }

                            Label {
                                text: anomalyMonitor ? anomalyMonitor.thresholdMultiplier.toFixed(1) + "x" : "1.0x"
                                font.pixelSize: 10
                                color: "#aaa"
                            }
                        }

                        Label {
                            text: "Samples: " + (anomalyMonitor ? anomalyMonitor.sampleCount : 0)
                            font.pixelSize: 9
                            color: "#666"
                        }
                    }
                }

                // Live instrument parameter dashboard
                Pane {
                    Layout.fillWidth: true
                    visible: discoveryPanelManager && discoveryPanelManager.instrumentCount > 0
                    background: Rectangle {
                        color: "#1a1a2e"
                        radius: 6
                        border.color: "#2a2a4a"
                        border.width: 1
                    }

                    DiscoveryDashboard {
                        manager: discoveryPanelManager
                        onParametersChanged: console.log("Discovery parameters updated")
                    }
                }

                // Planck Microscope toggle
                Button {
                    text: "🔭 Planck Microscope"
                    Layout.fillWidth: true
                    checkable: true
                    checked: planckContainer && planckContainer.visible
                    onClicked: {
                        if (planckContainer) {
                            planckContainer.visible = checked
                            if (checked) {
                                planckContainer.raise()
                            }
                        }
                    }
                }

                // Multi-user collaboration panel
                Pane {
                    Layout.fillWidth: true
                    visible: signalingClient != null && signalingClient.isConnected
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 4

                        Label { text: "Multi-User Collaboration"; font.bold: true; font.pixelSize: 12 }

                        RowLayout {
                            spacing: 6

                            Button {
                                text: sharedSession && sharedSession.isActive ? "⏹ Stop Sharing" : "▶ Share Session"
                                Layout.fillWidth: true
                                onClicked: {
                                    if (sharedSession) {
                                        sharedSession.setActive(!sharedSession.isActive)
                                    }
                                }
                            }
                        }

                        Label {
                            text: "Session: " + (sharedSession && sharedSession.sessionId && sharedSession.sessionId !== "" ? sharedSession.sessionId : "Not connected")
                            font.pixelSize: 10
                            color: "#888"
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "Peers: " + (signalingClient ? (sharedSession && sharedSession.isActive ? signalingClient.peerCount : 0) : 0)
                            font.pixelSize: 10
                            color: "#888"
                            Layout.fillWidth: true
                        }
                    }
                }

                Label {
                    text: discoveryPanelManager && discoveryPanelManager.scanRunning
                        ? "Scanning... " + Math.round(discoveryPanelManager.scanProgress * 100) + "%"
                        : discoveryPanelManager && discoveryPanelManager.findingsList.length > 0
                            ? discoveryPanelManager.findingsList.length + " findings"
                            : "Ready"
                    font.pixelSize: 10
                    color: "#666"
                    Layout.fillWidth: true
                }

                // Curvature probe readout
                                Pane {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 110
                
                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        spacing: 2
                
                                        Label { text: "Probe Readout"; font.bold: true; font.pixelSize: 12 }

                                        RowLayout {
                                            Label { text: "Kretschmann:"; color: "#888" }
                                            Label {
                                                text: viewportItem && viewportItem.kretschmann ? viewportItem.kretschmann : "—"
                                                color: "#4af"
                                                font.family: "monospace"
                                                Layout.fillWidth: true
                                                horizontalAlignment: Text.AlignRight
                                            }
                                        }

                                        RowLayout {
                                            Label { text: "Ricci Scalar:"; color: "#888" }
                                            Label {
                                                text: viewportItem && viewportItem.ricciScalar ? viewportItem.ricciScalar : "—"
                                                color: "#4af"
                                                font.family: "monospace"
                                                Layout.fillWidth: true
                                                horizontalAlignment: Text.AlignRight
                                            }
                                        }

                                        RowLayout {
                                            Label { text: "Weyl Scalar:"; color: "#888" }
                                            Label {
                                                text: viewportItem && viewportItem.weylSquared ? viewportItem.weylSquared : "—"
                                                color: "#4af"
                                                font.family: "monospace"
                                                Layout.fillWidth: true
                                                horizontalAlignment: Text.AlignRight
                                            }
                                        }

                                        RowLayout {
                                            Label { text: "Redshift:"; color: "#888" }
                                            Label {
                                                text: viewportItem && viewportItem.redshift ? viewportItem.redshift : "—"
                                                color: "#4af"
                                                font.family: "monospace"
                                                Layout.fillWidth: true
                                                horizontalAlignment: Text.AlignRight
                                            }
                                        }

                                        Connections {
                                            target: sceneGraphModel
                                            function onSelectedObjectIdChanged() {
                                                console.log("DIAG selectedObjectIdChanged ->", sceneGraphModel.selectedObjectId)
                                                var pos = sceneGraphModel.selectedObjectPosition()
                                                console.log("DIAG selectedObjectPosition ->", JSON.stringify(pos))
                                                if (!viewportItem) return
                                                if (pos && pos.x !== undefined) viewportItem.probeAt(pos.x, pos.y, pos.z)
                                                else { console.log("DIAG no position, clearProbe"); viewportItem.clearProbe() }
                                            }
                                        }
                                    }
                                }

                // Anomaly feed
                Pane {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 4

                        Label { text: "Anomaly Feed"; font.bold: true; font.pixelSize: 12 }

                        // Filter / sort controls
                        RowLayout {
                            spacing: 4

                            ComboBox {
                                id: severityFilter
                                Layout.preferredWidth: 70
                                model: ["All", "CRITICAL", "HIGH", "MEDIUM", "LOW", "INFO"]
                                onCurrentTextChanged: findingsModel.filterSeverity =
                                    (currentText === "All" ? "" : currentText)
                            }
                            ComboBox {
                                id: instrumentFilter
                                Layout.preferredWidth: 160
                                property var names: {
                                    var base = ["All"];
                                    var extra = ["LIGO", "IceCube"];
                                    var names = discoveryPanelManager ? discoveryPanelManager.instrumentNames : [];
                                    for (var i = 0; i < names.length; ++i) {
                                        if (names[i] !== "All" && !extra.includes(names[i])) {
                                            base.push(names[i]);
                                        }
                                    }
                                    for (var j = 0; j < extra.length; ++j) {
                                        base.push(extra[j]);
                                    }
                                    return base;
                                }
                                model: names
                                onCurrentTextChanged: findingsModel.filterInstrument =
                                    (currentText === "All" ? "" : currentText)
                                contentItem: Text {
                                    text: instrumentFilter.currentText
                                    font: instrumentFilter.font
                                    elide: Text.ElideRight
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: instrumentFilter.leftPadding
                                    rightPadding: instrumentFilter.rightPadding
                                }
                            }
                            ComboBox {
                                id: sortCombo
                                Layout.preferredWidth: 80
                                model: ["Newest", "Severity", "Confidence"]
                                onCurrentTextChanged: {
                                    if (currentText === "Newest") {
                                        findingsModel.sortRole = "timestamp"
                                        findingsModel.sortAscending = false
                                    } else if (currentText === "Severity") {
                                        findingsModel.sortRole = "severity"
                                        findingsModel.sortAscending = false
                                    } else if (currentText === "Confidence") {
                                        findingsModel.sortRole = "confidence"
                                        findingsModel.sortAscending = false
                                    }
                                }
                            }
                        }

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            ListView {
                                id: anomalyList
                                model: findingsModel
                                delegate: ItemDelegate {
                                    width: parent ? parent.width : 0
                                    padding: 4
                                    onClicked: findingsModel.select(index)
                                    background: Rectangle {
                                        anchors.fill: parent
                                        radius: 4
                                        color: model.isAnomaly ? "#4a1a1a" : "transparent"
                                        border.color: model.isAnomaly ? "#f44" : "transparent"
                                        border.width: model.isAnomaly ? 1 : 0
                                    }

                                    RowLayout {
                                        spacing: 6

                                        Label {
                                            text: model.isAnomaly ? "‼" : ""
                                            font.pixelSize: 14
                                            color: "#f44"
                                            visible: model.isAnomaly
                                        }

                                        Rectangle {
                                            width: 8; height: 8; radius: 4
                                            color: model.severity === "CRITICAL" ? "#f44" :
                                                  model.severity === "HIGH" ? "#fa4" :
                                                  model.severity === "MEDIUM" ? "#faa" : "#4f4"
                                            visible: !model.isAnomaly
                                        }

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 1

                                            Label {
                                                text: model.instrumentName || ""
                                                font.pixelSize: 10
                                                color: model.isAnomaly ? "#f66" : "#888"
                                                font.bold: true
                                                elide: Text.ElideRight
                                                Layout.fillWidth: true
                                            }

                                            Label {
                                                text: model.description || ""
                                                font.pixelSize: 11
                                                color: model.isAnomaly ? "#f66" :
                                                       model.severity === "CRITICAL" ? "#f66" :
                                                       model.severity === "HIGH" ? "#fd4" :
                                                       model.severity === "MEDIUM" ? "#fa8" : "#6f6"
                                                wrapMode: Text.Wrap
                                                Layout.fillWidth: true
                                            }

                                            Label {
                                                text: "σ" + (model.confidence !== undefined ? (model.confidence * 10).toFixed(1) : "?")
                                                font.pixelSize: 9
                                                color: model.isAnomaly ? "#f88" : "#666"
                                            }
                                            Label {
                                                text: model.timestamp !== undefined ? "t=" + model.timestamp.toFixed(1) + "s" : ""
                                                font.pixelSize: 9
                                                color: model.isAnomaly ? "#f88" : "#444"
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Replay Archive panel
                        Pane {
                            id: replayContainer
                            Layout.fillWidth: true
                            Layout.preferredHeight: 140
                            visible: replayContainer && replayContainer.visible

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 4

                                Label { text: "Replay Archive"; font.bold: true; font.pixelSize: 12 }

                                RowLayout {
                                    spacing: 4

                                    TextField {
                                        id: replayDirField
                                        placeholderText: "Archive directory..."
                                        Layout.fillWidth: true
                                        font.pixelSize: 10
                                        text: "data/gcn_archive"
                                    }

                                    Button {
                                        text: replayStream && replayStream.running ? "Stop" : "Replay"
                                        Layout.preferredWidth: 70
                                        onClicked: {
                                            if (!replayStream) return
                                            if (replayStream.running) {
                                                replayStream.stop()
                                            } else {
                                                replayStream.setDirectory(replayDirField.text)
                                                replayStream.setSpeed(replaySpeedSlider.value)
                                                replayStream.start()
                                            }
                                        }
                                    }
                                }

                                RowLayout {
                                    spacing: 4

                                    Label { text: "Speed:"; color: "#888"; font.pixelSize: 10 }

                                    Slider {
                                        id: replaySpeedSlider
                                        from: 1; to: 100; stepSize: 1; value: 10
                                        Layout.fillWidth: true
                                        onValueChanged: {
                                            if (replayStream) replayStream.setSpeed(value)
                                        }
                                    }

                                    Label {
                                        text: replaySpeedSlider.value.toFixed(0) + "x"
                                        color: "#ccc"
                                        font.pixelSize: 10
                                        Layout.preferredWidth: 30
                                    }
                                }

                                Label {
                                    text: replayStream
                                        ? ("Loaded " + replayStream.totalFiles + " files, playing #" + replayStream.currentIndex)
                                        : "No archive loaded"
                                    font.pixelSize: 10
                                    color: "#666"
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        // Multi-messenger correlation panel
                        Pane {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 160

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 4

                                Label { text: "Multi-Messenger Correlations"; font.bold: true; font.pixelSize: 12 }

                                ScrollView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true

                                    ListView {
                                        model: discoveryPanelManager.correlationsList
                                        delegate: ItemDelegate {
                                            width: parent ? parent.width : 0
                                            padding: 4
                                            background: Rectangle {
                                                anchors.fill: parent
                                                radius: 4
                                                color: model.severity === "CRITICAL" ? "#4a1a1a" : "#1a1a2e"
                                                border.color: model.severity === "CRITICAL" ? "#f44" : "#333"
                                                border.width: 1
                                            }

                                            RowLayout {
                                                spacing: 6

                                                Label {
                                                    text: model.severity === "CRITICAL" ? "‼" : "🔗"
                                                    font.pixelSize: 14
                                                    color: model.severity === "CRITICAL" ? "#f44" : "#4af"
                                                }

                                                ColumnLayout {
                                                    Layout.fillWidth: true
                                                    spacing: 1

                                                    Label {
                                                        text: model.messengers ? model.messengers.join(" + ") : ""
                                                        font.pixelSize: 10
                                                        color: "#4af"
                                                        font.bold: true
                                                        elide: Text.ElideRight
                                                        Layout.fillWidth: true
                                                    }

                                                    Label {
                                                        text: model.description || ""
                                                        font.pixelSize: 10
                                                        color: "#ccc"
                                                        wrapMode: Text.Wrap
                                                        Layout.fillWidth: true
                                                    }

                                                    RowLayout {
                                                        spacing: 6
                                                        Label {
                                                            text: "σ" + (model.combinedConfidence !== undefined ? (model.combinedConfidence * 10).toFixed(1) : "?")
                                                            font.pixelSize: 9
                                                            color: "#888"
                                                        }
                                                        Label {
                                                            text: model.spatialScore !== undefined ? "proximity=" + (model.spatialScore * 100).toFixed(0) + "%" : ""
                                                            font.pixelSize: 9
                                                            color: "#666"
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }

                                Label {
                                    text: discoveryPanelManager.correlationCount > 0
                                        ? discoveryPanelManager.correlationCount + " correlation(s) detected"
                                        : "Awaiting multi-messenger coincidences..."
                                    font.pixelSize: 10
                                    color: "#666"
                                    Layout.fillWidth: true
                                }
                            }
                        }
                }

                // Detail pane for the selected finding
                        Pane {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 96
                            padding: 6
                            visible: findingsModel.currentFinding["id"] !== undefined
                            background: Rectangle { color: "#161616"; radius: 4 }

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 2

                                Label {
                                    text: (findingsModel.currentFinding["instrumentName"] || "")
                                          + "  [" + (findingsModel.currentFinding["severity"] || "") + "]"
                                    font.bold: true
                                    font.pixelSize: 11
                                    color: "#ddd"
                                }
                                Label {
                                    text: findingsModel.currentFinding["description"] || ""
                                    font.pixelSize: 10
                                    color: "#aaa"
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
                                }
                                Label {
                                    text: "σ" + (findingsModel.currentFinding["confidence"] !== undefined
                                          ? (findingsModel.currentFinding["confidence"] * 10).toFixed(1) : "?")
                                          + "   t=" + (findingsModel.currentFinding["timestamp"] !== undefined
                                          ? findingsModel.currentFinding["timestamp"].toFixed(1) : "?") + "s"
                                    font.pixelSize: 9
                                    color: "#777"
                                }
                            }
                        }
                    }
                }

                // Parameter sweep controls
                Pane {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 4

                        Label { text: "Parameter Sweep"; font.bold: true; font.pixelSize: 12 }

                        RowLayout {
                            Label { text: "Parameter:"; color: "#888" }
                            ComboBox {
                                id: sweepParamCombo
                                model: ["Mass", "Spin", "Charge", "Distance"]
                                Layout.fillWidth: true
                                font.pixelSize: 11
                            }
                        }

                        RowLayout {
                            Label { text: "Range:"; color: "#888" }
                            TextField { id: sweepFromField; text: "0.1"; Layout.fillWidth: true; font.pixelSize: 11 }
                            Label { text: "to"; color: "#888" }
                            TextField { id: sweepToField; text: "10.0"; Layout.fillWidth: true; font.pixelSize: 11 }
                        }

                        RowLayout {
                            Label { text: "Steps:"; color: "#888" }
                            SpinBox { id: sweepStepsBox; from: 10; to: 1000; value: 50; Layout.fillWidth: true }
                        }

                        Button {
                            text: "Run Sweep"
                            Layout.fillWidth: true
                            onClicked: runParameterSweep()
                        }

                        ProgressBar {
                            value: sweepProgress
                            visible: sweepRunning
                            Layout.fillWidth: true
                        }
                    }
                }
            }
            }
        }

    // =========================================================================
    // TIMELINE BAR (Bottom)
    // =========================================================================
    footer: Pane {
        height: 100

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 4
            spacing: 4

            // Transport controls + speed
            RowLayout {
                spacing: 4

                ToolButton {
                    icon.source: "qrc:/icons/step_back.svg"
                    ToolTip.text: "Step backward"
                    onClicked: stepSimulation(-0.1)
                }

                ToolButton {
                    icon.source: simulationPaused ? "qrc:/icons/play.svg" : "qrc:/icons/pause.svg"
                    ToolTip.text: simulationPaused ? "Play" : "Pause"
                    onClicked: simulationPaused = !simulationPaused
                }

                ToolButton {
                    icon.source: "qrc:/icons/step_forward.svg"
                    ToolTip.text: "Step forward"
                    onClicked: stepSimulation(0.1)
                }

                ToolButton {
                    icon.source: "qrc:/icons/stop.svg"
                    ToolTip.text: "Reset"
                    onClicked: resetSimulation()
                }

                Item { Layout.fillWidth: true }

                ToolButton {
                    text: "1x"
                    checkable: true
                    checked: timeScale === 1
                    onClicked: toolBar.timeScaleSliderValue = 1
                    font.pixelSize: 10
                }
                ToolButton {
                    text: "10x"
                    checkable: true
                    checked: Math.abs(timeScale - 10) < 0.1
                    onClicked: toolBar.timeScaleSliderValue = 2
                    font.pixelSize: 10
                }
                ToolButton {
                    text: "100x"
                    checkable: true
                    checked: Math.abs(timeScale - 100) < 1
                    onClicked: toolBar.timeScaleSliderValue = 3
                    font.pixelSize: 10
                }
            }

            // Time slider
            RowLayout {
                spacing: 8

                Label {
                    text: "tau"
                    color: "#888"
                    font.bold: true
                }

                Slider {
                    id: timeSlider
                    from: 0; to: 1000; stepSize: 0.1; value: 0
                    Layout.fillWidth: true
                    onMoved: simulationTime = value
                }

                Label {
                    text: simulationTime.toFixed(2) + " s"
                    color: "#ccc"
                    font.family: "monospace"
                    Layout.preferredWidth: 80
                }
            }
        }
    }

    // =========================================================================
    // DIALOGS
    // =========================================================================
    // Commented out for Qt 6.5.3: QtQuick.Dialogs 1.3 module not available
    /*
    FileDialog {
        id: fileDialog
        title: "Open Spacetime Configuration"
        nameFilters: ["Spacetime files (*.spc *.json)", "All files (*)"]
        onAccepted: loadConfiguration(fileDialog.selectedFile)
    }

    FileDialog {
        id: saveDialog
        title: "Save Snapshot"
        selectExisting: false
        nameFilters: ["Image files (*.png *.jpg)", "All files (*)"]
        onAccepted: saveSnapshot(saveDialog.selectedFile)
    }

    Dialog {
        id: aboutDialog
        title: "About QuantumVerse"
        standardButtons: Dialog.Ok
        modal: true

        ColumnLayout {
            Label {
                text: "QuantumVerse Simulator"
                font.pixelSize: 18
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: "4D Spacetime Explorer\n" +
                      "Version 1.0.0\n\n" +
                      "A general relativity and quantum gravity simulator\n" +
                      "inspired by Solar System Scope.\n\n" +
                      "Copyright (c) 2026 QuantumVerse Team"
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
    */

    // =========================================================================
    // SIMULATION LOGIC
    // =========================================================================
    Timer {
        id: simTimer
        interval: 16  // ~60 FPS
        // Interactive clock. In headless mode the C++ frame loop steps the
        // physics itself (dt = interval * timeScale), so this timer stands
        // down to keep one clock per run - otherwise each frame would take
        // two differently scaled steps.
        running: !simulationPaused && !headlessPhysics
        repeat: true
        onTriggered: {
            console.log("simTimer tick, paused=" + simulationPaused)
            var dt = (interval / 1000.0) * timeScale
            simulationTime += dt
            if (viewportItem) viewportItem.updateSimulation(dt)
            timeSlider.value = simulationTime
        }
    }

    // Camera4D animation tick (drives smooth transitions and inertia)
    Timer {
        id: cameraAnimTimer
        objectName: "cameraAnimTimer"
        // GATE: do not run the 4D camera animation tick when the camera came
        // from --cam-* CLI flags. A pending focus transition would otherwise
        // interpolate the camera toward its own target and drift off the CLI
        // values over ~1s.
        running: !cliCameraSet
        interval: 16  // ~60 FPS
        repeat: true
        onTriggered: {
            if (viewportItem && viewportItem.camera4DAdapter) {
                viewportItem.camera4DAdapter.onFrame(interval / 1000.0)
            }
        }
    }

    // =========================================================================
    // HELPER FUNCTIONS
    // =========================================================================
    function stepSimulation(dt) {
        if (dt === undefined) dt = 0.1
        simulationTime += dt
        if (viewportItem) viewportItem.updateSimulation(dt)
        timeSlider.value = simulationTime
    }

    function resetSimulation() {
        simulationTime = 0
        timeScale = 1
        toolBar.timeScaleSliderValue = 1
        timeSlider.value = 0
        if (viewportItem) viewportItem.resetView()
    }

    function loadConfiguration(url) {
        console.log("Loading configuration from:", url)
    }

    function saveSnapshot(url) {
        console.log("Saving snapshot to:", url)
    }

    function runParameterSweep() {
        console.log("Running parameter sweep...")
    }
}
