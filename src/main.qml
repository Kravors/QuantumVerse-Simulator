import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QuantumVerse 1.0

Window {
    id: mainWindow
    width: 1280
    height: 720
    visible: true
    title: "QuantumVerse Simulator"

    ColumnLayout {
        anchors.fill: parent

        QmlCamController {
            id: cameraController
        }

        SceneGraphModel {
            id: sceneGraph
        }

        Text {
            text: "QuantumVerse 4D Spacetime Simulator"
            font.pixelSize: 24
            horizontalAlignment: Text.AlignHCenter
        }
    }
}