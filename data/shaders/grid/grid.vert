#version 450 core
#extension GL_GOOGLE_include_directive : enable

// Include UBO definitions
#include "../common/ubo.glsl"

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

out vec3 v_normal;
out vec3 v_worldPos;
out float v_curvature;

uniform mat4 u_modelMatrix;

void main() {
    v_normal = mat3(u_modelMatrix) * a_normal;
    v_worldPos = vec3(u_modelMatrix * vec4(a_position, 1.0));
    
    // Simple displacement based on time for visualization
    vec3 displacedPos = a_position;
    displacedPos.y += 0.1 * sin(a_position.x * 2.0 + u_time) * cos(a_position.z * 2.0 + u_time);
    
    gl_Position = u_projectionMatrix * u_viewMatrix * vec4(displacedPos, 1.0);
    v_curvature = abs(displacedPos.y);
}

