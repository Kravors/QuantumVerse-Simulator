/**
 * @file OpenXRBackend.cpp
 * @brief Stub implementation of OpenXR backend
 *
 * This implementation provides a no-op stub that compiles without
 * the OpenXR SDK. When QUANTUMVERSE_USE_OPENXR_SDK=ON and the SDK
 * is available, this can be replaced with a real implementation.
 */

#include "OpenXRBackend.h"
#include <QSize>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {
namespace vr {

OpenXRBackend::OpenXRBackend() = default;

OpenXRBackend::~OpenXRBackend()
{
    shutdown();
}

bool OpenXRBackend::initialize(const std::string& applicationName)
{
    (void)applicationName;
    m_isStub = true;
    m_isActive = false;

    // Initialize with default pose at origin
    m_leftEyePose.position = Vec3(-m_config.ipd * 0.5f, 0.0f, 0.0f);
    m_leftEyePose.orientation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_leftEyePose.isValid = false;

    m_rightEyePose.position = Vec3(m_config.ipd * 0.5f, 0.0f, 0.0f);
    m_rightEyePose.orientation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_rightEyePose.isValid = false;

    return true;
}

void OpenXRBackend::shutdown()
{
    m_isActive = false;
    m_isStub = true;
}

bool OpenXRBackend::beginFrame()
{
    if (!m_isActive) return false;

    // Simulate frame timing
    m_lastFrameTime = 0.0;
    m_frameIndex++;

    return true;
}

void OpenXRBackend::endFrame()
{
    // Stub: no-op
}

bool OpenXRBackend::getHeadPose(HeadPose& leftEye, HeadPose& rightEye)
{
    if (!m_isActive) return false;

    leftEye = m_leftEyePose;
    rightEye = m_rightEyePose;

    // Update timestamp
    leftEye.timestamp = m_lastFrameTime;
    rightEye.timestamp = m_lastFrameTime;

    return m_isActive;
}

bool OpenXRBackend::getControllerState(ControllerState& left, ControllerState& right)
{
    if (!m_isActive) return false;

    left = m_leftController;
    right = m_rightController;

    return m_isActive;
}

QSize OpenXRBackend::getViewportSize(StereoEye eye) const
{
    // Return default viewport size for stub mode
    switch (eye) {
        case StereoEye::Left:
        case StereoEye::Right:
            return QSize(1920, 1080);
        case StereoEye::Mono:
        default:
            return QSize(3840, 1080);
    }
}

std::array<float, 16> OpenXRBackend::getProjectionMatrix(StereoEye eye, float nearClip, float farClip) const
{
    std::array<float, 16> matrix = {};

    // Simple perspective projection matrix (column-major)
    float fov = 90.0f * M_PI / 180.0f;  // 90 degree FOV
    float aspect = 1920.0f / 1080.0f;

    float f = 1.0f / std::tan(fov * 0.5f);
    float nf = 1.0f / (nearClip - farClip);

    matrix[0] = f / aspect;
    matrix[5] = f;
    matrix[10] = farClip * nf;
    matrix[11] = -1.0f;
    matrix[14] = nearClip * farClip * nf;

    // Adjust for stereo eye offset
    if (eye == StereoEye::Left) {
        matrix[8] = -0.05f;  // Left eye offset
    } else if (eye == StereoEye::Right) {
        matrix[8] = 0.05f;   // Right eye offset
    }

    return matrix;
}

std::array<float, 16> OpenXRBackend::getViewMatrix(StereoEye eye) const
{
    std::array<float, 16> matrix = {};

    // Identity view matrix for stub mode
    matrix[0] = 1.0f;
    matrix[5] = 1.0f;
    matrix[10] = 1.0f;
    matrix[15] = 1.0f;

    // Offset for stereo eye
    if (eye == StereoEye::Left) {
        matrix[12] = -m_config.ipd * 0.5f;
    } else if (eye == StereoEye::Right) {
        matrix[12] = m_config.ipd * 0.5f;
    }

    return matrix;
}

} // namespace vr
} // namespace quantumverse
