#include "UI4D.h"
#include <cmath>
#include <algorithm>
#include <iostream>

// Ensure M_PI is available on all platforms (MSVC <cmath> may not define it)
#ifndef M_PI
#define M_PI 3.14159265358979323846
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
    
    // Sun
    SolarSystemBody sun;
    sun.name = "Sun";
    sun.mass = 1.989e30;
    sun.radius = 696340000.0;
    sun.position = Event4D(0.0, 0.0, 0.0, 0.0);
    sun.velocity = Event4D(0.0, 0.0, 0.0, 0.0);
    sun.orbitalPeriod = 0.0;
    sun.semiMajorAxis = 0.0;
    sun.isCentralBody = true;
    sun.isStar = true;
    sun.showOrbit = false;
    sun.textureId = "sun_texture";
    solarSystem.bodies["Sun"] = sun;
    
    // Mercury
    SolarSystemBody mercury;
    mercury.name = "Mercury";
    mercury.mass = 0.055 * 5.972e24;
    mercury.radius = 2439700.0;
    mercury.semiMajorAxis = 0.387 * 149597870700.0;
    mercury.orbitalPeriod = 87.969 * 24 * 3600;
    double mercuryOrbitalVelocity = std::sqrt((Event4D::C * Event4D::C * 1.989e30) / mercury.semiMajorAxis);
    mercury.position = Event4D(0.0, mercury.semiMajorAxis, 0.0, 0.0);
    mercury.velocity = Event4D(0.0, 0.0, 0.0, -mercuryOrbitalVelocity);
    mercury.isCentralBody = false;
    mercury.isStar = false;
    mercury.showOrbit = true;
    mercury.textureId = "mercury_texture";
    solarSystem.bodies["Mercury"] = mercury;
    
    // Venus
    SolarSystemBody venus;
    venus.name = "Venus";
    venus.mass = 0.815 * 5.972e24;
    venus.radius = 6051800.0;
    venus.semiMajorAxis = 0.723 * 149597870700.0;
    venus.orbitalPeriod = 224.701 * 24 * 3600;
    double venusOrbitalVelocity = std::sqrt((Event4D::C * Event4D::C * 1.989e30) / venus.semiMajorAxis);
    venus.position = Event4D(0.0, venus.semiMajorAxis, 0.0, 0.0);
    venus.velocity = Event4D(0.0, 0.0, 0.0, -venusOrbitalVelocity);
    venus.isCentralBody = false;
    venus.isStar = false;
    venus.showOrbit = true;
    venus.textureId = "venus_texture";
    solarSystem.bodies["Venus"] = venus;
    
    // Earth
    SolarSystemBody earth;
    earth.name = "Earth";
    earth.mass = 5.972e24;
    earth.radius = 6371000.0;
    earth.semiMajorAxis = 1.0 * 149597870700.0;
    earth.orbitalPeriod = 365.256 * 24 * 3600;
    double earthOrbitalVelocity = std::sqrt((Event4D::C * Event4D::C * 1.989e30) / earth.semiMajorAxis);
    earth.position = Event4D(0.0, earth.semiMajorAxis, 0.0, 0.0);
    earth.velocity = Event4D(0.0, 0.0, 0.0, -earthOrbitalVelocity);
    earth.isCentralBody = false;
    earth.isStar = false;
    earth.showOrbit = true;
    earth.textureId = "earth_texture";
    solarSystem.bodies["Earth"] = earth;
    
    // Mars
    SolarSystemBody mars;
    mars.name = "Mars";
    mars.mass = 0.107 * 5.972e24;
    mars.radius = 3389500.0;
    mars.semiMajorAxis = 1.524 * 149597870700.0;
    mars.orbitalPeriod = 686.980 * 24 * 3600;
    double marsOrbitalVelocity = std::sqrt((Event4D::C * Event4D::C * 1.989e30) / mars.semiMajorAxis);
    mars.position = Event4D(0.0, mars.semiMajorAxis, 0.0, 0.0);
    mars.velocity = Event4D(0.0, 0.0, 0.0, -marsOrbitalVelocity);
    mars.isCentralBody = false;
    mars.isStar = false;
    mars.showOrbit = true;
    mars.textureId = "mars_texture";
    solarSystem.bodies["Mars"] = mars;
    
    // Jupiter
    SolarSystemBody jupiter;
    jupiter.name = "Jupiter";
    jupiter.mass = 317.8 * 5.972e24;
    jupiter.radius = 69911000.0;
    jupiter.semiMajorAxis = 5.204 * 149597870700.0;
    jupiter.orbitalPeriod = 11.862 * 365.256 * 24 * 3600;
    double jupiterOrbitalVelocity = std::sqrt((Event4D::C * Event4D::C * 1.989e30) / jupiter.semiMajorAxis);
    jupiter.position = Event4D(0.0, jupiter.semiMajorAxis, 0.0, 0.0);
    jupiter.velocity = Event4D(0.0, 0.0, 0.0, -jupiterOrbitalVelocity);
    jupiter.isCentralBody = false;
    jupiter.isStar = false;
    jupiter.showOrbit = true;
    jupiter.textureId = "jupiter_texture";
    solarSystem.bodies["Jupiter"] = jupiter;
    
    // Saturn
    SolarSystemBody saturn;
    saturn.name = "Saturn";
    saturn.mass = 95.2 * 5.972e24;
    saturn.radius = 58232000.0;
    saturn.semiMajorAxis = 9.582 * 149597870700.0;
    saturn.orbitalPeriod = 29.457 * 365.256 * 24 * 3600;
    double saturnOrbitalVelocity = std::sqrt((Event4D::C * Event4D::C * 1.989e30) / saturn.semiMajorAxis);
    saturn.position = Event4D(0.0, saturn.semiMajorAxis, 0.0, 0.0);
    saturn.velocity = Event4D(0.0, 0.0, 0.0, -saturnOrbitalVelocity);
    saturn.isCentralBody = false;
    saturn.isStar = false;
    saturn.showOrbit = true;
    saturn.textureId = "saturn_texture";
    solarSystem.bodies["Saturn"] = saturn;
    
    // Uranus
    SolarSystemBody uranus;
    uranus.name = "Uranus";
    uranus.mass = 14.5 * 5.972e24;
    uranus.radius = 25362000.0;
    uranus.semiMajorAxis = 19.201 * 149597870700.0;
    uranus.orbitalPeriod = 84.020 * 365.256 * 24 * 3600;
    double uranusOrbitalVelocity = std::sqrt((Event4D::C * Event4D::C * 1.989e30) / uranus.semiMajorAxis);
    uranus.position = Event4D(0.0, uranus.semiMajorAxis, 0.0, 0.0);
    uranus.velocity = Event4D(0.0, 0.0, 0.0, -uranusOrbitalVelocity);
    uranus.isCentralBody = false;
    uranus.isStar = false;
    uranus.showOrbit = true;
    uranus.textureId = "uranus_texture";
    solarSystem.bodies["Uranus"] = uranus;
    
    // Neptune
    SolarSystemBody neptune;
    neptune.name = "Neptune";
    neptune.mass = 17.1 * 5.972e24;
    neptune.radius = 24622000.0;
    neptune.semiMajorAxis = 30.047 * 149597870700.0;
    neptune.orbitalPeriod = 164.8 * 365.256 * 24 * 3600;
    double neptuneOrbitalVelocity = std::sqrt((Event4D::C * Event4D::C * 1.989e30) / neptune.semiMajorAxis);
    neptune.position = Event4D(0.0, neptune.semiMajorAxis, 0.0, 0.0);
    neptune.velocity = Event4D(0.0, 0.0, 0.0, -neptuneOrbitalVelocity);
    neptune.isCentralBody = false;
    neptune.isStar = false;
    neptune.showOrbit = true;
    neptune.textureId = "neptune_texture";
    solarSystem.bodies["Neptune"] = neptune;
    
    // Calculate initial orbit points
    for (auto& bodyPair : solarSystem.bodies) {
        calculateOrbitalTrajectory(bodyPair.second, 50);
    }
}

void UI4D::updateSolarSystemPositions(double currentTime) {
    for (auto& bodyPair : solarSystem.bodies) {
        SolarSystemBody& body = bodyPair.second;
        
        if (body.isCentralBody) {
            body.position = Event4D(0.0, 0.0, 0.0, 0.0);
            body.velocity = Event4D(0.0, 0.0, 0.0, 0.0);
            continue;
        }
        
        double phase = fmod(currentTime, body.orbitalPeriod) / body.orbitalPeriod * 2.0 * M_PI;
        
        body.position.t = currentTime;
        body.position.x = body.semiMajorAxis * std::cos(phase);
        body.position.y = 0.0;
        body.position.z = body.semiMajorAxis * std::sin(phase);
        
        body.velocity.t = 1.0;
        body.velocity.x = -body.semiMajorAxis * std::sin(phase) * (2.0 * M_PI / body.orbitalPeriod);
        body.velocity.y = 0.0;
        body.velocity.z = body.semiMajorAxis * std::cos(phase) * (2.0 * M_PI / body.orbitalPeriod);
    }
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

void UI4D::renderSolarSystem() {
    if (!showSolarSystem) return;
    
    static double simulationTime = 0.0;
    simulationTime += 0.01;
    updateSolarSystemPositions(simulationTime);
    
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
