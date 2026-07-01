#version 450 core

#include "../common/ubo.glsl"

in VS_OUT {
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
} fs_in;

out vec4 outColor;

// Material properties from UBO
uniform MaterialUBO u_material;
uniform IBLUBO u_ibl;

// Textures
uniform int textureArrayEnabled;
uniform sampler2DArray textureArray;
uniform samplerCube u_environmentMap;
uniform sampler2D u_brdfLUT;

// Light properties from UBO
// LightUBO is already included in ubo.glsl

// Constants
const float PI = 3.14159265359;

// Helper functions
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float num = a2;
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// Image-Based Lighting
vec3 calculateIBL(vec3 N, vec3 V, vec3 albedo, float metallic, float roughness) {
    vec3 F = vec3(0.04);
    F = fresnelSchlick(max(dot(N, V), 0.0), F);
    
    vec3 R = reflect(-V, N);
    vec3 irradiance = texture(u_environmentMap, N).rgb;
    vec3 diffuse = irradiance * albedo;
    
    vec3 specular = texture(u_brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rgb;
    vec3 specularReflection = specular * F * u_material.metallic;
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    return (kD * diffuse + specularReflection) * u_ibl.u_iblIntensity;
}

void main() {
    // Get material properties
    vec3 albedo = fs_in.color * u_material.u_albedo;
    float metallic = u_material.u_metallic;
    float roughness = u_material.u_roughness;
    float emissive = u_material.u_emissive;
    float normalScale = u_material.u_normalScale;
    
    // Sample texture if enabled
    vec3 texColor = vec3(1.0);
    if (textureArrayEnabled > 0 && fs_in.texLayer >= 0.0) {
        texColor = texture(textureArray, vec3(fs_in.texCoord, fs_in.texLayer)).rgb;
    }
    albedo *= texColor;
    
    // Apply gamma correction to albedo
    albedo = pow(albedo, vec3(2.2));
    
    // Normal normalization
    vec3 N = normalize(fs_in.normal);
    vec3 V = normalize(fs_in.viewDir);
    
    // Cook-Torrance PBR lighting
    vec3 lightDir = normalize(u_sunDirection);
    vec3 L = lightDir;
    vec3 H = normalize(V + L);
    
    // Calculate BRDF components
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 numerator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    vec3 specular = numerator / denominator;
    
    // Final lighting equation
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 directLighting = (kD * albedo / PI + specular) * u_sunColor * u_sunIntensity * NdotL;
    
    // Add IBL
    vec3 ibl = vec3(0.0);
    if (u_ibl.u_iblEnabled > 0.5) {
        ibl = calculateIBL(N, V, albedo, metallic, roughness);
    }
    
    // Combine all lighting
    vec3 result = directLighting + emissive * fs_in.emissive + ibl;
    
    // Auto-exposure (simplified)
    result *= u_exposure;
    
    // Gamma correction
    result = pow(result, vec3(1.0 / u_gamma));
    
    outColor = vec4(result, 1.0);
}