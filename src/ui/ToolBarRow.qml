/**
 * @file ToolBarRow.qml
 * @brief Extracted top toolbar component for QuantumVerse 4D Simulator
 *
 * Commit 1 of UI session: pixel-identical extraction of the toolbar from main.qml.
 * All Layout properties, spacing, control order, and visual output are preserved
 * exactly. External dependencies are injected as properties; user interactions are
 * exposed as signals so main.qml retains full control over behavior.
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls 6.0

ColumnLayout {
    id: toolbarLayout
    spacing: 2

    // === External dependencies (injected from main.qml) ===
    property Item targetViewport
    property Item targetCamController
    property bool simulationPaused: false
    property Item discoveryPanel
    property var tourController
    property var tourManager
    property var signalingClient

    // === Aliases for child ids referenced externally ===
    property alias rootLayout: toolbarLayout
    property alias timeScaleSliderValue: timeScaleSlider.value

    // === Signals ===
    signal resetViewClicked()
    signal focusSunClicked()
    signal pauseToggled(bool paused)
    signal planeAxisChanged(int axis)
    signal timeScaleChanged(real value)
    signal curvatureModeChanged(int mode)
    signal planeResolutionChanged(int resolution)
    signal blackHoleMassChanged(real mass)
    signal zoomChanged(real distance)
    signal blackHoleTypeChanged(string type)
    signal physicsGLogChanged(real logG)
    signal gravitationalWaveAudioToggled(bool enabled)
    signal injectAsteroid()
    signal clearAsteroids()
    signal discoveryToggled(bool visible)
    signal tourToggled(bool active)
    signal vrToggled(bool active)

    // Row 1: navigation, view controls, rotation planes, playback
    RowLayout {
        Layout.fillWidth: true
        spacing: 4

        ToolButton {
            icon.source: "qrc:/icons/open.svg"
            ToolTip.text: "Open Spacetime Configuration"
        }

        ToolSeparator {}

        ToolButton {
            text: "Reset View"
            onClicked: toolbarLayout.resetViewClicked()
            ToolTip.text: "Reset camera to default view"
        }

        ToolButton {
            text: "Focus Sun"
            onClicked: toolbarLayout.focusSunClicked()
            ToolTip.text: "Focus on central body"
        }

        ToolSeparator {}

        ToolButton {
            text: "XY"
            checkable: true
            checked: targetViewport && targetViewport.camera4DAdapter ? targetViewport.camera4DAdapter.activePlane === 0 : false
            onClicked: {
                if (targetViewport && targetViewport.camera4DAdapter) {
                    targetViewport.camera4DAdapter.activePlane = 0
                    toolbarLayout.planeAxisChanged(0)
                }
            }
            ToolTip.text: "Rotate in XY plane (azimuth/elevation)"
        }
        ToolButton {
            text: "XZ"
            checkable: true
            checked: targetViewport && targetViewport.camera4DAdapter ? targetViewport.camera4DAdapter.activePlane === 1 : false
            onClicked: {
                if (targetViewport && targetViewport.camera4DAdapter) {
                    targetViewport.camera4DAdapter.activePlane = 1
                    toolbarLayout.planeAxisChanged(1)
                }
            }
            ToolTip.text: "Rotate in XZ plane"
        }
        ToolButton {
            text: "YZ"
            checkable: true
            checked: targetViewport && targetViewport.camera4DAdapter ? targetViewport.camera4DAdapter.activePlane === 2 : false
            onClicked: {
                if (targetViewport && targetViewport.camera4DAdapter) {
                    targetViewport.camera4DAdapter.activePlane = 2
                    toolbarLayout.planeAxisChanged(2)
                }
            }
            ToolTip.text: "Rotate in YZ plane"
        }
        ToolButton {
            text: "TX"
            checkable: true
            checked: targetViewport && targetViewport.camera4DAdapter ? targetViewport.camera4DAdapter.activePlane === 3 : false
            onClicked: {
                if (targetViewport && targetViewport.camera4DAdapter) {
                    targetViewport.camera4DAdapter.activePlane = 3
                    toolbarLayout.planeAxisChanged(3)
                }
            }
            ToolTip.text: "Rotate in time-X plane (boost)"
        }
        ToolButton {
            text: "TY"
            checkable: true
            checked: targetViewport && targetViewport.camera4DAdapter ? targetViewport.camera4DAdapter.activePlane === 4 : false
            onClicked: {
                if (targetViewport && targetViewport.camera4DAdapter) {
                    targetViewport.camera4DAdapter.activePlane = 4
                    toolbarLayout.planeAxisChanged(4)
                }
            }
            ToolTip.text: "Rotate in time-Y plane (boost)"
        }
        ToolButton {
            text: "TZ"
            checkable: true
            checked: targetViewport && targetViewport.camera4DAdapter ? targetViewport.camera4DAdapter.activePlane === 5 : false
            onClicked: {
                if (targetViewport && targetViewport.camera4DAdapter) {
                    targetViewport.camera4DAdapter.activePlane = 5
                    toolbarLayout.planeAxisChanged(5)
                }
            }
            ToolTip.text: "Rotate in time-Z plane (boost)"
        }

        ToolSeparator {}

        ToolButton {
            text: simulationPaused ? "▶ Play" : "⏸ Pause"
            checkable: true
            checked: !simulationPaused
            onClicked: toolbarLayout.pauseToggled(!checked)
            ToolTip.text: "Play/Pause simulation"
        }

        Slider {
            id: timeScaleSlider
            from: 0; to: 6; stepSize: 0.1; value: 1
            onValueChanged: {
                timeScaleChanged(value)
            }
            ToolTip.text: "Time scale: " + timeScaleSliderValue.toFixed(1) + "x"
            Layout.preferredWidth: 100
        }

        Label {
            text: timeScaleSliderValue.toFixed(1) + "x"
            font.pixelSize: 11
            color: "#aaa"
        }
    }

    // Row 2: physics constants, zoom, injection, discovery
    RowLayout {
        Layout.fillWidth: true
        spacing: 4

        ComboBox {
            id: curvatureModeCombo
            model: ["Grid Deformation", "Riemann Color", "Curvature Scalar", "Time Dilation", "Geodesic Flow"]
            currentIndex: 0
            onCurrentIndexChanged: toolbarLayout.curvatureModeChanged(currentIndex)
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

        Label {
            text: "Plane Res:"
            font.pixelSize: 11
            color: "#aaa"
            visible: targetViewport && targetViewport.planeMode
        }
        Slider {
            id: planeResSlider
            from: 20; to: 200; stepSize: 10
            value: 100
            Layout.preferredWidth: 100
            visible: targetViewport && targetViewport.planeMode
            onValueChanged: {
                if (targetViewport) targetViewport.planeResolution = value
                toolbarLayout.planeResolutionChanged(value)
            }
            ToolTip.text: "Plane grid resolution (higher = smoother)"
        }

        ToolSeparator {}

        Label {
            text: "BH Mass:"
            font.pixelSize: 11
            color: "#aaa"
        }
        Slider {
            id: bhMassSlider
            from: 1.0; to: 1000.0; stepSize: 1.0
            value: 10.0
            Layout.preferredWidth: 120
            onValueChanged: {
                if (targetViewport) targetViewport.blackHoleMass = value
                toolbarLayout.blackHoleMassChanged(value)
            }
            ToolTip.text: "Black hole mass in solar masses"
        }
        Label {
            text: bhMassSlider.value.toFixed(0) + " M☉"
            font.pixelSize: 11
            color: "#aaa"
            Layout.preferredWidth: 55
        }

                ToolSeparator {}

                Label {
                    text: "Zoom:"
                    font.pixelSize: 11
                    color: "#aaa"
                }
                Slider {
                    id: zoomSlider
                    from: Math.log(1.0)
                    to: Math.log(5000.0)
                    value: Math.log(targetViewport ? targetViewport.cameraDistance : 150.0)
                    Layout.preferredWidth: 100
                    onValueChanged: {
                        if (targetViewport) {
                            targetViewport.cameraDistance = Math.exp(value)
                            if (targetCamController) {
                                targetCamController.distance = Math.exp(value)
                            }
                        }
                        toolbarLayout.zoomChanged(Math.exp(value))
                    }
                    ToolTip.text: "Camera zoom (logarithmic)"
                }
                Label {
                    text: (targetViewport ? (500.0 / targetViewport.cameraDistance * 100).toFixed(0) : "100") + "%"
                    font.pixelSize: 11
                    color: "#aaa"
                    Layout.preferredWidth: 40
                }

                ComboBox {
                    id: bhTypeCombo
                    model: ["Schwarzschild", "Kerr"]
                    currentIndex: 0
                    Layout.preferredWidth: 110
                    onCurrentIndexChanged: {
                        if (targetViewport) targetViewport.setBlackHoleType(model[currentIndex])
                        toolbarLayout.blackHoleTypeChanged(model[currentIndex])
                    }
                    ToolTip.text: "Black hole metric type"
                }
    }

    // Row 3: GW, injection, discovery, tours, VR, telemetry
    RowLayout {
        Layout.fillWidth: true
        spacing: 4

        Label {
            text: "GW: " + (targetViewport ? targetViewport.gravitationalWaveStrain.toExponential(1) : "—")
            font.pixelSize: 10
            color: "#f0f"
        }
        Label {
            text: (targetViewport ? (targetViewport.gravitationalWaveFreq * 1000).toFixed(2) + " mHz" : "—")
            font.pixelSize: 10
            color: "#f0f"
            Layout.preferredWidth: 60
        }

        ToolButton {
            text: "🔊 GW"
            checkable: true
            checked: false
            onToggled: {
                if (targetViewport) targetViewport.audioEnabled = checked
                toolbarLayout.gravitationalWaveAudioToggled(checked)
            }
            ToolTip.text: "Toggle gravitational wave audio"
        }

        ToolSeparator {}

        ToolButton {
            text: "☄️ Inject"
            onClicked: {
                toolbarLayout.injectAsteroid()
            }
            ToolTip.text: "Inject random asteroid"
        }
        ToolButton {
            text: "Clear"
            onClicked: {
                toolbarLayout.clearAsteroids()
            }
            ToolTip.text: "Clear all asteroids"
        }
        Label {
            text: "☄️" + (targetViewport ? targetViewport.asteroidCount : "0")
            font.pixelSize: 10
            color: "#fa0"
            Layout.preferredWidth: 25
        }

        ToolSeparator {}

        ToolButton {
            text: qsTr("Discover")
            checkable: true
            checked: discoveryPanel.visible
            onClicked: {
                toolbarLayout.discoveryToggled(!checked)
            }
            ToolTip.text: qsTr("Toggle Discovery Console")
        }

        ToolButton {
            text: qsTr("Tour")
            checkable: true
            checked: tourController && tourController.state !== TourController.Stopped
            enabled: tourManager && tourManager.tourCount() > 0
            onClicked: {
                toolbarLayout.tourToggled(!checked)
            }
            ToolTip.text: qsTr("Start / stop educational tour")
        }

        ToolButton {
            text: "VR"
            checkable: true
            enabled: true
            onClicked: {
                toolbarLayout.vrToggled(!checked)
            }
            ToolTip.text: "VR mode (requires OpenXR runtime)"
        }

        Item { Layout.fillWidth: true }

        Label {
            text: "KE: " + (targetViewport ? (targetViewport.liveKineticEnergy / 1e33).toFixed(2) + "e33 J" : "—")
            font.pixelSize: 10
            color: "#6af"
        }
        Label {
            text: "PE: " + (targetViewport ? (targetViewport.livePotentialEnergy / 1e33).toFixed(2) + "e33 J" : "—")
            font.pixelSize: 10
            color: "#f6a"
        }
        Label {
            text: "Earth v: " + (targetViewport ? (targetViewport.liveEarthSpeed / 1000).toFixed(1) + " km/s" : "—")
            font.pixelSize: 10
            color: "#6f6"
        }

        Slider {
            id: gSlider
            from: -13; to: -8; stepSize: 0.1
            value: -11.18
            Layout.preferredWidth: 100
            onValueChanged: {
                if (targetViewport) targetViewport.physicsGLog = value
                toolbarLayout.physicsGLogChanged(value)
            }
            ToolTip.text: "Gravitational constant (log10)"
        }
        Label {
            text: "10^" + gSlider.value.toFixed(1)
            font.pixelSize: 10
            color: "#aaa"
            Layout.preferredWidth: 45
        }

        Label {
            text: "FPS: " + (targetViewport && targetViewport.frameRate > 0 ? targetViewport.frameRate.toFixed(1) : "—")
            color: targetViewport && targetViewport.frameRate >= 45 ? "#4f4" : "#f44"
            font.pixelSize: 14
            font.bold: true
            Layout.rightMargin: 8
        }
    }
}
