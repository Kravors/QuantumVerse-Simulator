#version 450 core
#extension GL_GOOGLE_include_directive : enable

#include "../common/ubo.glsl"

out vec4 outColor;

in vec2 TexCoords;

uniform sampler2D u_currentFrame;
uniform sampler2D u_previousFrame;
uniform mat4 u_currentProjection;
uniform mat4 u_previousProjection;
uniform mat4 u_currentView;
uniform mat4 u_previousView;
uniform float u_jitterX;
uniform float u_jitterY;

// Reconstruct view space position from depth
vec3 reconstructViewPos(vec2 uv, float depth) {
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    clipPos = inverse(u_currentProjection) * clipPos;
    clipPos /= clipPos.w;
    
    vec4 viewPos = inverse(u_currentView) * clipPos;
    return viewPos.xyz;
}

// Jitter pattern (Halton sequence)
vec2 halton2D(int index) {
    float u = 0.0;
    float f = 1.0;
    int i = index;
    while (i > 0) {
        f /= 2.0;
        u += f * mod(float(i), 2.0);
        i = int(i / 2);
    }
    
    float v = 0.0;
    f = 1.0;
    i = index;
    while (i > 0) {
        f /= 3.0;
        v += f * mod(float(i), 3.0);
        i = int(i / 3);
    }
    
    return vec2(u, v);
}

void main() {
    // Sample current frame
    vec3 currentColor = texture(u_currentFrame, TexCoords).rgb;
    
    // Get jitter offset
    vec2 jitter = vec2(u_jitterX, u_jitterY);
    
    // Calculate UV with jitter
    vec2 jitteredUV = TexCoords + jitter;
    
    // Clamp to valid range
    jitteredUV = mod(jitteredUV, 1.0);
    
    // Sample previous frame at jittered position
    vec3 previousColor = texture(u_previousFrame, jitteredUV).rgb;
    
    // Calculate blend factor (start higher, decrease over time)
    float blendFactor = 0.05;
    if (u_sampleCount > 16) {
        blendFactor = 0.95;  // More stable after many samples
    }
    
    // Reject samples with large color differences (disocclusion)
    float colorDiff = length(currentColor - previousColor);
    if (colorDiff > 0.2) {
        blendFactor = 0.0;  // Use current sample
    }
    
    // Blend current and previous frames
    vec3 result = mix(currentColor, previousColor, blendFactor);
    
    // Apply sharpening (CAS)
    float sharpening = u_sharpening;
    if (sharpening > 0.0) {
        vec3 neighbors[4];
        neighbors[0] = texture(u_currentFrame, TexCoords + vec2(0.5/1920, 0)).rgb;
        neighbors[1] = texture(u_currentFrame, TexCoords + vec2(-0.5/1920, 0)).rgb;
        neighbors[2] = texture(u_currentFrame, TexCoords + vec2(0, 0.5/1080)).rgb;
        neighbors[3] = texture(u_currentFrame, TexCoords + vec2(0, -0.5/1080)).rgb;
        
        vec3 localAvg = (currentColor + neighbors[0] + neighbors[1] + neighbors[2] + neighbors[3]) * 0.2;
        vec3 sharp = 2.0 * localAvg - currentColor;
        result = mix(result, sharp, sharpening * 0.5);
    }
    
    outColor = vec4(result, 1.0);
}

