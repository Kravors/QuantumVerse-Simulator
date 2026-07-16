// Phase 5.3: Interaction Stress Test
// Floods QmlGlViewport with synthetic mouse/wheel input and property toggles
// to verify stability under rapid interaction.

#include <QGuiApplication>
#include <QDebug>
#include <cassert>

#include "qmlglviewport.h"

using namespace quantumverse;

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    qDebug() << "=== InteractionStressTest ===";

    QmlGlViewport* vp = new QmlGlViewport(nullptr);
    assert(vp != nullptr);

    vp->setShowGrid(true);
    vp->setShowGeodesics(true);

    const int iterations = 2000;
    for (int i = 0; i < iterations; ++i) {
        float x = static_cast<float>(i % 800);
        float y = static_cast<float>(i % 600);

        vp->handleMousePress(x, y, 0);
        vp->handleMouseMove(x + 1.0f, y + 1.0f, 0);
        vp->handleMouseReleased(x, y, 0);
        vp->handleWheel(static_cast<float>(i % 240 - 120));

        if (i % 50 == 0) {
            vp->setShowGrid(!vp->showGrid());
            vp->setShowGeodesics(!vp->showGeodesics());
            vp->setShowLightCones(!vp->showLightCones());
            vp->setShowQuantumGeometry(!vp->showQuantumGeometry());
        }
        if (i % 200 == 0) {
            QGuiApplication::processEvents();
        }
    }

    delete vp;

    qDebug() << "InteractionStressTest completed without crash.";
    return 0;
}
