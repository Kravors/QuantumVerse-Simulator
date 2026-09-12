// Phase 5.1: QmlGlViewport isolation test
// Validates property getters/setters, signal emissions, and Q_INVOKABLE methods
// without requiring a full QML scene or OpenGL context.

#include <QGuiApplication>
#include <QSignalSpy>
#include <QDebug>

#include "qmlglviewport.h"
#include "ui4d/UI4D.h"
#include "test_assert.h"

using namespace quantumverse;

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    qDebug() << "=== ViewportIsolationTest ===";

    // --- Construction and default state ---------------------------------------
    QmlGlViewport* viewport = new QmlGlViewport(nullptr);
    QV_CHECK(viewport != nullptr);

    QV_CHECK(viewport->showGrid() == true);
    QV_CHECK(viewport->showLightCones() == false);
    QV_CHECK(viewport->showGeodesics() == false);
    QV_CHECK(viewport->showQuantumGeometry() == false);
    QV_CHECK(viewport->curvatureMode() == 0);
    QV_CHECK(viewport->cameraDistance() == 150.0f);
    QV_CHECK(viewport->cameraAngleX() == 0.524f);
    QV_CHECK(viewport->cameraAngleY() == 0.785f);
    QV_CHECK(viewport->simulationTime() == 0.0f);
    QV_CHECK(viewport->frameRate() == 0.0f);

    // --- Property setters emit signals ----------------------------------------
    QSignalSpy gridSpy(viewport, &QmlGlViewport::showGridChanged);
    viewport->setShowGrid(false);
    QV_CHECK(gridSpy.count() == 1);
    QV_CHECK(viewport->showGrid() == false);

    viewport->setShowGrid(false); // no change -> no signal
    QV_CHECK(gridSpy.count() == 1);

    QSignalSpy geodesicsSpy(viewport, &QmlGlViewport::showGeodesicsChanged);
    viewport->setShowGeodesics(true);
    QV_CHECK(geodesicsSpy.count() == 1);
    QV_CHECK(viewport->showGeodesics() == true);

    viewport->setShowGeodesics(false);
    QV_CHECK(geodesicsSpy.count() == 2);
    QV_CHECK(viewport->showGeodesics() == false);

    viewport->setShowGeodesics(false); // no change -> no signal
    QV_CHECK(geodesicsSpy.count() == 2);

    QSignalSpy lightConesSpy(viewport, &QmlGlViewport::showLightConesChanged);
    viewport->setShowLightCones(true);
    QV_CHECK(lightConesSpy.count() == 1);
    QV_CHECK(viewport->showLightCones() == true);

    QSignalSpy quantumSpy(viewport, &QmlGlViewport::showQuantumGeometryChanged);
    viewport->setShowQuantumGeometry(true);
    QV_CHECK(quantumSpy.count() == 1);
    QV_CHECK(viewport->showQuantumGeometry() == true);

    QSignalSpy modeSpy(viewport, &QmlGlViewport::curvatureModeChanged);
    viewport->setCurvatureMode(3);
    QV_CHECK(modeSpy.count() == 1);
    QV_CHECK(viewport->curvatureMode() == 3);

    // --- Camera property setters ----------------------------------------------
    QSignalSpy distSpy(viewport, &QmlGlViewport::cameraDistanceChanged);
    viewport->setCameraDistance(123.5f);
    QV_CHECK(distSpy.count() == 1);
    QV_CHECK(viewport->cameraDistance() == 123.5f);

    QSignalSpy angleXSpy(viewport, &QmlGlViewport::cameraAngleXChanged);
    viewport->setCameraAngleX(0.5f);
    QV_CHECK(angleXSpy.count() == 1);
    QV_CHECK(viewport->cameraAngleX() == 0.5f);

    QSignalSpy angleYSpy(viewport, &QmlGlViewport::cameraAngleYChanged);
    viewport->setCameraAngleY(-0.25f);
    QV_CHECK(angleYSpy.count() == 1);
    QV_CHECK(viewport->cameraAngleY() == -0.25f);

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
    QV_CHECK(viewport->probeValid() == false);
    QV_CHECK(viewport->kretschmann() == QString("—"));
    QV_CHECK(viewport->ricciScalar() == QString("—"));
    QV_CHECK(viewport->weylSquared() == QString("—"));
    QV_CHECK(viewport->redshift() == QString("—"));

    viewport->clearProbe();
    QV_CHECK(viewport->probeValid() == false);

    // --- Headless screenshot / frame target helpers are no-ops without window --
    viewport->setHeadlessFrameTarget(10);
    viewport->requestScreenshot(QStringLiteral("/tmp/noop.png"));
    QV_CHECK(viewport->screenshotRequested() == false);
    QV_CHECK(viewport->headlessTargetReached() == false);

    delete viewport;

    qDebug() << "All ViewportIsolationTest checks passed.";
    return 0;
}
