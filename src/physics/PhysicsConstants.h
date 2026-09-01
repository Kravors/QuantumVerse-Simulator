/**
 * @file PhysicsConstants.h
 * @brief Configurable physical constants for the Variable Constants Sandbox
 *
 * Allows runtime modification of G, c, h for "multiverse" simulations.
 * Replaces the static constexpr values in Event4D with runtime-configurable ones.
 */

#ifndef QUANTUMVERSE_PHYSICS_CONSTANTS_H
#define QUANTUMVERSE_PHYSICS_CONSTANTS_H

#include <cmath>
#include <algorithm>

namespace quantumverse {

class PhysicsConstants {
public:
    static PhysicsConstants& instance() {
        static PhysicsConstants inst;
        return inst;
    }

    // Reset to CODATA 2018 values
    void reset() {
        G = 6.67430e-11;      // m^3 kg^-1 s^-2
        c = 299792458.0;       // m/s
        c2 = c * c;
        h = 6.62607015e-34;    // J s
    }

    // Setters with physical bounds
    void set_G(double val) { G = std::max(val, 0.0); }
    void set_c(double val) { c = std::max(val, 1.0); c2 = c * c; }
    void set_h(double val) { h = std::max(val, 0.0); }

    // Getters
    double get_G() const { return G; }
    double get_c() const { return c; }
    double get_c2() const { return c2; }
    double get_h() const { return h; }

    // Log-scaled setters for UI (allows easy exploration of orders of magnitude)
    void set_G_log(double log10_val) { G = std::pow(10.0, log10_val); }
    void set_c_log(double log10_val) { c = std::pow(10.0, log10_val); c2 = c * c; }

    double get_G_log() const { return std::log10(G); }
    double get_c_log() const { return std::log10(c); }

    double G = 6.67430e-11;
    double c = 299792458.0;
    double c2 = c * c;
    double h = 6.62607015e-34;

private:
    PhysicsConstants() = default;
};

// Inline accessor functions for clean syntax in physics code
inline double PHYS_G() { return PhysicsConstants::instance().G; }
inline double PHYS_C() { return PhysicsConstants::instance().c; }
inline double PHYS_C2() { return PhysicsConstants::instance().c2; }
inline double PHYS_H() { return PhysicsConstants::instance().h; }

} // namespace quantumverse

#endif // QUANTUMVERSE_PHYSICS_CONSTANTS_H
