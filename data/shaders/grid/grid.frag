#version 450 core
#extension GL_GOOGLE_include_directive : enable

#include "../common/ubo.glsl"

in vec3 v_normal;
in vec3 v_worldPos;
in float v_curvature;

out vec4 FragColor;

void main() {
    // Grid lines with curvature-based coloring
    vec3 color = vec3(0.2, 0.6, 1.0);
    
    // Add curvature visualization
    color.r += v_curvature * 0.5;
    color.g -= v_curvature * 0.3;
    
    // Simple lighting
    vec3 lightDir = normalize(u_sunDirection);
    float diff = max(dot(normalize(v_normal), lightDir), 0.0);
    vec3 ambient = 0.3 * color;
    vec3 diffuse = diff * color;
    
    FragColor = vec4(ambient + diffuse, 0.8);
}
