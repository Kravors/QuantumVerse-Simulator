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
    width: 1400
    height: 900
    title: "QuantumVerse Simulator - 4D Spacetime Explorer"
    color: "#0a0a1a"

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            console.log("TEST TIMER FIRED")
        }
    }

    property bool noViewport: Qt.application.arguments.indexOf("--noviewport") !== -1
    property Item viewportItem: viewportLoader.item

    Component.onCompleted: {
        console.log("sceneGraphModel count:", sceneGraphModel ? sceneGraphModel.count : "model not available")
        console.log("discoveryInstruments count:", discoveryPanelManager ? discoveryPanelManager.instrumentCount : "not available")
        console.log("noViewport=" + noViewport + " args=" + Qt.application.arguments)
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
        if (kg >= 1e30) return (kg / 1e30).toFixed(3) + " x 10^30 kg"
        if (kg >= 1e24) return (kg / 1e24).toFixed(3) + " x 10^24 kg"
        if (kg >= 1e6) return (kg / 1e6).toFixed(1) + " x 10^6 kg"
        return kg.toFixed(1) + " kg"
    }

    // =========================================================================
    // MENU BAR
    // =========================================================================
    menuBar: MenuBar {
        Menu {
            title: "&File"
            // Commented out for Qt 6.5.3: QtQuick.Dialogs 1.3 module not available
            // MenuItem { text: "&Open Spacetime..."; onTriggered: fileDialog.open() }
            // MenuItem { text: "&Save Snapshot..."; onTriggered: saveDialog.open() }
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
                    if (viewportItem) viewportItem.showGrid = checked
                }
            }
            MenuItem {
                text: "Show &Light Cones"
                checkable: true; checked: false
                onToggled: {
                    if (viewportItem) viewportItem.showLightCones = checked
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
        RowLayout {
            anchors.fill: parent
            spacing: 4

            ToolButton {
                icon.source: "qrc:/icons/open.svg"
                ToolTip.text: "Open Spacetime Configuration"
                // Commented out for Qt 6.5.3: QtQuick.Dialogs 1.3 module not available
                // onClicked: fileDialog.open()
            }

            ToolSeparator {}

            ToolButton {
                text: "Reset View"
                onClicked: {
                    if (viewportItem) viewportItem.resetView()
                }
                ToolTip.text: "Reset camera to default view"
            }

            ToolButton {
                text: "Focus Sun"
                onClicked: camController.reset()
                ToolTip.text: "Focus on central body"
            }

            ToolSeparator {}

            // Rotation Plane Selection
            ToolButton {
                text: "XY"
                checkable: true
                checked: viewportItem && viewportItem.camera4DAdapter ? viewportItem.camera4DAdapter.activePlane === 0 : false
                onClicked: if (viewportItem && viewportItem.camera4DAdapter) viewportItem.camera4DAdapter.activePlane = 0
                ToolTip.text: "Rotate in XY plane (azimuth/elevation)"
            }
            ToolButton {
                text: "XZ"
                checkable: true
                checked: viewportItem && viewportItem.camera4DAdapter ? viewportItem.camera4DAdapter.activePlane === 1 : false
                onClicked: if (viewportItem && viewportItem.camera4DAdapter) viewportItem.camera4DAdapter.activePlane = 1
                ToolTip.text: "Rotate in XZ plane"
            }
            ToolButton {
                text: "YZ"
                checkable: true
                checked: viewportItem && viewportItem.camera4DAdapter ? viewportItem.camera4DAdapter.activePlane === 2 : false
                onClicked: if (viewportItem && viewportItem.camera4DAdapter) viewportItem.camera4DAdapter.activePlane = 2
                ToolTip.text: "Rotate in YZ plane"
            }
            ToolButton {
                text: "TX"
                checkable: true
                checked: viewportItem && viewportItem.camera4DAdapter ? viewportItem.camera4DAdapter.activePlane === 3 : false
                onClicked: if (viewportItem && viewportItem.camera4DAdapter) viewportItem.camera4DAdapter.activePlane = 3
                ToolTip.text: "Rotate in time-X plane (boost)"
            }
            ToolButton {
                text: "TY"
                checkable: true
                checked: viewportItem && viewportItem.camera4DAdapter ? viewportItem.camera4DAdapter.activePlane === 4 : false
                onClicked: if (viewportItem && viewportItem.camera4DAdapter) viewportItem.camera4DAdapter.activePlane = 4
                ToolTip.text: "Rotate in time-Y plane (boost)"
            }
            ToolButton {
                text: "TZ"
                checkable: true
                checked: viewportItem && viewportItem.camera4DAdapter ? viewportItem.camera4DAdapter.activePlane === 5 : false
                onClicked: if (viewportItem && viewportItem.camera4DAdapter) viewportItem.camera4DAdapter.activePlane = 5
                ToolTip.text: "Rotate in time-Z plane (boost)"
            }

            ToolSeparator {}

            ToolButton {
                text: simulationPaused ? "▶ Play" : "⏸ Pause"
                checkable: true
                checked: !simulationPaused
                onClicked: simulationPaused = !checked
                ToolTip.text: "Play/Pause simulation"
            }

            Slider {
                id: timeScaleSlider
                from: 0; to: 6; stepSize: 0.1; value: 1
                onValueChanged: timeScale = Math.pow(10, value - 1)
                ToolTip.text: "Time scale: " + timeScale.toFixed(1) + "x"
                Layout.preferredWidth: 100
            }

            Label {
                text: timeScale.toFixed(1) + "x"
                font.pixelSize: 11
                color: "#aaa"
            }

            ToolSeparator {}

            ComboBox {
                id: curvatureModeCombo
                model: ["Grid Deformation", "Riemann Color", "Curvature Scalar", "Time Dilation", "Geodesic Flow"]
                currentIndex: 0
                onCurrentIndexChanged: if (viewportItem) viewportItem.curvatureMode = currentIndex
                Layout.preferredWidth: 178
                ToolTip.text: "Curvature visualization mode"
                contentItem: Text {
                    text: curvatureModeCombo.currentText
                    font: curvatureModeCombo.font
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: curvatureModeCombo.leftPadding
                    rightPadding: curvatureModeCombo.rightPadding
                }
            }

            ToolSeparator {}

            ToolButton {
                text: "Discover"
                checkable: true
                checked: discoveryPanel.visible
                onClicked: discoveryPanel.visible = checked
                ToolTip.text: "Toggle Discovery Console"
            }

            Item { Layout.fillWidth: true }

            Label {
                text: "FPS: " + (viewportItem && viewportItem.frameRate > 0 ? viewportItem.frameRate.toFixed(1) : "—")
                color: viewportItem && viewportItem.frameRate >= 45 ? "#4f4" : "#f44"
                font.pixelSize: 14
                font.bold: true
                Layout.rightMargin: 8
            }
        }
    }

    // =========================================================================
    // MAIN CONTENT AREA
    // =========================================================================
    RowLayout {
        id: mainContent
        // In an ApplicationWindow, declared children are parented to the
        // contentItem, which already excludes the header/footer. Filling
        // `parent` therefore reserves space for the footer correctly.
        // (`parent.contentItem` was undefined and collapsed the layout,
        // letting the panes overpaint the footer.)
        anchors.fill: parent
        spacing: 2

        // --- LEFT SIDEBAR: Object Browser ---
        Pane {
            id: leftSidebar
            Layout.preferredWidth: 280
            Layout.minimumWidth: 200
            Layout.maximumWidth: 400
            Layout.fillHeight: true
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
                                // Note: Filtering is handled by the model's data() method
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
                if (kg >= 1e30) return (kg / 1e30).toFixed(3) + " x 10^30 kg"
                if (kg >= 1e24) return (kg / 1e24).toFixed(3) + " x 10^24 kg"
                if (kg >= 1e6) return (kg / 1e6).toFixed(1) + " x 10^6 kg"
                return kg.toFixed(1) + " kg"
            }
        }

        // --- CENTRAL VIEWPORT ---
        Pane {
            id: viewportContainer
            Layout.fillWidth: true
            Layout.fillHeight: true

            Loader {
                id: viewportLoader
                anchors.fill: parent
                Layout.fillWidth: true
                Layout.fillHeight: true
                sourceComponent: noViewport ? placeholderComponent : viewportComponent
            }

             Component {
                 id: viewportComponent
                 QmlGlViewport {
                     id: viewport
                     anchors.fill: parent
                     Layout.fillWidth: true
                     Layout.fillHeight: true
                     showGrid: true
                     showGeodesics: true
                     showQuantumGeometry: false
                     Component.onCompleted: console.log("Viewport created, size:", width, "x", height)

                      Timer {
                          interval: 16
                          running: true
                          repeat: true
                          onTriggered: {
                              console.log("Timer tick, viewport size:", viewport.width, "x", viewport.height)
                              viewport.update()
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
                            viewport.handleWheel(wheel.angleDelta.y)
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
                      "Camera: az=" + (camController.azimuth * 180 / Math.PI).toFixed(1) + " el=" + (camController.elevation * 180 / Math.PI).toFixed(1) + " d=" + camController.distance.toFixed(1) +
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
            Layout.preferredWidth: 340
            Layout.minimumWidth: 260
            Layout.maximumWidth: 500
            Layout.fillHeight: true
            visible: true
            z: 1
            enabled: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
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
                                            Label { text: "Kretschmann:"; color: "#888"; Layout.fillWidth: true }
                                            Label { text: viewportItem.kretschmann; color: "#4af"; font.family: "monospace" }
                                        }

                                        RowLayout {
                                            Label { text: "Ricci Scalar:"; color: "#888"; Layout.fillWidth: true }
                                            Label { text: viewportItem.ricciScalar; color: "#4af"; font.family: "monospace" }
                                        }

                                        RowLayout {
                                            Label { text: "Weyl Scalar:"; color: "#888"; Layout.fillWidth: true }
                                            Label { text: viewportItem.weylSquared; color: "#4af"; font.family: "monospace" }
                                        }

                                        RowLayout {
                                            Label { text: "Redshift:"; color: "#888"; Layout.fillWidth: true }
                                            Label { text: viewportItem.redshift; color: "#4af"; font.family: "monospace" }
                                        }

                                        Connections {
                                            target: sceneGraphModel
                                            function onSelectedObjectIdChanged() {
                                                console.log("DIAG selectedObjectIdChanged ->", sceneGraphModel.selectedObjectId)
                                                var pos = sceneGraphModel.selectedObjectPosition()
                                                console.log("DIAG selectedObjectPosition ->", JSON.stringify(pos))
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
                                Layout.preferredWidth: 92
                                model: ["All", "CRITICAL", "HIGH", "MEDIUM", "LOW", "INFO"]
                                onCurrentTextChanged: findingsModel.filterSeverity =
                                    (currentText === "All" ? "" : currentText)
                            }
                            ComboBox {
                                id: instrumentFilter
                                Layout.preferredWidth: 210
                                property var names: ["All"].concat(
                                    discoveryPanelManager ? discoveryPanelManager.instrumentNames : [],
                                    ["LIGO", "IceCube"])
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
                                Layout.preferredWidth: 104
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
                            Label { text: "Parameter:"; color: "#888"; Layout.fillWidth: true }
                            ComboBox {
                                id: sweepParamCombo
                                model: ["Mass", "Spin", "Charge", "Distance"]
                                Layout.fillWidth: true
                                font.pixelSize: 11
                            }
                        }

                        RowLayout {
                            Label { text: "Range:"; color: "#888"; Layout.fillWidth: true }
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

    // =========================================================================
    // TIMELINE BAR (Bottom)
    // =========================================================================
    footer: Pane {
        height: 60

        RowLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 12

            // Transport controls
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
            }

            // Time slider
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

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

                // Slice index selector
                RowLayout {
                    spacing: 8

                    Label { text: "Slice:"; color: "#666"; font.pixelSize: 10 }

                    ComboBox {
                        id: sliceCombo
                        model: ["t = const", "tau = const", "Null", "Re(complex)", "Im(complex)"]
                        currentIndex: 0
                        Layout.preferredWidth: 120
                        font.pixelSize: 10
                    }

                    Slider {
                        id: sliceSlider
                        from: -50; to: 50; stepSize: 0.1; value: 0
                        Layout.fillWidth: true
                        onMoved: {
                            if (viewportItem) {
                                viewportItem.setSliceOffset(sliceCombo.currentIndex, value)
                            }
                        }
                    }
                }
            }

            // Speed indicator
            ColumnLayout {
                ToolButton {
                    text: "1x"
                    checkable: true
                    checked: timeScale === 1
                    onClicked: timeScaleSlider.value = 1
                    font.pixelSize: 10
                }
                ToolButton {
                    text: "10x"
                    checkable: true
                    checked: Math.abs(timeScale - 10) < 0.1
                    onClicked: timeScaleSlider.value = 2
                    font.pixelSize: 10
                }
                ToolButton {
                    text: "100x"
                    checkable: true
                    checked: Math.abs(timeScale - 100) < 0.1
                    onClicked: timeScaleSlider.value = 3
                    font.pixelSize: 10
                }
            }

            // Live ingest status indicator
            Item {
                Layout.preferredWidth: 12
                Layout.preferredHeight: 12
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                Rectangle {
                    anchors.centerIn: parent
                    width: 10; height: 10; radius: 5
                    color: "#4caf50"
                    border.color: "#81c784"
                    border.width: 1
                    opacity: 0.4 + 0.6 * (0.5 + 0.5 * Math.sin(Date.now() / 300))
                    ToolTip.visible: Qt.application.arguments.indexOf("--headless") === -1
                    ToolTip.text: kafkaListener && kafkaListener.consumerError.length === 0
                        ? "Live GCN ingest connected"
                        : "Live GCN ingest disconnected"
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
        running: !simulationPaused
        repeat: true
        onTriggered: {
            console.log("simTimer tick, paused=" + simulationPaused)
            var dt = (interval / 1000.0) * timeScale
            simulationTime += dt
            viewportItem.updateSimulation(dt)
            timeSlider.value = simulationTime
        }
    }

    // Camera4D animation tick (drives smooth transitions and inertia)
    Timer {
        id: cameraAnimTimer
        interval: 16  // ~60 FPS
        running: true
        repeat: true
        onTriggered: {
            if (viewportItem && viewportItem.camera4DAdapter) {
                viewportItem.camera4DAdapter.onFrame(interval / 1000.0)
            }
        }
    }

    // Continuous render update timer for QmlGlViewport
    Timer {
        interval: 16  // ~60 FPS
        running: true
        repeat: true
        onTriggered: {
            console.log("Render timer tick, viewport size:", viewportItem ? viewportItem.width + "x" + viewportItem.height : "null")
            if (viewportItem && viewportItem.width > 0 && viewportItem.height > 0) {
                viewportItem.update()
            }
        }
    }

    // =========================================================================
    // HELPER FUNCTIONS
    // =========================================================================
    function stepSimulation(dt) {
        if (dt === undefined) dt = 0.1
        simulationTime += dt
        viewportItem.updateSimulation(dt)
        timeSlider.value = simulationTime
    }

    function resetSimulation() {
        simulationTime = 0
        timeScale = 1
        timeScaleSlider.value = 1
        timeSlider.value = 0
        viewportItem.resetView()
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
