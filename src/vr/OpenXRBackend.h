/**
 * @file OpenXRBackend.h
 * @brief OpenXR runtime backend for VR headset integration
 *
 * This header defines the interface for OpenXR runtime interaction.
 * When the OpenXR SDK is available (QUANTUMVERSE_USE_OPENXR_SDK=ON),
 * the real implementation is compiled. Otherwise, a stub implementation
 * is used that provides no-op behavior for build compatibility.
 */

#ifndef QUANTUMVERSE_OPENXR_BACKEND_H
#define QUANTUMVERSE_OPENXR_BACKEND_H

#include "../vr/VRCommon.h"
#include <string>
#include <vector>
#include <functional>

namespace quantumverse {
namespace vr {

/**
 * @brief OpenXR backend interface for VR headset interaction
 *
 * Provides head pose tracking, controller input, and stereo rendering
 * support through the OpenXR runtime.
 */
class OpenXRBackend {
public:
    using PoseCallback = std::function<void(const HeadPose& leftEye, const HeadPose& rightEye)>;
    using ButtonCallback = std::function<void(int controllerIndex, int button, bool pressed)>;

    OpenXRBackend();
    ~OpenXRBackend();

    // Non-copyable
    OpenXRBackend(const OpenXRBackend&) = delete;
    OpenXRBackend& operator=(const OpenXRBackend&) = delete;
    OpenXRBackend(OpenXRBackend&&) = default;
    OpenXRBackend& operator=(OpenXRBackend&&) = default;

    /**
     * @brief Initialize the OpenXR runtime
     * @param applicationName Name of the application for OpenXR
     * @return true if initialization succeeded
     */
    bool initialize(const std::string& applicationName = "QuantumVerse");

    /**
     * @brief Shutdown the OpenXR runtime
     */
    void shutdown();

    /**
     * @brief Check if OpenXR is active and a headset is connected
     */
    bool isActive() const { return m_isActive; }

    /**
     * @brief Check if the backend is in stub mode (no real OpenXR)
     */
    bool isStub() const { return m_isStub; }

    /**
     * @brief Begin a VR frame
     * @return true if frame should be rendered
     */
    bool beginFrame();

    /**
     * @brief End a VR frame and submit to headset
     */
    void endFrame();

    /**
     * @brief Get the current head pose for both eyes
     * @param leftEye Output left eye pose
     * @param rightEye Output right eye pose
     * @return true if poses are valid
     */
    bool getHeadPose(HeadPose& leftEye, HeadPose& rightEye);

    /**
     * @brief Get the current state of both controllers
     * @param left Output left controller state
     * @param right Output right controller state
     * @return true if controller states are valid
     */
    bool getControllerState(ControllerState& left, ControllerState& right);

    /**
     * @brief Get the recommended viewport size for the current eye
     * @param eye Which eye to get viewport for
     * @return Viewport size in pixels
     */
    QSize getViewportSize(StereoEye eye) const;

    /**
     * @brief Get the projection matrix for an eye
     * @param eye Which eye to get projection for
     * @param nearClip Near clipping plane
     * @param farClip Far clipping plane
     * @return 4x4 projection matrix as float array (column-major)
     */
    std::array<float, 16> getProjectionMatrix(StereoEye eye, float nearClip, float farClip) const;

    /**
     * @brief Get the view matrix for an eye
     * @param eye Which eye to get view for
     * @return 4x4 view matrix as float array (column-major)
     */
    std::array<float, 16> getViewMatrix(StereoEye eye) const;

    /**
     * @brief Set the pose callback for head tracking
     * @param callback Function to call when head pose updates
     */
    void setPoseCallback(PoseCallback callback) { m_poseCallback = std::move(callback); }

    /**
     * @brief Set the button callback for controller input
     * @param callback Function to call when controller button state changes
     */
    void setButtonCallback(ButtonCallback callback) { m_buttonCallback = std::move(callback); }

    /**
     * @brief Get VR configuration
     */
    const VRConfig& getConfig() const { return m_config; }

    /**
     * @brief Set VR configuration
     */
    void setConfig(const VRConfig& config) { m_config = config; }

private:
    bool m_isActive = false;
    bool m_isStub = true;
    VRConfig m_config;

    // Cached pose data
    HeadPose m_leftEyePose;
    HeadPose m_rightEyePose;
    ControllerState m_leftController;
    ControllerState m_rightController;

    // Callbacks
    PoseCallback m_poseCallback;
    ButtonCallback m_buttonCallback;

    // Internal state
    double m_lastFrameTime = 0.0;
    uint64_t m_frameIndex = 0;
};

} // namespace vr
} // namespace quantumverse

#endif // QUANTUMVERSE_OPENXR_BACKEND_H
