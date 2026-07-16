// Phase 5.1: QmlGlViewport isolation test
// Validates property getters/setters, signal emissions, and Q_INVOKABLE methods
// without requiring a full QML scene or OpenGL context.

#include <QGuiApplication>
#include <QSignalSpy>
#include <QDebug>
#include <cassert>

#include "qmlglviewport.h"
#include "ui4d/UI4D.h"

using namespace quantumverse;

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    qDebug() << "=== ViewportIsolationTest ===";

    // --- Construction and default state ---------------------------------------
    QmlGlViewport* viewport = new QmlGlViewport(nullptr);
    assert(viewport != nullptr);

    assert(viewport->showGrid() == true);
    assert(viewport->showLightCones() == false);
    assert(viewport->showGeodesics() == true);
    assert(viewport->showQuantumGeometry() == false);
    assert(viewport->curvatureMode() == 0);
    assert(viewport->cameraDistance() == 50.0f);
    assert(viewport->cameraAngleX() == 0.0f);
    assert(viewport->cameraAngleY() == 0.0f);
    assert(viewport->simulationTime() == 0.0f);
    assert(viewport->frameRate() == 0.0f);

    // --- Property setters emit signals ----------------------------------------
    QSignalSpy gridSpy(viewport, &QmlGlViewport::showGridChanged);
    viewport->setShowGrid(false);
    assert(gridSpy.count() == 1);
    assert(viewport->showGrid() == false);

    viewport->setShowGrid(false); // no change -> no signal
    assert(gridSpy.count() == 1);

    QSignalSpy geodesicsSpy(viewport, &QmlGlViewport::showGeodesicsChanged);
    viewport->setShowGeodesics(false);
    assert(geodesicsSpy.count() == 1);
    assert(viewport->showGeodesics() == false);

    QSignalSpy lightConesSpy(viewport, &QmlGlViewport::showLightConesChanged);
    viewport->setShowLightCones(true);
    assert(lightConesSpy.count() == 1);
    assert(viewport->showLightCones() == true);

    QSignalSpy quantumSpy(viewport, &QmlGlViewport::showQuantumGeometryChanged);
    viewport->setShowQuantumGeometry(true);
    assert(quantumSpy.count() == 1);
    assert(viewport->showQuantumGeometry() == true);

    QSignalSpy modeSpy(viewport, &QmlGlViewport::curvatureModeChanged);
    viewport->setCurvatureMode(3);
    assert(modeSpy.count() == 1);
    assert(viewport->curvatureMode() == 3);

    // --- Camera property setters ----------------------------------------------
    QSignalSpy distSpy(viewport, &QmlGlViewport::cameraDistanceChanged);
    viewport->setCameraDistance(123.5f);
    assert(distSpy.count() == 1);
    assert(viewport->cameraDistance() == 123.5f);

    QSignalSpy angleXSpy(viewport, &QmlGlViewport::cameraAngleXChanged);
    viewport->setCameraAngleX(0.5f);
    assert(angleXSpy.count() == 1);
    assert(viewport->cameraAngleX() == 0.5f);

    QSignalSpy angleYSpy(viewport, &QmlGlViewport::cameraAngleYChanged);
    viewport->setCameraAngleY(-0.25f);
    assert(angleYSpy.count() == 1);
    assert(viewport->cameraAngleY() == -0.25f);

    // --- Q_INVOKABLE methods do not crash without a GL context ----------------
    viewport->zoomIn();
    viewport->zoomOut();
    viewport->resetView();
    viewport->updateSimulation(0.016);
    viewport->setSliceOffset(0, 5.0);
    viewport->setSliceOffset(1, -2.0);
    viewport->setSliceOffset(99, 1.0); // out-of-range must not crash

    // --- Probe API ------------------------------------------------------------
    // Without a metric the readout should stay invalid / placeholder.
    viewport->probeAt(1.0, 2.0, 3.0);
    assert(viewport->probeValid() == false);
    assert(viewport->kretschmann() == QString("—"));
    assert(viewport->ricciScalar() == QString("—"));
    assert(viewport->weylSquared() == QString("—"));
    assert(viewport->redshift() == QString("—"));

    viewport->clearProbe();
    assert(viewport->probeValid() == false);

    // --- Headless screenshot / frame target helpers are no-ops without window --
    viewport->setHeadlessFrameTarget(10);
    viewport->requestScreenshot(QStringLiteral("/tmp/noop.png"));
    assert(viewport->screenshotRequested() == false);
    assert(viewport->headlessTargetReached() == false);

    delete viewport;

    qDebug() << "All ViewportIsolationTest checks passed.";
    return 0;
}
