// Phase 5.2: UI4D unit test
// Exercises camera controls, slicing, probe, and causal-graph logic
// without any Qt or OpenGL dependency.

#include "ui4d/UI4D.h"
#include "spacetime/Event4D.h"
#include "spacetime/MetricTensor.h"
#include <cmath>
#include <cassert>
#include <iostream>

using namespace quantumverse;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main()
{
    std::cout << "=== UI4DTest ===" << std::endl;

    // --- Construction and solar system ----------------------------------------
    UI4D ui4d;
    {
        const auto& solar = ui4d.getSolarSystemData();
        assert(solar.bodies.count("Sun") == 1);
        assert(solar.bodies.count("Earth") == 1);
        assert(solar.bodies.at("Sun").isCentralBody);
        assert(solar.bodies.at("Earth").semiMajorAxis > 0.0);
        (void)solar;
    }

    // --- Config toggles -------------------------------------------------------
    ui4d.toggle4DView();
    assert(ui4d.getConfig().showLightCones == true);
    ui4d.toggleLightCones();
    assert(ui4d.getConfig().showLightCones == false);
    ui4d.toggleLightCones();
    assert(ui4d.getConfig().showLightCones == true);

    ui4d.toggleCausalGraph();
    assert(ui4d.getConfig().showCausalStructure == true);

    // --- Camera controls ------------------------------------------------------
    {
        const auto& cam = ui4d.getCamera();
        Event4D origPos = cam.getPosition();

        ui4d.translateCamera({0.0, 1.0, 0.0, 0.0});
        {
            const auto& p = ui4d.getCamera().getPosition();
            (void)origPos;
            assert(p.x == origPos.x + 1.0);
            (void)p;
        }

        ui4d.rotateCamera(1, 2, M_PI / 6.0);
        ui4d.zoomCamera(5.0);
    }

    // --- Slice views ----------------------------------------------------------
    ui4d.setSliceOffset(0, 7.5);
    ui4d.setSliceOffset(1, -3.0);
    ui4d.setSliceOffset(99, 1.0);
    {
        const auto& slices = ui4d.getSliceViews();
        (void)slices;
        assert(slices.size() >= 2u);
        assert(slices[0].getParameter() == 7.5);
        assert(slices[1].getParameter() == -3.0);
    }

    // --- Causal graph ---------------------------------------------------------
    ui4d.clearCausalGraph();
    Event4D e1(0.0, 0.0, 0.0, 0.0);
    Event4D e2(1.0, 3.0, 0.0, 0.0);
    Event4D e3(-1.0, 4.0, 0.0, 0.0);

    {
        int idx1 = ui4d.addEvent(e1);
        int idx2 = ui4d.addEvent(e2);
        int idx3 = ui4d.addEvent(e3);
        (void)idx1; (void)idx2; (void)idx3;
        assert(ui4d.getCausalGraph().getNodes().size() == 3u);
    }

    ui4d.selectEvent(1);
    assert(ui4d.getCausalGraph().getNodes().size() == 3u);
    assert(ui4d.getCausalGraph().areCausallyRelated(0, 1) == true);

    // --- Discovery probe ------------------------------------------------------
    ui4d.moveProbeTo(Event4D(0.0, 1e10, 0.0, 0.0));
    assert(ui4d.getProbe().getPosition().x == 1e10);

    {
        auto metric = std::make_shared<MetricTensor>();
        auto sample = ui4d.getProbe().sampleCurvature(metric);
        assert(sample.kretschmann >= 0.0);
        assert(sample.ricci == 0.0);
    }

    // --- Waypoints ------------------------------------------------------------
    ui4d.addWaypoint("Origin", Event4D(0.0, 0.0, 0.0, 0.0), "Start");
    ui4d.addWaypoint("Target", Event4D(1.0, 10.0, 0.0, 0.0), "End");

    // --- Anomaly / singularity editing stubs ----------------------------------
    ui4d.detectCurvatureAnomalies();
    ui4d.clearAnomalies();
    ui4d.toggleAnomalyDetection(true);
    ui4d.setAnomalyDetectionThreshold(500.0);

    ui4d.activateCurvatureManipulation(Event4D(), UI4D::CurvatureManipulationTool::POSITIVE);
    ui4d.setCurvatureManipulationRadius(15.0f);
    ui4d.setCurvatureManipulationStrength(2.0f);
    ui4d.deactivateCurvatureManipulation();

    ui4d.enableLensingEffect(true);
    ui4d.setLensingNumRings(8);
    ui4d.setLensingRingSpacing(3.0f);
    ui4d.setLensingIntensity(0.9f);

    ui4d.enableTimeDilationEffect(true);
    ui4d.setTimeDilationRange(0.2f, 3.0f);
    ui4d.setTimeDilationShowGrid(false);

    ui4d.enableQuantumFoamEffect(true);
    ui4d.setQuantumFoamScale(1.0e-34f);
    ui4d.setQuantumFoamFluctuationCount(200);
    ui4d.setQuantumFoamLifetime(0.05f);
    ui4d.setQuantumFoamEnergy(2.0e19f);

    // --- Layout management (no crash) -----------------------------------------
    ui4d.togglePanelVisibility(0);
    ui4d.collapsePanel(1);
    ui4d.expandPanel(1);
    ui4d.setPanelPosition(0, 10, 20);
    ui4d.setPanelSize(0, 640, 480);
    ui4d.startResize(0, 0, 0);
    ui4d.updateResize(100, 100);
    ui4d.endResize();

    std::cout << "All UI4DTest checks passed." << std::endl;
    return 0;
}
