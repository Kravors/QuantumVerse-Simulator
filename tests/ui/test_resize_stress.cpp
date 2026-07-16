// Phase 5.4: Resize/Fullscreen/Toggle Stress Test
// Rapidly resizes QmlGlViewport and toggles rendering state to verify
// FBO/texture resource management under stress.

#include <QGuiApplication>
#include <QDebug>
#include <cassert>

#include "qmlglviewport.h"

using namespace quantumverse;

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    qDebug() << "=== ResizeStressTest ===";

    QmlGlViewport* vp = new QmlGlViewport(nullptr);
    assert(vp != nullptr);

    vp->setShowGrid(true);
    vp->setShowGeodesics(true);
    vp->setShowLightCones(false);
    vp->setShowQuantumGeometry(false);

    // Rapid size oscillation via QQuickItem width/height setters
    for (int i = 0; i < 50; ++i) {
        vp->setWidth(1920);
        vp->setHeight(1080);
        QGuiApplication::processEvents();
        vp->setWidth(100);
        vp->setHeight(100);
        QGuiApplication::processEvents();
    }

    // Rapid property toggling
    for (int j = 0; j < 100; ++j) {
        vp->setShowGrid(j % 2 == 0);
        vp->setShowGeodesics(j % 2 == 0);
        vp->setShowLightCones(j % 2 != 0);
        vp->setShowQuantumGeometry(j % 2 != 0);
        vp->setCurvatureMode(j % 5);
        QGuiApplication::processEvents();
    }

    // Final resize to a non-trivial size
    vp->setWidth(1280);
    vp->setHeight(720);
    QGuiApplication::processEvents();

    delete vp;

    qDebug() << "ResizeStressTest completed without crash.";
    return 0;
}
