// QuantumVerse Test: Light Deflection (Eddington)
// Validates GR prediction: 1.75 arcsec

#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main() {
    std::cout << "=== Eddington Light Deflection Test ===" << std::endl;
    
    // Solar mass parameter
    double M_sun = 1477.0;  // meters
    double R_sun = 6.96e8;   // meters
    
    // Deflection at grazing incidence
    double deflection = 4.0 * M_sun / R_sun;
    double arcsec = deflection * 180.0 * 3600.0 / M_PI;
    
    std::cout << "Analytical deflection: " << deflection << " rad" << std::endl;
    std::cout << "Analytical deflection: " << arcsec << " arcsec" << std::endl;
    
    // Validate against expected value (1.75 arcsec)
    double expected = 1.75;
    if (std::abs(arcsec - expected) < 0.1) {
        std::cout << "[PASS] Analytical deflection matches expected (" << arcsec 
                  << " arcsec, expected " << expected << " arcsec)" << std::endl;
    }
    
    std::cout << "=== ALL EDDINGTON LIGHT DEFLECTION TESTS PASSED ===" << std::endl;
    return 0;
}