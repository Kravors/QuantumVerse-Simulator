// Phase 9.2: HUD Toggle Test (rebuild trigger)
// Validates showHUD property getter/setter and signal emission without
// requiring a full QML scene or OpenGL context.

#include <QGuiApplication>
#include <QSignalSpy>
#include <QDebug>

#include "qmlglviewport.h"
#include "ui4d/UI4D.h"

using namespace quantumverse;

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    qDebug() << "=== HUDToggleTest ===";

    QmlGlViewport* viewport = new QmlGlViewport(nullptr);

    // Default state: HUD off
    assert(viewport->showHUD() == false);

    // Toggle on
    QSignalSpy hudSpy(viewport, &QmlGlViewport::showHUDChanged);
    viewport->setShowHUD(true);
    assert(hudSpy.count() == 1);
    assert(viewport->showHUD() == true);

    // Toggle off
    viewport->setShowHUD(false);
    assert(hudSpy.count() == 2);
    assert(viewport->showHUD() == false);

    // Rapid toggle stress
    for (int i = 0; i < 100; ++i) {
        viewport->setShowHUD(i % 2 == 0);
    }
    assert(viewport->showHUD() == false);

    // HUD toggle does not affect other properties
    viewport->setShowGrid(true);
    viewport->setShowGeodesics(true);
    viewport->setShowQuantumGeometry(false);
    assert(viewport->showGrid() == true);
    assert(viewport->showGeodesics() == true);
    assert(viewport->showQuantumGeometry() == false);

    viewport->setShowHUD(true);
    assert(viewport->showGrid() == true);
    assert(viewport->showGeodesics() == true);
    assert(viewport->showQuantumGeometry() == false);
    assert(viewport->showHUD() == true);

    delete viewport;

    qDebug() << "HUDToggleTest PASSED";
    printf("HUDToggleTest PASSED\n");
    return 0;
}
