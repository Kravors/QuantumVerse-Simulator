#include <QTest>
#include <QObject>
#include <cmath>
#include <vector>
#include <algorithm>
#include "spacetime/LightCone.h"
#include "spacetime/MetricTensor.h"

class LightConeRenderingTest : public QObject
{
    Q_OBJECT

private slots:
    void test_flat_cone_generates_vertices()
    {
        quantumverse::Event4D origin(0.0, 0.0, 0.0, 0.0);
        quantumverse::LightCone cone(origin, quantumverse::LightConeType::FUTURE, M_PI / 4.0, 16);
        cone.generateMesh();

        QVERIFY(cone.isValid());
        QVERIFY(cone.getVertexCount() > 0);
        QVERIFY(cone.getIndexCount() > 0);
    }

    void test_cone_max_extent_is_positive()
    {
        quantumverse::Event4D origin(0.0, 0.0, 0.0, 0.0);
        quantumverse::LightCone cone(origin, quantumverse::LightConeType::FUTURE, M_PI / 4.0, 16);
        cone.generateMesh();

        QVERIFY(cone.getMaxExtent() > 0.0);
    }

    void test_cone_resolution_affects_vertex_count()
    {
        quantumverse::Event4D origin(0.0, 0.0, 0.0, 0.0);
        quantumverse::LightCone lowRes(origin, quantumverse::LightConeType::FUTURE, M_PI / 4.0, 8);
        quantumverse::LightCone highRes(origin, quantumverse::LightConeType::FUTURE, M_PI / 4.0, 32);
        lowRes.generateMesh();
        highRes.generateMesh();

        QVERIFY(highRes.getVertexCount() > lowRes.getVertexCount());
    }

    void test_cone_angle_affects_extent()
    {
        quantumverse::Event4D origin(0.0, 0.0, 0.0, 0.0);
        quantumverse::LightCone narrow(origin, quantumverse::LightConeType::FUTURE, M_PI / 8.0, 16);
        quantumverse::LightCone wide(origin, quantumverse::LightConeType::FUTURE, M_PI / 2.0, 16);
        narrow.generateMesh();
        wide.generateMesh();

        QVERIFY(wide.getMaxExtent() > narrow.getMaxExtent());
    }

    void test_flat_cone_vertex_positions_are_finite()
    {
        quantumverse::Event4D origin(0.0, 0.0, 0.0, 0.0);
        quantumverse::LightCone cone(origin, quantumverse::LightConeType::FUTURE, M_PI / 4.0, 16);
        cone.generateMesh();

        const auto &vertices = cone.getVertices();
        QVERIFY(!vertices.empty());
        for (const auto &v : vertices) {
            QVERIFY(std::isfinite(v.position[0]));
            QVERIFY(std::isfinite(v.position[1]));
            QVERIFY(std::isfinite(v.position[2]));
        }
    }

    void test_cone_with_metric_generates_geometry()
    {
        quantumverse::Event4D origin(0.0, 0.0, 0.0, 0.0);
        quantumverse::MetricTensor m = quantumverse::MetricTensor::schwarzschild(1.0, 10.0, M_PI / 2.0, 0.0);
        auto metric = std::make_shared<quantumverse::MetricTensor>(m);

        quantumverse::LightCone cone(origin, quantumverse::LightConeType::FUTURE, M_PI / 4.0, 16);
        cone.setMetric(metric);
        cone.generateMesh();

        QVERIFY(cone.isValid());
        QVERIFY(cone.getVertexCount() > 0);
    }
};

QTEST_MAIN(LightConeRenderingTest)
#include "test_lightcone_rendering.moc"
