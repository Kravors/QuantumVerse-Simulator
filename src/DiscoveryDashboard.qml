import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ColumnLayout {
    id: root
    property var manager: discoveryPanelManager

    signal parametersChanged()

    // Header: instrument name + status
    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Label {
            text: (manager && manager.activeInstrumentInfo && manager.activeInstrumentInfo.name)
                  ? manager.activeInstrumentInfo.name : "No Instrument"
            font.pixelSize: 16
            font.bold: true
            color: "#ffffff"
            Layout.fillWidth: true
        }

        Rectangle {
            width: 12
            height: 12
            radius: 6
            color: (manager && manager.instrumentCount > 0) ? "#4caf50" : "#888888"
        }

        Label {
            text: (manager && manager.instrumentCount > 0) ? "Active" : "Inactive"
            font.pixelSize: 12
            color: "#a0a0a0"
        }
    }

    Rectangle {
        Layout.fillWidth: true
        height: 1
        color: "#3a3a3a"
    }

    // Parameter controls
    Repeater {
        model: manager ? manager.instrumentParameters : []
        delegate: ParameterSlider {
            Layout.fillWidth: true
            paramName: modelData.name
            paramValue: modelData.value
            paramMin: modelData.min
            paramMax: modelData.max

            onValueChanged: function(newValue) {
                manager.setInstrumentParameter(paramName, newValue)
                root.parametersChanged()
            }
        }
    }

    Label {
        visible: manager && manager.instrumentParameters.length === 0
        text: "No tunable parameters for this instrument"
        color: "#666666"
        font.pixelSize: 12
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        padding: 20
    }

    Rectangle {
        Layout.fillWidth: true
        height: 1
        color: "#3a3a3a"
    }

    // Footer: reset-all + active instrument summary
    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Label {
            text: "Active: " + ((manager && manager.activeInstrumentInfo && manager.activeInstrumentInfo.name)
                  ? manager.activeInstrumentInfo.name : "None")
            font.pixelSize: 10
            color: "#666666"
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        Button {
            text: "Reset All"
            font.pixelSize: 10
            flat: true
            visible: manager && manager.instrumentParameters.length > 0
            onClicked: {
                for (var i = 0; i < manager.instrumentParameters.length; i++) {
                    var p = manager.instrumentParameters[i]
                    var resetVal = (p.min + p.max) / 2.0
                    manager.setInstrumentParameter(p.name, resetVal)
                }
                root.parametersChanged()
            }
            background: Rectangle {
                color: "transparent"
                radius: 4
                border.color: "#4a9eff"
                border.width: 1
            }
            contentItem: Text {
                text: parent.text
                color: "#4a9eff"
                font.pixelSize: 10
            }
        }
    }
}
