#version 450 core
#extension GL_GOOGLE_include_directive : enable

#include "../common/ubo.glsl"

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_modelMatrix;
uniform float u_throatRadius;

out vec3 v_worldPos;
out float v_radius;

void main() {
    v_worldPos = vec3(u_modelMatrix * vec4(a_position, 1.0));
    v_radius = length(a_position);
    
    gl_Position = u_projectionMatrix * u_viewMatrix * vec4(v_worldPos, 1.0);
}

