/**
 * @file test_spacetime_gps.cpp
 * @brief Unit tests for the SpacetimeGPS object registry + telemetry feed.
 */

#include <QCoreApplication>
#include <QSignalSpy>
#include <QModelIndex>
#include <memory>
#include <cmath>
#include <iostream>

#include "ui4d/SpacetimeGPS.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "test_assert.h"

using namespace quantumverse;

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    std::cout << "=== SpacetimeGPS Test ===" << std::endl;

    SpacetimeGPS gps;
    QV_CHECK(gps.count() == 0);
    QV_CHECK(gps.trackingEnabled());
    QV_CHECK(gps.updateHz() == 10);

    QSignalSpy countSpy(&gps, &SpacetimeGPS::countChanged);

    // Register a planet and a black hole.
    SpacetimeGPS::Tracked earth;
    earth.id = QStringLiteral("body:earth");
    earth.name = QStringLiteral("Earth");
    earth.type = QStringLiteral("Planet");
    earth.pos = Event4D(0.0, 1.496e11, 0.0, 0.0); // 1 AU on +X
    earth.vel = Event4D(0.0, 0.0, 29780.0, 0.0);
    earth.spinX = 0.0; earth.spinY = 1.0; earth.spinZ = 0.0;
    earth.spinRate = 7.292e-5;
    gps.registerObject(earth);

    SpacetimeGPS::Tracked bh;
    bh.id = QStringLiteral("bh:primary");
    bh.name = QStringLiteral("Kerr BH");
    bh.type = QStringLiteral("BlackHole");
    bh.pos = Event4D(0.0, 0.0, 0.0, 0.0);
    bh.spinY = 1.0;
    bh.spinRate = 0.5;
    gps.registerObject(bh);

    QV_CHECK(gps.count() == 2);
    QV_CHECK(countSpy.count() == 2);

    // Role data.
    QModelIndex idx = gps.index(0);
    QV_CHECK(gps.data(idx, SpacetimeGPS::IdRole).toString() == QStringLiteral("body:earth"));
    QV_CHECK(gps.data(idx, SpacetimeGPS::NameRole).toString() == QStringLiteral("Earth"));
    QV_CHECK(gps.data(idx, SpacetimeGPS::TypeRole).toString() == QStringLiteral("Planet"));
    QV_CHECK(qAbs(gps.data(idx, SpacetimeGPS::XRole).toDouble() - 1.496e11) < 1e-6);
    QV_CHECK(qAbs(gps.data(idx, SpacetimeGPS::SpeedRole).toDouble() - 29780.0) < 1e-3);
    QV_CHECK(gps.data(idx, SpacetimeGPS::PinnedRole).toBool() == false);

    // types() list.
    QStringList types = gps.types();
    QV_CHECK(types.contains(QStringLiteral("All")));
    QV_CHECK(types.contains(QStringLiteral("Planet")));
    QV_CHECK(types.contains(QStringLiteral("BlackHole")));

    // Update from a metric first so r / redshift are computed for every object.
    auto metric = std::make_shared<quantumverse::SchwarzschildMetric>(1.989e30); // 1 Msun
    gps.updateFromSimulation(nullptr, metric.get(), 0.0, 0.016);

    // details() map now reflects computed r / redshift.
    QVariantMap det = gps.details(QStringLiteral("body:earth"));
    QV_CHECK(det.value(QStringLiteral("id")).toString() == QStringLiteral("body:earth"));
    QV_CHECK(qAbs(det.value(QStringLiteral("r")).toDouble() - 1.496e11) < 1e-6);
    QV_CHECK(std::isfinite(det.value(QStringLiteral("redshift")).toDouble()));

    // bh at origin -> r == 0
    QV_CHECK(qAbs(gps.data(gps.index(1), SpacetimeGPS::RRole).toDouble()) < 1e-9);
    QV_CHECK(std::isfinite(gps.data(gps.index(1), SpacetimeGPS::RedshiftRole).toDouble()));

    // pin toggles PinnedRole and emits dataChanged.
    QSignalSpy dataSpy(&gps, &SpacetimeGPS::dataChanged);
    gps.pin(QStringLiteral("body:earth"), true);
    QV_CHECK(dataSpy.count() >= 1);
    QV_CHECK(gps.data(gps.index(0), SpacetimeGPS::PinnedRole).toBool() == true);

    // focusOn emits focusRequested with viewport-scaled coords (scale=1 default).
    QSignalSpy focusSpy(&gps, &SpacetimeGPS::focusRequested);
    gps.focusOn(QStringLiteral("bh:primary"));
    QV_CHECK(focusSpy.count() == 1);
    QVariantList args = focusSpy.takeFirst();
    QV_CHECK(args.at(0).toString() == QStringLiteral("bh:primary"));
    QV_CHECK(qAbs(args.at(1).toDouble()) < 1e-9);
    QV_CHECK(qAbs(args.at(2).toDouble()) < 1e-9);

    // track() history.
    QVariantList trk = gps.track(QStringLiteral("body:earth"));
    QV_CHECK(trk.size() >= 1);

    std::cout << "=== SpacetimeGPS Test: ALL PASS ===" << std::endl;
    return 0;
}
