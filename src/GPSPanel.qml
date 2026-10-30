import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: root
    color: "#0d1117"
    border.color: "#1f2937"
    radius: 6

    property string selectedId: ""

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        RowLayout {
            Text {
                text: "\u{1F68F} Spacetime GPS"
                color: "#00ffcc"; font.bold: true; font.pixelSize: 14
                Layout.alignment: Qt.AlignLeft
            }
            Item { Layout.fillWidth: true }
            Switch {
                checked: spacetimeGPS.trackingEnabled
                onToggled: spacetimeGPS.trackingEnabled = checked
            }
        }

        RowLayout {
            spacing: 6
            TextField {
                id: gpsSearch
                placeholderText: "Filter bodies…"
                Layout.fillWidth: true
                font.pixelSize: 10
            }
            ComboBox {
                id: gpsTypeFilter
                model: spacetimeGPS.types()
                currentIndex: 0
                font.pixelSize: 10
                Layout.preferredWidth: 120
            }
        }

        ListView {
            id: gpsList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: spacetimeGPS
            delegate: Rectangle {
                required property string id
                required property string name
                required property string type
                required property color color
                required property double r
                required property double speed
                required property double spinRate
                required property bool pinned

                visible: (gpsTypeFilter.currentIndex === 0 || type === gpsTypeFilter.currentText)
                       && (gpsSearch.text === "" || name.toLowerCase().includes(gpsSearch.text.toLowerCase()))
                height: visible ? 44 : 0
                color: spacetimeGPS.selectedId === id ? "#1f2937" : "transparent"
                border.color: "#2a2a4a"
                border.width: 1

                RowLayout {
                    anchors.fill: parent; anchors.margins: 6
                    Rectangle {
                        width: 10; height: 10; radius: 5; color: color
                        Layout.alignment: Qt.AlignVCenter
                    }
                    ColumnLayout {
                        spacing: 1
                        Text { text: name; color: "white"; font.pixelSize: 12; elide: Text.ElideRight }
                        Text {
                            text: qsTr("r=%1  v=%2  \u03c9=%3")
                                    .arg(r.toFixed(2)).arg(speed.toFixed(2)).arg(spinRate.toFixed(3))
                            color: "#8b949e"; font.pixelSize: 10
                        }
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        text: "Focus"
                        implicitWidth: 56; implicitHeight: 24
                        font.pixelSize: 9
                        onClicked: spacetimeGPS.focusOn(id)
                    }
                    Button {
                        text: pinned ? "\u{2605}" : "\u{2606}"
                        implicitWidth: 32; implicitHeight: 24
                        font.pixelSize: 10
                        onClicked: spacetimeGPS.pin(id, !pinned)
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: spacetimeGPS.selectedId = id
                }
            }
        }

        // Selected object detail table
        GridLayout {
            columns: 4
            visible: spacetimeGPS.selectedId !== ""
            Layout.fillWidth: true
            spacing: 4

            RowLayout { Label { text: "t:"; color: "#8b949e"; font.pixelSize: 10 } Label { text: spacetimeGPS.details(spacetimeGPS.selectedId).t; color: "white"; font.pixelSize: 10; font.family: "monospace" } }
            RowLayout { Label { text: "x:"; color: "#8b949e"; font.pixelSize: 10 } Label { text: spacetimeGPS.details(spacetimeGPS.selectedId).x; color: "white"; font.pixelSize: 10; font.family: "monospace" } }
            RowLayout { Label { text: "y:"; color: "#8b949e"; font.pixelSize: 10 } Label { text: spacetimeGPS.details(spacetimeGPS.selectedId).y; color: "white"; font.pixelSize: 10; font.family: "monospace" } }
            RowLayout { Label { text: "z:"; color: "#8b949e"; font.pixelSize: 10 } Label { text: spacetimeGPS.details(spacetimeGPS.selectedId).z; color: "white"; font.pixelSize: 10; font.family: "monospace" } }
            RowLayout { Label { text: "r:"; color: "#8b949e"; font.pixelSize: 10 } Label { text: spacetimeGPS.details(spacetimeGPS.selectedId).r; color: "white"; font.pixelSize: 10; font.family: "monospace" } }
            RowLayout { Label { text: "\u03B8:"; color: "#8b949e"; font.pixelSize: 10 } Label { text: spacetimeGPS.details(spacetimeGPS.selectedId).theta; color: "white"; font.pixelSize: 10; font.family: "monospace" } }
            RowLayout { Label { text: "\u03C6:"; color: "#8b949e"; font.pixelSize: 10 } Label { text: spacetimeGPS.details(spacetimeGPS.selectedId).phi; color: "white"; font.pixelSize: 10; font.family: "monospace" } }
            RowLayout { Label { text: "z(red):"; color: "#8b949e"; font.pixelSize: 10 } Label { text: spacetimeGPS.details(spacetimeGPS.selectedId).redshift; color: "white"; font.pixelSize: 10; font.family: "monospace" } }
            RowLayout { Label { text: "\u03C4:"; color: "#8b949e"; font.pixelSize: 10 } Label { text: spacetimeGPS.details(spacetimeGPS.selectedId).properTime; color: "white"; font.pixelSize: 10; font.family: "monospace" } }
        }

        // GPS trail canvas
        Pane {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            visible: spacetimeGPS.selectedId !== ""
            background: Rectangle { color: "#0a0a0f"; radius: 4 }

            ColumnLayout {
                anchors.fill: parent; anchors.margins: 6; spacing: 4
                Label { text: "GPS Trail (X/Y)"; font.bold: true; font.pixelSize: 10; color: "#8b949e" }

                Canvas {
                    id: gpsTrail
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    onPaint: {
                        const ctx = getContext("2d");
                        if (!ctx) return;
                        ctx.clearRect(0, 0, width, height);
                        const id = spacetimeGPS.selectedId;
                        if (!id) return;
                        const pts = spacetimeGPS.track(id, 500);
                        if (pts.length < 2) {
                            ctx.fillStyle = "#8b949e";
                            ctx.font = "12px monospace";
                            ctx.fillText("(no trail yet — select a body)", 10, height / 2);
                            return;
                        }
                        // bounds
                        let minX = Infinity, maxX = -Infinity, minY = Infinity, maxY = -Infinity;
                        for (let i = 0; i < pts.length; i++) {
                            minX = Math.min(minX, pts[i].x);
                            maxX = Math.max(maxX, pts[i].x);
                            minY = Math.min(minY, pts[i].y);
                            maxY = Math.max(maxY, pts[i].y);
                        }
                        const pad = 6;
                        const pw = width - 2 * pad;
                        const ph = height - 2 * pad;
                        const dx = (maxX - minX) || 1;
                        const dy = (maxY - minY) || 1;
                        const sx = pw / dx;
                        const sy = ph / dy;
                        const scale = Math.min(sx, sy);
                        const offX = pad + (pw - dx * scale) / 2 - minX * scale;
                        const offY = pad + (ph - dy * scale) / 2 - minY * scale;

                        ctx.strokeStyle = "#00ffcc";
                        ctx.lineWidth = 2;
                        ctx.beginPath();
                        let first = true;
                        for (let i = 0; i < pts.length; i++) {
                            const px = offX + pts[i].x * scale;
                            const py = offY + pts[i].y * scale;
                            if (first) { ctx.moveTo(px, py); first = false; }
                            else ctx.lineTo(px, py);
                        }
                        ctx.stroke();
                    }
                    Connections {
                        target: spacetimeGPS
                        function onCountChanged() { gpsTrail.requestPaint(); }
                        function onSelectedIdChanged() { gpsTrail.requestPaint(); }
                    }
                }
            }
        }
    }
}
