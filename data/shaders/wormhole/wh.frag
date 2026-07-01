#version 450 core

#include "../common/ubo.glsl"

in vec3 v_worldPos;
in float v_radius;

out vec4 FragColor;

uniform float u_time;
uniform float u_throatRadius;

void main() {
    // Wormhole: translucent blue/green glow with noise
    float noise = sin(v_radius * 10.0 + u_time * 2.0) * cos(v_radius * 5.0 - u_time);
    float alpha = 0.3 + 0.2 * sin(u_time * 3.0);
    
    vec3 color = vec3(0.2, 0.6, 1.0);
    color += noise * 0.2;
    
    FragColor = vec4(color, alpha);
}