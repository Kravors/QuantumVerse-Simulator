import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

/**
 * @file SandboxBodyDelegate.qml
 * @brief One row in the SandboxPanel body ListView.
 *
 * Displays name/type/mass and a delete button. Clicking the row selects it
 * (sets ListView.currentIndex) so the edit form below can modify it.
 */
ItemDelegate {
    width: ListView.view.width
    height: 44

    property string bodyId: model.id
    property string bodyName: model.name
    property string bodyType: model.type

    RowLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 8

        // Select on click
        MouseArea {
            anchors.fill: parent
            onClicked: ListView.view.currentIndex = index
        }

        Rectangle {
            width: 10; height: 10; radius: 5
            color: bodyType === "star" ? "#ffcc00" : (bodyType === "black_hole" ? "#6666ff" : "#aaaaaa")
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Label {
                text: bodyName
                font.bold: ListView.isCurrentItem
                font.pixelSize: 12
                color: ListView.isCurrentItem ? "#ffffff" : "#cccccc"
                elide: Text.ElideRight
            }
            Label {
                text: bodyType + "  ·  " + Number(model.mass_kg).toFixed(2) + " kg"
                font.pixelSize: 10
                color: "#888888"
            }
        }

        Button {
            text: "🗑"
            font.pixelSize: 14
            width: 36; height: 36
            flat: true
            onClicked: {
                sandboxManager.removeBody(bodyId)
            }
        }
    }
}
