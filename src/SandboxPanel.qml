import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0

/**
 * @file SandboxPanel.qml
 * @brief Right-rail panel for the V1 Sandbox scene.
 *
 * Provides: New Sandbox, Save, Load; a ListView of bodies (delegate:
 * SandboxBodyDelegate); and an edit form for the selected body (type,
 * name, mass, radius, position, delete).
 *
 * Bound to the `sandboxManager` context property exposed from main_qml.cpp.
 */
Item {
    id: sandboxPanel
    width: 360
    property int selectedIndex: bodyList.currentIndex

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 10

        // ===== Header =====
        RowLayout {
            Layout.fillWidth: true
            Label {
                text: "Sandbox"
                font.bold: true
                font.pixelSize: 16
                Layout.fillWidth: true
            }
            Button {
                text: "New Sandbox"
                width: 110
                onClicked: sandboxManager.clear()
            }
        }

        // ===== Save / Load =====
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            TextInput {
                id: pathInput
                Layout.fillWidth: true
                placeholderText: "Save/load path"
                text: sandboxManager.defaultSaveDir() + "/sandbox.json"
                font.pixelSize: 11
            }
            Button {
                text: "Save"
                width: 64
                onClicked: {
                    var ok = sandboxManager.saveTo(pathInput.text);
                    if (!ok) console.log("Sandbox save failed");
                }
            }
            Button {
                text: "Load"
                width: 64
                onClicked: {
                    var ok = sandboxManager.loadFrom(pathInput.text);
                    if (!ok) console.log("Sandbox load failed");
                }
            }
        }

        // ===== Body list =====
        Label {
            text: "Bodies"
            font.bold: true
            font.pixelSize: 12
            color: "#888888"
        }
        ListView {
            id: bodyList
            Layout.fillWidth: true
            Layout.preferredHeight: 220
            model: sandboxManager.bodies
            delegate: SandboxBodyDelegate {}
            currentIndex: selectedIndex >= 0 ? selectedIndex : 0
            highlight: Rectangle { color: "#3a3a5c"; radius: 4 }
            highlightFollowsCurrentItem: true
        }

        // ===== Edit form (selected body) =====
        Pane {
            Layout.fillWidth: true
            visible: bodyList.count > 0 && bodyList.currentIndex >= 0

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                Label {
                    text: "Edit body"
                    font.bold: true
                    font.pixelSize: 12
                    color: "#888888"
                }

                // Selected body data
                property var currentBody: bodyList.currentIndex >= 0
                    ? sandboxManager.bodies[bodyList.currentIndex]
                    : null

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label { text: "Type"; font.pixelSize: 10; color: "#aaa" }
                    ComboBox {
                        Layout.fillWidth: true
                        model: ["star", "planet", "black_hole"]
                        currentText: currentBody ? currentBody.type : "planet"
                        font.pixelSize: 11
                        onCurrentTextChanged:
                            sandboxManager.updateBody(currentBody.id, { type: currentText })
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label { text: "Name"; font.pixelSize: 10; color: "#aaa" }
                    TextField {
                        Layout.fillWidth: true
                        text: currentBody ? currentBody.name : ""
                        font.pixelSize: 11
                        onEditingFinished:
                            sandboxManager.updateBody(currentBody.id, { name: text })
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label { text: "Mass (kg)"; font.pixelSize: 10; color: "#aaa" }
                    TextField {
                        Layout.fillWidth: true
                        text: currentBody ? Number(currentBody.mass_kg).toFixed(4) : "0"
                        font.pixelSize: 11
                        onEditingFinished:
                            sandboxManager.updateBody(currentBody.id, { mass_kg: Number(text) })
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label { text: "Radius (m)"; font.pixelSize: 10; color: "#aaa" }
                    TextField {
                        Layout.fillWidth: true
                        text: currentBody ? Number(currentBody.radius_m).toFixed(4) : "0"
                        font.pixelSize: 11
                        onEditingFinished:
                            sandboxManager.updateBody(currentBody.id, { radius_m: Number(text) })
                    }
                }

                GridLayout {
                    columns: 3
                    Layout.fillWidth: true
                    spacing: 6
                    RowLayout {
                        Label { text: "X"; font.pixelSize: 10; color: "#aaa" }
                        TextField {
                            Layout.fillWidth: true
                            text: currentBody ? Number(currentBody.x).toFixed(2) : "0"
                            font.pixelSize: 11
                            onEditingFinished:
                                sandboxManager.updateBody(currentBody.id, { x: Number(text) })
                        }
                    }
                    RowLayout {
                        Label { text: "Y"; font.pixelSize: 10; color: "#aaa" }
                        TextField {
                            Layout.fillWidth: true
                            text: currentBody ? Number(currentBody.y).toFixed(2) : "0"
                            font.pixelSize: 11
                            onEditingFinished:
                                sandboxManager.updateBody(currentBody.id, { y: Number(text) })
                        }
                    }
                    RowLayout {
                        Label { text: "Z"; font.pixelSize: 10; color: "#aaa" }
                        TextField {
                            Layout.fillWidth: true
                            text: currentBody ? Number(currentBody.z).toFixed(2) : "0"
                            font.pixelSize: 11
                            onEditingFinished:
                                sandboxManager.updateBody(currentBody.id, { z: Number(text) })
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Button {
                        text: "Delete"
                        Layout.fillWidth: true
                        onClicked: sandboxManager.removeBody(currentBody.id)
                    }
                }
            }
        }
    }
}
