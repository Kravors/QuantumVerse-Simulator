#version 450 core

#include "../common/ubo.glsl"

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_modelMatrix;
uniform float u_time;

out vec3 v_normal;
out vec3 v_worldPos;

void main() {
    v_normal = mat3(u_modelMatrix) * a_normal;
    v_worldPos = vec3(u_modelMatrix * vec4(a_position, 1.0));
    
    gl_Position = u_projectionMatrix * u_viewMatrix * vec4(v_worldPos, 1.0);
}