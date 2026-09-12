// Phase 9.2: HUD Toggle Test (rebuild trigger)
// Validates showHUD property getter/setter and signal emission without
// requiring a full QML scene or OpenGL context.

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
    qDebug() << "=== HUDToggleTest ===";

    QmlGlViewport* viewport = new QmlGlViewport(nullptr);

    // Default state: HUD off
    QV_CHECK(viewport->showHUD() == false);

    // Toggle on
    QSignalSpy hudSpy(viewport, &QmlGlViewport::showHUDChanged);
    viewport->setShowHUD(true);
    QV_CHECK(hudSpy.count() == 1);
    QV_CHECK(viewport->showHUD() == true);

    // Toggle off
    viewport->setShowHUD(false);
    QV_CHECK(hudSpy.count() == 2);
    QV_CHECK(viewport->showHUD() == false);

    // Rapid toggle stress
    for (int i = 0; i < 100; ++i) {
        viewport->setShowHUD(i % 2 == 0);
    }
    QV_CHECK(viewport->showHUD() == false);

    // HUD toggle does not affect other properties
    viewport->setShowGrid(true);
    viewport->setShowGeodesics(true);
    viewport->setShowQuantumGeometry(false);
    QV_CHECK(viewport->showGrid() == true);
    QV_CHECK(viewport->showGeodesics() == true);
    QV_CHECK(viewport->showQuantumGeometry() == false);

    viewport->setShowHUD(true);
    QV_CHECK(viewport->showGrid() == true);
    QV_CHECK(viewport->showGeodesics() == true);
    QV_CHECK(viewport->showQuantumGeometry() == false);
    QV_CHECK(viewport->showHUD() == true);

    delete viewport;

    qDebug() << "HUDToggleTest PASSED";
    printf("HUDToggleTest PASSED\n");
    return 0;
}
