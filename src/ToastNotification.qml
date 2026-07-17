/**
 * @file ToastNotification.qml
 * @brief Slide-in toast overlay for live alert notifications
 *
 * Displays a small overlay in the top-right corner when a new alert
 * is processed. Shows the alert type and source, then fades out
 * after 5 seconds. Connected to DiscoveryPanelManager::liveAlertProcessed.
 */

import QtQuick
import QtQuick.Controls.Basic 6.0
import QtQuick.Layouts

Rectangle {
    id: toast
    width: 320
    height: 48
    radius: 6
    color: "#1e1e2e"
    border.color: "#4f4"
    border.width: 1
    opacity: 0
    anchors.top: parent.top
    anchors.right: parent.right
    anchors.margins: 12
    z: 9999

    property string alertType: ""
    property string origin: ""

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        Label {
            text: "🔔"
            font.pixelSize: 18
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 1

            Label {
                text: "Live Alert Processed"
                font.bold: true
                font.pixelSize: 11
                color: "#4f4"
            }

            Label {
                text: toast.alertType + " — " + toast.origin
                font.pixelSize: 10
                color: "#ccc"
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }

    states: State {
        name: "visible"
        PropertyChanges { target: toast; opacity: 1; x: 0 }
    }

    transitions: Transition {
        NumberAnimation { properties: "opacity, x"; duration: 300; easing.type: Easing.OutCubic }
    }

    Timer {
        interval: 5000
        running: toast.alertType !== ""
        onTriggered: {
            toast.alertType = ""
            toast.origin = ""
            toast.opacity = 0
        }
    }
}
