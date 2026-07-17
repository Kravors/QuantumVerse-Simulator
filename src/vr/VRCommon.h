/**
 * @file VRCommon.h
 * @brief Common VR types and utilities for QuantumVerse
 */

#ifndef QUANTUMVERSE_VR_COMMON_H
#define QUANTUMVERSE_VR_COMMON_H

#include <cmath>
#include <cstdint>

namespace quantumverse {
namespace vr {

/**
 * @brief 3D vector for positions and directions
 */
struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vec3() = default;
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    float length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    Vec3 normalized() const {
        float len = length();
        if (len > 1e-6f) {
            return Vec3(x / len, y / len, z / len);
        }
        return Vec3(0.0f, 0.0f, 0.0f);
    }

    Vec3 operator-(const Vec3& other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }

    Vec3 operator+(const Vec3& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }

    Vec3 operator*(float scalar) const {
        return Vec3(x * scalar, y * scalar, z * scalar);
    }
};

/**
 * @brief Quaternion for rotations
 */
struct Quat {
    float w = 1.0f;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Quat() = default;
    Quat(float w, float x, float y, float z) : w(w), x(x), y(y), z(z) {}

    static Quat fromEuler(float pitch, float yaw, float roll) {
        float cy = std::cos(yaw * 0.5f);
        float sy = std::sin(yaw * 0.5f);
        float cp = std::cos(pitch * 0.5f);
        float sp = std::sin(pitch * 0.5f);
        float cr = std::cos(roll * 0.5f);
        float sr = std::sin(roll * 0.5f);

        return Quat(
            cr * cp * cy + sr * sp * sy,
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy
        );
    }
};

/**
 * @brief Head pose data from VR headset
 */
struct HeadPose {
    Vec3 position;
    Quat orientation;
    float linearVelocityX = 0.0f;
    float linearVelocityY = 0.0f;
    float linearVelocityZ = 0.0f;
    float angularVelocityX = 0.0f;
    float angularVelocityY = 0.0f;
    float angularVelocityZ = 0.0f;
    double timestamp = 0.0;
    bool isValid = false;
};

/**
 * @brief Controller state from VR input
 */
struct ControllerState {
    enum Button {
        ButtonTrigger = 0,
        ButtonGrip = 1,
        ButtonThumbstick = 2,
        ButtonA = 3,
        ButtonB = 4,
        ButtonTouchpad = 5
    };

    Vec3 position;
    Quat orientation;
    float triggerValue = 0.0f;
    float gripValue = 0.0f;
    bool buttons[6] = {false, false, false, false, false, false};
    Vec3 thumbstickAxis;
    double timestamp = 0.0;
    bool isValid = false;
};

/**
 * @brief Stereo eye configuration
 */
enum class StereoEye {
    Left = 0,
    Right = 1,
    Mono = 2
};

/**
 * @brief VR configuration parameters
 */
struct VRConfig {
    bool enabled = false;
    bool debugMode = false;
    float ipd = 0.063f;  // Inter-pupillary distance in meters
    float nearClip = 0.1f;
    float farClip = 1000.0f;
    float worldScale = 1.0f;
    bool showControllers = true;
    bool showGuardian = true;
    bool enablePassthrough = false;
};

} // namespace vr
} // namespace quantumverse

#endif // QUANTUMVERSE_VR_COMMON_H
