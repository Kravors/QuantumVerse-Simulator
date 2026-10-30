import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

// Dynamically builds a parameter form from experimentRunner.parametersFor(id).
// Exposes expId and a values() map consumed by ExperimentsPanel.
Item {
    id: root
    property string expId: ""
    // read-only snapshot of current control values, keyed by parameter name
    property var values: {}

    onExpIdChanged: {
        root.values = {}
    }

    ColumnLayout {
        id: form
        anchors.fill: parent
        spacing: 6
        Repeater {
            model: experimentRunner.parametersFor(root.expId)
            delegate: Rectangle {
                color: "transparent"
                height: 52
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 2
                    spacing: 2

                    Label {
                        text: modelData.label
                        font.pixelSize: 10
                        color: "#ccc"
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Label {
                        text: modelData.help
                        font.pixelSize: 8
                        color: "#666"
                        visible: modelData.help !== ""
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                    }

                    RowLayout {
                        spacing: 4

                        SpinBox {
                            id: numBox
                            visible: modelData.type === "double" || modelData.type === "int"
                            from: modelData.min
                            to: modelData.max
                            stepSize: modelData.type === "int" ? 1 : 0.1
                            value: modelData.defaultValue
                            Layout.fillWidth: true
                            decimals: modelData.type === "int" ? 0 : 4
                            onValueChanged: root.values[modelData.name] = value
                            Component.onCompleted: root.values[modelData.name] = value
                        }

                        ComboBox {
                            id: enumBox
                            visible: modelData.type === "enum"
                            model: modelData.options
                            currentText: modelData.defaultValue
                            Layout.fillWidth: true
                            font.pixelSize: 10
                            onActivated: root.values[modelData.name] = currentText
                            Component.onCompleted: root.values[modelData.name] = currentText
                        }

                        Switch {
                            id: boolBox
                            visible: modelData.type === "bool"
                            checked: modelData.defaultValue === true
                            onCheckedChanged: root.values[modelData.name] = checked
                            Component.onCompleted: root.values[modelData.name] = checked
                        }

                        TextField {
                            id: strBox
                            visible: modelData.type === "string"
                            text: modelData.defaultValue
                            Layout.fillWidth: true
                            font.pixelSize: 10
                            onTextEdited: root.values[modelData.name] = text
                            Component.onCompleted: root.values[modelData.name] = text
                        }

                        Label {
                            visible: modelData.type === "double" || modelData.type === "int"
                            text: modelData.unit
                            color: "#8b949e"
                            font.pixelSize: 9
                        }
                    }
                }
            }
        }
    }
}
