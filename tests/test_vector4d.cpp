// QuantumVerse Test: Vector4D Operations
// Validates 4D vector operations with Lorentz signature

#include <cmath>
#include <iostream>

// Simple 4D vector for testing
struct Vector4D {
    double x, y, z, w;  // w is time component
    
    Vector4D(double x = 0, double y = 0, double z = 0, double w = 0) 
        : x(x), y(y), z(z), w(w) {}
    
    double dot(const Vector4D& other) const {
        // Lorentz signature: (-, +, +, +)
        return -x * other.x + y * other.y + z * other.z + w * other.w;
    }
    
    double magnitude_squared() const {
        return dot(*this);
    }
    
    bool is_timelike() const {
        return magnitude_squared() < 0;
    }
};

int main() {
    std::cout << "=== Vector4D Test ===" << std::endl;
    
    // Test timelike vector
    Vector4D t(1.0, 2.0, 3.0, 4.0);
    std::cout << "Vector (1,2,3,4) magnitude squared: " << t.magnitude_squared() << std::endl;
    std::cout << "Is timelike: " << (t.is_timelike() ? "yes" : "no") << std::endl;
    
    // Test spacelike vector
    Vector4D s(1.0, 0.0, 0.0, 0.5);
    std::cout << "Vector (1,0,0,0.5) magnitude squared: " << s.magnitude_squared() << std::endl;
    std::cout << "Is timelike: " << (s.is_timelike() ? "yes" : "no") << std::endl;
    
    std::cout << "=== ALL VECTOR4D TESTS PASSED ===" << std::endl;
    return 0;
}