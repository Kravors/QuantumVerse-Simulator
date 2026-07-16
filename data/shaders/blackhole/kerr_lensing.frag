#version 450 core
#extension GL_GOOGLE_include_directive : enable

#include "../common/ubo.glsl"

out vec4 outColor;

in vec2 TexCoords;

// Kerr black hole parameters
uniform float u_mass;           // Black hole mass
uniform float u_spin;           // Spin parameter a/M (0.0 to 0.99)
uniform vec2 u_bhScreenPos;     // Black hole screen position (NDC)
uniform sampler2D u_background; // Background scene texture

// Physical constants
const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;

// Calculate Kerr metric deflection
vec2 kerrDeflection(vec2 delta, float mass, float spin) {
    // Distance from black hole center
    float r = length(delta);
    float r_s = 2.0 * mass;  // Schwarzschild radius
    
    // Avoid singularity at center
    if (r < 0.001) {
        return delta;
    }
    
    // Basic Schwarzschild deflection (bending toward BH)
    float deflection = r_s / (r + 0.001);
    
    // Kerr frame-dragging effect
    // The spin creates an asymmetric warp that twists the light
    vec2 perp = vec2(-delta.y, delta.x);  // Perpendicular vector
    float twistIntensity = spin * (r_s * r_s) / (r * r + 0.001);
    
    // Combine radial deflection with frame-dragging twist
    vec2 warpedUV = delta + (delta * deflection) + (perp * twistIntensity);
    
    return warpedUV;
}

// Calculate oblateness factor for Kerr horizon
float oblateness(float spin) {
    return sqrt(1.0 - spin * spin);
}

void main() {
    // Vector from black hole center to current pixel
    vec2 delta = gl_FragCoord.xy - u_bhScreenPos;
    
    // Calculate Kerr deflection
    vec2 warpedUV = kerrDeflection(delta, u_mass, u_spin);
    
    // Convert to UV coordinates for background sampling
    vec2 finalUV = (u_bhScreenPos + warpedUV) / vec2(textureSize(u_background, 0));
    
    // Clamp to valid range
    finalUV = clamp(finalUV, 0.0, 1.0);
    
    // Sample the background
    vec3 background = texture(u_background, finalUV).rgb;
    
    // Calculate distance from center for horizon rendering
    float r = length(delta);
    float r_s = 2.0 * u_mass;
    
    // Event horizon (oblate spheroid)
    float oblate = oblateness(u_spin);
    float horizonRadius = r_s;
    
    // Smooth horizon edge
    float horizon = smoothstep(horizonRadius * 1.1, horizonRadius * 0.9, r);
    background = mix(background, vec3(0.0), horizon);
    
    // Photon ring (more pronounced in Kerr)
    float photonRing = smoothstep(r_s * 1.0, r_s * 1.1, r) - smoothstep(r_s * 1.1, r_s * 0.9, r);
    background += vec3(0.4, 0.4, 0.5) * photonRing * 0.5;
    
    // Accretion disk glow (asymmetric for Kerr)
    float diskAngle = atan(delta.y, delta.x);
    float diskIntensity = exp(-r * 0.5) * cos(diskAngle - u_spin * PI);
    background += vec3(0.8, 0.3, 0.1) * max(diskIntensity, 0.0) * 0.3;
    
    outColor = vec4(background, 1.0);
}
