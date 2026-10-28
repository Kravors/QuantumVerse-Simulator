// QuantumVerse Test: SandboxManager — editable static body scene with
// JSON save/load round-trip.
//
// Exercises addBody/updateBody/removeBody/clear and the save→clear→load
// round-trip invariant (V1 core requirement).

#include "sandbox/SandboxManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>

#include <iostream>

#include "test_assert.h"

using namespace quantumverse;

int main(int /*argc*/, char* /*argv*/[])
{
    int argc = 0;
    QCoreApplication app(argc, nullptr);

    std::cout << "=== SandboxManagerTest ===" << std::endl;

    SandboxManager mgr;

    // --- addBody --------------------------------------------------------------
    QString id1 = mgr.addBody("star", "Test Star");
    QV_CHECK(!id1.isEmpty());
    QV_CHECK(mgr.bodies().size() == 1);

    QString id2 = mgr.addBody("planet", "Test Planet");
    QV_CHECK(!id2.isEmpty());
    QV_CHECK(id2 != id1);
    QV_CHECK(mgr.bodies().size() == 2);

    // Verify body fields
    QVariantList bodies = mgr.bodies();
    QV_CHECK(bodies[0].toMap().value("name").toString() == "Test Star");
    QV_CHECK(bodies[0].toMap().value("type").toString() == "star");
    QV_CHECK(bodies[1].toMap().value("name").toString() == "Test Planet");
    QV_CHECK(bodies[1].toMap().value("type").toString() == "planet");

    // --- updateBody -----------------------------------------------------------
    mgr.updateBody(id1, QVariantMap{{"mass_kg", 1.989e30}, {"radius_m", 696340000.0}});
    bodies = mgr.bodies();
    QV_CHECK_NEAR(bodies[0].toMap().value("mass_kg").toDouble(), 1.989e30, 1e20);
    QV_CHECK_NEAR(bodies[0].toMap().value("radius_m").toDouble(), 696340000.0, 1e-3);

    // updateBody on unknown id should not crash and not change count
    mgr.updateBody("nonexistent-id", QVariantMap{{"name", "x"}});
    QV_CHECK(mgr.bodies().size() == 2);

    // --- removeBody -----------------------------------------------------------
    mgr.removeBody(id2);
    QV_CHECK(mgr.bodies().size() == 1);
    QV_CHECK(mgr.bodies()[0].toMap().value("id").toString() == id1);

    // removeBody on unknown id should not crash
    mgr.removeBody("nonexistent-id");
    QV_CHECK(mgr.bodies().size() == 1);

    // --- clear ----------------------------------------------------------------
    mgr.clear();
    QV_CHECK(mgr.bodies().size() == 0);

    // --- save / load round-trip (the V1 invariant) ---------------------------
    QString tmpPath = QDir::tempPath() + "/sandboxmanager_test.json";
    QFile::remove(tmpPath); // ensure clean start

    mgr.addBody("black_hole", "Sgr A*");
    mgr.addBody("star", "Sun");
    QV_CHECK(mgr.bodies().size() == 2);

    QV_CHECK(mgr.saveTo(tmpPath));
    QV_CHECK(mgr.currentFilePath() == tmpPath);

    // clear, then load — must reproduce exactly what was saved
    mgr.clear();
    QV_CHECK(mgr.bodies().size() == 0);

    QV_CHECK(mgr.loadFrom(tmpPath));
    QV_CHECK(mgr.bodies().size() == 2);
    QV_CHECK(mgr.currentFilePath() == tmpPath);

    bodies = mgr.bodies();
    QV_CHECK(bodies[0].toMap().value("name").toString() == "Sgr A*");
    QV_CHECK(bodies[0].toMap().value("type").toString() == "black_hole");
    QV_CHECK(bodies[1].toMap().value("name").toString() == "Sun");
    QV_CHECK(bodies[1].toMap().value("type").toString() == "star");

    // Verify positions survive round-trip (set explicit positions before save)
    mgr.clear();
    mgr.addBody("planet", "Earth");
    mgr.updateBody(mgr.bodies()[0].toMap().value("id").toString(),
                   QVariantMap{{"x", 1.0}, {"y", 2.0}, {"z", 3.0}});
    QV_CHECK(mgr.saveTo(tmpPath));
    mgr.clear();
    QV_CHECK(mgr.loadFrom(tmpPath));
    QV_CHECK(mgr.bodies().size() == 1);
    QV_CHECK_NEAR(mgr.bodies()[0].toMap().value("x").toDouble(), 1.0, 1e-9);
    QV_CHECK_NEAR(mgr.bodies()[0].toMap().value("y").toDouble(), 2.0, 1e-9);
    QV_CHECK_NEAR(mgr.bodies()[0].toMap().value("z").toDouble(), 3.0, 1e-9);

    // --- defaultSaveDir -------------------------------------------------------
    QString defDir = mgr.defaultSaveDir();
    QV_CHECK(!defDir.isEmpty());
    QV_CHECK(QDir(defDir).exists());

    // cleanup
    QFile::remove(tmpPath);

    std::cout << "=== ALL SANDBOXMANAGER TESTS PASSED ===" << std::endl;
    return 0;
}
