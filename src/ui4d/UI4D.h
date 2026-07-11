#ifndef QUANTUMVERSE_UI4D_H
#define QUANTUMVERSE_UI4D_H

/**
 * QuantumVerse Simulator - 4D User Interface
 * 
 * A "powerfully advanced" graphical user interface for the QuantumVerse Simulator
 * that enables users to inhabit the four-dimensional Lorentzian manifold – seeing,
 * navigating, and editing spacetime as a true 4D entity.
 * 
 * Based on: plans/plan.md - QuantumVerse 4D UI Design Plan
 * 
 * Design Philosophy:
 * - Direct 4D Manipulation: Events, world-lines, and curvature are native 4D objects
 * - Multiple Simultaneous Slices: Several coordinated views reconstruct 4D structure
 * - Causal & Geometric Clarity: Signature (− + + +) respected in all visual elements
 * - Extensible to Complex & Topological Regimes: Handles imaginary coordinates and topology modes
 */

#include <vector>
#include <memory>
#include <array>
#include <string>
#include <map>
#include <set>
#include "PlanckMicroscope.h"
#include "../spacetime/Event4D.h"
#include "../spacetime/MetricTensor.h"
#include "../physics/GeodesicIntegrator.h"
#include "../rendering/CurvatureRenderer.h"
#include "../rendering/QuantumGeometryRenderer.h"

// Ensure M_PI is available on all platforms (MSVC <cmath> may not define it)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

// Forward declarations
class Camera4D;
class SliceView;
class CausalGraph;
class DiscoveryProbe;
class SingularityHandler;

// 4D UI Configuration
struct UI4DConfig {
    bool showLightCones = true;
    bool showCausalStructure = true;
    bool showCurvatureGrid = true;
    bool showWorldLines = true;
    bool showTimeGeography = true;
    bool showTopologyIndicators = true;
    
    float curvatureScale = 1.0f;
    float lightConeTransparency = 0.3f;
    float worldLineThickness = 2.0f;
    
    int numSlices = 4;  // Number of simultaneous views
    float sliceSpacing = 10.0f;
};

// 4D Camera Model
// Defined by position in R^4, look-at point, and two up-vectors
class Camera4D {
private:
    Event4D position;      // Camera position in spacetime
    Event4D lookAt;        // Look-at point in spacetime
    std::array<double, 4> upVector1;  // First up-vector (spatial)
    std::array<double, 4> upVector2;  // Second up-vector (temporal/spatial)
    
    double fov;            // Field of view (radians)
    double nearPlane;      // Near clipping plane
    double farPlane;       // Far clipping plane
    
    // 4D projection matrix (4x4 for 4D->3D, then 3D->2D)
    std::array<std::array<double, 4>, 4> projectionMatrix;
    std::array<std::array<double, 4>, 4> viewMatrix;
    
    void computeViewMatrix() {
        // Compute 4D view matrix from position, lookAt, and up vectors
        // This is a 4D analog of the lookAt transformation
        
        std::array<double, 4> forward;
        double length = 0.0;
        for (int i = 0; i < 4; i++) {
            forward[i] = lookAt[i] - position[i];
            length += forward[i] * forward[i];
        }
        length = std::sqrt(length);
        
        if (length < 1e-10) return;  // Avoid division by zero
        
        for (int i = 0; i < 4; i++) {
            forward[i] /= length;
        }
        
        // Compute orthonormal basis in 4D (Gram-Schmidt)
        // This creates a 4D coordinate frame for the camera
        // Implementation simplified for clarity
        // Full implementation would use 4D rotation matrices (SO(4))
    }
    
public:
    Camera4D(
        const Event4D& pos = Event4D(),
        const Event4D& target = Event4D(1.0, 0.0, 0.0, 0.0),
        double fov_ = M_PI / 4.0
    ) : position(pos), lookAt(target), fov(fov_), nearPlane(0.1), farPlane(1000.0) {
        // Initialize up vectors to standard basis
        upVector1 = {0.0, 1.0, 0.0, 0.0};
        upVector2 = {0.0, 0.0, 1.0, 0.0};
        computeViewMatrix();
    }
    
    /**
     * @brief Reset camera to a known safe default view
     * 
     * Positions the camera at a safe distance from the origin with
     * a clear view of the spacetime grid. This is useful for recovering
     * from black screen issues caused by camera being inside a singularity
     * or at an invalid position.
     */
    void reset() {
        // Safe default: position at (0, 5, 10, 0) looking at origin
        // This provides a clear view of the central region
        position = Event4D(0.0, 0.0, 5.0, 10.0);
        lookAt = Event4D(0.0, 0.0, 0.0, 0.0);
        upVector1 = {0.0, 1.0, 0.0, 0.0};
        upVector2 = {0.0, 0.0, 0.0, 1.0};
        fov = M_PI / 4.0;
        nearPlane = 0.1;
        farPlane = 1000.0;
        computeViewMatrix();
    }
    
    // Move camera in 4D
    void translate(const std::array<double, 4>& delta) {
        for (int i = 0; i < 4; i++) {
            position[i] += delta[i];
            lookAt[i] += delta[i];
        }
        computeViewMatrix();
    }
    
    // Rotate in 4D (SO(4) rotation in one of 6 planes)
    void rotate(int plane1, int plane2, double angle) {
        // plane1, plane2 ∈ {0,1,2,3} where 0=t, 1=x, 2=y, 3=z
        // Rotation in the plane spanned by axes plane1 and plane2
        
        double cosA = std::cos(angle);
        double sinA = std::sin(angle);
        
        // Apply rotation to lookAt relative to position
        double v1 = lookAt[plane1] - position[plane1];
        double v2 = lookAt[plane2] - position[plane2];
        
        lookAt[plane1] = position[plane1] + cosA * v1 - sinA * v2;
        lookAt[plane2] = position[plane2] + sinA * v1 + cosA * v2;
        
        // Also rotate up vectors
        double u1_1 = upVector1[plane1], u1_2 = upVector1[plane2];
        upVector1[plane1] = cosA * u1_1 - sinA * u1_2;
        upVector1[plane2] = sinA * u1_1 + cosA * u1_2;
        
        double u2_1 = upVector2[plane1], u2_2 = upVector2[plane2];
        upVector2[plane1] = cosA * u2_1 - sinA * u2_2;
        upVector2[plane2] = sinA * u2_1 + cosA * u2_2;
        
        computeViewMatrix();
    }
    
    // Zoom (4D dolly)
    void zoom(double factor) {
        std::array<double, 4> direction;
        double length = 0.0;
        for (int i = 0; i < 4; i++) {
            direction[i] = lookAt[i] - position[i];
            length += direction[i] * direction[i];
        }
        length = std::sqrt(length);
        
        if (length < 1e-10) return;
        
        for (int i = 0; i < 4; i++) {
            direction[i] /= length;
            position[i] += direction[i] * factor;
            lookAt[i] += direction[i] * factor;
        }
        computeViewMatrix();
    }
    
    // Change slicing offset (displace slicing hyperplane)
    void setSliceOffset(double offset) {
        // Move the slicing hyperplane along its normal
        // This affects which 3D slice is displayed
        // Implementation depends on slicing mode
    }
    
    // Getters
    const Event4D& getPosition() const { return position; }
    const Event4D& getLookAt() const { return lookAt; }
    const std::array<double, 4>& getUpVector1() const { return upVector1; }
    const std::array<double, 4>& getUpVector2() const { return upVector2; }
    
    // Project 4D point to 3D (for rendering)
     std::array<double, 3> projectTo3D(const Event4D& point) const {
         // Transform to camera space
         std::array<double, 4> viewCoords;
         for (int i = 0; i < 4; i++) {
             viewCoords[i] = point[i] - position[i];
         }

         // Apply perspective projection (simplified)
         // Full implementation would use proper 4D->3D projection matrix
         double w = viewCoords[0];  // Time component
         if (std::abs(w) < 1e-10) w = 1e-10;

         std::array<double, 3> result;
         result[0] = viewCoords[1] / w;  // x
         result[1] = viewCoords[2] / w;  // y
         result[2] = viewCoords[3] / w;  // z

         return result;
     }
    
     // Compute 4x4 view matrix for GPU rendering (4D camera space transform)
      // Returns column-major 4x4 matrix as array of 16 components (OpenGL order)
      // This projects 4D spacetime coordinates to 3D camera space for rendering
      std::array<double, 16> computeViewMatrix4x4() const {
          std::array<double, 16> m = {0};

          // Compute forward direction (lookAt - position) in 4D
          std::array<double, 4> forward;
          double fLen = 0.0;
          for (int i = 0; i < 4; i++) {
              forward[i] = lookAt[i] - position[i];
              fLen += forward[i] * forward[i];
          }
          fLen = std::sqrt(fLen);
          if (fLen < 1e-10) {
              // Identity fallback
              m[0] = m[5] = m[10] = m[15] = 1.0;
              return m;
          }
          for (int i = 0; i < 4; i++) forward[i] /= fLen;

          // Build orthonormal basis via Gram-Schmidt in 4D spatial subspace
          // Right vector = up1 × forward (spatial cross product)
          std::array<double, 4> right;
          right[0] = 0.0;  // Time component zero for spatial basis
          right[1] = upVector1[2] * forward[3] - upVector1[3] * forward[2];
          right[2] = upVector1[3] * forward[1] - upVector1[1] * forward[3];
          right[3] = upVector1[1] * forward[2] - upVector1[2] * forward[1];
          double rLen = std::sqrt(right[1]*right[1] + right[2]*right[2] + right[3]*right[3]);
          if (rLen > 1e-10) {
              right[1] /= rLen; right[2] /= rLen; right[3] /= rLen;
          } else {
              right = {0.0, 1.0, 0.0, 0.0};
          }

          // True up = forward × right (in 4D spatial subspace)
          std::array<double, 4> trueUp;
          trueUp[0] = 0.0;
          trueUp[1] = forward[2] * right[3] - forward[3] * right[2];
          trueUp[2] = forward[3] * right[1] - forward[1] * right[3];
          trueUp[3] = forward[1] * right[2] - forward[2] * right[1];
          double uLen = std::sqrt(trueUp[1]*trueUp[1] + trueUp[2]*trueUp[2] + trueUp[3]*trueUp[3]);
          if (uLen > 1e-10) {
              trueUp[1] /= uLen; trueUp[2] /= uLen; trueUp[3] /= uLen;
          } else {
              trueUp = {0.0, 0.0, 1.0, 0.0};
          }

          // Fourth basis: temporal component for 4D projection
          // This captures the time-axis component for proper 4D-to-3D projection
          std::array<double, 4> tVec;
          tVec[0] = forward[0];
           tVec[1] = forward[1] - right[1] * (right[1]*forward[1] + right[2]*forward[2] + right[3]*forward[3]);
           tVec[2] = forward[2] - trueUp[1] * (trueUp[1]*forward[1] + trueUp[2]*forward[2] + trueUp[3]*forward[3]);
           tVec[3] = forward[3] - (right[3] * (right[1]*forward[1] + right[2]*forward[2] + right[3]*forward[3])) +
                                       trueUp[3] * (trueUp[1]*forward[1] + trueUp[2]*forward[2] + trueUp[3]*forward[3]);
          double tLen = std::sqrt(tVec[0]*tVec[0] + tVec[1]*tVec[1] + tVec[2]*tVec[2] + tVec[3]*tVec[3]);
          if (tLen > 1e-10) {
              for (int i = 0; i < 4; i++) tVec[i] /= tLen;
          } else {
              tVec = {1.0, 0.0, 0.0, 0.0};
          }

          // Build view matrix (column-major for OpenGL)
          // Maps 4D world coordinates to camera-aligned 4D space
          // Column 0: right spatial axis
          m[0] = right[1];   m[1] = trueUp[1];   m[2] = -forward[1];  m[3] = 0.0;
          // Column 1: up spatial axis
          m[4] = right[2];   m[5] = trueUp[2];   m[6] = -forward[2];  m[7] = 0.0;
          // Column 2: depth axis (4th component for 4D projection)
          m[8] = right[3];   m[9] = trueUp[3];   m[10] = -forward[3]; m[11] = 0.0;
          // Column 3: translation (position offset in camera basis)
          m[12] = -(right[1]*position[1] + right[2]*position[2] + right[3]*position[3]);
          m[13] = -(trueUp[1]*position[1] + trueUp[2]*position[2] + trueUp[3]*position[3]);
          m[14] = forward[1]*position[1] + forward[2]*position[2] + forward[3]*position[3];
          m[15] = 1.0;

          return m;
      }
    
     // Compute 4x4 perspective projection matrix (column-major, OpenGL style)
     std::array<double, 16> computeProjectionMatrix4x4(double aspectRatio = 1.0) const {
         std::array<double, 16> m = {0};
         double f = 1.0 / std::tan(fov * 0.5);
         m[0] = f / aspectRatio;
         m[5] = f;
         m[10] = -(farPlane + nearPlane) / (farPlane - nearPlane);
         m[11] = -1.0;
         m[14] = -(2.0 * farPlane * nearPlane) / (farPlane - nearPlane);
         m[15] = 0.0;
         return m;
     }
    
    // Unproject screen coordinates to 4D ray
    std::pair<Event4D, std::array<double, 4>> screenTo4DRay(
        double screenX, double screenY, double screenWidth, double screenHeight) const {
        // Convert screen coordinates to normalized device coordinates
        double ndcX = (2.0 * screenX / screenWidth) - 1.0;
        double ndcY = 1.0 - (2.0 * screenY / screenHeight);
        
        // Create ray in 4D (simplified: ray in 3D subspace at current time)
        Event4D origin = position;
        std::array<double, 4> direction;
        
        // Direction based on NDC and camera orientation
        // This is a simplified version; full implementation would use
        // inverse projection matrix
        direction[0] = 1.0;  // Time component
        direction[1] = ndcX;
        direction[2] = ndcY;
        direction[3] = -1.0;  // Forward in z
        
        // Normalize spatial components
        double spatialLen = std::sqrt(
            direction[1]*direction[1] + 
            direction[2]*direction[2] + 
            direction[3]*direction[3]);
        if (spatialLen > 0) {
            for (int i = 1; i < 4; i++) {
                direction[i] /= spatialLen;
            }
        }
        
        return {origin, direction};
    }
};

// Slicing Mode Enumeration
enum class SlicingMode {
    FIXED_T,        // Constant coordinate time
    PROPER_TIME,    // Constant proper time along reference world-line
    NULL_SLICE,     // Null hypersurface (light-front)
    COMPLEX_REAL,   // Real part of complex spacetime
    COMPLEX_IMAG,   // Imaginary part of complex spacetime
    COUNT           // Number of slicing modes (sentinel)
};

// 3D Slice View
// Represents a 3D hypersurface of the 4D spacetime
class SliceView {
private:
    SlicingMode mode;
    double sliceParameter;  // t0, τ0, etc.
    
    std::shared_ptr<MetricTensor> metric;
    std::vector<Event4D> slicePoints;
    
    // Reference world-line for proper-time slicing
    std::vector<Event4D> referenceWorldLine;
    
public:
    SliceView(SlicingMode m = SlicingMode::FIXED_T, double param = 0.0)
        : mode(m), sliceParameter(param) {}
    
    void setMetric(std::shared_ptr<MetricTensor> m) {
        metric = m;
    }
    
    void setMode(SlicingMode m) {
        mode = m;
        updateSlice();
    }
    
    void setParameter(double param) {
        sliceParameter = param;
        updateSlice();
    }
    
    void setReferenceWorldLine(const std::vector<Event4D>& worldline) {
        referenceWorldLine = worldline;
        if (mode == SlicingMode::PROPER_TIME) {
            updateSlice();
        }
    }
    
    // Update slice based on current mode and parameter
     // Implementation in UI4D.cpp
     void updateSlice();
    
    const std::vector<Event4D>& getPoints() const {
        return slicePoints;
    }
    
    SlicingMode getMode() const { return mode; }
    double getParameter() const { return sliceParameter; }
};

// Causal Graph
// Graph-based representation of causal relations between events
class CausalGraph {
private:
    struct CausalNode {
        Event4D event;
        std::vector<int> causalFuture;  // Indices of events in causal future
        std::vector<int> causalPast;    // Indices of events in causal past
        bool isSelected = false;
    };
    
    std::vector<CausalNode> nodes;
    
public:
    // Add event to causal graph
    int addEvent(const Event4D& event) {
        CausalNode node;
        node.event = event;
        
        // Check causal relations with existing nodes
        for (size_t i = 0; i < nodes.size(); i++) {
            double interval = event.intervalSquared(nodes[i].event);
            
            if (interval > 0 && event[0] > nodes[i].event[0]) {
                // Event is in causal future of node i
                nodes[i].causalFuture.push_back(nodes.size());
                node.causalPast.push_back(i);
            } else if (interval > 0 && event[0] < nodes[i].event[0]) {
                // Event is in causal past of node i
                nodes[i].causalPast.push_back(nodes.size());
                node.causalFuture.push_back(i);
            }
            // interval == 0: lightlike (null)
            // interval < 0: spacelike (no causal relation)
        }
        
        nodes.push_back(node);
        return nodes.size() - 1;
    }
    
    // Get causal future of event
    const std::vector<int>& getCausalFuture(int index) const {
        static const std::vector<int> empty;
        if (index < 0 || index >= (int)nodes.size()) return empty;
        return nodes[index].causalFuture;
    }
    
    // Get causal past of event
    const std::vector<int>& getCausalPast(int index) const {
        static const std::vector<int> empty;
        if (index < 0 || index >= (int)nodes.size()) return empty;
        return nodes[index].causalPast;
    }
    
    // Check if two events are causally related
    bool areCausallyRelated(int index1, int index2) const {
        if (index1 < 0 || index1 >= (int)nodes.size()) return false;
        if (index2 < 0 || index2 >= (int)nodes.size()) return false;
        
        double interval = nodes[index1].event.intervalSquared(nodes[index2].event);
        return interval >= 0;  // Timelike or lightlike
    }
    
    // Find closed timelike curves (causality violations)
    std::vector<std::vector<int>> findClosedTimelikeCurves() const {
        std::vector<std::vector<int>> ctcList;
        
        // Simple check: look for cycles in causal graph
        // Full implementation would use depth-first search
        
        return ctcList;
    }
    
    const std::vector<CausalNode>& getNodes() const {
        return nodes;
    }
    
    void clear() {
        nodes.clear();
    }
};

// Discovery Probe
// 4D cursor that samples fields at any spacetime point
class DiscoveryProbe {
private:
    Event4D position;
    
public:
    DiscoveryProbe(const Event4D& pos = Event4D()) : position(pos) {}
    
    void setPosition(const Event4D& pos) {
        position = pos;
    }
    
    const Event4D& getPosition() const {
        return position;
    }
    
    // Sample curvature scalars at probe position
    struct CurvatureSample {
        double kretschmann;    // Kretschmann scalar
        double ricci;          // Ricci scalar
        double weyl;           // Weyl scalar (simplified)
    };
    
    CurvatureSample sampleCurvature(
        const std::shared_ptr<MetricTensor>& metric) const {
        
        CurvatureSample sample;
        
        // Simplified: compute curvature for Schwarzschild metric
        // Full implementation would compute Riemann tensor components
        
        double r = std::sqrt(
            position[1]*position[1] + 
            position[2]*position[2] + 
            position[3]*position[3]);
        
        // For Schwarzschild: K = 48 G^2 M^2 / (c^4 r^6)
        double rs = 2.0 * Event4D::G * 1.989e30 / (Event4D::C * Event4D::C);
        if (r > rs * 1.1) {
            sample.kretschmann = 48.0 * rs * rs / (r*r*r*r*r*r);
        } else {
            sample.kretschmann = 0.0;  // Inside horizon - simplified
        }
        
        sample.ricci = 0.0;  // Vacuum solution
        sample.weyl = sample.kretschmann;  // Simplified
        
        return sample;
    }
    
    // Sample stress-energy tensor
    struct StressEnergySample {
        double energyDensity;
        double pressure;
        std::array<double, 3> momentumFlux;
    };
    
    StressEnergySample sampleStressEnergy(
        const std::shared_ptr<MetricTensor>& metric) const {
        
        StressEnergySample sample;
        sample.energyDensity = 0.0;  // Vacuum
        sample.pressure = 0.0;
        sample.momentumFlux = {0.0, 0.0, 0.0};
        return sample;
    }
};

// Main 4D UI Class
class UI4D {
public:
    // Power-up features - public structs for UI access
    struct Waypoint {
        std::string name;
        Event4D position;
        std::string description;
        bool isActive;
    };
    
    struct WaypointFlightConfig {
        bool enabled = false;
        float speed = 1.0f;          // Waypoints per second
        float progress = 0.0f;       // Progress through current segment (0-1)
        int currentSegment = 0;      // Current waypoint segment being interpolated
        bool loop = false;           // Loop back to first waypoint after last
    };
    
    // Curvature anomaly detection
    struct AnomalyAlert {
        Event4D position;
        double curvatureValue;
        std::string description;
        bool isAcknowledged;
        double timestamp;
    };
    
    // Spacetime curvature manipulation
    struct CurvatureManipulationTool {
        bool active = false;
        float radius = 10.0f;        // Effect radius in meters
        float strength = 1.0f;       // Curvature strength multiplier
        Event4D center;              // Center point of manipulation
        enum Type { POSITIVE, NEGATIVE, WAVE } type = POSITIVE;
    };
    
    // Gravitational lensing visualization
    struct LensingEffect {
        bool enabled = false;
        int numRings = 5;            // Number of Einstein rings to show
        float ringSpacing = 5.0f;    // Spacing between rings
        float intensity = 0.8f;      // Lensing intensity
    };
    
    // Time dilation visualization
    struct TimeDilationEffect {
        bool enabled = false;
        float minDilation = 0.5f;    // Minimum time dilation factor
        float maxDilation = 2.0f;    // Maximum time dilation factor
        bool showGrid = true;        // Show time dilation grid
    };
    
    // Quantum foam visualization (Planck scale)
    struct QuantumFoamEffect {
        bool enabled = false;
        float scale = 1.0e-35f;      // Planck scale
        int fluctuationCount = 100;  // Number of quantum fluctuations
        float lifetime = 0.1f;       // Lifetime of fluctuations (seconds)
        float energy = 1.0e19f;      // Energy scale (GeV)
    };
    
private:
    UI4DConfig config;
    
    Camera4D camera;
    std::vector<SliceView> sliceViews;
    CausalGraph causalGraph;
    DiscoveryProbe probe;
    
    std::shared_ptr<MetricTensor> currentMetric;
    std::shared_ptr<CurvatureRenderer> curvatureRenderer;
    std::shared_ptr<QuantumGeometryRenderer> quantumRenderer;  // Quantum geometry visualizer
    
    // Planck Microscope widget (can be embedded or separate window)
    PlanckMicroscope* planckMicroscope;  // Optional: owned by UI layout
    
    // View linking state
    std::set<int> highlightedEvents;      // Events highlighted due to view linking
    std::set<int> causalFutureEvents;     // Events in causal future of selected event
    std::set<int> causalPastEvents;       // Events in causal past of selected event
    
    // Solar system specific data
    struct SolarSystemBody {
        std::string name;
        double mass;           // kg
        double radius;         // meters
        Event4D position;      // current position in spacetime
        Event4D velocity;      // velocity in spacetime (dx/dt, dy/dt, dz/dt)
        double orbitalPeriod;  // seconds
        double semiMajorAxis;  // meters
        std::vector<Event4D> orbitPoints; // pre-calculated orbit points
        bool isCentralBody;    // true for the Sun
        bool isStar;           // true for stars (Sun, other stars)
        bool showOrbit;        // whether to render orbit
        std::string textureId; // for rendering (if using textures)
    };
    
    struct SolarSystemData {
        std::map<std::string, SolarSystemBody> bodies;
        Event4D barycenter;    // Solar system barycenter
        double scaleFactor;    // Scaling factor for visualization
        bool showOrbits = true;
        bool showLabels = true;
        bool showDataPanels = true;
    };
    
    SolarSystemData solarSystem;
    
    // Real-time editing data structures
    struct WorldLinePoint {
        Event4D event;
        double properTime;     // Proper time along the world-line
        bool isControlPoint;   // Whether this is a control point for editing
    };
    
    struct WorldLine {
        std::string name;                  // Name of the object (e.g., "Earth")
        std::vector<WorldLinePoint> points; // Points along the world-line
        bool isClosed;                     // Whether the world-line is closed (periodic)
        double mass;                       // Mass of the object (for gravity)
        bool isSelected;                   // Whether this world-line is currently selected for editing
    };
    
    struct SingularityEditData {
        std::shared_ptr<SingularityHandler> singularity; // The singularity being edited
        bool isPositionEditing;    // Whether we're editing position
        bool isMassEditing;        // Whether we're editing mass
        bool isSpinEditing;        // Whether we're editing spin/angular momentum
        bool isChargeEditing;      // Whether we're editing charge
    };
    
    struct BodyPropertyEditData {
        std::string bodyName;      // Name of the body being edited
        bool isMassEditing;        // Whether we're editing mass
        bool isRadiusEditing;      // Whether we're editing radius
        bool isCompositionEditing; // Whether we're editing composition
        bool isVelocityEditing;    // Whether we're editing velocity
    };
    
    // Editing state
    std::vector<WorldLine> worldLines;           // Collection of world-lines in the scene
    std::shared_ptr<SingularityEditData> editingSingularity; // Currently edited singularity
    std::shared_ptr<BodyPropertyEditData> editingBody;       // Currently edited body
    bool isCreatingEvent;                        // Whether we're in event creation mode
    bool isCreatingWorldLine;                    // Whether we're in world-line creation mode
    std::vector<Event4D> newWorldLinePoints;     // Points for new world-line being created
    
    // UI state
    bool show4DView = true;
    bool showSliceViews = true;
    bool showCausalGraph = false;
    bool showDiscoveryPanel = true;
    bool showSolarSystem = true;
    bool showQuantumGeometry = false;  // Quantum geometry overlay toggle
    
    // Active quantum theory name (e.g., "CDT", "Spin Foam", "GFT", "Causal Set")
    std::string activeQuantumTheory;
    
    // Layout management
    struct Panel {
        bool visible = true;
        bool resizable = true;
        int minWidth = 100;
        int minHeight = 100;
        int width = 400;
        int height = 300;
        int x = 0;
        int y = 0;
        bool isCollapsed = false;
    };
    
    enum class LayoutMode {
        SINGLE_VIEW,      // Only 4D viewport
        SPLIT_VERTICAL,   // Two panels side by side
        SPLIT_HORIZONTAL, // Two panels stacked
        QUAD_VIEW,        // Four equal panels
        CUSTOM            // Custom layout
    };
    
    LayoutMode currentLayout = LayoutMode::QUAD_VIEW;
    Panel panel4DView;
    Panel panelSliceViews;
    Panel panelCausalGraph;
    Panel panelDiscoveryPanel;
    Panel panelSolarSystem;
    
    // Splitter state
    bool isResizing = false;
    int resizePanelIndex = -1; // Index of panel being resized
    int resizeStartX = 0;
    int resizeStartY = 0;
    
    // Selected event
    int selectedEventIndex = -1;
    
    // Mapping from event indices to body names for solar system bodies
     std::map<int, std::string> eventToBodyMap;
     
    // Solar system specific UI state
     std::string selectedBody;  // Currently selected body for detailed view
     bool showOrbitalTrajectories = true;
     bool showBodyMarkers = true;
     bool showDataPanels = true;
     
    // Singularities for anomaly detection
     std::vector<std::shared_ptr<SingularityHandler>> singularities;
     
    // Power-up features data
    std::vector<Waypoint> waypoints;
    int activeWaypointIndex = -1;
    WaypointFlightConfig waypointFlight;
    std::vector<AnomalyAlert> anomalyAlerts;
    double anomalyDetectionThreshold = 1000.0; // Kretschmann scalar threshold
    bool anomalyDetectionEnabled = true;
    CurvatureManipulationTool curvatureTool;
    LensingEffect lensingEffect;
    TimeDilationEffect timeDilationEffect;
    QuantumFoamEffect quantumFoamEffect;

public:
    UI4D() {
        // Initialize with default configuration
        sliceViews.resize(config.numSlices);
        
        // Set up slice views with different modes
        if (config.numSlices >= 1) {
            sliceViews[0].setMode(SlicingMode::FIXED_T);
            sliceViews[0].setParameter(0.0);
        }
        if (config.numSlices >= 2) {
            sliceViews[1].setMode(SlicingMode::FIXED_T);
            sliceViews[1].setParameter(10.0);
        }
        if (config.numSlices >= 3) {
            sliceViews[2].setMode(SlicingMode::NULL_SLICE);
        }
        if (config.numSlices >= 4) {
            sliceViews[3].setMode(SlicingMode::PROPER_TIME);
        }
        
        // Initialize solar system data
        initializeSolarSystem();
        createSolarSystemEventMarkers();
        
        // Initialize editing state
        isCreatingEvent = false;
        isCreatingWorldLine = false;
    }
    
    void setMetric(std::shared_ptr<MetricTensor> metric) {
        currentMetric = metric;
        for (auto& slice : sliceViews) {
            slice.setMetric(metric);
        }
    }
    
    void setCurvatureRenderer(std::shared_ptr<CurvatureRenderer> renderer) {
        curvatureRenderer = renderer;
    }
    
    // Power-up features - Waypoint management
    void addWaypoint(const std::string& name, const Event4D& position, const std::string& description);
    void removeWaypoint(int index);
    void setActiveWaypoint(int index);
    void clearWaypoints();
    
    // Waypoint flight system
    void updateWaypointFlight(float deltaTime);
    void startWaypointFlight();
    void stopWaypointFlight();
    void setWaypointFlightSpeed(float speed);
    void setWaypointFlightLoop(bool loop);
    
    // Camera controls
    void translateCamera(const std::array<double, 4>& delta) {
        camera.translate(delta);
    }
    
    void rotateCamera(int plane1, int plane2, double angle) {
        camera.rotate(plane1, plane2, angle);
    }
    
    void zoomCamera(double factor) {
        camera.zoom(factor);
    }
    
    void setSliceOffset(int viewIndex, double offset) {
        if (viewIndex >= 0 && viewIndex < (int)sliceViews.size()) {
            sliceViews[viewIndex].setParameter(offset);
        }
    }
    
    // Event management
    int addEvent(const Event4D& event) {
        return causalGraph.addEvent(event);
    }
    
    void selectEvent(int index) {
        if (index >= 0 && index < (int)causalGraph.getNodes().size()) {
            selectedEventIndex = index;
            
            // Move camera to selected event
            const auto& event = causalGraph.getNodes()[index].event;
            camera = Camera4D(event, Event4D(event[0] + 1.0, event[1], event[2], event[3]));
        }
    }
    
    // Probe controls
    void moveProbeTo(const Event4D& position) {
        probe.setPosition(position);
    }
    
    // Solar system specific methods
    void initializeSolarSystem();
    void updateSolarSystemPositions(double currentTime);
    void renderSolarSystem();
    void calculateOrbitalTrajectory(const SolarSystemBody& body, int points = 100);
    void renderPlanetaryDataPanel(const std::string& bodyName);
    void createSolarSystemEventMarkers();
     void setSolarSystemScale(double scale);
     void toggleOrbitalTrajectories(bool show) { showOrbitalTrajectories = show; }
     void toggleBodyMarkers(bool show) { showBodyMarkers = show; }
     void toggleDataPanels(bool show) { showDataPanels = show; }
 
     // Anomaly detection methods
      void detectCurvatureAnomalies();
      void acknowledgeAnomaly(int index);
      void clearAnomalies();
      void setAnomalyDetectionThreshold(double threshold);
      void toggleAnomalyDetection(bool enable);
      void addSingularity(std::shared_ptr<SingularityHandler> singularity);
 
      // Spacetime curvature manipulation methods
      void activateCurvatureManipulation(const Event4D& position, CurvatureManipulationTool::Type type);
      void deactivateCurvatureManipulation();
      void setCurvatureManipulationRadius(float radius);
      void setCurvatureManipulationStrength(float strength);
 
      // Gravitational lensing visualization methods
      void enableLensingEffect(bool enable);
      void setLensingNumRings(int numRings);
      void setLensingRingSpacing(float spacing);
      void setLensingIntensity(float intensity);
 
      // Time dilation visualization methods
      void enableTimeDilationEffect(bool enable);
      void setTimeDilationRange(float minDilation, float maxDilation);
      void setTimeDilationShowGrid(bool showGrid);
 
      // Quantum foam visualization methods
      void enableQuantumFoamEffect(bool enable);
      void setQuantumFoamScale(float scale);
      void setQuantumFoamFluctuationCount(int count);
      void setQuantumFoamLifetime(float lifetime);
      void setQuantumFoamEnergy(float energy);
 
     // Real-time event editing
     void startEventCreation(const Event4D& position);
     void endEventCreation();
     void deleteSelectedEvent();
     
     // Real-time world-line editing
     void startWorldLineCreation(const Event4D& position);
     void addWorldLinePoint(const Event4D& position);
     void endWorldLineCreation();
     void editWorldLineControlPoint(int worldLineIndex, int pointIndex, const Event4D& newPosition);
     void deleteWorldLine(int worldLineIndex);
     
     // Real-time singularity editing
     void startSingularityEditing(std::shared_ptr<SingularityHandler> singularity);
     void editSingularityPosition(const std::array<double, 3>& newPosition);
     void editSingularityMass(double newMass);
     void editSingularitySpin(double newAngularMomentum);
     void editSingularityCharge(double newCharge);
     void endSingularityEditing();
     
     // Real-time body property editing
     void startBodyPropertyEditing(const std::string& bodyName);
     void editBodyMass(double newMass);
     void editBodyRadius(double newRadius);
     void editBodyComposition(const std::string& composition);
     void editBodyVelocity(const Event4D& newVelocity);
     void endBodyPropertyEditing();
     
     // Rendering
     void render() {
         // Render 4D view with quantum geometry if active
         if (show4DView) {
             if (quantumRenderer && quantumRenderer->getCurrentType() != QuantumGeometryType::NONE) {
                 // Use quantum renderer
                 // quantumRenderer->render(camera, sliceViews, *currentMetric);
             } else if (curvatureRenderer) {
                 // Use classical curvature renderer
                 // curvatureRenderer->render(camera, sliceViews, *currentMetric);
             }
         }
 
         // Render slice views
         if (showSliceViews) {
             for (size_t i = 0; i < sliceViews.size(); i++) {
                 // Render each slice
             }
         }
 
         // Render solar system if enabled
         if (showSolarSystem) {
             renderSolarSystem();
         }
 
         // Render causal graph
         if (showCausalGraph) {
             // Render causal relations
         }
 
         // Render discovery panel
         if (showDiscoveryPanel) {
             renderDiscoveryPanel();
         }
     }
     
     void renderDiscoveryPanel() {
         // Display curvature samples, causal info, etc.
         if (selectedEventIndex >= 0) {
             const auto& node = causalGraph.getNodes()[selectedEventIndex];
             
             // Sample curvature at selected event
             auto curvature = probe.sampleCurvature(currentMetric);
             
             // Display information
             // (In actual implementation, would render to screen)
         }
         
         // Render selected body data panel if applicable
         if (showDataPanels && !selectedBody.empty() && 
             solarSystem.bodies.find(selectedBody) != solarSystem.bodies.end()) {
             renderPlanetaryDataPanel(selectedBody);
         }
     }
     
     // Getters
     const Camera4D& getCamera() const { return camera; }
     const CausalGraph& getCausalGraph() const { return causalGraph; }
     const DiscoveryProbe& getProbe() const { return probe; }
     const SolarSystemData& getSolarSystemData() const { return solarSystem; }
 
     // Slice view access for renderers
     const std::vector<SliceView>& getSliceViews() const { return sliceViews; }
     std::shared_ptr<MetricTensor> getCurrentMetric() const { return currentMetric; }
     
     // UI state
     void toggle4DView() { show4DView = !show4DView; }
     void toggleSliceViews() { showSliceViews = !showSliceViews; }
     void toggleCausalGraph() { showCausalGraph = !showCausalGraph; }
     void toggleDiscoveryPanel() { showDiscoveryPanel = !showDiscoveryPanel; }
     void toggleSolarSystem() { showSolarSystem = !showSolarSystem; }
     
     // Visualization mode controls
     void toggleLightCones() { config.showLightCones = !config.showLightCones; }
     void toggleCausalStructure() { config.showCausalStructure = !config.showCausalStructure; }
     void toggleCurvatureGrid() { config.showCurvatureGrid = !config.showCurvatureGrid; }
     void toggleWorldLines() { config.showWorldLines = !config.showWorldLines; }
     void toggleTimeGeography() { config.showTimeGeography = !config.showTimeGeography; }
     void toggleTopologyIndicators() { config.showTopologyIndicators = !config.showTopologyIndicators; }
     
     // Slice view controls
     void setSliceMode(int viewIndex, SlicingMode mode);
     void setSliceParameter(int viewIndex, double parameter);
     void nextSliceMode(int viewIndex);
     void previousSliceMode(int viewIndex);
     
     // Layout management
     void setLayoutMode(LayoutMode mode);
     LayoutMode getLayoutMode() const { return currentLayout; }
     void togglePanelVisibility(int panelIndex);
     void collapsePanel(int panelIndex);
     void expandPanel(int panelIndex);
     void setPanelPosition(int panelIndex, int x, int y);
     void setPanelSize(int panelIndex, int width, int height);
     void startResize(int panelIndex, int startX, int startY);
     void updateResize(int currentX, int currentY);
     void endResize();
     
     // Configuration getters
     const UI4DConfig& getConfig() const { return config; }
     UI4DConfig& getConfig() { return config; }
 
     // Quantum Gravity Integration
     void setQuantumRenderer(std::shared_ptr<QuantumGeometryRenderer> renderer);
     void setPlanckMicroscope(PlanckMicroscope* microscope);
     PlanckMicroscope* getPlanckMicroscope() const { return planckMicroscope; }
 
     // Theory selection (delegates to TheoryManager if available)
     void setActiveQuantumTheory(const std::string& theoryName);
     std::string getActiveQuantumTheory() const;
 
     // Toggle quantum geometry overlay
     void toggleQuantumGeometry(bool enable);
     bool isQuantumGeometryEnabled() const {
         return quantumRenderer && quantumRenderer->getCurrentType() != QuantumGeometryType::NONE;
     }
 };
 
 } // namespace quantumverse
 
 #endif // QUANTUMVERSE_UI4D_H