#version 450 core

#include "../common/ubo.glsl"

// Vertex attributes
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

// Instance attributes for per-object data
// Layout: model(16) + color(3) + metallic(1) + roughness(1) + emissive(3) + texLayer(1) = 25 floats
layout(location = 4) in mat4 aModelMatrix;
layout(location = 8) in vec3 aColor;
layout(location = 9) in float aMetallic;
layout(location = 10) in float aRoughness;
layout(location = 11) in vec3 aEmissive;
layout(location = 12) in float aTexLayer;

out VS_OUT {
    vec3 worldPos;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec2 texCoord;
    vec3 viewDir;
    vec3 color;
    float metallic;
    float roughness;
    vec3 emissive;
    flat float texLayer;
} vs_out;

uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;
uniform float u_scaleFactor;  // Converts meters to scene units

void main() {
    // Transform position to world space
    vec4 worldPos4 = aModelMatrix * vec4(aPos * aScaleFactor, 1.0);
    vs_out.worldPos = worldPos4.xyz;
    
    // Normal, tangent, bitangent in world space
    mat3 normalMatrix = mat3(aModelMatrix);
    vs_out.normal = normalize(normalMatrix * aNormal);
    vs_out.tangent = normalize(normalMatrix * aTangent);
    vs_out.bitangent = normalize(normalMatrix * aBitangent);
    
    vs_out.texCoord = aTexCoord;
    vs_out.color = aColor;
    vs_out.metallic = aMetallic;
    vs_out.roughness = aRoughness;
    vs_out.emissive = aEmissive;
    vs_out.texLayer = aTexLayer;
    
    // View direction
    vec4 viewPos = u_viewMatrix * worldPos4;
    vs_out.viewDir = normalize(-viewPos.xyz);
    
    gl_Position = u_projectionMatrix * viewPos;
}