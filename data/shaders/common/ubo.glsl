/**
 * @file ubo.glsl
 * @brief Uniform Buffer Object definitions for QuantumVerse Simulator
 * 
 * Layout: std140 for cross-platform compatibility
 * Binding points:
 *   0 - SceneUBO (view, projection, camera, time)
 *   1 - LightUBO (sun direction, color, intensity)
 *   2 - MaterialUBO (PBR material properties)
 *   3 - IBLUBO (Image-Based Lighting parameters)
 *   4 - PostProcessUBO (TAA, auto-exposure, etc.)
 */

#ifndef QUANTUMVERSE_UBO_GLSL
#define QUANTUMVERSE_UBO_GLSL

// Scene UBO - Binding point 0
layout(std140, binding = 0) uniform SceneUBO {
    mat4 u_viewMatrix;           // 64 bytes (16 * 4)
    mat4 u_projectionMatrix;     // 64 bytes
    vec4 u_cameraPosition;       // 16 bytes (w = 4D time coordinate)
    vec3 u_cameraVelocity;       // 12 bytes - camera velocity for Doppler effect
    float u_time;                // 4 bytes
    float u_sliceOffset;         // 4 bytes (for 4D slicing)
    float u_padding0;            // 4 bytes (padding for alignment)
    float u_padding1;            // 4 bytes (padding for alignment)
};

// Light UBO - Binding point 1
layout(std140, binding = 1) uniform LightUBO {
    vec3 u_sunDirection;         // 12 bytes
    float u_sunIntensity;        // 4 bytes
    vec3 u_sunColor;             // 12 bytes
    float u_ambientIntensity;    // 4 bytes
    vec3 u_paddingLight;         // 12 bytes (padding)
};

// Black Hole UBO - Binding point 5
layout(std140, binding = 5) uniform BlackHoleUBO {
    float u_blackHoleMass;       // 4 bytes - mass in geometric units
    float u_blackHoleSpin;       // 4 bytes - spin parameter a/M (0.0 to 0.99)
    vec2 u_blackHoleScreenPos;   // 8 bytes - screen position in NDC
    float u_paddingBH[5];        // 20 bytes - padding
};

// Material UBO - Binding point 2 (PBR properties)
layout(std140, binding = 2) uniform MaterialUBO {
    vec3 u_albedo;               // 12 bytes - base color
    float u_metallic;            // 4 bytes - metallic value (0-1)
    float u_roughness;           // 4 bytes - roughness value (0-1)
    float u_emissive;            // 4 bytes - emissive strength
    float u_normalScale;         // 4 bytes - normal map intensity
    vec3 u_paddingMaterial;      // 12 bytes (padding)
};

// IBL UBO - Binding point 3 (Image-Based Lighting)
layout(std140, binding = 3) uniform IBLUBO {
    vec3 u_iblIntensity;         // 12 bytes - IBL light intensity
    float u_iblEnabled;           // 4 bytes - IBL enable flag
    vec3 u_iblRotation;          // 12 bytes - skybox rotation (Euler angles)
    float u_paddingIBL1;         // 4 bytes
    vec3 u_skyboxColor;          // 12 bytes - fallback sky color
    float u_paddingIBL2;         // 4 bytes
};

// Post-Processing UBO - Binding point 4
layout(std140, binding = 4) uniform PostProcessUBO {
    float u_taaEnabled;          // 4 bytes - TAA enable flag
    float u_taaJitter;           // 4 bytes - TAA jitter amount
    float u_exposure;            // 4 bytes - auto-exposure value
    float u_sharpening;          // 4 bytes - CAS/FSR sharpening strength
    float u_gamma;               // 4 bytes - gamma correction
    int u_sampleCount;           // 4 bytes - current sample count (for TAA)
    float u_paddingPP[3];        // 12 bytes - padding
};

#endif // QUANTUMVERSE_UBO_GLSL