#ifndef QUANTUMVERSE_SINGULARITY_HANDLER_H
#define QUANTUMVERSE_SINGULARITY_HANDLER_H

/**
 * QuantumVerse Simulator - Gravitational Singularity Handler
 * 
 * Manages black hole singularities with event horizon detection,
 * tidal force calculations, and geodesic termination
 * 
 * Based on: Gravitational_singularity.md, Curved_spacetime.md
 * Types: Schwarzschild, Kerr, Reissner-Nordström, Naked singularities
 */

// Define _USE_MATH_DEFINES BEFORE including cmath for MSVC compatibility
#define _USE_MATH_DEFINES
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <cmath>
#include <limits>

// Fallback M_PI definition for platforms where _USE_MATH_DEFINES doesn't work
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <vector>
#include <memory>
#include <array>
#include "../spacetime/Event4D.h"
#include "../spacetime/MetricTensor.h"

namespace quantumverse {

// Singularity type enumeration
enum class SingularityType {
    SCHWARZSCHILD,      ///< Non-rotating, uncharged
    KERR,               ///< Rotating (J ≠ 0)
    REISSNER_NORDSTROM,  ///< Charged (Q ≠ 0)
    KERR_NEWMAN,        ///< Rotating and charged
    NAKED,              ///< Super-extremal (J > M² or Q > M)
    FUZZBALL,           ///< String theory regular black hole
    POINT,              ///< Point singularity (simplified)
    // Quantum-corrected regular black holes (Phase 1.6)
    HAYWARD,            ///< Hayward regular black hole (minimal length)
    BARDEEN,            ///< Bardeen regular black hole (magnetic charge)
    LOOP_QUANTUM,       ///< Loop quantum gravity black hole (polymer correction)
    REGULAR_GAUSS       ///< Gaussian-sourced regular black hole
};

// Singularity properties
struct SingularityProperties {
    SingularityType type;
    double mass;                    ///< Mass in kg
    double angular_momentum;        ///< J in kg·m²/s
    double charge;                  ///< Charge in Coulombs
    double schwarzschild_radius;    ///< r_s = 2GM/c²
    double event_horizon_radius;    ///< Outer horizon radius
    double inner_horizon_radius;    ///< Inner horizon (Cauchy horizon)
    double ergosphere_radius;       ///< Ergosphere boundary (equatorial)
    bool has_ergosphere;
    bool is_naked;                  ///< No event horizon
    
    SingularityProperties() 
        : type(SingularityType::SCHWARZSCHILD),
          mass(1.0),
          angular_momentum(0.0),
          charge(0.0),
          schwarzschild_radius(0.0),
          event_horizon_radius(0.0),
          inner_horizon_radius(0.0),
          ergosphere_radius(0.0),
          has_ergosphere(false),
          is_naked(false) {}
};

// Tidal force information
struct TidalForce {
    double radial_stretch;      ///< Radial tidal acceleration (m/s² per meter)
    double lateral_compression; ///< Lateral tidal acceleration (m/s² per meter)
    double spaghettification;   ///< Spaghettification factor
    bool lethal;                ///< Tidal forces lethal to humans
    
    TidalForce() 
        : radial_stretch(0.0), lateral_compression(0.0), 
          spaghettification(0.0), lethal(false) {}
};

class SingularityHandler {
private:
    SingularityProperties props;
    std::array<double, 3> position;  ///< Singularity center position (x, y, z)
    MetricTensor localMetric;        ///< Local metric near singularity
    
    // Compute Schwarzschild radius
    double computeSchwarzschildRadius(double mass) const {
        // r_s = 2GM / c^2  (standard Schwarzschild radius formula)
        return 2.0 * Event4D::G * mass / (Event4D::C * Event4D::C);
    }
    
    // Compute event horizon radius for Kerr black hole
    double computeKerrEventHorizon(double mass, double angular_momentum) const {
        double rs = computeSchwarzschildRadius(mass);
        double a = angular_momentum / (mass * Event4D::C);  // Dimensionless spin
        return rs / 2.0 * (1.0 + std::sqrt(1.0 - a * a));
    }
    
    // Compute ergosphere radius (equatorial)
    double computeErgosphereRadius(double mass, double angular_momentum) const {
        double rs = computeSchwarzschildRadius(mass);
        double a = angular_momentum / (mass * Event4D::C);
        return rs / 2.0 * (1.0 + std::sqrt(1.0 - a * a * std::cos(0.0) * std::cos(0.0)));
    }
    
    // Check if singularity is naked (no event horizon)
    bool checkNakedSingularity(double mass, double angular_momentum, double charge) const {
        double rs = computeSchwarzschildRadius(mass);
        double a = angular_momentum / (mass * Event4D::C);
        double q = charge * charge / (4.0 * M_PI * 8.854187817e-12 * Event4D::C2 * mass);
        
        // Condition for naked singularity: J > M² or Q > M (in geometric units)
        return (a > 1.0) || (q > 1.0);
    }

public:
    // Constructor
    SingularityHandler(
        SingularityType type_,
        double mass_,
        double angular_momentum_ = 0.0,
        double charge_ = 0.0,
        const std::array<double, 3>& pos = {0.0, 0.0, 0.0}
    ) : position(pos) {
        props.type = type_;
        props.mass = mass_;
        props.angular_momentum = angular_momentum_;
        props.charge = charge_;
        
        // Compute properties
        props.schwarzschild_radius = computeSchwarzschildRadius(mass_);
        props.is_naked = checkNakedSingularity(mass_, angular_momentum_, charge_);
        
        if (type_ == SingularityType::KERR || type_ == SingularityType::KERR_NEWMAN) {
            props.event_horizon_radius = computeKerrEventHorizon(mass_, angular_momentum_);
            props.ergosphere_radius = computeErgosphereRadius(mass_, angular_momentum_);
            props.has_ergosphere = true;
            props.inner_horizon_radius = props.schwarzschild_radius / 2.0;  // Cauchy horizon
        } else if (type_ == SingularityType::SCHWARZSCHILD) {
            props.event_horizon_radius = props.schwarzschild_radius;
            props.has_ergosphere = false;
        } else if (type_ == SingularityType::REISSNER_NORDSTROM) {
            // RN horizon: r+ = M + sqrt(M² - Q²) in geometric units
            // In SI: r+ = r_s/2 * (1 + sqrt(1 - q²)) where q = Q²/(4πε₀c²M)
            double q = charge_ * charge_ / (4.0 * M_PI * 8.854187817e-12 * Event4D::C2 * mass_);
            props.event_horizon_radius = props.schwarzschild_radius / 2.0 * (1.0 + std::sqrt(1.0 - q * q));
            props.has_ergosphere = false;
            props.inner_horizon_radius = props.schwarzschild_radius / 2.0 * (1.0 - std::sqrt(1.0 - q * q));
        } else if (type_ == SingularityType::NAKED) {
            props.is_naked = true;
            props.has_ergosphere = false;
        } else if (type_ == SingularityType::HAYWARD || 
                   type_ == SingularityType::BARDEEN || 
                   type_ == SingularityType::LOOP_QUANTUM ||
                   type_ == SingularityType::REGULAR_GAUSS) {
            // Regular black holes have event horizons (though modified)
            props.event_horizon_radius = props.schwarzschild_radius;
            props.has_ergosphere = false;
        }
        
        // Create local metric
        updateLocalMetric();
    }
    
    // Update local metric based on singularity type
    void updateLocalMetric() {
        if (props.type == SingularityType::KERR || props.type == SingularityType::KERR_NEWMAN) {
            // Use Kerr metric
            double r = props.event_horizon_radius * 2.0;  // Sample at 2x horizon
            double theta = M_PI / 2.0;  // Equatorial plane
            localMetric = MetricTensor::kerr(props.mass, props.angular_momentum, r, theta);
        } else {
            // Use Schwarzschild metric
            double r = props.event_horizon_radius * 2.0;
            double theta = M_PI / 2.0;
            double phi = 0.0;
            localMetric = MetricTensor::schwarzschild(props.mass, r, theta, phi);
        }
    }
    
    // Check if event is inside event horizon
    bool isInsideEventHorizon(const Event4D& event) const {
        if (props.is_naked) return false;  // No event horizon
        
        double dx = event.x - position[0];
        double dy = event.y - position[1];
        double dz = event.z - position[2];
        double r = std::sqrt(dx * dx + dy * dy + dz * dz);
        
        return r < props.event_horizon_radius;
    }
    
    // Check if event is in ergosphere
    bool isInErgosphere(const Event4D& event) const {
        if (!props.has_ergosphere) return false;
        
        double dx = event.x - position[0];
        double dy = event.y - position[1];
        double dz = event.z - position[2];
        double r = std::sqrt(dx * dx + dy * dy + dz * dz);
        
        return r < props.ergosphere_radius && r > props.event_horizon_radius;
    }
    
    // Compute tidal forces at given position
    TidalForce computeTidalForces(const Event4D& event) const {
        TidalForce force;
        
        double dx = event.x - position[0];
        double dy = event.y - position[1];
        double dz = event.z - position[2];
        double r = std::sqrt(dx * dx + dy * dy + dz * dz);
        
        if (r < props.schwarzschild_radius * 0.1) {
            // Very close to singularity - extreme forces
            force.radial_stretch = 1e12 * Event4D::C2 / props.schwarzschild_radius;
            force.lateral_compression = -0.5 * force.radial_stretch;
            force.spaghettification = 10.0;  // Extreme
            force.lethal = true;
        } else if (r < props.event_horizon_radius * 2.0) {
            // Near event horizon
            force.radial_stretch = Event4D::C2 * props.schwarzschild_radius / (r * r * r);
            force.lateral_compression = -0.5 * force.radial_stretch;
            force.spaghettification = force.radial_stretch / 100.0;
            force.lethal = force.spaghettification > 1000.0;
        } else {
            // Far from singularity - Newtonian approximation
            force.radial_stretch = 2.0 * Event4D::C2 * props.schwarzschild_radius / (r * r * r);
            force.lateral_compression = -force.radial_stretch / 2.0;
            force.spaghettification = force.radial_stretch / 1000.0;
            force.lethal = false;
        }
        
        return force;
    }
    
    // Check if geodesic should terminate at singularity
    bool shouldTerminateGeodesic(const Event4D& event, double proper_time) const {
        // Terminate if inside event horizon and past Cauchy horizon
        if (isInsideEventHorizon(event)) {
            double dx = event.x - position[0];
            double dy = event.y - position[1];
            double dz = event.z - position[2];
            double r = std::sqrt(dx * dx + dy * dy + dz * dz);
            
            // Terminate at singularity (r ≈ 0) or after reasonable proper time
            return r < props.schwarzschild_radius * 0.01 || proper_time > 1000.0;
        }
        
        return false;
    }
    
    // Get metric tensor near singularity
    MetricTensor getLocalMetric() const {
        return localMetric;
    }
    
    // Get singularity properties
    const SingularityProperties& getProperties() const {
        return props;
    }
    
    // Get position
    const std::array<double, 3>& getPosition() const {
        return position;
    }
    
    // Set position
    void setPosition(const std::array<double, 3>& new_pos) {
        position = new_pos;
    }
    
    // Compute gravitational redshift factor
    double getGravitationalRedshift(const Event4D& event) const {
        double dx = event.x - position[0];
        double dy = event.y - position[1];
        double dz = event.z - position[2];
        double r = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (r <= props.schwarzschild_radius) {
            return 0.0;  // Infinite redshift at horizon
        }

        return std::sqrt(1.0 - props.schwarzschild_radius / r);
    }

    // Compute frame-dragging angular velocity (Kerr only)
    double getFrameDraggingAngularVelocity(const Event4D& event) const {
        if (props.type != SingularityType::KERR && props.type != SingularityType::KERR_NEWMAN) {
            return 0.0;
        }

        double dx = event.x - position[0];
        double dy = event.y - position[1];
        double dz = event.z - position[2];
        double r = std::sqrt(dx * dx + dy * dy + dz * dz);
        double theta = std::acos(dz / r);

        double a = props.angular_momentum / (props.mass * Event4D::C);
        double rs = props.schwarzschild_radius;

        // Lense-Thirring precession
        return 2.0 * Event4D::C * a * rs / (r * r * r * (1.0 + a * a / (r * r)));
    }

    // ========== Quantum-Corrected Regular Black Holes (Phase 1.6) ==========

    /**
      * Hayward Regular Black Hole Metric
      * f(r) = 1 - (2M r²)/(r³ + 2M L²)
      * where M is the Schwarzschild radius (r_s = 2GM/c²) and L is Planck length
      * No curvature singularity at r=0; de Sitter core.
      */
    MetricTensor getHaywardMetric(double r, double theta, double phi) const {
        double M = props.schwarzschild_radius;  // Use r_s instead of mass in kg
        double L = 1.616e-35;  // Planck length

        double f = 1.0 - (2.0 * M * r * r) / (r * r * r + 2.0 * M * L * L);

        MetricTensor metric;
        // Diagonal metric in spherical coordinates (t, r, θ, φ)
        metric.g[0][0] = -f;  // g_tt
        metric.g[1][1] = 1.0 / f;  // g_rr
        metric.g[2][2] = r * r;    // g_θθ
        metric.g[3][3] = r * r * std::sin(theta) * std::sin(theta);  // g_φφ

        // Off-diagonals zero
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (i != j) metric.g[i][j] = 0.0;
            }
        }

        return metric;
    }

    /**
      * Bardeen Regular Black Hole Metric
      * f(r) = 1 - (2M r²)/(r³ + g²)
      * where g is a length scale related to magnetic charge: g² = Q² M (in geometric units)
      * Regular core sourced by nonlinear electrodynamics (Born-Infeld).
      */
    MetricTensor getBardeenMetric(double r, double theta, double phi) const {
        double M = props.schwarzschild_radius;  // Use r_s instead of mass in kg
        double Q = props.charge;  // Magnetic charge parameter (length scale)

        // g² = Q² M in geometric units, but we need to scale for SI
        double g_squared = Q * Q * M;
        double denominator = r * r * r + g_squared;
        double f = 1.0 - (2.0 * M * r * r) / denominator;

        MetricTensor metric;
        metric.g[0][0] = -f;
        metric.g[1][1] = 1.0 / f;
        metric.g[2][2] = r * r;
        metric.g[3][3] = r * r * std::sin(theta) * std::sin(theta);

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (i != j) metric.g[i][j] = 0.0;
            }
        }

        return metric;
    }

    /**
      * Loop Quantum Black Hole Metric
      * Based on Ashtekar-Olmedo-Singh effective metric:
      * f(r) = (1 - 2M/ρ) / (1 + ρ²/L²)
      * where ρ = r (1 + r²/L²)^(1/2) is quantum-corrected radial function
      * M is the Schwarzschild radius (r_s = 2GM/c²), L = Planck length
      * Replaces singularity with quantum bounce.
      */
    MetricTensor getLoopQuantumMetric(double r, double theta, double phi) const {
        double M = props.schwarzschild_radius;  // Use r_s instead of mass in kg
        double L = 1.616e-35;  // Planck length

        // Quantum-corrected radial coordinate
        double rho = r * std::sqrt(1.0 + r * r / (L * L));

        double f = (1.0 - 2.0 * M / rho) / (1.0 + rho * rho / (L * L));

        MetricTensor metric;
        metric.g[0][0] = -f;
        metric.g[1][1] = 1.0 / f;
        metric.g[2][2] = r * r;
        metric.g[3][3] = r * r * std::sin(theta) * std::sin(theta);

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (i != j) metric.g[i][j] = 0.0;
            }
        }

        return metric;
    }

    /**
     * Get metric for regular black hole based on type
     */
    MetricTensor getRegularBlackHoleMetric(double r, double theta, double phi) const {
        switch (props.type) {
            case SingularityType::HAYWARD:
                return getHaywardMetric(r, theta, phi);
            case SingularityType::BARDEEN:
                return getBardeenMetric(r, theta, phi);
            case SingularityType::LOOP_QUANTUM:
                return getLoopQuantumMetric(r, theta, phi);
            case SingularityType::REGULAR_GAUSS:
                return getGaussMetric(r, theta, phi);
            default:
                // Fall back to classical metric
                if (props.type == SingularityType::KERR || props.type == SingularityType::KERR_NEWMAN) {
                    return MetricTensor::kerr(props.mass, props.angular_momentum, r, theta);
                } else {
                    return MetricTensor::schwarzschild(props.mass, r, theta, phi);
                }
        }
    }

    /**
      * Gaussian-sourced regular black hole
      * Matter source: T_μν ~ exp(-r²/σ²) gives regular core
      */
    MetricTensor getGaussMetric(double r, double theta, double phi) const {
        double M = props.schwarzschild_radius;  // Use r_s instead of mass in kg
        double sigma = 1.0 * 1.616e-35;  // Gaussian width ~ Planck length

        // Approximate effective mass function
        double M_eff = M * std::erf(r / sigma);  // Error function regularization

        double f = 1.0 - 2.0 * M_eff / r;

        MetricTensor metric;
        metric.g[0][0] = -f;
        metric.g[1][1] = 1.0 / f;
        metric.g[2][2] = r * r;
        metric.g[3][3] = r * r * std::sin(theta) * std::sin(theta);

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (i != j) metric.g[i][j] = 0.0;
            }
        }

        return metric;
    }

    /**
     * Hawking Temperature
     * T_H = κ / (2π) where κ is surface gravity
     * For regular black holes, temperature goes to zero as M→0 (remnant)
     */
    double getHawkingTemperature() const {
        if (props.type == SingularityType::HAYWARD) {
            double L = 1.616e-35;
            double r_h = getHaywardHorizonRadius();
            // κ = (r_h - r_+) / (2 r_h r_+) where r_+ is outer horizon
            // Simplified: T ~ 1/(8πM) for large M, but saturates at T_max ~ 1/(2πL)
            return 1.0 / (8.0 * M_PI * props.mass) * (1.0 + std::pow(L / (2.0 * props.mass), 2));
        } else if (props.type == SingularityType::BARDEEN) {
            // Bardeen BH temperature: T ~ 0 for M→0, peaks then decreases
            return 1.0 / (4.0 * M_PI * props.mass) * (1.0 - std::pow(props.charge, 2) / (2.0 * props.mass * props.mass));
        } else if (props.type == SingularityType::LOOP_QUANTUM) {
            // Loop quantum BH: T = 0 at minimum mass (Planck mass)
            double M_planck = 2.176e-8;  // Planck mass in kg
            if (props.mass <= M_planck) return 0.0;
            double T_classical = 1.0 / (8.0 * M_PI * props.mass);
            double correction = 1.0 - std::pow(M_planck / props.mass, 2);
            return T_classical * correction;
        } else {
            // Classical Schwarzschild: T = 1/(8πM) (in geometric units)
            return Event4D::C2 * Event4D::C / (8.0 * M_PI * Event4D::G * props.mass);
        }
    }

    /**
     * Hawking evaporation rate (mass loss)
     * dM/dt = -α / M²  (for Schwarzschild, α ≈ ħ c⁴ / (15360 π G²))
     * For regular BHs, evaporation stops at Planck mass remnant.
     */
    double getEvaporationRate() const {
        double T = getHawkingTemperature();
        // Power ~ A T⁴, A ~ r_h², for BH: dM/dt ~ -1/M² (Schwarzschild)
        double alpha = 3.561e25;  // ħ c⁴ / (15360 π G²) in SI (kg³/s)
        double rate = -alpha / (props.mass * props.mass * props.mass);

        // For regular BHs, rate goes to zero near Planck mass
        if (props.type != SingularityType::SCHWARZSCHILD && props.type != SingularityType::KERR) {
            double M_planck = 2.176e-8;
            if (props.mass < 10 * M_planck) {
                rate *= std::exp(-(M_planck / props.mass));  // Suppressed near Planck mass
            }
        }

        return rate;
    }

    /**
     * Evolve black hole mass due to Hawking radiation
     * dt in seconds
     */
    void evolveHawkingEvaporation(double dt) {
        double dMdt = getEvaporationRate();
        props.mass += dMdt * dt;

        if (props.mass < 0) props.mass = 0;

        // Recompute properties
        props.schwarzschild_radius = computeSchwarzschildRadius(props.mass);
        updateLocalMetric();
    }

    /**
     * Check if black hole has evaporated completely
     */
    bool isEvaporated() const {
        double M_planck = 2.176e-8;
        return props.mass < 1.1 * M_planck;  // Near Planck mass remnant
    }

    /**
      * Get Hayward horizon radius (outer root of f(r)=0)
      * For large M, r_h ≈ r_s (Schwarzschild radius)
      * For small M near Planck scale, quantum correction becomes significant
      */
    double getHaywardHorizonRadius() const {
        double M = props.schwarzschild_radius;  // Use r_s instead of mass in kg
        double L = 1.616e-35;  // Planck length
        // Solve r³ + 2ML² - 2Mr² = 0
        // In geometric units: r_h ≈ 2M - (2/3) L²/M  (perturbative for M >> L)
        // For large M, the correction is negligible
        double correction = (2.0/3.0) * L * L / M;
        return M - correction;
    }

    /**
     * Check if curvature is finite at r=0 (regular BH)
     */
    bool isCurvatureFiniteAtOrigin() const {
        return (props.type == SingularityType::HAYWARD ||
                props.type == SingularityType::BARDEEN ||
                props.type == SingularityType::LOOP_QUANTUM ||
                props.type == SingularityType::REGULAR_GAUSS);
    }

    /**
      * Compute Kretschmann scalar at radius r
      * For regular BHs, K(0) is finite.
      */
    double computeKretschmannAtRadius(double r) const {
        if (props.type == SingularityType::HAYWARD) {
            // Hayward: K ~ 48 M² / (r⁶) * (1 + O(r²/L²))
            // At r=0: K(0) = 48 / (L⁴) (finite!)
            double L = 1.616e-35;
            if (r < 1e-10) {
                return 48.0 / (L * L * L * L);  // Finite Planck-scale curvature
            }
            double M = props.schwarzschild_radius;  // Use r_s instead of mass in kg
            return 48.0 * M * M / (r * r * r * r * r * r);
        } else if (props.type == SingularityType::BARDEEN) {
            // Bardeen: K(0) = 48 Q⁴ / M⁶ (finite)
            double Q = props.charge;
            double M3 = props.mass * props.mass * props.mass;
            return 48.0 * Q * Q * Q * Q / (M3 * M3);
        } else {
            // Classical: K ~ 48 M² / r⁶ → ∞ as r→0
            double M = props.schwarzschild_radius;  // Use r_s instead of mass in kg
            return 48.0 * M * M / (r * r * r * r * r * r);
        }
    }
};

// Singularity editor for user interaction
class SingularityEditor {
private:
    std::vector<std::shared_ptr<SingularityHandler>> singularities;
    
public:
    // Create new singularity
    std::shared_ptr<SingularityHandler> createSingularity(
        SingularityType type,
        double mass,
        double angular_momentum = 0.0,
        double charge = 0.0,
        const std::array<double, 3>& position = {0.0, 0.0, 0.0}
    ) {
        auto singularity = std::make_shared<SingularityHandler>(
            type, mass, angular_momentum, charge, position
        );
        singularities.push_back(singularity);
        return singularity;
    }
    
    // Remove singularity
    void removeSingularity(std::shared_ptr<SingularityHandler> singularity) {
        auto it = std::find(singularities.begin(), singularities.end(), singularity);
        if (it != singularities.end()) {
            singularities.erase(it);
        }
    }
    
    // Get all singularities
    const std::vector<std::shared_ptr<SingularityHandler>>& getAllSingularities() const {
        return singularities;
    }
    
    // Find nearest singularity to event
    std::shared_ptr<SingularityHandler> findNearestSingularity(const Event4D& event) const {
        if (singularities.empty()) return nullptr;
        
        std::shared_ptr<SingularityHandler> nearest = nullptr;
        double min_distance = std::numeric_limits<double>::max();
        
        for (const auto& singularity : singularities) {
            const auto& pos = singularity->getPosition();
            double dx = event.x - pos[0];
            double dy = event.y - pos[1];
            double dz = event.z - pos[2];
            double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            
            if (distance < min_distance) {
                min_distance = distance;
                nearest = singularity;
            }
        }
        
        return nearest;
    }
    
    // Check if any geodesic should terminate
    bool shouldTerminateAnyGeodesic(const Event4D& event, double proper_time) const {
        for (const auto& singularity : singularities) {
            if (singularity->shouldTerminateGeodesic(event, proper_time)) {
                return true;
            }
        }
        return false;
    }
    
    // Get combined metric from all singularities (superposition approximation)
    MetricTensor getCombinedMetric(const Event4D& event) const {
        // Start with Minkowski metric
        MetricTensor combined;
        
        // Add perturbations from each singularity (weak field approximation)
        for (const auto& singularity : singularities) {
            MetricTensor local = singularity->getLocalMetric();
            // Simplified superposition - in practice would need full nonlinear treatment
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    // Average metrics (placeholder for proper superposition)
                    combined.g[i][j] = 0.5 * (combined.g[i][j] + local.g[i][j]);
                }
            }
        }
        
        return combined;
    }
};

} // namespace quantumverse

#endif // QUANTUMVERSE_SINGULARITY_HANDLER_H