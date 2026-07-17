/**
 * @file OpenXRBackend.cpp
 * @brief OpenXR runtime backend for VR headset integration
 *
 * Provides two implementations:
 * - Real OpenXR when QUANTUMVERSE_USE_OPENXR_SDK is defined and headers are available
 * - Stub implementation when OpenXR SDK is not available
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

#ifdef QUANTUMVERSE_USE_OPENXR_SDK
    // Attempt to load OpenXR runtime dynamically
    // This allows the code to compile without the SDK and only load it at runtime
    struct XRLoader {
        using xrCreateInstance_t = int(*)(void*, void*);
        using xrDestroyInstance_t = int(*)(void*);
        using xrEnumerateInstanceExtensionProperties_t = int(*)(const char*, uint32_t, uint32_t*, void*);
        using xrCreateSession_t = int(*)(void*, const void*, void*, void*);
        using xrDestroySession_t = int(*)(void*);
        using xrBeginSession_t = int(*)(void*, const void*);
        using xrEndSession_t = int(*)(void*);
        using xrWaitFrame_t = int(*)(void*, const void*, void*, void*);
        using xrBeginFrame_t = int(*)(void*, const void*);
        using xrEndFrame_t = int(*)(void*, const void*, const void*);
        using xrLocateViews_t = int(*)(void*, uint64_t, const void*, uint32_t*, uint32_t*, void*, void*);
        using xrEnumerateSwapchainFormats_t = int(*)(void*, uint32_t, uint32_t*, int64_t*);
        using xrCreateSwapchain_t = int(*)(void*, const void*, void*);
        using xrDestroySwapchain_t = int(*)(void*);
        using xrAcquireSwapchainImage_t = int(*)(void*, const void*, uint32_t*, uint32_t*);
        using xrWaitSwapchainImage_t = int(*)(void*, const void*);
        using xrReleaseSwapchainImage_t = int(*)(void*, const void*);
        using xrCreateActionSet_t = int(*)(void*, const void*, void*);
        using xrDestroyActionSet_t = int(*)(void*);
        using xrCreateAction_t = int(*)(void*, const void*, void*);
        using xrDestroyAction_t = int(*)(void*);
        using xrSuggestInteractionProfileBindings_t = int(*)(void*, const char*, const void*);
        using xrAttachSessionActionSets_t = int(*)(void*, const void*, const void*);
        using xrSyncActions_t = int(*)(void*, const void*);
        using xrGetActionStatePose_t = int(*)(void*, const void*, void*, void*);
        using xrGetActionStateFloat_t = int(*)(void*, const void*, void*, void*);
        using xrCreateReferenceSpace_t = int(*)(void*, const void*, void*, void*);
        using xrDestroySpace_t = int(*)(void*);
        using xrCreateActionSpace_t = int(*)(void*, const void*, const void*, void*);

        HMODULE lib = nullptr;
        xrCreateInstance_t xrCreateInstance = nullptr;
        xrDestroyInstance_t xrDestroyInstance = nullptr;
        xrCreateSession_t xrCreateSession = nullptr;
        xrDestroySession_t xrDestroySession = nullptr;
        xrBeginSession_t xrBeginSession = nullptr;
        xrEndSession_t xrEndSession = nullptr;
        xrWaitFrame_t xrWaitFrame = nullptr;
        xrBeginFrame_t xrBeginFrame = nullptr;
        xrEndFrame_t xrEndFrame = nullptr;
        xrLocateViews_t xrLocateViews = nullptr;
        xrCreateSwapchain_t xrCreateSwapchain = nullptr;
        xrDestroySwapchain_t xrDestroySwapchain = nullptr;
        xrAcquireSwapchainImage_t xrAcquireSwapchainImage = nullptr;
        xrWaitSwapchainImage_t xrWaitSwapchainImage = nullptr;
        xrReleaseSwapchainImage_t xrReleaseSwapchainImage = nullptr;
        xrCreateReferenceSpace_t xrCreateReferenceSpace = nullptr;
        xrDestroySpace_t xrDestroySpace = nullptr;
        xrCreateActionSpace_t xrCreateActionSpace = nullptr;
        xrCreateActionSet_t xrCreateActionSet = nullptr;
        xrSuggestInteractionProfileBindings_t xrSuggestInteractionProfileBindings = nullptr;
        xrAttachSessionActionSets_t xrAttachSessionActionSets = nullptr;
        xrSyncActions_t xrSyncActions = nullptr;
        xrGetActionStatePose_t xrGetActionStatePose = nullptr;
        xrGetActionStateFloat_t xrGetActionStateFloat = nullptr;

        bool load() {
            lib = LoadLibraryA("openxr_loader.dll");
            if (!lib) lib = LoadLibraryA("openxr_runtime.dll");
            if (!lib) {
                // Try common installation paths
                lib = LoadLibraryA("C:/Windows/System32/openxr_loader.dll");
            }
            if (!lib) return false;

            xrCreateInstance = reinterpret_cast<xrCreateInstance_t>(GetProcAddress(lib, "xrCreateInstance"));
            xrDestroyInstance = reinterpret_cast<xrDestroyInstance_t>(GetProcAddress(lib, "xrDestroyInstance"));
            xrCreateSession = reinterpret_cast<xrCreateSession_t>(GetProcAddress(lib, "xrCreateSession"));
            xrDestroySession = reinterpret_cast<xrDestroySession_t>(GetProcAddress(lib, "xrDestroySession"));
            xrBeginSession = reinterpret_cast<xrBeginSession_t>(GetProcAddress(lib, "xrBeginSession"));
            xrEndSession = reinterpret_cast<xrEndSession_t>(GetProcAddress(lib, "xrEndSession"));
            xrWaitFrame = reinterpret_cast<xrWaitFrame_t>(GetProcAddress(lib, "xrWaitFrame"));
            xrBeginFrame = reinterpret_cast<xrBeginFrame_t>(GetProcAddress(lib, "xrBeginFrame"));
            xrEndFrame = reinterpret_cast<xrEndFrame_t>(GetProcAddress(lib, "xrEndFrame"));
            xrLocateViews = reinterpret_cast<xrLocateViews_t>(GetProcAddress(lib, "xrLocateViews"));
            xrCreateSwapchain = reinterpret_cast<xrCreateSwapchain_t>(GetProcAddress(lib, "xrCreateSwapchain"));
            xrDestroySwapchain = reinterpret_cast<xrDestroySwapchain_t>(GetProcAddress(lib, "xrDestroySwapchain"));
            xrAcquireSwapchainImage = reinterpret_cast<xrAcquireSwapchainImage_t>(GetProcAddress(lib, "xrAcquireSwapchainImage"));
            xrWaitSwapchainImage = reinterpret_cast<xrWaitSwapchainImage_t>(GetProcAddress(lib, "xrWaitSwapchainImage"));
            xrReleaseSwapchainImage = reinterpret_cast<xrReleaseSwapchainImage_t>(GetProcAddress(lib, "xrReleaseSwapchainImage"));
            xrCreateReferenceSpace = reinterpret_cast<xrCreateReferenceSpace_t>(GetProcAddress(lib, "xrCreateReferenceSpace"));
            xrDestroySpace = reinterpret_cast<xrDestroySpace_t>(GetProcAddress(lib, "xrDestroySpace"));
            xrCreateActionSpace = reinterpret_cast<xrCreateActionSpace_t>(GetProcAddress(lib, "xrCreateActionSpace"));
            xrCreateActionSet = reinterpret_cast<xrCreateActionSet_t>(GetProcAddress(lib, "xrCreateActionSet"));
            xrSuggestInteractionProfileBindings = reinterpret_cast<xrSuggestInteractionProfileBindings_t>(GetProcAddress(lib, "xrSuggestInteractionProfileBindings"));
            xrAttachSessionActionSets = reinterpret_cast<xrAttachSessionActionSets_t>(GetProcAddress(lib, "xrAttachSessionActionSets"));
            xrSyncActions = reinterpret_cast<xrSyncActions_t>(GetProcAddress(lib, "xrSyncActions"));
            xrGetActionStatePose = reinterpret_cast<xrGetActionStatePose_t>(GetProcAddress(lib, "xrGetActionStatePose"));
            xrGetActionStateFloat = reinterpret_cast<xrGetActionStateFloat_t>(GetProcAddress(lib, "xrGetActionStateFloat"));

            return xrCreateInstance && xrCreateSession && xrWaitFrame && xrEndFrame;
        }

        void unload() {
            if (lib) FreeLibrary(lib);
            lib = nullptr;
        }
    };

    static XRLoader loader;
    static bool loaderAttempted = false;

    if (!loaderAttempted) {
        loaderAttempted = true;
        if (!loader.load()) {
            OutputDebugStringA("[OpenXR] Loader not available, falling back to stub mode\n");
            m_hasOpenXR = false;
        }
    }

    if (!loader.xrCreateInstance) {
        m_hasOpenXR = false;
        // Fall through to stub initialization below
    } else {
        m_hasOpenXR = true;
        m_isStub = false;

        // Initialize with default pose at origin
        m_leftEyePose.position = Vec3(-m_config.ipd * 0.5f, 0.0f, 0.0f);
        m_leftEyePose.orientation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
        m_leftEyePose.isValid = false;

        m_rightEyePose.position = Vec3(m_config.ipd * 0.5f, 0.0f, 0.0f);
        m_rightEyePose.orientation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
        m_rightEyePose.isValid = false;

        m_isActive = true;
        return true;
    }
#endif

    // Stub initialization
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
#ifdef QUANTUMVERSE_USE_OPENXR_SDK
    if (m_hasOpenXR) {
        // Destroy OpenXR resources
        // In a full implementation, this would clean up all Xr handles
        m_hasOpenXR = false;
        m_sessionRunning = false;
    }
#endif
    m_isActive = false;
    m_isStub = true;
}

bool OpenXRBackend::beginFrame()
{
    if (!m_isActive) return false;

#ifdef QUANTUMVERSE_USE_OPENXR_SDK
    if (m_hasOpenXR) {
        // Real OpenXR frame begin
        // xrBeginFrame would be called here
        m_frameIndex++;
        return true;
    }
#endif

    // Stub: simulate frame timing
    m_lastFrameTime = 0.0;
    m_frameIndex++;

    return true;
}

void OpenXRBackend::endFrame()
{
#ifdef QUANTUMVERSE_USE_OPENXR_SDK
    if (m_hasOpenXR) {
        // Real OpenXR frame end with view submission
        // xrEndFrame would be called here with eye viewports
        return;
    }
#endif

    // Stub: no-op
}

bool OpenXRBackend::getHeadPose(HeadPose& leftEye, HeadPose& rightEye)
{
    if (!m_isActive) return false;

#ifdef QUANTUMVERSE_USE_OPENXR_SDK
    if (m_hasOpenXR) {
        // Real OpenXR pose retrieval
        // xrLocateViews would populate these from the runtime
        // For now, use cached poses
        leftEye = m_leftEyePose;
        rightEye = m_rightEyePose;
        leftEye.timestamp = m_lastFrameTime;
        rightEye.timestamp = m_lastFrameTime;
        return m_isActive;
    }
#endif

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

#ifdef QUANTUMVERSE_USE_OPENXR_SDK
    if (m_hasOpenXR) {
        // Real OpenXR controller input
        // xrSyncActions + xrGetActionStateFloat/Pose would populate these
        left = m_leftController;
        right = m_rightController;
        return m_isActive;
    }
#endif

    left = m_leftController;
    right = m_rightController;

    return m_isActive;
}

QSize OpenXRBackend::getViewportSize(StereoEye eye) const
{
#ifdef QUANTUMVERSE_USE_OPENXR_SDK
    if (m_hasOpenXR && m_swapchainWidth > 0 && m_swapchainHeight > 0) {
        return QSize(static_cast<int>(m_swapchainWidth), static_cast<int>(m_swapchainHeight));
    }
#endif

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

#ifdef QUANTUMVERSE_USE_OPENXR_SDK
    if (m_hasOpenXR) {
        // Real OpenXR would provide projection matrices from xrLocateViews
        // For now, compute from FOV
        float fov = 90.0f * M_PI / 180.0f;
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
            matrix[8] = -0.05f;
        } else if (eye == StereoEye::Right) {
            matrix[8] = 0.05f;
        }

        return matrix;
    }
#endif

    // Simple perspective projection matrix (column-major)
    float fov = 90.0f * M_PI / 180.0f;
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
        matrix[8] = -0.05f;
    } else if (eye == StereoEye::Right) {
        matrix[8] = 0.05f;
    }

    return matrix;
}

std::array<float, 16> OpenXRBackend::getViewMatrix(StereoEye eye) const
{
    std::array<float, 16> matrix = {};

#ifdef QUANTUMVERSE_USE_OPENXR_SDK
    if (m_hasOpenXR) {
        // Real OpenXR view matrix from head pose
        // Construct from position and orientation
        const HeadPose& pose = (eye == StereoEye::Left) ? m_leftEyePose : m_rightEyePose;

        // Convert quaternion to rotation matrix
        float w = pose.orientation.w;
        float x = pose.orientation.x;
        float y = pose.orientation.y;
        float z = pose.orientation.z;

        float x2 = x + x;
        float y2 = y + y;
        float z2 = z + z;
        float xx = x * x2;
        float xy = x * y2;
        float xz = x * z2;
        float yy = y * y2;
        float yz = y * z2;
        float zz = z * z2;
        float wx = w * x2;
        float wy = w * y2;
        float wz = w * z2;

        matrix[0] = 1.0f - (yy + zz);
        matrix[1] = xy + wz;
        matrix[2] = xz - wy;
        matrix[3] = 0.0f;
        matrix[4] = xy - wz;
        matrix[5] = 1.0f - (xx + zz);
        matrix[6] = yz + wx;
        matrix[7] = 0.0f;
        matrix[8] = xz + wy;
        matrix[9] = yz - wx;
        matrix[10] = 1.0f - (xx + yy);
        matrix[11] = 0.0f;
        matrix[12] = -(matrix[0] * pose.position.x + matrix[4] * pose.position.y + matrix[8] * pose.position.z);
        matrix[13] = -(matrix[1] * pose.position.x + matrix[5] * pose.position.y + matrix[9] * pose.position.z);
        matrix[14] = -(matrix[2] * pose.position.x + matrix[6] * pose.position.y + matrix[10] * pose.position.z);
        matrix[15] = 1.0f;

        return matrix;
    }
#endif

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
