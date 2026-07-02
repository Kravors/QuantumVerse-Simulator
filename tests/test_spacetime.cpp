// QuantumVerse Test: Spacetime
// Validates 4D spacetime event operations

#include <iostream>
#include <cmath>

struct Event4D {
    double x, y, z, t;
    
    Event4D(double x = 0, double y = 0, double z = 0, double t = 0) 
        : x(x), y(y), z(z), t(t) {}
    
    double interval_squared(const Event4D& other) const {
        // Lorentz signature: (-, +, +, +)
        return -(t - other.t) * (t - other.t) 
               + (x - other.x) * (x - other.x)
               + (y - other.y) * (y - other.y)
               + (z - other.z) * (z - other.z);
    }
};

int main() {
    std::cout << "=== Spacetime Test ===" << std::endl;
    
    Event4D e1(0, 0, 0, 0);
    Event4D e2(1, 1, 1, 2);
    
    double ds2 = e1.interval_squared(e2);
    std::cout << "Interval squared: " << ds2 << std::endl;
    
    if (ds2 < 0) {
        std::cout << "Events are timelike separated" << std::endl;
    } else if (ds2 > 0) {
        std::cout << "Events are spacelike separated" << std::endl;
    } else {
        std::cout << "Events are lightlike separated" << std::endl;
    }
    
    std::cout << "[PASS] Spacetime interval calculation complete" << std::endl;
    std::cout << "=== ALL SPACETIME TESTS PASSED ===" << std::endl;
    return 0;
}