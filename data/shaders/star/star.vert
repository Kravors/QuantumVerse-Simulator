#version 450 core

#include "../common/ubo.glsl"

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

// Instance attributes for per-star data
layout(location = 3) in vec3 a_instancePosition;
layout(location = 4) in vec3 a_instanceColor;
layout(location = 5) in float a_instanceSize;

out vec3 v_color;
out vec3 v_worldPos;
out float v_dopplerShift;  // Doppler shift factor for relativistic color shift

void main() {
    v_color = a_instanceColor;
    v_worldPos = a_instancePosition + a_position * a_instanceSize;
    
    // Calculate Doppler shift for relativistic color effect
    // Direction from camera to star
    vec3 toStar = normalize(v_worldPos - u_cameraPosition.xyz);
    // Relative velocity component towards the star
    float v_dot = dot(u_cameraVelocity, toStar);
    // Relativistic Doppler factor: sqrt((1+v/c)/(1-v/c)) for blueshift when approaching
    // Simplified non-relativistic approximation for visual effect
    float beta = v_dot / 299792458.0;  // speed of light in m/s
    v_dopplerShift = 1.0 + beta;  // Non-relativistic approximation
    
    gl_Position = u_projectionMatrix * u_viewMatrix * vec4(v_worldPos, 1.0);
}