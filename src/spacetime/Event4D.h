#ifndef QUANTUMVERSE_EVENT4D_H
#define QUANTUMVERSE_EVENT4D_H

/**
 * QuantumVerse Simulator - 4D Spacetime Event
 * 
 * Represents an event in 4-dimensional Lorentzian manifold (spacetime)
 * Coordinates: (t, x, y, z) where t is temporal and (x,y,z) are spatial
 * 
 * Based on: Spacetime.md - Fundamentals of Spacetime
 * Reference: Minkowski space formulation
 */

// Enable M_PI on MSVC
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <cmath>
#include <array>
#include <ostream>

namespace quantumverse {

class Event4D {
public:
    // Coordinates: (t, x, y, z)
    double t;  ///< Time coordinate (seconds)
    double x;  ///< X spatial coordinate (meters)
    double y;  ///< Y spatial coordinate (meters)
    double z;  ///< Z spatial coordinate (meters)

    // Speed of light in vacuum (m/s)
    static constexpr double C = 299792458.0;
    static constexpr double C2 = C * C;
    
    // Gravitational constant (m³ kg⁻¹ s⁻²)
    static constexpr double G = 6.67430e-11;

    // Default constructor - origin of spacetime
    Event4D() : t(0.0), x(0.0), y(0.0), z(0.0) {}

    // Parameterized constructor
    Event4D(double t_, double x_, double y_, double z_) 
        : t(t_), x(x_), y(y_), z(z_) {}

    // Copy constructor
    Event4D(const Event4D&) = default;
    
    // Copy assignment
    Event4D& operator=(const Event4D&) = default;
    
    // Move constructor
    Event4D(Event4D&&) = default;

    // Spacetime interval squared (Minkowski metric, mostly-minus convention)
    // ds² = c²dt² - dx² - dy² - dz²
    double intervalSquared(const Event4D& other) const {
        double dt = other.t - t;
        double dx = other.x - x;
        double dy = other.y - y;
        double dz = other.z - z;
        return C2 * dt * dt - dx * dx - dy * dy - dz * dz;
    }

    // Proper time between events (for timelike separated events)
    // dτ = √(dt² - dx²/c² - dy²/c² - dz²/c²)
    double properTime(const Event4D& other) const {
        double ds2 = intervalSquared(other);
        if (ds2 > 0) {
            return std::sqrt(ds2) / C;  // Proper time in seconds
        }
        return 0.0;  // Spacelike or lightlike separation
    }

    // Spatial distance (Euclidean, for simultaneous events)
    double spatialDistance(const Event4D& other) const {
        double dx = other.x - x;
        double dy = other.y - y;
        double dz = other.z - z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    // Spatial length from origin: r = sqrt(x² + y² + z²)
    double spatialLength() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    // Polar angle θ (from z-axis), range [0, π]
    double theta() const {
        double r = spatialLength();
        if (r < 1e-15) return 0.0;
        return std::acos(z / r);
    }

    // Azimuthal angle φ (from x-axis in xy-plane), range [0, 2π)
    double phi() const {
        return std::atan2(y, x);
    }

    // Check if events are timelike separated
    bool isTimelike(const Event4D& other) const {
        return intervalSquared(other) > 0;
    }

    // Check if events are lightlike (null) separated
    bool isLightlike(const Event4D& other) const {
        double ds2 = intervalSquared(other);
        return std::abs(ds2) < 1e-10;  // Numerical tolerance
    }

    // Check if events are spacelike separated
    bool isSpacelike(const Event4D& other) const {
        return intervalSquared(other) < 0;
    }

    // Lorentz transformation to another inertial frame
    // boostVelocity in x-direction (m/s)
    Event4D lorentzTransform(double boostVelocity) const {
        double beta = boostVelocity / C;
        double gamma = 1.0 / std::sqrt(1.0 - beta * beta);
        
        double t_prime = gamma * (t - boostVelocity * x / C2);
        double x_prime = gamma * (x - boostVelocity * t);
        
        return Event4D(t_prime, x_prime, y, z);
    }

    // Operator overloads
    Event4D operator+(const Event4D& other) const {
        return Event4D(t + other.t, x + other.x, y + other.y, z + other.z);
    }

    Event4D operator-(const Event4D& other) const {
        return Event4D(t - other.t, x - other.x, y - other.y, z - other.z);
    }

    Event4D operator*(double scalar) const {
        return Event4D(t * scalar, x * scalar, y * scalar, z * scalar);
    }

    bool operator==(const Event4D& other) const {
        return t == other.t && x == other.x && y == other.y && z == other.z;
    }

    bool operator!=(const Event4D& other) const {
        return !(*this == other);
    }

    // Array access
    double& operator[](int i) {
        switch(i) {
            case 0: return t;
            case 1: return x;
            case 2: return y;
            case 3: return z;
            default: return t;  // Should throw exception
        }
    }

    const double& operator[](int i) const {
        switch(i) {
            case 0: return t;
            case 1: return x;
            case 2: return y;
            case 3: return z;
            default: return t;  // Should throw exception
        }
    }
};

// Output stream operator
inline std::ostream& operator<<(std::ostream& os, const Event4D& event) {
    os << "Event4D(t=" << event.t << ", x=" << event.x 
       << ", y=" << event.y << ", z=" << event.z << ")";
    return os;
}

} // namespace quantumverse

#endif // QUANTUMVERSE_EVENT4D_H