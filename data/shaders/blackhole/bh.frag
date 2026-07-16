#version 450 core
#extension GL_GOOGLE_include_directive : enable

#include "../common/ubo.glsl"

in vec3 v_normal;
in vec3 v_worldPos;

out vec4 FragColor;

uniform float u_mass;

void main() {
    // Black hole: dark sphere with accretion disk effect
    vec3 color = vec3(0.1, 0.1, 0.2);
    
    // Add some subtle glow at the edges
    vec3 normal = normalize(v_normal);
    vec3 viewDir = normalize(-v_worldPos);
    float edge = 1.0 - max(dot(normal, viewDir), 0.0);
    color += edge * 0.3 * sin(u_time * 2.0 + length(v_worldPos));
    
    FragColor = vec4(color, 0.9);
}
