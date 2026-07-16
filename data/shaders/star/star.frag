#version 450 core
#extension GL_GOOGLE_include_directive : enable

#include "../common/ubo.glsl"

in vec3 v_color;
in vec3 v_worldPos;
in float v_dopplerShift;  // Doppler shift factor from vertex shader

out vec4 FragColor;

void main() {
    // Apply relativistic Doppler color shift
    // Blue shift when approaching (v_dopplerShift > 1), red shift when receding
    vec3 dopplerColor = v_color;
    
    // Non-relativistic approximation:
    // - Blue shift: boost blue channel
    // - Red shift: boost red channel
    if (v_dopplerShift > 1.0) {
        // Approaching - blueshift (more blue)
        float boost = v_dopplerShift - 1.0;
        dopplerColor.b = min(dopplerColor.b * (1.0 + boost * 0.5), 1.0);
        dopplerColor.r = max(dopplerColor.r * (1.0 - boost * 0.2), 0.0);
        dopplerColor.g = max(dopplerColor.g * (1.0 - boost * 0.1), 0.0);
    } else if (v_dopplerShift < 1.0) {
        // Receding - redshift (more red)
        float boost = 1.0 - v_dopplerShift;
        dopplerColor.r = min(dopplerColor.r * (1.0 + boost * 0.5), 1.0);
        dopplerColor.b = max(dopplerColor.b * (1.0 - boost * 0.2), 0.0);
        dopplerColor.g = max(dopplerColor.g * (1.0 - boost * 0.1), 0.0);
    }
    
    // Simple star rendering - emissive point source with Doppler effect
    FragColor = vec4(dopplerColor, 1.0);
}
