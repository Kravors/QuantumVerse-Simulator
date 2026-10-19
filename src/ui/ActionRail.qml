/**
 * @file ActionRail.qml
 * @brief Left-edge vertical action rail for workflow actions
 *
 * Commit 3 of UI session: Move Inject/Clear/Discover/Tour/VR from the top
 * toolbar row into an icon-only vertical rail on the left edge, below the
 * topbar. Keeps the primary toolbar row clean (9 controls max).
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls 6.0
import QuantumVerse 1.0

ColumnLayout {
    id: actionRail
    spacing: Theme.spaceSm
    Layout.preferredWidth: 56
    Layout.fillHeight: true

    property var targetViewport
    property var tourController
    property var tourManager
    property var signalingClient
    property Item discoveryPanel

    signal injectAsteroid()
    signal clearAsteroids()
    signal discoveryToggled(bool visible)
    signal tourToggled(bool active)
    signal vrToggled(bool active)
    property bool tourAvailable: typeof tourController !== 'undefined' && tourController !== null
        && tourManager !== null && tourManager.tourCount() > 0
    property bool vrAvailable: typeof signalingClient !== 'undefined' && signalingClient !== null

    ToolButton {
        icon.source: "qrc:/icons/syringe.svg"
        ToolTip.text: "Inject random asteroid"
        onClicked: actionRail.injectAsteroid()
        background: Rectangle {
            color: parent.hovered ? Theme.surfaceHover : "transparent"
            radius: Theme.radiusSm
            implicitWidth: Theme.railBtn
            implicitHeight: Theme.railBtn
        }
    }

    ToolButton {
        icon.source: "qrc:/icons/trash-2.svg"
        ToolTip.text: "Clear all asteroids"
        onClicked: actionRail.clearAsteroids()
        background: Rectangle {
            color: parent.hovered ? Theme.surfaceHover : "transparent"
            radius: Theme.radiusSm
            implicitWidth: Theme.railBtn
            implicitHeight: Theme.railBtn
        }
    }

    ToolButton {
        icon.source: "qrc:/icons/search.svg"
        checkable: true
        checked: discoveryPanel.visible
        ToolTip.text: "Toggle Discovery Console"
        onClicked: actionRail.discoveryToggled(!checked)
        background: Rectangle {
            color: parent.checked ? Theme.surfaceActive : (parent.hovered ? Theme.surfaceHover : "transparent")
            radius: Theme.radiusSm
            implicitWidth: Theme.railBtn
            implicitHeight: Theme.railBtn
        }
    }

    ToolButton {
        icon.source: "qrc:/icons/compass.svg"
        checkable: true
        enabled: tourAvailable
        checked: tourAvailable && tourController.state !== TourController.Stopped
        ToolTip.text: "Start / stop educational tour"
        onClicked: if (tourAvailable) actionRail.tourToggled(!checked)
        background: Rectangle {
            color: parent.checked ? Theme.surfaceActive : (parent.hovered ? Theme.surfaceHover : "transparent")
            radius: Theme.radiusSm
            implicitWidth: Theme.railBtn
            implicitHeight: Theme.railBtn
        }
    }

    ToolButton {
        icon.source: "qrc:/icons/glasses.svg"
        checkable: true
        enabled: vrAvailable
        ToolTip.text: vrAvailable ? "VR mode (requires OpenXR runtime)" : "VR unavailable (no signaling server)"
        onClicked: if (vrAvailable) actionRail.vrToggled(!checked)
        background: Rectangle {
            color: parent.checked ? Theme.surfaceActive : (parent.hovered ? Theme.surfaceHover : "transparent")
            radius: Theme.radiusSm
            implicitWidth: Theme.railBtn
            implicitHeight: Theme.railBtn
        }
    }
}
