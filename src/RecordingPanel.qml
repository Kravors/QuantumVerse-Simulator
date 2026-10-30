import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: root
    color: "#0d1117"
    border.color: "#1f2937"
    radius: 6

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        RowLayout {
            Text {
                text: "\u{1F4BE} Recording"
                color: "#00ffcc"; font.bold: true; font.pixelSize: 14
                Layout.alignment: Qt.AlignLeft
            }
            Item { Layout.fillWidth: true }
            Switch {
                checked: viewportItem && viewportItem.recording
                onToggled: {
                    if (checked) viewportItem.startRecording(6000, 1)
                    else viewportItem.stopRecording()
                }
            }
        }

        // Status
        RowLayout {
            spacing: 6
            Label {
                text: viewportItem.recording ? "RECORDING" : (viewportItem.playing ? "PLAYING" : "IDLE")
                color: viewportItem.recording ? "#ff4444" : (viewportItem.playing ? "#44ff44" : "#8b949e")
                font.bold: true; font.pixelSize: 10
            }
            Label {
                text: "frames: " + viewportItem.totalFrames
                color: "#8b949e"; font.pixelSize: 10
            }
        }

        // Controls
        RowLayout {
            spacing: 6
            Button {
                text: viewportItem.recording ? "Stop" : "Record"
                enabled: viewportItem !== null
                implicitWidth: 70; implicitHeight: 28
                font.pixelSize: 10
                onClicked: {
                    if (viewportItem.recording) viewportItem.stopRecording()
                    else viewportItem.startRecording(6000, 1)
                }
            }
            Button {
                text: "Replay"
                enabled: viewportItem.playing === false && viewportItem.totalFrames > 0
                implicitWidth: 60; implicitHeight: 28
                font.pixelSize: 10
                onClicked: viewportItem.playRecording()
            }
            Button {
                text: "Pause"
                enabled: viewportItem.playing
                implicitWidth: 55; implicitHeight: 28
                font.pixelSize: 10
                onClicked: viewportItem.pausePlayback()
            }
            Button {
                text: "Stop"
                enabled: viewportItem.playing
                implicitWidth: 55; implicitHeight: 28
                font.pixelSize: 10
                onClicked: viewportItem.stopPlayback()
            }
        }

        // Session info
        Pane {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            background: Rectangle { color: "#0a0a0f"; radius: 4 }
            ColumnLayout {
                anchors.fill: parent;                 anchors.margins: 6; spacing: 4
                Label { text: "Session"; font.bold: true; font.pixelSize: 10; color: "#8b949e" }
                Label {
                    text: "Recording to data/sessions/<uuid>/frames.bin"
                    color: "#8b949e"; font.pixelSize: 9; wrapMode: Text.WordWrap
                }
                Label {
                    text: "Schema v1: time, bodies, camera, BH mass"
                    color: "#8b949e"; font.pixelSize: 9
                }
            }
        }
    }
}
