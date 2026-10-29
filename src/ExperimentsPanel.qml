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

        Text {
            text: "\u{1F9EA} Experiments"
            color: "#00ffcc"; font.bold: true; font.pixelSize: 14
            Layout.fillWidth: true
        }

        RowLayout {
            spacing: 6
            ComboBox {
                id: catCombo
                model: ["All", "Sweep", "GR Validation", "Cosmology", "Discovery", "Quantum Gravity"]
                font.pixelSize: 10
            }
            Item { Layout.fillWidth: true }
            BusyIndicator {
                running: experimentRunner.running
                visible: experimentRunner.running
                Layout.preferredWidth: 18; Layout.preferredHeight: 18
            }
        }

        // Experiment list
        ListView {
            id: expList
            Layout.fillWidth: true
            Layout.preferredHeight: 150
            clip: true
            model: experimentRunner
            delegate: Rectangle {
                required property string id
                required property string name
                required property string category
                required property string description
                height: (catCombo.currentIndex === 0 || category === catCombo.currentText) ? 52 : 0
                color: experimentRunner.currentId === id ? "#1f2937" : "transparent"
                border.color: "#2a2a4a"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 4; spacing: 1
                    Text { text: name; color: "white"; font.pixelSize: 12; elide: Text.ElideRight }
                    Text { text: category + "\u2014" + description; color: "#8b949e"; font.pixelSize: 9; elide: Text.ElideRight }
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: paramForm.expId = id
                }
            }
        }

        // Parameter form (built dynamically)
        ExperimentParamForm {
            id: paramForm
            Layout.fillWidth: true
        }

        // Run controls
        RowLayout {
            spacing: 6
            Button {
                text: experimentRunner.running ? "Cancel" : "\u25B6 Run"
                Layout.fillWidth: true
                onClicked: {
                    if (experimentRunner.running) experimentRunner.cancel();
                    else experimentRunner.start(paramForm.expId, paramForm.values);
                }
            }
            Button {
                text: "Export"
                enabled: !experimentRunner.running && experimentRunner.currentId !== ""
                Layout.preferredWidth: 70
                onClicked: experimentRunner.exportLast("csv")
            }
            Button {
                text: "Export Validation Report"
                enabled: !experimentRunner.running && experimentRunner.currentId.startsWith("cosmology_")
                Layout.preferredWidth: 150
                onClicked: experimentRunner.exportValidationReport("csv")
            }
            Item { Layout.fillWidth: true }
            Text {
                text: Math.round(experimentRunner.progress * 100) + "%"
                color: "#8b949e"; font.pixelSize: 11
            }
        }

        ProgressBar {
            Layout.fillWidth: true
            from: 0; to: 1; value: experimentRunner.progress
        }

        // Scalars table
        GridLayout {
            columns: 2
            Layout.fillWidth: true
            visible: Object.keys(experimentRunner.currentScalars).length > 0
            spacing: 4
            Repeater {
                model: Object.keys(experimentRunner.currentScalars)
                delegate: RowLayout {
                    spacing: 6
                    Label { text: modelData + ":"; color: "#8b949e"; font.pixelSize: 9 }
                    Label { text: experimentRunner.currentScalars[modelData]; color: "white"; font.pixelSize: 9; font.family: "monospace"; Layout.fillWidth: true }
                }
            }
        }

        // Tension vs code correctness notice: when a check fails (pass=false)
        // but rel_error_pct is small, the code matches the reference — the
        // parameters simply disagree with observations (e.g. H0 tension).
        Text {
            visible: experimentRunner.currentId.startsWith("cosmology_") && tensionDominant()
            color: "#ffaa00"; font.pixelSize: 9; font.italic: true; wrapMode: Text.WordWrap
            Layout.fillWidth: true
            text: "\u26A0 Some checks fail due to parameter choice, not code: "
                + "H0 tension between Planck (67.66) and SH0ES (73.04) "
                + "is expected. Check 'rel_error_pct' for code correctness."
        }

        function tensionDominant() {
            const scalars = experimentRunner.currentScalars;
            const keys = Object.keys(scalars);
            for (let i = 0; i < keys.length; i++) {
                const k = keys[i];
                if (k.endsWith("_pass") && scalars[k] === false) {
                    const base = k.slice(0, -5); // strip "_pass"
                    const relErrKey = base + "_rel_err";
                    if (scalars.hasOwnProperty(relErrKey)) {
                        const relErr = Math.abs(parseFloat(scalars[relErrKey]));
                        if (relErr < 1.0) return true; // code matches reference
                    }
                }
            }
            return false;
        }

        // Curve canvas
        Canvas {
            id: expCanvas
            Layout.fillWidth: true
            Layout.fillHeight: true
            onPaint: {
                const ctx = getContext("2d");
                if (!ctx) return;
                ctx.clearRect(0, 0, width, height);
                const pts = experimentRunner.currentCurve;
                if (pts.length < 2) {
                    ctx.fillStyle = "#8b949e";
                    ctx.font = "12px monospace";
                    ctx.fillText("(no curve — experiment returned no data)", 10, height / 2);
                    return;
                }
                let minX = Infinity, maxX = -Infinity, minY = Infinity, maxY = -Infinity;
                for (let i = 0; i < pts.length; i++) {
                    minX = Math.min(minX, pts[i].x);
                    maxX = Math.max(maxX, pts[i].x);
                    minY = Math.min(minY, pts[i].y);
                    maxY = Math.max(maxY, pts[i].y);
                }
                const pad = 30;
                const pw = width - 2 * pad;
                const ph = height - 2 * pad;
                const dx = (maxX - minX) || 1;
                const dy = (maxY - minY) || 1;
                const sx = pw / dx, sy = ph / dy;
                const scale = Math.min(sx, sy);
                const offX = pad + (pw - dx * scale) / 2 - minX * scale;
                const offY = pad + (ph - dy * scale) / 2 - minY * scale;

                ctx.strokeStyle = "#00ffcc";
                ctx.lineWidth = 2;
                ctx.beginPath();
                for (let i = 0; i < pts.length; i++) {
                    const px = offX + pts[i].x * scale;
                    const py = offY + (maxY - pts[i].y) * scale; // flip Y so up is up
                    if (i === 0) ctx.moveTo(px, py);
                    else ctx.lineTo(px, py);
                }
                ctx.stroke();
            }
            Connections {
                target: experimentRunner
                function onCurrentCurveChanged() { expCanvas.requestPaint(); }
            }
        }

        // Live log
        Pane {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            background: Rectangle { color: "#0a0a0f"; radius: 4 }
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 4; spacing: 2
                Label { text: "Run log"; font.bold: true; font.pixelSize: 10; color: "#8b949e" }
                TextArea {
                    id: expLog
                    readOnly: true
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    text: experimentRunner.currentLog
                    color: "#00ff66"; font.family: "Consolas"; font.pixelSize: 9
                    wrapMode: Text.WordWrap
                }
            }
        }

        // History section
        Pane {
            Layout.fillWidth: true
            Layout.preferredHeight: 130
            background: Rectangle { color: "#0a0a0f"; radius: 4 }
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 4; spacing: 2
                Label { text: "History"; font.bold: true; font.pixelSize: 10; color: "#8b949e" }
                ListView {
                    id: historyList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: experimentRunner.history()
                    delegate: Rectangle {
                        required property var modelData
                        height: 26
                        color: mouse.containsMouse ? "#1f2937" : "transparent"
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 4
                            Rectangle {
                                width: 6; height: 6; radius: 3
                                color: modelData.ok ? "#00ff66" : "#ff5555"
                                Layout.alignment: Qt.AlignVCenter
                            }
                            Text {
                                text: modelData.name + "  •  " + modelData.outputDir
                                color: "white"
                                font.pixelSize: 9
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                            Text {
                                text: modelData.ok ? "✓" : "✗"
                                color: modelData.ok ? "#00ff66" : "#ff5555"
                                font.pixelSize: 9
                            }
                        }
                        MouseArea {
                            id: mouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: experimentRunner.showHistoryEntry(modelData.id)
                        }
                    }
                }
            }
        }
    }
}
