import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

RowLayout {
    id: root
    property string paramName: ""
    property real paramValue: 0.0
    property real paramMin: 0.0
    property real paramMax: 1.0
    property real stepSize: (paramMax - paramMin) / 100.0

    signal valueChanged(real newValue)

    Label {
        text: paramName
        Layout.preferredWidth: 150
        elide: Text.ElideRight
        font.pixelSize: 12
        color: "#e0e0e0"
    }

    Slider {
        id: slider
        Layout.fillWidth: true
        from: paramMin
        to: paramMax
        value: paramValue
        stepSize: root.stepSize

        onValueChanged: {
            root.paramValue = value
            root.valueChanged(value)
        }

        background: Rectangle {
            color: "#2a2a2a"
            radius: 4
            height: 6
            Rectangle {
                width: slider.position * parent.width
                height: parent.height
                color: "#4a9eff"
                radius: 4
            }
        }

        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            width: 16
            height: 16
            radius: 8
            color: slider.pressed ? "#6ab0ff" : "#4a9eff"
            border.color: "#ffffff"
            border.width: 1
        }
    }

    Label {
        text: root.paramValue.toFixed(3)
        Layout.preferredWidth: 64
        horizontalAlignment: Text.AlignRight
        font.pixelSize: 12
        font.family: "monospace"
        color: "#a0c0e0"
    }

    Button {
        text: "↺"
        Layout.preferredWidth: 30
        Layout.preferredHeight: 30
        font.pixelSize: 14
        background: Rectangle {
            color: "transparent"
            radius: 4
            border.color: "#4a9eff"
            border.width: 1
        }
        onClicked: {
            var resetVal = (paramMin + paramMax) / 2.0
            slider.value = resetVal
        }
        ToolTip.text: "Reset to default"
        ToolTip.visible: hovered
    }
}
