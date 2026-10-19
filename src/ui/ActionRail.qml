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
        text: "☄️"
        ToolTip.text: "Inject random asteroid"
        onClicked: actionRail.injectAsteroid()
    }

    ToolButton {
        text: "🗑️"
        ToolTip.text: "Clear all asteroids"
        onClicked: actionRail.clearAsteroids()
    }

    ToolButton {
        text: "🔍"
        checkable: true
        checked: discoveryPanel.visible
        ToolTip.text: "Toggle Discovery Console"
        onClicked: actionRail.discoveryToggled(!checked)
    }

    ToolButton {
        text: "🎬"
        checkable: true
        enabled: tourAvailable
        checked: tourAvailable && tourController.state !== TourController.Stopped
        ToolTip.text: "Start / stop educational tour"
        onClicked: if (tourAvailable) actionRail.tourToggled(!checked)
    }

    ToolButton {
        text: "🥽"
        checkable: true
        enabled: vrAvailable
        ToolTip.text: vrAvailable ? "VR mode (requires OpenXR runtime)" : "VR unavailable (no signaling server)"
        onClicked: if (vrAvailable) actionRail.vrToggled(!checked)
    }
}
