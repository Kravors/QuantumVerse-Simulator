#include "glad.h"
#include "UI4D.h"
#include "../config/ConfigLoader.h"
#include "physics/PhysicsConstants.h"
#include "../rendering/ComputeShader.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

#if defined(_MSC_VER)
#include <intrin.h>
#endif

// Ensure M_PI is available on all platforms (MSVC <cmath> may not define it)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Include glad.h for OpenGL function prototypes if available
#if __has_include("glad.h")
#include "glad.h"
#define HAS_GLAD 1
#else
#define HAS_GLAD 0
#endif

namespace quantumverse {

// ============================================================================
// Real-time editing methods implementation
// ============================================================================

void UI4D::startEventCreation(const Event4D& position) {
    isCreatingEvent = true;
    std::cout << "Starting event creation at: (" 
              << position.t << ", " << position.x << ", " 
              << position.y << ", " << position.z << ")" << std::endl;
}

void UI4D::endEventCreation() {
    isCreatingEvent = false;
    std::cout << "Ending event creation" << std::endl;
}

void UI4D::deleteSelectedEvent() {
    if (selectedEventIndex >= 0 && selectedEventIndex < (int)causalGraph.getNodes().size()) {
        causalGraph.clear();
        selectedEventIndex = -1;
        selectedBody.clear();
        std::cout << "Deleted selected event" << std::endl;
    }
}

void UI4D::startWorldLineCreation(const Event4D& position) {
    isCreatingWorldLine = true;
    newWorldLinePoints.clear();
    newWorldLinePoints.push_back(position);
    std::cout << "Starting world-line creation at: (" 
              << position.t << ", " << position.x << ", " 
              << position.y << ", " << position.z << ")" << std::endl;
}

void UI4D::addWorldLinePoint(const Event4D& position) {
    if (isCreatingWorldLine) {
        newWorldLinePoints.push_back(position);
        std::cout << "Added world-line point at: (" 
                  << position.t << ", " << position.x << ", " 
                  << position.y << ", " << position.z << ")" << std::endl;
    }
}

void UI4D::endWorldLineCreation() {
    if (isCreatingWorldLine && !newWorldLinePoints.empty()) {
        WorldLine newWorldLine;
        newWorldLine.name = "New Object";
        newWorldLine.isClosed = false;
        newWorldLine.mass = 1.0e24;
        
        for (size_t i = 0; i < newWorldLinePoints.size(); i++) {
            WorldLinePoint point;
            point.event = newWorldLinePoints[i];
            point.properTime = 0.0;
            point.isControlPoint = true;
            newWorldLine.points.push_back(point);
        }
        
        worldLines.push_back(newWorldLine);
        std::cout << "Ended world-line creation with " 
                  << newWorldLinePoints.size() << " points" << std::endl;
    }
    isCreatingWorldLine = false;
    newWorldLinePoints.clear();
}

void UI4D::editWorldLineControlPoint(int worldLineIndex, int pointIndex, const Event4D& newPosition) {
    if (worldLineIndex >= 0 && worldLineIndex < (int)worldLines.size() &&
        pointIndex >= 0 && pointIndex < (int)worldLines[worldLineIndex].points.size()) {
        worldLines[worldLineIndex].points[pointIndex].event = newPosition;
        std::cout << "Edited world-line control point (" 
                  << worldLineIndex << ", " << pointIndex << ") to: (" 
                  << newPosition.t << ", " << newPosition.x << ", " 
                  << newPosition.y << ", " << newPosition.z << ")" << std::endl;
    }
}

void UI4D::deleteWorldLine(int worldLineIndex) {
    if (worldLineIndex >= 0 && worldLineIndex < (int)worldLines.size()) {
        worldLines.erase(worldLines.begin() + worldLineIndex);
        std::cout << "Deleted world-line at index: " << worldLineIndex << std::endl;
    }
}

void UI4D::startSingularityEditing(std::shared_ptr<SingularityHandler> singularity) {
    editingSingularity = std::make_shared<SingularityEditData>();
    editingSingularity->singularity = singularity;
    editingSingularity->isPositionEditing = false;
    editingSingularity->isMassEditing = false;
    editingSingularity->isSpinEditing = false;
    editingSingularity->isChargeEditing = false;
    std::cout << "Started singularity editing" << std::endl;
}

void UI4D::editSingularityPosition(const std::array<double, 3>& newPosition) {
    if (editingSingularity && editingSingularity->singularity) {
        editingSingularity->singularity->setPosition(newPosition);
        editingSingularity->isPositionEditing = true;
        std::cout << "Edited singularity position to: (" 
                  << newPosition[0] << ", " << newPosition[1] << ", " 
                  << newPosition[2] << ")" << std::endl;
    }
}

void UI4D::editSingularityMass(double newMass) {
    if (editingSingularity && editingSingularity->singularity) {
        std::cout << "Edited singularity mass to: " << newMass << " kg" << std::endl;
        editingSingularity->isMassEditing = true;
    }
}

void UI4D::editSingularitySpin(double newAngularMomentum) {
    if (editingSingularity && editingSingularity->singularity) {
        std::cout << "Edited singularity spin to: " << newAngularMomentum << " J" << std::endl;
        editingSingularity->isSpinEditing = true;
    }
}

void UI4D::editSingularityCharge(double newCharge) {
    if (editingSingularity && editingSingularity->singularity) {
        std::cout << "Edited singularity charge to: " << newCharge << " C" << std::endl;
        editingSingularity->isChargeEditing = true;
    }
}

void UI4D::endSingularityEditing() {
    editingSingularity.reset();
    std::cout << "Ended singularity editing" << std::endl;
}

void UI4D::startBodyPropertyEditing(const std::string& bodyName) {
    if (solarSystem.bodies.find(bodyName) != solarSystem.bodies.end()) {
        editingBody = std::make_shared<BodyPropertyEditData>();
        editingBody->bodyName = bodyName;
        editingBody->isMassEditing = false;
        editingBody->isRadiusEditing = false;
        editingBody->isCompositionEditing = false;
        editingBody->isVelocityEditing = false;
        std::cout << "Started body property editing for: " << bodyName << std::endl;
    }
}

void UI4D::editBodyMass(double newMass) {
    if (editingBody && solarSystem.bodies.find(editingBody->bodyName) != solarSystem.bodies.end()) {
        solarSystem.bodies[editingBody->bodyName].mass = newMass;
        editingBody->isMassEditing = true;
        std::cout << "Edited body mass to: " << newMass << " kg" << std::endl;
    }
}

void UI4D::editBodyRadius(double newRadius) {
    if (editingBody && solarSystem.bodies.find(editingBody->bodyName) != solarSystem.bodies.end()) {
        solarSystem.bodies[editingBody->bodyName].radius = newRadius;
        editingBody->isRadiusEditing = true;
        std::cout << "Edited body radius to: " << newRadius << " m" << std::endl;
    }
}

void UI4D::editBodyComposition(const std::string& composition) {
    if (editingBody && solarSystem.bodies.find(editingBody->bodyName) != solarSystem.bodies.end()) {
        editingBody->isCompositionEditing = true;
        std::cout << "Edited body composition to: " << composition << std::endl;
    }
}

void UI4D::editBodyVelocity(const Event4D& newVelocity) {
    if (editingBody && solarSystem.bodies.find(editingBody->bodyName) != solarSystem.bodies.end()) {
        solarSystem.bodies[editingBody->bodyName].velocity = newVelocity;
        editingBody->isVelocityEditing = true;
        std::cout << "Edited body velocity to: (" 
                  << newVelocity.t << ", " << newVelocity.x << ", " 
                  << newVelocity.y << ", " << newVelocity.z << ")" << std::endl;
    }
}

void UI4D::endBodyPropertyEditing() {
    editingBody.reset();
    std::cout << "Ended body property editing" << std::endl;
}

// ============================================================================
    // Power-up features implementation
    // ============================================================================

    void UI4D::addWaypoint(const std::string& name, const Event4D& position, const std::string& description) {
        Waypoint wp;
        wp.name = name;
        wp.position = position;
        wp.description = description;
        wp.isActive = false;
        waypoints.push_back(wp);
        std::cout << "Added waypoint: " << name << std::endl;
    }

    void UI4D::removeWaypoint(int index) {
        if (index >= 0 && index < static_cast<int>(waypoints.size())) {
            waypoints.erase(waypoints.begin() + index);
            if (activeWaypointIndex >= index) {
                activeWaypointIndex = -1;
            }
            std::cout << "Removed waypoint at index: " << index << std::endl;
        }
    }

    void UI4D::setActiveWaypoint(int index) {
        if (index >= 0 && index < static_cast<int>(waypoints.size())) {
            // Deactivate current waypoint
            if (activeWaypointIndex >= 0 && activeWaypointIndex < static_cast<int>(waypoints.size())) {
                waypoints[activeWaypointIndex].isActive = false;
            }
            
            // Activate new waypoint
            activeWaypointIndex = index;
            waypoints[index].isActive = true;
            
            // Move camera to waypoint
            camera = Camera4D(waypoints[index].position, 
                            Event4D(waypoints[index].position.t + 1.0, 
                                   waypoints[index].position.x, 
                                   waypoints[index].position.y, 
                                   waypoints[index].position.z));
            std::cout << "Set active waypoint: " << waypoints[index].name << std::endl;
        }
    }

    void UI4D::clearWaypoints() {
        waypoints.clear();
        activeWaypointIndex = -1;
        std::cout << "Cleared all waypoints" << std::endl;
    }
    
    // Waypoint flight system implementation
    void UI4D::updateWaypointFlight(float deltaTime) {
        if (!waypointFlight.enabled || waypoints.size() < 2) return;
        
        // Update progress through current segment
        waypointFlight.progress += waypointFlight.speed * deltaTime;
        
        // If we've completed the current segment, move to next
        if (waypointFlight.progress >= 1.0f) {
            waypointFlight.progress = 0.0f;
            waypointFlight.currentSegment++;
            
            // Check if we've reached the end
            if (waypointFlight.currentSegment >= static_cast<int>(waypoints.size()) - 1) {
                if (waypointFlight.loop) {
                    waypointFlight.currentSegment = 0; // Loop back to start
                } else {
                    waypointFlight.enabled = false; // Stop at end
                    return;
                }
            }
        }
        
        // Interpolate position between current and next waypoint
        if (waypointFlight.currentSegment < static_cast<int>(waypoints.size()) - 1) {
            const Waypoint& current = waypoints[waypointFlight.currentSegment];
            const Waypoint& next = waypoints[waypointFlight.currentSegment + 1];
            
            // Linear interpolation in 4D space
            Event4D interpolatedPos;
            interpolatedPos.t = current.position.t + (next.position.t - current.position.t) * waypointFlight.progress;
            interpolatedPos.x = current.position.x + (next.position.x - current.position.x) * waypointFlight.progress;
            interpolatedPos.y = current.position.y + (next.position.y - current.position.y) * waypointFlight.progress;
            interpolatedPos.z = current.position.z + (next.position.z - current.position.z) * waypointFlight.progress;
            
            // Update camera to follow the interpolated path
            camera = Camera4D(interpolatedPos, 
                            Event4D(interpolatedPos.t + 1.0, 
                                   interpolatedPos.x, 
                                   interpolatedPos.y, 
                                   interpolatedPos.z));
        }
    }

    void UI4D::startWaypointFlight() {
        if (waypoints.size() >= 2) {
            waypointFlight.enabled = true;
            waypointFlight.progress = 0.0f;
            waypointFlight.currentSegment = 0;
            std::cout << "Started waypoint flight" << std::endl;
        }
    }

    void UI4D::stopWaypointFlight() {
        waypointFlight.enabled = false;
        std::cout << "Stopped waypoint flight" << std::endl;
    }

    void UI4D::setWaypointFlightSpeed(float speed) {
        waypointFlight.speed = speed;
        std::cout << "Waypoint flight speed set to: " << speed << std::endl;
    }

    void UI4D::setWaypointFlightLoop(bool loop) {
        waypointFlight.loop = loop;
        std::cout << "Waypoint flight loop set to: " << (loop ? "true" : "false") << std::endl;
    }

    void UI4D::addSingularity(std::shared_ptr<SingularityHandler> singularity) {
       singularities.push_back(singularity);
       std::cout << "Added singularity for anomaly detection" << std::endl;
   }

   void UI4D::detectCurvatureAnomalies() {
        if (!anomalyDetectionEnabled || !currentMetric) return;

        // Sample curvature at various points in the scene
        const int sampleGridSize = 10;
        const float sampleRange = 50.0f;
        const float step = sampleRange * 2.0f / (sampleGridSize - 1);

        for (int i = 0; i < sampleGridSize; i++) {
            for (int j = 0; j < sampleGridSize; j++) {
                for (int k = 0; k < sampleGridSize; k++) {
                    float x = -sampleRange + i * step;
                    float y = -sampleRange + j * step;
                    float z = -sampleRange + k * step;
                    
                    Event4D event(0.0, x, y, z); // Sample at t=0 for simplicity
                    
                    // Calculate curvature
                    double curvature = 0.0;
                    if (!singularities.empty()) {
                        for (const auto& singularity : singularities) {
                            const auto& pos = singularity->getPosition();
                            double dx = event.x - pos[0];
                            double dy = event.y - pos[1];
                            double dz = event.z - pos[2];
                            double r = std::sqrt(dx * dx + dy * dy + dz * dz);
                            double rs = singularity->getProperties().schwarzschild_radius;
                            if (r > rs * 1.1) {
                                curvature += 48.0 * rs * rs / (r * r * r * r * r * r);
                            }
                        }
                    }
                    
                // Check if anomaly detected
                    if (curvature > anomalyDetectionThreshold) {
                        // Check if we already have an alert for this area
                        bool duplicate = false;
                        for (const auto& alert : anomalyAlerts) {
                            double dx = alert.position.x - x;
                            double dy = alert.position.y - y;
                            double dz = alert.position.z - z;
                            double dist = std::sqrt(dx*dx + dy*dy + dz*dz);
                            if (dist < 5.0) { // Within 5 meters
                                duplicate = true;
                                break;
                            }
                        }
                        
                        if (!duplicate) {
                            AnomalyAlert alert;
                            alert.position = event;
                            alert.curvatureValue = curvature;
                            alert.description = "High curvature anomaly detected";
                            alert.isAcknowledged = false;
                            alert.timestamp = 0.0; // Would be set to current time in real implementation
                            anomalyAlerts.push_back(alert);
                            std::cout << "Anomaly detected at (" << x << ", " << y << ", " << z << ") with curvature " << curvature << std::endl;
                        }
                    }
                }
            }
        }
    }

    void UI4D::acknowledgeAnomaly(int index) {
        if (index >= 0 && index < static_cast<int>(anomalyAlerts.size())) {
            anomalyAlerts[index].isAcknowledged = true;
            std::cout << "Acknowledged anomaly at index: " << index << std::endl;
        }
    }

    void UI4D::clearAnomalies() {
        anomalyAlerts.clear();
        std::cout << "Cleared all anomaly alerts" << std::endl;
    }

    void UI4D::setAnomalyDetectionThreshold(double threshold) {
        anomalyDetectionThreshold = threshold;
        std::cout << "Anomaly detection threshold set to: " << threshold << std::endl;
    }

    void UI4D::toggleAnomalyDetection(bool enable) {
        anomalyDetectionEnabled = enable;
        std::cout << "Anomaly detection " << (enable ? "enabled" : "disabled") << std::endl;
    }

     // Spacetime curvature manipulation methods
     // ============================================================================
     
     void UI4D::activateCurvatureManipulation(const Event4D& position, CurvatureManipulationTool::Type type) {
         curvatureTool.active = true;
         curvatureTool.center = position;
         curvatureTool.type = type;
         std::cout << "Activated curvature manipulation at (" 
                   << position.t << ", " << position.x << ", " 
                   << position.y << ", " << position.z << ") with type: " 
                   << (type == CurvatureManipulationTool::POSITIVE ? "positive" : 
                       type == CurvatureManipulationTool::NEGATIVE ? "negative" : "wave") 
                   << std::endl;
     }
     
     void UI4D::deactivateCurvatureManipulation() {
         curvatureTool.active = false;
         std::cout << "Deactivated curvature manipulation" << std::endl;
     }
     
     void UI4D::setCurvatureManipulationRadius(float radius) {
         curvatureTool.radius = radius;
         std::cout << "Set curvature manipulation radius to: " << radius << std::endl;
     }
     
     void UI4D::setCurvatureManipulationStrength(float strength) {
         curvatureTool.strength = strength;
         std::cout << "Set curvature manipulation strength to: " << strength << std::endl;
     }
     
     // Gravitational lensing visualization methods
     // ============================================================================
     
     void UI4D::enableLensingEffect(bool enable) {
         lensingEffect.enabled = enable;
         std::cout << "Gravitational lensing effect " << (enable ? "enabled" : "disabled") << std::endl;
     }
     
     void UI4D::setLensingNumRings(int numRings) {
         lensingEffect.numRings = numRings;
         std::cout << "Set lensing number of rings to: " << numRings << std::endl;
     }
     
     void UI4D::setLensingRingSpacing(float spacing) {
         lensingEffect.ringSpacing = spacing;
         std::cout << "Set lensing ring spacing to: " << spacing << std::endl;
     }
     
     void UI4D::setLensingIntensity(float intensity) {
         lensingEffect.intensity = intensity;
         std::cout << "Set lensing intensity to: " << intensity << std::endl;
     }
     
     // Time dilation visualization methods
     // ============================================================================
     
     void UI4D::enableTimeDilationEffect(bool enable) {
         timeDilationEffect.enabled = enable;
         std::cout << "Time dilation effect " << (enable ? "enabled" : "disabled") << std::endl;
     }
     
     void UI4D::setTimeDilationRange(float minDilation, float maxDilation) {
         timeDilationEffect.minDilation = minDilation;
         timeDilationEffect.maxDilation = maxDilation;
         std::cout << "Set time dilation range to: [" << minDilation << ", " << maxDilation << "]" << std::endl;
     }
     
     void UI4D::setTimeDilationShowGrid(bool showGrid) {
         timeDilationEffect.showGrid = showGrid;
         std::cout << "Set time dilation show grid to: " << (showGrid ? "true" : "false") << std::endl;
     }
     
     // Quantum foam visualization methods
     // ============================================================================
     
     void UI4D::enableQuantumFoamEffect(bool enable) {
         quantumFoamEffect.enabled = enable;
         std::cout << "Quantum foam effect " << (enable ? "enabled" : "disabled") << std::endl;
     }
     
     void UI4D::setQuantumFoamScale(float scale) {
         quantumFoamEffect.scale = scale;
         std::cout << "Set quantum foam scale to: " << scale << std::endl;
     }
     
     void UI4D::setQuantumFoamFluctuationCount(int count) {
         quantumFoamEffect.fluctuationCount = count;
         std::cout << "Set quantum foam fluctuation count to: " << count << std::endl;
     }
     
     void UI4D::setQuantumFoamLifetime(float lifetime) {
         quantumFoamEffect.lifetime = lifetime;
         std::cout << "Set quantum foam lifetime to: " << lifetime << std::endl;
     }
     
     void UI4D::setQuantumFoamEnergy(float energy) {
         quantumFoamEffect.energy = energy;
         std::cout << "Set quantum foam energy to: " << energy << std::endl;
     }
     
     // Solar system specific methods implementation
     // ============================================================================

    void UI4D::initializeSolarSystem() {
    solarSystem.barycenter = Event4D(0.0, 0.0, 0.0, 0.0);
    solarSystem.scaleFactor = 1.0;

    // Load config from file or use defaults
    auto& config = ConfigLoader::instance();
    if (!config.isLoaded()) {
        config.loadFromFile("config/simulator.json");
    }

    const double AU = 149597870700.0;
    const double DAY = 24.0 * 3600.0;

    // Central body (Sun) from config
    {
        const auto& cfg = config.config().central_body;
        SolarSystemBody body;
        body.name = cfg.name.empty() ? "Sun" : cfg.name;
        body.mass = cfg.mass_kg;
        body.radius = cfg.radius_m;
        body.position = Event4D(0.0, 0.0, 0.0, 0.0);
        body.velocity = Event4D(0.0, 0.0, 0.0, 0.0);
        body.orbitalPeriod = 0.0;
        body.semiMajorAxis = 0.0;
        body.isCentralBody = true;
        body.isStar = cfg.is_star;
        body.showOrbit = false;
        body.textureId = cfg.name + "_texture";
        solarSystem.bodies[body.name] = body;
    }

    // Planets from config
    for (const auto& cfg : config.config().bodies) {
        SolarSystemBody body;
        body.name = cfg.name;
        body.mass = cfg.mass_kg;
        body.radius = cfg.radius_m;
        body.semiMajorAxis = cfg.semi_major_axis_au * AU;
        body.orbitalPeriod = cfg.orbital_period_days * DAY;

        double orbitalVelocity = 0.0;
        if (body.semiMajorAxis > 0.0) {
            double centralMass = config.config().central_body.mass_kg;
            orbitalVelocity = std::sqrt((PHYS_G() * centralMass) / body.semiMajorAxis);
        }
        body.position = Event4D(0.0, body.semiMajorAxis, 0.0, 0.0);
        body.velocity = Event4D(0.0, 0.0, 0.0, -orbitalVelocity);
        body.isCentralBody = false;
        body.isStar = false;
        body.showOrbit = true;
        body.textureId = cfg.name + "_texture";
        solarSystem.bodies[body.name] = body;
    }

    // Calculate initial orbit points
    for (auto& bodyPair : solarSystem.bodies) {
        calculateOrbitalTrajectory(bodyPair.second, 50);
    }

    std::cout << "[SolarSystem] Initialized " << solarSystem.bodies.size()
              << " bodies from config" << std::endl;
}

void UI4D::updateSolarSystemPositions(double currentTime) {
    // N-Body Leapfrog Integration (kept for backward compatibility)
    double dt = 1.0;
    stepSimulation(dt, 100);
}

void UI4D::stepSimulation(double dt, int substeps) {
    if (substeps < 1) substeps = 1;

    // Adaptive time stepping: calculate max acceleration to determine substep count
    double maxAccel = computeMaxAcceleration();
    
    // Target: each substep should move bodies no more than a fraction of the softening length
    // Higher acceleration → more substeps needed for stability
    double adaptiveSubsteps = substeps;
    
    if (maxAccel > 0.0) {
        // Courant-like condition: dt < sqrt(softening / maxAccel)
        double softening = 1e6; // 1000 km softening (matches force calculation)
        double stableDt = 0.1 * std::sqrt(softening / maxAccel);
        adaptiveSubsteps = std::max(1.0, std::ceil(dt / stableDt));
        // Cap at 1000 substeps to prevent freezing
        adaptiveSubsteps = std::min(adaptiveSubsteps, 1000.0);
    }
    
    substeps = static_cast<int>(adaptiveSubsteps);
    double sub_dt = dt / substeps;

    // Collect bodies into a vector for indexed access
    std::vector<SolarSystemBody*> bodyPtrs;
    bodyPtrs.reserve(solarSystem.bodies.size());
    for (auto& pair : solarSystem.bodies) {
        bodyPtrs.push_back(&pair.second);
    }
    int n = static_cast<int>(bodyPtrs.size());

    for (int step = 0; step < substeps; ++step) {
        // Use GPU acceleration when body count exceeds threshold and GPU is available
        if (gpuAccel.enabled && n > gpuAccel.gpuThreshold) {
            stepSimulationGPU(sub_dt, bodyPtrs, n);
        } else {
            stepSimulationCPU(sub_dt, bodyPtrs, n);
        }
    }

    // Store telemetry for live visualization
    lastTelemetry_ = getTelemetry();

    // Record frame if recording
    recordFrame();
}

double UI4D::computeMaxAcceleration() const {
    double maxAccel = 0.0;
    
    std::vector<const SolarSystemBody*> ptrs;
    ptrs.reserve(solarSystem.bodies.size());
    for (auto& pair : solarSystem.bodies) {
        ptrs.push_back(&pair.second);
    }
    int n = static_cast<int>(ptrs.size());
    
    for (int i = 0; i < n; ++i) {
        if (ptrs[i]->isCentralBody) continue;
        
        double acc[3] = {0.0, 0.0, 0.0};
        for (int j = 0; j < n; ++j) {
            if (i == j) continue;
            
            double dx = ptrs[j]->position.x - ptrs[i]->position.x;
            double dy = ptrs[j]->position.y - ptrs[i]->position.y;
            double dz = ptrs[j]->position.z - ptrs[i]->position.z;
            double r2 = dx * dx + dy * dy + dz * dz;
            double softening = 1e6;
            double r_soft = std::sqrt(r2 + softening * softening);
            double a_mag = PHYS_G() * ptrs[j]->mass / (r_soft * r_soft);
            
            acc[0] += a_mag * (dx / r_soft);
            acc[1] += a_mag * (dy / r_soft);
            acc[2] += a_mag * (dz / r_soft);
        }
        
        double a_total = std::sqrt(acc[0] * acc[0] + acc[1] * acc[1] + acc[2] * acc[2]);
        if (a_total > maxAccel) maxAccel = a_total;
    }
    
    return maxAccel;
}

// --- Telemetry Recording & Replay ---

void UI4D::startRecording(int maxFrames, int frameInterval) {
    stopRecording();
    recordingState.frames.clear();
    recordingState.maxFrames = maxFrames;
    recordingState.frameInterval = frameInterval;
    recordingState.frameCounter = 0;
    recordingState.currentFrame = 0;
    recordingState.recording = true;
    recordingState.playing = false;
    std::cout << "[Telemetry] Recording started (max " << maxFrames << " frames, interval " << frameInterval << ")" << std::endl;
}

void UI4D::stopRecording() {
    if (recordingState.recording) {
        recordingState.recording = false;
        std::cout << "[Telemetry] Recording stopped (" << recordingState.frames.size() << " frames captured)" << std::endl;
    }
}

void UI4D::recordFrame() {
    if (!recordingState.recording) return;
    
    recordingState.frameCounter++;
    if (recordingState.frameCounter % recordingState.frameInterval != 0) return;
    
    if (static_cast<int>(recordingState.frames.size()) >= recordingState.maxFrames) {
        recordingState.recording = false;
        std::cout << "[Telemetry] Recording buffer full" << std::endl;
        return;
    }
    
    TelemetryFrame frame;
    frame.time = recordingState.frames.size() * recordingState.frameInterval / 60.0; // ~60fps
    
    auto tel = getTelemetry();
    frame.totalEnergy = tel.totalEnergy;
    frame.kineticEnergy = tel.totalKineticEnergy;
    frame.potentialEnergy = tel.totalPotentialEnergy;
    
    for (auto& pair : solarSystem.bodies) {
        auto& body = pair.second;
        frame.names.push_back(body.name);
        frame.positions.push_back(body.position.x);
        frame.positions.push_back(body.position.y);
        frame.positions.push_back(body.position.z);
        frame.velocities.push_back(body.velocity.x);
        frame.velocities.push_back(body.velocity.y);
        frame.velocities.push_back(body.velocity.z);
    }
    
    recordingState.frames.push_back(frame);
}

void UI4D::playRecording() {
    if (recordingState.frames.empty()) return;
    recordingState.playing = true;
    recordingState.recording = false;
    recordingState.currentFrame = 0;
    std::cout << "[Telemetry] Playback started (" << recordingState.frames.size() << " frames)" << std::endl;
}

void UI4D::pausePlayback() {
    recordingState.playing = false;
}

void UI4D::stopPlayback() {
    recordingState.playing = false;
    recordingState.currentFrame = 0;
}

void UI4D::scrubToFrame(int frameIndex) {
    if (recordingState.frames.empty()) return;
    recordingState.currentFrame = std::max(0, std::min(frameIndex, static_cast<int>(recordingState.frames.size()) - 1));
    applyFrame(recordingState.currentFrame);
}

void UI4D::scrubToTime(double time) {
    if (recordingState.frames.empty()) return;
    for (int i = 0; i < static_cast<int>(recordingState.frames.size()); ++i) {
        if (recordingState.frames[i].time >= time) {
            scrubToFrame(i);
            return;
        }
    }
}

double UI4D::recordingDuration() const {
    if (recordingState.frames.empty()) return 0.0;
    return recordingState.frames.back().time;
}

void UI4D::exportToCSV(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[Telemetry] Failed to open " << filepath << " for export" << std::endl;
        return;
    }
    
    // Header
    file << "frame,time";
    if (!recordingState.frames.empty()) {
        for (auto& name : recordingState.frames[0].names) {
            file << "," << name << "_x," << name << "_y," << name << "_z";
            file << "," << name << "_vx," << name << "_vy," << name << "_vz";
        }
    }
    file << ",total_energy,kinetic_energy,potential_energy\n";
    
    // Data
    for (int i = 0; i < static_cast<int>(recordingState.frames.size()); ++i) {
        auto& frame = recordingState.frames[i];
        file << i << "," << frame.time;
        for (double v : frame.positions) file << "," << v;
        for (double v : frame.velocities) file << "," << v;
        file << "," << frame.totalEnergy << "," << frame.kineticEnergy << "," << frame.potentialEnergy << "\n";
    }
    
    file.close();
    std::cout << "[Telemetry] Exported " << recordingState.frames.size() << " frames to " << filepath << std::endl;
}

void UI4D::clearRecording() {
    recordingState.frames.clear();
    recordingState.currentFrame = 0;
    recordingState.recording = false;
    recordingState.playing = false;
}

void UI4D::applyFrame(int frameIndex) {
    if (frameIndex < 0 || frameIndex >= static_cast<int>(recordingState.frames.size())) return;
    
    auto& frame = recordingState.frames[frameIndex];
    
    // Build name lookup for current bodies
    std::unordered_map<std::string, SolarSystemBody*> bodyMap;
    for (auto& pair : solarSystem.bodies) {
        bodyMap[pair.first] = &pair.second;
    }
    
    // Apply recorded state
    for (int i = 0; i < static_cast<int>(frame.names.size()); ++i) {
        auto it = bodyMap.find(frame.names[i]);
        if (it == bodyMap.end()) continue;
        
        auto* body = it->second;
        body->position.x = frame.positions[i * 3 + 0];
        body->position.y = frame.positions[i * 3 + 1];
        body->position.z = frame.positions[i * 3 + 2];
        body->velocity.x = frame.velocities[i * 3 + 0];
        body->velocity.y = frame.velocities[i * 3 + 1];
        body->velocity.z = frame.velocities[i * 3 + 2];
    }
}

void UI4D::stepSimulationCPU(double sub_dt, std::vector<SolarSystemBody*>& bodyPtrs, int n) {
    // 1. Calculate accelerations for all bodies
    for (int i = 0; i < n; ++i) {
        SolarSystemBody* bi = bodyPtrs[i];
        bi->acc[0] = 0.0;
        bi->acc[1] = 0.0;
        bi->acc[2] = 0.0;

        if (bi->isCentralBody) continue;

        for (int j = 0; j < n; ++j) {
            if (i == j) continue;
            SolarSystemBody* bj = bodyPtrs[j];

            double dx = bj->position.x - bi->position.x;
            double dy = bj->position.y - bi->position.y;
            double dz = bj->position.z - bi->position.z;
            double r2 = dx * dx + dy * dy + dz * dz;

            double softening = 1e6;
            double r_soft = std::sqrt(r2 + softening * softening);
            double a_mag = PHYS_G() * bj->mass / (r_soft * r_soft);

            bi->acc[0] += a_mag * (dx / r_soft);
            bi->acc[1] += a_mag * (dy / r_soft);
            bi->acc[2] += a_mag * (dz / r_soft);
            
            // Lense-Thirring precession (frame-dragging) from rotating central body
            // Only apply when the other body is the central (massive) body
            if (bj->isCentralBody && m_kerrSpin > 1e-10) {
                double G = PHYS_G();
                double c2 = PHYS_C2();
                double J = m_kerrSpin * G * bj->mass * bj->mass; // J = a*GM²/c (dimensionless spin)
                
                double r = std::sqrt(r2);
                if (r > softening) {
                    // Lense-Thirring acceleration: a_LT = (2GJ/c²r³) * [3(r·v)r/r² - v×(Ĵ×r̂)]
                    // Simplified: assume J along z-axis
                    double vx = bi->velocity.x;
                    double vy = bi->velocity.y;
                    double vz = bi->velocity.z;
                    
                    double r_dot_v = dx * vx + dy * vy + dz * vz;
                    double r_hat_x = dx / r;
                    double r_hat_y = dy / r;
                    double r_hat_z = dz / r;
                    
                    // Ĵ × r̂ (J along z)
                    double J_cross_r_x = -r_hat_y;
                    double J_cross_r_y = r_hat_x;
                    double J_cross_r_z = 0.0;
                    
                    // v × (Ĵ × r̂)
                    double v_cross_Jr_x = vy * J_cross_r_z - vz * J_cross_r_y;
                    double v_cross_Jr_y = vz * J_cross_r_x - vx * J_cross_r_z;
                    double v_cross_Jr_z = vx * J_cross_r_y - vy * J_cross_r_x;
                    
                    // 3(r·v)r/r² - v×(Ĵ×r̂)
                    double factor = 2.0 * G * J / (c2 * r * r * r);
                    bi->acc[0] += factor * (3.0 * r_dot_v * r_hat_x / r2 - v_cross_Jr_x);
                    bi->acc[1] += factor * (3.0 * r_dot_v * r_hat_y / r2 - v_cross_Jr_y);
                    bi->acc[2] += factor * (3.0 * r_dot_v * r_hat_z / r2 - v_cross_Jr_z);
                }
            }
        }
    }

    // 2. Leapfrog Kick-Drift-Kick
    for (int i = 0; i < n; ++i) {
        if (bodyPtrs[i]->isCentralBody) continue;
        bodyPtrs[i]->velocity.x += bodyPtrs[i]->acc[0] * sub_dt * 0.5;
        bodyPtrs[i]->velocity.y += bodyPtrs[i]->acc[1] * sub_dt * 0.5;
        bodyPtrs[i]->velocity.z += bodyPtrs[i]->acc[2] * sub_dt * 0.5;
    }

    for (int i = 0; i < n; ++i) {
        if (bodyPtrs[i]->isCentralBody) continue;
        bodyPtrs[i]->position.x += bodyPtrs[i]->velocity.x * sub_dt;
        bodyPtrs[i]->position.y += bodyPtrs[i]->velocity.y * sub_dt;
        bodyPtrs[i]->position.z += bodyPtrs[i]->velocity.z * sub_dt;
        bodyPtrs[i]->position.t += sub_dt;
    }

    for (int i = 0; i < n; ++i) {
        SolarSystemBody* bi = bodyPtrs[i];
        bi->acc[0] = 0.0;
        bi->acc[1] = 0.0;
        bi->acc[2] = 0.0;

        if (bi->isCentralBody) continue;

        for (int j = 0; j < n; ++j) {
            if (i == j) continue;
            SolarSystemBody* bj = bodyPtrs[j];

            double dx = bj->position.x - bi->position.x;
            double dy = bj->position.y - bi->position.y;
            double dz = bj->position.z - bi->position.z;
            double r2 = dx * dx + dy * dy + dz * dz;
            double softening = 1e6;
            double r_soft = std::sqrt(r2 + softening * softening);
            double a_mag = PHYS_G() * bj->mass / (r_soft * r_soft);

            bi->acc[0] += a_mag * (dx / r_soft);
            bi->acc[1] += a_mag * (dy / r_soft);
            bi->acc[2] += a_mag * (dz / r_soft);
        }
    }

    for (int i = 0; i < n; ++i) {
        if (bodyPtrs[i]->isCentralBody) continue;
        bodyPtrs[i]->velocity.x += bodyPtrs[i]->acc[0] * sub_dt * 0.5;
        bodyPtrs[i]->velocity.y += bodyPtrs[i]->acc[1] * sub_dt * 0.5;
        bodyPtrs[i]->velocity.z += bodyPtrs[i]->acc[2] * sub_dt * 0.5;
    }
}

void UI4D::stepSimulationGPU(double sub_dt, std::vector<SolarSystemBody*>& bodyPtrs, int n) {
#if HAS_GLAD
    // Ensure GPU resources are initialized
    if (!gpuAccel.initialized || n > static_cast<int>(gpuAccel.maxBodies)) {
        initGPUAcceleration();
        if (!gpuAccel.initialized || n > static_cast<int>(gpuAccel.maxBodies)) {
            // Fallback to CPU if GPU init fails
            stepSimulationCPU(sub_dt, bodyPtrs, n);
            return;
        }
    }

    // Find central body index
    uint32_t centralIdx = 0;
    for (uint32_t i = 0; i < static_cast<uint32_t>(n); ++i) {
        if (bodyPtrs[i]->isCentralBody) {
            centralIdx = i;
            break;
        }
    }

    // Upload positions and masses to SSBOs
    gpuAccel.positionData.resize(n * 4);
    gpuAccel.massData.resize(n);
    gpuAccel.accelerationData.resize(n * 4);

    for (int i = 0; i < n; ++i) {
        gpuAccel.positionData[i * 4 + 0] = static_cast<float>(bodyPtrs[i]->position.x);
        gpuAccel.positionData[i * 4 + 1] = static_cast<float>(bodyPtrs[i]->position.y);
        gpuAccel.positionData[i * 4 + 2] = static_cast<float>(bodyPtrs[i]->position.z);
        gpuAccel.positionData[i * 4 + 3] = 0.0f;
        gpuAccel.massData[i] = static_cast<float>(bodyPtrs[i]->mass);
    }

    // Upload data
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gpuAccel.ssboPositions);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, n * 4 * sizeof(float), gpuAccel.positionData.data());

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gpuAccel.ssboMasses);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, n * sizeof(float), gpuAccel.massData.data());

    // Bind SSBOs to compute shader binding points
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gpuAccel.ssboPositions);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, gpuAccel.ssboMasses);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, gpuAccel.ssboAccelerations);

    // Set uniforms and dispatch
    gpuAccel.computeShader->use();
    gpuAccel.computeShader->setUniform("G", static_cast<float>(PHYS_G()));
    gpuAccel.computeShader->setUniform("softening", 1e6f);
    gpuAccel.computeShader->setUniform("bodyCount", static_cast<uint32_t>(n));
    gpuAccel.computeShader->setUniform("centralBodyIndex", centralIdx);

    uint32_t numGroups = (n + 255) / 256;
    gpuAccel.computeShader->dispatch(numGroups, 1, 1);

    // Ensure compute shader completes before reading back
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Read back accelerations
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gpuAccel.ssboAccelerations);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, n * 4 * sizeof(float), gpuAccel.accelerationData.data());

    // Store accelerations
    for (int i = 0; i < n; ++i) {
        bodyPtrs[i]->acc[0] = gpuAccel.accelerationData[i * 4 + 0];
        bodyPtrs[i]->acc[1] = gpuAccel.accelerationData[i * 4 + 1];
        bodyPtrs[i]->acc[2] = gpuAccel.accelerationData[i * 4 + 2];
    }

    // Leapfrog Kick-Drift-Kick (on CPU)
    for (int i = 0; i < n; ++i) {
        if (bodyPtrs[i]->isCentralBody) continue;
        bodyPtrs[i]->velocity.x += bodyPtrs[i]->acc[0] * sub_dt * 0.5;
        bodyPtrs[i]->velocity.y += bodyPtrs[i]->acc[1] * sub_dt * 0.5;
        bodyPtrs[i]->velocity.z += bodyPtrs[i]->acc[2] * sub_dt * 0.5;
    }

    for (int i = 0; i < n; ++i) {
        if (bodyPtrs[i]->isCentralBody) continue;
        bodyPtrs[i]->position.x += bodyPtrs[i]->velocity.x * sub_dt;
        bodyPtrs[i]->position.y += bodyPtrs[i]->velocity.y * sub_dt;
        bodyPtrs[i]->position.z += bodyPtrs[i]->velocity.z * sub_dt;
        bodyPtrs[i]->position.t += sub_dt;
    }

    // Second GPU dispatch for new positions
    for (int i = 0; i < n; ++i) {
        gpuAccel.positionData[i * 4 + 0] = static_cast<float>(bodyPtrs[i]->position.x);
        gpuAccel.positionData[i * 4 + 1] = static_cast<float>(bodyPtrs[i]->position.y);
        gpuAccel.positionData[i * 4 + 2] = static_cast<float>(bodyPtrs[i]->position.z);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gpuAccel.ssboPositions);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, n * 4 * sizeof(float), gpuAccel.positionData.data());

    gpuAccel.computeShader->use();
    gpuAccel.computeShader->setUniform("G", static_cast<float>(PHYS_G()));
    gpuAccel.computeShader->setUniform("softening", 1e6f);
    gpuAccel.computeShader->setUniform("bodyCount", static_cast<uint32_t>(n));
    gpuAccel.computeShader->setUniform("centralBodyIndex", centralIdx);

    gpuAccel.computeShader->dispatch(numGroups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gpuAccel.ssboAccelerations);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, n * 4 * sizeof(float), gpuAccel.accelerationData.data());

    for (int i = 0; i < n; ++i) {
        bodyPtrs[i]->acc[0] = gpuAccel.accelerationData[i * 4 + 0];
        bodyPtrs[i]->acc[1] = gpuAccel.accelerationData[i * 4 + 1];
        bodyPtrs[i]->acc[2] = gpuAccel.accelerationData[i * 4 + 2];
    }

    for (int i = 0; i < n; ++i) {
        if (bodyPtrs[i]->isCentralBody) continue;
        bodyPtrs[i]->velocity.x += bodyPtrs[i]->acc[0] * sub_dt * 0.5;
        bodyPtrs[i]->velocity.y += bodyPtrs[i]->acc[1] * sub_dt * 0.5;
        bodyPtrs[i]->velocity.z += bodyPtrs[i]->acc[2] * sub_dt * 0.5;
    }
#else
    // No OpenGL available, fallback to CPU
    stepSimulationCPU(sub_dt, bodyPtrs, n);
#endif
}

void UI4D::initGPUAcceleration() {
#if HAS_GLAD
    if (gpuAccel.initialized) return;

    if (!ComputeShader::isComputeSupported()) {
        std::cout << "[GPU] Compute shaders not supported on this GPU" << std::endl;
        gpuAccel.enabled = false;
        return;
    }

    gpuAccel.computeShader = new ComputeShader();

    // Try loading from file first, then fallback to embedded source
    const char* embeddedSource = R"(
#version 430 core

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) readonly buffer PositionBuffer {
    vec4 positions[];
};

layout(std430, binding = 1) readonly buffer MassBuffer {
    float masses[];
};

layout(std430, binding = 2) writeonly buffer AccelerationBuffer {
    vec4 accelerations[];
};

uniform float G;
uniform float softening;
uniform uint bodyCount;
uniform uint centralBodyIndex;

void main() {
    uint i = gl_GlobalInvocationID.x;
    if (i >= bodyCount) return;

    if (i == centralBodyIndex) {
        accelerations[i] = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }

    vec3 pos_i = positions[i].xyz;
    vec3 acc = vec3(0.0);

    for (uint j = 0u; j < bodyCount; j++) {
        if (j == i) continue;

        vec3 pos_j = positions[j].xyz;
        vec3 diff = pos_j - pos_i;

        float r2 = dot(diff, diff);
        float r_soft = sqrt(r2 + softening * softening);

        float m_j = masses[j];
        float a_mag = G * m_j / (r_soft * r_soft * r_soft);

        acc += a_mag * diff;
    }

    accelerations[i] = vec4(acc, 0.0);
}
)";

    std::string shaderPath = "data/shaders/nbody.comp";
    bool loaded = gpuAccel.computeShader->loadFromFile(shaderPath);
    if (!loaded) {
        loaded = gpuAccel.computeShader->loadFromSource(embeddedSource);
    }

    if (!loaded) {
        std::cout << "[GPU] Failed to compile compute shader" << std::endl;
        delete gpuAccel.computeShader;
        gpuAccel.computeShader = nullptr;
        gpuAccel.enabled = false;
        return;
    }

    // Create SSBOs with capacity for up to 4096 bodies
    gpuAccel.maxBodies = 4096;

    glGenBuffers(1, &gpuAccel.ssboPositions);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gpuAccel.ssboPositions);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gpuAccel.maxBodies * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &gpuAccel.ssboMasses);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gpuAccel.ssboMasses);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gpuAccel.maxBodies * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &gpuAccel.ssboAccelerations);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, gpuAccel.ssboAccelerations);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gpuAccel.maxBodies * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    gpuAccel.positionData.resize(gpuAccel.maxBodies * 4);
    gpuAccel.massData.resize(gpuAccel.maxBodies);
    gpuAccel.accelerationData.resize(gpuAccel.maxBodies * 4);

    gpuAccel.initialized = true;
    gpuAccel.enabled = true;

    std::cout << "[GPU] N-body acceleration initialized (max " << gpuAccel.maxBodies << " bodies)" << std::endl;
#else
    gpuAccel.enabled = false;
    gpuAccel.initialized = false;
#endif
}

void UI4D::shutdownGPUAcceleration() {
#if HAS_GLAD
    if (!gpuAccel.initialized) return;

    if (gpuAccel.ssboPositions != 0) {
        glDeleteBuffers(1, &gpuAccel.ssboPositions);
        gpuAccel.ssboPositions = 0;
    }
    if (gpuAccel.ssboMasses != 0) {
        glDeleteBuffers(1, &gpuAccel.ssboMasses);
        gpuAccel.ssboMasses = 0;
    }
    if (gpuAccel.ssboAccelerations != 0) {
        glDeleteBuffers(1, &gpuAccel.ssboAccelerations);
        gpuAccel.ssboAccelerations = 0;
    }

    delete gpuAccel.computeShader;
    gpuAccel.computeShader = nullptr;

    gpuAccel.initialized = false;
    gpuAccel.enabled = false;

    std::cout << "[GPU] N-body acceleration shut down" << std::endl;
#endif
}

void UI4D::calculateOrbitalTrajectory(const SolarSystemBody& body, int points) {
    auto it = solarSystem.bodies.find(body.name);
    if (it != solarSystem.bodies.end()) {
        SolarSystemBody& mutableBody = it->second;
        mutableBody.orbitPoints.clear();
        
        for (int i = 0; i < points; i++) {
            double phase = static_cast<double>(i) / points * 2.0 * M_PI;
            double x = body.semiMajorAxis * std::cos(phase);
            double y = 0.0;
            double z = body.semiMajorAxis * std::sin(phase);
            mutableBody.orbitPoints.push_back(Event4D(0.0, x, y, z));
        }
    }
}

UI4D::PhysicsTelemetry UI4D::getTelemetry() const {
    PhysicsTelemetry tel;
    tel.bodyCount = static_cast<int>(solarSystem.bodies.size());

    std::vector<const SolarSystemBody*> ptrs;
    ptrs.reserve(solarSystem.bodies.size());
    for (auto& pair : solarSystem.bodies) {
        ptrs.push_back(&pair.second);
    }
    int n = static_cast<int>(ptrs.size());

    // Kinetic energy: sum of 0.5 * m * v^2
    for (int i = 0; i < n; ++i) {
        double vx = ptrs[i]->velocity.x;
        double vy = ptrs[i]->velocity.y;
        double vz = ptrs[i]->velocity.z;
        double v2 = vx * vx + vy * vy + vz * vz;
        tel.totalKineticEnergy += 0.5 * ptrs[i]->mass * v2;
    }

    // Potential energy: sum of -G * m1 * m2 / r for all pairs
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            double dx = ptrs[j]->position.x - ptrs[i]->position.x;
            double dy = ptrs[j]->position.y - ptrs[i]->position.y;
            double dz = ptrs[j]->position.z - ptrs[i]->position.z;
            double r = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (r > 0.0) {
                tel.totalPotentialEnergy -= PHYS_G() * ptrs[i]->mass * ptrs[j]->mass / r;
            }
        }
    }

    tel.totalEnergy = tel.totalKineticEnergy + tel.totalPotentialEnergy;

    // Angular momentum (Z-component): sum of m * (x*vy - y*vx)
    for (int i = 0; i < n; ++i) {
        tel.angularMomentumZ += ptrs[i]->mass * (
            ptrs[i]->position.x * ptrs[i]->velocity.y -
            ptrs[i]->position.y * ptrs[i]->velocity.x
        );
    }

    // Earth-specific data
    auto it = solarSystem.bodies.find("Earth");
    if (it != solarSystem.bodies.end()) {
        auto& earth = it->second;
        double vx = earth.velocity.x;
        double vy = earth.velocity.y;
        double vz = earth.velocity.z;
        tel.earthOrbitalSpeed = std::sqrt(vx * vx + vy * vy + vz * vz);
        double dx = earth.position.x;
        double dz = earth.position.z;
        tel.earthDistance = std::sqrt(dx * dx + dz * dz);
    }

    // Gravitational wave strain (quadrupole approximation)
    // For a binary/multiple system, the characteristic strain is:
    // h ≈ (2G / c⁴r) * Q̈ where Q̈ is the second time derivative of quadrupole moment
    // For a circular orbit: h ≈ (4G/c⁴) * (μ * v²) / r_obs
    // where μ is reduced mass, v is orbital velocity, r_obs is observer distance
    {
        // Find central body (Sun) and compute chirp mass with Earth
        auto sunIt = solarSystem.bodies.find("Sun");
        auto earthIt = solarSystem.bodies.find("Earth");
        if (sunIt != solarSystem.bodies.end() && earthIt != solarSystem.bodies.end()) {
            auto& sun = sunIt->second;
            auto& earth = earthIt->second;

            double dx = earth.position.x - sun.position.x;
            double dy = earth.position.y - sun.position.y;
            double dz = earth.position.z - sun.position.z;
            double r = std::sqrt(dx * dx + dy * dy + dz * dz);

            if (r > 0.0) {
                // Reduced mass: μ = m1*m2 / (m1+m2)
                double mu = (sun.mass * earth.mass) / (sun.mass + earth.mass);

                // Orbital velocity of Earth
                double vx = earth.velocity.x - sun.velocity.x;
                double vy = earth.velocity.y - sun.velocity.y;
                double vz = earth.velocity.z - sun.velocity.z;
                double v2 = vx * vx + vy * vy + vz * vz;

                // Observer distance (use 1 AU as reference)
                double r_obs = 1.496e11; // 1 AU

                // Gravitational wave strain amplitude
                // h ≈ (4G / c⁴) * μ * v² / r_obs
                double G = PHYS_G();
                double c4 = PHYS_C2() * PHYS_C2();
                tel.gravitationalWaveStrain = (4.0 * G / c4) * mu * v2 / r_obs;

                // Gravitational wave frequency: f_gw = 2 * f_orb
                // f_orb = v / (2πr)
                double f_orbital = std::sqrt(v2) / (2.0 * M_PI * r);
                tel.gravitationalWaveFreq = 2.0 * f_orbital;

                // Chirp mass: Mc = (m1*m2)^(3/5) / (m1+m2)^(1/5)
                double m1 = sun.mass;
                double m2 = earth.mass;
                tel.chirpMass = std::pow(m1 * m2, 3.0/5.0) / std::pow(m1 + m2, 1.0/5.0);
            }
        }
    }

    return tel;
}

void UI4D::injectAsteroid(double x, double y, double z, double vx, double vy, double vz, double mass)
{
    SolarSystemBody asteroid;
    asteroid.name = "Asteroid_" + std::to_string(asteroidCount());
    asteroid.mass = mass;
    asteroid.radius = 1000.0; // 1 km default
    asteroid.position = Event4D(0.0, x, y, z);
    asteroid.velocity = Event4D(0.0, vx, vy, vz);
    asteroid.acc[0] = asteroid.acc[1] = asteroid.acc[2] = 0.0;
    asteroid.orbitalPeriod = 0.0;
    asteroid.semiMajorAxis = 0.0;
    asteroid.isCentralBody = false;
    asteroid.isStar = false;
    asteroid.showOrbit = false;
    asteroid.textureId = "asteroid_texture";
    solarSystem.bodies[asteroid.name] = asteroid;

    std::cout << "[Asteroid] Injected " << asteroid.name << " at (" << x << ", " << y << ", " << z << ")"
              << " v=(" << vx << ", " << vy << ", " << vz << ")"
              << " m=" << mass << " kg" << std::endl;
}

void UI4D::clearAsteroids()
{
    // Remove all bodies that are not the Sun or planets
    std::vector<std::string> toRemove;
    for (auto& pair : solarSystem.bodies) {
        if (pair.second.name.find("Asteroid_") == 0) {
            toRemove.push_back(pair.first);
        }
    }
    for (auto& name : toRemove) {
        solarSystem.bodies.erase(name);
    }
}

void UI4D::removeLastAsteroid()
{
    // Find the last injected asteroid (highest number)
    std::string lastAsteroid;
    int lastNum = -1;
    for (auto& pair : solarSystem.bodies) {
        if (pair.second.name.find("Asteroid_") == 0) {
            // Extract number from name
            std::string numStr = pair.second.name.substr(9); // After "Asteroid_"
            int num = std::stoi(numStr);
            if (num > lastNum) {
                lastNum = num;
                lastAsteroid = pair.first;
            }
        }
    }
    if (!lastAsteroid.empty()) {
        solarSystem.bodies.erase(lastAsteroid);
    }
}

int UI4D::asteroidCount() const
{
    int count = 0;
    for (auto& pair : solarSystem.bodies) {
        if (pair.second.name.find("Asteroid_") == 0) {
            count++;
        }
    }
    return count;
}

// ============================================================================
// Scenario System Implementation
// ============================================================================

bool UI4D::loadScenario(const Scenario& scenario) {
    std::cout << "[Scenario] Loading: " << scenario.name << std::endl;

    auto& state = scenarioState;
    state.active = true;
    state.playing = false;
    state.time = 0.0;
    state.time_scale = scenario.time_scale;
    state.next_event_index = 0;
    state.current_scenario = std::make_shared<Scenario>(scenario);

    clearAsteroids();

    if (scenario.physics.override_constants) {
        auto& pc = PhysicsConstants::instance();
        pc.set_G(scenario.physics.G);
        pc.set_c(scenario.physics.c);
        pc.set_h(scenario.physics.h);
        std::cout << "[Scenario] Physics constants overridden: G=" << scenario.physics.G
                  << " c=" << scenario.physics.c << " h=" << scenario.physics.h << std::endl;
    }

    const double AU = 149597870700.0;
    const double DAY = 24.0 * 3600.0;

    solarSystem.bodies.clear();

    {
        SolarSystemBody body;
        body.name = scenario.central_body.name;
        body.mass = scenario.central_body.mass_kg;
        body.radius = scenario.central_body.radius_m;
        body.position = Event4D(0.0, 0.0, 0.0, 0.0);
        body.velocity = Event4D(0.0, 0.0, 0.0, 0.0);
        body.orbitalPeriod = 0.0;
        body.semiMajorAxis = 0.0;
        body.isCentralBody = true;
        body.isStar = scenario.central_body.is_star;
        body.showOrbit = false;
        body.textureId = scenario.central_body.name + "_texture";
        solarSystem.bodies[body.name] = body;
    }

    for (const auto& cfg : scenario.bodies) {
        addBodyFromConfig(cfg);
    }

    for (auto& bodyPair : solarSystem.bodies) {
        calculateOrbitalTrajectory(bodyPair.second, 50);
    }

    if (!scenario.camera_path.empty()) {
        applyScenarioCamera();
    }

    applyScenarioAudio();

    std::cout << "[Scenario] Loaded " << solarSystem.bodies.size() << " bodies, "
              << scenario.events.size() << " events, "
              << scenario.camera_path.size() << " camera keyframes" << std::endl;

    return true;
}

void UI4D::playScenario() {
    if (!scenarioState.active) {
        std::cerr << "[Scenario] No scenario loaded" << std::endl;
        return;
    }
    scenarioState.playing = true;
    std::cout << "[Scenario] Playing: " << scenarioState.current_scenario->name << std::endl;
}

void UI4D::pauseScenario() {
    scenarioState.playing = false;
    std::cout << "[Scenario] Paused at t=" << scenarioState.time << "s" << std::endl;
}

void UI4D::stopScenario() {
    scenarioState.playing = false;
    scenarioState.time = 0.0;
    scenarioState.next_event_index = 0;
    std::cout << "[Scenario] Stopped" << std::endl;

    if (scenarioState.current_scenario) {
        loadScenario(*scenarioState.current_scenario);
    }
}

void UI4D::stepScenario(double dt) {
    if (!scenarioState.active || !scenarioState.playing) return;

    auto& state = scenarioState;
    double prev_time = state.time;
    state.time += dt * state.time_scale;

    if (state.current_scenario && state.time > state.current_scenario->duration) {
        state.time = state.current_scenario->duration;
        state.playing = false;
        std::cout << "[Scenario] Reached end of scenario at t=" << state.time << "s" << std::endl;
    }

    processScenarioEvents(prev_time, state.time);

    applyScenarioCamera();
}

void UI4D::processScenarioEvents(double prev_time, double curr_time) {
    if (!scenarioState.current_scenario) return;

    const auto& events = scenarioState.current_scenario->events;
    auto& idx = scenarioState.next_event_index;

    while (idx < static_cast<int>(events.size()) && events[idx].time <= curr_time) {
        if (events[idx].time >= prev_time) {
            executeScenarioEvent(events[idx]);
        }
        idx++;
    }
}

void UI4D::executeScenarioEvent(const ScenarioEvent& event) {
    std::cout << "[Scenario] Event at t=" << event.time << "s: "
              << eventTypeToString(event.type);

    switch (event.type) {
        case ScenarioEventType::INJECT_ASTEROID:
            injectAsteroidFromEvent(event);
            break;

        case ScenarioEventType::SET_CAMERA:
            std::cout << " (camera override)";
            break;

        case ScenarioEventType::SET_CONSTANT: {
            auto& pc = PhysicsConstants::instance();
            if (event.str_param == "G" || event.str_param == "all") {
                pc.set_G(event.params[0]);
            }
            if (event.str_param == "c" || event.str_param == "all") {
                pc.set_c(event.params[0]);
            }
            if (event.str_param == "h" || event.str_param == "all") {
                pc.set_h(event.params[0]);
            }
            std::cout << " " << event.str_param << "=" << event.params[0];
            break;
        }

        case ScenarioEventType::ADD_BODY:
            std::cout << " body=" << event.str_param;
            break;

        case ScenarioEventType::REMOVE_BODY: {
            auto it = solarSystem.bodies.find(event.str_param);
            if (it != solarSystem.bodies.end() && !it->second.isCentralBody) {
                solarSystem.bodies.erase(it);
                std::cout << " removed " << event.str_param;
            }
            break;
        }

        case ScenarioEventType::SET_TIME_SCALE:
            scenarioState.time_scale = event.params[0];
            std::cout << " timescale=" << event.params[0];
            break;

        case ScenarioEventType::AUDIO_SETTINGS:
            std::cout << " audio config";
            break;

        case ScenarioEventType::APPLY_FORCE:
            std::cout << " force on " << event.target_body;
            break;

        case ScenarioEventType::SET_CENTRAL_BODY:
            std::cout << " central_body=" << event.str_param;
            break;

        case ScenarioEventType::WAIT:
        default:
            break;
    }

    std::cout << std::endl;
}

void UI4D::applyScenarioCamera() {
    if (!scenarioState.current_scenario) return;

    const auto& path = scenarioState.current_scenario->camera_path;
    if (path.empty()) return;

     ScenarioCameraState state = interpolateCamera(path, scenarioState.time);
     setCameraFromState(state);
}

void UI4D::applyScenarioAudio() {
    if (!scenarioState.current_scenario) return;
    const auto& audio = scenarioState.current_scenario->audio;
    std::cout << "[Scenario] Audio: enabled=" << audio.enabled
              << " vol=" << audio.volume
              << " freq=" << audio.base_frequency << std::endl;
}

void UI4D::setCameraFromState(const ScenarioCameraState& state) {
    camera = Camera4D(
        Event4D(0.0, state.position[0], state.position[1], state.position[2]),
        Event4D(0.0, state.target[0], state.target[1], state.target[2]),
        state.fov * M_PI / 180.0
    );
}

void UI4D::injectAsteroidFromEvent(const ScenarioEvent& event) {
    double x = event.params[0];
    double y = event.params[1];
    double z = event.params[2];
    double vx = event.params[3];
    double vy = event.params[4];
    double vz = event.params[5];
    double mass = event.params[6] > 0.0 ? event.params[6] : 1e15;
    injectAsteroid(x, y, z, vx, vy, vz, mass);
}

void UI4D::addBodyFromConfig(const ScenarioBodyConfig& config) {
    const double AU = 149597870700.0;
    const double DAY = 24.0 * 3600.0;

    SolarSystemBody body;
    body.name = config.name;
    body.mass = config.mass_kg;
    body.radius = config.radius_m;
    body.semiMajorAxis = config.semi_major_axis_au * AU;
    body.orbitalPeriod = config.orbital_period_days * DAY;
    body.isCentralBody = config.is_central_body;
    body.isStar = config.is_star;
    body.showOrbit = !config.is_central_body;
    body.textureId = config.name + "_texture";

    if (body.semiMajorAxis > 0.0 && !config.is_central_body) {
        double centralMass = scenarioState.current_scenario ?
            scenarioState.current_scenario->central_body.mass_kg : 1.989e30;
        double orbitalVelocity = std::sqrt((PHYS_G() * centralMass) / body.semiMajorAxis);
        body.position = Event4D(0.0, body.semiMajorAxis, 0.0, 0.0);
        body.velocity = Event4D(0.0, 0.0, 0.0, -orbitalVelocity);
    } else {
        body.position = Event4D(0.0, config.position[0], config.position[1], config.position[2]);
        body.velocity = Event4D(0.0, config.velocity[0], config.velocity[1], config.velocity[2]);
    }

    solarSystem.bodies[body.name] = body;
}

void UI4D::renderSolarSystem() {
    if (!showSolarSystem) return;

    if (scenarioState.active && scenarioState.playing) {
        stepScenario(0.016);
    }

    // N-Body physics step (called from render at ~60fps)
    stepSimulation(0.016, 10); // 10 substeps of 1.6ms each
    
    for (const auto& bodyPair : solarSystem.bodies) {
        const SolarSystemBody& body = bodyPair.second;
        
        if (!showBodyMarkers) continue;
        
        Event4D scaledPos(
            body.position.t,
            body.position.x * solarSystem.scaleFactor,
            body.position.y * solarSystem.scaleFactor,
            body.position.z * solarSystem.scaleFactor
        );
        
        std::cout << "Rendering " << body.name << " at position: ("
                  << scaledPos.t << ", " << scaledPos.x << ", "
                  << scaledPos.y << ", " << scaledPos.z << ")" << std::endl;
        
        if (showOrbitalTrajectories && !body.orbitPoints.empty()) {
            std::cout << "Rendering orbit for " << body.name << " with "
                      << body.orbitPoints.size() << " points" << std::endl;
        }
        
        if (solarSystem.showLabels) {
            std::cout << "Label for " << body.name << " at position: ("
                      << scaledPos.t << ", " << scaledPos.x << ", "
                      << scaledPos.y << ", " << scaledPos.z << ")" << std::endl;
        }
    }
}

void UI4D::renderPlanetaryDataPanel(const std::string& bodyName) {
    if (!showDataPanels) return;
    
    auto it = solarSystem.bodies.find(bodyName);
    if (it == solarSystem.bodies.end()) {
        std::cout << "Body not found: " << bodyName << std::endl;
        return;
    }
    
    const SolarSystemBody& body = it->second;
    
    std::cout << "=== " << body.name << " Data Panel ===" << std::endl;
    std::cout << "Mass: " << body.mass << " kg" << std::endl;
    std::cout << "Radius: " << body.radius << " m" << std::endl;
    std::cout << "Semi-major axis: " << body.semiMajorAxis << " m" << std::endl;
    std::cout << "Orbital period: " << body.orbitalPeriod << " s (" 
              << body.orbitalPeriod / (24.0 * 3600.0) << " days)" << std::endl;
    
    if (!body.isCentralBody) {
        double orbitalVelocity = std::sqrt((Event4D::C * Event4D::C * 1.989e30) / body.semiMajorAxis);
        std::cout << "Orbital velocity: " << orbitalVelocity << " m/s" << std::endl;
    }
    
    std::cout << "Current position: (" << body.position.t << ", "
              << body.position.x << ", " << body.position.y << ", "
              << body.position.z << ")" << std::endl;
    std::cout << "Current velocity: (" << body.velocity.t << ", "
              << body.velocity.x << ", " << body.velocity.y << ", "
              << body.velocity.z << ")" << std::endl;
}

void UI4D::createSolarSystemEventMarkers() {
    eventToBodyMap.clear();
    
    for (const auto& bodyPair : solarSystem.bodies) {
        const SolarSystemBody& body = bodyPair.second;
        int eventIndex = addEvent(body.position);
        eventToBodyMap[eventIndex] = body.name;
    }
}

void UI4D::setSolarSystemScale(double scale) {
    solarSystem.scaleFactor = scale;
}

// ============================================================================
// Slice view controls
// ============================================================================

void UI4D::setSliceMode(int viewIndex, SlicingMode mode)
{
    if (viewIndex >= 0 && viewIndex < (int)sliceViews.size()) {
        sliceViews[viewIndex].setMode(mode);
    }
}

void UI4D::setSliceParameter(int viewIndex, double parameter)
{
    if (viewIndex >= 0 && viewIndex < (int)sliceViews.size()) {
        sliceViews[viewIndex].setParameter(parameter);
    }
}

void UI4D::nextSliceMode(int viewIndex)
{
    if (viewIndex >= 0 && viewIndex < (int)sliceViews.size()) {
        SlicingMode current = sliceViews[viewIndex].getMode();
        int modeValue = static_cast<int>(current);
        int nextValue = (modeValue + 1) % static_cast<int>(SlicingMode::COUNT);
        sliceViews[viewIndex].setMode(static_cast<SlicingMode>(nextValue));
    }
}

void UI4D::previousSliceMode(int viewIndex)
{
    if (viewIndex >= 0 && viewIndex < (int)sliceViews.size()) {
        SlicingMode current = sliceViews[viewIndex].getMode();
        int modeValue = static_cast<int>(current);
        int prevValue = (modeValue - 1 + static_cast<int>(SlicingMode::COUNT)) % static_cast<int>(SlicingMode::COUNT);
        sliceViews[viewIndex].setMode(static_cast<SlicingMode>(prevValue));
    }
}

// ============================================================================
// SliceView Implementation (in UI4D.cpp for access to full class)
// ============================================================================

namespace {

#if 0
/**
 * @brief Compute proper time along a world-line segment
 * @param metric The metric tensor at the evaluation point
 * @param e1 Start event
 * @param e2 End event
 * @return Proper time interval (timelike > 0, spacelike < 0, null = 0)
 */
double computeProperTimeInterval(const MetricTensor& metric, const Event4D& e1, const Event4D& e2)
{
    double dt = e2.t - e1.t;
    double dx = e2.x - e1.x;
    double dy = e2.y - e1.y;
    double dz = e2.z - e1.z;
    double coords[4] = {dt, dx, dy, dz};

    double ds2 = 0.0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            ds2 += metric.g[i][j] * coords[i] * coords[j];
        }
    }
    return ds2;  // Negative for timelike, positive for spacelike
}
#endif

/**
 * @brief Find the event on a world-line closest to a given proper time target
 * @param worldLine The reference world-line
 * @param targetProperTime Target proper time
 * @return Index of the closest event
 */
int findProperTimeIndex(const std::vector<Event4D>& worldLine, double targetProperTime)
{
    if (worldLine.empty()) return 0;

    // Accumulate proper time along the world-line
    double accumulated = 0.0;
    double bestDiff = std::abs(targetProperTime);
    int bestIndex = 0;

    for (size_t i = 1; i < worldLine.size(); i++) {
        double dt = worldLine[i].t - worldLine[i-1].t;
        double dx = worldLine[i].x - worldLine[i-1].x;
        double dy = worldLine[i].y - worldLine[i-1].y;
        double dz = worldLine[i].z - worldLine[i-1].z;

        // Use Minkowski interval as approximation for proper time increment
        double ds2 = -(dt*dt) + dx*dx + dy*dy + dz*dz;
        if (ds2 < 0) {
            accumulated += std::sqrt(-ds2);
        }

        double diff = std::abs(accumulated - targetProperTime);
        if (diff < bestDiff) {
            bestDiff = diff;
            bestIndex = static_cast<int>(i);
        }
    }
    return bestIndex;
}

/**
 * @brief Compute spatial direction orthogonal to world-line at a point
 * @param worldLine The reference world-line
 * @param index Index of the point on the world-line
 * @return Three orthonormal spatial vectors perpendicular to the 4-velocity
 */
std::array<std::array<double, 4>, 3> computeOrthogonalFrame(
    const std::vector<Event4D>& worldLine, int index)
{
    std::array<std::array<double, 4>, 3> frame;

    // Compute 4-velocity (tangent to world-line)
    std::array<double, 4> u = {1.0, 0.0, 0.0, 0.0};  // Default

    if (index > 0 && index < static_cast<int>(worldLine.size()) - 1) {
        // Central difference for velocity
        double dt = worldLine[index+1].t - worldLine[index-1].t;
        double dx = worldLine[index+1].x - worldLine[index-1].x;
        double dy = worldLine[index+1].y - worldLine[index-1].y;
        double dz = worldLine[index+1].z - worldLine[index-1].z;
        double norm = std::sqrt(std::abs(-dt*dt + dx*dx + dy*dy + dz*dz));
        if (norm > 1e-10) {
            u = {dt/norm, dx/norm, dy/norm, dz/norm};
        }
    } else if (worldLine.size() >= 2) {
        // Forward or backward difference at endpoints
        int next = std::min(index + 1, static_cast<int>(worldLine.size()) - 1);
        double dt = worldLine[next].t - worldLine[index].t;
        double dx = worldLine[next].x - worldLine[index].x;
        double dy = worldLine[next].y - worldLine[index].y;
        double dz = worldLine[next].z - worldLine[index].z;
        double norm = std::sqrt(std::abs(-dt*dt + dx*dx + dy*dy + dz*dz));
        if (norm > 1e-10) {
            u = {dt/norm, dx/norm, dy/norm, dz/norm};
        }
    }

    // Gram-Schmidt to find 3 orthogonal spatial directions
    // Start with standard basis vectors and orthogonalize against u
    std::array<double, 4> e1 = {0.0, 1.0, 0.0, 0.0};
    std::array<double, 4> e2 = {0.0, 0.0, 1.0, 0.0};
    std::array<double, 4> e3 = {0.0, 0.0, 0.0, 1.0};

    // Project out u component (using Minkowski inner product)
    auto projectOut = [&u](std::array<double, 4>& v) {
        double dot = -(u[0]*v[0]) + u[1]*v[1] + u[2]*v[2] + u[3]*v[3];
        double uNorm = -(u[0]*u[0]) + u[1]*u[1] + u[2]*u[2] + u[3]*u[3];
        if (std::abs(uNorm) > 1e-10) {
            for (int i = 0; i < 4; i++) {
                v[i] -= (dot / uNorm) * u[i];
            }
        }
    };

    projectOut(e1);
    projectOut(e2);
    projectOut(e3);

    // Normalize (spatial norm)
    auto spatialNorm = [](const std::array<double, 4>& v) {
        return std::sqrt(v[1]*v[1] + v[2]*v[2] + v[3]*v[3]);
    };

    double n1 = spatialNorm(e1);
    if (n1 > 1e-10) {
        for (int i = 0; i < 4; i++) e1[i] /= n1;
    }

    // Make e2 orthogonal to e1
    double dot12 = e1[1]*e2[1] + e1[2]*e2[2] + e1[3]*e2[3];
    for (int i = 0; i < 4; i++) e2[i] -= dot12 * e1[i];
    double n2 = spatialNorm(e2);
    if (n2 > 1e-10) {
        for (int i = 0; i < 4; i++) e2[i] /= n2;
    }

    // Make e3 orthogonal to e1 and e2
    double dot13 = e1[1]*e3[1] + e1[2]*e3[2] + e1[3]*e3[3];
    double dot23 = e2[1]*e3[1] + e2[2]*e3[2] + e2[3]*e3[3];
    for (int i = 0; i < 4; i++) e3[i] -= dot13 * e1[i] + dot23 * e2[i];
    double n3 = spatialNorm(e3);
    if (n3 > 1e-10) {
        for (int i = 0; i < 4; i++) e3[i] /= n3;
    }

    frame[0] = e1;
    frame[1] = e2;
    frame[2] = e3;
    return frame;
}

} // anonymous namespace

void SliceView::updateSlice()
{
    slicePoints.clear();

    // Generate points on the 3D slice based on current mode
    int gridSize = 20;
    double range = 50.0;

    switch (mode) {
        case SlicingMode::FIXED_T: {
            // Generate spatial points at t = sliceParameter
            for (int i = 0; i < gridSize; i++) {
                for (int j = 0; j < gridSize; j++) {
                    for (int k = 0; k < gridSize; k++) {
                        double x = -range + 2.0 * range * i / (gridSize - 1);
                        double y = -range + 2.0 * range * j / (gridSize - 1);
                        double z = -range + 2.0 * range * k / (gridSize - 1);
                        slicePoints.push_back(Event4D(sliceParameter, x, y, z));
                    }
                }
            }
            break;
        }

        case SlicingMode::PROPER_TIME: {
            // Generate points at constant proper time along reference world-line
            if (!referenceWorldLine.empty()) {
                // Find the event on the world-line closest to the target proper time
                int idx = findProperTimeIndex(referenceWorldLine, sliceParameter);
                const Event4D& origin = referenceWorldLine[idx];

                // Compute orthonormal spatial frame at this point
                auto frame = computeOrthogonalFrame(referenceWorldLine, idx);

                // Generate spatial grid in the orthogonal frame
                for (int i = 0; i < gridSize; i++) {
                    for (int j = 0; j < gridSize; j++) {
                        for (int k = 0; k < gridSize; k++) {
                            double offset = range * (2.0 * i / (gridSize - 1) - 1.0);
                            double x = origin.x + offset * frame[0][1]
                                             + range * (2.0 * j / (gridSize - 1) - 1.0) * frame[1][1]
                                             + range * (2.0 * k / (gridSize - 1) - 1.0) * frame[2][1];
                            double y = origin.y + offset * frame[0][2]
                                             + range * (2.0 * j / (gridSize - 1) - 1.0) * frame[1][2]
                                             + range * (2.0 * k / (gridSize - 1) - 1.0) * frame[2][2];
                            double z = origin.z + offset * frame[0][3]
                                             + range * (2.0 * j / (gridSize - 1) - 1.0) * frame[1][3]
                                             + range * (2.0 * k / (gridSize - 1) - 1.0) * frame[2][3];

                            // Use the proper time of the reference event as the time coordinate
                            slicePoints.push_back(Event4D(origin.t, x, y, z));
                        }
                    }
                }
            } else {
                // Fallback to fixed-t slice if no reference world-line
                for (int i = 0; i < gridSize; i++) {
                    for (int j = 0; j < gridSize; j++) {
                        for (int k = 0; k < gridSize; k++) {
                            double x = -range + 2.0 * range * i / (gridSize - 1);
                            double y = -range + 2.0 * range * j / (gridSize - 1);
                            double z = -range + 2.0 * range * k / (gridSize - 1);
                            slicePoints.push_back(Event4D(sliceParameter, x, y, z));
                        }
                    }
                }
            }
            break;
        }

        case SlicingMode::NULL_SLICE: {
            // Generate points on a null hypersurface emanating from an origin event
            // A null surface satisfies ds^2 = 0 from the origin
            Event4D origin(0.0, 0.0, 0.0, 0.0);
            if (!referenceWorldLine.empty()) {
                int idx = std::min(static_cast<int>(referenceWorldLine.size()) - 1,
                                   std::max(0, static_cast<int>(referenceWorldLine.size() * sliceParameter / range)));
                origin = referenceWorldLine[idx];
            }

            // Parameterize the null cone: t = t0 + r, spatial point at distance r in direction (theta, phi)
            for (int i = 0; i < gridSize; i++) {
                double r = range * (i + 1) / gridSize;
                int nTheta = std::max(4, gridSize / 2);
                int nPhi = std::max(4, gridSize);

                for (int j = 0; j < nTheta; j++) {
                    for (int k = 0; k < nPhi; k++) {
                        double theta = M_PI * j / (nTheta - 1);
                        double phi = 2.0 * M_PI * k / nPhi;

                        double x = origin.x + r * std::sin(theta) * std::cos(phi);
                        double y = origin.y + r * std::sin(theta) * std::sin(phi);
                        double z = origin.z + r * std::cos(theta);
                        double t = origin.t + r;  // Future null cone

                        slicePoints.push_back(Event4D(t, x, y, z));
                    }
                }
            }
            break;
        }

        case SlicingMode::COMPLEX_REAL: {
            // Complex spacetime: real part of complexified coordinates
            // z = x + iy, with real slice showing Re(z)
            for (int i = 0; i < gridSize; i++) {
                for (int j = 0; j < gridSize; j++) {
                    for (int k = 0; k < gridSize; k++) {
                        double x = -range + 2.0 * range * i / (gridSize - 1);
                        double y = -range + 2.0 * range * j / (gridSize - 1);
                        double z = -range + 2.0 * range * k / (gridSize - 1);

                        // Complex time: t_c = t0 + i*sliceParameter
                        // Real part uses cosh(sliceParameter) * t0
                        double tReal = sliceParameter * std::cosh(x / range);
                        slicePoints.push_back(Event4D(tReal, x, y, z));
                    }
                }
            }
            break;
        }

        case SlicingMode::COMPLEX_IMAG: {
            // Complex spacetime: imaginary part of complexified coordinates
            for (int i = 0; i < gridSize; i++) {
                for (int j = 0; j < gridSize; j++) {
                    for (int k = 0; k < gridSize; k++) {
                        double x = -range + 2.0 * range * i / (gridSize - 1);
                        double y = -range + 2.0 * range * j / (gridSize - 1);
                        double z = -range + 2.0 * range * k / (gridSize - 1);

                        // Imaginary part uses sinh(sliceParameter) * t0
                        double tImag = sliceParameter * std::sinh(x / range);
                        slicePoints.push_back(Event4D(tImag, x, y, z));
                    }
                }
            }
            break;
        }

        default: {
        }
    }
}

// ============================================================================
// Layout management
// ============================================================================

void UI4D::setLayoutMode(LayoutMode mode) {
    currentLayout = mode;
    std::cout << "Layout mode set to: " << static_cast<int>(mode) << std::endl;
}

void UI4D::togglePanelVisibility(int panelIndex) {
    std::cout << "Toggling visibility of panel: " << panelIndex << std::endl;
}

void UI4D::collapsePanel(int panelIndex) {
    std::cout << "Collapsing panel: " << panelIndex << std::endl;
}

void UI4D::expandPanel(int panelIndex) {
    std::cout << "Expanding panel: " << panelIndex << std::endl;
}

void UI4D::setPanelPosition(int panelIndex, int x, int y) {
    std::cout << "Setting position of panel " << panelIndex << " to (" << x << ", " << y << ")" << std::endl;
}

void UI4D::setPanelSize(int panelIndex, int width, int height) {
    std::cout << "Setting size of panel " << panelIndex << " to (" << width << ", " << height << ")" << std::endl;
}

void UI4D::startResize(int panelIndex, int startX, int startY) {
    isResizing = true;
    resizePanelIndex = panelIndex;
    resizeStartX = startX;
    resizeStartY = startY;
    std::cout << "Starting resize of panel " << panelIndex << " at (" << startX << ", " << startY << ")" << std::endl;
}

void UI4D::updateResize(int currentX, int currentY) {
    if (!isResizing || resizePanelIndex < 0) return;
    
    int deltaX = currentX - resizeStartX;
    int deltaY = currentY - resizeStartY;
    std::cout << "Updating resize: delta (" << deltaX << ", " << deltaY << ")" << std::endl;
}

void UI4D::endResize() {
    isResizing = false;
    resizePanelIndex = -1;
    std::cout << "Ending resize" << std::endl;
}

// ============================================================================
// Quantum Gravity Integration
// ============================================================================

void UI4D::setQuantumRenderer(std::shared_ptr<QuantumGeometryRenderer> renderer) {
    quantumRenderer = std::move(renderer);
    std::cout << "Quantum geometry renderer set" << std::endl;
}

void UI4D::setPlanckMicroscope(PlanckMicroscope* microscope) {
    planckMicroscope = microscope;
    if (planckMicroscope) {
        std::cout << "Planck microscope attached" << std::endl;
    }
}

void UI4D::setActiveQuantumTheory(const std::string& theoryName) {
    activeQuantumTheory = theoryName;
    
    if (quantumRenderer) {
        QuantumGeometryType type = QuantumGeometryType::NONE;
        if (theoryName == "CDT" || theoryName == "causal_dynamical_triangulations") {
            type = QuantumGeometryType::CDT;
        } else if (theoryName == "Spin Foam" || theoryName == "spin_foam") {
            type = QuantumGeometryType::SPIN_FOAM;
        } else if (theoryName == "GFT" || theoryName == "group_field_theory") {
            type = QuantumGeometryType::GFT;
        } else if (theoryName == "Causal Set" || theoryName == "causal_set") {
            type = QuantumGeometryType::CAUSAL_SET;
        }
        quantumRenderer->setCurrentType(type);
    }
    
    std::cout << "Active quantum theory set to: " << theoryName << std::endl;
}

std::string UI4D::getActiveQuantumTheory() const {
    return activeQuantumTheory;
}

void UI4D::toggleQuantumGeometry(bool enable) {
    showQuantumGeometry = enable;
    if (quantumRenderer) {
        if (enable) {
            if (activeQuantumTheory.empty()) {
                setActiveQuantumTheory("CDT");
            }
        }
    }
    std::cout << "Quantum geometry overlay " << (enable ? "enabled" : "disabled") << std::endl;
}

} // namespace quantumverse
