#version 450 core
#extension GL_GOOGLE_include_directive : enable

#include "../common/ubo.glsl"

out vec4 outColor;

in vec2 TexCoords;

uniform float u_mass;           // Black hole mass in geometric units
uniform float u_resolution;     // Screen resolution for ray marching steps
uniform int u_maxSteps;         // Maximum ray march steps
uniform float u_stepSize;       // Step size for ray marching

// Schwarzschild metric constants
const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;

// Calculate the Schwarzschild radius
float schwarzschildRadius(float mass) {
    return 2.0 * mass;
}

// Calculate the deflection angle for light passing at distance r
float deflectionAngle(float r, float mass) {
    float rs = schwarzschildRadius(mass);
    return 4.0 * rs / r;
}

// Ray march through the gravitational field
vec3 rayMarch(vec2 uv, float mass) {
    // Convert UV to screen space coordinates (-1 to 1)
    vec2 screenPos = uv * 2.0 - 1.0;
    
    // Calculate the impact parameter (closest approach distance)
    float impact = length(screenPos);
    
    // Schwarzschild radius
    float rs = schwarzschildRadius(mass);
    
    // Check if we're inside the photon sphere (r = 1.5 * rs)
    if (impact < 1.5 * rs) {
        // Light would be captured - return black
        return vec3(0.0);
    }
    
    // Calculate the deflection angle
    float theta = atan(screenPos.y, screenPos.x);
    float deflect = deflectionAngle(impact, mass);
    
    // Sample the environment in the deflected direction
    vec2 sampleUV = vec2(
        cos(theta + deflect) * 0.5 + 0.5,
        sin(theta + deflect) * 0.5 + 0.5
    );
    
    // Sample the background (this would be the environment texture in full implementation)
    // For now, we'll create a procedural starfield
    vec3 background = vec3(0.02, 0.04, 0.08);  // Dark space blue
    
    // Add stars
    float starPattern = sin(sampleUV.x * 100.0) * cos(sampleUV.y * 100.0);
    if (starPattern > 0.9) {
        background = vec3(1.0, 1.0, 1.0);  // White stars
    }
    
    // Add accretion disk glow
    float diskRadius = 0.1;
    float diskFalloff = smoothstep(diskRadius, 0.0, impact);
    background += vec3(0.8, 0.4, 0.1) * diskFalloff * 0.5;
    
    return background;
}

// Alternative: Full raymarching implementation
vec3 rayMarchFull(vec2 uv, float mass) {
    vec2 screenPos = uv * 2.0 - 1.0;
    float screenDist = length(screenPos);
    
    // Schwarzschild radius
    float rs = schwarzschildRadius(mass);
    
    // Skip if inside photon sphere
    if (screenDist < 1.5 * rs) {
        return vec3(0.05, 0.05, 0.1);  // Dark region near event horizon
    }
    
    // Ray marching parameters
    int steps = int(u_resolution / 10.0);
    if (steps > u_maxSteps) steps = u_maxSteps;
    
    vec2 delta = (uv * 2.0 - 1.0) / float(steps);
    vec2 pos = uv * 2.0 - 1.0;
    
    vec3 accumulatedColor = vec3(0.0);
    float accumulatedWeight = 0.0;
    
    for (int i = 0; i < 100; i++) {
        if (i >= steps) break;
        
        float r = length(pos);
        
        // Skip if inside event horizon
        if (r < rs) {
            accumulatedColor = vec3(0.0);
            break;
        }
        
        // Calculate deflection
        float deflection = deflectionAngle(r, mass);
        
        // Sample background
        vec2 sampleUV = vec2(
            atan(pos.y, pos.x) + deflection,
            log(r / rs) / log(10.0)
        );
        sampleUV = mod(sampleUV / PI, 1.0);
        
        // Procedural background
        vec3 bg = vec3(0.02, 0.04, 0.08);
        
        // Add stars
        float stars = sin(sampleUV.x * 123.3) * cos(sampleUV.y * 456.7);
        if (stars > 0.95) {
            bg = vec3(1.0);
        }
        
        // Weight based on distance from horizon
        float weight = 1.0 / (r * r);
        accumulatedColor += bg * weight;
        accumulatedWeight += weight;
        
        pos += delta;
    }
    
    if (accumulatedWeight > 0.0) {
        accumulatedColor /= accumulatedWeight;
    }
    
    return accumulatedColor;
}

void main() {
    // Use the simpler ray march for performance
    vec3 color = rayMarch(TexCoords, u_mass);
    
    // Add event horizon
    float rs = schwarzschildRadius(u_mass);
    vec2 center = vec2(0.5);
    float dist = distance(TexCoords, center);
    
    // Smooth event horizon
    float horizon = smoothstep(rs * 0.5, rs * 0.4, dist);
    color = mix(color, vec3(0.0), horizon);
    
    // Add lensing ring (photon ring)
    float ring = smoothstep(rs * 0.95, rs * 1.05, dist);
    color += vec3(0.3, 0.3, 0.4) * ring * 0.3;
    
    outColor = vec4(color, 1.0);
}
