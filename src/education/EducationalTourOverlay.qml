/**
 * @file EducationalTourOverlay.qml
 * @brief Popup overlay that drives an EducationalTour through the QML
 *        TourController context property.
 *
 * The overlay is a TourController-driven popup.  It reads
 *   tourController.state        -> TourState enum (Stopped / Running / Paused)
 *   tourController.currentTour  -> QVariant holding an EducationalTour (Q_GADGET)
 *   tourController.currentStep  -> int, zero-based index into currentTour.steps
 *
 * Visibility is bound to the controller state: shown whenever the tour is
 * Running or Paused, hidden when Stopped.
 */

import QtQuick
import QtQuick.Controls.Basic 6.0
import QtQuick.Layouts 1.15
import QuantumVerse 1.0

Popup {
    id: tourOverlay
    anchors.centerIn: parent
    width: Math.min(640, (parent ? parent.width : 1024) - 48)
    height: Math.min(440, (parent ? parent.height : 768) - 48)

    // ---- Helper functions (must precede visual tree in Qt 6.11 QML) ----------
    function isVisible() {
        if (!tourController) return false
        return tourController.state === TourController.Running ||
               tourController.state === TourController.Paused
    }

    function tourTitle() {
        if (!tourController || !tourController.currentTour)
            return "Educational Tour"
        var t = tourController.currentTour
        return t.title || "Educational Tour"
    }

    function stepCounter() {
        if (!tourController || !tourController.currentTour)
            return ""
        var steps = tourController.currentTour.steps
        var idx = tourController.currentStep
        var count = steps ? steps.length : 0
        if (count === 0)
            return "Step " + (idx + 1) + " / 0"
        return "Step " + (idx + 1) + " / " + count
    }

    function stepBody() {
        if (!tourController || !tourController.currentTour)
            return ""
        var steps = tourController.currentTour.steps
        var idx = tourController.currentStep
        if (!steps || idx >= steps.length)
            return ""
        var step = steps[idx]
        if (!step) return ""
        return step.body || ""
    }

    function canGoPrevious() {
        if (!tourController) return false
        return tourController.currentStep > 0
    }

    function isRunningOrPaused() {
        if (!tourController) return false
        return tourController.state === TourController.Running ||
               tourController.state === TourController.Paused
    }

    function playPauseText() {
        if (!tourController) return "Pause"
        if (tourController.state === TourController.Paused)
            return "Resume"
        return "Pause"
    }

    function togglePauseResume() {
        if (!tourController) return
        if (tourController.state === TourController.Paused)
            tourController.resume()
        else
            tourController.pause()
    }

    visible: tourOverlay.isVisible()
    modal: false
    focus: true
    closePolicy: Popup.NoAutoClose

    background: Rectangle {
        color: "#1a1a2e"
        border.color: "#4a4a7a"
        border.width: 1
        radius: 6
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // ---- Header: title + step counter -------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 20

            RowLayout {
                anchors.fill: parent

                Label {
                    text: tourOverlay.tourTitle()
                    font.pixelSize: 18
                    font.bold: true
                    color: "#e0e0f0"
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                Label {
                    text: tourOverlay.stepCounter()
                    color: "#a0a0c0"
                    font.pixelSize: 12
                }
            }
        }

        // ---- Step body text ---------------------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ScrollView {
                anchors.fill: parent

                Label {
                    text: tourOverlay.stepBody()
                    wrapMode: Text.WordWrap
                    color: "#d0d0e0"
                    font.pixelSize: 14
                    width: parent.width
                }
            }
        }

        // ---- Footer: controls -------------------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 40

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 8

                Button {
                    text: "Previous"
                    enabled: tourOverlay.canGoPrevious()
                    onClicked: tourController.prev()
                }

                Button {
                    text: tourOverlay.playPauseText()
                    enabled: tourOverlay.isRunningOrPaused()
                    onClicked: tourOverlay.togglePauseResume()
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "Next"
                    enabled: tourOverlay.isRunningOrPaused()
                    onClicked: tourController.next()
                }

                Button {
                    text: "Close"
                    onClicked: tourController.stop()
                }
            }
        }
    }
}
